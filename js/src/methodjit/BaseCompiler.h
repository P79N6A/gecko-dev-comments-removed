






































#if !defined jsjaeger_compilerbase_h__ && defined JS_METHODJIT
#define jsjaeger_compilerbase_h__

#include "jscntxt.h"
#include "jstl.h"
#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/LinkBuffer.h"
#include "assembler/assembler/RepatchBuffer.h"
#include "assembler/jit/ExecutableAllocator.h"
#include <limits.h>

#if defined JS_CPU_ARM
# define POST_INST_OFFSET(__expr) ((__expr) - sizeof(ARMWord))
#else
# define POST_INST_OFFSET(__expr) (__expr)
#endif

namespace js {
namespace mjit {

struct MacroAssemblerTypedefs {
    typedef JSC::MacroAssembler::Label Label;
    typedef JSC::MacroAssembler::Imm32 Imm32;
    typedef JSC::MacroAssembler::ImmPtr ImmPtr;
    typedef JSC::MacroAssembler::RegisterID RegisterID;
    typedef JSC::MacroAssembler::FPRegisterID FPRegisterID;
    typedef JSC::MacroAssembler::Address Address;
    typedef JSC::MacroAssembler::BaseIndex BaseIndex;
    typedef JSC::MacroAssembler::AbsoluteAddress AbsoluteAddress;
    typedef JSC::MacroAssembler MacroAssembler;
    typedef JSC::MacroAssembler::Jump Jump;
    typedef JSC::MacroAssembler::JumpList JumpList;
    typedef JSC::MacroAssembler::Call Call;
    typedef JSC::MacroAssembler::DataLabelPtr DataLabelPtr;
    typedef JSC::MacroAssembler::DataLabel32 DataLabel32;
    typedef JSC::FunctionPtr FunctionPtr;
    typedef JSC::RepatchBuffer RepatchBuffer;
    typedef JSC::CodeLocationLabel CodeLocationLabel;
    typedef JSC::CodeLocationDataLabel32 CodeLocationDataLabel32;
    typedef JSC::CodeLocationJump CodeLocationJump;
    typedef JSC::CodeLocationCall CodeLocationCall;
    typedef JSC::CodeLocationInstruction CodeLocationInstruction;
    typedef JSC::ReturnAddressPtr ReturnAddressPtr;
    typedef JSC::MacroAssemblerCodePtr MacroAssemblerCodePtr;
    typedef JSC::JITCode JITCode;
#if defined JS_CPU_ARM
    typedef JSC::ARMWord ARMWord;
#endif
};

class BaseCompiler : public MacroAssemblerTypedefs
{
  protected:
    JSContext *cx;

  public:
    BaseCompiler() : cx(NULL)
    { }

    BaseCompiler(JSContext *cx) : cx(cx)
    { }
};




class LinkerHelper : public JSC::LinkBuffer
{
  protected:
    Assembler &masm;
#ifdef DEBUG
    bool verifiedRange;
#endif

  public:
    LinkerHelper(Assembler &masm, JSC::CodeKind kind) : JSC::LinkBuffer(kind)
        , masm(masm)
#ifdef DEBUG
        , verifiedRange(false)
#endif
    { }

    ~LinkerHelper() {
        JS_ASSERT(verifiedRange);
    }

    bool verifyRange(const JSC::JITCode &other) {
#ifdef DEBUG
        verifiedRange = true;
#endif
#ifdef JS_CPU_X64
        uintptr_t lowest = JS_MIN(uintptr_t(m_code), uintptr_t(other.start()));

        uintptr_t myEnd = uintptr_t(m_code) + m_size;
        uintptr_t otherEnd = uintptr_t(other.start()) + other.size();
        uintptr_t highest = JS_MAX(myEnd, otherEnd);

        return (highest - lowest < INT_MAX);
#else
        return true;
#endif
    }

    bool verifyRange(JITScript *jit) {
        return verifyRange(JSC::JITCode(jit->code.m_code.executableAddress(), jit->code.m_size));
    }

    JSC::ExecutablePool *init(JSContext *cx) {
        
        
        JSScript *script = cx->fp()->script();
        JSC::ExecutableAllocator *allocator = script->compartment()->jaegerCompartment()->execAlloc();
        JSC::ExecutablePool *pool;
        m_code = executableAllocAndCopy(masm, allocator, &pool);
        if (!m_code) {
            js_ReportOutOfMemory(cx);
            return NULL;
        }
        m_size = masm.size();   
        return pool;
    }

    JSC::CodeLocationLabel finalize() {
        masm.finalize(*this);
        return finalizeCodeAddendum();
    }

    void maybeLink(MaybeJump jump, JSC::CodeLocationLabel label) {
        if (!jump.isSet())
            return;
        link(jump.get(), label);
    }

    size_t size() const {
        return m_size;
    }
};

class NativeStubLinker : public LinkerHelper
{
  public:
#ifdef JS_CPU_X64
    typedef JSC::MacroAssembler::DataLabelPtr FinalJump;
#else
    typedef JSC::MacroAssembler::Jump FinalJump;
#endif

    NativeStubLinker(Assembler &masm, JITScript *jit, jsbytecode *pc, FinalJump done)
        : LinkerHelper(masm, JSC::METHOD_CODE), jit(jit), pc(pc), done(done)
    {}

    bool init(JSContext *cx);

    void patchJump(JSC::CodeLocationLabel target) {
#ifdef JS_CPU_X64
        patch(done, target);
#else
        link(done, target);
#endif
    }

  private:
    JITScript *jit;
    jsbytecode *pc;
    FinalJump done;
};

bool
NativeStubEpilogue(VMFrame &f, Assembler &masm, NativeStubLinker::FinalJump *result,
                   int32 initialFrameDepth, int32 vpOffset,
                   MaybeRegisterID typeReg, MaybeRegisterID dataReg);













#ifdef JS_CPU_ARM
template <size_t reservedSpace>
class AutoReserveICSpace {
    typedef Assembler::Label Label;

    Assembler           &masm;
    bool                didCheck;
    bool                *overflowSpace;
    int                 flushCount;

  public:
    AutoReserveICSpace(Assembler &masm, bool *overflowSpace)
        : masm(masm), didCheck(false), overflowSpace(overflowSpace)
    {
        masm.ensureSpace(reservedSpace);
        flushCount = masm.flushCount();
    }

    

    void check() {
        JS_ASSERT(!didCheck);
        didCheck = true;

        if (masm.flushCount() != flushCount)
            *overflowSpace = true;
    }

    ~AutoReserveICSpace() {
        
        if (!didCheck) {
            check();
        }
    }
};

# define RESERVE_IC_SPACE(__masm)       AutoReserveICSpace<256> arics(__masm, &this->overflowICSpace)
# define CHECK_IC_SPACE()               arics.check()




# define RESERVE_OOL_SPACE(__masm)      AutoReserveICSpace<2048> arics_ool(__masm, &this->overflowICSpace)



# define CHECK_OOL_SPACE()              arics_ool.check()
#else
# define RESERVE_IC_SPACE(__masm)
# define CHECK_IC_SPACE()
# define RESERVE_OOL_SPACE(__masm)
# define CHECK_OOL_SPACE()
#endif

} 
} 

#endif
