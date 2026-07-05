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

	std::string word = (out.size() > 1) ? "files " : "file ";
	LOG(INFO) << "Processing " << word << JSL::String::stitch(out, ", ");
	return out;
}

int main(int argc, char **argv)
{
	Initialise(argc, argv);
	auto targets = GetTargets();

	for (auto f : targets)
	{
		auto file = HeaderFile(f);
	}
}
