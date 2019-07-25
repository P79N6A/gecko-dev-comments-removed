






#ifndef NameFunctions_h__
#define NameFunctions_h__

struct JSContext;

namespace js {
namespace frontend {

struct ParseNode;

bool
NameFunctions(JSContext *cx, ParseNode *pn);

} 
} 

#endif 
