







































#include "StubCompiler.h"
#include "Compiler.h"

using namespace js;
using namespace mjit;

StubCompiler::StubCompiler(JSContext *cx, mjit::Compiler &cc, FrameState &frame, JSScript *script)
  : cx(cx), cc(cc), frame(frame), script(script), exits(SystemAllocPolicy())
{
}

void
StubCompiler::linkExit(Jump j)
{
    
    exits.append(ExitPatch(j, masm.label()));
}

void
StubCompiler::syncAndSpill()
{
    frame.sync(masm);
}

void *
StubCompiler::getCallTarget(void *fun)
{
#ifdef JS_CPU_ARM
    










    void *pfun = JS_FUNC_TO_DATA_PTR(void *, JaegerStubVeneer);

    




    masm.move(Imm32(intptr_t(fun)), ARMRegisters::ip);
#else
    




    void *pfun = fun;
#endif
    return pfun;
}

typedef JSC::MacroAssembler::RegisterID RegisterID;
typedef JSC::MacroAssembler::ImmPtr ImmPtr;
typedef JSC::MacroAssembler::Imm32 Imm32;


#if defined(JS_CPU_X86) || defined(JS_CPU_ARM)
static const RegisterID ClobberInCall = JSC::X86Registers::ecx;
#elif defined(JS_CPU_ARM)
static const RegisterID ClobberInCall = JSC::ARMRegisters::r2;
#endif

JS_STATIC_ASSERT(ClobberInCall != Registers::ArgReg1);

JSC::MacroAssembler::Call
StubCompiler::scall(void *ptr)
{
    void *pfun = getCallTarget(ptr);

    
    masm.storePtr(ImmPtr(cc.getPC()),
                  FrameAddress(offsetof(VMFrame, regs) + offsetof(JSFrameRegs, pc)));

    
    masm.addPtr(Imm32(sizeof(JSStackFrame) +
                (frame.stackDepth() + script->nfixed) * sizeof(jsval)),
                FrameState::FpReg, ClobberInCall);

    
    masm.storePtr(ClobberInCall,
                  FrameAddress(offsetof(VMFrame, regs) + offsetof(JSFrameRegs, sp)));
    
    
    masm.move(Assembler::stackPointerRegister, Registers::ArgReg0);

    return masm.call(pfun);
}

