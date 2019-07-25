







































#include "TrampolineCompiler.h"
#include "StubCalls.h"
#include "assembler/assembler/LinkBuffer.h"

namespace js {
namespace mjit {

#define CHECK_RESULT(x) if (!(x)) return false
#define COMPILE(which, pool, how) CHECK_RESULT(compileTrampoline((void **)(&(which)), &pool, how))
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
TrampolineCompiler::compileTrampoline(void **where, JSC::ExecutablePool **pool,
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
    *where = result + masm.distanceOf(entry);

    return true;
}








bool
TrampolineCompiler::generateForceReturn(Assembler &masm)
{
    
    Jump noCallObj = masm.branchPtr(Assembler::Equal,
                                    Address(JSFrameReg, JSStackFrame::offsetCallObj()),
                                    ImmPtr(0));
    masm.stubCall(stubs::PutCallObject, NULL, 0);
    noCallObj.linkTo(masm.label(), &masm);

    
    Jump noArgsObj = masm.branchPtr(Assembler::Equal,
                                    Address(JSFrameReg, JSStackFrame::offsetArgsObj()),
                                    ImmIntPtr(0));
    masm.stubCall(stubs::PutArgsObject, NULL, 0);
    noArgsObj.linkTo(masm.label(), &masm);

    



    masm.loadPtr(Address(JSFrameReg, offsetof(JSStackFrame, down)), Registers::ReturnReg);
    masm.storePtr(Registers::ReturnReg, FrameAddress(offsetof(VMFrame, regs.fp)));

    Address rval(JSFrameReg, JSStackFrame::offsetReturnValue());
    masm.loadPayload(rval, JSReturnReg_Data);
    masm.loadTypeTag(rval, JSReturnReg_Type);

    masm.restoreReturnAddress();

    masm.move(Registers::ReturnReg, JSFrameReg);
#ifdef DEBUG
    masm.storePtr(ImmPtr(JSStackFrame::sInvalidPC),
                  Address(JSFrameReg, offsetof(JSStackFrame, savedPC)));
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

