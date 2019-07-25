








































#include "jscompartment.h"
#include "assembler/assembler/MacroAssembler.h"
#include "ion/IonCompartment.h"
#include "ion/IonLinker.h"
#include "ion/IonFrames.h"
#include "ion/Bailouts.h"
#include "ion/arm/Bailouts-arm.h"

using namespace js;
using namespace js::ion;

static void
GenerateReturn(MacroAssembler &masm, int returnCode)
{
    
    masm.ma_mov(Imm32(returnCode), r0);
    masm.startDataTransferM(IsLoad, sp, IA, WriteBack);
    masm.transferReg(r4);
    masm.transferReg(r5);
    masm.transferReg(r6);
    masm.transferReg(r7);
    masm.transferReg(r8);
    masm.transferReg(r9);
    masm.transferReg(r10);
    masm.transferReg(r11);
    
    masm.transferReg(pc);
    masm.finishDataTransfer();
    masm.dumpPool();
}








IonCode *
IonCompartment::generateEnterJIT(JSContext *cx)
{
    MacroAssembler masm(cx);
    Assembler *aasm = &masm;
    
    masm.startDataTransferM(IsStore, sp, DB, WriteBack);
    masm.transferReg(r3); 
    masm.transferReg(r4); 
    masm.transferReg(r5); 
    masm.transferReg(r6); 
    masm.transferReg(r7); 
    masm.transferReg(r8); 
    masm.transferReg(r9); 
    masm.transferReg(r10); 
    masm.transferReg(r11); 
    
    masm.transferReg(lr);  
    
    masm.finishDataTransfer();
    
    aasm->as_dtr(IsLoad, 32, Offset, r10, DTRAddr(sp, DtrOffImm(40)));
    aasm->as_mov(r9, lsl(r1, 3)); 
    
    
    
    
    
    
    
    aasm->as_add(r9, r9, Imm8(16-4));

#if 0
    
    
    
    
    
    
    
    aasm->as_sub(sp, sp, Imm8(4));
    aasm->as_orr(sp, sp, Imm8(4));
#endif
    
    aasm->as_sub(r4, sp, O2RegImmShift(r1, LSL, 3)); 
    
    aasm->as_sub(sp, r4, Imm8(16)); 
    
    
    aasm->as_mov(r5, O2Reg(r1), SetCond);

    
    
    {
        Label header, footer;
        
        aasm->as_b(&footer, Assembler::Zero);
        
        masm.bind(&header);
        aasm->as_sub(r5, r5, Imm8(1), SetCond);
        
        
        
        aasm->as_extdtr(IsLoad,  64, true, PostIndex, r6, EDtrAddr(r2, EDtrOffImm(8)));
        aasm->as_extdtr(IsStore, 64, true, PostIndex, r6, EDtrAddr(r4, EDtrOffImm(8)));
        aasm->as_b(&header, Assembler::NonZero);
        masm.bind(&footer);
    }
    masm.startDataTransferM(IsStore, sp, IB, NoWriteBack);
    masm.transferReg(r9);  
    masm.transferReg(r10); 
    masm.transferReg(r11); 
    masm.finishDataTransfer();
    
    aasm->as_dtr(IsStore, 32, Offset, pc, DTRAddr(sp, DtrOffImm(0)));
    
    aasm->as_bx(r0);
    
    
    aasm->as_dtr(IsLoad, 32, Offset, r5, DTRAddr(sp, DtrOffImm(0)));
    
    aasm->as_add(sp, sp, O2Reg(r5));
    
    
    aasm->as_dtr(IsLoad, 32, PostIndex, r5, DTRAddr(sp, DtrOffImm(4)));
    
    
    
    ASSERT(JSReturnReg_Type.code() == JSReturnReg_Data.code()+1);
    
    ASSERT((JSReturnReg_Data.code() & 1) == 0);
    aasm->as_extdtr(IsStore, 64, true, Offset,
                    JSReturnReg_Data, EDtrAddr(r5, EDtrOffImm(0)));
    GenerateReturn(masm, JS_TRUE);
    Linker linker(masm);
    return linker.newCode(cx);
}

IonCode *
IonCompartment::generateReturnError(JSContext *cx)
{
    MacroAssembler masm(cx);
    
    masm.ma_pop(r0);
    masm.ma_add(r0, sp, sp);

    GenerateReturn(masm, JS_FALSE);
    Linker linker(masm);
    return linker.newCode(cx);
}

