








































#include "Assembler-x64.h"
#include "ion/IonCompartment.h"
#include "ion/IonLinker.h"

using namespace js::ion;
using namespace JSC;






IonCode *
IonCompartment::generateEnterJIT(JSContext *cx)
{
    Assembler masm;

    
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

    
    masm.push(ArgReg4); 

    
    
    
    
    
    
    masm.mov(ArgReg2, r12); 
    masm.andl(Imm32(1), Operand(r12)); 
    masm.xorl(Imm32(1), Operand(r12)); 
    masm.shll(Imm32(3), r12); 

    masm.subq(r12, rsp); 

    


    masm.mov(ArgReg2, r13); 
    masm.subq(Imm32(1), r13);
    masm.shll(Imm32(3), r13); 
    masm.addq(ArgReg3, r13); 

    Label loopHeader, loopEnd;

    
    masm.bind(&loopHeader);

    masm.cmpq(r13, ArgReg3);
    masm.j(AssemblerX86Shared::LessThan, &loopEnd);

    masm.push(Operand(r13, 0)); 

    masm.subq(Imm32(8), r13); 

    masm.jmp(&loopHeader); 
    masm.bind(&loopEnd);

    


    masm.shll(Imm32(3), ArgReg2);
    masm.addq(ArgReg2, r12); 
    masm.push(r12); 

    masm.call(Operand(ArgReg1)); 

    masm.pop(r12); 
    masm.addq(r12, rsp); 

    


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

    LinkerT<Assembler> linker(masm);
    return linker.newCode(cx);
}
