// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Implementation of client response classes.
 * @see response.hpp
 */

#include <cctype>
#include <initializer_list>
#include <sstream>

#include "errors.hpp"

#include "response.hpp"

const std::string Response::NOREQUEST = "!";

const std::string Response::STRINGS[] = {
	"OHAI",     // Code::OHAI
	"IAMA",     // Code::IAMA
	"FLOAD",    // Code::FLOAD
	"EJECT",    // Code::EJECT
	"POS",      // Code::POS
	"END",      // Code::END
	"PLAY",     // Code::PLAY
	"STOP",     // Code::STOP
	"ACK",      // Code::ACK
	"DUMP",     // Code::DUMP
	"QUIT"      // Code::QUIT
};

Response::Response(const std::string &tag, Response::Code code)
{
	this->string = Response::EscapeArg(tag) + " " + Response::STRINGS[static_cast<int>(code)];
}

Response &Response::AddArg(const std::string &arg)
{
	this->string += " " + Response::EscapeArg(arg);
	return *this;
}

std::string Response::Pack() const
{
	return this->string;
}

/* static */ std::string Response::EscapeArg(const std::string &arg)
{
	bool escaping = false;
	std::string escaped;

	for (char c : arg) {
		// These are the characters (including all whitespace, via
		// isspace())  whose presence means we need to single-quote
		// escape the argument.
		bool is_escaper = c == '"' || c == '\'' || c == '\\';
		if (isspace(c) || is_escaper) escaping = true;

		// Since we use single-quote escaping, the only thing we need
		// to escape by itself is single quotes, which are replaced by
		// the sequence '\'' (break out of single quotes, escape a
		// single quote, then re-enter single quotes).
		escaped += (c == '\'') ? R"('\'')" : std::string(1, c);
	}

	// Only single-quote escape if necessary.
	// Otherwise, it wastes two characters!
	if (escaping) return "'" + escaped + "'";
	return escaped;
}

//
// ResponseSink
//

void ResponseSink::Respond(size_t, Response &) const
{
	// By default, do nothing.
}
