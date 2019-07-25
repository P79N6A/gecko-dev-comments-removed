






































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
    LinkerHelper(Assembler &masm) : masm(masm)
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
        JSC::ExecutableAllocator *allocator = script->compartment->jaegerCompartment()->execAlloc();
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













#ifdef JS_CPU_ARM
template <size_t reservedSpace>
class AutoReserveICSpace {
    typedef Assembler::Label Label;

    Assembler           &masm;
#ifdef DEBUG
    Label               startLabel;
    bool                didCheck;
#endif

  public:
    AutoReserveICSpace(Assembler &masm) : masm(masm) {
        masm.ensureSpace(reservedSpace);
#ifdef DEBUG
        didCheck = false;

        startLabel = masm.label();

        
        masm.allowPoolFlush(false);

        JaegerSpew(JSpew_Insns, " -- BEGIN CONSTANT-POOL-FREE REGION -- \n");
#endif
    }

    

    void check() {
#ifdef DEBUG
        JS_ASSERT(!didCheck);
        didCheck = true;

        Label endLabel = masm.label();
        int spaceUsed = masm.differenceBetween(startLabel, endLabel);

        
        JaegerSpew(JSpew_Insns,
                   " -- END CONSTANT-POOL-FREE REGION: %u bytes used of %u reserved. -- \n",
                   spaceUsed, reservedSpace);

        
        JS_ASSERT(spaceUsed >= 0);
        JS_ASSERT(size_t(spaceUsed) <= reservedSpace);

        
        masm.allowPoolFlush(true);
#endif
    }

    ~AutoReserveICSpace() {
#ifdef DEBUG
        
        if (!didCheck) {
            check();
        }
#endif
    }
};

# define RESERVE_IC_SPACE(__masm)       AutoReserveICSpace<128> arics(__masm)
# define CHECK_IC_SPACE()               arics.check()




# define RESERVE_OOL_SPACE(__masm)      AutoReserveICSpace<256> arics_ool(__masm)



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
