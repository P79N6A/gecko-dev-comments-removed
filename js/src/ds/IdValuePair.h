





#ifndef ds_IdValuePair_h
#define ds_IdValuePair_h

#include "NamespaceImports.h"

namespace js {

struct IdValuePair
{
    jsid id;
    Value value;

    IdValuePair() {}
    IdValuePair(jsid idArg)
      : id(idArg), value(UndefinedValue())
    {}
};

} 

#endif 
