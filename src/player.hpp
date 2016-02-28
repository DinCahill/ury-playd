// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Declaration of the Player class, and associated types.
 * @see player.cpp
 */

#ifndef PLAYD_PLAYER_HPP
#define PLAYD_PLAYER_HPP

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "audio/audio_sink.hpp"
#include "audio/audio_source.hpp"
#include "audio/audio.hpp"
#include "response.hpp"

/**
 * A Player contains a loaded audio file and a command API for manipulating it.
 * @see Audio
 * @see AudioSystem
 */
class Player
{
public:
	/// Type for functions that construct sinks.
	using SinkFn =
	        std::function<std::unique_ptr<AudioSink>(const AudioSource &, int)>;

	/// Type for functions that construct sources.
	using SourceFn =
	        std::function<std::unique_ptr<AudioSource>(const std::string &)>;

	/**
	 * Constructs a Player.
	 * @param device_id The device ID to which sinks shall output.
	 * @param sink The function to be used for building sinks.
	 * @param sources The map of file extensions to functions used for building sources.
	 */
	Player(int device_id, SinkFn sink, std::map<std::string, SourceFn> sources);

	/// Deleted copy constructor.
	Player(const Player &) = delete;

	/// Deleted copy-assignment constructor.
	Player &operator=(const Player &) = delete;

	/**
	 * Sets the ResponseSink to which this Player shall send responses.
	 * This sink shall be the target for WelcomeClient, as well as
	 * any responses generated by RunCommand or Update.
	 * @param io The response sink (invariably the IO system).
	 */
	void SetIo(ResponseSink &io);

	/**
	 * Instructs the Player to perform a cycle of work.
	 * This includes decoding the next frame and responding to commands.
	 * @return Whether the player has more cycles of work to do.
	 */
	bool Update();

	//
	// Commands
	//

	/**
	 * Tells the audio file to start or stop playing.
	 * @param tag The tag of the request calling this command.
	 *   For unsolicited state changes, use Response::NOREQUEST.
	 * @param playing True if playing; false otherwise.
	 * @see Play
	 * @see Stop
	 */
	Response SetPlaying(const std::string &tag, bool playing);

	/**
	 * Dumps the current player state to the given ID.
	 *
	 * @param id The ID of the connection to which the Player should
	 *   route any responses.  For broadcasts, use 0.
	 * @param tag The tag of the request calling this command.
	 *   For unsolicited dumps, use Response::NOREQUEST.
	 * @return The result of dumping, which is always success.
	 */
	Response Dump(size_t id, const std::string &tag) const;

	/**
	 * Ejects the current loaded song, if any.
	 * @param tag The tag of the request calling this command.
	 *   For unsolicited ejects, use Response::NOREQUEST.
	 * @return Whether the ejection succeeded.
	 */
	Response Eject(const std::string &tag);

	/**
	 * Ends a file, stopping and rewinding.
	 * @param tag The tag of the request calling this command.
	 *   For unsolicited ends, use Response::NOREQUEST.
	 * @return Whether the end succeeded.
	 */
	Response End(const std::string &tag);

	/**
	 * Loads a file.
	 * @param tag The tag of the request calling this command.
	 *   For unsolicited loads, use Response::NOREQUEST.
	 * @param path The absolute path to a track to load.
	 * @return Whether the load succeeded.
	 */
	Response Load(const std::string &tag, const std::string &path);

	/**
	 * Seeks to a given position in the current file.
	 * @param tag The tag of the request calling this command.
	 *   For unsolicited seeks, use Response::NOREQUEST.
	 * @param pos_str A string containing a timestamp, in microseconds
	 * @return Whether the seek succeeded.
	 */
	Response Pos(const std::string &tag, const std::string &pos_str);

	/**
	 * Quits playd.
	 * @param tag The tag of the request calling this command.
	 *   For unsolicited quits, use Response::NOREQUEST.
	 * @return Whether the quit succeeded.
	 */
	Response Quit(const std::string &tag);

private:
	int device_id;                           ///< The sink's device ID.
	SinkFn sink;                             ///< The sink create function.
	std::map<std::string, SourceFn> sources; ///< The file formats map.
	std::unique_ptr<Audio> file;             ///< The loaded audio file.
	bool is_running;                         ///< Whether the Player runs.
	const ResponseSink *io;                  ///< The sink for responses.
	std::uint64_t last_pos;                  ///< The last-sent position.

	/**
	 * Parses pos_str as a seek timestamp.
	 * @param pos_str The time string to be parsed.
	 * @return The parsed time.
	 * @exception std::out_of_range
	 *   See http://www.cplusplus.com/reference/string/stoull/#exceptions
	 * @exception std::invalid_argument
	 *   See http://www.cplusplus.com/reference/string/stoull/#exceptions
	 * @exception SeekError
	 *   Raised if checks beyond those done by stoull fail.
	 */
	static std::uint64_t PosParse(const std::string &pos_str);

	/**
	 * Performs an actual seek.
	 * This does not do any EOF handling.
	 * @param tag The tag of the request calling this command.
	 *   For unsolicited seeks, use Response::NOREQUEST.
	 * @param pos The new position, in microseconds.
	 * @exception SeekError
	 *   Raised if the seek is out of range (usually EOF).
	 * @see Player::Seek
	 */
	void PosRaw(const std::string &tag, std::uint64_t pos);

	/**
	 * Emits a response for the current audio state to the sink.
	 *
	 * @param id The ID of the connection to which the Player should
	 *   route any responses.  For broadcasts, use 0.
	 * @param tag The tag of the request calling this command.
	 *   For unsolicited dumps, use Response::NOREQUEST.
	 *
	 * @see DumpRaw
	 */
	void DumpState(size_t id, const std::string &tag) const;

	/**
	 * Outputs a response, if there is a ResponseSink attached.
	 *
	 * Otherwise, this method does nothing.
	 * @param id The ID of the client receiving this response.
	 *   Use 0 for broadcasts.
	 * @param response The Response to output.
	 */
	void Respond(int id, Response rs) const;

	/**
	 * Determines whether we can broadcast a POS response.
	 *
	 * To prevent spewing massive amounts of POS responses, we only send a
	 * broadcast if the number of seconds has changed since the last
	 * time CanAnnounceTime() was called.
	 *
	 * This is *not* idempotent.  A CanAnnounceTime(x) less than one second
	 * before a CanAnnounceTime(x) will _always_ be false.
	 *
	 * @param micros The value of the POS response, in microseconds.
	 * @return Whether it is polite to broadcast POS.
	 */
	bool CanAnnounceTime(std::uint64_t micros);

	//
	// Audio subsystem
	//

	/**
	 * Loads a file, creating an Audio for it.
	 * @param path The path to a file.
	 * @return A unique pointer to the Audio for that file.
	 */
	std::unique_ptr<Audio> LoadRaw(const std::string &path) const;

	/**
	 * Loads a file, creating an AudioSource.
	 * @param path The path to the file to load.
	 * @return An AudioSource pointer (may be nullptr, if no available
	 *   and suitable AudioSource was found).
	 * @see Load
	 */
	std::unique_ptr<AudioSource> LoadSource(const std::string &path) const;
};

#endif // PLAYD_PLAYER_HPP
