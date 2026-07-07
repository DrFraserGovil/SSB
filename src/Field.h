#pragma once
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

class Nested
{
  public:
	std::string Type;
	std::string Name;
	int Line;
	bool IsJSLConfigurable = false;

	Nested(std::string type, std::string id, int line);
	std::string Format(std::string_view wrapper = "t");
};
class JSLField
{
  public:
	std::string Type;
	std::string Identifier;
	std::string DefaultValue;
	std::string DisplayName;
	int Line;

	JSLField(std::string type, std::string id, std::string val, int line);
	JSLField(Nested notCapured);
	std::optional<std::string> ProcessComments(std::vector<std::string> comments);
	std::set<std::string> Aliases;
	std::string Documentation;
	std::string Format(std::string_view wrapper, std::string nspace);
};
extern std::map<std::string, std::string> globalAliases;
