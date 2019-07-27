





#include "jit/Bailouts.h"
#include "jit/JitCompartment.h"
#include "jit/JitFrames.h"
#include "jit/Linker.h"
#ifdef JS_ION_PERF
# include "jit/PerfSpewer.h"
#endif
#include "jit/arm64/SharedICHelpers-arm64.h"
#include "jit/VMFunctions.h"

using namespace js;
using namespace js::jit;



static const LiveRegisterSet AllRegs =
    LiveRegisterSet(GeneralRegisterSet(Registers::AllMask & ~(1 << 31 | 1 << 30 | 1 << 29| 1 << 28)),
                FloatRegisterSet(FloatRegisters::AllMask));






JitCode*
JitRuntime::generateEnterJIT(JSContext* cx, EnterJitType type)
{
    MacroAssembler masm(cx);

    const Register reg_code      = IntArgReg0; 
    const Register reg_argc      = IntArgReg1; 
    const Register reg_argv      = IntArgReg2; 
    const Register reg_osrFrame  = IntArgReg3; 
    const Register reg_callee    = IntArgReg4; 
    const Register reg_scope     = IntArgReg5; 
    const Register reg_osrNStack = IntArgReg6; 
    const Register reg_vp        = IntArgReg7; 

    MOZ_ASSERT(OsrFrameReg == IntArgReg3);

    
    masm.SetStackPointer64(sp);

    
    masm.push(r29, r30);
    masm.moveStackPtrTo(r29);

    
    
    masm.push(r19, r20, r21, r22);
    masm.push(r23, r24, r25, r26);
    masm.push(r27, r28, r7,  r30);

    
    
    masm.push(d8,  d9,  d10, d11);
    masm.push(d12, d13, d14, d15);

#ifdef DEBUG
    
    masm.movePtr(ImmWord(0xdeadd00d), r23);
    masm.movePtr(ImmWord(0xdeadd11d), r24);
    masm.push(r23, r24);
#endif

    
    
    
    
    
    
    
    
    masm.Mov(PseudoStackPointer64, sp);
    masm.SetStackPointer64(PseudoStackPointer64);

    
    masm.moveStackPtrTo(BaselineFrameReg);
    
    masm.moveStackPtrTo(r19);

    
    {
        Label noNewTarget;
        Imm32 constructingToken(CalleeToken_FunctionConstructing);
        masm.branchTest32(Assembler::Zero, reg_callee, constructingToken, &noNewTarget);
        masm.add32(Imm32(1), reg_argc);
        masm.bind(&noNewTarget);
    }

    
    
    
    
    
    

    
    
    {
        vixl::UseScratchRegisterScope temps(&masm.asVIXL());

        const ARMRegister tmp_argc = temps.AcquireX();
        const ARMRegister tmp_sp = temps.AcquireX();

        Label noArguments;
        Label loopHead;

        masm.movePtr(reg_argc, tmp_argc.asUnsized());

        
        
        
        masm.subFromStackPtr(Imm32(8));

        
        masm.Sub(PseudoStackPointer64, PseudoStackPointer64, Operand(tmp_argc, vixl::SXTX, 3));

        
        masm.andToStackPtr(Imm32(~0xff));
        masm.moveStackPtrTo(tmp_sp.asUnsized());

        masm.branchTestPtr(Assembler::Zero, reg_argc, reg_argc, &noArguments);

        
        
        {
            masm.bind(&loopHead);

            
            masm.Ldr(x24, MemOperand(ARMRegister(reg_argv, 64), Operand(8), vixl::PostIndex));

            
            masm.Str(x24, MemOperand(tmp_sp, Operand(8), vixl::PostIndex));

            
            masm.Subs(tmp_argc, tmp_argc, Operand(1));

            
            masm.B(&loopHead, vixl::Condition::ge);
        }

        masm.bind(&noArguments);
    }
    masm.checkStackAlignment();

    
    
    
    masm.unboxInt32(Address(reg_vp, 0x0), ip0);
    masm.push(ip0, reg_callee);
    masm.checkStackAlignment();

    
    masm.subStackPtrFrom(r19);

    
    masm.makeFrameDescriptor(r19, JitFrame_Entry);
    masm.Push(r19);

    Label osrReturnPoint;
    if (type == EnterJitBaseline) {
        
        Label notOsr;
        masm.branchTestPtr(Assembler::Zero, OsrFrameReg, OsrFrameReg, &notOsr);

        
        masm.Adr(ScratchReg2_64, &osrReturnPoint);
        masm.push(ScratchReg2, BaselineFrameReg);

        
        masm.subFromStackPtr(Imm32(BaselineFrame::Size()));
        masm.moveStackPtrTo(BaselineFrameReg);

        
        masm.Lsl(w19, ARMRegister(reg_osrNStack, 32), 3); 
        masm.subFromStackPtr(r19);

        
        masm.addPtr(Imm32(BaselineFrame::Size() + BaselineFrame::FramePointerOffset), r19);
        masm.makeFrameDescriptor(r19, JitFrame_BaselineJS);
        masm.asVIXL().Push(x19, xzr); 
        
        masm.enterFakeExitFrame(ExitFrameLayout::BareToken());

        masm.push(BaselineFrameReg, reg_code);

        
        masm.setupUnalignedABICall(3, r19);
        masm.passABIArg(BaselineFrameReg); 
        masm.passABIArg(reg_osrFrame); 
        masm.passABIArg(reg_osrNStack);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void*, jit::InitBaselineFrameForOsr));

        masm.pop(r19, BaselineFrameReg);
        MOZ_ASSERT(r19 != ReturnReg);

        masm.addToStackPtr(Imm32(ExitFrameLayout::SizeWithFooter()));
        masm.addPtr(Imm32(BaselineFrame::Size()), BaselineFrameReg);

        Label error;
        masm.branchIfFalseBool(ReturnReg, &error);

        masm.jump(r19);

        
        
        masm.bind(&error);
        masm.Add(masm.GetStackPointer64(), BaselineFrameReg64, Operand(2 * sizeof(uintptr_t)));
        masm.syncStackPtr();
        masm.moveValue(MagicValue(JS_ION_ERROR), JSReturnOperand);
        masm.B(&osrReturnPoint);

        masm.bind(&notOsr);
        masm.movePtr(reg_scope, R1_);
    }

    
    
    masm.call(reg_code);

    
    if (type == EnterJitBaseline)
        masm.bind(&osrReturnPoint);

    
    masm.Pop(r19);
    masm.Add(masm.GetStackPointer64(), masm.GetStackPointer64(),
             Operand(x19, vixl::LSR, FRAMESIZE_SHIFT));
    masm.syncStackPtr();
    masm.SetStackPointer64(sp);

