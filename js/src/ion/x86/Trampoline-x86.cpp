








































#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/LinkBuffer.h"
#include "ion/IonCompartment.h"
#include "ion/IonLinker.h"

using namespace js::ion;
using namespace JSC;

IonCode *
IonCompartment::generateEnterJIT(JSContext *cx)
{
    typedef MacroAssembler::Label Label;
    typedef MacroAssembler::Jump Jump;
    typedef MacroAssembler::Address Address;
    typedef MacroAssembler::Imm32 Imm32;

    MacroAssembler masm;

    
    masm.push(X86Registers::ebp);
    masm.move(X86Registers::esp, X86Registers::ebp);

    
    masm.push(X86Registers::ebx);
    masm.push(X86Registers::esi);
    masm.push(X86Registers::edi);

    



    
    
    masm.load32(Address(X86Registers::ebp, 12), X86Registers::eax);
    masm.sub32(Imm32(1), X86Registers::eax);
    masm.lshift32(Imm32(3), X86Registers::eax);

    
    masm.loadPtr(Address(X86Registers::ebp, 16), X86Registers::ebx);

    
    masm.add32(X86Registers::ebx, X86Registers::eax);

    
    Label loopHeader = masm.label();
    Jump loopCondition = masm.branch32(MacroAssembler::LessThan, X86Registers::eax, X86Registers::ebx);

    
    masm.push(Address(X86Registers::eax, 4)); 
    masm.push(Address(X86Registers::eax, 0)); 

    
    masm.sub32(Imm32(8), X86Registers::eax);

    
    masm.jump(loopHeader);
    loopCondition.linkTo(masm.label(), &masm);

    



    
    masm.call(Address(X86Registers::ebp, 8));

    
    masm.loadPtr(Address(X86Registers::ebp, 20), X86Registers::eax);
    masm.store32(X86Registers::ecx, Address(X86Registers::eax, 0)); 
    masm.store32(X86Registers::edx, Address(X86Registers::eax, 4)); 

    


    
    masm.load32(Address(X86Registers::ebp, 12), X86Registers::eax);
    masm.lshift32(Imm32(3), X86Registers::eax);

    
    masm.add32(X86Registers::eax, X86Registers::esp);

    
    masm.pop(X86Registers::edi);
    masm.pop(X86Registers::esi);
    masm.pop(X86Registers::ebx);

    
    masm.pop(X86Registers::ebp);

    LinkerT<MacroAssembler> linker(masm);
    return linker.newCode(cx);
}

