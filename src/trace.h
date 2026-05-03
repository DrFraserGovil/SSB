#pragma once
#include <sstream>
#include <vector>
#include <filesystem>

struct ErrorObj
{
    std::string Message;
    int Line;
    std::filesystem::path File;
    ErrorObj(int l, std::string file);
    ErrorObj(){};
    std::string Format(int i, size_t namebuff);
};
class Trace
{
    public:
        static Trace Error(int line, std::string file);
        static std::string Flush();
        static std::vector<ErrorObj> & Stack();

        template<class T>
        Trace & operator<<(const T & msg)
        {
            buffer << msg;
            return *this;
        }

        ~Trace();
    private:
        std::ostringstream buffer;

};  


#define TRACE Trace::Error(__LINE__,__FILE__)
// Trace ERROR();