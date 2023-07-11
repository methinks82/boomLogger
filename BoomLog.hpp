/*!
BoomLog.hpp
@author Alex Schlieck
@version 0.1
@date 2023-05-31
--------------------------------------------------

!!! REQUIRES C++17 or newer to compile


A small, simple logging library

For simple usage call one of the following functions to create an event:
boom::Log::debug(msg);
boom::Log::info(msg);
boom::Log::warning(msg);
boom::Log::error(msg);
boom::Log::critical(msg);

This will send your message to the console and write it to a text file.
For more advanced usage see the readme file
*/


#ifndef BOOM_LOGGER_HPP
#define BOOM_LOGGER_HPP

#include <string>
#include <iostream>
#include <iomanip> // put_time
#include <map>
#include <vector>
#include <fstream>
#include <chrono>
#include <sstream>

#if _DEBUG
#define SHOW_DEBUG
#endif

/// @brief Class to provide simple logging functionality
namespace boom
{
	/// @brief the different types of messages that can be logged
	enum LEVELS { 
		DBG = 1,		// Values being tracked while debugging. Automatically ignored in release build
		INFO = 2,		// Track that something expected has happened
		WARNING = 4,	// Track that something unexpected has happened
		ERR = 8,		// A non-fatal error has occurred
		CRITICAL = 16	// A fatal error has occurred, program may crash	
	};

	const int ALL_LEVELS = 31;// Used to set a stream to listen to all of the levels

	/// @brief Stores one message with associated data
	class Event
	{
	public:
		Event() : level(LEVELS::INFO)
		{}

		/// @brief Constructor
		/// @param level 
		/// @param msg 
		/// @param timestamp
		Event(	LEVELS level,
				const std::string& msg, 
				const std::string& source = "",
				const std::string& code = "",
				std::chrono::time_point<std::chrono::system_clock> timestamp = std::chrono::system_clock::now()
			)
			:level(level), msg(msg), source(source), code(code), timestamp(timestamp) {}


		/// @brief Generate a string containing all of the event's data
		/// @return String with event's data
		std::string toString()
		{
			std::string levelSymbol;
			switch (level)
			{
			case(LEVELS::DBG): levelSymbol = " # ";
				break;
			case(LEVELS::INFO): levelSymbol = "   ";
				break;
			case(LEVELS::WARNING): levelSymbol = " ! ";
				break;
			case(LEVELS::ERR): levelSymbol = "!! ";
				break;
			case(LEVELS::CRITICAL): levelSymbol = "!!!";
				break;
			}


			std::stringstream result;
			//TODO: Allow user to use custom date format
			const std::string dateFormat("%Y/%m/%d/%H:%M:%S - ");

			struct tm output;

			auto converted = std::chrono::system_clock::to_time_t(timestamp);
			localtime_s(&output, &converted);


			result << levelSymbol << std::put_time(&output, dateFormat.c_str());
			if (code != "")
			{
				result << "[" << code << "] ";
			}

			result << msg;

			if (source != "")
			{
				result << " (from " << source << ")";
			}
			return result.str();
		}

		/// @brief When the event happened
		std::chrono::time_point<std::chrono::system_clock> timestamp;
		LEVELS level;
		std::string msg;
		std::string source;
		std::string code;
	};

	/// @brief Possible destinations that events can be sent to
	class Stream
	{
	public:
		/// @brief Default constructor
		Stream() : levels(ALL_LEVELS) {}

		/// @brief Change the event levels to listen to
		/// @param levels 
		void setLevels(int levels)
		{
			this->levels = levels;
		}

		/// @brief Alert this stream of a new event
		/// Check if the event is of a level that is being monitored and handle if it is
		/// @param event the new event to handle
		void call(Event& event)
		{
			//TODO: skip debug events if not debugging

			if ((levels & event.level) == event.level)
			{
				handle(event);
			}
		}

		/// @brief 
		/// @param event The new event to handle
		virtual void handle(Event& event) = 0;

	private:
		/// @brief the types of events that this handler is listening for
		int levels;
	};


	/// @brief Stream that writes the events to a text file
	class TextFileStream : public Stream
	{
	public:
		/// @brief Constructor
		/// @param path Name of file to write events to, default is Log.txt
		TextFileStream(const std::string& fileName = "Log.txt") :file(fileName)
		{}

		/// @brief Write the event to a text file
		/// @param event to be written to file
		virtual void handle(Event& event)
		{
			std::fstream f{file, f.app | f.out};
			if (f.is_open())
			{
				std::string msg = event.toString();
				f.write(msg.c_str(), msg.size());
			}
			f.close();
		}

		/// @brief set a new filename for the stream to write to 
		/// @param filename new name of the log file
		void setFilename(const std::string& filename)
		{
			file = filename;
		}

	private:
		/// @brief Name of file to write to
		std::string file;
	};

	/// @brief Stream that outputs the events to the console
	class ConsoleStream : public Stream
	{
	public:
		/// @brief Send the event to be displayed on a console
		/// @param event to be sent to console
		virtual void handle(Event& event)
		{
			std::cout << event.toString();
		}
	};

