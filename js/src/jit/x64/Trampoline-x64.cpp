





#include "jit/Bailouts.h"
#include "jit/IonFrames.h"
#include "jit/IonLinker.h"
#include "jit/JitCompartment.h"
#ifdef JS_ION_PERF
# include "jit/PerfSpewer.h"
#endif
#include "jit/ParallelFunctions.h"
#include "jit/VMFunctions.h"
#include "jit/x64/BaselineHelpers-x64.h"

using namespace js;
using namespace js::jit;



static const RegisterSet AllRegs =
  RegisterSet(GeneralRegisterSet(Registers::AllMask),
              FloatRegisterSet(FloatRegisters::AllMask));







JitCode *
JitRuntime::generateEnterJIT(JSContext *cx, EnterJitType type)
{
    MacroAssembler masm(cx);

    const Register reg_code  = IntArgReg0;
    const Register reg_argc  = IntArgReg1;
    const Register reg_argv  = IntArgReg2;
    JS_ASSERT(OsrFrameReg == IntArgReg3);

#if defined(_WIN64)
    const Operand token  = Operand(rbp, 16 + ShadowStackSpace);
    const Operand scopeChain = Operand(rbp, 24 + ShadowStackSpace);
    const Operand numStackValuesAddr = Operand(rbp, 32 + ShadowStackSpace);
    const Operand result = Operand(rbp, 40 + ShadowStackSpace);
#else
    const Register token = IntArgReg4;
    const Register scopeChain = IntArgReg5;
    const Operand numStackValuesAddr = Operand(rbp, 16 + ShadowStackSpace);
    const Operand result = Operand(rbp, 24 + ShadowStackSpace);
#endif

    
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

    
    masm.subq(Imm32(16 * 10 + 8), rsp);

    masm.movdqa(xmm6, Operand(rsp, 16 * 0));
    masm.movdqa(xmm7, Operand(rsp, 16 * 1));
    masm.movdqa(xmm8, Operand(rsp, 16 * 2));
    masm.movdqa(xmm9, Operand(rsp, 16 * 3));
    masm.movdqa(xmm10, Operand(rsp, 16 * 4));
    masm.movdqa(xmm11, Operand(rsp, 16 * 5));
    masm.movdqa(xmm12, Operand(rsp, 16 * 6));
    masm.movdqa(xmm13, Operand(rsp, 16 * 7));
    masm.movdqa(xmm14, Operand(rsp, 16 * 8));
    masm.movdqa(xmm15, Operand(rsp, 16 * 9));
#endif

    
    masm.spsMarkJit(&cx->runtime()->spsProfiler, rbp, rbx);

    
    masm.push(result);

    
    masm.mov(rsp, r14);

    
    masm.mov(reg_argc, r13);
    masm.shll(Imm32(3), r13);

    
    
    
    
    masm.mov(rsp, r12);
    masm.subq(r13, r12);
    masm.subq(Imm32(8), r12);
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

    
    
    
    masm.movq(result, reg_argc);
    masm.unboxInt32(Operand(reg_argc, 0), reg_argc);
    masm.push(reg_argc);

    
    masm.push(token);

    


    masm.subq(rsp, r14);

    
    masm.makeFrameDescriptor(r14, JitFrame_Entry);
    masm.push(r14);

    CodeLabel returnLabel;
    if (type == EnterJitBaseline) {
        
        GeneralRegisterSet regs(GeneralRegisterSet::All());
        regs.takeUnchecked(OsrFrameReg);
        regs.take(rbp);
        regs.take(reg_code);

        
        
        
        regs.takeUnchecked(JSReturnOperand);
        Register scratch = regs.takeAny();

        Label notOsr;
        masm.branchTestPtr(Assembler::Zero, OsrFrameReg, OsrFrameReg, &notOsr);

        Register numStackValues = regs.takeAny();
        masm.movq(numStackValuesAddr, numStackValues);

        
        masm.mov(returnLabel.dest(), scratch);
        masm.push(scratch);
        masm.push(rbp);

        
        Register framePtr = rbp;
        masm.subPtr(Imm32(BaselineFrame::Size()), rsp);
        masm.mov(rsp, framePtr);

#ifdef XP_WIN
        
        masm.mov(numStackValues, scratch);
        masm.lshiftPtr(Imm32(3), scratch);
        masm.subPtr(scratch, framePtr);
        {
            masm.movePtr(rsp, scratch);
            masm.subPtr(Imm32(WINDOWS_BIG_FRAME_TOUCH_INCREMENT), scratch);

            Label touchFrameLoop;
            Label touchFrameLoopEnd;
            masm.bind(&touchFrameLoop);
            masm.branchPtr(Assembler::Below, scratch, framePtr, &touchFrameLoopEnd);
            masm.store32(Imm32(0), Address(scratch, 0));
            masm.subPtr(Imm32(WINDOWS_BIG_FRAME_TOUCH_INCREMENT), scratch);
            masm.jump(&touchFrameLoop);
            masm.bind(&touchFrameLoopEnd);
        }
        masm.mov(rsp, framePtr);
#endif

        
        Register valuesSize = regs.takeAny();
        masm.mov(numStackValues, valuesSize);
        masm.shll(Imm32(3), valuesSize);
        masm.subPtr(valuesSize, rsp);

        
        masm.addPtr(Imm32(BaselineFrame::Size() + BaselineFrame::FramePointerOffset), valuesSize);
        masm.makeFrameDescriptor(valuesSize, JitFrame_BaselineJS);
        masm.push(valuesSize);
        masm.push(Imm32(0)); 
        
        masm.enterFakeExitFrame(IonExitFrameLayout::BareToken());

        regs.add(valuesSize);

        masm.push(framePtr);
        masm.push(reg_code);

        masm.setupUnalignedABICall(3, scratch);
        masm.passABIArg(framePtr); 
        masm.passABIArg(OsrFrameReg); 
        masm.passABIArg(numStackValues);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, jit::InitBaselineFrameForOsr));

        masm.pop(reg_code);
        masm.pop(framePtr);

        JS_ASSERT(reg_code != ReturnReg);

        Label error;
        masm.addPtr(Imm32(IonExitFrameLayout::SizeWithFooter()), rsp);
        masm.addPtr(Imm32(BaselineFrame::Size()), framePtr);
        masm.branchIfFalseBool(ReturnReg, &error);

        masm.jump(reg_code);

        
        
        masm.bind(&error);
        masm.mov(framePtr, rsp);
        masm.addPtr(Imm32(2 * sizeof(uintptr_t)), rsp);
        masm.moveValue(MagicValue(JS_ION_ERROR), JSReturnOperand);
        masm.mov(returnLabel.dest(), scratch);
        masm.jump(scratch);

        masm.bind(&notOsr);
        masm.movq(scopeChain, R1.scratchReg());
    }

    
    masm.call(reg_code);

    if (type == EnterJitBaseline) {
        
        masm.bind(returnLabel.src());
        if (!masm.addCodeLabel(returnLabel))
            return nullptr;
    }

    
    masm.pop(r14);              
    masm.shrq(Imm32(FRAMESIZE_SHIFT), r14);
    masm.addq(r14, rsp);        

    


    masm.pop(r12); 
    masm.storeValue(JSReturnOperand, Operand(r12, 0));

    
    masm.spsUnmarkJit(&cx->runtime()->spsProfiler, rbx);

    
