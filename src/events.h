#pragma once
#include <JSL/Async.h>
#include <thread>
namespace fs = std::filesystem;


void ActivateEventLoop(std::vector<fs::path> files);

void IPC_Message(std::string_view cmd);


// class EventHandler : public JSL::ParallelEventManager
// {
// 	public:
// 		static void Run(
// 		static void SendCommand(std::string_view cmd);
// 	private:
// 		EventHandler(std::vector<fs::path> files, size_t ncores, JSL::Watcher & watcher);
// 		// JSL::Watcher Watcher;


// 		void ProcessCommand(std::string_view cmd);
// 		bool Paused = false;
// };



