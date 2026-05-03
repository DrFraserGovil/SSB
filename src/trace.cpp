#include "trace.h"
#include <JSL/Display/Log.h>
#include <JSL/Strings.h>

ErrorObj::ErrorObj(int line, std::string file)
{
    Message = "";
    Line = line;
    File = file;
}

std::string ErrorObj::Format(int level,size_t buffersize)
{
    std::ostringstream os;
    auto ls = JSL::foldToWidth(Message,60);

    auto name = File.filename().string();


    os << "Level " << level << ": " << ls[0] << "  " << name << std::string(buffersize+2-name.length(),' ') << "line " << Line;
    for (size_t i = 1; i < ls.size(); ++i)
    {
        os << "\n" << std::string(9, ' ') << ls[i];
    }
    // os << "Level " << level << ": " << Message << "    " << Line << " " << File.filename().string();
    return os.str();
}


Trace Trace::Error(int line, std::string file)
{
    Stack().emplace_back(line,file);
    return Trace();
}

std::string Trace::Flush()
{
    std::ostringstream s;

    size_t maxnamelength = 0;
    auto & stack = Stack();
    for (auto e : stack)
    {
        size_t ell = e.File.filename().string().size();
        maxnamelength = std::max(ell,maxnamelength); 
    }


    s << "\nERROR";
    for (int i = stack.size() - 1; i >= 0; --i)
    {
        s << "\n" << stack[i].Format(stack.size() - i,maxnamelength);
    }
    stack.clear();
    LOG(ERROR) << s.str();
    return s.str();
}

Trace::~Trace()
{
    // LOG(ERROR) << buffer.str();
    Stack().back().Message = buffer.str();
}

std::vector<ErrorObj> & Trace::Stack()
{
    static std::vector<ErrorObj> out;
    return out;
}