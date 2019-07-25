








































#include "jscompartment.h"
#include "assembler/assembler/MacroAssembler.h"
#include "ion/IonCompartment.h"
#include "ion/IonLinker.h"
#include "ion/IonFrames.h"
#include "ion/Bailouts.h"

using namespace js;
using namespace js::ion;

static void
GenerateReturn(MacroAssembler &masm, int returnCode)
{
    
#if defined(_WIN64)
    masm.pop(rsi);
    masm.pop(rdi);
#endif
    masm.pop(r15);
    masm.pop(r14);
    masm.pop(r13);
    masm.pop(r12);
    masm.pop(rbx);

    
    masm.pop(rbp);
    masm.movl(Imm32(returnCode), rax);
    masm.ret();
}






IonCode *
IonCompartment::generateEnterJIT(JSContext *cx)
{
    const Register reg_code = ArgReg0;
    const Register reg_argc = ArgReg1;
    const Register reg_argv = ArgReg2;
    const Register reg_vp   = ArgReg3;
#if defined(_WIN64)
    const Operand token = Operand(rbp, 8 + ShadowStackSpace);
#else
    const Register token = ArgReg4;
#endif

    MacroAssembler masm(cx);

    
    masm.push(rbp);
    masm.mov(rsp, rbp);

    
    masm.push(rbx);
    masm.push(r12);
    masm.push(r13);
    masm.push(r14);
    masm.push(r15);
#if defined(_WIN64)
    masm.push(rdi);
    masm.push(rsi);
#endif

    
    masm.push(reg_vp);

    
    masm.mov(rsp, r14);

    
    masm.mov(reg_argc, r13);
    masm.shll(Imm32(3), r13);

    
    
    
    
    masm.mov(rsp, r12);
    masm.subq(r13, r12);
    masm.subq(Imm32(8), r12);
    masm.andl(Imm32(0xf), r12);
    masm.subq(r12, rsp);

    



    
    masm.addq(reg_argv, r13); 

    
    {
        Label header, footer;
        masm.bind(&header);

        masm.cmpq(r13, reg_argv);
        masm.j(AssemblerX86Shared::BelowOrEqual, &footer);

        masm.subq(Imm32(8), r13);
        masm.push(Operand(r13, 0));
        masm.jmp(&header);

        masm.bind(&footer);
    }

    
    
    masm.push(token);

    


    masm.subq(rsp, r14);
    masm.push(r14);

    
    masm.call(reg_code);

    
    masm.pop(r14);
    masm.addq(r14, rsp);

    


    masm.pop(r12); 
    masm.movq(JSReturnReg, Operand(r12, 0));

    GenerateReturn(masm, JS_TRUE);

    Linker linker(masm);
    return linker.newCode(cx);
}

IonCode *
IonCompartment::generateReturnError(JSContext *cx)
{
    MacroAssembler masm(cx);

    
    
    masm.pop(r14);
    masm.addq(r14, rsp);

    
    masm.pop(r11);

    GenerateReturn(masm, JS_FALSE);
    
    Linker linker(masm);
    return linker.newCode(cx);
}

static void
GenerateBailoutThunk(MacroAssembler &masm, uint32 frameClass)
{
    
    masm.reserveStack(Registers::Total * sizeof(void *));
    for (uint32 i = 0; i < Registers::Total; i++)
        masm.movq(Register::FromCode(i), Operand(rsp, i * sizeof(void *)));

    
    masm.reserveStack(FloatRegisters::Total * sizeof(double));
    for (uint32 i = 0; i < FloatRegisters::Total; i++)
        masm.movsd(FloatRegister::FromCode(i), Operand(rsp, i * sizeof(double)));

    
    masm.movq(rsp, r8);

    
    masm.setupUnalignedABICall(1, rax);
    masm.setABIArg(0, r8);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, Bailout));

    
    
    
    
    
    uint32 bailoutFrameSize = sizeof(void *) * Registers::Total +
                              sizeof(double) * FloatRegisters::Total;
    masm.addq(Imm32(bailoutFrameSize), rsp);
    masm.pop(rcx);
    masm.lea(Operand(rsp, rcx, TimesOne, sizeof(void *)), rsp);

    Label exception;

    
    masm.testl(rax, rax);
    masm.j(Assembler::NonZero, &exception);

    
    
    
    masm.movq(rsp, rax);

    
    masm.subq(Imm32(sizeof(Value)), rsp);
    masm.movq(rsp, rcx);

    
    masm.setupUnalignedABICall(2, rdx);
    masm.setABIArg(0, rax);
    masm.setABIArg(1, rcx);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ThunkToInterpreter));

    
    masm.movq(Operand(rsp, 0), JSReturnReg);
    masm.addq(Imm32(8), rsp);

    
    masm.testl(rax, rax);
    masm.j(Assembler::Zero, &exception);

    
    masm.ret();

    masm.bind(&exception);


    
    masm.movq(rsp, rax);
    masm.setupUnalignedABICall(1, rcx);
    masm.setABIArg(0, rax);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, HandleException));

    
    masm.addq(rax, rsp);
    masm.ret();
}

IonCode *
IonCompartment::generateBailoutTable(JSContext *cx, uint32 frameClass)
{
    JS_NOT_REACHED("x64 does not use bailout tables");
    return NULL;
}

IonCode *
IonCompartment::generateBailoutHandler(JSContext *cx)
{
    MacroAssembler masm;

    GenerateBailoutThunk(masm, NO_FRAME_SIZE_CLASS_ID);

    Linker linker(masm);
    return linker.newCode(cx);
}

