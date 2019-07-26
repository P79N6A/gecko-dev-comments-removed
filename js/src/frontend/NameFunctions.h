





#ifndef frontend_NameFunctions_h
#define frontend_NameFunctions_h

#include "js/TypeDecls.h"

namespace js {

class ExclusiveContext;

namespace frontend {

class ParseNode;

bool
NameFunctions(ExclusiveContext *cx, ParseNode *pn);

} 
} 

#endif 
