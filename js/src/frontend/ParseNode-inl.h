





#ifndef frontend_ParseNode_inl_h
#define frontend_ParseNode_inl_h

#include "frontend/ParseNode.h"

#include "frontend/SharedContext.h"

namespace js {
namespace frontend {

inline PropertyName *
ParseNode::name() const
{
    JS_ASSERT(isKind(PNK_FUNCTION) || isKind(PNK_NAME));
    JSAtom *atom = isKind(PNK_FUNCTION) ? pn_funbox->function()->atom() : pn_atom;
    return atom->asPropertyName();
}

inline JSAtom *
ParseNode::atom() const
{
    JS_ASSERT(isKind(PNK_STRING));
    return pn_atom;
}

} 
} 

#endif 
