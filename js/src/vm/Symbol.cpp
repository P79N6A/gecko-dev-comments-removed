





#include "vm/Symbol.h"

#include "jscntxt.h"
#include "jscompartment.h"

#include "gc/Rooting.h"

#include "jscompartmentinlines.h"
#include "jsgcinlines.h"

using JS::Symbol;
using namespace js;

Symbol *
Symbol::newInternal(ExclusiveContext *cx, JS::SymbolCode code, JSAtom *description)
{
    MOZ_ASSERT(cx->compartment() == cx->atomsCompartment());
    MOZ_ASSERT(cx->atomsCompartment()->runtimeFromAnyThread()->currentThreadHasExclusiveAccess());

    
    Symbol *p = gc::AllocateNonObject<Symbol, NoGC>(cx);
    if (!p) {
        js_ReportOutOfMemory(cx);
        return nullptr;
    }
    return new (p) Symbol(code, description);
}

Symbol *
Symbol::new_(ExclusiveContext *cx, JS::SymbolCode code, JSString *description)
{
    RootedAtom atom(cx);
    if (description) {
        atom = AtomizeString(cx, description);
        if (!atom)
            return nullptr;
    }

    
    
    AutoLockForExclusiveAccess lock(cx);
    AutoCompartment ac(cx, cx->atomsCompartment());
    return newInternal(cx, code, atom);
}

Symbol *
Symbol::for_(js::ExclusiveContext *cx, HandleString description)
{
    JSAtom *atom = AtomizeString(cx, description);
    if (!atom)
        return nullptr;

    AutoLockForExclusiveAccess lock(cx);

    SymbolRegistry &registry = cx->symbolRegistry();
    SymbolRegistry::AddPtr p = registry.lookupForAdd(atom);
    if (p)
        return *p;

    AutoCompartment ac(cx, cx->atomsCompartment());
    Symbol *sym = newInternal(cx, SymbolCode::InSymbolRegistry, atom);
    if (!sym)
        return nullptr;

    
    
    if (!registry.add(p, sym)) {
        
        js_ReportOutOfMemory(cx);
        return nullptr;
    }
    return sym;
}

#ifdef DEBUG
void
Symbol::dump(FILE *fp)
{
    if (isWellKnownSymbol()) {
        
        description_->dumpCharsNoNewline(fp);
    } else if (code_ == SymbolCode::InSymbolRegistry || code_ == SymbolCode::UniqueSymbol) {
        fputs(code_ == SymbolCode::InSymbolRegistry ? "Symbol.for(" : "Symbol(", fp);

        if (description_)
            description_->dumpCharsNoNewline(fp);
        else
            fputs("undefined", fp);

        fputc(')', fp);

        if (code_ == SymbolCode::UniqueSymbol)
            fprintf(fp, "@%p", (void *) this);
    } else {
        fprintf(fp, "<Invalid Symbol code=%u>", unsigned(code_));
    }
}
#endif  

void
SymbolRegistry::sweep()
{
    for (Enum e(*this); !e.empty(); e.popFront()) {
        Symbol *sym = e.front();
        if (IsSymbolAboutToBeFinalized(&sym))
            e.removeFront();
    }
}
