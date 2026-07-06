#pragma once
#include "Class.h"
#include <filesystem>
#include <map>
#include <string>
#include <vector>
namespace fs = std::filesystem;
class
	HeaderFile
{
  public:
	HeaderFile(fs::path file);

	void RegisterClassNames(std::vector<std::string> &registry);
	void GetFields(const std::vector<std::string> &registry);
	void PrepareOutput();

  private:
	void FindClasses();
	std::vector<std::string> Lines;
	fs::path File;
	std::map<std::string, ClassDeclare> Classes;
};
