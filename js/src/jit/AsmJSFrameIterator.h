





#ifndef jit_AsmJSFrameIterator_h
#define jit_AsmJSFrameIterator_h

#include "mozilla/NullPtr.h"

#include <stdint.h>

class JSAtom;
struct JSContext;

namespace js {

class AsmJSActivation;
class AsmJSModule;
namespace jit { struct CallSite; class MacroAssembler; class Label; }


class AsmJSFrameIterator
{
    const AsmJSModule *module_;
    const jit::CallSite *callsite_;
    uint8_t *fp_;

    
    
    const void *codeRange_;

    void settle();

  public:
    explicit AsmJSFrameIterator() : module_(nullptr) {}
    explicit AsmJSFrameIterator(const AsmJSActivation &activation);
    void operator++();
    bool done() const { return !fp_; }
    JSAtom *functionDisplayAtom() const;
    unsigned computeLine(uint32_t *column) const;
};




void
GenerateAsmJSFunctionPrologue(jit::MacroAssembler &masm, unsigned framePushed,
                              jit::Label *maybeOverflowThunk, jit::Label *overflowExit);
void
GenerateAsmJSFunctionEpilogue(jit::MacroAssembler &masm, unsigned framePushed,
                              jit::Label *maybeOverflowThunk, jit::Label *overflowExit);
void
GenerateAsmJSStackOverflowExit(jit::MacroAssembler &masm, jit::Label *overflowExit,
                               jit::Label *throwLabel);

void
GenerateAsmJSEntryPrologue(jit::MacroAssembler &masm);
void
GenerateAsmJSEntryEpilogue(jit::MacroAssembler &masm);

void
GenerateAsmJSFFIExitPrologue(jit::MacroAssembler &masm, unsigned framePushed);
void
GenerateAsmJSFFIExitEpilogue(jit::MacroAssembler &masm, unsigned framePushed);

} 

#endif 
