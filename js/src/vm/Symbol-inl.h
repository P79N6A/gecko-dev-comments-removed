





#ifndef vm_Symbol_inl_h
#define vm_Symbol_inl_h

#include "vm/Symbol.h"

#include "gc/Marking.h"

#include "js/RootingAPI.h"

#include "jsgcinlines.h"

inline void
JS::Symbol::markChildren(JSTracer *trc)
{
    if (description_)
        MarkStringUnbarriered(trc, &description_, "description");
}

#endif 
