#pragma once 
#include <vector>
#include <iostream>
#include <string>
#include <string_view>
#include <deque>
struct ContentBlock
{
    std::string Text;
    bool IsLeaf = false;
    std::vector<ContentBlock> Members;
    ContentBlock() : Members{}{};

    void Print(int idt = 0)
    {
        std::string indent = (idt > 1) ? std::string(idt-1,'\t') : "";
        if (IsLeaf)
        {
            if (Text.size() > 0)
            {
                std::cout << indent << Text << std::endl;
            }
            else
            {
                std::cout << "|\n";
            }
        }
        else
        {
            if (idt > 0)
            {
                std::cout << indent << "{" << std::endl;
            }
            
            for (auto member : Members)
            {
                member.Print(idt + 1);
            }

             if (idt > 0)
            {
                std::cout << indent << "}" << std::endl;
            }


        }
    }

    static ContentBlock Leaf(std::string_view line)
    {
        ContentBlock out;
        out.Text = std::string{line};
        out.IsLeaf = true;
        return out;
    }
    void AddLeaf(std::string_view line)
    {
        Members.emplace_back(Leaf(line));
    }
    ContentBlock & Prepare()
    {
        Members.emplace_back();
        return Members.back();
    }
    void Add(ContentBlock block)
    {
        Members.push_back(block);
    }

    std::string Flatten();
};



class ParsedSetting
{
    public:
        std::string Type;
        std::string Name;
        std::string Keys;
        std::string Default;
        std::string Note;
        bool Initialised= false;
        ParsedSetting(std::string_view name, std::deque<std::string> data);
        static std::vector<std::string> & RegisteredKeys();
        std::string ProcessKeys();
};
class ParsedCommand
{
    public:
        std::string Name;
        std::string Description;
        ParsedCommand(std::string_view name, std::string_view text) : Name{name}, Description{text}{};
};
class ParsedAggregator
{
    public:
        std::string Name;
        std::vector<ParsedAggregator> Nested;
        std::vector<ParsedSetting> Members;
        std::vector<ParsedCommand> Commands;
        ParsedAggregator() = default;
        ParsedAggregator(std::string_view name, ContentBlock contents);
        bool Initialised = false;
        void Print(int idt = 0);
    private:
        bool GetNested(ContentBlock contents);
        bool GetCommands(ContentBlock contents);
        bool GetMembers(ContentBlock contents);
};
