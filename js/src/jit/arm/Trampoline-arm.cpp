





#include "jscompartment.h"

#include "jit/arm/BaselineHelpers-arm.h"
#include "jit/Bailouts.h"
#include "jit/JitCompartment.h"
#include "jit/JitFrames.h"
#include "jit/JitSpewer.h"
#include "jit/Linker.h"
#ifdef JS_ION_PERF
# include "jit/PerfSpewer.h"
#endif
#include "jit/VMFunctions.h"

using namespace js;
using namespace js::jit;

static const FloatRegisterSet NonVolatileFloatRegs =
    FloatRegisterSet((1ULL << FloatRegisters::d8) |
                     (1ULL << FloatRegisters::d9) |
                     (1ULL << FloatRegisters::d10) |
                     (1ULL << FloatRegisters::d11) |
                     (1ULL << FloatRegisters::d12) |
                     (1ULL << FloatRegisters::d13) |
                     (1ULL << FloatRegisters::d14) |
                     (1ULL << FloatRegisters::d15));

static void
GenerateReturn(MacroAssembler &masm, int returnCode, SPSProfiler *prof)
{
    
    masm.transferMultipleByRuns(NonVolatileFloatRegs, IsLoad, StackPointer, IA);

    
    masm.addPtr(Imm32(sizeof(void*)), sp);

    
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
    masm.flushBuffer();
}