#ifdef DEBUG
    
    masm.pop(r24, r23);
    Label x23OK, x24OK;

    masm.branchPtr(Assembler::Equal, r23, ImmWord(0xdeadd00d), &x23OK);
    masm.breakpoint();
    masm.bind(&x23OK);

    masm.branchPtr(Assembler::Equal, r24, ImmWord(0xdeadd11d), &x24OK);
    masm.breakpoint();
    masm.bind(&x24OK);
#endif

    
    masm.pop(d15, d14, d13, d12);
    masm.pop(d11, d10,  d9,  d8);

    
    
    masm.pop(r30, r7,  r28, r27);
    masm.pop(r26, r25, r24, r23);
    masm.pop(r22, r21, r20, r19);

    
    masm.storeValue(JSReturnOperand, Address(reg_vp, 0));

    
    masm.pop(r30, r29);

    
    masm.abiret();

    Linker linker(masm);
    JitCode* code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "EnterJIT");
#endif

    return code;
}

JitCode*
JitRuntime::generateInvalidator(JSContext* cx)
{
    MacroAssembler masm;

    masm.push(r0, r1, r2, r3);

    masm.PushRegsInMask(AllRegs);
    masm.moveStackPtrTo(r0);

    masm.Sub(x1, masm.GetStackPointer64(), Operand(sizeof(size_t)));
    masm.Sub(x2, masm.GetStackPointer64(), Operand(sizeof(size_t) + sizeof(void*)));
    masm.moveToStackPtr(r2);

    masm.setupUnalignedABICall(3, r10);
    masm.passABIArg(r0);
    masm.passABIArg(r1);
    masm.passABIArg(r2);

    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void*, InvalidationBailout));

    masm.pop(r2, r1);

    masm.Add(masm.GetStackPointer64(), masm.GetStackPointer64(), x1);
    masm.Add(masm.GetStackPointer64(), masm.GetStackPointer64(),
             Operand(sizeof(InvalidationBailoutStack)));
    masm.syncStackPtr();

    JitCode* bailoutTail = cx->runtime()->jitRuntime()->getBailoutTail();
    masm.branch(bailoutTail);

    Linker linker(masm);
    return linker.newCode<NoGC>(cx, OTHER_CODE);
}

