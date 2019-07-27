





#ifndef ds_IdValuePair_h
#define ds_IdValuePair_h

#include "NamespaceImports.h"

#include "js/Id.h"

namespace js {

struct IdValuePair
{
    jsid id;
    Value value;

    IdValuePair() {}
    explicit IdValuePair(jsid idArg)
      : id(idArg), value(UndefinedValue())
    {}
};

} 

#endif 