IonCode *
IonCompartment::generateArgumentsRectifier(JSContext *cx)
{
    MacroAssembler masm(cx);
#if 0
    
    
    JS_ASSERT(ArgumentsRectifierReg == r8);

    
    masm.ma_ldr(DTRAddr(sp, DtrOffsetImm(offsetof(IonFrameData, calleeToken_))), r1);
    masm.ma_ldrh(EDTRAddr(r1, EDtrOffsetImm(offsetof(JSFunction, nargs))), r2);

    masm.ma_sub(r2, r8, r2);
    
    masm.moveValue(UndefinedValue(), r10);

    masm.movq(rsp, rbp); 

    
    {
        Label undefLoopTop;
        masm.bind(&undefLoopTop);

        masm.push(r10);
        masm.subl(Imm32(1), rcx);

        masm.testl(rcx, rcx);
        masm.j(Assembler::NonZero, &undefLoopTop);
    }

    
    masm.movq(r8, r9);
    masm.shlq(Imm32(3), r9); 

    masm.movq(rbp, rcx);
    masm.addq(Imm32(sizeof(IonFrameData)), rcx);
    masm.addq(r9, rcx);

    
    {
        Label copyLoopTop, initialSkip;

        masm.jump(&initialSkip);

        masm.bind(&copyLoopTop);
        masm.subq(Imm32(sizeof(Value)), rcx);
        masm.subl(Imm32(1), r8);
        masm.bind(&initialSkip);

        masm.mov(Operand(rcx, 0x0), rdx);
        masm.push(rdx);

        masm.testl(r8, r8);
        masm.j(Assembler::NonZero, &copyLoopTop);
    }

    
    masm.subq(rsp, rbp);
    masm.shll(Imm32(IonFramePrefix::FrameTypeBits), rbp);
    masm.orl(Imm32(IonFramePrefix::RectifierFrame), rbp);

    
    masm.push(rax); 
    masm.push(rbp); 

    
    
    masm.movq(Operand(rax, offsetof(JSFunction, u.i.script)), rax);
    masm.movq(Operand(rax, offsetof(JSScript, ion)), rax);
    masm.movq(Operand(rax, offsetof(IonScript, method_)), rax);
    masm.movq(Operand(rax, IonCode::OffsetOfCode()), rax);
    masm.call(rax);

    
    masm.pop(rbp);            
    masm.shrl(Imm32(IonFramePrefix::FrameTypeBits), rbp); 
    masm.pop(r11);            
    masm.addq(rbp, rsp);      

    masm.ret();
#endif
    Linker linker(masm);
    return linker.newCode(cx);
}

static void
GenerateBailoutThunk(MacroAssembler &masm, uint32 frameClass)
{
    
    
    
    masm.startDataTransferM(IsStore, sp, DB, WriteBack);
    
    
    for (uint32 i = 0; i < Registers::Total; i++)
        masm.transferReg(Register::FromCode(i));
    masm.finishDataTransfer();
    
    
    masm.startFloatTransferM(IsStore, sp, DB, WriteBack);
    for (uint32 i = 0; i < FloatRegisters::Total; i++)
        masm.transferFloatReg(FloatRegister::FromCode(i));
    masm.finishFloatTransfer();

    
    
    
    
    
    

    
    masm.ma_mov(Imm32(frameClass), r4);
    
    
    
    
    masm.startDataTransferM(IsStore, sp, DB, WriteBack);
    
    masm.transferReg(r4);
    
    
    masm.transferReg(lr);
    masm.finishDataTransfer();

    
    
    
    masm.setupAlignedABICall(1);

    
    
    

    
    
    

    
    masm.setABIArg(0, sp);

    
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, Bailout));
    
    uint32 bailoutFrameSize = sizeof(void *) + 
                              sizeof(double) * FloatRegisters::Total +
                              sizeof(void *) * Registers::Total;

    if (frameClass == NO_FRAME_SIZE_CLASS_ID) {
        
        masm.as_dtr(IsLoad, 32, Offset,
                    r4, DTRAddr(sp, DtrOffImm(offsetof(BailoutStack, frameSize_))));
        
        
        
        
        masm.ma_add(sp, Imm32(bailoutFrameSize+12), sp);
        masm.as_add(sp, sp, O2Reg(r4));
    } else {
        uint32 frameSize = FrameSizeClass::FromClass(frameClass).frameSize();
        masm.ma_add(Imm32(frameSize 
                          + sizeof(void*) 
                          + bailoutFrameSize) 
                    , sp);
    }

    Label exception;

    
    masm.as_cmp(r0, Imm8(0));
    masm.as_b(&exception, Assembler::NonZero);
    
    
    
    masm.as_mov(r0, O2Reg(sp));
    
    masm.as_sub(sp, sp, Imm8(sizeof(Value)));
    
    masm.as_mov(r1, O2Reg(sp));
    
    masm.setupAlignedABICall(2);
    
    masm.setABIArg(0, r0);
    masm.setABIArg(1, r1);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ThunkToInterpreter));

    
    
    masm.as_extdtr(IsLoad, 64, true, PostIndex,
                   JSReturnReg_Data, EDtrAddr(sp, EDtrOffImm(8)));

    
    masm.as_cmp(r0, Imm8(0));
    masm.as_b(&exception, Assembler::Zero);
    masm.as_dtr(IsLoad, 32, PostIndex, pc, DTRAddr(sp, DtrOffImm(4)));
    masm.bind(&exception);

    
    masm.as_mov(r0, O2Reg(sp));
    masm.setupAlignedABICall(1);
    masm.setABIArg(0,r0);
    void *func = JS_FUNC_TO_DATA_PTR(void *, ion::HandleException);
    masm.callWithABI(func);

    
    masm.as_add(sp, sp, O2Reg(r0));
    
    
    masm.as_dtr(IsLoad, 32, PostIndex, pc, DTRAddr(sp, DtrOffImm(4)));
}

IonCode *
IonCompartment::generateBailoutTable(JSContext *cx, uint32 frameClass)
{
    MacroAssembler masm;

    Label bailout;
    for (size_t i = 0; i < BAILOUT_TABLE_SIZE; i++)
        masm.ma_bl(&bailout);
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