JitCode*
JitRuntime::generateArgumentsRectifier(JSContext* cx, void** returnAddrOut)
{
    MacroAssembler masm;

    
    masm.push(lr);

    
    masm.Ldr(w0, MemOperand(masm.GetStackPointer64(), RectifierFrameLayout::offsetOfNumActualArgs()));
    masm.Ldr(x1, MemOperand(masm.GetStackPointer64(), RectifierFrameLayout::offsetOfCalleeToken()));

    
    
    masm.And(x5, x1, Operand(CalleeTokenMask));

    
    masm.Ldrh(x6, MemOperand(x5, JSFunction::offsetOfNargs()));

    static_assert(CalleeToken_FunctionConstructing == 0x1, "Constructing must be low-order bit");
    masm.And(x4, x1, Operand(CalleeToken_FunctionConstructing));
    masm.Add(x7, x6, x4);

    
    MOZ_ASSERT(ArgumentsRectifierReg == r8, "x8 used for argc in Arguments Rectifier");
    masm.Add(x3, masm.GetStackPointer64(), Operand(x8, vixl::LSL, 3));
    masm.Add(x3, x3, Operand(sizeof(RectifierFrameLayout)));

    
    
    
    Label noPadding;
    masm.Tbnz(x7, 0, &noPadding);
    masm.asVIXL().Push(xzr);
    masm.Add(x7, x7, Operand(1));
    masm.bind(&noPadding);

    {
        Label notConstructing;
        masm.Cbz(x4, &notConstructing);

        
        
        
        masm.loadPtr(Address(r3, sizeof(Value)), r4);
        masm.Push(r4);

        masm.bind(&notConstructing);
    }

    
    masm.Sub(w2, w6, w8);

    
    masm.moveValue(UndefinedValue(), r4);

    
    {
        Label undefLoopTop;
        masm.bind(&undefLoopTop);
        masm.Push(r4);
        masm.Subs(w2, w2, Operand(1));
        masm.B(&undefLoopTop, Assembler::NonZero);
    }

    
    {
        Label copyLoopTop;
        masm.bind(&copyLoopTop);
        masm.Ldr(x4, MemOperand(x3, -sizeof(Value), vixl::PostIndex));
        masm.Push(r4);
        masm.Subs(x8, x8, Operand(1));
        masm.B(&copyLoopTop, Assembler::NotSigned);
    }

    
    masm.Add(x6, x7, Operand(1));
    masm.Lsl(x6, x6, 3);

    
    masm.makeFrameDescriptor(r6, JitFrame_Rectifier);

    masm.push(r0,  
              r1,  
              r6); 

    
    masm.Ldr(x3, MemOperand(x5, JSFunction::offsetOfNativeOrScript()));
    masm.loadBaselineOrIonRaw(r3, r3, nullptr);
    masm.call(r3);
    uint32_t returnOffset = masm.currentOffset();

    
    
    masm.Ldr(x4, MemOperand(masm.GetStackPointer64(), 24, vixl::PostIndex));

    
    
    masm.Add(masm.GetStackPointer64(), masm.GetStackPointer64(),
             Operand(x4, vixl::LSR, FRAMESIZE_SHIFT));

    
    masm.ret();

    Linker linker(masm);
    JitCode* code = linker.newCode<NoGC>(cx, OTHER_CODE);

    if (returnAddrOut) {
        CodeOffsetLabel returnLabel(returnOffset);
        returnLabel.fixup(&masm);
        *returnAddrOut = (void*) (code->raw() + returnLabel.offset());
    }

    return code;
}

