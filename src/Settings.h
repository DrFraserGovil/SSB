#include <JSL/Interface/Aggregator.h>
#include <filesystem>

class SettingsObj : public JSL::Interface::Aggregator<SettingsObj>
{
  public:
	bool Verbose = false;
	bool Quiet = false;

	std::vector<std::string> extensions = {".h", ".hpp"};

	template <class T>
	void FieldList(T &&t)
	{
		t(JSL::Interface::Field(Verbose, "Verbose", {"v"}, false, "A verbosity measure"));
		t(JSL::Interface::Field(Quiet, "Quiet Mode", {"q"}, false, "A quietness measure"));
	}
};
extern SettingsObj Settings;
