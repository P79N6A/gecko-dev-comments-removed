





#include "jscompartment.h"

#include "jit/Bailouts.h"
#include "jit/BaselineJIT.h"
#include "jit/JitCompartment.h"
#include "jit/JitFrames.h"
#include "jit/JitSpewer.h"
#include "jit/Linker.h"
#ifdef JS_ION_PERF
# include "jit/PerfSpewer.h"
#endif
#include "jit/VMFunctions.h"
#include "jit/x86/BaselineHelpers-x86.h"

#include "jsscriptinlines.h"

using namespace js;
using namespace js::jit;



static const RegisterSet AllRegs =
  RegisterSet(GeneralRegisterSet(Registers::AllMask),
              FloatRegisterSet(FloatRegisters::AllMask));

enum EnterJitEbpArgumentOffset {
    ARG_JITCODE         = 2 * sizeof(void *),
    ARG_ARGC            = 3 * sizeof(void *),
    ARG_ARGV            = 4 * sizeof(void *),
    ARG_STACKFRAME      = 5 * sizeof(void *),
    ARG_CALLEETOKEN     = 6 * sizeof(void *),
    ARG_SCOPECHAIN      = 7 * sizeof(void *),
    ARG_STACKVALUES     = 8 * sizeof(void *),
    ARG_RESULT          = 9 * sizeof(void *)
};