#if defined(_WIN64)
    masm.movdqa(Operand(rsp, 16 * 0), xmm6);
    masm.movdqa(Operand(rsp, 16 * 1), xmm7);
    masm.movdqa(Operand(rsp, 16 * 2), xmm8);
    masm.movdqa(Operand(rsp, 16 * 3), xmm9);
    masm.movdqa(Operand(rsp, 16 * 4), xmm10);
    masm.movdqa(Operand(rsp, 16 * 5), xmm11);
    masm.movdqa(Operand(rsp, 16 * 6), xmm12);
    masm.movdqa(Operand(rsp, 16 * 7), xmm13);
    masm.movdqa(Operand(rsp, 16 * 8), xmm14);
    masm.movdqa(Operand(rsp, 16 * 9), xmm15);

    masm.addq(Imm32(16 * 10 + 8), rsp);

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
    JitCode *code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "EnterJIT");
#endif

    return code;
}

JitCode *
JitRuntime::generateInvalidator(JSContext *cx)
{
    AutoIonContextAlloc aica(cx);
    MacroAssembler masm(cx);

    

    masm.addq(Imm32(sizeof(uintptr_t)), rsp);

    
    masm.PushRegsInMask(AllRegs);

    masm.movq(rsp, rax); 

    
    masm.reserveStack(sizeof(size_t));
    masm.movq(rsp, rbx);

    
    masm.reserveStack(sizeof(void *));
    masm.movq(rsp, r9);

    masm.setupUnalignedABICall(3, rdx);
    masm.passABIArg(rax);
    masm.passABIArg(rbx);
    masm.passABIArg(r9);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, InvalidationBailout));

    masm.pop(r9); 
    masm.pop(rbx); 

    
    masm.lea(Operand(rsp, rbx, TimesOne, sizeof(InvalidationBailoutStack)), rsp);

    
    JitCode *bailoutTail = cx->runtime()->jitRuntime()->getBailoutTail();
    masm.jmp(bailoutTail);

    Linker linker(masm);
    JitCode *code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "Invalidator");
