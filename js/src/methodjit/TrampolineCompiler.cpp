







































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
TrampolineCompiler::compileTrampoline(Trampolines::TrampolinePtr *where,
                                      JSC::ExecutablePool **poolp, TrampolineGenerator generator)
{
    Assembler masm;

    Label entry = masm.label();
    CHECK_RESULT(generator(masm));
    JS_ASSERT(entry.isValid());

    bool ok;
    JSC::LinkBuffer buffer(&masm, execAlloc, poolp, &ok);
    if (!ok) 
        return false;
    masm.finalize(buffer);
    uint8 *result = (uint8*)buffer.finalizeCodeAddendum().dataLocation();
    *where = JS_DATA_TO_FUNC_PTR(Trampolines::TrampolinePtr, result + masm.distanceOf(entry));

    return true;
}








bool
TrampolineCompiler::generateForceReturn(Assembler &masm)
{
    
    Jump noActObjs = masm.branchTest32(Assembler::Zero, FrameFlagsAddress(),
                                       Imm32(StackFrame::HAS_CALL_OBJ | StackFrame::HAS_ARGS_OBJ));
    masm.fallibleVMCall(JS_FUNC_TO_DATA_PTR(void *, stubs::PutActivationObjects), NULL, 0);
    noActObjs.linkTo(masm.label(), &masm);

    
    masm.loadValueAsComponents(UndefinedValue(), JSReturnReg_Type, JSReturnReg_Data);
    Jump rvalClear = masm.branchTest32(Assembler::Zero,
                                       FrameFlagsAddress(), Imm32(StackFrame::HAS_RVAL));
    Address rvalAddress(JSFrameReg, StackFrame::offsetOfReturnValue());
    masm.loadValueAsComponents(rvalAddress, JSReturnReg_Type, JSReturnReg_Data);
    rvalClear.linkTo(masm.label(), &masm);

    
    masm.loadPtr(Address(JSFrameReg, StackFrame::offsetOfNcode()), Registers::ReturnReg);
    masm.jump(Registers::ReturnReg);
    return true;
}

#if (defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)) || defined(_WIN64)
bool
TrampolineCompiler::generateForceReturnFast(Assembler &masm)
{
#ifdef _WIN64
    masm.addPtr(Imm32(32), Registers::StackPointer);
#else
    
    
    masm.addPtr(Imm32(16), Registers::StackPointer);
#endif
    return generateForceReturn(masm);
}
#endif

} 
} 

