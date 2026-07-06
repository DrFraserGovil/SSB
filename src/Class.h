#pragma once
#include "Field.h"
#include <filesystem>
#include <optional>
#include <regex>
#include <string>
class ClassDeclare
{
  public:
	std::string Name;
	std::filesystem::path Host;
	bool IsPublic;
	bool IsNested;
	bool HasInclude;
	bool IsJSLConfigurable;
	std::string Namespace;
	int StartLine;
	int EndLine;

	ClassDeclare(std::smatch grab, std::filesystem::path host, std::vector<std::string> &lines, int linestart, int lineend);
	ClassDeclare() : Lines({}) { IsJSLConfigurable = false; };
	void GetFields(const std::vector<std::string> &possibleChildren);

	std::optional<std::string> WriteToFile();

	std::string Format(std::string tname, std::string iname);

  private:
	std::string DisplayName;
	std::filesystem::path GenFile;
	std::vector<std::string> Lines;
	std::vector<JSLField> Fields;
	std::vector<Nested> SubGroup;
	std::optional<std::string> RejectFields(JSLField &suspected);
	std::optional<std::string> RejectNested(Nested &suspected, const std::vector<std::string> &registry);
	std::vector<std::string> CaptureComments(int line);
	void GetSelfMeta();
	std::string IdealForm();
};
