





#include "jscompartment.h"
#include "assembler/assembler/MacroAssembler.h"
#include "ion/IonCompartment.h"
#include "ion/IonLinker.h"
#include "ion/IonFrames.h"
#include "ion/IonSpewer.h"
#include "ion/Bailouts.h"
#include "ion/VMFunctions.h"
#include "ion/arm/BaselineHelpers-arm.h"
#include "ion/ExecutionModeInlines.h"

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

struct EnterJITStack
{
    void *r0; 

    
    void *r4;
    void *r5;
    void *r6;
    void *r7;
    void *r8;
    void *r9;
    void *r10;
    void *r11;
    
    void *lr;

    
    
    
    
    
    CalleeToken token;
    JSObject *scopeChain;
    size_t numStackValues;
    Value *vp;
};








IonCode *
IonRuntime::generateEnterJIT(JSContext *cx, EnterJitType type)
{

    const Register reg_code  = r0;
    const Register reg_argc  = r1;
    const Register reg_argv  = r2;
    const Register reg_frame = r3;

    const Address slot_token(sp, offsetof(EnterJITStack, token));
    const Address slot_vp(sp, offsetof(EnterJITStack, vp));

    JS_ASSERT(OsrFrameReg == reg_frame);

    MacroAssembler masm(cx);
    AutoFlushCache afc("GenerateEnterJIT", cx->runtime->ionRuntime());
    Assembler *aasm = &masm;

    
    
    
    masm.startDataTransferM(IsStore, sp, DB, WriteBack);
    masm.transferReg(r0); 
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

    
    masm.movePtr(sp, r8);

    
    masm.loadPtr(slot_token, r9);

    
    if (type == EnterJitBaseline)
        masm.movePtr(sp, r11);

    
    masm.loadPtr(slot_vp, r10);
    masm.unboxInt32(Address(r10, 0), r10);

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

    masm.ma_sub(r8, sp, r8);
    masm.makeFrameDescriptor(r8, IonFrame_Entry);

    masm.startDataTransferM(IsStore, sp, IB, NoWriteBack);
                           
    masm.transferReg(r8);  
    masm.transferReg(r9);  
    masm.transferReg(r10); 
    masm.finishDataTransfer();

    Label returnLabel;
    if (type == EnterJitBaseline) {
        
        GeneralRegisterSet regs(GeneralRegisterSet::All());
        regs.take(JSReturnOperand);
        regs.takeUnchecked(OsrFrameReg);
        regs.take(r11);
        regs.take(ReturnReg);

        const Address slot_numStackValues(r11, offsetof(EnterJITStack, numStackValues));

        Label notOsr;
        masm.branchTestPtr(Assembler::Zero, OsrFrameReg, OsrFrameReg, &notOsr);

        Register scratch = regs.takeAny();

        Register numStackValues = regs.takeAny();
        masm.load32(slot_numStackValues, numStackValues);

        
        
        
        
        
        {
            AutoForbidPools afp(&masm);
            Label skipJump;
            masm.mov(pc, scratch);
            masm.addPtr(Imm32(2 * sizeof(uint32_t)), scratch);
            masm.storePtr(scratch, Address(sp, 0));
            masm.jump(&skipJump);
            masm.jump(&returnLabel);
            masm.bind(&skipJump);
        }

        
        masm.push(r11);

        
        Register framePtr = r11;
        masm.subPtr(Imm32(BaselineFrame::Size()), sp);
        masm.mov(sp, framePtr);

        
        masm.ma_lsl(Imm32(3), numStackValues, scratch);
        masm.ma_sub(sp, scratch, sp);

        
        masm.addPtr(Imm32(BaselineFrame::Size() + BaselineFrame::FramePointerOffset), scratch);
        masm.makeFrameDescriptor(scratch, IonFrame_BaselineJS);
        masm.push(scratch);
        masm.push(Imm32(0)); 
        masm.enterFakeExitFrame();

        masm.push(framePtr); 
        masm.push(r0); 

        masm.setupUnalignedABICall(3, scratch);
        masm.passABIArg(r11); 
        masm.passABIArg(OsrFrameReg); 
        masm.passABIArg(numStackValues);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ion::InitBaselineFrameForOsr));

        Register jitcode = regs.takeAny();
        masm.pop(jitcode);
        masm.pop(framePtr);

        JS_ASSERT(jitcode != ReturnReg);

        Label error;
        masm.addPtr(Imm32(IonExitFrameLayout::SizeWithFooter()), sp);
        masm.addPtr(Imm32(BaselineFrame::Size()), framePtr);
        masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, &error);

        masm.jump(jitcode);

        
        
        masm.bind(&error);
        masm.mov(framePtr, sp);
        masm.addPtr(Imm32(2 * sizeof(uintptr_t)), sp);
        masm.moveValue(MagicValue(JS_ION_ERROR), JSReturnOperand);
        masm.jump(&returnLabel);

        masm.bind(&notOsr);
        
        JS_ASSERT(R1.scratchReg() != r0);
        masm.loadPtr(Address(r11, offsetof(EnterJITStack, scopeChain)), R1.scratchReg());
    }

    
    masm.ma_callIonNoPush(r0);

    if (type == EnterJitBaseline) {
        
        masm.bind(&returnLabel);
    }

    
    
    
    aasm->as_sub(sp, sp, Imm8(4));

    
    masm.loadPtr(Address(sp, IonJSFrameLayout::offsetOfDescriptor()), r5);
    aasm->as_add(sp, sp, lsr(r5, FRAMESIZE_SHIFT));

    
    masm.loadPtr(slot_vp, r5);
    masm.storeValue(JSReturnOperand, Address(r5, 0));

    
    
    
    
    
    
    

    
    aasm->as_add(sp, sp, Imm8(4));

    
    GenerateReturn(masm, JS_TRUE);

    Linker linker(masm);
    return linker.newCode(cx, JSC::OTHER_CODE);
}

