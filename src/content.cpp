#include "content.h"
#include <JSL.h>
#include <map>
#include <functional>
#include <regex>


std::string stringify(std::string_view word)
{
	if (word.empty()){return "\"\"";}
	return (word.starts_with('"') ? "" : "\"") + std::string{word} + (word.ends_with('"') ? "" : "\"");
}

bool captureBlock(std::vector<ContentBlock> contents, std::function<bool(ContentBlock block,std::string)> process)
{
	std::string capture = "";
	for (auto block: contents)
	{
		if (block.IsLeaf && block.Text.size() > 0)
		{
			capture = block.Text; //JSL::getLower(block.Text);
			LOG(DEBUG) << "\tCaptured element " << capture;
			continue;
		}
	
		if (!block.IsLeaf)
		{
			if ((capture == ""))
			{
				LOG(ERROR) << "Unheaded block found";
				return false;
			}

			bool success = process(block,capture);
			if (!success){return false;}
			capture = "";
		}
	}
	return true;
}


std::string ContentBlock::Flatten()
{
	if (IsLeaf) { return Text;};
   
	std::ostringstream os;
	for (auto & member: Members)
	{
		os << "{" << member.Flatten() << "}";
	}
	return os.str();
}

ParsedAggregator::ParsedAggregator(std::string_view name, ContentBlock contents)
{
	Name = name;
	LOG(DEBUG) << "Parsing aggregate " << name;
	std::vector<std::string> Expected = {"members","categories","commands"};
	std::set<std::string> Extracted;


	bool success = captureBlock(contents.Members,[&](auto block,std::string capture)
	{
		JSL::toLower(capture);
		if (JSL::contains(capture,Expected))
		{
			if (Extracted.contains(capture))
			{
				LOG(ERROR) << "Duplicate " << capture << " header inside " << name;
				return false;
			}
			Extracted.insert(capture);
			if (capture == "categories")
			{
				return GetNested(block);
			}
			if (capture == "commands")
			{
				return GetCommands(block);
			}
			if (capture == "members")
			{
				return GetMembers(block);
			}
		}
		else
		{
			LOG(ERROR) << "Unknown block header: '" << capture << "'";
			return false;
		}
		return true;
	});

	if (!success)
	{
		Initialised = false;
		return;
	}

	Initialised = true;
}

void ParsedAggregator::Print(int idt)
{
	std::string indent = std::string(idt,'\t');
	std::string indent_p1 = std::string(idt+1,'\t');
	std::string indent_p2 = std::string(idt+2,'\t');
	std::string indent_p3 = std::string(idt+3,'\t');

	std::cout << indent << Name << "\n";
	std::cout << indent << "{\n";

	if (Members.size() > 0)
	{
		std::cout << indent_p1 << "Members\n";
		std::cout << indent_p1 << "{\n";
		for (auto & cmd :  Members)
		{
			std::cout << indent_p2 << cmd.Type <<" "  << cmd.Name << "\n";
			std::cout << indent_p2 << "{\n";
			std::cout << indent_p3 << "Default = " << cmd.Default <<"\n";
			std::cout << indent_p3 << "Trigger = " << cmd.Keys <<"\n";
			std::cout << indent_p3 << "Note = " << cmd.Note <<"\n";
			std::cout << indent_p2 << "}\n";
			// nest.Print(idt+2);
		}
		std::cout << indent_p1 << "}\n";
	}


	if (Nested.size() > 0)
	{
		std::cout << indent_p1 << "Categories\n";
		std::cout << indent_p1 << "{\n";
		for (auto & nest : Nested)
		{
			nest.Print(idt+2);
		}
		std::cout << indent_p1 << "}\n";
	}
	if (Commands.size() > 0)
	{
		std::cout << indent_p1 << "Commands\n";
		std::cout << indent_p1 << "{\n";
		for (auto & cmd :  Commands)
		{
			std::cout << indent_p2 << cmd.Name << "\n";
			std::cout << indent_p2 << "{\n";
			std::cout << indent_p3 << cmd.Description <<"\n";
			std::cout << indent_p2 << "}\n";
			// nest.Print(idt+2);
		}
		std::cout << indent_p1 << "}\n";
	}
	
	std::cout << indent << "}\n";


}

bool ParsedAggregator::GetNested(ContentBlock contents)
{
	LOG(DEBUG) << "\tParsing nesting";
	bool r = captureBlock(contents.Members,[&](auto block, std::string capture)
	{
		auto tmp = ParsedAggregator(capture,block);
		if (tmp.Initialised)
		{
			Nested.push_back(tmp);
			return true;
		}
		return false;
	});
	return r;
}
bool ParsedAggregator::GetCommands(ContentBlock contents)
{
	LOG(DEBUG) << "\tParsing commands";
	bool r= captureBlock(contents.Members,[&](auto block, std::string capture)
	{
		std::ostringstream os;
		bool first = true;
		for (auto b : block.Members)
		{
			if (b.IsLeaf)
			{
				if (JSL::trim_view(b.Text).size() != 0)
				{
					if (!first){ os << "\\n";};
					os << b.Text;
					first = false;
				}
			}
		}
		Commands.emplace_back(capture,stringify(os.str()));
		return true;
	});
	return r;
}

bool ParsedAggregator::GetMembers(ContentBlock contents)
{
	LOG(DEBUG) << "\tParsing commands";
	bool r= captureBlock(contents.Members,[&](auto block, std::string capture)
	{

		std::deque<std::string> stream;
		for (auto & member : block.Members)
		{
			if (member.IsLeaf || stream.empty()) {
				std::string s{JSL::trim(member.Flatten())};
				if (!s.empty())
				{
					stream.push_back(member.Flatten());
				}
			}
			else
			{
				stream.back() += " " + member.Flatten();
			}
		}
		
		auto tmp = ParsedSetting(capture,stream);
		if (tmp.Initialised)
		{
			Members.push_back(tmp);
			return true;
		}

		LOG(ERROR) << "Could not initialise settings member " << capture;
		return false;
	});
	return r;
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

	Initialised = true;
}

std::vector<std::string> & ParsedSetting::RegisteredKeys()
{
	static std::vector<std::string> reg{};
	return reg;
}