





#include "jit/Bailouts.h"
#include "jit/JitCompartment.h"
#include "jit/JitFrames.h"
#include "jit/Linker.h"
#ifdef JS_ION_PERF
# include "jit/PerfSpewer.h"
#endif
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
    masm.assertStackAlignment(ABIStackAlignment, -int32_t(sizeof(uintptr_t)) );

    const Register reg_code  = IntArgReg0;
    const Register reg_argc  = IntArgReg1;
    const Register reg_argv  = IntArgReg2;
    MOZ_ASSERT(OsrFrameReg == IntArgReg3);

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

    masm.vmovdqa(xmm6, Operand(rsp, 16 * 0));
    masm.vmovdqa(xmm7, Operand(rsp, 16 * 1));
    masm.vmovdqa(xmm8, Operand(rsp, 16 * 2));
    masm.vmovdqa(xmm9, Operand(rsp, 16 * 3));
    masm.vmovdqa(xmm10, Operand(rsp, 16 * 4));
    masm.vmovdqa(xmm11, Operand(rsp, 16 * 5));
    masm.vmovdqa(xmm12, Operand(rsp, 16 * 6));
    masm.vmovdqa(xmm13, Operand(rsp, 16 * 7));
    masm.vmovdqa(xmm14, Operand(rsp, 16 * 8));
    masm.vmovdqa(xmm15, Operand(rsp, 16 * 9));
