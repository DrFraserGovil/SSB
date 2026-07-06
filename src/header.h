#pragma once
#include "Class.h"
#include <filesystem>
#include <map>
#include <string>
#include <tuple>
#include <vector>
namespace fs = std::filesystem;
class
	HeaderFile
{
  public:
	HeaderFile(fs::path file);

  private:
	void Scan();
	void FindClasses();
	std::vector<std::string> Lines;
	std::vector<std::tuple<std::string, int, int>> ClassBlocks;
	fs::path File;
	std::map<std::string, ClassDeclare> Classes;
};
