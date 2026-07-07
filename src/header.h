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
	size_t GetFields(const std::vector<std::string> &registry);
	void PrepareOutput();

	fs::path File;

  private:
	void FindClasses();
	std::vector<std::string> Lines;
	std::map<std::string, ClassDeclare> Classes;
};
