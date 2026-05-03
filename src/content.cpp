#include "content.h"
#include <JSL.h>
#include <map>
#include <functional>
#include <regex>
#include "trace.h"

bool captureBlock(std::vector<ContentBlock> contents, std::function<bool(ContentBlock block,std::string)> process)
{
	std::string capture = "";
	for (auto block: contents)
	{
		if (block.IsLeaf && block.Text.size() > 0)
		{
			capture = JSL::trim(block.Text,"//"); //JSL::getLower(block.Text);
			LOG(DEBUG) << "\tCaptured element " << capture;
			continue;
		}
	
		if (!block.IsLeaf)
		{
			if ((capture == ""))
			{
				TRACE << "Unheaded block found";
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

void ParsedAggregator::GlobalWarn(bool isRoot)
{
	if (IsGlobal && !isRoot)
	{
		LOG(WARN) << Name << " is marked global but is not root: this will be ignored";
	}

	for (auto & sub : Nested)
	{
		sub.GlobalWarn(false);
	}
}

ParsedAggregator::ParsedAggregator(std::string_view name, ContentBlock contents)
{
	LOG(DEBUG) << "Parsing aggregate " << name;
	auto ns = JSL::split_view(name,":");
	name = JSL::trim_view(ns[0]);
	if (ns.size() > 1)
	{
		if (JSL::iEquals(JSL::trim_view(ns[1]),"global"))
		{
			IsGlobal = true;
		}
	}
	
	if (JSL::split_view(name," ").size() > 1)
	{
		TRACE << "Aggregator names cannot contain spaces (" << name <<")";
		return;
	}
	Name = name;

	std::vector<std::string> Expected = {"members","categories","commands"};
	std::set<std::string> Extracted;


	bool success = captureBlock(contents.Members,[&](auto block,std::string capture)
	{
		JSL::toLower(capture);
		if (JSL::contains(capture,Expected))
		{
			if (Extracted.contains(capture))
			{
				TRACE << "Duplicate " << capture << " header inside " << name;
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
			TRACE << "Unknown block header: '" << capture << "'";
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


		auto sv = JSL::split_view(capture,":");
		capture = JSL::trim_view(sv[0]);
		bool isdefault = false;
		if (sv.size() > 1)
		{
			auto cmd = JSL::trim(sv[1]);
			if (cmd == "default")
			{
				isdefault = true;
			}
		}

		if (JSL::split_view(capture," ").size() > 1)
		{
			TRACE << "Command names cannot contain spaces";
			return false;
		}
		Commands.emplace_back(capture,stringify(os.str()),isdefault);
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

		TRACE << "Could not initialise settings member " << capture;
		return false;
	});
	return r;
}

std::string ParsedAggregator::MakeHeader(std::string parentName)
{
	bool IsRoot = (parentName.size() == 0);
	std::ostringstream os;

	if (IsRoot)
	{
		os << "#pragma once\n#include <JSL/Parameters.h>\n\n";
	}
	std::string myName = parentName + Name;
	
	if (Name.find("Obj") == std::string::npos)
	{
		myName += "Obj";
	} 

	ObjectName = myName;
	parentName += Name + "_";
	for (auto & el : Nested)
	{
		os << el.MakeHeader(parentName) << "\n";
	}

	os << "class " << myName <<" : public ";
	if (IsRoot)
	{
		os << "JSL::Parameter::Aggregator\n";
	}
	else
	{
		os << "JSL::Parameter::NestedAggregator\n";
	}
	os << "{\n";
	os << "\tpublic:\n";

	if (Members.size() > 0) {os << "\t\t//Settings values\n";}
	for (auto & var : Members)
	{
		var.BasicDeclare(os);
	}

	if (Nested.size() > 0){os << "\t\t//Nested objects\n";}
	for (auto & var : Nested)
	{
		os << "\t\t" << var.ObjectName << " " << var.Name << ";\n";
	}

	os << "\n\t\t//Constructor: register & connect the variables\n";
	os << "\t\t" << ObjectName << "()\n\t\t{\n";
	auto idt = std::string(3,'\t');
	os << idt << "Name = " << stringify(Name) <<";\n";
	for (auto & var : Members)
	{
		var.Set(os);
	}
	for (auto & nest: Nested)
	{
		os << "\t\t\tNestGroup(" << nest.Name << "," << stringify(nest.Name) << ");\n";
	}

	for (auto & command:Commands)
	{
		os << idt << (command.IsDefault ? "DefaultCommand" : "AddCommand");
		
		os << "(" << stringify(command.Name) << ", " << command.Description <<");\n";
	}

	os << "\t\t}\n";
	if (Members.size() > 0)
	{	
		os << "\tprivate:\n";
		os << "\t\t//Hidden configurables\n";
	}

	for (auto & var : Members)
	{
		var.ConfigDeclare(os);
	}
	os << "};\n";

	if (IsGlobal && IsRoot)
	{
		os << "\n//declaration for a global variable";
		os << "\nextern " << ObjectName << " "  << Name <<";\n";
	}
	return os.str();

}

void ParsedAggregator::CommandSweep(ParsedAggregator * root)
{
	bool wasRoot = false;
	if (root)
	{
		if (Commands.size() > 0)
		{
			LOG(WARN) << "Commands assigned to " << Name << " are for presentation only.\nIn the final output they will be attributed to " << root->Name;
		}
		for (auto & cmd : Commands)
		{
			root->OfferCommand(cmd);
		}
	}
	else
	{
		wasRoot = true;
		//process called by root
		root = this;
	}

	for (auto & nest : Nested)
	{
		nest.CommandSweep(root);
	}


	if (wasRoot)
	{
		bool defaultFound = false;
		for (auto & cmd: Commands)
		{
			if (cmd.IsDefault)
			{
				if (defaultFound)
				{
					LOG(WARN) << "Multiple default commands set. " << cmd.Name << " is no longer default";
					cmd.IsDefault = false;
				}

				defaultFound = true;
			}
		}
	}
}

void ParsedAggregator::OfferCommand(ParsedCommand & cmd)
{
	for (auto & old : Commands)
	{
		if (cmd.Name == old.Name)
		{
			return;
		}
	}
	Commands.push_back(cmd);
}