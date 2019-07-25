








































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
    
    masm.pop(edi);
    masm.pop(esi);
    masm.pop(ebx);

    
    masm.pop(ebp);
    masm.movl(Imm32(returnCode), eax);
    masm.ret();
}






IonCode *
IonCompartment::generateEnterJIT(JSContext *cx)
{
    MacroAssembler masm(cx);

    
    masm.push(ebp);
    masm.movl(esp, ebp);

    
    masm.push(ebx);
    masm.push(esi);
    masm.push(edi);

    
    
    masm.movl(Operand(ebp, 12), eax);
    masm.shll(Imm32(3), eax);

    
    
    
    
    
    
    
    
    masm.movl(esp, ecx);
    masm.subl(eax, ecx);
    masm.subl(Imm32(12), ecx);

    
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

    
    masm.push(Operand(ebp, 24));

    
    
    masm.movl(Operand(ebp, 12), eax);
    masm.shll(Imm32(3), eax);
    masm.addl(eax, ecx);
    masm.addl(Imm32(4), ecx);

    
    masm.orl(Imm32(0x1), ecx); 
    masm.push(ecx);

    



    
    masm.call(Operand(ebp, 8));

    
    
    masm.pop(eax);
    masm.xorl(Imm32(0x1), eax); 
    masm.addl(eax, esp);

    
    
    
    
    
    
    
    
    
    
    
    
    masm.movl(Operand(esp, 32), eax);
    masm.movl(JSReturnReg_Type, Operand(eax, 4));
    masm.movl(JSReturnReg_Data, Operand(eax, 0));

    


    GenerateReturn(masm, JS_TRUE);

    Linker linker(masm);
    return linker.newCode(cx);
}

IonCode *
IonCompartment::generateReturnError(JSContext *cx)
{
    MacroAssembler masm(cx);

    masm.pop(eax);              
    masm.xorl(Imm32(0x1), eax); 
    masm.addl(eax, esp);        

    GenerateReturn(masm, JS_FALSE);
    
    Linker linker(masm);
    return linker.newCode(cx);
}

IonCode *
IonCompartment::generateArgumentsRectifier(JSContext *cx)
{
    MacroAssembler masm(cx);

    
    
    JS_ASSERT(ArgumentsRectifierReg == esi);

    
    masm.movl(Operand(esp, offsetof(IonFrameData, calleeToken_)), eax);
    masm.load16(Operand(eax, offsetof(JSFunction, nargs)), ecx);
    masm.subl(esi, ecx);

    masm.moveValue(UndefinedValue(), ebx, edi);

    masm.movl(esp, ebp); 

    
    {
        Label undefLoopTop;
        masm.bind(&undefLoopTop);

        masm.push(ebx); 
        masm.push(edi); 
        masm.subl(Imm32(1), ecx);

        masm.testl(ecx, ecx);
        masm.j(Assembler::NonZero, &undefLoopTop);
    }

    
    masm.movl(esi, edi);
    masm.shll(Imm32(3), edi); 

    masm.movl(ebp, ecx);
    masm.addl(Imm32(sizeof(IonFrameData)), ecx);
    masm.addl(edi, ecx);

    
    {
        Label copyLoopTop, initialSkip;

        masm.jump(&initialSkip);

        masm.bind(&copyLoopTop);
        masm.subl(Imm32(sizeof(Value)), ecx);
        masm.subl(Imm32(1), esi);
        masm.bind(&initialSkip);

        masm.mov(Operand(ecx, sizeof(Value)/2), edx);
        masm.push(edx);
        masm.mov(Operand(ecx, 0x0), edx);
        masm.push(edx);

        masm.testl(esi, esi);
        masm.j(Assembler::NonZero, &copyLoopTop);
    }

    
    masm.subl(esp, ebp);
    masm.shll(Imm32(IonFramePrefix::FrameTypeBits), ebp);
    masm.orl(Imm32(IonFramePrefix::RectifierFrame), ebp);

    
    masm.push(eax); 
    masm.push(ebp); 

    
    
    masm.movl(Operand(eax, offsetof(JSFunction, u.i.script)), eax);
    masm.movl(Operand(eax, offsetof(JSScript, ion)), eax);
    masm.movl(Operand(eax, offsetof(IonScript, method_)), eax);
    masm.movl(Operand(eax, IonCode::OffsetOfCode()), eax);
    masm.call(eax);

    
    masm.pop(ebp);            
    masm.shrl(Imm32(IonFramePrefix::FrameTypeBits), ebp); 
    masm.pop(edi);            
    masm.addl(ebp, esp);      

    masm.ret();

    Linker linker(masm);
    return linker.newCode(cx);
}