static void
PushBailoutFrame(MacroAssembler& masm, uint32_t frameClass, Register spArg)
{
    
    
    
    
    
    

    
    
    

    
    
    masm.subFromStackPtr(Imm32(Registers::TotalPhys * sizeof(void*)));
    for (uint32_t i = 0; i < Registers::TotalPhys; i += 2) {
        masm.Stp(ARMRegister::XRegFromCode(i),
                 ARMRegister::XRegFromCode(i + 1),
                 MemOperand(masm.GetStackPointer64(), i * sizeof(void*)));
    }

    
    
    
    masm.subFromStackPtr(Imm32(FloatRegisters::TotalPhys * sizeof(double)));
    for (uint32_t i = 0; i < FloatRegisters::TotalPhys; i += 2) {
        masm.Stp(ARMFPRegister::DRegFromCode(i),
                 ARMFPRegister::DRegFromCode(i + 1),
                 MemOperand(masm.GetStackPointer64(), i * sizeof(void*)));
    }

    
    
    
    
    
    

    
    masm.Mov(x9, frameClass);

    
    
    
    
    masm.push(r30, r9);
    masm.moveStackPtrTo(spArg);
}

static void
GenerateBailoutThunk(JSContext* cx, MacroAssembler& masm, uint32_t frameClass)
{
    PushBailoutFrame(masm, frameClass, r0);

    
    
    
    
    const int sizeOfBailoutInfo = sizeof(void*) * 2;
    masm.reserveStack(sizeOfBailoutInfo);
    masm.moveStackPtrTo(r1);

    masm.setupUnalignedABICall(2, r2);
    masm.passABIArg(r0);
    masm.passABIArg(r1);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void*, Bailout));

    masm.Ldr(x2, MemOperand(masm.GetStackPointer64(), 0));
    masm.addToStackPtr(Imm32(sizeOfBailoutInfo));

    static const uint32_t BailoutDataSize = sizeof(void*) * Registers::Total +
                                            sizeof(double) * FloatRegisters::TotalPhys;

    if (frameClass == NO_FRAME_SIZE_CLASS_ID) {
        vixl::UseScratchRegisterScope temps(&masm.asVIXL());
        const ARMRegister scratch64 = temps.AcquireX();

        masm.Ldr(scratch64, MemOperand(masm.GetStackPointer64(), sizeof(uintptr_t)));
        masm.addToStackPtr(Imm32(BailoutDataSize + 32));
        masm.addToStackPtr(scratch64.asUnsized());
    } else {
        uint32_t frameSize = FrameSizeClass::FromClass(frameClass).frameSize();
        masm.addToStackPtr(Imm32(frameSize + BailoutDataSize + sizeof(void*)));
    }

    
    JitCode* bailoutTail = cx->runtime()->jitRuntime()->getBailoutTail();
    masm.branch(bailoutTail);
}

JitCode*
JitRuntime::generateBailoutTable(JSContext* cx, uint32_t frameClass)
{
    
    MacroAssembler masm;
    masm.breakpoint();
    Linker linker(masm);
    return linker.newCode<NoGC>(cx, OTHER_CODE);
}

JitCode*
JitRuntime::generateBailoutHandler(JSContext* cx)
{
    MacroAssembler masm(cx);
    GenerateBailoutThunk(cx, masm, NO_FRAME_SIZE_CLASS_ID);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "BailoutHandler");
#endif

    Linker linker(masm);
    return linker.newCode<NoGC>(cx, OTHER_CODE);
}