#endif

    
    masm.push(result);

    
    masm.mov(rsp, r14);

    
    masm.mov(reg_argc, r13);
    masm.shll(Imm32(3), r13);   
    static_assert(sizeof(Value) == 1 << 3, "Constant is baked in assembly code");

    
    
    
    
    
    
    
    
    
    masm.mov(rsp, r12);
    masm.subq(r13, r12);
    static_assert(sizeof(JitFrameLayout) % JitStackAlignment == 0,
      "No need to consider the JitFrameLayout for aligning the stack");
    masm.andl(Imm32(JitStackAlignment - 1), r12);
    masm.subq(r12, rsp);

    



    
    masm.addq(reg_argv, r13); 

    
    {
        Label header, footer;
        masm.bind(&header);

        masm.cmpPtr(r13, reg_argv);
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
        
        masm.enterFakeExitFrame(ExitFrameLayout::BareToken());

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

        MOZ_ASSERT(reg_code != ReturnReg);

        Label error;
        masm.addPtr(Imm32(ExitFrameLayout::SizeWithFooter()), rsp);
        masm.addPtr(Imm32(BaselineFrame::Size()), framePtr);
        masm.branchIfFalseBool(ReturnReg, &error);

        
        
        {
            Label skipProfilingInstrumentation;
            Register realFramePtr = numStackValues;
            AbsoluteAddress addressOfEnabled(cx->runtime()->spsProfiler.addressOfEnabled());
            masm.branch32(Assembler::Equal, addressOfEnabled, Imm32(0),
                          &skipProfilingInstrumentation);
            masm.lea(Operand(framePtr, sizeof(void*)), realFramePtr);
            masm.profilerEnterFrame(realFramePtr, scratch);
            masm.bind(&skipProfilingInstrumentation);
        }

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

    
    
    masm.assertStackAlignment(JitStackAlignment, sizeof(uintptr_t));

    
    masm.call(reg_code);

    if (type == EnterJitBaseline) {
        
        masm.bind(returnLabel.src());
        masm.addCodeLabel(returnLabel);
    }

    
    masm.pop(r14);              
    masm.shrq(Imm32(FRAMESIZE_SHIFT), r14);
    masm.addq(r14, rsp);        

    


    masm.pop(r12); 
    masm.storeValue(JSReturnOperand, Operand(r12, 0));

    
#if defined(_WIN64)
    masm.vmovdqa(Operand(rsp, 16 * 0), xmm6);
    masm.vmovdqa(Operand(rsp, 16 * 1), xmm7);
    masm.vmovdqa(Operand(rsp, 16 * 2), xmm8);
    masm.vmovdqa(Operand(rsp, 16 * 3), xmm9);
    masm.vmovdqa(Operand(rsp, 16 * 4), xmm10);
    masm.vmovdqa(Operand(rsp, 16 * 5), xmm11);
    masm.vmovdqa(Operand(rsp, 16 * 6), xmm12);
    masm.vmovdqa(Operand(rsp, 16 * 7), xmm13);
    masm.vmovdqa(Operand(rsp, 16 * 8), xmm14);
    masm.vmovdqa(Operand(rsp, 16 * 9), xmm15);

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
    AutoJitContextAlloc ajca(cx);
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
JitRuntime::generateArgumentsRectifier(JSContext *cx, void **returnAddrOut)
{
    

    MacroAssembler masm(cx);
    
    
    

    
    
    MOZ_ASSERT(ArgumentsRectifierReg == r8);

    
    masm.addl(Imm32(1), r8);

    
    masm.loadPtr(Address(rsp, RectifierFrameLayout::offsetOfCalleeToken()), rax);
    masm.mov(rax, rcx);
    masm.andq(Imm32(uint32_t(CalleeTokenMask)), rcx);
    masm.movzwl(Operand(rcx, JSFunction::offsetOfNargs()), rcx);

    
    
    
    static_assert(sizeof(JitFrameLayout) % JitStackAlignment == 0,
      "No need to consider the JitFrameLayout for aligning the stack");
    static_assert(JitStackAlignment % sizeof(Value) == 0,
      "Ensure that we can pad the stack by pushing extra UndefinedValue");

    const uint32_t alignment = JitStackAlignment / sizeof(Value);
    MOZ_ASSERT(IsPowerOfTwo(alignment));
    masm.addl(Imm32(alignment - 1  + 1 ), rcx);
    masm.andl(Imm32(~(alignment - 1)), rcx);

    
    masm.subq(r8, rcx);

    
    
    
    
    
    
    

    
    masm.loadPtr(Address(rsp, RectifierFrameLayout::offsetOfNumActualArgs()), rdx);

    masm.moveValue(UndefinedValue(), r10);

    masm.movq(rsp, r9); 

    
    {
        Label undefLoopTop;
        masm.bind(&undefLoopTop);

        masm.push(r10);
        masm.subl(Imm32(1), rcx);
        masm.j(Assembler::NonZero, &undefLoopTop);
    }

    
    static_assert(sizeof(Value) == 8, "TimesEight is used to skip arguments");

    
    
    BaseIndex b = BaseIndex(r9, r8, TimesEight, sizeof(RectifierFrameLayout) - sizeof(Value));
    masm.lea(Operand(b), rcx);

    
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

    
    
    masm.andq(Imm32(uint32_t(CalleeTokenMask)), rax);
    masm.loadPtr(Address(rax, JSFunction::offsetOfNativeOrScript()), rax);
    masm.loadBaselineOrIonRaw(rax, rax, nullptr);
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

    
    
    
    
    
    
    
    static const uint32_t BailoutDataSize = sizeof(RegisterDump);
    masm.addq(Imm32(BailoutDataSize), rsp);
    masm.pop(rcx);
    masm.lea(Operand(rsp, rcx, TimesOne, sizeof(void *)), rsp);

    
    JitCode *bailoutTail = cx->runtime()->jitRuntime()->getBailoutTail();
    masm.jmp(bailoutTail);
}

JitCode *
JitRuntime::generateBailoutTable(JSContext *cx, uint32_t frameClass)
{
    MOZ_CRASH("x64 does not use bailout tables");
}

JitCode *
JitRuntime::generateBailoutHandler(JSContext *cx)
{
    MacroAssembler masm;
    GenerateBailoutThunk(cx, masm, NO_FRAME_SIZE_CLASS_ID);

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
    MOZ_ASSERT(functionWrappers_);
    MOZ_ASSERT(functionWrappers_->initialized());
    VMWrapperMap::AddPtr p = functionWrappers_->lookupForAdd(&f);
    if (p)
        return p->value();

    
    MacroAssembler masm;

    
    
    GeneralRegisterSet regs = GeneralRegisterSet(Register::Codes::WrapperMask);

    
    JS_STATIC_ASSERT((Register::Codes::VolatileMask & ~Register::Codes::WrapperMask) == 0);

    
    Register cxreg = IntArgReg0;
    regs.take(cxreg);

    
    
    
    
    
    
    
    masm.enterExitFrame(&f);
    masm.loadJSContext(cxreg);

    
    Register argsBase = InvalidReg;
    if (f.explicitArgs) {
        argsBase = r10;
        regs.take(argsBase);
        masm.lea(Operand(rsp, ExitFrameLayout::SizeWithFooter()), argsBase);
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
        masm.branchTestPtr(Assembler::Zero, rax, rax, masm.failureLabel());
        break;
      case Type_Bool:
        masm.testb(rax, rax);
        masm.j(Assembler::Zero, masm.failureLabel());
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
        MOZ_ASSERT(cx->runtime()->jitSupportsFloatingPoint);
        masm.loadDouble(Address(esp, 0), ReturnDoubleReg);
        masm.freeStack(sizeof(double));
        break;

      case Type_Pointer:
        masm.loadPtr(Address(esp, 0), ReturnReg);
        masm.freeStack(sizeof(uintptr_t));
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

    MOZ_ASSERT(PreBarrierReg == rdx);
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

    
    
    {
        Label skipProfilingInstrumentation;
        AbsoluteAddress addressOfEnabled(cx->runtime()->spsProfiler.addressOfEnabled());
        masm.branch32(Assembler::Equal, addressOfEnabled, Imm32(0), &skipProfilingInstrumentation);
        masm.profilerExitFrame();
        masm.bind(&skipProfilingInstrumentation);
    }

    masm.ret();

    Linker linker(masm);
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

JitCode *
JitRuntime::generateProfilerExitFrameTailStub(JSContext *cx)
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
        masm.branchPtr(Assembler::Equal, StackPointer, scratch1, &checkOk);
        masm.assumeUnreachable(
            "Mismatch between stored lastProfilingFrame and current stack pointer.");
        masm.bind(&checkOk);
    }
#endif

    
    masm.loadPtr(Address(StackPointer, JitFrameLayout::offsetOfDescriptor()), scratch1);

    
    
    
    masm.movePtr(scratch1, scratch2);
    masm.rshiftPtr(Imm32(FRAMESIZE_SHIFT), scratch1);
    masm.and32(Imm32((1 << FRAMETYPE_BITS) - 1), scratch2);

    
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

        
        
        masm.lea(Operand(StackPointer, scratch1, TimesOne, JitFrameLayout::Size()), scratch2);
        masm.storePtr(scratch2, lastProfilingFrame);
        masm.ret();
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    masm.bind(&handle_BaselineStub);
    {
        BaseIndex stubFrameReturnAddr(StackPointer, scratch1, TimesOne,
                                      JitFrameLayout::Size() +
                                      BaselineStubFrameLayout::offsetOfReturnAddress());
        masm.loadPtr(stubFrameReturnAddr, scratch2);
        masm.storePtr(scratch2, lastProfilingCallSite);

        BaseIndex stubFrameSavedFramePtr(StackPointer, scratch1, TimesOne,
                                         JitFrameLayout::Size() - (2 * sizeof(void *)));
        masm.loadPtr(stubFrameSavedFramePtr, scratch2);
        masm.addPtr(Imm32(sizeof(void *)), scratch2); 
        masm.storePtr(scratch2, lastProfilingFrame);
        masm.ret();
    }


    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    masm.bind(&handle_Rectifier);
    {
        
        masm.lea(Operand(StackPointer, scratch1, TimesOne, JitFrameLayout::Size()), scratch2);
        masm.loadPtr(Address(scratch2, RectifierFrameLayout::offsetOfDescriptor()), scratch3);
        masm.movePtr(scratch3, scratch1);
        masm.and32(Imm32((1 << FRAMETYPE_BITS) - 1), scratch3);
        masm.rshiftPtr(Imm32(FRAMESIZE_SHIFT), scratch1);

        
        
        

        
        Label handle_Rectifier_BaselineStub;
        masm.branch32(Assembler::NotEqual, scratch3, Imm32(JitFrame_IonJS),
                      &handle_Rectifier_BaselineStub);

        
        
        masm.loadPtr(Address(scratch2, RectifierFrameLayout::offsetOfReturnAddress()), scratch3);
        masm.storePtr(scratch3, lastProfilingCallSite);

        
        masm.lea(Operand(scratch2, scratch1, TimesOne, RectifierFrameLayout::Size()), scratch3);
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
        BaseIndex stubFrameReturnAddr(scratch2, scratch1, TimesOne,
                                         RectifierFrameLayout::Size() +
                                         BaselineStubFrameLayout::offsetOfReturnAddress());
        masm.loadPtr(stubFrameReturnAddr, scratch3);
        masm.storePtr(scratch3, lastProfilingCallSite);

        BaseIndex stubFrameSavedFramePtr(scratch2, scratch1, TimesOne,
                                         RectifierFrameLayout::Size() - (2 * sizeof(void *)));
        masm.loadPtr(stubFrameSavedFramePtr, scratch3);
        masm.addPtr(Imm32(sizeof(void *)), scratch3);
        masm.storePtr(scratch3, lastProfilingFrame);
        masm.ret();
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    masm.bind(&handle_IonAccessorIC);
    {
        
        masm.lea(Operand(StackPointer, scratch1, TimesOne, JitFrameLayout::Size()), scratch2);

        
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

        
        
        masm.lea(Operand(scratch2, scratch3, TimesOne, IonAccessorICFrameLayout::Size()), scratch1);
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
    JitCode *code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "ProfilerExitFrameStub");
#endif

    return code;
}