JitCode *
JitRuntime::generateEnterJIT(JSContext *cx, EnterJitType type)
{
    MacroAssembler masm(cx);
    masm.assertStackAlignment(ABIStackAlignment, -int32_t(sizeof(uintptr_t)) );

    
    masm.push(ebp);
    masm.movl(esp, ebp);

    
    
    
    masm.push(ebx);
    masm.push(esi);
    masm.push(edi);

    
    
    masm.movl(esp, esi);

    
    masm.loadPtr(Address(ebp, ARG_ARGC), eax);
    masm.shll(Imm32(3), eax);

    
    
    
    
    
    
    
    
    
    masm.movl(esp, ecx);
    masm.subl(eax, ecx);
    static_assert(sizeof(JitFrameLayout) % JitStackAlignment == 0,
      "No need to consider the JitFrameLayout for aligning the stack");

    
    masm.andl(Imm32(JitStackAlignment - 1), ecx);
    masm.subl(ecx, esp);

    



    
    masm.loadPtr(Address(ebp, ARG_ARGV), ebx);

    
    masm.addl(ebx, eax);

    
    {
        Label header, footer;
        masm.bind(&header);

        masm.cmp32(eax, ebx);
        masm.j(Assembler::BelowOrEqual, &footer);

        
        masm.subl(Imm32(8), eax);

        
        masm.push(Operand(eax, 4));
        masm.push(Operand(eax, 0));

        masm.jmp(&header);
        masm.bind(&footer);
    }


    
    
    
    masm.mov(Operand(ebp, ARG_RESULT), eax);
    masm.unboxInt32(Address(eax, 0x0), eax);
    masm.push(eax);

    
    masm.push(Operand(ebp, ARG_CALLEETOKEN));

    
    
    masm.loadPtr(Address(ebp, ARG_STACKFRAME), OsrFrameReg);

    


    
    masm.subl(esp, esi);
    masm.makeFrameDescriptor(esi, JitFrame_Entry);
    masm.push(esi);

    CodeLabel returnLabel;
    if (type == EnterJitBaseline) {
        
        GeneralRegisterSet regs(GeneralRegisterSet::All());
        regs.take(JSReturnOperand);
        regs.takeUnchecked(OsrFrameReg);
        regs.take(ebp);
        regs.take(ReturnReg);

        Register scratch = regs.takeAny();

        Label notOsr;
        masm.branchTestPtr(Assembler::Zero, OsrFrameReg, OsrFrameReg, &notOsr);

        Register numStackValues = regs.takeAny();
        masm.loadPtr(Address(ebp, ARG_STACKVALUES), numStackValues);

        Register jitcode = regs.takeAny();
        masm.loadPtr(Address(ebp, ARG_JITCODE), jitcode);

        
        masm.mov(returnLabel.dest(), scratch);
        masm.push(scratch);

        
        masm.push(ebp);

        
        Register framePtr = ebp;
        masm.subPtr(Imm32(BaselineFrame::Size()), esp);
        masm.mov(esp, framePtr);

#ifdef XP_WIN
        
        masm.mov(numStackValues, scratch);
        masm.shll(Imm32(3), scratch);
        masm.subPtr(scratch, framePtr);
        {
            masm.movePtr(esp, scratch);
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
        masm.mov(esp, framePtr);
#endif

        
        masm.mov(numStackValues, scratch);
        masm.shll(Imm32(3), scratch);
        masm.subPtr(scratch, esp);

        
        masm.addPtr(Imm32(BaselineFrame::Size() + BaselineFrame::FramePointerOffset), scratch);
        masm.makeFrameDescriptor(scratch, JitFrame_BaselineJS);
        masm.push(scratch); 
        masm.push(Imm32(0));
        
        masm.enterFakeExitFrame(ExitFrameLayout::BareToken());

        masm.push(framePtr);
        masm.push(jitcode);

        masm.setupUnalignedABICall(3, scratch);
        masm.passABIArg(framePtr); 
        masm.passABIArg(OsrFrameReg); 
        masm.passABIArg(numStackValues);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, jit::InitBaselineFrameForOsr));

        masm.pop(jitcode);
        masm.pop(framePtr);

        MOZ_ASSERT(jitcode != ReturnReg);

        Label error;
        masm.addPtr(Imm32(ExitFrameLayout::SizeWithFooter()), esp);
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

        masm.jump(jitcode);

        
        
        masm.bind(&error);
        masm.mov(framePtr, esp);
        masm.addPtr(Imm32(2 * sizeof(uintptr_t)), esp);
        masm.moveValue(MagicValue(JS_ION_ERROR), JSReturnOperand);
        masm.mov(returnLabel.dest(), scratch);
        masm.jump(scratch);

        masm.bind(&notOsr);
        masm.loadPtr(Address(ebp, ARG_SCOPECHAIN), R1.scratchReg());
    }

    
    
    masm.assertStackAlignment(JitStackAlignment, sizeof(uintptr_t));

    



    masm.call(Operand(ebp, ARG_JITCODE));

    if (type == EnterJitBaseline) {
        
        masm.bind(returnLabel.src());
        masm.addCodeLabel(returnLabel);
    }

    
    
    masm.pop(eax);
    masm.shrl(Imm32(FRAMESIZE_SHIFT), eax); 
    masm.addl(eax, esp);

    
    
    
    
    
    
    
    
    masm.loadPtr(Address(esp, ARG_RESULT + 3 * sizeof(void *)), eax);
    masm.storeValue(JSReturnOperand, Operand(eax, 0));

    



    
    masm.pop(edi);
    masm.pop(esi);
    masm.pop(ebx);

    
    masm.pop(ebp);
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

    
    
    
    
    
    
    
    
    

    masm.addl(Imm32(sizeof(uintptr_t)), esp);

    
    masm.PushRegsInMask(AllRegs);

    masm.movl(esp, eax); 

    
    masm.reserveStack(sizeof(size_t));
    masm.movl(esp, ebx);

    
    masm.reserveStack(sizeof(void *));
    masm.movl(esp, ecx);

    masm.setupUnalignedABICall(3, edx);
    masm.passABIArg(eax);
    masm.passABIArg(ebx);
    masm.passABIArg(ecx);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, InvalidationBailout));

    masm.pop(ecx); 
    masm.pop(ebx); 

    
    masm.lea(Operand(esp, ebx, TimesOne, sizeof(InvalidationBailoutStack)), esp);

    
    JitCode *bailoutTail = cx->runtime()->jitRuntime()->getBailoutTail();
    masm.jmp(bailoutTail);

    Linker linker(masm);
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
    
    
    

    
    
    MOZ_ASSERT(ArgumentsRectifierReg == esi);

    
    masm.loadPtr(Address(esp, RectifierFrameLayout::offsetOfCalleeToken()), eax);
    masm.mov(eax, ecx);
    masm.andl(Imm32(CalleeTokenMask), ecx);
    masm.movzwl(Operand(ecx, JSFunction::offsetOfNargs()), ecx);

    
    
    
    
    static_assert(sizeof(JitFrameLayout) % JitStackAlignment == 0,
      "No need to consider the JitFrameLayout for aligning the stack");
    static_assert((sizeof(Value) + 2 * sizeof(void *)) % JitStackAlignment == 0,
      "No need to consider |this| and the frame pointer and its padding for aligning the stack");
    static_assert(JitStackAlignment % sizeof(Value) == 0,
      "Ensure that we can pad the stack by pushing extra UndefinedValue");

    const uint32_t alignment = JitStackAlignment / sizeof(Value);
    MOZ_ASSERT(IsPowerOfTwo(alignment));
    masm.addl(Imm32(alignment - 1 ), ecx);
    masm.andl(Imm32(~(alignment - 1)), ecx);
    masm.subl(esi, ecx);

    
    masm.loadPtr(Address(esp, RectifierFrameLayout::offsetOfNumActualArgs()), edx);

    masm.moveValue(UndefinedValue(), ebx, edi);

    
    
    
    
    masm.push(FramePointer);
    masm.movl(esp, FramePointer); 
    masm.push(FramePointer );

    
    
    
    
    
    
    
    
    

    
    {
        Label undefLoopTop;
        masm.bind(&undefLoopTop);

        masm.push(ebx); 
        masm.push(edi); 
        masm.subl(Imm32(1), ecx);
        masm.j(Assembler::NonZero, &undefLoopTop);
    }

    
    
    BaseIndex b = BaseIndex(FramePointer, esi, TimesEight,
                            sizeof(RectifierFrameLayout) + sizeof(void*));
    masm.lea(Operand(b), ecx);

    
    masm.addl(Imm32(1), esi);
    {
        Label copyLoopTop;

        masm.bind(&copyLoopTop);
        masm.push(Operand(ecx, sizeof(Value)/2));
        masm.push(Operand(ecx, 0x0));
        masm.subl(Imm32(sizeof(Value)), ecx);
        masm.subl(Imm32(1), esi);
        masm.j(Assembler::NonZero, &copyLoopTop);
    }

    
    masm.lea(Operand(FramePointer, sizeof(void*)), ebx);
    masm.subl(esp, ebx);
    masm.makeFrameDescriptor(ebx, JitFrame_Rectifier);

    
    masm.push(edx); 
    masm.push(eax); 
    masm.push(ebx); 

    
    
    masm.andl(Imm32(CalleeTokenMask), eax);
    masm.loadPtr(Address(eax, JSFunction::offsetOfNativeOrScript()), eax);
    masm.loadBaselineOrIonRaw(eax, eax, nullptr);
    masm.call(eax);
    uint32_t returnOffset = masm.currentOffset();

    
    masm.pop(ebx);            
    masm.shrl(Imm32(FRAMESIZE_SHIFT), ebx); 
    masm.pop(edi);            
    masm.pop(edi);            

    
    BaseIndex unwind = BaseIndex(esp, ebx, TimesOne, -int32_t(sizeof(void*)));
    masm.lea(Operand(unwind), esp);

    masm.pop(FramePointer);
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
PushBailoutFrame(MacroAssembler &masm, uint32_t frameClass, Register spArg)
{
    
    if (JitSupportsSimd()) {
        masm.PushRegsInMask(AllRegs);
    } else {
        
        
        
        
        
        RegisterSet set = AllRegs;
        for (GeneralRegisterBackwardIterator iter(set.gprs()); iter.more(); iter++)
            masm.Push(*iter);

        masm.reserveStack(sizeof(RegisterDump::FPUArray));
        for (FloatRegisterBackwardIterator iter(set.fpus()); iter.more(); iter++) {
            FloatRegister reg = *iter;
            Address spillAddress(StackPointer, reg.getRegisterDumpOffsetInBytes());
            masm.storeDouble(reg, spillAddress);
        }
    }

    
    masm.push(Imm32(frameClass));

    
    masm.movl(esp, spArg);
}

