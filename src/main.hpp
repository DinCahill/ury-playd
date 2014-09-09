// This file is part of playd.
// playd is licenced under the MIT license: see LICENSE.txt.

/**
 * @file
 * Declaration of the playd class.
 * @see main.cpp
 */

#ifndef PS_MAIN_HPP
#define PS_MAIN_HPP

#include <chrono>
#include "audio/audio_system.hpp"
#include "cmd.hpp"
#include "io/io_reactor.hpp"
#include "player/player.hpp"
#include "time_parser.hpp"

/**
 * The playd application.
 *
 * This class contains all the state required by playd, with the exception
 * of that introduced by external C libraries.  It is a RAII class, so
 * constructing playd will load playd's library dependencies, and
 * destructing it will unload them.  It is probably not safe to create more than
 * one playd.
 */
class playd {
public:
	/**
	 * Constructs a playd, initialising its libraries.
	 * @param argc The argument count from the main function.
	 * @param argv The argument vector from the main function.
	 */
	playd(int argc, char *argv[]);

	/**
	 * Runs playd.
	 * @return The exit code, which may be returned by the program.
	 */
	int Run();

private:
	/// The period between position announcements from the Player object.
	static const std::chrono::microseconds POSITION_PERIOD;

	std::vector<std::string> arguments; ///< The argument vector.
	AudioSystem audio;                  ///< The audio subsystem.
	Player player;                      ///< The player subsystem.
	CommandHandler handler;             ///< The command handler.
	Player::TP time_parser;             ///< The seek time parser.
	std::unique_ptr<IoReactor> io;      ///< The I/O handler.

	/**
	 * Tries to get the output device ID from program arguments.
	 * @return The device ID, -1 if invalid selection (or none).
	 */
	int GetDeviceID();

	/**
	 * Lists on stdout all sound devices to which the audio output may
	 * connect.
	 * This is mainly for the benefit of the end user.
	 */
	void ListOutputDevices();

	/**
	 * Registers the playd command set on the given Player.
	 * @param p The Player on which the commands will act.
	 */
	void RegisterCommands(Player *p);
};

#endif // PS_MAIN_HPP