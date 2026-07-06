#include <JSL/Interface/Aggregator.h>

using namespace JSL::Interface;

class ModesObj : public Aggregator<ModesObj>
{
  public:
	bool ActiveFix = false;

	template <class T>
	void FieldList(T &&t)
	{
		t(Field(ActiveFix, "Active Fix", {"fix", "f"}, false, "If true, alters the source code when JSL errors are detected"));
	}
};

class SettingsObj : public JSL::Interface::Aggregator<SettingsObj>
{
  public:
	/*
	   int V = 1;
	   */
	bool Verbose = false;
	bool Quiet = false;
	std::vector<std::string> extensions = {".h", ".hpp"};

	ModesObj Modes;
	template <class T>
	void FieldList(T &&t)
	{
		t(JSL::Interface::Field(Verbose, "Verbose", {"v"}, false, "A verbosity measure"));
		t(JSL::Interface::Field(Quiet, "Quiet Mode", {"q"}, false, "A quietness measure"));
		t(Modes);
	}
};
extern SettingsObj Settings;