#endif

    return code;
}

JitCode *
JitRuntime::generateArgumentsRectifier(JSContext *cx, ExecutionMode mode, void **returnAddrOut)
{
    

    MacroAssembler masm(cx);

    
    
    JS_ASSERT(ArgumentsRectifierReg == r8);

    
    masm.loadPtr(Address(rsp, IonRectifierFrameLayout::offsetOfCalleeToken()), rax);
    masm.movzwl(Operand(rax, JSFunction::offsetOfNargs()), rcx);
    masm.subq(r8, rcx);

    
    masm.loadPtr(Address(rsp, IonRectifierFrameLayout::offsetOfNumActualArgs()), rdx);

    masm.moveValue(UndefinedValue(), r10);

    masm.movq(rsp, r9); 

    
    {
        Label undefLoopTop;
        masm.bind(&undefLoopTop);

        masm.push(r10);
        masm.subl(Imm32(1), rcx);
        masm.j(Assembler::NonZero, &undefLoopTop);
    }

    
    BaseIndex b = BaseIndex(r9, r8, TimesEight, sizeof(IonRectifierFrameLayout));
    masm.lea(Operand(b), rcx);

    
    masm.addl(Imm32(1), r8);
    {
        Label copyLoopTop;

        masm.bind(&copyLoopTop);
        masm.push(Operand(rcx, 0x0));
        masm.subq(Imm32(sizeof(Value)), rcx);
        masm.subl(Imm32(1), r8);
        masm.j(Assembler::NonZero, &copyLoopTop);
    }

    
    masm.subq(rsp, r9);
    masm.makeFrameDescriptor(r9, JitFrame_Rectifier);

    
    masm.push(rdx); 
    masm.push(rax); 
    masm.push(r9); 

    
    
    masm.loadPtr(Address(rax, JSFunction::offsetOfNativeOrScript()), rax);
    masm.loadBaselineOrIonRaw(rax, rax, mode, nullptr);
    masm.call(rax);
    uint32_t returnOffset = masm.currentOffset();

    
    masm.pop(r9);             
    masm.shrq(Imm32(FRAMESIZE_SHIFT), r9);
    masm.pop(r11);            
    masm.pop(r11);            
    masm.addq(r9, rsp);       

    masm.ret();

    Linker linker(masm);
    JitCode *code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "ArgumentsRectifier");
#endif

    CodeOffsetLabel returnLabel(returnOffset);
    returnLabel.fixup(&masm);
    if (returnAddrOut)
        *returnAddrOut = (void *) (code->raw() + returnLabel.offset());
    return code;
}

static void
PushBailoutFrame(MacroAssembler &masm, Register spArg)
{
    
    masm.PushRegsInMask(AllRegs);

    
    masm.movq(rsp, spArg);
}

static void
GenerateBailoutThunk(JSContext *cx, MacroAssembler &masm, uint32_t frameClass)
{
    PushBailoutFrame(masm, r8);

    
    masm.reserveStack(sizeof(void *));
    masm.movq(rsp, r9);

    
    masm.setupUnalignedABICall(2, rax);
    masm.passABIArg(r8);
    masm.passABIArg(r9);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, Bailout));

    masm.pop(r9); 

    
    
    
    
    
    
    
    static const uint32_t BailoutDataSize = sizeof(void *) * Registers::Total +
                                            sizeof(double) * FloatRegisters::Total;
    masm.addq(Imm32(BailoutDataSize), rsp);
    masm.pop(rcx);
    masm.lea(Operand(rsp, rcx, TimesOne, sizeof(void *)), rsp);

    
    JitCode *bailoutTail = cx->runtime()->jitRuntime()->getBailoutTail();
    masm.jmp(bailoutTail);
}

