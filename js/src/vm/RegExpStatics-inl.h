





#ifndef vm_RegExpStatics_inl_h
#define vm_RegExpStatics_inl_h

#include "vm/RegExpStatics.h"

namespace js {

inline void
RegExpStatics::setMultiline(JSContext *cx, bool enabled)
{
    aboutToWrite();
    if (enabled) {
        flags = RegExpFlag(flags | MultilineFlag);
        markFlagsSet(cx);
    } else {
        flags = RegExpFlag(flags & ~MultilineFlag);
    }
}

inline void
RegExpStatics::markFlagsSet(JSContext *cx)
{
    







    JS_ASSERT(this == cx->global()->getRegExpStatics());

    types::MarkTypeObjectFlags(cx, cx->global(), types::OBJECT_FLAG_REGEXP_FLAGS_SET);
}

inline void
RegExpStatics::reset(JSContext *cx, JSString *newInput, bool newMultiline)
{
    aboutToWrite();
    clear();
    pendingInput = newInput;
    setMultiline(cx, newMultiline);
    checkInvariants();
}

} 

#endif 