static void
GenerateBailoutThunk(JSContext *cx, MacroAssembler &masm, uint32_t frameClass)
{
    PushBailoutFrame(masm, frameClass, eax);

    
    masm.reserveStack(sizeof(void *));
    masm.movl(esp, ebx);

    
    masm.setupUnalignedABICall(2, ecx);
    masm.passABIArg(eax);
    masm.passABIArg(ebx);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, Bailout));

    masm.pop(ecx); 

    
    static const uint32_t BailoutDataSize = 0
        + sizeof(void *) 
        + sizeof(RegisterDump);

    
    if (frameClass == NO_FRAME_SIZE_CLASS_ID) {
        
        
        
        
        
        masm.addl(Imm32(BailoutDataSize), esp);
        masm.pop(ebx);
        masm.addl(Imm32(sizeof(uint32_t)), esp);
        masm.addl(ebx, esp);
    } else {
        
        
        
        
        uint32_t frameSize = FrameSizeClass::FromClass(frameClass).frameSize();
        masm.addl(Imm32(BailoutDataSize + sizeof(void *) + frameSize), esp);
    }

    
    JitCode *bailoutTail = cx->runtime()->jitRuntime()->getBailoutTail();
    masm.jmp(bailoutTail);
}

JitCode *
JitRuntime::generateBailoutTable(JSContext *cx, uint32_t frameClass)
{
    MacroAssembler masm;

    Label bailout;
    for (size_t i = 0; i < BAILOUT_TABLE_SIZE; i++)
        masm.call(&bailout);
    masm.bind(&bailout);

    GenerateBailoutThunk(cx, masm, frameClass);

    Linker linker(masm);
    JitCode *code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "BailoutHandler");