struct EnterJITStack
{
    double d8;
    double d9;
    double d10;
    double d11;
    double d12;
    double d13;
    double d14;
    double d15;

    
    void *padding;

    
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








JitCode *
JitRuntime::generateEnterJIT(JSContext *cx, EnterJitType type)
{
    const Address slot_token(sp, offsetof(EnterJITStack, token));
    const Address slot_vp(sp, offsetof(EnterJITStack, vp));

    MOZ_ASSERT(OsrFrameReg == r3);

    MacroAssembler masm(cx);
    Assembler *aasm = &masm;

    
    
    
    masm.startDataTransferM(IsStore, sp, DB, WriteBack);
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

    
    masm.subPtr(Imm32(sizeof(void*)), sp);

    
    masm.transferMultipleByRuns(NonVolatileFloatRegs, IsStore, sp, DB);

    
    masm.movePtr(sp, r8);

    
    masm.loadPtr(slot_token, r9);

    
    if (type == EnterJitBaseline)
        masm.movePtr(sp, r11);

    
    masm.loadPtr(slot_vp, r10);
    masm.unboxInt32(Address(r10, 0), r10);

    
    
    
    
    
    
    
    
    
    
    
    aasm->as_sub(r4, sp, O2RegImmShift(r1, LSL, 3));    
    masm.ma_and(Imm32(~(JitStackAlignment - 1)), r4, r4);
    
    static_assert(sizeof(JitFrameLayout) % JitStackAlignment == 0,
      "No need to consider the JitFrameLayout for aligning the stack");
    
    aasm->as_sub(sp, r4, Imm8(sizeof(JitFrameLayout)));

    
    
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
    masm.makeFrameDescriptor(r8, JitFrame_Entry);

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
            AutoForbidPools afp(&masm, 5);
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

#ifdef XP_WIN
        
        
        masm.ma_lsl(Imm32(3), numStackValues, scratch);
        masm.subPtr(scratch, framePtr);
        {
            masm.ma_sub(sp, Imm32(WINDOWS_BIG_FRAME_TOUCH_INCREMENT), scratch);

            Label touchFrameLoop;
            Label touchFrameLoopEnd;
            masm.bind(&touchFrameLoop);
            masm.branchPtr(Assembler::Below, scratch, framePtr, &touchFrameLoopEnd);
            masm.store32(Imm32(0), Address(scratch, 0));
            masm.subPtr(Imm32(WINDOWS_BIG_FRAME_TOUCH_INCREMENT), scratch);
            masm.jump(&touchFrameLoop);
            masm.bind(&touchFrameLoopEnd);
        }
        masm.mov(sp, framePtr);
#endif

        
        masm.ma_lsl(Imm32(3), numStackValues, scratch);
        masm.ma_sub(sp, scratch, sp);

        
        masm.addPtr(Imm32(BaselineFrame::Size() + BaselineFrame::FramePointerOffset), scratch);
        masm.makeFrameDescriptor(scratch, JitFrame_BaselineJS);
        masm.push(scratch);
        masm.push(Imm32(0)); 
        
        masm.enterFakeExitFrame(ExitFrameLayout::BareToken());

        masm.push(framePtr); 
        masm.push(r0); 

        masm.setupUnalignedABICall(3, scratch);
        masm.passABIArg(r11); 
        masm.passABIArg(OsrFrameReg); 
        masm.passABIArg(numStackValues);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, jit::InitBaselineFrameForOsr));

        Register jitcode = regs.takeAny();
        masm.pop(jitcode);
        masm.pop(framePtr);

        MOZ_ASSERT(jitcode != ReturnReg);

        Label error;
        masm.addPtr(Imm32(ExitFrameLayout::SizeWithFooter()), sp);
        masm.addPtr(Imm32(BaselineFrame::Size()), framePtr);
        masm.branchIfFalseBool(ReturnReg, &error);

        
        
        {
            Label skipProfilingInstrumentation;
            Register realFramePtr = numStackValues;
            AbsoluteAddress addressOfEnabled(cx->runtime()->spsProfiler.addressOfEnabled());
            masm.branch32(Assembler::Equal, addressOfEnabled, Imm32(0),
                          &skipProfilingInstrumentation);
            masm.ma_add(framePtr, Imm32(sizeof(void*)), realFramePtr);
            masm.profilerEnterFrame(realFramePtr, scratch);
            masm.bind(&skipProfilingInstrumentation);
        }

        masm.jump(jitcode);

        
        
        masm.bind(&error);
        masm.mov(framePtr, sp);
        masm.addPtr(Imm32(2 * sizeof(uintptr_t)), sp);
        masm.moveValue(MagicValue(JS_ION_ERROR), JSReturnOperand);
        masm.jump(&returnLabel);

        masm.bind(&notOsr);
        
        MOZ_ASSERT(R1.scratchReg() != r0);
        masm.loadPtr(Address(r11, offsetof(EnterJITStack, scopeChain)), R1.scratchReg());
    }

    
    
    masm.assertStackAlignment(JitStackAlignment);

    
    masm.ma_callJitNoPush(r0);

    if (type == EnterJitBaseline) {
        
        masm.bind(&returnLabel);
    }

    
    
    
    aasm->as_sub(sp, sp, Imm8(4));

    
    masm.loadPtr(Address(sp, JitFrameLayout::offsetOfDescriptor()), r5);
    aasm->as_add(sp, sp, lsr(r5, FRAMESIZE_SHIFT));

    
    masm.loadPtr(slot_vp, r5);
    masm.storeValue(JSReturnOperand, Address(r5, 0));

    
    
    
    
    
    

    
    GenerateReturn(masm, true, &cx->runtime()->spsProfiler);

    Linker linker(masm);
    AutoFlushICache afc("EnterJIT");
    JitCode *code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "EnterJIT");
#endif

    return code;
}