JitCode*
JitRuntime::generateVMWrapper(JSContext* cx, const VMFunction& f)
{
    MOZ_ASSERT(functionWrappers_);
    MOZ_ASSERT(functionWrappers_->initialized());
    VMWrapperMap::AddPtr p = functionWrappers_->lookupForAdd(&f);
    if (p)
        return p->value();

    MacroAssembler masm(cx);

    
    
    AllocatableGeneralRegisterSet regs(Register::Codes::WrapperMask);

    
    JS_STATIC_ASSERT((Register::Codes::VolatileMask & ~Register::Codes::WrapperMask) == 0);

    
    
    
    
    masm.push(lr);

    
    Register reg_cx = IntArgReg0;
    regs.take(reg_cx);

    
    
    
    
    
    
    
    masm.enterExitFrame(&f);
    masm.loadJSContext(reg_cx);

    
    Register argsBase = InvalidReg;
    if (f.explicitArgs) {
        
        
        argsBase = r8;
        regs.take(argsBase);
        masm.Add(ARMRegister(argsBase, 64), masm.GetStackPointer64(),
                 Operand(ExitFrameLayout::SizeWithFooter()));
    }

    
    Register outReg = InvalidReg;
    switch (f.outParam) {
      case Type_Value:
        outReg = regs.takeAny();
        masm.reserveStack(sizeof(Value));
        masm.moveStackPtrTo(outReg);
        break;

      case Type_Handle:
        outReg = regs.takeAny();
        masm.PushEmptyRooted(f.outParamRootType);
        masm.moveStackPtrTo(outReg);
        break;

      case Type_Int32:
      case Type_Bool:
        outReg = regs.takeAny();
        masm.reserveStack(sizeof(int64_t));
        masm.moveStackPtrTo(outReg);
        break;

      case Type_Double:
        outReg = regs.takeAny();
        masm.reserveStack(sizeof(double));
        masm.moveStackPtrTo(outReg);
        break;

      case Type_Pointer:
        outReg = regs.takeAny();
        masm.reserveStack(sizeof(uintptr_t));
        masm.moveStackPtrTo(outReg);
        break;

      default:
        MOZ_ASSERT(f.outParam == Type_Void);
        break;
    }

    masm.setupUnalignedABICall(f.argc(), regs.getAny());
    masm.passABIArg(reg_cx);

    size_t argDisp = 0;

    
    for (uint32_t explicitArg = 0; explicitArg < f.explicitArgs; explicitArg++) {
        MoveOperand from;
        switch (f.argProperties(explicitArg)) {
          case VMFunction::WordByValue:
            masm.passABIArg(MoveOperand(argsBase, argDisp),
                            (f.argPassedInFloatReg(explicitArg) ? MoveOp::DOUBLE : MoveOp::GENERAL));
            argDisp += sizeof(void*);
            break;

          case VMFunction::WordByRef:
            masm.passABIArg(MoveOperand(argsBase, argDisp, MoveOperand::EFFECTIVE_ADDRESS),
                            MoveOp::GENERAL);
            argDisp += sizeof(void*);
            break;

          case VMFunction::DoubleByValue:
          case VMFunction::DoubleByRef:
            MOZ_CRASH("NYI: AArch64 callVM should not be used with 128bit values.");
        }
    }

    
    
    
    
    if (outReg != InvalidReg)
        masm.passABIArg(outReg);

    masm.callWithABI(f.wrapped);

    
    if (!masm.GetStackPointer64().Is(vixl::sp))
        masm.Mov(masm.GetStackPointer64(), vixl::sp);

    
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
      case Type_Value:
        masm.Ldr(ARMRegister(JSReturnReg, 64), MemOperand(masm.GetStackPointer64()));
        masm.freeStack(sizeof(Value));
        break;

      case Type_Handle:
        masm.popRooted(f.outParamRootType, ReturnReg, JSReturnOperand);
        break;

      case Type_Int32:
        masm.Ldr(ARMRegister(ReturnReg, 32), MemOperand(masm.GetStackPointer64()));
        masm.freeStack(sizeof(int64_t));
        break;

      case Type_Bool:
        masm.Ldrb(ARMRegister(ReturnReg, 32), MemOperand(masm.GetStackPointer64()));
        masm.freeStack(sizeof(int64_t));
        break;

      case Type_Double:
        MOZ_ASSERT(cx->runtime()->jitSupportsFloatingPoint);
        masm.Ldr(ARMFPRegister(ReturnDoubleReg, 64), MemOperand(masm.GetStackPointer64()));
        masm.freeStack(sizeof(double));
        break;

      case Type_Pointer:
        masm.Ldr(ARMRegister(ReturnReg, 64), MemOperand(masm.GetStackPointer64()));
        masm.freeStack(sizeof(uintptr_t));
        break;

      default:
        MOZ_ASSERT(f.outParam == Type_Void);
        break;
    }

    masm.leaveExitFrame();
    masm.retn(Imm32(sizeof(ExitFrameLayout) +
              f.explicitStackSlots() * sizeof(void*) +
              f.extraValuesToPop * sizeof(Value)));

    Linker linker(masm);
    JitCode* wrapper = linker.newCode<NoGC>(cx, OTHER_CODE);
    if (!wrapper)
        return nullptr;

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(wrapper, "VMWrapper");
#endif

    
    
    if (!functionWrappers_->relookupOrAdd(p, &f, wrapper))
        return nullptr;

    return wrapper;
}

