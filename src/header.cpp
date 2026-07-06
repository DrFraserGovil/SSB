#include "header.h"
#include "Class.h"
#include "JSL/IO/GetFile.h"
#include <JSL/Log.h>
#include <JSL/Strings/Trim.h>
#include <regex>

HeaderFile::HeaderFile(fs::path file) : File(file)
{

	LOG(INFO) << " - Scanning " << file;

	Lines = JSL::IO::getFileLines(file);
	FindClasses();
}
void HeaderFile::Scan()
{
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
			ClassDeclare newClass(classcatch, lineStart, lineEnd);
			if (newClass.IsJSLConfigurable)
			{
				newClass.GetFields(Lines, foundNames);
				Classes[newClass.Name] = newClass;
				foundNames.push_back(newClass.Name);
			}
		}
		else
		{
			++i;
		}
	}
}