IonCode *
IonRuntime::generateInvalidator(JSContext *cx)
{
    
    MacroAssembler masm(cx);
    
    
    
    
    
    
    
    masm.ma_and(Imm32(~7), sp, sp);
    masm.startDataTransferM(IsStore, sp, DB, WriteBack);
    
    
    for (uint32_t i = 0; i < Registers::Total; i++)
        masm.transferReg(Register::FromCode(i));
    masm.finishDataTransfer();

    masm.startFloatTransferM(IsStore, sp, DB, WriteBack);
    for (uint32_t i = 0; i < FloatRegisters::Total; i++)
        masm.transferFloatReg(FloatRegister::FromCode(i));
    masm.finishFloatTransfer();

    masm.ma_mov(sp, r0);
    const int sizeOfRetval = sizeof(size_t)*2;
    masm.reserveStack(sizeOfRetval);
    masm.mov(sp, r1);
    const int sizeOfBailoutInfo = sizeof(void *)*2;
    masm.reserveStack(sizeOfBailoutInfo);
    masm.mov(sp, r2);
    masm.setupAlignedABICall(3);
    masm.passABIArg(r0);
    masm.passABIArg(r1);
    masm.passABIArg(r2);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, InvalidationBailout));

    masm.ma_ldr(Address(sp, 0), r2);
    masm.ma_ldr(Address(sp, sizeOfBailoutInfo), r1);
    
    
    masm.ma_add(sp, Imm32(sizeof(InvalidationBailoutStack) + sizeOfRetval + sizeOfBailoutInfo), sp);
    
    
    masm.ma_add(sp, r1, sp);
    masm.generateBailoutTail(r1, r2);
    Linker linker(masm);
    IonCode *code = linker.newCode(cx, JSC::OTHER_CODE);
    IonSpew(IonSpew_Invalidate, "   invalidation thunk created at %p", (void *) code->raw());
    return code;
}

