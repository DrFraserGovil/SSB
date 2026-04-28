#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <deque>
class ParsedSetting
{
    public:
        std::string Type;
        std::string Name;
        std::string ConfigName;
        std::string Keys;
        std::string Default;
        std::string Note;
        bool Initialised= false;
        ParsedSetting(std::string_view name, std::deque<std::string> data);
        static std::vector<std::string> & RegisteredKeys();
        std::string ProcessKeys();
        void BasicDeclare(std::ostringstream & os);
        void ConfigDeclare(std::ostringstream & os);
        void Set(std::ostringstream & os);
};

std::string stringify(std::string_view word);