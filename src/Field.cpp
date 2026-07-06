#include "Field.h"
#include "Settings.h"
#include <JSL.h>
std::regex aliasCapture(R"(^@alias(?:es)?\s+(\S.*))");
std::regex briefCapture(R"(^@brief\s+(\S.*))");
std::regex detailsCapture(R"(^@details?\s+(\S.*))");
std::regex displayCapture(R"(^@display?\s+(\S.*))");
JSLField::JSLField(std::string type, std::string id, std::string val, int line) : Type(type), Identifier(id), DefaultValue(val)
{
	Line = line;
}

JSLField::JSLField(Nested nonCaptured)
{
	Type = nonCaptured.Type;
	Identifier = nonCaptured.Name;
	DefaultValue = "{}";
	Line = nonCaptured.Line;
}

void cleanStr(std::string &str, const std::string &target)
{
	size_t pos = 0;
	while ((pos = str.find(target, pos)) != std::string::npos)
	{
		str.erase(pos, target.length());
	}
}
void stripStr(std::string &str, char prefix)
{
	while (str.size() > 0 && str[0] == prefix)
	{
		str = str.substr(1);
	}
}
void quote(std::string &wrapped)
{
	if (wrapped[0] != '"') { wrapped = "\"" + wrapped; }
	if (wrapped.back() != '"') { wrapped = wrapped + "\""; }
}
std::optional<std::string> JSLField::ProcessComments(std::vector<std::string> comments)
{

	if (comments.empty())
	{
		return "No metadata comments";
	}
	std::vector<std::string> brief;
	DisplayName = Identifier;
	for (auto &cmt : comments)
	{
		cleanStr(cmt, "*/");
		cleanStr(cmt, "/*");
		stripStr(cmt, '/');
		stripStr(cmt, '!'); // remove doxygen
		cmt = JSL::String::trim(cmt);

		std::vector<std::string> tripHazards = {"@param", "@tparam", "@return"};
		for (auto &hzd : tripHazards)
		{
			if (cmt.find(hzd) != cmt.npos)
			{
				return "Metadata '" + hzd + "' implies this is not parameter";
			}
		}

		std::smatch groups;
		if (std::regex_match(cmt, groups, aliasCapture))
		{
			std::vector<std::string_view> sp;
			auto vec = groups[1].str();
			if (vec.find(",") != vec.npos)
			{
				sp = JSL::String::split_view(vec, ",");
			}
			else
			{
				sp = JSL::String::split_view(vec, " ");
			}
			for (auto cmd : sp)
			{
				auto trimmed = JSL::String::trim_view(cmd);
				if (!trimmed.empty())
				{
					std::string wrapped(trimmed);
					quote(wrapped);
					Aliases.emplace(wrapped);
				}
			}
		}

		if (std::regex_match(cmt, groups, briefCapture))
		{
			brief.emplace_back(groups[1]);
		}
		if (std::regex_match(cmt, groups, detailsCapture))
		{
			brief.emplace_back(groups[1]);
		}
		if (std::regex_match(cmt, groups, displayCapture))
		{
			DisplayName = JSL::String::trim(groups[1].str());
		}
	}

	if (Aliases.empty())
	{
		return "No @aliases provided";
	}
	if (brief.empty())
	{
		return "No documentation - @brief or @detail - provided";
	}
	Documentation = JSL::String::stitch(brief, "\\n");
	quote(Documentation);
	return std::nullopt;
}

std::string JSLField::Format(std::string_view wrapper, std::string nspace)
{
	std::ostringstream os;
	os << wrapper << "(" << nspace << "Field<" << Type << ">(" << Identifier << ", \"" << DisplayName << "\", ";
	os << "{" << JSL::String::stitch(Aliases, ", ") << "}, ";
	os << DefaultValue << ", ";
	os << Documentation << "));";
	return os.str();
}
std::string Nested::Format(std::string_view wrapper)
{
	std::ostringstream os;
	os << wrapper << "(" << Name << ");";
	return os.str();
}
Nested::Nested(std::string type, std::string id, int line) : Type(type), Name(id)
{
	Line = line;
}