#endif

    return code;
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

    
    Register cxreg = regs.takeAny();

    
    
    
    
    
    
    
    masm.enterExitFrame(&f);
    masm.loadJSContext(cxreg);

    
    Register argsBase = InvalidReg;
    if (f.explicitArgs) {
        argsBase = regs.takeAny();
        masm.lea(Operand(esp, ExitFrameLayout::SizeWithFooter()), argsBase);
    }

    
    Register outReg = InvalidReg;
    switch (f.outParam) {
      case Type_Value:
        outReg = regs.takeAny();
        masm.Push(UndefinedValue());
        masm.movl(esp, outReg);
        break;

      case Type_Handle:
        outReg = regs.takeAny();
        masm.PushEmptyRooted(f.outParamRootType);
        masm.movl(esp, outReg);
        break;

      case Type_Int32:
      case Type_Pointer:
      case Type_Bool:
        outReg = regs.takeAny();
        masm.reserveStack(sizeof(int32_t));
        masm.movl(esp, outReg);
        break;

      case Type_Double:
        outReg = regs.takeAny();
        masm.reserveStack(sizeof(double));
        masm.movl(esp, outReg);
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
            
            
            masm.passABIArg(MoveOperand(argsBase, argDisp), MoveOp::GENERAL);
            argDisp += sizeof(void *);
            masm.passABIArg(MoveOperand(argsBase, argDisp), MoveOp::GENERAL);
            argDisp += sizeof(void *);
            break;
          case VMFunction::WordByRef:
            masm.passABIArg(MoveOperand(argsBase, argDisp, MoveOperand::EFFECTIVE_ADDRESS),
                            MoveOp::GENERAL);
            argDisp += sizeof(void *);
            break;
          case VMFunction::DoubleByRef:
            masm.passABIArg(MoveOperand(argsBase, argDisp, MoveOperand::EFFECTIVE_ADDRESS),
                            MoveOp::GENERAL);
            argDisp += 2 * sizeof(void *);
            break;
        }
    }

    
    if (outReg != InvalidReg)
        masm.passABIArg(outReg);

    masm.callWithABI(f.wrapped);

    
    switch (f.failType()) {
      case Type_Object:
        masm.branchTestPtr(Assembler::Zero, eax, eax, masm.failureLabel());
        break;
      case Type_Bool:
        masm.testb(eax, eax);
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
        masm.Pop(JSReturnOperand);
        break;

      case Type_Int32:
      case Type_Pointer:
        masm.Pop(ReturnReg);
        break;

      case Type_Bool:
        masm.Pop(ReturnReg);
        masm.movzbl(ReturnReg, ReturnReg);
        break;

      case Type_Double:
        if (cx->runtime()->jitSupportsFloatingPoint)
            masm.Pop(ReturnDoubleReg);
        else
            masm.assumeUnreachable("Unable to pop to float reg, with no FP support.");
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

    RegisterSet save;
    if (cx->runtime()->jitSupportsFloatingPoint) {
        save = RegisterSet(GeneralRegisterSet(Registers::VolatileMask),
                           FloatRegisterSet(FloatRegisters::VolatileMask));
    } else {
        save = RegisterSet(GeneralRegisterSet(Registers::VolatileMask),
                           FloatRegisterSet());
    }
    masm.PushRegsInMask(save);

    MOZ_ASSERT(PreBarrierReg == edx);
    masm.movl(ImmPtr(cx->runtime()), ecx);

    masm.setupUnalignedABICall(2, eax);
    masm.passABIArg(ecx);
    masm.passABIArg(edx);
    masm.callWithABI(IonMarkFunction(type));

    masm.PopRegsInMask(save);
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

    Register scratch1 = eax;
    Register scratch2 = ecx;
    Register scratch3 = edx;

    
    masm.loadPtr(Address(esp, 0), scratch1);

    
    masm.mov(ebp, scratch2);
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
    masm.mov(ebp, esp);
    masm.pop(ebp);

    
    
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

    masm.generateBailoutTail(edx, ecx);

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

    Register scratch1 = eax;
    Register scratch2 = ebx;
    Register scratch3 = esi;
    Register scratch4 = edi;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
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