JitCode *
JitRuntime::generateInvalidator(JSContext *cx)
{
    
    MacroAssembler masm(cx);
    
    
    
    
    
    
    masm.ma_and(Imm32(~7), sp, sp);
    masm.startDataTransferM(IsStore, sp, DB, WriteBack);
    
    
    for (uint32_t i = 0; i < Registers::Total; i++)
        masm.transferReg(Register::FromCode(i));
    masm.finishDataTransfer();

    
    
    
    if (FloatRegisters::ActualTotalPhys() != FloatRegisters::TotalPhys) {
        int missingRegs = FloatRegisters::TotalPhys - FloatRegisters::ActualTotalPhys();
        masm.ma_sub(Imm32(missingRegs * sizeof(double)), sp);
    }

    masm.startFloatTransferM(IsStore, sp, DB, WriteBack);
    for (uint32_t i = 0; i < FloatRegisters::ActualTotalPhys(); i++)
        masm.transferFloatReg(FloatRegister(i, FloatRegister::Double));
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

    
    JitCode *bailoutTail = cx->runtime()->jitRuntime()->getBailoutTail();
    masm.branch(bailoutTail);

    Linker linker(masm);
    AutoFlushICache afc("Invalidator");
    JitCode *code = linker.newCode<NoGC>(cx, OTHER_CODE);
    JitSpew(JitSpew_IonInvalidate, "   invalidation thunk created at %p", (void *) code->raw());

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "Invalidator");
#endif

    return code;
}

JitCode *
JitRuntime::generateArgumentsRectifier(JSContext *cx, void **returnAddrOut)
{
    MacroAssembler masm(cx);
    masm.pushReturnAddress();

    
    
    MOZ_ASSERT(ArgumentsRectifierReg == r8);

    
    masm.ma_ldr(DTRAddr(sp, DtrOffImm(RectifierFrameLayout::offsetOfNumActualArgs())), r0);

    
    masm.ma_ldr(DTRAddr(sp, DtrOffImm(RectifierFrameLayout::offsetOfCalleeToken())), r1);
    masm.ma_and(Imm32(CalleeTokenMask), r1, r6);
    masm.ma_ldrh(EDtrAddr(r6, EDtrOffImm(JSFunction::offsetOfNargs())), r6);

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

    

    masm.ma_alu(r3, lsl(r8, 3), r3, OpAdd); 
    masm.ma_add(r3, Imm32(sizeof(RectifierFrameLayout)), r3);

    
    {
        Label copyLoopTop;
        masm.bind(&copyLoopTop);
        masm.ma_dataTransferN(IsLoad, 64, true, r3, Imm32(-8), r4, PostIndex);
        masm.ma_dataTransferN(IsStore, 64, true, sp, Imm32(-8), r4, PreIndex);

        masm.ma_sub(r8, Imm32(1), r8, SetCond);
        masm.ma_b(&copyLoopTop, Assembler::NotSigned);
    }

    
    masm.ma_add(r6, Imm32(1), r6);
    masm.ma_lsl(Imm32(3), r6, r6);

    
    masm.makeFrameDescriptor(r6, JitFrame_Rectifier);

    
    masm.ma_push(r0); 
    masm.ma_push(r1); 
    masm.ma_push(r6); 

    
    
    masm.andPtr(Imm32(CalleeTokenMask), r1);
    masm.ma_ldr(DTRAddr(r1, DtrOffImm(JSFunction::offsetOfNativeOrScript())), r3);
    masm.loadBaselineOrIonRaw(r3, r3, nullptr);
    masm.ma_callJitHalfPush(r3);

    uint32_t returnOffset = masm.currentOffset();

    
    
    
    
    
    
    

    
    masm.ma_dtr(IsLoad, sp, Imm32(12), r4, PostIndex);

    
    
    
    
    
    
    

    
    masm.ma_alu(sp, lsr(r4, FRAMESIZE_SHIFT), sp, OpAdd);

    masm.ret();
    Linker linker(masm);
    AutoFlushICache afc("ArgumentsRectifier");
    JitCode *code = linker.newCode<NoGC>(cx, OTHER_CODE);

    CodeOffsetLabel returnLabel(returnOffset);
    returnLabel.fixup(&masm);
    if (returnAddrOut)
        *returnAddrOut = (void *) (code->raw() + returnLabel.offset());

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "ArgumentsRectifier");
#endif

    return code;
}

