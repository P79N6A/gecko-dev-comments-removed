







































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

bool
TrampolineCompiler::compile()
{
#ifdef JS_METHODJIT_SPEW
    JMCheckLogging();
#endif

    COMPILE(trampolines->forceReturn, trampolines->forceReturnPool, generateForceReturn);

    return true;
}

void
TrampolineCompiler::release(Trampolines *tramps)
{
    RELEASE(tramps->forceReturn, tramps->forceReturnPool);
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
#if defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)
    
    
    masm.addPtr(Imm32(8), Registers::StackPointer);
#endif
    
    Jump noCallObj = masm.branchPtr(Assembler::Equal,
                                    Address(JSFrameReg, offsetof(JSStackFrame, callobj)),
                                    ImmPtr(0));
    masm.stubCall(stubs::PutCallObject, NULL, 0);
    noCallObj.linkTo(masm.label(), &masm);

    
    Jump noArgsObj = masm.branchPtr(Assembler::Equal,
                                    Address(JSFrameReg, offsetof(JSStackFrame, argsobj)),
                                    ImmIntPtr(0));
    masm.stubCall(stubs::PutArgsObject, NULL, 0);
    noArgsObj.linkTo(masm.label(), &masm);

    





    masm.loadPtr(Address(JSFrameReg, offsetof(JSStackFrame, down)), Registers::ReturnReg);
    masm.loadPtr(FrameAddress(offsetof(VMFrame, cx)), Registers::ArgReg1);
    masm.storePtr(Registers::ReturnReg, FrameAddress(offsetof(VMFrame, fp)));
    masm.storePtr(Registers::ReturnReg, Address(Registers::ArgReg1, offsetof(JSContext, fp)));
    masm.sub32(Imm32(1), FrameAddress(offsetof(VMFrame, inlineCallCount)));

    Address rval(JSFrameReg, offsetof(JSStackFrame, rval));
    masm.loadPayload(rval, JSReturnReg_Data);
    masm.loadTypeTag(rval, JSReturnReg_Type);
    masm.move(Registers::ReturnReg, JSFrameReg);
    masm.loadPtr(Address(JSFrameReg, offsetof(JSStackFrame, ncode)), Registers::ReturnReg);
#ifdef DEBUG
    masm.storePtr(ImmPtr(JSStackFrame::sInvalidPC),
                  Address(JSFrameReg, offsetof(JSStackFrame, savedPC)));
#endif

    masm.ret();
    return true;
}

} 
} 

