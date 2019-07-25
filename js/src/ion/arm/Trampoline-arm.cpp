








































#include "jscompartment.h"
#include "assembler/assembler/MacroAssembler.h"
#include "ion/IonCompartment.h"
#include "ion/IonLinker.h"
#include "ion/IonFrames.h"
#include "ion/Bailouts.h"
#include "ion/VMFunctions.h"

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
IonCompartment::generateOsrPrologue(JSContext *cx)
{
    
    
    
    
    
    
    
    
    
    

    JS_ASSERT(enterJIT_);
    return enterJIT_;
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
    
    aasm->as_dtr(IsLoad, 32, Offset, r11, DTRAddr(sp, DtrOffImm(40)));

    
    
    
    
    masm.as_dtr(IsLoad, 32, Offset, OsrFrameReg, DTRAddr(sp, DtrOffImm(44)));
    
#if 0
    JS_STATIC_ASSERT(OsrFrameReg == r10);
#endif
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
    masm.makeFrameDescriptor(r9, IonFrame_Entry);
#ifdef DEBUG
    masm.ma_mov(Imm32(0xdeadbeef), r8);
#endif
    masm.startDataTransferM(IsStore, sp, IB, NoWriteBack);
                           
    masm.transferReg(r8);  
    masm.transferReg(r9);  
    masm.transferReg(r11); 
    masm.finishDataTransfer();
    
    aasm->as_dtr(IsStore, 32, Offset, pc, DTRAddr(sp, DtrOffImm(0)));
    
    aasm->as_blx(r0);
    
    
    
    aasm->as_dtr(IsLoad, 32, Offset, r5, DTRAddr(sp, DtrOffImm(4)));
    
    aasm->as_add(sp, sp, lsr(r5,FRAMETYPE_BITS));
    
    
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
    
    
    JS_ASSERT(ArgumentsRectifierReg == r8);

    
    masm.ma_ldr(DTRAddr(sp, DtrOffImm(IonFrameData::offsetOfCalleeToken())), r1);
    masm.ma_ldrh(EDtrAddr(r1, EDtrOffImm(offsetof(JSFunction, nargs))), r6);

    masm.ma_sub(r6, r8, r2);

    masm.moveValue(UndefinedValue(), r4, r5);

    masm.ma_mov(sp, r3); 

    
    {
        Label undefLoopTop;
        masm.bind(&undefLoopTop);
        masm.ma_dataTransferN(IsStore, 64, true, sp, Imm32(-8), r4, PreIndex);
        masm.ma_sub(r2, Imm32(1), r2);

        masm.ma_b(&undefLoopTop, Assembler::NonZero);
    }

    

    masm.ma_alu(r3, lsl(r8, 3), r3, op_add); 
    masm.ma_add(r3, Imm32(sizeof(IonFrameData)), r3);

    
    {
        Label copyLoopTop;
        masm.bind(&copyLoopTop);
        masm.ma_dataTransferN(IsLoad, 64, true, r3, Imm32(-8), r4, PostIndex);
        masm.ma_dataTransferN(IsStore, 64, true, sp, Imm32(-8), r4, PreIndex);

        masm.ma_sub(r8, Imm32(1), r8, SetCond);
        masm.ma_b(&copyLoopTop, Assembler::NonZero);
    }

    
    masm.makeFrameDescriptor(r6, IonFrame_Rectifier);

    
    masm.ma_push(r0); 
    masm.ma_push(r1); 
    masm.ma_push(r6); 

    
    
    masm.ma_ldr(DTRAddr(r3, DtrOffImm(offsetof(JSFunction, u.i.script_))), r3);
    masm.ma_ldr(DTRAddr(r3, DtrOffImm(offsetof(JSScript, ion))), r3);
    masm.ma_ldr(DTRAddr(r3, DtrOffImm(offsetof(IonScript, method_))), r3);
    masm.ma_ldr(DTRAddr(r3, DtrOffImm(IonCode::OffsetOfCode())), r3);
    masm.ma_callIonHalfPush(r3);

    
    masm.ma_pop(r3);            
    
    masm.ma_pop(r11);            
    masm.ma_alu(sp, lsr(r3, FRAMETYPE_BITS), sp, op_add);      

    masm.ret();
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
                    r4, DTRAddr(sp, DtrOffImm(4)));
        
        
        
        
        
        
        masm.ma_add(sp, Imm32(bailoutFrameSize+12), sp);
        masm.as_add(sp, sp, O2Reg(r4));
    } else {
        uint32 frameSize = FrameSizeClass::FromClass(frameClass).frameSize();
        masm.ma_add(Imm32(frameSize 
                          + sizeof(void*) 
                          + bailoutFrameSize) 
                    , sp);
    }

    Label frameFixupDone;
    masm.ma_ldr(Operand(sp, IonJSFrameLayout::offsetOfDescriptor()), r1);
    
    masm.ma_and(Imm32(FRAMETYPE_BITS), r1, r2);
    masm.ma_cmp(r2, Imm32(IonFrame_JS));
    masm.ma_b(&frameFixupDone, Assembler::NotEqual);
    {
        JS_STATIC_ASSERT(sizeof(IonJSFrameLayout) >= sizeof(IonExitFrameLayout));
        ptrdiff_t difference = sizeof(IonJSFrameLayout) - sizeof(IonExitFrameLayout);
        masm.ma_add(r1, Imm32(difference << FRAMETYPE_BITS), r1);
        masm.ma_str(r1, Operand(sp, IonJSFrameLayout::offsetOfDescriptor()));
    }
    masm.bind(&frameFixupDone);
    masm.linkExitFrame();

    Label interpret;
    Label exception;

    
    
    
    
    

    masm.ma_cmp(r0, Imm32(BAILOUT_RETURN_FATAL_ERROR));
    masm.ma_b(&interpret, Assembler::LessThan);
    masm.ma_b(&exception, Assembler::Equal);

    
    masm.setupAlignedABICall(1);
    masm.setABIArg(0, r0);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ReflowTypeInfo));

    masm.ma_cmp(r0, Imm32(0));
    masm.ma_b(&exception, Assembler::Equal);

    masm.bind(&interpret);
    
    masm.as_sub(sp, sp, Imm8(sizeof(Value)));

    
    masm.setupAlignedABICall(1);
    masm.setABIArg(0, sp);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ThunkToInterpreter));

    
    
    masm.as_extdtr(IsLoad, 64, true, PostIndex,
                   JSReturnReg_Data, EDtrAddr(sp, EDtrOffImm(8)));

    
    masm.as_cmp(r0, Imm8(0));
    masm.ma_b(&exception, Assembler::Zero);
    masm.as_dtr(IsLoad, 32, PostIndex, pc, DTRAddr(sp, DtrOffImm(4)));
    masm.bind(&exception);
    masm.handleException();
