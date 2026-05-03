#include "parse.h"
#include "JSL.h"


std::string stringify(std::string_view word)
{
	if (word.empty()){return "\"\"";}
	return (word.starts_with('"') ? "" : "\"") + std::string{word} + (word.ends_with('"') ? "" : "\"");
}

std::string ParsedSetting::ProcessKeys()
{
	std::string_view val = Keys;
	auto & keyRegister = RegisteredKeys();
	std::ostringstream os;
	std::vector<char> badChar = {'{','}',']','[','(',')','-'};

	while(JSL::contains(val.front(),badChar)){
		val = val.substr(1);
	}
	while(JSL::contains(val.back(),badChar))
	{
		val = val.substr(0,val.size()-1);
	}

	auto v = JSL::split(val,",");
	for (auto & cmd : v)
	{
		cmd = stringify(JSL::trim_view(cmd));
		if (JSL::contains(std::string{cmd},keyRegister))
		{
			LOG(ERROR) << "The key " << cmd << " is already in use (Encountered in " << Type << " " << Name << ")";
			throw std::runtime_error("Duplicate key");
		}
		keyRegister.push_back(cmd);
	}
	os << "{" << JSL::join(v,", ") << "}";
	return os.str();
}

ParsedSetting::ParsedSetting(std::string_view name, std::deque<std::string> data)
{
	LOG(DEBUG) << "Parsing " << name;
	name = JSL::trim_view(name,"//");
	auto sp = JSL::split_view(name, " ");

	Name = sp.back();
	sp.pop_back();
	
	if (sp.size() > 0)
	{
		Type = JSL::join(sp," ");

		std::regex e("(^|[^:])\\b(vector|string)\\b");
		Type = std::regex_replace(Type,e,"$1std::$2");
	}
	else
	{
		LOG(ERROR) << "Could not locate type for parameter '" << name << "'";
		return;
	}
	
	for (auto el : data)
	{
		std::string_view cmd = JSL::trim_view(el,"//");
		if (cmd.empty()) { continue;}
		//remove spurious semicolons
		while (cmd.back() == ';')
		{
			cmd  = cmd.substr(0,cmd.size()-1);
		}
		auto splitter = cmd.find_first_of('=');


		if (splitter == std::string_view::npos)
		{
			LOG(ERROR) << "Setting specifications must take the value [key] = [value] (" << cmd << ")";
			return;
		}
		
		std::string_view key = JSL::trim_view(cmd.substr(0,splitter));
		std::string_view val = JSL::trim_view(cmd.substr(splitter+1)); //we don't want the equals

		LOG(DEBUG) << "Captured " << key << " = " << val << " for " << Name;
		if (JSL::iEquals(key,"note"))
		{
			Note = val;
			continue;
		}
		if (JSL::iEquals(key,"default"))
		{
			Default = val;
			continue;
		}
		if (JSL::iEquals(key,"trigger"))
		{
			Keys = val;
			continue;
		}

		LOG(ERROR) << "Unknown command '" << key << "' encountered in " << el;
		return;
	}

	if (Type == "toggle")
	{
		Type = "bool";
		Default = "false";
	}

	if (Default.size() == 0)
	{
		LOG(ERROR) << "No default value provided for " << Name;
		return;	
	}

	if (Note.size() == 0)
	{
		LOG(ERROR) << "No description provided for " << Name;
		return;
	}
	Note = stringify(Note);

	Keys = ProcessKeys();
	if (Keys.size() == 0)
	{	
		LOG(ERROR) << "No valid triggers provided for " << Name;
		return;
	}
	ConfigName = "config" + Name;
	Initialised = true;
}

std::vector<std::string> & ParsedSetting::RegisteredKeys()
{
	static std::vector<std::string> reg{};
	return reg;
}

void ParsedSetting::BasicDeclare(std::ostringstream & os)
{
	//remove quotes from note
	auto b = Note.find_first_not_of('"');
	auto e = Note.find_last_not_of('"');
	os << "\t\t//! @brief " << Note.substr(b,e-b) <<"\n";
	os << "\t\t" << Type <<" " << Name <<";\n\n";
}
void ParsedSetting::ConfigDeclare(std::ostringstream & os)
{
	os << "\t\tJSL::Parameter::Setting<" << Type <<"> " << ConfigName <<";\n";
}

void ParsedSetting::Set(std::ostringstream & os)
{
	os << "\t\t\tSet(" << Name << ", " << ConfigName << ", (" << Type << ")" << Default << ", " << Keys << ", " << stringify(Name) << ", " << Note << ");\n";
}