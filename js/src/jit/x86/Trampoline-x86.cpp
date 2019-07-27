





#include "jscompartment.h"

#include "jit/Bailouts.h"
#include "jit/BaselineJIT.h"
#include "jit/IonFrames.h"
#include "jit/IonLinker.h"
#include "jit/IonSpewer.h"
#include "jit/JitCompartment.h"
#ifdef JS_ION_PERF
# include "jit/PerfSpewer.h"
#endif
#include "jit/ParallelFunctions.h"
#include "jit/VMFunctions.h"
#include "jit/x86/BaselineHelpers-x86.h"

#include "jsscriptinlines.h"

#include "jit/ExecutionMode-inl.h"

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

    
    masm.push(ebp);
    masm.movl(esp, ebp);

    
    
    
    masm.push(ebx);
    masm.push(esi);
    masm.push(edi);

    
    masm.spsMarkJit(&cx->runtime()->spsProfiler, ebp, ebx);

    
    
    masm.movl(esp, esi);

    
    masm.loadPtr(Address(ebp, ARG_ARGC), eax);
    masm.shll(Imm32(3), eax);

    
    
    
    
    
    
    
    
    masm.movl(esp, ecx);
    masm.subl(eax, ecx);
    masm.subl(Imm32(4 * 3), ecx);

    
    masm.andl(Imm32(15), ecx);
    masm.subl(ecx, esp);

    



    
    masm.loadPtr(Address(ebp, ARG_ARGV), ebx);

    
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
        
        masm.enterFakeExitFrame(IonExitFrameLayout::BareToken());

        masm.push(framePtr);
        masm.push(jitcode);

        masm.setupUnalignedABICall(3, scratch);
        masm.passABIArg(framePtr); 
        masm.passABIArg(OsrFrameReg); 
        masm.passABIArg(numStackValues);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, jit::InitBaselineFrameForOsr));

        masm.pop(jitcode);
        masm.pop(framePtr);

        JS_ASSERT(jitcode != ReturnReg);

        Label error;
        masm.addPtr(Imm32(IonExitFrameLayout::SizeWithFooter()), esp);
        masm.addPtr(Imm32(BaselineFrame::Size()), framePtr);
        masm.branchIfFalseBool(ReturnReg, &error);

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

    



    masm.call(Operand(ebp, ARG_JITCODE));

    if (type == EnterJitBaseline) {
        
        masm.bind(returnLabel.src());
        if (!masm.addCodeLabel(returnLabel))
            return nullptr;
    }

    
    
    masm.pop(eax);
    masm.shrl(Imm32(FRAMESIZE_SHIFT), eax); 
    masm.addl(eax, esp);

    
    
    
    
    
    
    
    
    
    masm.loadPtr(Address(esp, ARG_RESULT + 4 * sizeof(void *)), eax);
    masm.storeValue(JSReturnOperand, Operand(eax, 0));

    


    
    masm.spsUnmarkJit(&cx->runtime()->spsProfiler, ebx);

    
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
    AutoIonContextAlloc aica(cx);
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
    IonSpew(IonSpew_Invalidate, "   invalidation thunk created at %p", (void *) code->raw());

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "Invalidator");
#endif

    return code;
}

JitCode *
JitRuntime::generateArgumentsRectifier(JSContext *cx, ExecutionMode mode, void **returnAddrOut)
{
    MacroAssembler masm(cx);

    
    
    JS_ASSERT(ArgumentsRectifierReg == esi);

    
    masm.loadPtr(Address(esp, IonRectifierFrameLayout::offsetOfCalleeToken()), eax);
    masm.movzwl(Operand(eax, JSFunction::offsetOfNargs()), ecx);
    masm.subl(esi, ecx);

    
    masm.loadPtr(Address(esp, IonRectifierFrameLayout::offsetOfNumActualArgs()), edx);

    masm.moveValue(UndefinedValue(), ebx, edi);

    
    
    
    
    masm.push(FramePointer);
    masm.movl(esp, FramePointer); 

    
    {
        Label undefLoopTop;
        masm.bind(&undefLoopTop);

        masm.push(ebx); 
        masm.push(edi); 
        masm.subl(Imm32(1), ecx);
        masm.j(Assembler::NonZero, &undefLoopTop);
    }

    
    
    BaseIndex b = BaseIndex(FramePointer, esi, TimesEight,
                            sizeof(IonRectifierFrameLayout) + sizeof(void*));
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

    
    
    masm.loadPtr(Address(eax, JSFunction::offsetOfNativeOrScript()), eax);
    masm.loadBaselineOrIonRaw(eax, eax, mode, nullptr);
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
    
    masm.PushRegsInMask(AllRegs);

    
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

    
    const uint32_t BailoutDataSize = sizeof(void *) + 
                                   sizeof(double) * FloatRegisters::Total +
                                   sizeof(void *) * Registers::Total;

    
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

static void
GenerateParallelBailoutThunk(MacroAssembler &masm, uint32_t frameClass)
{
    
    
    

    PushBailoutFrame(masm, frameClass, eax);

    
    
    masm.reserveStack(sizeof(uint8_t *));
    masm.movePtr(esp, ebx);

    masm.setupUnalignedABICall(2, ecx);
    masm.passABIArg(eax);
    masm.passABIArg(ebx);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, BailoutPar));

    
    masm.moveValue(MagicValue(JS_ION_ERROR), JSReturnOperand);
    masm.loadPtr(Address(esp, 0), esp);
    masm.ret();
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
JitRuntime::generateBailoutHandler(JSContext *cx, ExecutionMode mode)
{
    MacroAssembler masm;

    switch (mode) {
      case SequentialExecution:
        GenerateBailoutThunk(cx, masm, NO_FRAME_SIZE_CLASS_ID);
        break;
      case ParallelExecution:
        GenerateParallelBailoutThunk(masm, NO_FRAME_SIZE_CLASS_ID);
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

    
    Register cxreg = regs.takeAny();

    
    
    
    
    
    
    
    masm.enterExitFrameAndLoadContext(&f, cxreg, regs.getAny(), f.executionMode);

    
    Register argsBase = InvalidReg;
    if (f.explicitArgs) {
        argsBase = regs.takeAny();
        masm.lea(Operand(esp, IonExitFrameLayout::SizeWithFooter()), argsBase);
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
        masm.branchTestPtr(Assembler::Zero, eax, eax, masm.failureLabel(f.executionMode));
        break;
      case Type_Bool:
        masm.testb(eax, eax);
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

    RegisterSet save;
    if (cx->runtime()->jitSupportsFloatingPoint) {
        save = RegisterSet(GeneralRegisterSet(Registers::VolatileMask),
                           FloatRegisterSet(FloatRegisters::VolatileMask));
    } else {
        save = RegisterSet(GeneralRegisterSet(Registers::VolatileMask),
                           FloatRegisterSet());
    }
    masm.PushRegsInMask(save);

    JS_ASSERT(PreBarrierReg == edx);
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

    masm.generateBailoutTail(edx, ecx);

    Linker linker(masm);
    JitCode *code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "BailoutTailStub");
#endif

    return code;
}
