#pragma once
#include "Field.h"
#include <regex>
#include <string>
class ClassDeclare
{
  public:
	std::string Name;
	bool IsPublic;
	bool IsNested;
	bool IsJSLConfigurable;
	std::string Namespace;
	int StartLine;
	int EndLine;
	ClassDeclare(std::smatch grab, int linestart, int lineend);
	ClassDeclare() { IsJSLConfigurable = false; };
	void GetFields(std::vector<std::string> lines, std::vector<std::string> &possibleChildren);
	std::vector<JSLField> Fields;
};
