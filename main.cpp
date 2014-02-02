/*
 * This file is part of Playslave-C++.
 * Playslave-C++ is licenced under MIT License. See LICENSE.txt for more details.
 */

#include <thread>
#include <chrono>
#include <iostream>

#include "cmd.h"

#include "cuppa/io.h"

#include "constants.h"		/* LOOP_NSECS */
#include "messages.h"		/* MSG_xyz */
#include "player.h"

static std::string DeviceId(int argc, char *argv[]);
static void MainLoop(Player &player);
static void ListOutputDevices();
static void RegisterListeners(Player &p);

/* Names of the states in enum state. */
const static std::unordered_map<State, std::string> STATES = {
	{ State::EJECTED, "Ejected" },
	{ State::STOPPED, "Stopped" },
	{ State::PLAYING, "Playing" },
	{ State::QUITTING, "Quitting" }
};


/* The main entry point. */
int
main(int argc, char *argv[])
{
	int	exit_code = EXIT_SUCCESS;

	try {
		audio::InitialiseLibraries();

		Player p(DeviceId(argc, argv));
		RegisterListeners(p);
		MainLoop(p);
	} catch (enum error) {
		std::cerr << "Unhandled exception caught, going away now.";
		exit_code = EXIT_FAILURE;
	}

	audio::CleanupLibraries();

	return exit_code;
}

static void RegisterListeners(Player &p)
{
	p.RegisterPositionListener([](uint64_t position) { response(R_TIME, "%u", position); }, TIME_USECS);
	p.RegisterStateListener([](State old_state, State new_state) {
		response(R_STAT, "%s %s", STATES.at(old_state).c_str(), STATES.at(new_state).c_str());
	});
}

static void
MainLoop(Player &p)
{
	/* Set of commands that can be performed on the player. */
	command_set PLAYER_CMDS = {
		/* Nullary commands */
		{ "play", [&](const cmd_words &) { return p.Play(); } },
		{ "stop", [&](const cmd_words &) { return p.Stop(); } },
		{ "ejct", [&](const cmd_words &) { return p.Eject(); } },
		{ "quit", [&](const cmd_words &) { return p.Quit(); } },
		/* Unary commands */
		{ "load", [&](const cmd_words &words) { return p.Load(words[1]); } },
		{ "seek", [&](const cmd_words &words) { return p.Seek(words[1]); } }
	};

	response(R_OHAI, "%s", MSG_OHAI);	/* Say hello */

	while (p.CurrentState() != State::QUITTING) {
		/*
		* Possible Improvement: separate command checking and player
		* updating into two threads.  Player updating is quite
		* intensive and thus impairs the command checking latency.
		* Do this if it doesn't make the code too complex.
		*/
		check_commands(PLAYER_CMDS);
		/* TODO: Check to see if err was fatal */
		p.Update();

		std::this_thread::sleep_for(std::chrono::nanoseconds(LOOP_NSECS));
	}

	response(R_TTFN, "%s", MSG_TTFN);	/* Wave goodbye */
}

/* Tries to parse the device ID. */
static std::string DeviceId(int argc, char *argv[])
{
	std::string device = "";

	/*
	 * Possible Improvement: This is rather dodgy code for getting the
	 * device ID out of the command line arguments, maybe make it a bit
	 * more robust.
	 */
	if (argc < 2) {
		ListOutputDevices();
		throw error(E_BAD_CONFIG, MSG_DEV_NOID);
	} else {
		device = std::string(argv[1]);
	}

	return device;
}

static void ListOutputDevices()
{
	for (auto device : audio::ListDevices()) {
		std::cout << device.first << ": " << device.second << std::endl;
	}
}
