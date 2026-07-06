#include "Field.h"
#include "Settings.h"
#include <JSL.h>
JSLField::JSLField(std::string type, std::string id, std::string val, int line) : Type(type), Identifier(id), DefaultValue(val)
{
	Line = line;
}
Nested::Nested(std::string type, std::string id, int line) : Type(type), Name(id)
{
	Line = line;
}
