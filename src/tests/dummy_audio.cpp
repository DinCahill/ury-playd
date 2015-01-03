// This file is part of playd.
// playd is licensed under the MIT licence: see LICENCE.txt.

/**
 * @file
 * Implementation of DummyAudio and related classes.
 */

#include <cstdint>
#include <string>
#include <vector>

#include "../audio/audio.hpp"
#include "../audio/audio_system.hpp"
#include "../io/io_response.hpp"
#include "dummy_audio.hpp"

//
// DummyAudio
//

DummyAudio::DummyAudio(DummyAudioSystem &sys) : sys(sys)
{
}

void DummyAudio::Emit(std::initializer_list<Response::Code> codes, const ResponseSink *sink)
{
	if (sink == nullptr) return;

	for (auto &code : codes) {
		auto r = Response(code);

		if (code == Response::Code::STATE) {
			auto playing = this->sys.started;
			r.Arg(playing ? "Playing" : "Stopped");
		} else if (code == Response::Code::FILE) {
			r.Arg(this->sys.path);
		} else {
			continue;
		}

		sink->Respond(r);
	}
}

void DummyAudio::SetPlaying(bool playing)
{
	this->sys.started = playing;
}

Audio::State DummyAudio::Update()
{
	return this->sys.state;
}

std::uint64_t DummyAudio::Position() const
{
	return this->sys.pos;
}

void DummyAudio::Seek(std::uint64_t position)
{
	this->sys.pos = position;
}

//
// DummyAudioSystem
//

DummyAudioSystem::DummyAudioSystem() : path(""), pos(0), state(Audio::State::STOPPED)
{
}

std::unique_ptr<Audio> DummyAudioSystem::Null() const
{
	return std::unique_ptr<Audio>(new NoAudio());
}

std::unique_ptr<Audio> DummyAudioSystem::Load(const std::string &path) const
{
	// Kids, don't try this at home.
	// Were this not a test mock, I'd shoot myself for this!  ~ Matt
	DummyAudioSystem &notconst = const_cast<DummyAudioSystem &>(*this);
	notconst.path = path;
	return std::unique_ptr<Audio>(new DummyAudio(notconst));
}