static void
GenerateParallelBailoutThunk(MacroAssembler &masm)
{
    
    
    

    PushBailoutFrame(masm, r8);

    
    
    masm.reserveStack(sizeof(uint8_t *));
    masm.movePtr(rsp, r9);

    masm.setupUnalignedABICall(2, rax);
    masm.passABIArg(r8);
    masm.passABIArg(r9);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, BailoutPar));

    
    masm.moveValue(MagicValue(JS_ION_ERROR), JSReturnOperand);
    masm.loadPtr(Address(rsp, 0), rsp);
    masm.ret();
}

JitCode *
JitRuntime::generateBailoutTable(JSContext *cx, uint32_t frameClass)
{
    MOZ_CRASH("x64 does not use bailout tables");
}

JitCode *
JitRuntime::generateBailoutHandler(JSContext *cx, ExecutionMode mode)
{
    MacroAssembler masm;

    switch (mode) {
      case SequentialExecution:
        GenerateBailoutThunk(cx, masm, NO_FRAME_SIZE_CLASS_ID);
        break;
      case ParallelExecution:
        GenerateParallelBailoutThunk(masm);
        break;
      default:
        MOZ_CRASH("No such execution mode");
    }

    Linker linker(masm);
    JitCode *code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "BailoutHandler");
#endif

    return code;
}

JitCode *
JitRuntime::generateVMWrapper(JSContext *cx, const VMFunction &f)
{
    JS_ASSERT(!StackKeptAligned);
    JS_ASSERT(functionWrappers_);
    JS_ASSERT(functionWrappers_->initialized());
    VMWrapperMap::AddPtr p = functionWrappers_->lookupForAdd(&f);
    if (p)
        return p->value();

    
    MacroAssembler masm;

    
    
    GeneralRegisterSet regs = GeneralRegisterSet(Register::Codes::WrapperMask);

    
    JS_STATIC_ASSERT((Register::Codes::VolatileMask & ~Register::Codes::WrapperMask) == 0);

    
    Register cxreg = IntArgReg0;
    regs.take(cxreg);

    
    
    
    
    
    
    
    masm.enterExitFrameAndLoadContext(&f, cxreg, regs.getAny(), f.executionMode);

    
    Register argsBase = InvalidReg;
    if (f.explicitArgs) {
        argsBase = r10;
        regs.take(argsBase);
        masm.lea(Operand(rsp,IonExitFrameLayout::SizeWithFooter()), argsBase);
    }

    
    Register outReg = InvalidReg;
    switch (f.outParam) {
      case Type_Value:
        outReg = regs.takeAny();
        masm.reserveStack(sizeof(Value));
        masm.movq(esp, outReg);
        break;

      case Type_Handle:
        outReg = regs.takeAny();
        masm.PushEmptyRooted(f.outParamRootType);
        masm.movq(esp, outReg);
        break;

      case Type_Int32:
      case Type_Bool:
        outReg = regs.takeAny();
        masm.reserveStack(sizeof(int32_t));
        masm.movq(esp, outReg);
        break;

      case Type_Double:
        outReg = regs.takeAny();
        masm.reserveStack(sizeof(double));
        masm.movq(esp, outReg);
        break;

      case Type_Pointer:
        outReg = regs.takeAny();
        masm.reserveStack(sizeof(uintptr_t));
        masm.movq(esp, outReg);
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
            if (f.argPassedInFloatReg(explicitArg))
                masm.passABIArg(MoveOperand(argsBase, argDisp), MoveOp::DOUBLE);
            else
                masm.passABIArg(MoveOperand(argsBase, argDisp), MoveOp::GENERAL);
            argDisp += sizeof(void *);
            break;
          case VMFunction::WordByRef:
            masm.passABIArg(MoveOperand(argsBase, argDisp, MoveOperand::EFFECTIVE_ADDRESS),
                            MoveOp::GENERAL);
            argDisp += sizeof(void *);
            break;
          case VMFunction::DoubleByValue:
          case VMFunction::DoubleByRef:
            MOZ_CRASH("NYI: x64 callVM should not be used with 128bits values.");
        }
    }

    
    if (outReg != InvalidReg)
        masm.passABIArg(outReg);

    masm.callWithABI(f.wrapped);

    
    switch (f.failType()) {
      case Type_Object:
        masm.branchTestPtr(Assembler::Zero, rax, rax, masm.failureLabel(f.executionMode));
        break;
      case Type_Bool:
        masm.testb(rax, rax);
        masm.j(Assembler::Zero, masm.failureLabel(f.executionMode));
        break;
      default:
        MOZ_CRASH("unknown failure kind");
    }

    
    switch (f.outParam) {
      case Type_Handle:
        masm.popRooted(f.outParamRootType, ReturnReg, JSReturnOperand);
        break;

      case Type_Value:
        masm.loadValue(Address(esp, 0), JSReturnOperand);
        masm.freeStack(sizeof(Value));
        break;

      case Type_Int32:
        masm.load32(Address(esp, 0), ReturnReg);
        masm.freeStack(sizeof(int32_t));
        break;

      case Type_Bool:
        masm.load8ZeroExtend(Address(esp, 0), ReturnReg);
        masm.freeStack(sizeof(int32_t));
        break;

      case Type_Double:
        JS_ASSERT(cx->runtime()->jitSupportsFloatingPoint);
        masm.loadDouble(Address(esp, 0), ReturnDoubleReg);
        masm.freeStack(sizeof(double));
        break;

      case Type_Pointer:
        masm.loadPtr(Address(esp, 0), ReturnReg);
        masm.freeStack(sizeof(uintptr_t));
        break;

      default:
        JS_ASSERT(f.outParam == Type_Void);
        break;
    }
    masm.leaveExitFrame();
    masm.retn(Imm32(sizeof(IonExitFrameLayout) +
                    f.explicitStackSlots() * sizeof(void *) +
                    f.extraValuesToPop * sizeof(Value)));

    Linker linker(masm);
    JitCode *wrapper = linker.newCode<NoGC>(cx, OTHER_CODE);
    if (!wrapper)
        return nullptr;

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(wrapper, "VMWrapper");
#endif

    
    
    if (!functionWrappers_->relookupOrAdd(p, &f, wrapper))
        return nullptr;

    return wrapper;
}

