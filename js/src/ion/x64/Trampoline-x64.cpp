








































#include "Assembler-x64.h"
#include "ion/IonCompartment.h"
#include "ion/IonLinker.h"

using namespace js::ion;
using namespace JSC;






IonCode *
IonCompartment::generateEnterJIT(JSContext *cx)
{
    const Register reg_code = ArgReg1;
    const Register reg_argc = ArgReg2;
    const Register reg_argv = ArgReg3;
    const Register reg_vp   = ArgReg4;

    MacroAssembler masm;

    
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

    


    masm.subq(rsp, r14);
    masm.push(r14);

    
    masm.call(reg_code);

    
    masm.pop(r14);
    masm.addq(r14, rsp);

    


    masm.pop(r12); 
    masm.movq(JSReturnReg, Operand(r12, 0));

    
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
    masm.ret();

    Linker linker(masm);
    return linker.newCode(cx);
}

