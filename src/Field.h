#pragma once
#include <string>
class JSLField
{
  public:
	std::string Type;
	std::string Identifier;
	std::string DefaultValue;
	std::string CommentBlock;
	int Line;
	bool IsJSLConfigurable = false;

	JSLField(std::string type, std::string id, std::string val, int line);
};

class Nested
{
  public:
	std::string Type;
	std::string Name;
	int Line;
	bool IsJSLConfigurable = false;

	Nested(std::string type, std::string id, int line);
};
