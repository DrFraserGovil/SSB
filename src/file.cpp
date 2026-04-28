#include "file.h"
#include <JSL.h>
#include <stack>
#include "settings.h"
namespace fs = std::filesystem;
SSBFile::SSBFile(fs::path path) : Path(path), Contents{}
{
    bool success = ExtractContents();
    
    if (success)
    {
        success = ConstructAggregators();
    }

    // if (success)
    // {
    //     Print();
    // }
}


void SSBFile::Print()
{
    Contents.Print(0);
}


bool isPure(std::string_view line)
{
    if (line.find("}") != std::string_view::npos) return false;
    if (line.find("{") != std::string_view::npos) return false;

    return true;
}


bool SSBFile::ExtractContents()
{
    auto lines = JSL::getFileLines(Path);
    std::stack<ContentBlock *> parents;

    std::stack<char> quotes;

    parents.push(&Contents);

    for (size_t l = 0; l < lines.size(); ++l)
    {
        std::string_view line = lines[l];

        if (isPure(line))
        {
            parents.top()->AddLeaf(JSL::trim(line));
            continue;
        }

        size_t prevCol = 0;

        for (size_t col = 0; col < line.size(); ++col)
        {
            auto c = line[col];
            if (c == '"' || c == '\'')
            {
                if (quotes.empty() || c != quotes.top())
                {
                    quotes.push(c);
                }
                else
                {
                    quotes.pop();
                }
                
            }

            if ((c == '{' || c == '}') && quotes.empty())
            {
                if (col > prevCol)
                {
                    parents.top()->AddLeaf(JSL::trim(line.substr(prevCol,col-prevCol)));
                }
                prevCol = col+1;
                if (c == '{')
                {
                    parents.push(&parents.top()->Prepare());
                }
                else
                {
                    if (parents.size() > 1)
                    {
                        parents.pop();
                    }
                    else
                    {
                        LOG(ERROR) << "Mismatched braces found on line " << l << " of " << Path.filename();
                        return false;
                    }
                }

            }
            
        }
        auto remaining = JSL::trim(line.substr(prevCol,std::string_view::npos));
        if (remaining.size() > 0)
        {
            parents.top()->AddLeaf(remaining);
        }
    }
    if (parents.size() > 1)
    {
        LOG(ERROR) << "Unclosed braces found in " << Path.filename();
        return false;
    }
    return true;
}

bool SSBFile::ConstructAggregators()
{
    //check how many objects were loaded into the root
    std::vector<std::string> names;
    int nBlocks = 0;
    for (auto & l : Contents.Members)
    {
        if (l.IsLeaf)
        {
            std::string n{JSL::trim_view(JSL::split_view(l.Text,":")[0])};

            if (!n.empty())
            {
                if (JSL::contains(n,names))
                {
                    LOG(ERROR) << "Duplicate construct name found at root level: " << n;
                    return false;
                }
                names.push_back(n);
            }
        }
        else
        {
            ++nBlocks;
        }
    }

    
    if (names.size() > 1)
    {
        if (Settings.AutoRoot)
        {
            std::string proposedName = "SettingsWrapper";
            while (JSL::contains(proposedName,names))
            {
                proposedName += "Wrapper";
            }
            LOG(WARN) << "Wrapping " << JSL::MakeString(names) << " into a root aggregator: "  << proposedName;
            ContentBlock newC;
            newC.AddLeaf(proposedName);
            auto & n = newC.Prepare();
            n.AddLeaf("Categories");
            n.Add(Contents);
            Contents = newC;
            nBlocks = 1;
        }
        else
        {
            LOG(ERROR) << "Multiple top-level objects specified: cannot deduce entry point. Run with '-r' to rectify.";
            return false;
        }

    }


    if (nBlocks != 1)
    {
        LOG(ERROR) << Path.filename().string() << " is not a valid SSB file\n" << ((nBlocks == 0) ? "No blocks found " : "Unheaded blocks detected");
        return false;
    }

    //Here we have a single top-level object
    std::string top = Contents.Members[0].Text;
    Root = ParsedAggregator(top,Contents.Members[1]);
    if (Root.Initialised)
    {
        // Root.Print();
        std::cout << Root.MakeHeader("");
        return true;
    }
    return false;
}