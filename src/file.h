#pragma once

#include <filesystem>
#include "content.h"

class SSBFile
{
    public:
        SSBFile(std::filesystem::path path);
        void Print();
        bool WriteTo(std::filesystem::path output);
    private:
        std::filesystem::path Path;
        ContentBlock Contents;
        ParsedAggregator Root;
        bool ConstructAggregators();
        bool ExtractContents();
        
};