	/// @brief Stream that stores a copy of each event
	class ArchiveStream : public Stream
	{
	public:
		/// @brief Store the event to be handled later
		/// @param event to be sent to stored
		virtual void handle(Event& event)
		{
			events.push_back(event);
		}

		/// @brief Access the stored events
		/// @return a vector containing all stored events
		std::vector<Event>& getEvents()
		{
			return events;
		}

	private:
		/// @brief a container to store the events
		std::vector<Event> events;
	};



	class Log
	{
	public:

		//TODO: Add destructor, including cleaning up default streams

		/// @brief Get a pointer to a registered stream with the given name
		/// @param name the stream to look for
		/// @return pointer to stream with given name
		/// @return nullptr if there no matching stream is found
		static Stream* getStream(const std::string& name)
		{
			Stream* result = nullptr;
			auto inst = getInstance();
			auto found = inst->streams.find(name);
			if (found != inst->streams.end())
			{
				result = found->second;
			}
			return result;
		}

		/// @brief 
		/// @param name 
		/// @param stream 
		static void addStream(const std::string& name, Stream* stream)
		{
			auto inst = getInstance();
			inst->streams[name] = stream;
		}

		/// @brief Remove a stream's pointer from the list and return the pointer so that it can be deleted
		/// @param name ID of the stream to remove
		/// @return pointer to the removed stream
		/// @return nullptr if no matching stream was found
		static Stream* removeStream(const std::string& name)
		{
			auto inst = getInstance();
			Stream* result = getStream(name);
			if (result != nullptr)
			{
				inst->streams.erase(name);
			}
			return result;
		}

		/// @brief Allow the logger to handle debug messages even if in release build
		/// @param show whether to display debug messages in release build
		static void forceDebug(bool show)
		{
			showDebug = show;
		}

		/// @brief generate a new log event
		/// @param level define how the event will be handled
		/// @param msg what is to be output
		/// @param source [optional] the fuction that caused the event to be created
		/// @param code [optional] a user-defined code that represents the errror
		static void log(LEVELS level, const std::string& msg, const std::string& source ="", const std::string& code = "")
		{
			bool debugVisible = showDebug;
			#ifdef SHOW_DEBUG
				debugVisible = true;
			#endif // LOG_DEBUG

			// check if we should show debug messages
			if (level != LEVELS::DBG || debugVisible)
			{
				auto inst = getInstance();
				Event e{ level, msg, source, code };

				for (auto s : inst->streams)
				{
					s.second->call(e);
				}
			}
		}

		/// @brief Create an event that is only visible during debugging
		/// Useful for checking variable values etc.
		/// @param msg description of the event
		/// @param source [optional] the fuction that caused the event to be created
		/// @param code [optional] a user-defined code that represents the errror

		static void debug(const std::string& msg, const std::string& source = "", const std::string& code = "")
		{
			log(LEVELS::DBG, msg, source, code);
		}

		/// @brief Create an event to track that something normal has happened
		/// Something is loaded, a user connects, an action is taken, etc.
		/// @param msg description of the event
		/// @param source [optional] the fuction that caused the event to be created
		/// @param code [optional] a user-defined code that represents the errror
		static void info(const std::string& msg, const std::string& source = "", const std::string& code = "")
		{
			log(LEVELS::INFO, msg, source, code);
		}

		/// @brief Create an event to alert that something unexpected has happened
		/// A file or a service is not available, user entered invalid input, etc.
		/// @param msg description of the event
		/// @param source [optional] the fuction that caused the event to be created
		/// @param code [optional] a user-defined code that represents the errror
		static void warning(const std::string& msg, const std::string& source = "", const std::string& code = "")
		{
			log(LEVELS::WARNING, msg, source, code);
		}

		/// @brief Create an event indicating an error that can be handled or caught
		/// @param msg description of the event
		/// @param source [optional] the fuction that caused the event to be created
		/// @param code [optional] a user-defined code that represents the errror
		static void error(const std::string& msg, const std::string& source = "", const std::string& code = "")
		{
			log(LEVELS::ERR, msg, source, code);
		}

		/// @brief Create a new event indicating a critical error that could cause the program to crash
		/// @param msg description of the event
		/// @param source [optional] the fuction that caused the event to be created
		/// @param code [optional] a user-defined code that represents the errror
		static void critical(const std::string& msg, const std::string& source = "", const std::string& code = "")
		{
			log(LEVELS::CRITICAL, msg, source, code);
		}

	private:

		/// @brief Constructor, private to make it a singleton
		Log() = default;

		/// @brief Copy constructor, not used
		/// @param obj object to copy, not used
		Log(const Log& obj) = delete;

		/// @brief get access to a single instance of the logger
		/// Create a new instance if none exists
		/// @return pointer to the instantiated singleton
		static Log* getInstance()
		{
			if (instance == nullptr)
			{
				instance = new Log();
				instance->addStream("defaultTextFile", new TextFileStream("log.txt"));
				instance->addStream("defaultConsole", new ConsoleStream());
			}
			return instance;
		}

		/// @brief The singleton instance
		inline static Log* instance = nullptr;

		/// @brief List of all the streams that are listening to events
		std::map<std::string, Stream*>streams;

		/// @brief Whether to display debug messages in release build
		inline static bool showDebug = false;
	};


	

}

#endif // !BOOM_LOGGER_HPP