IonCode *
IonRuntime::generateArgumentsRectifier(JSContext *cx, ExecutionMode mode, void **returnAddrOut)
{
    MacroAssembler masm(cx);
    
    
    JS_ASSERT(ArgumentsRectifierReg == r8);

    
    masm.ma_ldr(DTRAddr(sp, DtrOffImm(IonRectifierFrameLayout::offsetOfNumActualArgs())), r0);

    
    masm.ma_ldr(DTRAddr(sp, DtrOffImm(IonRectifierFrameLayout::offsetOfCalleeToken())), r1);
    masm.clearCalleeTag(r1, mode);
    masm.ma_ldrh(EDtrAddr(r1, EDtrOffImm(offsetof(JSFunction, nargs))), r6);

    masm.ma_sub(r6, r8, r2);

    masm.moveValue(UndefinedValue(), r5, r4);

    masm.ma_mov(sp, r3); 
    masm.ma_mov(sp, r7); 

    
    {
        Label undefLoopTop;
        masm.bind(&undefLoopTop);
        masm.ma_dataTransferN(IsStore, 64, true, sp, Imm32(-8), r4, PreIndex);
        masm.ma_sub(r2, Imm32(1), r2, SetCond);

        masm.ma_b(&undefLoopTop, Assembler::NonZero);
    }

    

    masm.ma_alu(r3, lsl(r8, 3), r3, op_add); 
    masm.ma_add(r3, Imm32(sizeof(IonRectifierFrameLayout)), r3);

    
    {
        Label copyLoopTop;
        masm.bind(&copyLoopTop);
        masm.ma_dataTransferN(IsLoad, 64, true, r3, Imm32(-8), r4, PostIndex);
        masm.ma_dataTransferN(IsStore, 64, true, sp, Imm32(-8), r4, PreIndex);

        masm.ma_sub(r8, Imm32(1), r8, SetCond);
        masm.ma_b(&copyLoopTop, Assembler::Unsigned);
    }

    
    masm.ma_add(r6, Imm32(1), r6);
    masm.ma_lsl(Imm32(3), r6, r6);

    
    masm.makeFrameDescriptor(r6, IonFrame_Rectifier);

    
    masm.ma_push(r0); 
    masm.ma_push(r1); 
    masm.ma_push(r6); 

    
    
    masm.ma_ldr(DTRAddr(r1, DtrOffImm(JSFunction::offsetOfNativeOrScript())), r3);
    masm.loadBaselineOrIonRaw(r3, r3, mode, NULL);
    masm.ma_callIonHalfPush(r3);

    uint32_t returnOffset = masm.currentOffset();

    
    
    
    
    
    
    

    
    masm.ma_dtr(IsLoad, sp, Imm32(12), r4, PostIndex);

    
    
    
    
    
    
    

    
    masm.ma_alu(sp, lsr(r4, FRAMESIZE_SHIFT), sp, op_add);

    masm.ret();
    Linker linker(masm);
    IonCode *code = linker.newCode(cx, JSC::OTHER_CODE);

    CodeOffsetLabel returnLabel(returnOffset);
    returnLabel.fixup(&masm);
    if (returnAddrOut)
        *returnAddrOut = (void *) (code->raw() + returnLabel.offset());
    return code;
}

static void
GenerateBailoutThunk(MacroAssembler &masm, uint32_t frameClass)
{
    
    
    
    
    
    

    
    
    
    masm.startDataTransferM(IsStore, sp, DB, WriteBack);
    
    
    for (uint32_t i = 0; i < Registers::Total; i++)
        masm.transferReg(Register::FromCode(i));
    masm.finishDataTransfer();

    masm.startFloatTransferM(IsStore, sp, DB, WriteBack);
    for (uint32_t i = 0; i < FloatRegisters::Total; i++)
        masm.transferFloatReg(FloatRegister::FromCode(i));
    masm.finishFloatTransfer();

    
    
    
    
    
    

    
    masm.ma_mov(Imm32(frameClass), r4);
    
    
    
    
    masm.startDataTransferM(IsStore, sp, DB, WriteBack);
    
    masm.transferReg(r4);
    
    
    masm.transferReg(lr);
    masm.finishDataTransfer();

    
    
    
    masm.ma_mov(sp, r0);
    const int sizeOfBailoutInfo = sizeof(void *)*2;
    masm.reserveStack(sizeOfBailoutInfo);
    masm.mov(sp, r1);
    masm.setupAlignedABICall(2);

    
    
    

    
    
    

    
    masm.passABIArg(r0);
    masm.passABIArg(r1);

    
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, Bailout));
    masm.ma_ldr(Address(sp, 0), r2);
    masm.ma_add(sp, Imm32(sizeOfBailoutInfo), sp);
    
    uint32_t bailoutFrameSize = sizeof(void *) + 
                              sizeof(double) * FloatRegisters::Total +
                              sizeof(void *) * Registers::Total;

    if (frameClass == NO_FRAME_SIZE_CLASS_ID) {
        
        masm.as_dtr(IsLoad, 32, Offset,
                    r4, DTRAddr(sp, DtrOffImm(4)));
        
        
        
        
        
        
        masm.ma_add(sp, Imm32(bailoutFrameSize+12), sp);
        masm.as_add(sp, sp, O2Reg(r4));
    } else {
        uint32_t frameSize = FrameSizeClass::FromClass(frameClass).frameSize();
        masm.ma_add(Imm32(frameSize 
                          + sizeof(void*) 
                          + bailoutFrameSize) 
                    , sp);
    }
    masm.generateBailoutTail(r1, r2);
}

