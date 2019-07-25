








































#include "assembler/assembler/MacroAssembler.h"
#include "ion/IonCompartment.h"
#include "ion/IonLinker.h"

using namespace js::ion;






IonCode *
IonCompartment::generateEnterJIT(JSContext *cx)
{
    MacroAssembler masm(cx);

    
    masm.push(ebp);
    masm.movl(Operand(esp), ebp);

    
    masm.push(ebx);
    masm.push(esi);
    masm.push(edi);

    
    
    masm.movl(Operand(ebp, 12), eax);
    masm.shll(Imm32(3), eax);

    
    
    
    
    
    
    
    masm.movl(esp, ecx);
    masm.subl(eax, ecx);
    masm.subl(Imm32(8), ecx);

    
    masm.andl(Imm32(15), ecx);
    masm.subl(ecx, esp);

    



    
    masm.movl(Operand(ebp, 16), ebx);

    
    masm.addl(ebx, eax);

    
    {
        Label header, footer;
        masm.bind(&header);

        masm.cmpl(eax, ebx);
        masm.j(Assembler::BelowOrEqual, &footer);

        
        masm.subl(Imm32(8), eax);

        
        masm.push(Operand(eax, 4));
        masm.push(Operand(eax, 0));

        masm.jmp(&header);
        masm.bind(&footer);
    }

    
    
    masm.movl(Operand(ebp, 12), eax);
    masm.shll(Imm32(3), eax);
    masm.addl(eax, ecx);
    masm.push(ecx);

    



    
    masm.call(Operand(ebp, 8));

    
    
    masm.pop(eax);
    masm.addl(eax, esp);

    
    
    
    
    
    
    
    
    
    
    
    
    masm.movl(Operand(esp, 32), eax);
    masm.movl(JSReturnReg_Type, Operand(eax, 4));
    masm.movl(JSReturnReg_Data, Operand(eax, 0));

    



    
    masm.pop(edi);
    masm.pop(esi);
    masm.pop(ebx);

    
    masm.pop(ebp);
    masm.movl(Imm32(1), eax);
    masm.ret();

    Linker linker(masm);
    return linker.newCode(cx);
}

