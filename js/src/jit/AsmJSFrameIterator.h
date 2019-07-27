





#ifndef jit_AsmJSFrameIterator_h
#define jit_AsmJSFrameIterator_h

#include "mozilla/NullPtr.h"

#include <stdint.h>

class JSAtom;

namespace js {

class AsmJSActivation;
class AsmJSModule;
namespace jit { struct CallSite; }


class AsmJSFrameIterator
{
    const AsmJSModule *module_;
    const jit::CallSite *callsite_;
    uint8_t *sp_;

    void settle(uint8_t *returnAddress);

  public:
    explicit AsmJSFrameIterator(const AsmJSActivation *activation);
    void operator++();
    bool done() const { return !module_; }
    JSAtom *functionDisplayAtom() const;
    unsigned computeLine(uint32_t *column) const;
};

} 

#endif 
