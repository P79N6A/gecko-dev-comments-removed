





#ifndef jit_AsmJSFrameIterator_h
#define jit_AsmJSFrameIterator_h

#include "mozilla/NullPtr.h"

#include <stdint.h>

#include "js/ProfilingFrameIterator.h"

class JSAtom;
struct JSContext;

namespace js {

class AsmJSActivation;
class AsmJSModule;
struct AsmJSFunctionLabels;
namespace jit { class CallSite; class MacroAssembler; class Label; }





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





enum AsmJSExitReason
{
    AsmJSNoExit,
    AsmJSFFI,
    AsmJSInterrupt
};




class AsmJSProfilingFrameIterator
{
    const AsmJSModule *module_;
    uint8_t *callerFP_;
    void *callerPC_;
    AsmJSExitReason exitReason_;

    
    
    const void *codeRange_;

    void initFromFP(const AsmJSActivation &activation);

  public:
    AsmJSProfilingFrameIterator() : codeRange_(nullptr) {}
    AsmJSProfilingFrameIterator(const AsmJSActivation &activation);
    AsmJSProfilingFrameIterator(const AsmJSActivation &activation,
                                const JS::ProfilingFrameIterator::RegisterState &state);
    void operator++();
    bool done() const { return !codeRange_; }

    typedef JS::ProfilingFrameIterator::Kind Kind;
    Kind kind() const;

    JSAtom *functionDisplayAtom() const;
    const char *functionFilename() const;
    unsigned functionLine() const;

    const char *nonFunctionDescription() const;
};




void
GenerateAsmJSFunctionPrologue(jit::MacroAssembler &masm, unsigned framePushed,
                              AsmJSFunctionLabels *labels);
void
GenerateAsmJSFunctionEpilogue(jit::MacroAssembler &masm, unsigned framePushed,
                              AsmJSFunctionLabels *labels);
void
GenerateAsmJSStackOverflowExit(jit::MacroAssembler &masm, jit::Label *overflowExit,
                               jit::Label *throwLabel);

void
GenerateAsmJSEntryPrologue(jit::MacroAssembler &masm, jit::Label *begin);
void
GenerateAsmJSEntryEpilogue(jit::MacroAssembler &masm);

void
GenerateAsmJSExitPrologue(jit::MacroAssembler &masm, unsigned framePushed, AsmJSExitReason reason,
                          jit::Label *begin);
void
GenerateAsmJSExitEpilogue(jit::MacroAssembler &masm, unsigned framePushed, AsmJSExitReason reason,
                          jit::Label *profilingReturn);

} 

#endif 
