#pragma once

#include <filesystem>
#include "content.h"

class SSBFile
{
    public:
        SSBFile(std::filesystem::path path);
        void Print();
        bool WriteTo(std::filesystem::path output);
        bool StatusGood;
        static void Convert(std::filesystem::path path);
    private:
        std::filesystem::path Path;
        ContentBlock Contents;
        ParsedAggregator Root;
        bool ConstructAggregators();
        bool ExtractContents();

};