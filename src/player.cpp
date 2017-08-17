// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file player.cpp
 * Main implementation file for the Player class.
 * @see player.h
 */

#include <cassert>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "audio/audio.h"
#include "audio/audio_sink.h"
#include "audio/audio_source.h"
#include "errors.h"
#include "messages.h"
#include "player.h"
#include "response.h"

using namespace std::chrono;

Player::Player(int device_id, SinkFn sink, std::map<std::string, SourceFn> sources)
    : device_id(device_id),
      sink(std::move(sink)),
      sources(std::move(sources)),
      file(std::make_unique<Null_audio>()),
      dead(false),
      io(nullptr),
      last_pos(0)
{
}

void Player::SetIo(Response_sink &io)
{
	this->io = &io;
}

bool Player::Update()
{
	assert(this->file != nullptr);
	auto as = this->file->Update();

	if (as == Audio::State::at_end) this->End(Response::NOREQUEST);
	if (as == Audio::State::playing) {
		// Since the audio is currently playing, the position may have
		// advanced since last update.  So we need to update it.
		auto pos = this->file->Position();
		if (this->CanBroadcastPos(pos))
			this->BroadcastPos(Response::NOREQUEST, pos);
	}

	return !this->dead;
}

//
// Commands
//

Response Player::Dump(size_t id, const std::string &tag) const
{
	if (this->dead) return Response::Failure(tag, MSG_CMD_PLAYER_CLOSING);

	this->DumpState(id, tag);

	// This information won't exist if there is no file.
	if (this->file->CurrentState() != Audio::State::none) {
		auto file = this->file->File();
		this->Respond(
		        id, Response(tag, Response::Code::fload).AddArg(file));

		auto pos = this->file->Position();
		this->AnnouncePos(id, tag, pos);
	}

	return Response::Success(tag);
}

Response Player::Eject(const std::string &tag)
{
	if (this->dead) return Response::Failure(tag, MSG_CMD_PLAYER_CLOSING);

	// Silently ignore ejects on ejected files.
	// Concurrently speaking, this should be fine, as we are the only
	// thread that can eject or un-eject files.
	if (this->file->CurrentState() == Audio::State::none) {
		return Response::Success(tag);
	}

	assert(this->file != nullptr);
	this->file = std::make_unique<Null_audio>();

	this->DumpState(0, tag);

	return Response::Success(tag);
}

Response Player::End(const std::string &tag)
{
	if (this->dead) return Response::Failure(tag, MSG_CMD_PLAYER_CLOSING);

	// Let upstream know that the file ended by itself.
	// This is needed for auto-advancing playlists, etc.
	this->Respond(0, Response(Response::NOREQUEST, Response::Code::end));

	this->SetPlaying(tag, false);

	// Rewind the file back to the start.  We can't use Player::Pos() here
	// in case End() is called from Pos(); a seek failure could start an
	// infinite loop.
	this->PosRaw(Response::NOREQUEST, microseconds{0});

	return Response::Success(tag);
}

Response Player::Load(const std::string &tag, const std::string &path)
{
	if (this->dead) return Response::Failure(tag, MSG_CMD_PLAYER_CLOSING);
	if (path.empty()) return Response::Invalid(tag, MSG_LOAD_EMPTY_PATH);

	assert(this->file != nullptr);

	// Bin the current file as soon as possible.
	// This ensures that we don't have any situations where two files are
	// contending over resources, or the current file spends a second or
	// two flushing its remaining audio.
	this->Eject(Response::NOREQUEST);

	try {
		this->file = this->LoadRaw(path);
	} catch (File_error &e) {
		// File errors aren't fatal, so catch them here.
		return Response::Failure(tag, e.Message());
	}

	assert(this->file != nullptr);
	this->last_pos = seconds{0};

	// A load will change all of the player's state in one go,
	// so just send a Dump() instead of writing out all of the responses
	// here.
	// Don't take the response from here, though, because it has the wrong
	// tag.
	this->Dump(0, Response::NOREQUEST);

	return Response::Success(tag);
}

Response Player::Pos(const std::string &tag, const std::string &pos_str)
{
	if (this->dead) return Response::Failure(tag, MSG_CMD_PLAYER_CLOSING);

	microseconds pos{0};
	try {
		pos = PosParse(pos_str);
	} catch (Seek_error &e) {
		// Seek errors here are a result of clients sending weird times.
		// Thus, we tell them off.
		return Response::Invalid(tag, e.Message());
	}

	try {
		this->PosRaw(tag, pos);
	} catch (Null_audio_error) {
		return Response::Invalid(tag, MSG_CMD_NEEDS_LOADED);
	} catch (Seek_error) {
		// Seek failures here are a result of the decoder not liking the
		// seek position (usually because it's outside the audio file!).
		// Thus, unlike above, we try to recover.

		Debug() << "Seek failure" << std::endl;

		// Make it look to the client as if the seek ran off the end of
		// the file.
		this->End(tag);
	}

	// If we've made it all the way down here, we deserve to succeed.
	return Response::Success(tag);
}

