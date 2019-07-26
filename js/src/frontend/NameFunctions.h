





#ifndef frontend_NameFunctions_h
#define frontend_NameFunctions_h

struct JSContext;

namespace js {
namespace frontend {

class ParseNode;

bool
NameFunctions(JSContext *cx, ParseNode *pn);

} 
} 

#endif 