IonCode *
IonRuntime::generateBailoutTable(JSContext *cx, uint32_t frameClass)
{
    MacroAssembler masm(cx);

    Label bailout;
    for (size_t i = 0; i < BAILOUT_TABLE_SIZE; i++)
        masm.ma_bl(&bailout);
    masm.bind(&bailout);

    GenerateBailoutThunk(masm, frameClass);

    Linker linker(masm);
    return linker.newCode(cx, JSC::OTHER_CODE);
}

IonCode *
IonRuntime::generateBailoutHandler(JSContext *cx)
{
    MacroAssembler masm(cx);
    GenerateBailoutThunk(masm, NO_FRAME_SIZE_CLASS_ID);

    Linker linker(masm);
    return linker.newCode(cx, JSC::OTHER_CODE);
}

IonCode *
IonRuntime::generateVMWrapper(JSContext *cx, const VMFunction &f)
{
    typedef MoveResolver::MoveOperand MoveOperand;

    JS_ASSERT(functionWrappers_);
    JS_ASSERT(functionWrappers_->initialized());
    VMWrapperMap::AddPtr p = functionWrappers_->lookupForAdd(&f);
    if (p)
        return p->value;

    
    MacroAssembler masm(cx);
    GeneralRegisterSet regs = GeneralRegisterSet(Register::Codes::WrapperMask);

    
    JS_STATIC_ASSERT((Register::Codes::VolatileMask & ~Register::Codes::WrapperMask) == 0);

    
    Register cxreg = r0;

    
    
    
    
    
    
    masm.enterExitFrameAndLoadContext(&f, cxreg, regs.getAny(), f.executionMode);

    
    Register argsBase = InvalidReg;
    if (f.explicitArgs) {
        argsBase = r5;
        regs.take(argsBase);
        masm.ma_add(sp, Imm32(IonExitFrameLayout::SizeWithFooter()), argsBase);
    }

    
    Register outReg = InvalidReg;
    switch (f.outParam) {
      case Type_Value:
        outReg = r4;
        regs.take(outReg);
        masm.reserveStack(sizeof(Value));
        masm.ma_mov(sp, outReg);
        break;

      case Type_Handle:
        outReg = r4;
        regs.take(outReg);
        masm.PushEmptyRooted(f.outParamRootType);
        masm.ma_mov(sp, outReg);
        break;

      case Type_Int32:
        outReg = r4;
        regs.take(outReg);
        masm.reserveStack(sizeof(int32_t));
        masm.ma_mov(sp, outReg);
        break;

      default:
        JS_ASSERT(f.outParam == Type_Void);
        break;
    }

    masm.setupUnalignedABICall(f.argc(), regs.getAny());
    masm.passABIArg(cxreg);

    size_t argDisp = 0;

    
    for (uint32_t explicitArg = 0; explicitArg < f.explicitArgs; explicitArg++) {
        MoveOperand from;
        switch (f.argProperties(explicitArg)) {
          case VMFunction::WordByValue:
            masm.passABIArg(MoveOperand(argsBase, argDisp));
            argDisp += sizeof(void *);
            break;
          case VMFunction::DoubleByValue:
            
            
            JS_ASSERT(f.argPassedInFloatReg(explicitArg));
            masm.passABIArg(MoveOperand(argsBase, argDisp, MoveOperand::FLOAT));
            argDisp += sizeof(double);
            break;
          case VMFunction::WordByRef:
            masm.passABIArg(MoveOperand(argsBase, argDisp, MoveOperand::EFFECTIVE));
            argDisp += sizeof(void *);
            break;
          case VMFunction::DoubleByRef:
            masm.passABIArg(MoveOperand(argsBase, argDisp, MoveOperand::EFFECTIVE));
            argDisp += 2 * sizeof(void *);
            break;
        }
    }

    
    if (outReg != InvalidReg)
        masm.passABIArg(outReg);

    masm.callWithABI(f.wrapped);

    
    Label failure;
    switch (f.failType()) {
      case Type_Object:
      case Type_Bool:
        
        masm.branch32(Assembler::Equal, r0, Imm32(0), &failure);
        break;
      case Type_ParallelResult:
        masm.branch32(Assembler::NotEqual, r0, Imm32(TP_SUCCESS), &failure);
        break;
      default:
        JS_NOT_REACHED("unknown failure kind");
        break;
    }

    
    switch (f.outParam) {
      case Type_Handle:
        masm.popRooted(f.outParamRootType, ReturnReg, JSReturnOperand);
        break;

      case Type_Value:
        masm.loadValue(Address(sp, 0), JSReturnOperand);
        masm.freeStack(sizeof(Value));
        break;

      case Type_Int32:
        masm.load32(Address(sp, 0), ReturnReg);
        masm.freeStack(sizeof(int32_t));
        break;

      default:
        JS_ASSERT(f.outParam == Type_Void);
        break;
    }
    masm.leaveExitFrame();
    masm.retn(Imm32(sizeof(IonExitFrameLayout) +
                    f.explicitStackSlots() * sizeof(void *) +
                    f.extraValuesToPop * sizeof(Value)));

    masm.bind(&failure);
    masm.handleFailure(f.executionMode);

    Linker linker(masm);
    IonCode *wrapper = linker.newCode(cx, JSC::OTHER_CODE);
    if (!wrapper)
        return NULL;

    
    
    if (!functionWrappers_->relookupOrAdd(p, &f, wrapper))
        return NULL;

    return wrapper;
}

