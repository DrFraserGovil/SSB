#include <JSL.h>
#include "settings.h"
#include <filesystem>
#include "file.h"
// #include "../tst.hpp"
SettingsObject Settings;

namespace fs = std::filesystem;

std::pair<std::string,std::vector<fs::path>> ProcessCommands()
{
    auto [commands,files]  = Settings.ParseCommands();
    
    if (commands.size() > 1)
    {
        LOG(ERROR) << "Too many commands passed " << JSL::MakeString(commands) << ". Only one command allowed.";
        throw std::logic_error("Command parse error");
    }
    std::string out = "build";
    if (commands.size() == 1)
    {
        out = *commands.begin();
    }

    std::vector<fs::path> paths;
    for (auto file : files)
    {
        fs::path tmp{file};
        bool isDiskFile = fs::exists(tmp) && fs::is_regular_file(tmp);
        if (!isDiskFile)
        {
            LOG(ERROR) << "'" << file <<"' is not a file on disk";
            throw std::runtime_error("Invalid input error");
        }
        bool isSSB = Settings.RelaxedFileExtension || tmp.extension() == Settings.FileExtension;
        if (isDiskFile && isSSB)
        {
            paths.push_back(tmp);
        }
        else
        {
            LOG(ERROR) << "'" << file <<"' is not a valid SSB file";
            throw std::runtime_error("Invalid input error");
        }
    }

    if (JSL::contains(out,std::vector<std::string>{"pause","resume"}) && paths.size() > 0)
    {
        LOG(ERROR) << "Cannot accept file input with a " << out << " command";
        throw std::runtime_error("Invalid input error");
    }
    if (out == "build" && paths.size() == 0)
    {
        LOG(ERROR) << "Must provide an input file to build";
        throw std::runtime_error("Invalid input error");

    }

     LOG(INFO) << "Executing command '" << out << ((paths.size()> 0) ? "' on '" + (std::string)paths[0] + "'" : "'");
    return {out,paths};
}

int main(int argc, char **argv)
{

    try
    {

        Settings.Parse(argc,argv);
        if (Settings.Verbose)
        {
            JSL::Log::Global::Config.SetLevel(DEBUG);
        }
        JSL::Log::Global::Config.ShowHeaders = false;
        auto [cmd, files] = ProcessCommands();
       
        if (cmd== "build")
        {
            for (auto file : files)
            {
                SSBFile f(file);
                file.replace_extension(".hpp");
                f.WriteTo(file);
            }
        }

    }
    catch(const std::exception& e)
    {
        LOG(INFO) << "Did not recover from error '" << e.what() << "'.\nExiting";
    }
    

}