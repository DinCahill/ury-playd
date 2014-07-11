// This file is part of Playslave-C++.
// Playslave-C++ is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the Playslave class.
 * @see main.cpp
 */

#ifndef PS_MAIN_HPP
#define PS_MAIN_HPP

#include "audio/audio_system.hpp" // AudioSystem
#include "cmd.hpp"                // CommandHandler
#include "io/io_reactor.hpp"      // IoReactor
#include "player/player.hpp"      // Player
#include "time_parser.hpp"        // TimeParser

/**
 * The Playslave++ application.
 *
 * This class contains all the state required by Playslave, with the exception
 * of that introduced by external C libraries.  It is a RAII class, so
 * constructing Playslave will load Playslave's library dependencies, and
 * destructing it will unload them.  It is probably not safe to create more than
 * one Playslave.
 */
class Playslave {
public:
	/**
	 * Constructs a Playslave, initialising its libraries.
	 * @param argc  The argument count from the main function.
	 * @param argv  The argument vector from the main function.
	 */
	Playslave(int argc, char *argv[]);

	/**
	 * Runs Playslave.
	 * @return The exit code, which may be returned by the program.
	 */
	int Run();

private:
	std::vector<std::string> arguments; ///< The argument vector.
	AudioSystem audio;                  ///< The audio subsystem.

	Player player;                      ///< The player subsystem.
	CommandHandler handler;             ///< The command handler.
	Player::TP time_parser;             ///< The seek time parser.
	std::unique_ptr<IoReactor> io;           ///< The I/O handler.

	/**
	 * Tries to get the output device ID from stdin.
	 * If there is no stdin, the program lists the available devices and
	 * dies.
	 * @param system The audio system.
	 * @param argc The program argument count (from main()).
	 * @param argv The program argument vector (from main()).
	 * @return The device ID, as a string.
	 */
	std::string DeviceID();

	/**
	 * Lists on stdout all sound devices to which the audio output may
	 * connect.
	 * This is mainly for the benefit of the end user.
	 */
	void ListOutputDevices();

	/**
	 * Performs the Playslave's main loop.
	 */
	void MainLoop();

	/**
	 * Registers various listeners with the Player.
	 * This is so time and state changes can be sent out on stdout.
	 */
	void RegisterListeners();

	/**
	 * Registers the Playslave command set on the given Player.
	 * @param p The Player on which the commands will act.
	 */
	void RegisterCommands(Player *p);
};

#endif // PS_MAIN_HPP
