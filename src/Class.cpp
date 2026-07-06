#include "Class.h"
#include "Settings.h"
#include <JSL.h>
#include <optional>

bool hasFieldList(std::vector<std::string> &lines, int a, int b)
{
	for (int line = a; line <= b; ++line)
	{
		auto t = JSL::String::trim_view(lines[line], "//");
		if (t.find("void FieldList") != t.npos)
		{
			return true;
		}
	}
	return false;
}

ClassDeclare::ClassDeclare(std::smatch grab, std::filesystem::path host, std::vector<std::string> &lines, int linestart, int lineend) : Host(host), Lines(lines)
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

	GenFile = Host.parent_path() / (Host.stem().string() + "." + Name + ".dat");
	GenFile.replace_extension(Settings.Strings.Suffix);
	IsJSLConfigurable = (IsPublic && IsNested) || Settings.Modes.ActiveFix;
	bool alreadyHasList = hasFieldList(lines, linestart, lineend);
	IsJSLConfigurable = IsJSLConfigurable && !alreadyHasList;

	if (!IsJSLConfigurable)
	{
		std::ostringstream os;
		os << "Ignoring class " << Name << "; because:";
		if (!IsPublic) { os << "\n\t* Aggregator inheritance not declared public"; }
		if (!IsNested) { os << "\n\t* Aggregator CRTP pattern not deployed"; }
		if (alreadyHasList) { os << "\n\t* Already has a FieldList member"; }
		else
		{
			os << "\n\tEnable active-fixing (--fix) to automatically resolve this issues";
		}
		LOG(WARN) << os.str();
	}
	GetSelfMeta();
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
std::regex includeCapture("^\\s*#include \"(.*)\"");
void ClassDeclare::GetFields(const std::vector<std::string> &possibleChildren)
{

	LOG(INFO) << JSL::Display::Colour(80, 180, 70) << "   = Scanning class " << Name << " [" << StartLine + 1 << "-" << EndLine + 1 << "]";
	std::vector<JSLField> suspectedFields;
	std::vector<Nested> suspectedNesteds;
	bool commented = false;
	bool haveWarned = false;
	HasInclude = false;
	bool lineDanger = false;
	for (int i = StartLine; i < EndLine; ++i)
	{
		auto line = JSL::String::trim(Lines[i], "//");
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
			std::smatch groups;
			if (std::regex_search(line, groups, simpleCapture))
			{
				if (lineDanger && !haveWarned)
				{
					LOG(WARN) << "Single-line '/* [] */' comments may disrupt parsing on line " << i + 1 << ". Please check output for validity";
					haveWarned = true;
				}
				auto fullMatch = std::regex_match(line, groups, fullCapture);
				if (fullMatch)
				{
					suspectedFields.emplace_back(groups[1], groups[2], groups[3], i);
				}
				else
				{
					std::string suspect = groups[1];
					auto jam = jammedTemplate(suspect);
					if (jam)
					{

						suspectedFields.emplace_back(jam.value().first, jam.value().second, groups[2], i);
					}
				}
			}

			if (std::regex_search(line, groups, nestedCapture))
			{
				suspectedNesteds.emplace_back(groups[1].str(), groups[2].str(), i);
			}

			if (std::regex_search(line, groups, includeCapture))
			{
				auto trim = JSL::String::trim(groups[1].str());
				if (trim == GenFile.filename().string())
				{
					HasInclude = true;
				}
			}
		}
	}

	std::string indent(5, ' ');

	// process nestings
	if (suspectedNesteds.size() > 0)
	{
		LOG(DEBUG) << indent << suspectedNesteds.size() << " possible nestings detected";
		for (auto suspect : suspectedNesteds)
		{
			auto reject = RejectNested(suspect, possibleChildren);
			if (reject)
			{
				LOG(DEBUG) << JSL::Display::Colour(120, 30, 30) << indent << "  - Ignored " << JSL::Display::Italics() << suspect.Type << " " << suspect.Name << JSL::Display::Italics(false) << " (" << reject.value() << ")";
				suspectedFields.emplace_back(suspect);
			}
			else
			{
				LOG(DEBUG) << indent << "  - Caught: " << JSL::Display::Italics() << suspect.Type << " " << suspect.Name;
				SubGroup.push_back(suspect);
			}
		}
	}
	// process fields
	if (suspectedFields.size() > 0)
	{
		LOG(DEBUG) << indent << suspectedFields.size() << " possible fields detected";
		for (auto suspect : suspectedFields)
		{
			auto reject = RejectFields(suspect);
			if (reject)
			{
				LOG(DEBUG) << JSL::Display::Colour(120, 30, 30) << indent << "  - Ignored " << JSL::Display::Italics() << suspect.Type << " " << suspect.Identifier << JSL::Display::Italics(false) << " (" << reject.value() << ")";
			}
			else
			{
				LOG(DEBUG) << indent << "  - Caught: " << JSL::Display::Italics() << suspect.Type << " " << suspect.Identifier;
				Fields.push_back(suspect);
				// SubGroup.push_back(suspect);
			}
		}
	}
}

