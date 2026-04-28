#pragma once
#include <JSL.h>


class SettingsObject : public JSL::Parameter::RootAggregator
{

    public:
        bool Verbose;
        bool RelaxedFileExtension;
        std::string FileExtension;
        bool AutoRoot;

        SettingsObject()
        {
            Name = "Settings";
            Set(Verbose,configVerbose,false,{"v","verbose"},"Verbose","When true, the logger outputs DEBUG-level output");
            Set(RelaxedFileExtension,configRelaxExtension,false,"relax-extension","Relaxed Extension Matching","When true, any files can be read in as SSB inputs. When false, only those matching the FileExtension variable will match.");
            Set(FileExtension,configFileExtension,".ssb","match-extension","File Extension","The file extension required for a file to be processed by the system");
            Set(AutoRoot,configAutoRoot,false,{"r","root"},"Autoroot","If true and multiple top-level objects specified, an aggregate root to contain them all will be generated");

            DefaultCommand("build","Builds the selected file");
            AddCommand("[filename]", "The name of the file to be acted upon. Must not share a name with any of the other commands");
            AddCommand("pause","Pauses any active watcher routines");
            AddCommand("resume","Resumes any active watcher routines");
            AddCommand("shutdown","Shuts down any active watcher routines");
            AddCommand("watch","Launches a watcher routine ");
        }
    private:
        JSL::Parameter::Setting<bool> configVerbose;
        JSL::Parameter::Setting<bool> configRelaxExtension;
        JSL::Parameter::Setting<std::string> configFileExtension;
        JSL::Parameter::Setting<bool> configAutoRoot;
};

extern SettingsObject Settings;