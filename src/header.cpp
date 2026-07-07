#include "header.h"
#include "Class.h"
#include "JSL/IO/GetFile.h"
#include "Settings.h"
#include <JSL/Log.h>
#include <JSL/Strings/Trim.h>
#include <regex>
HeaderFile::HeaderFile(fs::path file) : File(file)
{

	LOG(INFO) << " - Scanning " << file;

	Lines = JSL::IO::getFileLines(file);
	FindClasses();
}

/* The groups are:
	1. class name
	2. public declaration (optional - causes error if not present)
	3. JSL::Interface namespace (optional)
	4. CRTP template (optional - causes error if not present)
	5. same-line open brace (optional)
*/

std::regex classFinder(R"(^\s*class (\S*)\s*:\s*(public )?(\S*)?Aggregator\s*(<\S*>)?\s*(\{)?)");
void HeaderFile::FindClasses()
{
	size_t i = 0;
	std::vector<std::string> foundNames;
	while (i < Lines.size())
	{
		std::smatch classcatch;
		auto match = std::regex_search(Lines[i], classcatch, classFinder);
		if (match)
		{
			// now scan through to find the matching "};" for the end class
			int depth = 0;
			int lineEnd = -1;
			int lineStart = i;
			size_t j = classcatch[0].str().size() - 1;
			while (i < Lines.size() && lineEnd < 0)
			{
				while (j < Lines[i].size())
				{
					char ltr = Lines[i][j];
					if (ltr == '{')
					{
						depth += 1;
					}
					if (ltr == '}')
					{
						depth -= 1;
						if (depth < 0)
						{
							std::ostringstream os;
							os << "Class '" << classcatch[1] << "' at " << File.string() << ":" << i << " has mismatched braces.";
							throw std::runtime_error(os.str());
						}
						if (depth == 0)
						{
							lineEnd = i;
							break;
						}
					}
					++j;
				}
				++i;
				j = 0;
			}
			if (lineEnd < 0)
			{
				std::ostringstream os;
				os << "Class '" << classcatch[1] << "' at " << File.string() << ":" << lineStart << " has a badly formed body";
				throw std::runtime_error(os.str());
			}

			// Classes.emplace_back(classcatch, lineStart, lineEnd);
			ClassDeclare newClass(classcatch, File, Lines, lineStart, lineEnd);
			if (newClass.IsJSLConfigurable)
			{
				Classes[newClass.Name] = newClass;
			}
		}
		else
		{
			++i;
		}
	}
}
void HeaderFile::RegisterClassNames(std::vector<std::string> &registry)
{
	for (auto &[_, c] : Classes)
	{
		registry.push_back(c.Name);
	}
}
size_t HeaderFile::GetFields(const std::vector<std::string> &registry)
{
	size_t c = 0;
	for (auto &[name, obj] : Classes)
	{
		c += obj.GetFields(registry);
	}
	return c;
}

void HeaderFile::PrepareOutput()
{
	LOG(INFO) << " - " << File.string() << " classes:";
	std::map<std::string, std::string> tweaks;
	for (auto &[name, obj] : Classes)
	{
		auto tweaked = obj.WriteToFile();
		if (tweaked)
		{
			tweaks[name] = tweaked.value();
		}
	}

	if (!tweaks.empty())
	{
		auto hidden = File.parent_path() / ("." + File.filename().string() + ".bak");
		LOG(WARN) << "   = Altering source code; backup can be found at " << hidden.string();
		if (std::filesystem::exists(hidden))
		{
			std::filesystem::remove(hidden);
		}
		std::filesystem::copy(File, hidden);
		std::ostringstream os;
		for (size_t i = 0; i < Lines.size(); ++i)
		{
			int jumpto = -1;
			for (auto [name, tweak] : tweaks)
			{
				if (Classes[name].StartLine == (int)i)
				{
					os << tweak << "\n";
					jumpto = Classes[name].EndLine;
					break;
				}
			}
			if (jumpto > 0)
			{
				i = jumpto;
			}
			else
			{
				os << Lines[i] << "\n";
			}
		}

		JSL::IO::writeString(File, os.str());
	}
}
