#pragma once
#include <JSL/Async.h>
#include <thread>
namespace fs = std::filesystem;


class EventHandler
{
	public:
		static void Run(std::vector<fs::path> files);
		static void SendCommand(std::string_view cmd);
	private:
		EventHandler(std::vector<fs::path> files, JSL::Watcher & watcher);
		JSL::Watcher Watcher;


		void ProcessCommand(std::string_view cmd);
};



