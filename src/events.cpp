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

void EventHandler::Run(std::vector<fs::path> files)
{
    auto watcher = JSL::Watcher::Create(Settings.Watcher.Socket, Settings.Watcher.ReplyTimeout, Settings.Watcher.ForceAcquire);

    if (!watcher)
    {
        LOG(INFO) << "Could not establish initial connection";
        ContingencyPlan(files);
        return;
    }

    EventHandler handler(files,watcher.value());
}

EventHandler::EventHandler(std::vector<fs::path> files, JSL::Watcher & watcher) : Watcher(std::move(watcher))
{
    Watcher.SetCInCallback([this](auto msg)
	{
		if (!msg.empty())
		{
			if (JSL::iEquals(msg,"shutdown") || JSL::iEquals(msg,"exit"))
			{
				Watcher.Message(msg);
			} // this has a cascade that shuts down the sockets
            else
            {
                ProcessCommand(msg);
            }
		}
		LOG(INFO) << JSL::Terminal::CursorUp;
	});
	Watcher.SetSocketCallback([this](auto msg){
		ProcessCommand(msg);
		LOG(INFO) << JSL::Terminal::CursorUp;
	});


    Watcher.SetInotifyCallback([this](auto msg){
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
            Watcher.Unwatch(msg.Path);
            LOG(INFO) << msg.Path.string() << " no longer on disk.";
        }
    });

	Watcher.SetBlockingTime(4);
    Watcher.SetDebounce(Settings.Watcher.Debounce);
	Watcher.SetMaxRuntime(Settings.Watcher.IdleTimeout);

    for (auto & file : files)
    {
        Watcher.Watch(file);
    }

	JSL::Log::Global::Config.SetPrompt(JSL::Format::Yellow + ">> " + JSL::Format::Cyan);
	LOG(INFO) << "Beginning watcher routine";
	Watcher.Run();
	JSL::Log::Global::Config.ResetPrompt();
	LOG(INFO) << "Watcher has shut down";
}


void EventHandler::SendCommand(std::string_view cmd)
{
	// auto sender = JSL::Antenna::Create(loc.string());

	if (JSL::Antenna::Transmit(Settings.Watcher.Socket,cmd,Settings.Watcher.ReplyTimeout))
	{
		LOG(INFO) << "Message acknowledged";
	}
	else
	{
		LOG(ERROR) << "Message not acknowledged by host";
	}
}

void EventHandler::ProcessCommand(std::string_view cmd)
{
    auto q= JSL::split(JSL::getLower(cmd)," ");
    if (q[0] == "filewatch" || q[0] == "watch")
    {
        LOG(INFO) << "Watching " << q[1];
        Watcher.Watch(q[1]);
    }
    if (q[0] == "pause")
    {
        Paused = true;
    }
    if (q[0] == "resume")
    {
        Paused = false;
    }

}