#if 0
    
    masm.as_mov(r0, O2Reg(sp));
    masm.setupAlignedABICall(1);
    masm.setABIArg(0,r0);
    void *func = JS_FUNC_TO_DATA_PTR(void *, ion::HandleException);
    masm.callWithABI(func);

    
    masm.as_add(sp, sp, O2Reg(r0));
    
    
    masm.as_dtr(IsLoad, 32, PostIndex, pc, DTRAddr(sp, DtrOffImm(4)));
#endif
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

IonCode *
IonCompartment::generateVMWrapper(JSContext *cx, const VMFunction &f)
{
    typedef MoveResolver::MoveOperand MoveOperand;

    JS_ASSERT(functionWrappers_);
    JS_ASSERT(functionWrappers_->initialized());
    VMWrapperMap::AddPtr p = functionWrappers_->lookupForAdd(&f);
    if (p)
        return p->value;
    MacroAssembler masm;
    
    
    const GeneralRegisterSet allocatableRegs(Registers::AllocatableMask & ~Registers::ArgRegMask);
    GeneralRegisterSet regs(allocatableRegs);
    
    
    
    
    
    
    Register argsBase = InvalidReg;
    if (f.explicitArgs) {
        argsBase = regs.takeAny();
        masm.ma_add(sp, Imm32(sizeof(IonExitFrameLayout)), argsBase);
    }

    
    Register outReg = InvalidReg;
    if (f.outParam == Type_Value) {
        outReg = regs.takeAny();
        masm.reserveStack(sizeof(Value));
        masm.ma_mov(sp, outReg);
    }

    Register temp = regs.takeAny();
    
    masm.setupUnalignedABICall(f.argc(), temp);

    
    Register cxreg = r0;
    
    masm.movePtr(ImmWord(&JS_THREAD_DATA(cx)->ionJSContext), cxreg);
    masm.setABIArg(0, cxreg);

    
    if (f.explicitArgs) {
        for (uint32 i = 0; i < f.explicitArgs; i++)
            masm.setABIArg(i + 1, MoveOperand(argsBase, i * sizeof(void *)));
    }

    
    if (outReg != InvalidReg)
        masm.setABIArg(f.argc() - 1, outReg);

    masm.callWithABI(f.wrapped);

    
    JS_ASSERT(f.failType() == Type_Bool || f.failType() == Type_Object);
    Label exception;
    masm.ma_cmp(r0, Imm32(0));
    masm.ma_b(&exception,Assembler::Zero);

    
    if (f.outParam == Type_Value) {
        masm.ma_ldrd(EDtrAddr(sp, EDtrOffImm(0)), JSReturnReg_Data);
        masm.freeStack(sizeof(Value));
    }

    
    regs = GeneralRegisterSet::Not(GeneralRegisterSet(Registers::JSCallMask | Registers::CallMask));
    temp = regs.takeAny();

    
    
    
    
    
    
    
    masm.breakpoint();
    masm.ma_sub(pc, Imm32(128), temp);
    masm.ma_add(Imm32(sizeof(IonExitFrameLayout) + f.explicitArgs * sizeof(void *)), sp);
    masm.ma_push(temp);
    masm.ret();

    masm.bind(&exception);
    masm.handleException();

    Linker linker(masm);
    IonCode *wrapper = linker.newCode(cx);
    if (!wrapper || !functionWrappers_->add(p, &f, wrapper))
        return NULL;

    return wrapper;
}
