/*!
UnitTest.cpp
@author Alex Schlieck
@version 0.1
@date 2023-05-31

---------------------------------
Unit tests for the miniLogger Library
Built using catch2lib

*/

#define CATCH_CONFIG_MAIN

#include <iostream>
#include "catch.hpp"
#include "BoomLog.hpp"



using namespace boom;

class TestStream : public Stream
{
public:
	TestStream()
	{}

	std::string getMsg()
	{
		return e.msg;
	}
	
	std::string getTime()
	{
		const std::string dateFormat("%Y/%m/%d/%H:%M:%S - ");

		struct tm output;
		std::stringstream result;

		auto converted = std::chrono::system_clock::to_time_t(e.timestamp);
		localtime_s(&output, &converted);
		result << std::put_time(&output, dateFormat.c_str());
		return result.str();
	}

	std::string getEventString()
	{
		return e.toString();
	}

	virtual void handle(Event& event)
	{
		e = event;
	}

private:
	Event e;
};

TEST_CASE("Logger")
{
	SECTION("Event")
	{
		// create a known timestamp to compare to
		std::tm target = {};
		std::stringstream timestamp("Jan 1 2000 12:00:00");
		timestamp >> std::get_time(&target, "%b %d %Y %H:%M:%S");
		auto tp = std::chrono::system_clock::from_time_t(std::mktime(&target));
		
		Event e(LEVELS::WARNING, "Test Message", "", "", tp);
		REQUIRE(e.toString() == " ! 2000/01/01/12:00:00 - Test Message");
	}

	SECTION("Stream")
	{
		TestStream* t = new TestStream;
		t->setLevels(LEVELS::INFO | LEVELS::DBG);
		Log::addStream("Test", t);
		Log::log(LEVELS::INFO, "InfoEvent");
		REQUIRE(t->getMsg() == "InfoEvent");
		Log::log(LEVELS::DBG, "DebugEvent");
		REQUIRE(t->getMsg() == "DebugEvent");
		Log::log(LEVELS::ERR, "InvalidStream");
		REQUIRE(t->getMsg() != "InvalidStream");

		delete Log::removeStream("Test");
	}

	SECTION("Streams Management")
	{
		REQUIRE(Log::getStream("Target") == nullptr);
		TestStream* t1 = new TestStream;
		Log::addStream("Target", t1);
		REQUIRE(Log::getStream("Target") == t1);
		REQUIRE(Log::removeStream("Target") == t1);
		REQUIRE(Log::getStream("Target") == nullptr);
		delete t1;
	}

	SECTION("Logger Levels")
	{
		TestStream* t_dbg_info = new TestStream();
		t_dbg_info->setLevels(LEVELS::DBG | LEVELS::INFO | LEVELS::ERR);

		TestStream* t_warn_crit = new TestStream();
		t_warn_crit->setLevels(LEVELS::WARNING | LEVELS::CRITICAL | LEVELS::ERR);

		Log::addStream("dbg_info", t_dbg_info);
		Log::addStream("warn_crit", t_warn_crit);

		Log::debug("dbg_msg");
		REQUIRE(t_dbg_info->getMsg() == "dbg_msg");
		REQUIRE(t_warn_crit->getMsg() != "dbg_msg");

		Log::info("info_msg");
		REQUIRE(t_dbg_info->getMsg() == "info_msg");
		REQUIRE(t_warn_crit->getMsg() != "info_msg");

		Log::warning("warn_msg");
		REQUIRE(t_dbg_info->getMsg() != "warn_msg");
		REQUIRE(t_warn_crit->getMsg() == "warn_msg");

		Log::critical("crit_msg");
		REQUIRE(t_dbg_info->getMsg() != "crit_msg");
		REQUIRE(t_warn_crit->getMsg() == "crit_msg");

		Log::error("err_msg");
		REQUIRE(t_dbg_info->getMsg() == "err_msg");
		REQUIRE(t_warn_crit->getMsg() == "err_msg");

		delete Log::removeStream("dbg_info");
		delete Log::removeStream("warn_crit");
	}

	SECTION("Output Formatting")
	{
		TestStream t;
		Log::addStream("Test", &t);

		std::string expected;

		Log::debug("dbg_msg");
		expected = " # " + t.getTime() + "dbg_msg";
		REQUIRE(t.getEventString() == expected);

		Log::info("info_msg");
		expected = "   " + t.getTime() + "info_msg";
		REQUIRE(t.getEventString() == expected);

		Log::warning("warning_msg");
		expected = " ! " + t.getTime() + "warning_msg";
		REQUIRE(t.getEventString() == expected);

		Log::error("error_msg");
		expected = "!! " + t.getTime() + "error_msg";
		REQUIRE(t.getEventString() == expected);

		Log::critical("critical_msg");
		expected = "!!!" + t.getTime() + "critical_msg";
		REQUIRE(t.getEventString() == expected);
		Log::removeStream("Test");
	}

	SECTION("Optional Formatting")
	{
		TestStream * t = new TestStream;
		Log::addStream("Test", t);

		std::string expected;

		Log::info("info_msg", "", "C0001");
		expected = "   " + t->getTime() + "[C0001] info_msg";
		REQUIRE(t->getEventString() == expected);

		Log::info("info_msg", "UnitTest");
		expected = "   " + t->getTime() + "info_msg (from UnitTest)";
		REQUIRE(t->getEventString() == expected);

		Log::info("info_msg", "UnitTest", "C0002");
		expected = "   " + t->getTime() + "[C0002] info_msg (from UnitTest)";
		REQUIRE(t->getEventString() == expected);

		delete(Log::removeStream("Test"));
	}

	SECTION("Debug Messages")
	{

		TestStream* t = new TestStream;
		Log::addStream("Test", t);

		#define SHOW_DEBUG // simulate Debug build

		Log::info("Info");
		Log::debug("Debug");
		REQUIRE(t->getMsg() == "Debug");

		// Find a way to simulate and test the macro that would be generated in release build
	}
}
