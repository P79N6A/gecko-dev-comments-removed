





#ifndef vm_Symbol_h
#define vm_Symbol_h

#include "mozilla/Attributes.h"

#include "jsalloc.h"

#include "gc/Barrier.h"

#include "js/RootingAPI.h"
#include "js/TypeDecls.h"

namespace JS {

class Symbol : public js::gc::BarrieredCell<Symbol>
{
  private:
    
    
    uint32_t inSymbolRegistry_;
    JSAtom *description_;

    
    
    
    uint64_t unused2_;

    Symbol(bool inRegistry, JSAtom *desc)
        : inSymbolRegistry_(inRegistry), description_(desc) {}

    Symbol(const Symbol &) MOZ_DELETE;
    void operator=(const Symbol &) MOZ_DELETE;

    static Symbol *
    newInternal(js::ExclusiveContext *cx, bool inRegistry, JSAtom *description);

  public:
    static Symbol *new_(js::ExclusiveContext *cx, bool inRegistry, JSString *description);
    static Symbol *for_(js::ExclusiveContext *cx, js::HandleString description);

    JSAtom *description() const { return description_; }
    bool isInSymbolRegistry() const { return inSymbolRegistry_; }

    static inline js::ThingRootKind rootKind() { return js::THING_ROOT_SYMBOL; }
    inline void markChildren(JSTracer *trc);
    inline void finalize(js::FreeOp *) {}
};

} 

namespace js {


struct HashSymbolsByDescription
{
    typedef JS::Symbol *Key;
    typedef JSAtom *Lookup;

    static HashNumber hash(Lookup l) {
        return HashNumber(reinterpret_cast<uintptr_t>(l));
    }
    static bool match(Key sym, Lookup l) {
        return sym->description() == l;
    }
};







typedef HashSet<JS::Symbol *, HashSymbolsByDescription, SystemAllocPolicy> SymbolHashSet;
















class SymbolRegistry : public SymbolHashSet
{
  public:
    SymbolRegistry() : SymbolHashSet() {}
    void sweep();
};

} 

#endif 
