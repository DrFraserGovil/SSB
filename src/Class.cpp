#include "Class.h"
#include "Settings.h"
#include <JSL.h>
#include <optional>
ClassDeclare::ClassDeclare(std::smatch grab, int linestart, int lineend)
{
	StartLine = linestart;
	EndLine = lineend;

	Name = grab[1];
	IsPublic = grab[2].matched;
	Namespace = "";
	if (grab[3].matched)
	{
		Namespace = grab[3].str();
	}
	IsNested = grab[4].matched;
	if (IsNested)
	{
		std::string capture = grab[4].str();
		std::string nestCapture = capture.substr(1, capture.size() - 2);
		IsNested = (nestCapture == Name);
	}

	IsJSLConfigurable = (IsPublic && IsNested) || Settings.Modes.ActiveFix;

	if (IsJSLConfigurable)
	{
		LOG(INFO) << JSL::Display::Colour(80, 180, 70) << "   = Detected class " << Name << " [" << linestart + 1 << "-" << lineend + 1 << "]";
	}
	else
	{
		std::ostringstream os;
		os << "Ignoring class " << Name << "; because:";
		if (!IsPublic) { os << "\n\t* Aggregator inheritance not declared public"; }
		if (!IsNested) { os << "\n\t* Aggregator CRTP pattern not deployed"; }
		os << "\n\tEnable active-fixing (--fix) to automatically resolve this issues";
		LOG(WARN) << os.str();
	}
}

std::optional<std::pair<std::string, std::string>> jammedTemplate(std::string_view jam)
{
	int level = 0;
	jam = JSL::String::trim_view(jam);

	for (size_t i = 0; i < jam.size(); ++i)
	{
		char ltr = jam[i];
		if (ltr == '<')
		{
			++level;
			continue;
		}
		if (ltr == '>')
		{
			--level;
			if (level < 0) { return std::nullopt; }
			if (level == 0)
			{
				if (i < jam.size() - 1)
				{

					std::pair<std::string, std::string> out = {std::string(jam.substr(0, i + 1)), std::string(jam.substr(i + 1))};
					return out;
				}
				{
					return std::nullopt;
				}
			}
		}
	}

	return std::nullopt;
}
std::regex fullCapture("^\\s*(.*)\\s+(\\w+)\\s*=\\s*(.*?)\\s*;(.*)$");
std::regex simpleCapture("^\\s*(\\S.*)=\\s*(.*?)\\s*;(.*)$");
std::regex nestedCapture(R"(^\s*(\S*)\s+(\S*)\s*;)");
void ClassDeclare::GetFields(std::vector<std::string> lines, std::vector<std::string> &possibleChildren)
{

	std::vector<JSLField> suspectedFields;
	std::vector<Nested> suspectedNesteds;
	bool commented = false;
	bool haveWarned = false;
	bool lineDanger = false;
	for (int i = StartLine; i < EndLine; ++i)
	{
		auto line = JSL::String::trim(lines[i], "//");
		/*/*
			Inline comments have been stripped, but we need to be careful about multiline comments (like this one), as there's no inline indicator that the element is dormant
		*/

		auto foundOpen = line.find("/*") != line.npos;
		auto foundClose = line.find("*/") != line.npos;

		if (!commented)
		{
			if (foundClose && !foundOpen)
			{
				LOG(ERROR) << "Class " << Name << " closes a multiline comment without opening one. This indicates the class has been sliced.";
				throw std::runtime_error("Sliced class");
			}
			if (foundOpen && !foundClose)
			{
				commented = true;
				lineDanger = false;
			}
		}
		else
		{
			if (foundOpen && !foundClose) { /* weird nested MLC, but not an error */ }
			if (foundClose && !foundOpen)
			{
				commented = false;
				lineDanger = false;
			}
		}
		if (foundOpen && foundClose)
		{
			lineDanger = true;
		}

		// now that we've determined if we're in a comment or not, check the strings
		if (!commented)
		{
			std::smatch simple;
			auto match = std::regex_search(line, simple, simpleCapture);
			if (match)
			{
				if (lineDanger && !haveWarned)
				{
					LOG(WARN) << "Single-line '/* [] */' comments may disrupt parsing on line " << i + 1 << ". Please check output for validity";
					haveWarned = true;
				}
				std::smatch complex;
				auto fullMatch = std::regex_match(line, complex, fullCapture);
				if (fullMatch)
				{
					suspectedFields.emplace_back(complex[1], complex[2], complex[3], i);
				}
				else
				{
					std::string suspect = simple[1];
					auto jam = jammedTemplate(suspect);
					if (jam)
					{

						suspectedFields.emplace_back(jam.value().first, jam.value().second, simple[2], i);
					}
				}
			}

			auto nest = std::regex_search(line, simple, nestedCapture);
			if (nest)
			{
				suspectedNesteds.emplace_back(simple[1].str(), simple[2].str(), i);
			}
		}
	}

	LOG(DEBUG) << "     " << suspectedFields.size() << " possible fields detected";
	for (auto suspect : suspectedFields)
	{
		LOG(DEBUG) << " - " << suspect.Line << ": " << suspect.Type << " / " << suspect.Identifier << " / " << suspect.DefaultValue;
	}
	LOG(DEBUG) << suspectedNesteds.size() << " possible nestings detected";
	for (auto suspect : suspectedNesteds)
	{
		LOG(DEBUG) << " - " << suspect.Line << ": " << suspect.Type << " / " << suspect.Name;
	}
}