static void
PushBailoutFrame(MacroAssembler &masm, uint32_t frameClass, Register spArg)
{
    
    
    
    
    
    

    
    
    

    masm.startDataTransferM(IsStore, sp, DB, WriteBack);
    
    
    for (uint32_t i = 0; i < Registers::Total; i++)
        masm.transferReg(Register::FromCode(i));
    masm.finishDataTransfer();

    
    
    
    if (FloatRegisters::ActualTotalPhys() != FloatRegisters::TotalPhys) {
        int missingRegs = FloatRegisters::TotalPhys - FloatRegisters::ActualTotalPhys();
        masm.ma_sub(Imm32(missingRegs * sizeof(double)), sp);
    }
    masm.startFloatTransferM(IsStore, sp, DB, WriteBack);
    for (uint32_t i = 0; i < FloatRegisters::ActualTotalPhys(); i++)
        masm.transferFloatReg(FloatRegister(i, FloatRegister::Double));
    masm.finishFloatTransfer();

    
    
    
    
    
    

    
    masm.ma_mov(Imm32(frameClass), r4);
    
    
    
    
    masm.startDataTransferM(IsStore, sp, DB, WriteBack);
    
    masm.transferReg(r4);
    
    
    masm.transferReg(lr);
    masm.finishDataTransfer();

    masm.ma_mov(sp, spArg);
}

static void
GenerateBailoutThunk(JSContext *cx, MacroAssembler &masm, uint32_t frameClass)
{
    PushBailoutFrame(masm, frameClass, r0);

    
    
    
    const int sizeOfBailoutInfo = sizeof(void *)*2;
    masm.reserveStack(sizeOfBailoutInfo);
    masm.mov(sp, r1);
    masm.setupAlignedABICall(2);

    
    

    
    masm.passABIArg(r0);
    masm.passABIArg(r1);

    
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, Bailout));
    masm.ma_ldr(Address(sp, 0), r2);
    masm.ma_add(sp, Imm32(sizeOfBailoutInfo), sp);
    
    uint32_t bailoutFrameSize = 0
        + sizeof(void *) 
        + sizeof(RegisterDump);

    if (frameClass == NO_FRAME_SIZE_CLASS_ID) {
        
        masm.as_dtr(IsLoad, 32, Offset,
                    r4, DTRAddr(sp, DtrOffImm(4)));
        
        
        
        
        
        
        masm.ma_add(sp, Imm32(bailoutFrameSize+12), sp);
        masm.as_add(sp, sp, O2Reg(r4));
    } else {
        uint32_t frameSize = FrameSizeClass::FromClass(frameClass).frameSize();
        masm.ma_add(Imm32(
                          
                          frameSize
                          
                          
                          + sizeof(void*)
                          
                          + bailoutFrameSize)
                    , sp);
    }

    
    JitCode *bailoutTail = cx->runtime()->jitRuntime()->getBailoutTail();
    masm.branch(bailoutTail);
}

JitCode *
JitRuntime::generateBailoutTable(JSContext *cx, uint32_t frameClass)
{
    MacroAssembler masm(cx);

    {
        
        Label bailout;
        AutoForbidPools afp(&masm, BAILOUT_TABLE_SIZE);
        for (size_t i = 0; i < BAILOUT_TABLE_SIZE; i++)
            masm.ma_bl(&bailout);
        masm.bind(&bailout);
    }

    GenerateBailoutThunk(cx, masm, frameClass);

    Linker linker(masm);
    AutoFlushICache afc("BailoutTable");
    JitCode *code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "BailoutTable");
#endif

    return code;
}

JitCode *
JitRuntime::generateBailoutHandler(JSContext *cx)
{
    MacroAssembler masm(cx);
    GenerateBailoutThunk(cx, masm, NO_FRAME_SIZE_CLASS_ID);

    Linker linker(masm);
    AutoFlushICache afc("BailoutHandler");
    JitCode *code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "BailoutHandler");
#endif

    return code;
}

