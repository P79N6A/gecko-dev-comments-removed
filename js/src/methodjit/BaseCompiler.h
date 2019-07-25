






































#if !defined jsjaeger_compilerbase_h__ && defined JS_METHODJIT
#define jsjaeger_compilerbase_h__

#include "jscntxt.h"
#include "jstl.h"
#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/LinkBuffer.h"
#include "assembler/assembler/RepatchBuffer.h"
#include "assembler/jit/ExecutableAllocator.h"
#include <limits.h>

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
    typedef JSC::CodeLocationCall CodeLocationCall;
    typedef JSC::ReturnAddressPtr ReturnAddressPtr;
    typedef JSC::MacroAssemblerCodePtr MacroAssemblerCodePtr;
    typedef JSC::JITCode JITCode;
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

  protected:

    JSC::ExecutablePool *
    getExecPool(JSScript *script, size_t size) {
        return BaseCompiler::GetExecPool(cx, script, size);
    }

  public:
    static JSC::ExecutablePool *
    GetExecPool(JSContext *cx, JSScript *script, size_t size) {
        JaegerCompartment *jc = script->compartment->jaegerCompartment;
        JSC::ExecutablePool *pool = jc->poolForSize(size);
        if (!pool)
            js_ReportOutOfMemory(cx);
        return pool;
    }
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
        JSC::ExecutablePool *ep = BaseCompiler::GetExecPool(cx, script, masm.size());
        if (!ep)
            return ep;

        m_size = masm.size();
        m_code = executableCopy(masm, ep);
        if (!m_code) {
            ep->release();
            js_ReportOutOfMemory(cx);
            return NULL;
        }
        return ep;
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
class AutoReserveICSpace {
    typedef Assembler::Label Label;
    static const size_t reservedSpace = 68;

    Assembler           &masm;
#ifdef DEBUG
    Label               startLabel;
#endif

  public:
    AutoReserveICSpace(Assembler &masm) : masm(masm) {
        masm.ensureSpace(reservedSpace);
#ifdef DEBUG
        startLabel = masm.label();

        
        masm.allowPoolFlush(false);

        JaegerSpew(JSpew_Insns, " -- BEGIN CONSTANT-POOL-FREE REGION -- \n");
#endif
    }

    ~AutoReserveICSpace() {
#ifdef DEBUG
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
};
# define RESERVE_IC_SPACE(__masm) AutoReserveICSpace arics(__masm)
#else
# define RESERVE_IC_SPACE(__masm)
#endif

} 
} 

#endif
