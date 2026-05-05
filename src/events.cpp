#include "events.h"
#include <JSL.h>
#include "settings.hpp"
#include "file.h"

void ContingencyPlan(std::vector<fs::path> files)
{
	fs::path loc = fs::temp_directory_path() / Settings.Watcher.Socket;
	//we reach here if the watcher did not initialise; probably due to an extant watcher
	if (fs::exists(loc))
	{
		//this is the case for an extant watcher
		if (files.empty())
		{
			LOG(ERROR) << "Cannot launch a passive watch process for " << loc.string() << ": one is already running";
			throw std::runtime_error("Process already running");
		}
		else
		{
			auto sender = JSL::Antenna::Hotline::Create(Settings.Watcher.Socket,2);
			
			if (!sender)
			{
				return;
			}
			bool response = true;
			for (auto & file: files)
			{
				response &= sender->Send("filewatch " + file.string());
				LOG(DEBUG) << "watch " << file.string() << " " << response;
			}

			
			
			if (response)
			{
				LOG(WARN) << "A watcher process for " << loc.string() << "already exists\nThe input files will now be monitored by that process";
				exit(0);
			}
			else
			{
				LOG(ERROR) << "Could not initialise a connection to " << loc.string() << ", but it refused attempts to takeover. \nTry running with -f option.";
			}
		}	
	}
	else
	{
		LOG(ERROR) << "Catastrophic error: could not establish socket connections";
		exit(-1);
	}
}


void ProcessCommand(std::string_view cmd, bool & Paused, JSL::Watcher * Watch)
{
	cmd = JSL::trim_view(cmd);


	auto q= JSL::split(JSL::getLower(cmd)," ");
	if (q[0] == "filewatch" || q[0] == "watch")
	{
		LOG(INFO) << "Watching " << q[1];
		Watch->Watch(q[1]);
	}
	if (cmd == "pause")
	{
		Paused = true;
	}
	if (cmd== "resume")
	{
		Paused = false;
	}
	if (cmd == "help")
	{
		Settings.HelpMenu();
	}

	if (cmd == "status")
	{
		LOG(INFO) << "-- SSB STATUS --";
		auto a = Watch->GetWatchedFiles();
		if (!a.empty())
		{
			LOG(INFO) << "Compiler:  " << (Paused ? JSL::Format::Red + "Paused" : JSL::Format::Green + "Running");
			std::ostringstream os;
			os << "Watching:  " << JSL::Format::Italics;
			bool isFirst = true;
			for (auto f : a)
			{
				if (!isFirst)
				{
					os << "\n" <<  std::string(11,' ');
				}
				isFirst = false;
				os << f.string();
			}
			LOG(INFO) << os.str();
		}
		else
		{
			LOG(INFO) << "Compiler:  " << JSL::Format::Yellow + JSL::Format::Bold << "Waiting for files";
		}
		auto rt = Watch->GetRuntime();
		LOG(INFO) <<  "Uptime:    " << JSL::FormatDuration(rt.count() * 1.0 / 1e9) << JSL::Format::Cyan + JSL::Format::Italics<< " (timeout after " << JSL::FormatDuration(Settings.Watcher.IdleTimeout * 60) <<")";
	}


}


void ActivateEventLoop(std::vector<fs::path> files)
{
	LOG(DEBUG) << "Initialising event loop";
	auto watcher = JSL::Watcher::Create(Settings.Watcher.Socket, Settings.Watcher.ReplyTimeout, Settings.Watcher.ForceAcquire);

	if (!watcher)
	{
		LOG(INFO) << "Could not establish initial connection";
		ContingencyPlan(files);
		return;
	}

	size_t ncores =3;
	JSL::ParallelEventManager Manager(ncores,watcher.value());
   
	bool Paused = false;

	std::string_view prompt = ">> ";
	if (JSL::Terminal::IsANSICapable())
	{
		prompt = JSL::Format::Blue + ">> " + JSL::Format::Cyan;
	}
	JSL::Watcher * watch = Manager.GetWatcher();
	Manager.SetCInCallback([prompt,&Paused,watch](auto msg)
	{
		if (!JSL::trim_view(msg).empty())
		{
			ProcessCommand(msg,Paused,watch);
		}
		else
		{
			if (JSL::Terminal::IsANSICapable())
			{
				std::cout << JSL::Terminal::CursorUp;
			}
		}
		std::cout << JSL::Terminal::ClearLine << prompt <<std::flush;
	});

	Manager.SetSocketCallback([prompt,&Paused,watch](auto msg){
		ProcessCommand(msg,Paused,watch);
		std::cout << JSL::Terminal::ClearLine << prompt <<std::flush;
	});


	Manager.SetInotifyCallback([prompt,&Paused,watch](auto msg){
		LOG(INFO) << "inotify pinged " << msg.Path.string();


		if (!(msg.Mask & IN_IGNORED) &&  fs::exists(msg.Path))
		{
			if (!Paused)
			{
				SSBFile::Convert(msg.Path);
			}
		}
		else
		{
			watch->Unwatch(msg.Path);
			LOG(INFO) << msg.Path.string() << " no longer on disk.";
		}
		std::cout << JSL::Terminal::ClearLine << prompt <<std::flush;
	});

	watch->SetBlockingTime(4);
	watch->SetDebounce(Settings.Watcher.Debounce);
	watch->SetMaxRuntime(Settings.Watcher.IdleTimeout);

	for (auto & file : files)
	{
		watcher->Watch(file);
	}

	JSL::Log::Global::Config.SetPrompt(prompt);
	LOG(INFO) << "Beginning watcher routine";
	
	Manager.Run();

	JSL::Log::Global::Config.ResetPrompt();
	LOG(INFO) << "Watcher has shut down";
}


void IPC_Message(std::string_view cmd)
{
	if (JSL::Antenna::Transmit(Settings.Watcher.Socket,cmd,Settings.Watcher.ReplyTimeout))
	{
		LOG(INFO) << "Message acknowledged";
	}
	else
	{
		LOG(ERROR) << "Message not acknowledged by host";
	}
}
