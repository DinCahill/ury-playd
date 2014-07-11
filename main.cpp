// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Main entry point and implementation of the Playslave class.
 * @see main.cpp
 */

#include <chrono>
#include <iostream>

#include "constants.h" // POSITION_PERIOD
#include "cmd.hpp"
#include "io/io_responder.hpp"
#include "io/io_reactor.hpp"
#include "io/io_reactor_asio.hpp"
#include "io/io_reactor_std.hpp"
#include "messages.h"
#include "player/player.hpp"
#include "audio/audio_system.hpp"
#include "main.hpp"

/**
 * The main entry point.
 * @param argc Program argument count.
 * @param argv Program argument vector.
 */
int main(int argc, char *argv[])
{
	Playslave ps{argc, argv};
	return ps.Run();
}

void Playslave::ListOutputDevices()
{
	this->audio.OnDevices([](const AudioSystem::Device &device) {
		std::cout << device.first << ": " << device.second << std::endl;
	});
}

std::string Playslave::DeviceID()
{
	std::string device = "";

	// TODO: Perhaps make this section more robust.
	if (this->arguments.size() < 2) {
		ListOutputDevices();
		throw ConfigError(MSG_DEV_NOID);
	} else {
		device = std::string(this->arguments[1]);
	}

	return device;
}

void Playslave::RegisterListeners()
{
	this->player->SetPositionListenerPeriod(POSITION_PERIOD);
	this->player->RegisterPositionListener([this](
	                std::chrono::microseconds position) {
		std::uint64_t p = position.count();
		io->Respond(Response::TIME, p);
	});
	this->player->RegisterStateListener([this](Player::State old_state,
	                                           Player::State new_state) {
		io->Respond(Response::STAT, Player::StateString(old_state),
		            Player::StateString(new_state));
		if (new_state == Player::State::QUITTING) {
			io->End();
		}
	});
}

void Playslave::MainLoop()
{
	io->Run();
}

Playslave::Playslave(int argc, char *argv[]) : audio{}
{
	for (int i = 0; i < argc; i++) {
		this->arguments.push_back(std::string(argv[i]));
	}

	this->time_parser = decltype(
	                this->time_parser) {new Player::TP{Player::TP::UnitMap{
	                {"us", Player::TP::MkTime<std::chrono::microseconds>},
	                {"usec", Player::TP::MkTime<std::chrono::microseconds>},
	                {"usecs",
	                 Player::TP::MkTime<std::chrono::microseconds>},
	                {"ms", Player::TP::MkTime<std::chrono::milliseconds>},
	                {"msec", Player::TP::MkTime<std::chrono::milliseconds>},
	                {"msecs",
	                 Player::TP::MkTime<std::chrono::milliseconds>},
	                {"s", Player::TP::MkTime<std::chrono::seconds>},
	                {"sec", Player::TP::MkTime<std::chrono::seconds>},
	                {"secs", Player::TP::MkTime<std::chrono::seconds>},
	                {"m", Player::TP::MkTime<std::chrono::minutes>},
	                {"min", Player::TP::MkTime<std::chrono::minutes>},
	                {"mins", Player::TP::MkTime<std::chrono::minutes>},
	                {"h", Player::TP::MkTime<std::chrono::hours>},
	                {"hour", Player::TP::MkTime<std::chrono::hours>},
	                {"hours", Player::TP::MkTime<std::chrono::hours>},
	                // Default when there is no unit
	                {"", Player::TP::MkTime<std::chrono::microseconds>}}}};

	this->player = decltype(this->player) {
	                new Player{this->audio, *this->time_parser}};

	CommandHandler *h = new CommandHandler;

	using std::string;

	h->AddNullary("play", [&]() { return this->player->Play(); });
	h->AddNullary("stop", [&]() { return this->player->Stop(); });
	h->AddNullary("ejct", [&]() { return this->player->Eject(); });
	h->AddNullary("quit", [&]() { return this->player->Quit(); });

	h->AddUnary("load", [&](const string &s) { return this->player->Load(s); });
	h->AddUnary("seek", [&](const string &s) { return this->player->Seek(s); });

	this->handler = decltype(this->handler)(h);

	IoReactor *io = nullptr;
	if (this->arguments.size() == 4) {
		io = new AsioTcpIoReactor((*this->player), (*this->handler),
		                       this->arguments.at(2),
		                       this->arguments.at(3));
	} else {
#ifdef _WIN32
		io = new AsioWinIoReactor((*this->player), (*this->handler));
#else
		io = new StdIoReactor((*this->player), (*this->handler));
#endif // _WIN32
	}
	this->io = decltype(this->io)(io);
}

int Playslave::Run()
{
	int exit_code = EXIT_SUCCESS;

	try
	{
		// Don't roll this into the constructor: it'll go out of scope!
		this->audio.SetDeviceID(DeviceID());
		RegisterListeners();
		MainLoop();
	}
	catch (Error &error)
	{
		io->RespondWithError(error);
		Debug("Unhandled exception caught, going away now.");
		exit_code = EXIT_FAILURE;
	}

	return exit_code;
}