JitCode *
JitRuntime::generatePreBarrier(JSContext *cx, MIRType type)
{
    MacroAssembler masm;

    RegisterSet regs = RegisterSet(GeneralRegisterSet(Registers::VolatileMask),
                                   FloatRegisterSet(FloatRegisters::VolatileMask));
    masm.PushRegsInMask(regs);

    JS_ASSERT(PreBarrierReg == rdx);
    masm.mov(ImmPtr(cx->runtime()), rcx);

    masm.setupUnalignedABICall(2, rax);
    masm.passABIArg(rcx);
    masm.passABIArg(rdx);
    masm.callWithABI(IonMarkFunction(type));

    masm.PopRegsInMask(regs);
    masm.ret();

    Linker linker(masm);
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

    Register scratch1 = rax;
    Register scratch2 = rcx;
    Register scratch3 = rdx;

    
    masm.loadPtr(Address(rsp, 0), scratch1);

    
    masm.mov(rbp, scratch2);
    masm.subPtr(Imm32(BaselineFrame::Size()), scratch2);

    
    
    
    masm.movePtr(ImmPtr(nullptr), BaselineStubReg);
    EmitEnterStubFrame(masm, scratch3);

    JitCode *code = cx->runtime()->jitRuntime()->getVMWrapper(HandleDebugTrapInfo);
    if (!code)
        return nullptr;

    masm.push(scratch1);
    masm.push(scratch2);
    EmitCallVM(code, masm);

    EmitLeaveStubFrame(masm);

    
    
    
    Label forcedReturn;
    masm.branchTest32(Assembler::NonZero, ReturnReg, ReturnReg, &forcedReturn);
    masm.ret();

    masm.bind(&forcedReturn);
    masm.loadValue(Address(ebp, BaselineFrame::reverseOffsetOfReturnValue()),
                   JSReturnOperand);
    masm.mov(rbp, rsp);
    masm.pop(rbp);
    masm.ret();

    Linker linker(masm);
    JitCode *codeDbg = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(codeDbg, "DebugTrapHandler");
#endif

    return codeDbg;
}

JitCode *
JitRuntime::generateExceptionTailStub(JSContext *cx)
{
    MacroAssembler masm;

    masm.handleFailureWithHandlerTail();

    Linker linker(masm);
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

    masm.generateBailoutTail(rdx, r9);

    Linker linker(masm);
    JitCode *code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "BailoutTailStub");
#endif

    return code;
}