JitCode *
JitRuntime::generateVMWrapper(JSContext *cx, const VMFunction &f)
{
    MOZ_ASSERT(functionWrappers_);
    MOZ_ASSERT(functionWrappers_->initialized());
    VMWrapperMap::AddPtr p = functionWrappers_->lookupForAdd(&f);
    if (p)
        return p->value();

    
    MacroAssembler masm(cx);
    GeneralRegisterSet regs = GeneralRegisterSet(Register::Codes::WrapperMask);

    
    JS_STATIC_ASSERT((Register::Codes::VolatileMask & ~Register::Codes::WrapperMask) == 0);

    
    Register cxreg = r0;
    regs.take(cxreg);

    
    
    
    
    
    
    
    if (f.expectTailCall == NonTailCall)
        masm.pushReturnAddress();

    masm.enterExitFrame(&f);
    masm.loadJSContext(cxreg);

    
    Register argsBase = InvalidReg;
    if (f.explicitArgs) {
        argsBase = r5;
        regs.take(argsBase);
        masm.ma_add(sp, Imm32(ExitFrameLayout::SizeWithFooter()), argsBase);
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
      case Type_Pointer:
      case Type_Bool:
        outReg = r4;
        regs.take(outReg);
        masm.reserveStack(sizeof(int32_t));
        masm.ma_mov(sp, outReg);
        break;

      case Type_Double:
        outReg = r4;
        regs.take(outReg);
        masm.reserveStack(sizeof(double));
        masm.ma_mov(sp, outReg);
        break;

      default:
        MOZ_ASSERT(f.outParam == Type_Void);
        break;
    }

    masm.setupUnalignedABICall(f.argc(), regs.getAny());
    masm.passABIArg(cxreg);

    size_t argDisp = 0;

    
    for (uint32_t explicitArg = 0; explicitArg < f.explicitArgs; explicitArg++) {
        MoveOperand from;
        switch (f.argProperties(explicitArg)) {
          case VMFunction::WordByValue:
            masm.passABIArg(MoveOperand(argsBase, argDisp), MoveOp::GENERAL);
            argDisp += sizeof(void *);
            break;
          case VMFunction::DoubleByValue:
            
            
            MOZ_ASSERT(f.argPassedInFloatReg(explicitArg));
            masm.passABIArg(MoveOperand(argsBase, argDisp), MoveOp::DOUBLE);
            argDisp += sizeof(double);
            break;
          case VMFunction::WordByRef:
            masm.passABIArg(MoveOperand(argsBase, argDisp, MoveOperand::EFFECTIVE_ADDRESS), MoveOp::GENERAL);
            argDisp += sizeof(void *);
            break;
          case VMFunction::DoubleByRef:
            masm.passABIArg(MoveOperand(argsBase, argDisp, MoveOperand::EFFECTIVE_ADDRESS), MoveOp::GENERAL);
            argDisp += 2 * sizeof(void *);
            break;
        }
    }

    
    if (outReg != InvalidReg)
        masm.passABIArg(outReg);

    masm.callWithABI(f.wrapped);

    
    switch (f.failType()) {
      case Type_Object:
        masm.branchTestPtr(Assembler::Zero, r0, r0, masm.failureLabel());
        break;
      case Type_Bool:
        masm.branchIfFalseBool(r0, masm.failureLabel());
        break;
      default:
        MOZ_CRASH("unknown failure kind");
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
      case Type_Pointer:
        masm.load32(Address(sp, 0), ReturnReg);
        masm.freeStack(sizeof(int32_t));
        break;

      case Type_Bool:
        masm.load8ZeroExtend(Address(sp, 0), ReturnReg);
        masm.freeStack(sizeof(int32_t));
        break;

      case Type_Double:
        if (cx->runtime()->jitSupportsFloatingPoint)
            masm.loadDouble(Address(sp, 0), ReturnDoubleReg);
        else
            masm.assumeUnreachable("Unable to load into float reg, with no FP support.");
        masm.freeStack(sizeof(double));
        break;

      default:
        MOZ_ASSERT(f.outParam == Type_Void);
        break;
    }
    masm.leaveExitFrame();
    masm.retn(Imm32(sizeof(ExitFrameLayout) +
                    f.explicitStackSlots() * sizeof(void *) +
                    f.extraValuesToPop * sizeof(Value)));

    Linker linker(masm);
    AutoFlushICache afc("VMWrapper");
    JitCode *wrapper = linker.newCode<NoGC>(cx, OTHER_CODE);
    if (!wrapper)
        return nullptr;

    
    
    if (!functionWrappers_->relookupOrAdd(p, &f, wrapper))
        return nullptr;

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(wrapper, "VMWrapper");
#endif

    return wrapper;
}

