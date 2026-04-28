#pragma once
#include <JSL.h>


class SettingsObject : public JSL::Parameter::RootAggregator
{

    public:
        bool Verbose;

        SettingsObject()
        {
            Set(Verbose,configVerbose,false,{"v","verbose"},"Verbose","When true, the logger outputs DEBUG-level output");

            DefaultCommand("build","Builds the selected file");
            AddCommand("[filename]", "The name of the file to be acted upon. Must not share a name with any of the other commands");
            AddCommand("pause","Pauses any active watcher routines");
            AddCommand("resume","Resumes any active watcher routines");
            AddCommand("shutdown","Shuts down any active watcher routines");
            AddCommand("watch","Launches a watcher routine ");
        }
    private:
        JSL::Parameter::Setting<bool> configVerbose;
};

extern SettingsObject Settings;