





#ifndef vm_Symbol_h
#define vm_Symbol_h

#include "mozilla/Attributes.h"

#include "gc/Barrier.h"

#include "js/RootingAPI.h"
#include "js/TypeDecls.h"

namespace JS {

class Symbol : public js::gc::BarrieredCell<Symbol>
{
  private:
    uint32_t unused1_;  
    JSAtom *description_;

    
    
    
    uint64_t unused2_;

    explicit Symbol(JSAtom *desc) : description_(desc) {}
    Symbol(const Symbol &) MOZ_DELETE;
    void operator=(const Symbol &) MOZ_DELETE;

  public:
    static Symbol *new_(js::ExclusiveContext *cx, JSString *description);

    JSAtom *description() const { return description_; }

    static inline js::ThingRootKind rootKind() { return js::THING_ROOT_SYMBOL; }
    inline void markChildren(JSTracer *trc);
    inline void finalize(js::FreeOp *) {}
};

} 

#endif 