JitCode *
JitRuntime::generatePreBarrier(JSContext *cx, MIRType type)
{
    MacroAssembler masm(cx);

    RegisterSet save;
    if (cx->runtime()->jitSupportsFloatingPoint) {
        save = RegisterSet(GeneralRegisterSet(Registers::VolatileMask),
                           FloatRegisterSet(FloatRegisters::VolatileDoubleMask));
    } else {
        save = RegisterSet(GeneralRegisterSet(Registers::VolatileMask),
                           FloatRegisterSet());
    }
    save.add(lr);
    masm.PushRegsInMask(save);

    MOZ_ASSERT(PreBarrierReg == r1);
    masm.movePtr(ImmPtr(cx->runtime()), r0);

    masm.setupUnalignedABICall(2, r2);
    masm.passABIArg(r0);
    masm.passABIArg(r1);
    masm.callWithABI(IonMarkFunction(type));
    save.take(AnyRegister(lr));
    save.add(pc);
    masm.PopRegsInMask(save);

    Linker linker(masm);
    AutoFlushICache afc("PreBarrier");
    JitCode *code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "PreBarrier");
#endif

    return code;
}

typedef bool (*HandleDebugTrapFn)(JSContext *, BaselineFrame *, uint8_t *, bool *);
static const VMFunction HandleDebugTrapInfo = FunctionInfo<HandleDebugTrapFn>(HandleDebugTrap);

JitCode *
JitRuntime::generateDebugTrapHandler(JSContext *cx)
{
    MacroAssembler masm;

    Register scratch1 = r0;
    Register scratch2 = r1;

    
    masm.mov(r11, scratch1);
    masm.subPtr(Imm32(BaselineFrame::Size()), scratch1);

    
    
    
    masm.movePtr(ImmPtr(nullptr), BaselineStubReg);
    EmitEnterStubFrame(masm, scratch2);

    JitCode *code = cx->runtime()->jitRuntime()->getVMWrapper(HandleDebugTrapInfo);
    if (!code)
        return nullptr;

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

    
    
    {
        Label skipProfilingInstrumentation;
        AbsoluteAddress addressOfEnabled(cx->runtime()->spsProfiler.addressOfEnabled());
        masm.branch32(Assembler::Equal, addressOfEnabled, Imm32(0), &skipProfilingInstrumentation);
        masm.profilerExitFrame();
        masm.bind(&skipProfilingInstrumentation);
    }

    masm.ret();

    Linker linker(masm);
    AutoFlushICache afc("DebugTrapHandler");
    JitCode *codeDbg = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(codeDbg, "DebugTrapHandler");
#endif

    return codeDbg;
}

JitCode *
JitRuntime::generateExceptionTailStub(JSContext *cx, void *handler)
{
    MacroAssembler masm;

    masm.handleFailureWithHandlerTail(handler);

    Linker linker(masm);
    AutoFlushICache afc("ExceptionTailStub");
    JitCode *code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "ExceptionTailStub");
#endif

    return code;
}

JitCode *
JitRuntime::generateBailoutTailStub(JSContext *cx)
{
    MacroAssembler masm;

    masm.generateBailoutTail(r1, r2);

    Linker linker(masm);
    AutoFlushICache afc("BailoutTailStub");
    JitCode *code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "BailoutTailStub");
#endif

    return code;
}