JitCode*
JitRuntime::generatePreBarrier(JSContext* cx, MIRType type)
{
    MacroAssembler masm(cx);

    LiveRegisterSet regs = LiveRegisterSet(GeneralRegisterSet(Registers::VolatileMask),
                                           FloatRegisterSet(FloatRegisters::VolatileMask));

    
    regs.add(lr);

    masm.PushRegsInMask(regs);

    MOZ_ASSERT(PreBarrierReg == r1);
    masm.movePtr(ImmPtr(cx->runtime()), r3);

    masm.setupUnalignedABICall(2, r0);
    masm.passABIArg(r3);
    masm.passABIArg(PreBarrierReg);
    masm.callWithABI(IonMarkFunction(type));

    
    masm.PopRegsInMask(regs);

    masm.abiret();

    Linker linker(masm);
    return linker.newCode<NoGC>(cx, OTHER_CODE);
}

typedef bool (*HandleDebugTrapFn)(JSContext*, BaselineFrame*, uint8_t*, bool*);
static const VMFunction HandleDebugTrapInfo = FunctionInfo<HandleDebugTrapFn>(HandleDebugTrap);

JitCode*
JitRuntime::generateDebugTrapHandler(JSContext* cx)
{
    MacroAssembler masm(cx);

    Register scratch1 = r0;
    Register scratch2 = r1;

    
    masm.Sub(ARMRegister(scratch1, 64), BaselineFrameReg64, Operand(BaselineFrame::Size()));

    
    
    
    masm.movePtr(ImmPtr(nullptr), ICStubReg);
    EmitEnterStubFrame(masm, scratch2);

    JitCode* code = cx->runtime()->jitRuntime()->getVMWrapper(HandleDebugTrapInfo);
    if (!code)
        return nullptr;

    masm.asVIXL().Push(vixl::lr, ARMRegister(scratch1, 64));
    EmitCallVM(code, masm);

    EmitLeaveStubFrame(masm);

    
    
    
    Label forcedReturn;
    masm.branchTest32(Assembler::NonZero, ReturnReg, ReturnReg, &forcedReturn);
    masm.abiret();

    masm.bind(&forcedReturn);
    masm.loadValue(Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfReturnValue()),
                   JSReturnOperand);
    masm.Mov(masm.GetStackPointer64(), BaselineFrameReg64);

    masm.pop(BaselineFrameReg, lr);
    masm.syncStackPtr();
    masm.abiret();

    Linker linker(masm);
    JitCode* codeDbg = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(codeDbg, "DebugTrapHandler");
#endif

    return codeDbg;
}

JitCode*
JitRuntime::generateExceptionTailStub(JSContext* cx, void* handler)
{
    MacroAssembler masm(cx);

    masm.handleFailureWithHandlerTail(handler);

    Linker linker(masm);
    JitCode* code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "ExceptionTailStub");
#endif

    return code;
}

JitCode*
JitRuntime::generateBailoutTailStub(JSContext* cx)
{
    MacroAssembler masm(cx);

    masm.generateBailoutTail(r1, r2);

    Linker linker(masm);
    JitCode* code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "BailoutTailStub");
