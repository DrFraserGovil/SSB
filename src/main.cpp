#include "Settings.h"
#include "header.h"
#include <JSL.h>
#include <filesystem>
#include <vector>
SettingsObj Settings;

void Initialise(int argc, char **argv)
{
	Settings.Parse(argc, argv);
	if (Settings.Verbose)
	{
		JSL::Log::Global().Level = DEBUG;
	}
	else if (Settings.Quiet)
	{
		JSL::Log::Global().Level = ERROR;
	}
	JSL::Log::Global().ShowHeaders = false;
	JSL::Log::Global().DebugColour = JSL::Display::Colour(60, 60, 130);
	LOG(DEBUG) << "LSJ Initialised";
}
namespace fs = std::filesystem;
std::vector<fs::path> GetTargets()
{
	std::vector<fs::path> out;
	std::vector<std::string> other;
	for (auto f : Settings.Commands)
	{
		fs::path file(f);
		if (fs::exists(file))
		{
			auto ext = file.extension();
			if (JSL::Vector::contains<std::string>(Settings.extensions, ext))
			{
				out.push_back(file);
			}
			else
			{
				LOG(WARN) << file << " is a file, but is not a C++ header-" << ext;
			}
		}
		else
		{
			other.push_back(f);
		}
	}

	if (!other.empty())
	{
		LOG(WARN) << "The following arguments are not real files:\n"
				  << JSL::String::stitch(other, ", ");
	}
	if (out.empty())
	{
		LOG(ERROR) << "No vaid target files were provided";
		exit(1);
	}

	return out;
}

std::map<std::string, std::string> globalAliases = {};
int main(int argc, char **argv)
{
	try
	{
		Initialise(argc, argv);

		auto targets = GetTargets();
		std::string word = (targets.size() > 1) ? "files " : "file ";
		LOG(INFO) << "Processing " << word << JSL::String::stitch(targets, ", ");
		// scan each file and scoop up the class definitions (but no more processing, as that requires the populated registry)
		std::vector<HeaderFile> headers;
		std::vector<std::string> registry;
		for (auto f : targets)
		{
			headers.emplace_back(f);
			headers.back().RegisterClassNames(registry);
		}

		// Scan each class file in each header, picking up JSL-field declares.
		//  The registry helps disambiguate nested classes; hence why it needed to be done first.
		LOG(INFO) << " - Found " << registry.size() << " classes in " << headers.size() << " file" << (headers.size() > 1 ? "s" : "") << "\n"
				  << "Beginning FieldScan";
		size_t count = 0;
		for (auto &header : headers)
		{
			LOG(INFO) << " - In file " << header.File.string() << ":";
			count += header.GetFields(registry);
		}

		LOG(INFO) << " - Found " << count << " valid JSL-fields.\nBeginning output writing";
		// Loop over the now-validated output and write it all to file
		for (auto &header : headers)
		{
			header.PrepareOutput();
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << "Critical error encountered\n"
				  << e.what();
		exit(1);
	}
}
