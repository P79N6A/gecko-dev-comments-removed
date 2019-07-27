





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

namespace AsmJSExit
{
    
    
    
    
    
    enum ReasonKind {
        Reason_None,
        Reason_FFI,
        Reason_Interrupt,
        Reason_Builtin
    };

    
    
    enum BuiltinKind {
        Builtin_ToInt32,
#if defined(JS_CODEGEN_ARM)
        Builtin_IDivMod,
        Builtin_UDivMod,
#endif
        Builtin_ModD,
        Builtin_SinD,
        Builtin_CosD,
        Builtin_TanD,
        Builtin_ASinD,
        Builtin_ACosD,
        Builtin_ATanD,
        Builtin_CeilD,
        Builtin_CeilF,
        Builtin_FloorD,
        Builtin_FloorF,
        Builtin_ExpD,
        Builtin_LogD,
        Builtin_PowD,
        Builtin_ATan2D,
        Builtin_Limit
    };

    
    
    typedef uint32_t Reason;

    static const uint32_t None = Reason_None;
    static const uint32_t FFI = Reason_FFI;
    static const uint32_t Interrupt = Reason_Interrupt;
    static inline Reason Builtin(BuiltinKind builtin) {
        return uint16_t(Reason_Builtin) | (uint16_t(builtin) << 16);
    }
    static inline ReasonKind ExtractReasonKind(Reason reason) {
        return ReasonKind(uint16_t(reason));
    }
    static inline BuiltinKind ExtractBuiltinKind(Reason reason) {
        JS_ASSERT(ExtractReasonKind(reason) == Reason_Builtin);
        return BuiltinKind(uint16_t(reason >> 16));
    }
}




class AsmJSProfilingFrameIterator
{
    const AsmJSModule *module_;
    uint8_t *callerFP_;
    void *callerPC_;
    AsmJSExit::Reason exitReason_;

    
    
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
GenerateAsmJSExitPrologue(jit::MacroAssembler &masm, unsigned framePushed, AsmJSExit::Reason reason,
                          jit::Label *begin);
void
GenerateAsmJSExitEpilogue(jit::MacroAssembler &masm, unsigned framePushed, AsmJSExit::Reason reason,
                          jit::Label *profilingReturn);


} 

#endif 
