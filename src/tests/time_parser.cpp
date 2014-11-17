// This file is part of playd.
// playd is licensed under the MIT licence: see LICENSE.txt.

/**
 * @file
 * Tests for the TimeParser class.
 */

#include "catch.hpp"
#include "../time_parser.hpp"
#include "../errors.hpp"

SCENARIO("TimeParsers successfully parse valid times", "[time-parser]") {
	GIVEN("A fresh TimeParser") {
		TimeParser t;

		WHEN("the TimeParser is fed a unit-less time") {
			TimeParser::MicrosecondPosition time = t.Parse("1234");

			THEN("the result matches the input") {
				REQUIRE(time == 1234ull);
			}
		}

		WHEN("the Tokeniser is fed a time in microseconds") {
			TimeParser::MicrosecondPosition time = t.Parse("1234us");

			THEN("the result matches the input") {
				REQUIRE(time == 1234ull);
			}
		}

		WHEN("the Tokeniser is fed a time in milliseconds") {
			TimeParser::MicrosecondPosition time = t.Parse("1234ms");

			THEN("the result is 1,000 times the input") {
				REQUIRE(time == (1234ull * 1000ull));
			}
		}

		WHEN("the Tokeniser is fed a time in seconds") {
			TimeParser::MicrosecondPosition time = t.Parse("1234s");

			THEN("the result is 1,000,000 times the input") {
				REQUIRE(time == (1234ull * 1000000ull));
			}
		}

		WHEN("the Tokeniser is fed a time in minutes") {
			TimeParser::MicrosecondPosition time = t.Parse("1234m");

			THEN("the result is 60,000,000 times the input") {
				REQUIRE(time == (1234ull * 60000000ull));
			}
		}

		WHEN("the Tokeniser is fed a time in hours") {
			TimeParser::MicrosecondPosition time = t.Parse("1234h");

			THEN("the result is 3,600,000,000 times the input") {
				REQUIRE(time == (1234ull * 3600000000ull));
			}
		}
	}
}

SCENARIO("TimeParsers raise exceptions on invalid times", "[time-parser]") {
	GIVEN("A fresh TimeParser") {
		TimeParser t;

		WHEN("the Tokeniser is fed a unit with no quantity") {
			THEN("a SeekError is thrown") {
				REQUIRE_THROWS_AS(t.Parse("s"), SeekError);
			}
		}

		WHEN("the Tokeniser is fed a quantity with an incorrect unit") {
			THEN("a std::out_of_range exception is thrown") {
				REQUIRE_THROWS_AS(t.Parse("1234z"), std::out_of_range);
			}
		}
	}
}