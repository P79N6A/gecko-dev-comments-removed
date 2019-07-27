





#ifndef vm_Symbol_h
#define vm_Symbol_h

#include "mozilla/Attributes.h"

#include <stdio.h>

#include "jsalloc.h"
#include "jsapi.h"

#include "gc/Barrier.h"
#include "gc/Marking.h"

#include "js/RootingAPI.h"
#include "js/TypeDecls.h"

namespace JS {

class Symbol : public js::gc::TenuredCell
{
  private:
    SymbolCode code_;
    JSAtom* description_;

    
    
    
    uint64_t unused2_;

    Symbol(SymbolCode code, JSAtom* desc)
        : code_(code), description_(desc)
    {
        
        (void)unused2_;
    }

    Symbol(const Symbol&) = delete;
    void operator=(const Symbol&) = delete;

    static Symbol*
    newInternal(js::ExclusiveContext* cx, SymbolCode code, JSAtom* description);

  public:
    static Symbol* new_(js::ExclusiveContext* cx, SymbolCode code, JSString* description);
    static Symbol* for_(js::ExclusiveContext* cx, js::HandleString description);

    JSAtom* description() const { return description_; }
    SymbolCode code() const { return code_; }

    bool isWellKnownSymbol() const { return uint32_t(code_) < WellKnownSymbolLimit; }

    static inline js::ThingRootKind rootKind() { return js::THING_ROOT_SYMBOL; }
    inline void markChildren(JSTracer* trc) {
        if (description_)
            js::TraceManuallyBarrieredEdge(trc, &description_, "description");
    }
    inline void finalize(js::FreeOp*) {}

#ifdef DEBUG
    void dump(FILE* fp = stderr);
#endif
};

} 

namespace js {


struct HashSymbolsByDescription
{
    typedef JS::Symbol* Key;
    typedef JSAtom* Lookup;

    static HashNumber hash(Lookup l) {
        return HashNumber(reinterpret_cast<uintptr_t>(l));
    }
    static bool match(Key sym, Lookup l) {
        return sym->description() == l;
    }
};







typedef HashSet<ReadBarrieredSymbol, HashSymbolsByDescription, SystemAllocPolicy> SymbolHashSet;
















class SymbolRegistry : public SymbolHashSet
{
  public:
    SymbolRegistry() : SymbolHashSet() {}
    void sweep();
};

} 

namespace js {


bool
SymbolDescriptiveString(JSContext* cx, JS::Symbol* sym, JS::MutableHandleValue result);

bool
IsSymbolOrSymbolWrapper(JS::Value v);

JS::Symbol*
ToSymbolPrimitive(JS::Value v);

} 

#endif 