#endif

    return code;
}
JitCode*
JitRuntime::generateProfilerExitFrameTailStub(JSContext* cx)
{
    MacroAssembler masm;

    Register scratch1 = r8;
    Register scratch2 = r9;
    Register scratch3 = r10;
    Register scratch4 = r11;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
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
        masm.branchStackPtr(Assembler::Equal, scratch1, &checkOk);
        masm.assumeUnreachable("Mismatch between stored lastProfilingFrame and current stack pointer.");
        masm.bind(&checkOk);
    }
#endif

    
    masm.loadPtr(Address(masm.getStackPointer(), JitFrameLayout::offsetOfDescriptor()), scratch1);

    
    
    
    masm.and32(Imm32((1 << FRAMESIZE_SHIFT) - 1), scratch1, scratch2);
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
        

        
        
        masm.loadPtr(Address(masm.getStackPointer(), JitFrameLayout::offsetOfReturnAddress()),
                     scratch2);
        masm.storePtr(scratch2, lastProfilingCallSite);

        
        
        masm.addPtr(masm.getStackPointer(), scratch1, scratch2);
        masm.syncStackPtr();
        masm.addPtr(Imm32(JitFrameLayout::Size()), scratch2, scratch2);
        masm.storePtr(scratch2, lastProfilingFrame);
        masm.ret();
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    masm.bind(&handle_BaselineStub);
    {
        masm.addPtr(masm.getStackPointer(), scratch1, scratch3);
        masm.syncStackPtr();
        Address stubFrameReturnAddr(scratch3,
                                    JitFrameLayout::Size() +
                                    BaselineStubFrameLayout::offsetOfReturnAddress());
        masm.loadPtr(stubFrameReturnAddr, scratch2);
        masm.storePtr(scratch2, lastProfilingCallSite);

        Address stubFrameSavedFramePtr(scratch3,
                                       JitFrameLayout::Size() - (2 * sizeof(void*)));
        masm.loadPtr(stubFrameSavedFramePtr, scratch2);
        masm.addPtr(Imm32(sizeof(void*)), scratch2); 
        masm.storePtr(scratch2, lastProfilingFrame);
        masm.ret();
    }


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    masm.bind(&handle_Rectifier);
    {
        
        masm.addPtr(masm.getStackPointer(), scratch1, scratch2);
        masm.syncStackPtr();
        masm.add32(Imm32(JitFrameLayout::Size()), scratch2);
        masm.loadPtr(Address(scratch2, RectifierFrameLayout::offsetOfDescriptor()), scratch3);
        masm.rshiftPtr(Imm32(FRAMESIZE_SHIFT), scratch3, scratch1);
        masm.and32(Imm32((1 << FRAMETYPE_BITS) - 1), scratch3);

        
        
        

        
        Label handle_Rectifier_BaselineStub;
        masm.branch32(Assembler::NotEqual, scratch3, Imm32(JitFrame_IonJS),
                      &handle_Rectifier_BaselineStub);

        
        
        masm.loadPtr(Address(scratch2, RectifierFrameLayout::offsetOfReturnAddress()), scratch3);
        masm.storePtr(scratch3, lastProfilingCallSite);

        
        masm.addPtr(scratch2, scratch1, scratch3);
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
        masm.addPtr(scratch2, scratch1, scratch3);
        Address stubFrameReturnAddr(scratch3, RectifierFrameLayout::Size() +
                                              BaselineStubFrameLayout::offsetOfReturnAddress());
        masm.loadPtr(stubFrameReturnAddr, scratch2);
        masm.storePtr(scratch2, lastProfilingCallSite);

        Address stubFrameSavedFramePtr(scratch3,
                                       RectifierFrameLayout::Size() - (2 * sizeof(void*)));
        masm.loadPtr(stubFrameSavedFramePtr, scratch2);
        masm.addPtr(Imm32(sizeof(void*)), scratch2);
        masm.storePtr(scratch2, lastProfilingFrame);
        masm.ret();
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    masm.bind(&handle_IonAccessorIC);
    {
        
        masm.addPtr(masm.getStackPointer(), scratch1, scratch2);
        masm.syncStackPtr();
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

        
        
        masm.addPtr(scratch2, scratch3, scratch1);
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
    JitCode* code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "ProfilerExitFrameStub");
#endif

    return code;
}