IonCode *
IonRuntime::generatePreBarrier(JSContext *cx, MIRType type)
{
    MacroAssembler masm(cx);

    RegisterSet save;
    if (cx->runtime->jitSupportsFloatingPoint) {
        save = RegisterSet(GeneralRegisterSet(Registers::VolatileMask),
                           FloatRegisterSet(FloatRegisters::VolatileMask));
    } else {
        save = RegisterSet(GeneralRegisterSet(Registers::VolatileMask),
                           FloatRegisterSet());
    }
    masm.PushRegsInMask(save);

    JS_ASSERT(PreBarrierReg == r1);
    masm.movePtr(ImmWord(cx->runtime), r0);

    masm.setupUnalignedABICall(2, r2);
    masm.passABIArg(r0);
    masm.passABIArg(r1);
    if (type == MIRType_Value) {
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, MarkValueFromIon));
    } else {
        JS_ASSERT(type == MIRType_Shape);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, MarkShapeFromIon));
    }

    masm.PopRegsInMask(save);
    masm.ret();

    Linker linker(masm);
    return linker.newCode(cx, JSC::OTHER_CODE);
}

typedef bool (*HandleDebugTrapFn)(JSContext *, BaselineFrame *, uint8_t *, JSBool *);
static const VMFunction HandleDebugTrapInfo = FunctionInfo<HandleDebugTrapFn>(HandleDebugTrap);

IonCode *
IonRuntime::generateDebugTrapHandler(JSContext *cx)
{
    MacroAssembler masm;

    Register scratch1 = r0;
    Register scratch2 = r1;

    
    masm.mov(r11, scratch1);
    masm.subPtr(Imm32(BaselineFrame::Size()), scratch1);

    
    
    
    masm.movePtr(ImmWord((void *)NULL), BaselineStubReg);
    EmitEnterStubFrame(masm, scratch2);

    IonCompartment *ion = cx->compartment->ionCompartment();
    IonCode *code = ion->getVMWrapper(HandleDebugTrapInfo);
    if (!code)
        return NULL;

    masm.push(lr);
    masm.push(scratch1);
    EmitCallVM(code, masm);

    EmitLeaveStubFrame(masm);

    
    
    
    Label forcedReturn;
    masm.branchTest32(Assembler::NonZero, ReturnReg, ReturnReg, &forcedReturn);
    masm.mov(lr, pc);

    masm.bind(&forcedReturn);
    masm.loadValue(Address(r11, BaselineFrame::reverseOffsetOfReturnValue()),
                   JSReturnOperand);
    masm.mov(r11, sp);
    masm.pop(r11);
    masm.ret();

    Linker linker(masm);
    return linker.newCode(cx, JSC::OTHER_CODE);
}
