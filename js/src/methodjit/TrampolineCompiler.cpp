







































#include "TrampolineCompiler.h"
#include "StubCalls.h"
#include "assembler/assembler/LinkBuffer.h"

namespace js {
namespace mjit {

#define CHECK_RESULT(x) if (!(x)) return false
#define COMPILE(which, pool, how) CHECK_RESULT(compileTrampoline(&(which), &pool, how))
#define RELEASE(which, pool) JS_BEGIN_MACRO \
    which = NULL;                           \
    if (pool)                               \
        pool->release();                    \
    pool = NULL;                            \
JS_END_MACRO

typedef JSC::MacroAssembler::Address Address;
typedef JSC::MacroAssembler::Label Label;
typedef JSC::MacroAssembler::Jump Jump;
typedef JSC::MacroAssembler::ImmPtr ImmPtr;
typedef JSC::MacroAssembler::Imm32 Imm32;
typedef JSC::MacroAssembler::Address Address;

bool
TrampolineCompiler::compile()
{
#ifdef JS_METHODJIT_SPEW
    JMCheckLogging();
#endif

    COMPILE(trampolines->forceReturn, trampolines->forceReturnPool, generateForceReturn);
#if (defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)) || defined(_WIN64)
    COMPILE(trampolines->forceReturnFast, trampolines->forceReturnFastPool, generateForceReturnFast);
#endif

    return true;
}

void
TrampolineCompiler::release(Trampolines *tramps)
{
    RELEASE(tramps->forceReturn, tramps->forceReturnPool);
#if (defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)) || defined(_WIN64)
    RELEASE(tramps->forceReturnFast, tramps->forceReturnFastPool);
#endif
}

bool
TrampolineCompiler::compileTrampoline(Trampolines::TrampolinePtr *where, JSC::ExecutablePool **pool,
                                      TrampolineGenerator generator)
{
    Assembler masm;

    Label entry = masm.label();
    CHECK_RESULT(generator(masm));
    JS_ASSERT(entry.isValid());

    *pool = execPool->poolForSize(masm.size());
    if (!*pool)
        return false;

    JSC::LinkBuffer buffer(&masm, *pool);
    uint8 *result = (uint8*)buffer.finalizeCodeAddendum().dataLocation();
    masm.finalize(result);
    *where = JS_DATA_TO_FUNC_PTR(Trampolines::TrampolinePtr, result + masm.distanceOf(entry));

    return true;
}








bool
TrampolineCompiler::generateForceReturn(Assembler &masm)
{
    
    Jump noActObjs = masm.branchTest32(Assembler::Zero,
                                       Address(JSFrameReg, JSStackFrame::offsetOfFlags()),
                                       Imm32(JSFRAME_HAS_CALL_OBJ | JSFRAME_HAS_ARGS_OBJ));
    masm.stubCall(stubs::PutActivationObjects, NULL, 0);
    noActObjs.linkTo(masm.label(), &masm);

    



    masm.loadPtr(Address(JSFrameReg, JSStackFrame::offsetOfPrev()), Registers::ReturnReg);
    masm.storePtr(Registers::ReturnReg, FrameAddress(offsetof(VMFrame, regs.fp)));

    Address rval(JSFrameReg, JSStackFrame::offsetOfReturnValue());
    masm.loadValueAsComponents(rval, JSReturnReg_Type, JSReturnReg_Data);

    masm.restoreReturnAddress();

    masm.move(Registers::ReturnReg, JSFrameReg);
#ifdef DEBUG
    masm.storePtr(ImmPtr(JSStackFrame::sInvalidpc),
                  Address(JSFrameReg, JSStackFrame::offsetOfSavedpc()));
#endif

    masm.ret();
    return true;
}

#if (defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)) || defined(_WIN64)
bool
TrampolineCompiler::generateForceReturnFast(Assembler &masm)
{
#ifdef _WIN64
    masm.addPtr(Imm32(32), Registers::StackPointer);
#else
    
    
    masm.addPtr(Imm32(8), Registers::StackPointer);
#endif
    return generateForceReturn(masm);
}
#endif

} 
} 