JitCode *
JitRuntime::generateProfilerExitFrameTailStub(JSContext *cx)
{
    MacroAssembler masm;

    Register scratch1 = r5;
    Register scratch2 = r6;
    Register scratch3 = r7;
    Register scratch4 = r8;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    Register actReg = scratch4;
    AbsoluteAddress activationAddr(GetJitContext()->runtime->addressOfProfilingActivation());
    masm.loadPtr(activationAddr, actReg);

    Address lastProfilingFrame(actReg, JitActivation::offsetOfLastProfilingFrame());
    Address lastProfilingCallSite(actReg, JitActivation::offsetOfLastProfilingCallSite());

#ifdef DEBUG
    
    {
        masm.loadPtr(lastProfilingFrame, scratch1);
        Label checkOk;
        masm.branchPtr(Assembler::Equal, scratch1, ImmWord(0), &checkOk);
        masm.branchPtr(Assembler::Equal, StackPointer, scratch1, &checkOk);
        masm.assumeUnreachable(
            "Mismatch between stored lastProfilingFrame and current stack pointer.");
        masm.bind(&checkOk);
    }
#endif

    
    masm.loadPtr(Address(StackPointer, JitFrameLayout::offsetOfDescriptor()), scratch1);

    
    
    
    masm.ma_and(Imm32((1 << FRAMESIZE_SHIFT) - 1), scratch1, scratch2);
    masm.rshiftPtr(Imm32(FRAMESIZE_SHIFT), scratch1);

    
    Label handle_IonJS;
    Label handle_BaselineStub;
    Label handle_Rectifier;
    Label handle_IonAccessorIC;
    Label handle_Entry;
    Label end;

    masm.branch32(Assembler::Equal, scratch2, Imm32(JitFrame_IonJS), &handle_IonJS);
    masm.branch32(Assembler::Equal, scratch2, Imm32(JitFrame_BaselineJS), &handle_IonJS);
    masm.branch32(Assembler::Equal, scratch2, Imm32(JitFrame_BaselineStub), &handle_BaselineStub);
    masm.branch32(Assembler::Equal, scratch2, Imm32(JitFrame_Rectifier), &handle_Rectifier);
    masm.branch32(Assembler::Equal, scratch2, Imm32(JitFrame_IonAccessorIC), &handle_IonAccessorIC);
    masm.branch32(Assembler::Equal, scratch2, Imm32(JitFrame_Entry), &handle_Entry);

    masm.assumeUnreachable("Invalid caller frame type when exiting from Ion frame.");

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    masm.bind(&handle_IonJS);
    {
        

        
        
        masm.loadPtr(Address(StackPointer, JitFrameLayout::offsetOfReturnAddress()), scratch2);
        masm.storePtr(scratch2, lastProfilingCallSite);

        
        
        masm.ma_add(StackPointer, scratch1, scratch2);
        masm.ma_add(scratch2, Imm32(JitFrameLayout::Size()), scratch2);
        masm.storePtr(scratch2, lastProfilingFrame);
        masm.ret();
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    masm.bind(&handle_BaselineStub);
    {
        masm.ma_add(StackPointer, scratch1, scratch3);
        Address stubFrameReturnAddr(scratch3,
                                    JitFrameLayout::Size() +
                                    BaselineStubFrameLayout::offsetOfReturnAddress());
        masm.loadPtr(stubFrameReturnAddr, scratch2);
        masm.storePtr(scratch2, lastProfilingCallSite);

        Address stubFrameSavedFramePtr(scratch3,
                                       JitFrameLayout::Size() - (2 * sizeof(void *)));
        masm.loadPtr(stubFrameSavedFramePtr, scratch2);
        masm.addPtr(Imm32(sizeof(void *)), scratch2); 
        masm.storePtr(scratch2, lastProfilingFrame);
        masm.ret();
    }


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    masm.bind(&handle_Rectifier);
    {
        
        masm.ma_add(StackPointer, scratch1, scratch2);
        masm.add32(Imm32(JitFrameLayout::Size()), scratch2);
        masm.loadPtr(Address(scratch2, RectifierFrameLayout::offsetOfDescriptor()), scratch3);
        masm.ma_lsr(Imm32(FRAMESIZE_SHIFT), scratch3, scratch1);
        masm.and32(Imm32((1 << FRAMETYPE_BITS) - 1), scratch3);

        
        
        

        
        Label handle_Rectifier_BaselineStub;
        masm.branch32(Assembler::NotEqual, scratch3, Imm32(JitFrame_IonJS),
                      &handle_Rectifier_BaselineStub);

        
        
        masm.loadPtr(Address(scratch2, RectifierFrameLayout::offsetOfReturnAddress()), scratch3);
        masm.storePtr(scratch3, lastProfilingCallSite);

        
        masm.ma_add(scratch2, scratch1, scratch3);
        masm.add32(Imm32(RectifierFrameLayout::Size()), scratch3);
        masm.storePtr(scratch3, lastProfilingFrame);
        masm.ret();

        
        masm.bind(&handle_Rectifier_BaselineStub);
#ifdef DEBUG
        {
            Label checkOk;
            masm.branch32(Assembler::Equal, scratch3, Imm32(JitFrame_BaselineStub), &checkOk);
            masm.assumeUnreachable("Unrecognized frame preceding baselineStub.");
            masm.bind(&checkOk);
        }
#endif
        masm.ma_add(scratch2, scratch1, scratch3);
        Address stubFrameReturnAddr(scratch3, RectifierFrameLayout::Size() +
                                              BaselineStubFrameLayout::offsetOfReturnAddress());
        masm.loadPtr(stubFrameReturnAddr, scratch2);
        masm.storePtr(scratch2, lastProfilingCallSite);

        Address stubFrameSavedFramePtr(scratch3,
                                       RectifierFrameLayout::Size() - (2 * sizeof(void *)));
        masm.loadPtr(stubFrameSavedFramePtr, scratch2);
        masm.addPtr(Imm32(sizeof(void *)), scratch2);
        masm.storePtr(scratch2, lastProfilingFrame);
        masm.ret();
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    masm.bind(&handle_IonAccessorIC);
    {
        
        masm.ma_add(StackPointer, scratch1, scratch2);
        masm.addPtr(Imm32(JitFrameLayout::Size()), scratch2);

        
        masm.loadPtr(Address(scratch2, IonAccessorICFrameLayout::offsetOfDescriptor()), scratch3);
#ifdef DEBUG
        
        masm.movePtr(scratch3, scratch1);
        masm.and32(Imm32((1 << FRAMETYPE_BITS) - 1), scratch1);
        {
            Label checkOk;
            masm.branch32(Assembler::Equal, scratch1, Imm32(JitFrame_IonJS), &checkOk);
            masm.assumeUnreachable("IonAccessorIC frame must be preceded by IonJS frame");
            masm.bind(&checkOk);
        }
#endif
        masm.rshiftPtr(Imm32(FRAMESIZE_SHIFT), scratch3);

        
        masm.loadPtr(Address(scratch2, IonAccessorICFrameLayout::offsetOfReturnAddress()), scratch1);
        masm.storePtr(scratch1, lastProfilingCallSite);

        
        
        masm.ma_add(scratch2, scratch3, scratch1);
        masm.addPtr(Imm32(IonAccessorICFrameLayout::Size()), scratch1);
        masm.storePtr(scratch1, lastProfilingFrame);
        masm.ret();
    }

    
    
    
    
    
    masm.bind(&handle_Entry);
    {
        masm.movePtr(ImmPtr(nullptr), scratch1);
        masm.storePtr(scratch1, lastProfilingCallSite);
        masm.storePtr(scratch1, lastProfilingFrame);
        masm.ret();
    }

    Linker linker(masm);
    AutoFlushICache afc("ProfilerExitFrameTailStub");
    JitCode *code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "ProfilerExitFrameStub");
#endif

    return code;
}