Response Player::SetPlaying(const std::string &tag, bool playing)
{
	if (this->dead) return Response::Failure(tag, MSG_CMD_PLAYER_CLOSING);

	// Why is SetPlaying not split between Start() and Stop()?, I hear the
	// best practices purists amongst you say.  Quite simply, there is a
	// large amount of fiddly exception boilerplate here that would
	// otherwise be duplicated between the two methods.

	assert(this->file != nullptr);

	try {
		this->file->SetPlaying(playing);
	} catch (Null_audio_error &e) {
		return Response::Invalid(tag, e.Message());
	}

	this->DumpState(0, Response::NOREQUEST);

	return Response::Success(tag);
}

Response Player::Quit(const std::string &tag)
{
	if (this->dead) return Response::Failure(tag, MSG_CMD_PLAYER_CLOSING);

	this->Eject(tag);
	this->dead = true;
	return Response::Success(tag);
}

//
// Command implementations
//

/* static */ microseconds Player::PosParse(const std::string &pos_str)
{
	size_t cpos = 0;

	// Try and see if this position string is negative.
	// Cheap and easy way: see if it has '-'.
	// This means we don't need to skip whitespace first, with no loss
	// of suction: no valid position string will contain '-'.
	if (pos_str.find('-') != std::string::npos) {
		throw Seek_error(MSG_SEEK_INVALID_VALUE);
	}

	std::uint64_t pos;
	try {
		pos = std::stoull(pos_str, &cpos);
	} catch (...) {
		throw Seek_error(MSG_SEEK_INVALID_VALUE);
	}

	// cpos will point to the first character in pos that wasn't a number.
	// We don't want any such characters here, so bail if the position isn't
	// at the end of the string.
	auto sl = pos_str.length();
	if (cpos != sl) throw Seek_error(MSG_SEEK_INVALID_VALUE);

	return microseconds{pos};
}

void Player::PosRaw(const std::string &tag, microseconds pos)
{
	Expects(this->file != nullptr);

	this->file->SetPosition(pos);
	this->BroadcastPos(tag, pos);
}

void Player::DumpState(size_t id, const std::string &tag) const
{
	Response::Code code = Response::Code::eject;

	switch (this->file->CurrentState()) {
		case Audio::State::at_end:
			code = Response::Code::end;
			break;
		case Audio::State::none:
			code = Response::Code::eject;
			break;
		case Audio::State::playing:
			code = Response::Code::play;
			break;
		case Audio::State::stopped:
			code = Response::Code::stop;
			break;
		default:
			// Just don't dump anything in this case.
			return;
	}

	this->Respond(id, Response(tag, code));
}

void Player::Respond(int id, Response rs) const
{
	if (this->io != nullptr) this->io->Respond(id, rs);
}

void Player::AnnouncePos(int id, const std::string &tag, microseconds pos) const
{
	this->Respond(id, Response(tag, Response::Code::pos)
	                          .AddArg(std::to_string(pos.count())));
}

bool Player::CanBroadcastPos(microseconds pos) const
{
	// Because last_pos is counted in seconds, this condition becomes
	// true whenever pos 'ticks over' to a different second from last_pos.
	// The result is that we broadcast at most roughly once a second.
	return this->last_pos < duration_cast<seconds>(pos);
}

void Player::BroadcastPos(const std::string &tag, microseconds pos)
{
	// This ensures we don't broadcast too often:
	// see CanBroadcastPos.
	this->last_pos = duration_cast<seconds>(pos);
	this->AnnouncePos(0, tag, pos);
}

std::unique_ptr<Audio> Player::LoadRaw(const std::string &path) const
{
	std::unique_ptr<Audio_source> source = this->LoadSource(path);
	assert(source != nullptr);

	auto sink = this->sink(*source, this->device_id);
	return std::make_unique<Basic_audio>(std::move(source), std::move(sink));
}

std::unique_ptr<Audio_source> Player::LoadSource(const std::string &path) const
{
	size_t extpoint = path.find_last_of('.');
	std::string ext = path.substr(extpoint + 1);

	auto ibuilder = this->sources.find(ext);
	if (ibuilder == this->sources.end()) {
		throw File_error("Unknown file format: " + ext);
	}

	return (ibuilder->second)(path);
}
