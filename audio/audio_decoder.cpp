// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Implementation of the AudioDecoder class.
 * @see audio/audio_decoder.hpp
 */

#include <functional>
#include <string>
#include <memory>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <map>

/* ffmpeg */
extern "C" {
#ifdef WIN32
#define inline __inline
#endif
#include <libavcodec/avcodec.h>
#include <libavcodec/version.h> /* For old version patchups */
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

#include "../errors.hpp"
#include "../constants.h"
#include "../messages.h"
#include "../sample_formats.hpp"

#include "audio_decoder.hpp"
#include "audio_resample.hpp"

AudioDecoder::AudioDecoder(const std::string &path)
{
	this->buffer = std::unique_ptr<unsigned char[]>(
	                new unsigned char[BUFFER_SIZE]);

	Open(path);
	InitialiseStream();
	InitialisePacket();
	InitialiseFrame();
	InitialiseResampler();

	Debug("stream id:", this->stream_id);
	Debug("codec:", this->stream->codec->codec->long_name);
}

AudioDecoder::~AudioDecoder()
{
}

/* @return The number of channels this decoder outputs. */
std::uint8_t AudioDecoder::ChannelCount() const
{
	return this->stream->codec->channels;
}

/* @return The size of this decoder's buffer, in samples. */
size_t AudioDecoder::BufferSampleCapacity() const
{
	return SampleCountForByteCount(BUFFER_SIZE);
}

/* @return The sample rate. */
double AudioDecoder::SampleRate() const
{
	return (double)this->stream->codec->sample_rate;
}

/* Converts stream position (in microseconds) to estimated sample count. */
std::uint64_t AudioDecoder::SampleCountForPositionMicroseconds(
                std::chrono::microseconds usec) const
{
	auto sample_micros = usec * SampleRate();
	return std::chrono::duration_cast<std::chrono::seconds>(sample_micros)
	                .count();
}

/* Converts sample count to estimated stream position (in microseconds). */
std::chrono::microseconds AudioDecoder::PositionMicrosecondsForSampleCount(
                std::uint64_t samples) const
{
	auto position_secs = std::chrono::seconds(samples) / SampleRate();
	return std::chrono::duration_cast<std::chrono::microseconds>(
	                position_secs);
}

/* Converts buffer size (in bytes) to sample count (in samples). */
std::uint64_t AudioDecoder::SampleCountForByteCount(std::uint64_t bytes) const
{
	return (bytes / ChannelCount()) / BytesPerSample();
}

/* Converts sample count (in samples) to buffer size (in bytes). */
std::uint64_t AudioDecoder::ByteCountForSampleCount(std::uint64_t samples) const
{
	return (samples * ChannelCount()) * BytesPerSample();
}

/* Returns the current number of bytes per sample. */
size_t AudioDecoder::BytesPerSample() const
{
	return av_get_bytes_per_sample(this->resampler->AVOutputFormat());
}

/* Attempts to seek to the position 'usec' milliseconds into the file. */
void AudioDecoder::SeekToPositionMicroseconds(
                std::chrono::microseconds position)
{
	std::int64_t ffmpeg_position = AvPositionFromMicroseconds(position);

	Debug("Seeking to:", ffmpeg_position);

	if (av_seek_frame(this->context.get(), this->stream_id, ffmpeg_position,
	                  AVSEEK_FLAG_ANY) != 0) {
		throw InternalError(MSG_SEEK_FAIL);
	}
}

std::int64_t AudioDecoder::AvPositionFromMicroseconds(
                std::chrono::microseconds position)
{
	auto position_timebase_microseconds =
	                (position * this->stream->time_base.den) /
	                this->stream->time_base.num;
	auto position_timebase_seconds =
	                std::chrono::duration_cast<std::chrono::seconds>(
	                                position_timebase_microseconds);
	return position_timebase_seconds.count();
}

/* Tries to decode an entire frame and returns a vector of its contents.
 *
 * If successful, returns a pointer to the resulting vector of decoded data,
 * which is owned by the caller.  If the return value is nullptr, we have run
 * out of frames to decode.
 */
std::vector<char> AudioDecoder::Decode()
{
	bool complete = false;
	bool more = true;
	std::vector<char> vec;

	while (!complete && more) {
		if (av_read_frame(this->context.get(), this->packet.get()) <
		    0) {
			more = false;
		} else if (this->packet->stream_index == this->stream_id) {
			complete = DecodePacket();
			if (complete) {
				vec = Resample();
			}
		}
	}

	return vec;
}

std::vector<char> AudioDecoder::Resample()
{
	return this->resampler->Resample(this->frame.get());
}

static std::map<AVSampleFormat, SampleFormat> sf_from_av = {
                {AV_SAMPLE_FMT_U8, SampleFormat::PACKED_UNSIGNED_INT_8},
                {AV_SAMPLE_FMT_S16, SampleFormat::PACKED_SIGNED_INT_16},
                {AV_SAMPLE_FMT_S32, SampleFormat::PACKED_SIGNED_INT_32},
                {AV_SAMPLE_FMT_FLT, SampleFormat::PACKED_FLOAT_32}};

/**
 * @return The sample format of the data returned by this decoder.
 */
SampleFormat AudioDecoder::OutputSampleFormat() const
{
	try
	{
		return sf_from_av.at(this->resampler->AVOutputFormat());
	}
	catch (std::out_of_range)
	{
		throw FileError(MSG_DECODE_BADRATE);
	}
}

void AudioDecoder::Open(const std::string &path)
{
	AVFormatContext *ctx = nullptr;

	if (avformat_open_input(&ctx, path.c_str(), NULL, NULL) < 0) {
		std::ostringstream os;
		os << "couldn't open " << path;
		throw FileError(os.str());
	}

	auto free_context = [](AVFormatContext *ctx) {
		avformat_close_input(&ctx);
	};
	this->context = std::unique_ptr<AVFormatContext,
	                                decltype(free_context)>(ctx,
	                                                        free_context);
}

void AudioDecoder::InitialiseStream()
{
	FindStreamInfo();
	FindStreamAndInitialiseCodec();
}

void AudioDecoder::FindStreamInfo()
{
	if (avformat_find_stream_info(this->context.get(), NULL) < 0) {
		throw FileError(MSG_DECODE_NOAUDIO);
	}
}

void AudioDecoder::FindStreamAndInitialiseCodec()
{
	AVCodec *codec;
	int stream = av_find_best_stream(this->context.get(),
	                                 AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);

	if (stream < 0) {
		throw FileError(MSG_DECODE_NOSTREAM);
	}

	InitialiseCodec(stream, codec);
}

void AudioDecoder::InitialiseCodec(int stream, AVCodec *codec)
{
	AVCodecContext *codec_context = this->context->streams[stream]->codec;
	if (avcodec_open2(codec_context, codec, NULL) < 0) {
		throw FileError(MSG_DECODE_NOCODEC);
	}

	this->stream = this->context->streams[stream];
	this->stream_id = stream;
}

void AudioDecoder::InitialiseFrame()
{
	auto frame_deleter = [](AVFrame *frame) { av_frame_free(&frame); };
	this->frame = std::unique_ptr<AVFrame, decltype(frame_deleter)>(
	                av_frame_alloc(), frame_deleter);
	if (this->frame == nullptr) {
		throw std::bad_alloc();
	}
}

void AudioDecoder::InitialisePacket()
{
	auto packet_deleter = [](AVPacket *packet) {
		av_free_packet(packet);
		delete packet;
	};
	this->packet = std::unique_ptr<AVPacket, decltype(packet_deleter)>(
	                new AVPacket, packet_deleter);

	AVPacket *pkt = this->packet.get();
	av_init_packet(pkt);
	pkt->data = this->buffer.get();
	pkt->size = BUFFER_SIZE;
}

void AudioDecoder::InitialiseResampler()
{
	std::function<Resampler *(const SampleByteConverter &,
	                          AVCodecContext *)> rs;
	if (UsingPlanarSampleFormat()) {
		rs = [](const SampleByteConverter &s, AVCodecContext *c) {
			return new PlanarResampler(s, c);
		};
	} else {
		rs = [](const SampleByteConverter &s, AVCodecContext *c) {
			return new PackedResampler(s, c);
		};
	}
	this->resampler = std::unique_ptr<Resampler>(
	                rs(*this, this->stream->codec));
}

bool AudioDecoder::UsingPlanarSampleFormat()
{
	return av_sample_fmt_is_planar(this->stream->codec->sample_fmt);
}

bool AudioDecoder::DecodePacket()
{
	int frame_finished = 0;

	if (avcodec_decode_audio4(this->stream->codec, this->frame.get(),
	                          &frame_finished, this->packet.get()) < 0) {
		throw FileError(MSG_DECODE_FAIL);
	}
	return frame_finished;
}