static void
GenerateBailoutThunk(MacroAssembler &masm, uint32 frameClass)
{
    
    masm.reserveStack(Registers::Total * sizeof(void *));
    for (uint32 i = 0; i < Registers::Total; i++)
        masm.movl(Register::FromCode(i), Operand(esp, i * sizeof(void *)));

    
    masm.reserveStack(FloatRegisters::Total * sizeof(double));
    for (uint32 i = 0; i < FloatRegisters::Total; i++)
        masm.movsd(FloatRegister::FromCode(i), Operand(esp, i * sizeof(double)));

    
    masm.push(Imm32(frameClass));

    
    masm.movl(esp, eax);

    
    masm.setupUnalignedABICall(1, ecx);
    masm.setABIArg(0, eax);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, Bailout));

    
    uint32 bailoutFrameSize = sizeof(void *) + 
                              sizeof(double) * FloatRegisters::Total +
                              sizeof(void *) * Registers::Total;
    
    if (frameClass == NO_FRAME_SIZE_CLASS_ID) {
        
        
        
        
        
        masm.addl(Imm32(bailoutFrameSize), esp);
        masm.pop(ecx);
        masm.lea(Operand(esp, ecx, TimesOne, sizeof(void *)), esp);
    } else {
        
        
        
        
        uint32 frameSize = FrameSizeClass::FromClass(frameClass).frameSize();
        masm.addl(Imm32(bailoutFrameSize + sizeof(void *) + frameSize), esp);
    }

    Label exception;

    
    masm.testl(eax, eax);
    masm.j(Assembler::NonZero, &exception);

    
    
    
    masm.movl(esp, eax);

    
    masm.subl(Imm32(sizeof(Value)), esp);
    masm.movl(esp, ecx);

    
    masm.setupUnalignedABICall(2, edx);
    masm.setABIArg(0, eax);
    masm.setABIArg(1, ecx);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ThunkToInterpreter));

    
    masm.movl(Operand(esp, 4), JSReturnReg_Type);
    masm.movl(Operand(esp, 0), JSReturnReg_Data);
    masm.addl(Imm32(8), esp);

    
    masm.testl(eax, eax);
    masm.j(Assembler::Zero, &exception);

    
    masm.ret();

    masm.bind(&exception);

    
    masm.movl(esp, eax);
    masm.setupUnalignedABICall(1, ecx);
    masm.setABIArg(0, eax);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, HandleException));

    
    masm.addl(eax, esp);
    masm.ret();
}

IonCode *
IonCompartment::generateBailoutTable(JSContext *cx, uint32 frameClass)
{
    MacroAssembler masm;

    Label bailout;
    for (size_t i = 0; i < BAILOUT_TABLE_SIZE; i++)
        masm.call(&bailout);
    masm.bind(&bailout);

    GenerateBailoutThunk(masm, frameClass);

    Linker linker(masm);
    return linker.newCode(cx);
}

IonCode *
IonCompartment::generateBailoutHandler(JSContext *cx)
{
    MacroAssembler masm;

    GenerateBailoutThunk(masm, NO_FRAME_SIZE_CLASS_ID);

    Linker linker(masm);
    return linker.newCode(cx);
}