std::optional<std::string> ClassDeclare::RejectFields(JSLField &suspected)
{
	return suspected.ProcessComments(CaptureComments(suspected.Line));
}
std::optional<std::string> ClassDeclare::RejectNested(Nested &suspected, const std::vector<std::string> &registry)
{
	if (JSL::Vector::contains(registry, suspected.Type))
	{
		return std::nullopt;
	}
	else
	{
		return "No matching aggregator found in list";
	}
}

std::regex nameCapture("@name\\s+(\\S.*)");
void ClassDeclare::GetSelfMeta()
{
	auto comments = CaptureComments(StartLine);
	DisplayName = Name;
	std::smatch groups;
	for (auto &cmt : comments)
	{
		if (std::regex_search(cmt, groups, nameCapture))
		{
			DisplayName = groups[1];
		}
	}
}

int findOpenMLC(int start, std::vector<std::string> &lines)
{
	int line = start;
	while (line >= 0)
	{
		auto clean = JSL::String::trim_view(lines[line]);
		if (line != start)
		{
			// if we find another close-mlc, then something very bad has happened, and there wont be a corresponding open
			auto crash = clean.find("*/");
			if (crash != clean.npos)
			{
				return start;
			}
		}
		auto find = clean.find("/*");
		if (find != clean.npos)
		{
			if (clean.substr(0, 2) != "//")
			{
				return line;
			}
		}
		--line;
	}
	return start; // if we got here, there was no close, so the MLC is never opened. It's either a syntax error, or a standlone comment
}

std::vector<std::string> ClassDeclare::CaptureComments(int start)
{
	int line = start - 1;
	std::vector<std::string> capture;
	// bool MLC = false;
	while (line >= 0)
	{
		auto string = JSL::String::trim_view(Lines[line]);

		if (string.size() < 2) // minum line size to carry comment info
		{
			return capture;
		}

		if (string.find("*/") != string.npos)
		{
			auto jump = findOpenMLC(line, Lines);
			capture.emplace_back(string);
			while (line > jump)
			{
				--line;
				string = JSL::String::trim_view(Lines[line]);
				capture.emplace_back(string);
			}
		}
		else
		{
			if (string.substr(0, 2) == "//")
			{
				capture.emplace_back(string);
			}
			else
			{
				return capture;
			}
		}
		--line;
	}
	return capture;
}
std::string ClassDeclare::Format(std::string tname, std::string iname)
{
	std::ostringstream os;
	os << "/* ================== WARNING ==================\n";
	os << "\t This code was autogenerated by the LSJ tool.\n";
	os << "\t Do not manually alter this code\n*/\n";
	os << "template<typename " << tname << ">\n"
	   << "void FieldList(" << tname << " &&" << iname << ")\n"
	   << "{\n"
	   << "\tName = \"" << DisplayName << "\";";

	for (auto &field : Fields)
	{
		os << "\n\t" << field.Format(iname, Namespace);
	}
	for (auto &nest : SubGroup)
	{
		os << "\n\t" << nest.Format(iname);
	}

	os << "\n}";

	return os.str();
}

std::optional<std::string> ClassDeclare::WriteToFile()
{
	auto data = Format(Settings.Strings.TemplateName, Settings.Strings.ObjectName);

	bool requiresOverwrite = true;
	if (std::filesystem::exists(GenFile))
	{
		auto oldData = JSL::IO::getFile(GenFile);
		requiresOverwrite = !(data == oldData);
	}

	if (requiresOverwrite)
	{
		LOG(DEBUG) << "Writing output to " << GenFile;
		JSL::IO::writeString(GenFile, data);
	}
	else
	{
		LOG(DEBUG) << "No change to data in " << GenFile;
	}

	bool sourceTweak = !(IsPublic && IsNested && HasInclude);
	if (sourceTweak)
	{
		return IdealForm();
	}
	else
	{
		return std::nullopt;
	}
}

std::string ClassDeclare::IdealForm()
{
	// LOG(ERROR) << Lines[StartLine]; auto sp = JSL::String::split_view(std::string_view inpuS
	std::string_view firstLine = Lines[StartLine];
	auto sp = JSL::String::split(firstLine, "//"); // capture comments
	auto dec = sp[0];
	std::string cmt;
	if (sp.size() > 1) { cmt = "\t//" + JSL::String::stitch(sp, 1, sp.size(), "//") + "\n"; }
	sp = JSL::String::split(dec, "{");
	dec = sp[0];
	auto rest = JSL::String::stitch(sp, 1, sp.size(), "{") + cmt;
	std::string extra = (sp.size() == 1) ? "\n" : "\n{\n";
	std::ostringstream os;
	os << "class " << Name << ": public " << Namespace << "Aggregator<" << Name << ">";
	os << extra
	   << rest;

	for (int j = StartLine + 1; j < EndLine; ++j)
	{
		os << Lines[j] << "\n";
	}

	if (!HasInclude)
	{
		os << "\t#include \"" << GenFile.filename().string() << "\"\n";
	}
	os << Lines[EndLine];
	return os.str();
}
