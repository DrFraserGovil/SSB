#pragma once
#include <JSL/Parameters.h>

class Settings_InputObj : public JSL::Parameter::NestedAggregator
{
	public:
		//Settings values
		//! @brief When true, any files can be read in as SSB inputs. When false, only those matching the FileExtension variable will match
		bool NoExtensionCheck;

		//! @brief The file extension required for a file to be processed by the syste
		std::string Extension;


		//Constructor: register & connect the variables
		Settings_InputObj()
		{
			Name = "Input";
			Set(NoExtensionCheck, configNoExtensionCheck, (bool)false, {"relax-extension"}, "NoExtensionCheck", "When true, any files can be read in as SSB inputs. When false, only those matching the FileExtension variable will match.");
			Set(Extension, configExtension, (std::string)".ssb", {"match-extension"}, "Extension", "The file extension required for a file to be processed by the system");
		}
	private:
		//Hidden configurables
		JSL::Parameter::Setting<bool> configNoExtensionCheck;
		JSL::Parameter::Setting<std::string> configExtension;
};

class Settings_OutputObj : public JSL::Parameter::NestedAggregator
{
	public:
		//Settings values
		//! @brief Specifies the directory to write the output to. By default this is relative to the source file's directory (see RelativeToHere
		std::string Directory;

		//! @brief When true, the Directory value is interpreted relative to the current working directory; when false it uses the source file instead
		bool RelativeToHere;

		//! @brief The file extension given to the output file
		std::string Extension;


		//Constructor: register & connect the variables
		Settings_OutputObj()
		{
			Name = "Output";
			Set(Directory, configDirectory, (std::string)".", {"o", "output"}, "Directory", "Specifies the directory to write the output to. By default this is relative to the source file's directory (see RelativeToHere)");
			Set(RelativeToHere, configRelativeToHere, (bool)false, {"from-here"}, "RelativeToHere", "When true, the Directory value is interpreted relative to the current working directory; when false it uses the source file instead.");
			Set(Extension, configExtension, (std::string)".hpp", {"out-extension"}, "Extension", "The file extension given to the output file.");
		}
	private:
		//Hidden configurables
		JSL::Parameter::Setting<std::string> configDirectory;
		JSL::Parameter::Setting<bool> configRelativeToHere;
		JSL::Parameter::Setting<std::string> configExtension;
};

class Settings_WatcherObj : public JSL::Parameter::NestedAggregator
{
	public:
		//Settings values
		//! @brief The length of time (in minutes) a watcher will stay idle before exiting. A negative value prevents timeouts from ever occuring
		double IdleTimeout;

		//! @brief The length of time (in seconds) a broadcaster will await a response from a watcher before determining that the watcher is dea
		double ReplyTimeout;

		//! @brief The minimum time delay between polling times (in ms
		size_t Debounce;

		//! @brief The UDS-socket the watcher and broadcaster use to communicate. Only one watcher can exist per socket, and commands are sent only to the watcher associated with the given socket
		std::string Socket;

		//! @brief Forcibly acquires the watcher for the chosen socket
		bool ForceAcquire;


		//Constructor: register & connect the variables
		Settings_WatcherObj()
		{
			Name = "Watcher";
			Set(IdleTimeout, configIdleTimeout, (double)10, {"idle"}, "IdleTimeout", "The length of time (in minutes) a watcher will stay idle before exiting. A negative value prevents timeouts from ever occuring.");
			Set(ReplyTimeout, configReplyTimeout, (double)1, {"reply-timeout"}, "ReplyTimeout", "The length of time (in seconds) a broadcaster will await a response from a watcher before determining that the watcher is dead");
			Set(Debounce, configDebounce, (size_t)50, {"delay"}, "Debounce", "The minimum time delay between polling times (in ms)");
			Set(Socket, configSocket, (std::string)"ssb.socket", {"socket"}, "Socket", "The UDS-socket the watcher and broadcaster use to communicate. Only one watcher can exist per socket, and commands are sent only to the watcher associated with the given socket.");
			Set(ForceAcquire, configForceAcquire, (bool)false, {"f", "force"}, "ForceAcquire", "Forcibly acquires the watcher for the chosen socket.");
		}
	private:
		//Hidden configurables
		JSL::Parameter::Setting<double> configIdleTimeout;
		JSL::Parameter::Setting<double> configReplyTimeout;
		JSL::Parameter::Setting<size_t> configDebounce;
		JSL::Parameter::Setting<std::string> configSocket;
		JSL::Parameter::Setting<bool> configForceAcquire;
};

class SettingsObj : public JSL::Parameter::Aggregator
{
	public:
		//Settings values
		//! @brief When true, the logger outputs DEBUG-level output
		bool Verbose;

		//! @brief When true, a failed SSB-convert will raise an error and exit the program. When false, error logs will be printed to the terminal, but no other action taken
		bool BuildFailIsError;

		//! @brief If true and multiple top-level objects specified, an aggregate root to contain them all will be generate
		bool AutoRoot;

		//Nested objects
		Settings_InputObj Input;
		Settings_OutputObj Output;
		Settings_WatcherObj Watcher;

		//Constructor: register & connect the variables
		SettingsObj()
		{
			Name = "Settings";
			Set(Verbose, configVerbose, (bool)false, {"v", "verbose"}, "Verbose", "When true, the logger outputs DEBUG-level output.");
			Set(BuildFailIsError, configBuildFailIsError, (bool)false, {"strict", "s"}, "BuildFailIsError", "When true, a failed SSB-convert will raise an error and exit the program. When false, error logs will be printed to the terminal, but no other action taken.");
			Set(AutoRoot, configAutoRoot, (bool)false, {"r", "root"}, "AutoRoot", "If true and multiple top-level objects specified, an aggregate root to contain them all will be generated");
			NestGroup(Input,"Input");
			NestGroup(Output,"Output");
			NestGroup(Watcher,"Watcher");
			DefaultCommand("build", "Builds the selected file");
			AddCommand("pause", "Pauses any active watcher routines");
			AddCommand("[filename]", "The name of the file to be acted upon. Must not share a name with any of the other commands");
			AddCommand("resume", "Resumes any active watcher routines");
			AddCommand("shutdown", "Shuts down any active watcher routines");
			AddCommand("watch", "Launches a watcher routine");
			AddCommand("status", "Queries an active watcher as to its status");
		}
	private:
		//Hidden configurables
		JSL::Parameter::Setting<bool> configVerbose;
		JSL::Parameter::Setting<bool> configBuildFailIsError;
		JSL::Parameter::Setting<bool> configAutoRoot;
};

//declaration for a global variable
extern SettingsObj Settings;
