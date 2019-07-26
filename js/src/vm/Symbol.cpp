





#include "vm/Symbol.h"

#include "jscntxt.h"
#include "jscompartment.h"

#include "gc/Rooting.h"

#include "jscompartmentinlines.h"
#include "jsgcinlines.h"

using JS::Symbol;
using namespace js;

Symbol *
Symbol::new_(ExclusiveContext *cx, JSString *description)
{
    RootedAtom atom(cx);
    if (description) {
        atom = AtomizeString(cx, description);
        if (!atom)
            return nullptr;
    }

    
    
    AutoLockForExclusiveAccess lock(cx);
    AutoCompartment ac(cx, cx->atomsCompartment());

    
    Symbol *p = gc::AllocateNonObject<Symbol, NoGC>(cx);
    if (!p) {
        js_ReportOutOfMemory(cx);
        return nullptr;
    }
    return new (p) Symbol(atom);
}
