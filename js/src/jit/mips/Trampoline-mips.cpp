





#include "mozilla/DebugOnly.h"

#include "jscompartment.h"

#include "jit/Bailouts.h"
#include "jit/JitCompartment.h"
#include "jit/JitFrames.h"
#include "jit/JitSpewer.h"
#include "jit/Linker.h"
#include "jit/mips/Bailouts-mips.h"
#include "jit/mips/BaselineHelpers-mips.h"
#ifdef JS_ION_PERF
# include "jit/PerfSpewer.h"
#endif
#include "jit/VMFunctions.h"

using namespace js;
using namespace js::jit;

static_assert(sizeof(uintptr_t) == sizeof(uint32_t), "Not 64-bit clean.");

struct EnterJITRegs
{
    double f30;
    double f28;
    double f26;
    double f24;
    double f22;
    double f20;

    
    uintptr_t align;

    
    uintptr_t ra;
    uintptr_t s7;
    uintptr_t s6;
    uintptr_t s5;
    uintptr_t s4;
    uintptr_t s3;
    uintptr_t s2;
    uintptr_t s1;
    uintptr_t s0;
};

struct EnterJITArgs
{
    
    void* jitcode; 
    int maxArgc;
    Value* maxArgv;
    InterpreterFrame* fp;

    
    CalleeToken calleeToken;
    JSObject* scopeChain;
    size_t numStackValues;
    Value* vp;
};

static void
GenerateReturn(MacroAssembler& masm, int returnCode)
{
    MOZ_ASSERT(masm.framePushed() == sizeof(EnterJITRegs));

    
    masm.loadPtr(Address(StackPointer, offsetof(EnterJITRegs, s0)), s0);
    masm.loadPtr(Address(StackPointer, offsetof(EnterJITRegs, s1)), s1);
    masm.loadPtr(Address(StackPointer, offsetof(EnterJITRegs, s2)), s2);
    masm.loadPtr(Address(StackPointer, offsetof(EnterJITRegs, s3)), s3);
    masm.loadPtr(Address(StackPointer, offsetof(EnterJITRegs, s4)), s4);
    masm.loadPtr(Address(StackPointer, offsetof(EnterJITRegs, s5)), s5);
    masm.loadPtr(Address(StackPointer, offsetof(EnterJITRegs, s6)), s6);
    masm.loadPtr(Address(StackPointer, offsetof(EnterJITRegs, s7)), s7);
    masm.loadPtr(Address(StackPointer, offsetof(EnterJITRegs, ra)), ra);

    
    masm.loadDouble(Address(StackPointer, offsetof(EnterJITRegs, f20)), f20);
    masm.loadDouble(Address(StackPointer, offsetof(EnterJITRegs, f22)), f22);
    masm.loadDouble(Address(StackPointer, offsetof(EnterJITRegs, f24)), f24);
    masm.loadDouble(Address(StackPointer, offsetof(EnterJITRegs, f26)), f26);
    masm.loadDouble(Address(StackPointer, offsetof(EnterJITRegs, f28)), f28);
    masm.loadDouble(Address(StackPointer, offsetof(EnterJITRegs, f30)), f30);

    masm.freeStack(sizeof(EnterJITRegs));

    masm.branch(ra);
}

static void
GeneratePrologue(MacroAssembler& masm)
{
    
    
    
    masm.reserveStack(sizeof(EnterJITRegs));
    masm.storePtr(s0, Address(StackPointer, offsetof(EnterJITRegs, s0)));
    masm.storePtr(s1, Address(StackPointer, offsetof(EnterJITRegs, s1)));
    masm.storePtr(s2, Address(StackPointer, offsetof(EnterJITRegs, s2)));
    masm.storePtr(s3, Address(StackPointer, offsetof(EnterJITRegs, s3)));
    masm.storePtr(s4, Address(StackPointer, offsetof(EnterJITRegs, s4)));
    masm.storePtr(s5, Address(StackPointer, offsetof(EnterJITRegs, s5)));
    masm.storePtr(s6, Address(StackPointer, offsetof(EnterJITRegs, s6)));
    masm.storePtr(s7, Address(StackPointer, offsetof(EnterJITRegs, s7)));
    masm.storePtr(ra, Address(StackPointer, offsetof(EnterJITRegs, ra)));

    masm.as_sd(f20, StackPointer, offsetof(EnterJITRegs, f20));
    masm.as_sd(f22, StackPointer, offsetof(EnterJITRegs, f22));
    masm.as_sd(f24, StackPointer, offsetof(EnterJITRegs, f24));
    masm.as_sd(f26, StackPointer, offsetof(EnterJITRegs, f26));
    masm.as_sd(f28, StackPointer, offsetof(EnterJITRegs, f28));
    masm.as_sd(f30, StackPointer, offsetof(EnterJITRegs, f30));
}









JitCode*
JitRuntime::generateEnterJIT(JSContext* cx, EnterJitType type)
{
    const Register reg_code = a0;
    const Register reg_argc = a1;
    const Register reg_argv = a2;
    const mozilla::DebugOnly<Register> reg_frame = a3;

    MOZ_ASSERT(OsrFrameReg == reg_frame);

    MacroAssembler masm(cx);
    GeneratePrologue(masm);

    const Address slotToken(sp, sizeof(EnterJITRegs) + offsetof(EnterJITArgs, calleeToken));
    const Address slotVp(sp, sizeof(EnterJITRegs) + offsetof(EnterJITArgs, vp));

    
    masm.movePtr(StackPointer, s4);

    
    masm.loadPtr(slotToken, s2);

    
    if (type == EnterJitBaseline)
        masm.movePtr(StackPointer, BaselineFrameReg);

    
    masm.loadPtr(slotVp, s3);
    masm.unboxInt32(Address(s3, 0), s3);

    



    masm.as_sll(s0, reg_argc, 3); 
    masm.addPtr(reg_argv, s0); 

    
    
    Label header, footer;
    
    masm.ma_b(s0, reg_argv, &footer, Assembler::BelowOrEqual, ShortJump);
    {
        masm.bind(&header);

        masm.subPtr(Imm32(2 * sizeof(uintptr_t)), s0);
        masm.subPtr(Imm32(2 * sizeof(uintptr_t)), StackPointer);

        ValueOperand value = ValueOperand(s6, s7);
        masm.loadValue(Address(s0, 0), value);
        masm.storeValue(value, Address(StackPointer, 0));

        masm.ma_b(s0, reg_argv, &header, Assembler::Above, ShortJump);
    }
    masm.bind(&footer);

    masm.subPtr(Imm32(2 * sizeof(uintptr_t)), StackPointer);
    masm.storePtr(s3, Address(StackPointer, sizeof(uintptr_t))); 
    masm.storePtr(s2, Address(StackPointer, 0)); 

    masm.subPtr(StackPointer, s4);
    masm.makeFrameDescriptor(s4, JitFrame_Entry);
    masm.push(s4); 

    CodeLabel returnLabel;
    if (type == EnterJitBaseline) {
        
        AllocatableGeneralRegisterSet regs(GeneralRegisterSet::All());
        regs.take(OsrFrameReg);
        regs.take(BaselineFrameReg);
        regs.take(reg_code);

        const Address slotNumStackValues(BaselineFrameReg, sizeof(EnterJITRegs) +
                                         offsetof(EnterJITArgs, numStackValues));
        const Address slotScopeChain(BaselineFrameReg, sizeof(EnterJITRegs) +
                                     offsetof(EnterJITArgs, scopeChain));

        Label notOsr;
        masm.ma_b(OsrFrameReg, OsrFrameReg, &notOsr, Assembler::Zero, ShortJump);

        Register scratch = regs.takeAny();

        Register numStackValues = regs.takeAny();
        masm.load32(slotNumStackValues, numStackValues);

        
        masm.subPtr(Imm32(sizeof(uintptr_t)), StackPointer);
        masm.ma_li(scratch, returnLabel.dest());
        masm.storePtr(scratch, Address(StackPointer, 0));

        
        masm.subPtr(Imm32(sizeof(uintptr_t)), StackPointer);
        masm.storePtr(BaselineFrameReg, Address(StackPointer, 0));

        
        Register framePtr = BaselineFrameReg;
        masm.subPtr(Imm32(BaselineFrame::Size()), StackPointer);
        masm.movePtr(StackPointer, framePtr);

        
        masm.ma_sll(scratch, numStackValues, Imm32(3));
        masm.subPtr(scratch, StackPointer);

        
        masm.addPtr(Imm32(BaselineFrame::Size() + BaselineFrame::FramePointerOffset), scratch);
        masm.makeFrameDescriptor(scratch, JitFrame_BaselineJS);

        
        masm.reserveStack(2 * sizeof(uintptr_t));
        masm.storePtr(scratch, Address(StackPointer, sizeof(uintptr_t))); 
        masm.storePtr(zero, Address(StackPointer, 0)); 

        
        masm.enterFakeExitFrame(ExitFrameLayout::BareToken());

        masm.reserveStack(2 * sizeof(uintptr_t));
        masm.storePtr(framePtr, Address(StackPointer, sizeof(uintptr_t))); 
        masm.storePtr(reg_code, Address(StackPointer, 0)); 

        masm.setupUnalignedABICall(3, scratch);
        masm.passABIArg(BaselineFrameReg); 
        masm.passABIArg(OsrFrameReg); 
        masm.passABIArg(numStackValues);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void*, jit::InitBaselineFrameForOsr));

        regs.add(OsrFrameReg);
        regs.add(scratch);
        regs.add(numStackValues);
        regs.take(JSReturnOperand);
        regs.take(ReturnReg);
        Register jitcode = regs.takeAny();
        masm.loadPtr(Address(StackPointer, 0), jitcode);
        masm.loadPtr(Address(StackPointer, sizeof(uintptr_t)), framePtr);
        masm.freeStack(2 * sizeof(uintptr_t));

        Label error;
        masm.freeStack(ExitFrameLayout::SizeWithFooter());
        masm.addPtr(Imm32(BaselineFrame::Size()), framePtr);
        masm.branchIfFalseBool(ReturnReg, &error);

        
        
        {
            Label skipProfilingInstrumentation;
            Register realFramePtr = numStackValues;
            AbsoluteAddress addressOfEnabled(cx->runtime()->spsProfiler.addressOfEnabled());
            masm.branch32(Assembler::Equal, addressOfEnabled, Imm32(0),
                          &skipProfilingInstrumentation);
            masm.ma_addu(realFramePtr, framePtr, Imm32(sizeof(void*)));
            masm.profilerEnterFrame(realFramePtr, scratch);
            masm.bind(&skipProfilingInstrumentation);
        }

        masm.jump(jitcode);

        
        
        masm.bind(&error);
        masm.movePtr(framePtr, StackPointer);
        masm.addPtr(Imm32(2 * sizeof(uintptr_t)), StackPointer);
        masm.moveValue(MagicValue(JS_ION_ERROR), JSReturnOperand);
        masm.ma_li(scratch, returnLabel.dest());
        masm.jump(scratch);

        masm.bind(&notOsr);
        
        MOZ_ASSERT(R1.scratchReg() != reg_code);
        masm.loadPtr(slotScopeChain, R1.scratchReg());
    }

    
    
    masm.assertStackAlignment(JitStackAlignment, sizeof(uintptr_t));

    
    masm.ma_callJitHalfPush(reg_code);

    if (type == EnterJitBaseline) {
        
        masm.bind(returnLabel.src());
        masm.addCodeLabel(returnLabel);
    }

    
    
    masm.pop(s0);
    masm.rshiftPtr(Imm32(4), s0);
    masm.addPtr(s0, StackPointer);

    
    masm.loadPtr(slotVp, s1);
    masm.storeValue(JSReturnOperand, Address(s1, 0));

    
    GenerateReturn(masm, ShortJump);

    Linker linker(masm);
    AutoFlushICache afc("GenerateEnterJIT");
    JitCode* code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "EnterJIT");
#endif

    return code;
}

JitCode*
JitRuntime::generateInvalidator(JSContext* cx)
{
    MacroAssembler masm(cx);

    
    
    static const uint32_t STACK_DATA_SIZE = sizeof(InvalidationBailoutStack) -
                                            2 * sizeof(uintptr_t);

    
    masm.checkStackAlignment();

    
    masm.subPtr(Imm32(STACK_DATA_SIZE), StackPointer);

    
    for (uint32_t i = 0; i < Registers::Total; i++) {
        Address address = Address(StackPointer, InvalidationBailoutStack::offsetOfRegs() +
                                                i * sizeof(uintptr_t));
        masm.storePtr(Register::FromCode(i), address);
    }

    
    
    for (uint32_t i = 0; i < FloatRegisters::TotalDouble; i ++)
        masm.as_sd(FloatRegister::FromIndex(i, FloatRegister::Double), StackPointer,
                   InvalidationBailoutStack::offsetOfFpRegs() + i * sizeof(double));

    
    masm.movePtr(StackPointer, a0);

    
    masm.subPtr(Imm32(2 * sizeof(uintptr_t)), StackPointer);
    
    masm.ma_addu(a1, StackPointer, Imm32(sizeof(uintptr_t)));
    
    masm.movePtr(StackPointer, a2);

    masm.setupAlignedABICall(3);
    masm.passABIArg(a0);
    masm.passABIArg(a1);
    masm.passABIArg(a2);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void*, InvalidationBailout));

    masm.loadPtr(Address(StackPointer, 0), a2);
    masm.loadPtr(Address(StackPointer, sizeof(uintptr_t)), a1);
    
    
    
    masm.addPtr(Imm32(sizeof(InvalidationBailoutStack) + 2 * sizeof(uintptr_t)), StackPointer);
    
    
    masm.addPtr(a1, StackPointer);

    
    JitCode* bailoutTail = cx->runtime()->jitRuntime()->getBailoutTail();
    masm.branch(bailoutTail);

    Linker linker(masm);
    AutoFlushICache afc("Invalidator");
    JitCode* code = linker.newCode<NoGC>(cx, OTHER_CODE);
    JitSpew(JitSpew_IonInvalidate, "   invalidation thunk created at %p", (void*) code->raw());

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "Invalidator");
#endif

    return code;
}

JitCode*
JitRuntime::generateArgumentsRectifier(JSContext* cx, void** returnAddrOut)
{
    MacroAssembler masm(cx);

    
    
    MOZ_ASSERT(ArgumentsRectifierReg == s3);

    Register numActArgsReg = t6;
    Register calleeTokenReg = t7;
    Register numArgsReg = t5;

    
    masm.loadPtr(Address(StackPointer, RectifierFrameLayout::offsetOfNumActualArgs()),
                 numActArgsReg);

    
    masm.loadPtr(Address(StackPointer, RectifierFrameLayout::offsetOfCalleeToken()),
                 calleeTokenReg);
    masm.mov(calleeTokenReg, numArgsReg);
    masm.andPtr(Imm32(CalleeTokenMask), numArgsReg);
    masm.load16ZeroExtend(Address(numArgsReg, JSFunction::offsetOfNargs()), numArgsReg);

    masm.ma_subu(t1, numArgsReg, s3);

    masm.moveValue(UndefinedValue(), ValueOperand(t3, t4));

    masm.movePtr(StackPointer, t2); 

    
    {
        Label undefLoopTop;
        masm.bind(&undefLoopTop);

        masm.subPtr(Imm32(sizeof(Value)), StackPointer);
        masm.storeValue(ValueOperand(t3, t4), Address(StackPointer, 0));
        masm.sub32(Imm32(1), t1);

        masm.ma_b(t1, t1, &undefLoopTop, Assembler::NonZero, ShortJump);
    }

    
    masm.ma_sll(t0, s3, Imm32(3)); 
    masm.addPtr(t0, t2); 
    masm.addPtr(Imm32(sizeof(RectifierFrameLayout)), t2);

    
    {
        Label copyLoopTop, initialSkip;

        masm.ma_b(&initialSkip, ShortJump);

        masm.bind(&copyLoopTop);
        masm.subPtr(Imm32(sizeof(Value)), t2);
        masm.sub32(Imm32(1), s3);

        masm.bind(&initialSkip);

        MOZ_ASSERT(sizeof(Value) == 2 * sizeof(uint32_t));
        
        masm.subPtr(Imm32(sizeof(Value)), StackPointer);
        masm.load32(Address(t2, NUNBOX32_TYPE_OFFSET), t0);
        masm.store32(t0, Address(StackPointer, NUNBOX32_TYPE_OFFSET));
        masm.load32(Address(t2, NUNBOX32_PAYLOAD_OFFSET), t0);
        masm.store32(t0, Address(StackPointer, NUNBOX32_PAYLOAD_OFFSET));

        masm.ma_b(s3, s3, &copyLoopTop, Assembler::NonZero, ShortJump);
    }

    
    masm.ma_addu(t0, numArgsReg, Imm32(1));
    masm.lshiftPtr(Imm32(3), t0);

    
    masm.makeFrameDescriptor(t0, JitFrame_Rectifier);

    
    masm.subPtr(Imm32(3 * sizeof(uintptr_t)), StackPointer);
    
    masm.storePtr(numActArgsReg, Address(StackPointer, 2 * sizeof(uintptr_t)));
    
    masm.storePtr(calleeTokenReg, Address(StackPointer, sizeof(uintptr_t)));
    
    masm.storePtr(t0, Address(StackPointer, 0));

    
    
    masm.andPtr(Imm32(CalleeTokenMask), calleeTokenReg);
    masm.loadPtr(Address(calleeTokenReg, JSFunction::offsetOfNativeOrScript()), t1);
    masm.loadBaselineOrIonRaw(t1, t1, nullptr);
    masm.ma_callJitHalfPush(t1);

    uint32_t returnOffset = masm.currentOffset();

    
    
    
    
    
    
    

    
    
    masm.loadPtr(Address(StackPointer, 0), t0);
    masm.rshiftPtr(Imm32(FRAMESIZE_SHIFT), t0); 

    
    masm.addPtr(Imm32(3 * sizeof(uintptr_t)), StackPointer);

    
    
    
    
    
    
    

    
    masm.addPtr(t0, StackPointer);

    masm.ret();
    Linker linker(masm);
    AutoFlushICache afc("ArgumentsRectifier");
    JitCode* code = linker.newCode<NoGC>(cx, OTHER_CODE);

    CodeOffsetLabel returnLabel(returnOffset);
    returnLabel.fixup(&masm);
    if (returnAddrOut)
        *returnAddrOut = (void*) (code->raw() + returnLabel.offset());

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "ArgumentsRectifier");
#endif

    return code;
}



static const uint32_t bailoutDataSize = sizeof(BailoutStack) - 2 * sizeof(uintptr_t);
static const uint32_t bailoutInfoOutParamSize = 2 * sizeof(uintptr_t);


















static void
PushBailoutFrame(MacroAssembler& masm, uint32_t frameClass, Register spArg)
{
    
    masm.checkStackAlignment();

    
    masm.subPtr(Imm32(bailoutDataSize), StackPointer);

    
    for (uint32_t i = 0; i < Registers::Total; i++) {
        uint32_t off = BailoutStack::offsetOfRegs() + i * sizeof(uintptr_t);
        masm.storePtr(Register::FromCode(i), Address(StackPointer, off));
    }

    
    
    for (uint32_t i = 0; i < FloatRegisters::TotalDouble; i++)
        masm.as_sd(FloatRegister::FromIndex(i, FloatRegister::Double), StackPointer,
                   BailoutStack::offsetOfFpRegs() + i * sizeof(double));

    
    
    
    masm.storePtr(ra, Address(StackPointer, BailoutStack::offsetOfFrameSize()));

    
    masm.storePtr(ImmWord(frameClass), Address(StackPointer, BailoutStack::offsetOfFrameClass()));

    
    masm.movePtr(StackPointer, spArg);
}

static void
GenerateBailoutThunk(JSContext* cx, MacroAssembler& masm, uint32_t frameClass)
{
    PushBailoutFrame(masm, frameClass, a0);

    
    masm.subPtr(Imm32(bailoutInfoOutParamSize), StackPointer);
    masm.storePtr(ImmPtr(nullptr), Address(StackPointer, 0));
    masm.movePtr(StackPointer, a1);

    masm.setupAlignedABICall(2);
    masm.passABIArg(a0);
    masm.passABIArg(a1);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void*, Bailout));

    
    masm.loadPtr(Address(StackPointer, 0), a2);

    
    if (frameClass == NO_FRAME_SIZE_CLASS_ID) {
        
        masm.loadPtr(Address(StackPointer,
                             bailoutInfoOutParamSize + BailoutStack::offsetOfFrameSize()), a1);

        
        masm.addPtr(Imm32(sizeof(BailoutStack) + bailoutInfoOutParamSize), StackPointer);
        
        masm.addPtr(a1, StackPointer);
    } else {
        uint32_t frameSize = FrameSizeClass::FromClass(frameClass).frameSize();
        
        masm.addPtr(Imm32(bailoutDataSize + bailoutInfoOutParamSize + frameSize), StackPointer);
    }

    
    JitCode* bailoutTail = cx->runtime()->jitRuntime()->getBailoutTail();
    masm.branch(bailoutTail);
}

JitCode*
JitRuntime::generateBailoutTable(JSContext* cx, uint32_t frameClass)
{
    MacroAssembler masm(cx);

    Label bailout;
    for (size_t i = 0; i < BAILOUT_TABLE_SIZE; i++) {
        
        int32_t offset = (BAILOUT_TABLE_SIZE - i) * BAILOUT_TABLE_ENTRY_SIZE;

        
        masm.as_bal(BOffImm16(offset));
        masm.nop();
    }
    masm.bind(&bailout);

    GenerateBailoutThunk(cx, masm, frameClass);

    Linker linker(masm);
    AutoFlushICache afc("BailoutTable");
    JitCode* code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "BailoutTable");
#endif

    return code;
}

JitCode*
JitRuntime::generateBailoutHandler(JSContext* cx)
{
    MacroAssembler masm(cx);
    GenerateBailoutThunk(cx, masm, NO_FRAME_SIZE_CLASS_ID);

    Linker linker(masm);
    AutoFlushICache afc("BailoutHandler");
    JitCode* code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "BailoutHandler");
#endif

    return code;
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

    AllocatableGeneralRegisterSet regs(GeneralRegisterSet(Register::Codes::WrapperMask));

    static_assert((Register::Codes::VolatileMask & ~Register::Codes::WrapperMask) == 0,
                  "Wrapper register set should be a superset of Volatile register set.");

    
    Register cxreg = a0;
    regs.take(cxreg);

    
    masm.enterExitFrame(&f);
    masm.loadJSContext(cxreg);

    
    Register argsBase = InvalidReg;
    if (f.explicitArgs) {
        argsBase = t1; 
        regs.take(argsBase);
        masm.ma_addu(argsBase, StackPointer, Imm32(ExitFrameLayout::SizeWithFooter()));
    }

    masm.alignStackPointer();

    
    
    uint32_t outParamSize = 0;
    switch (f.outParam) {
      case Type_Value:
        outParamSize = sizeof(Value);
        masm.reserveStack(outParamSize);
        break;

      case Type_Handle:
        {
            uint32_t pushed = masm.framePushed();
            masm.PushEmptyRooted(f.outParamRootType);
            outParamSize = masm.framePushed() - pushed;
        }
        break;

      case Type_Bool:
      case Type_Int32:
        MOZ_ASSERT(sizeof(uintptr_t) == sizeof(uint32_t));
      case Type_Pointer:
        outParamSize = sizeof(uintptr_t);
        masm.reserveStack(outParamSize);
        break;

      case Type_Double:
        outParamSize = sizeof(double);
        masm.reserveStack(outParamSize);
        break;
      default:
        MOZ_ASSERT(f.outParam == Type_Void);
        break;
    }

    uint32_t outParamOffset = 0;
    if (f.outParam != Type_Void) {
        
        MOZ_ASSERT(outParamSize <= sizeof(double));
        outParamOffset += sizeof(double) - outParamSize;
    }
    
    outParamOffset += f.doubleByRefArgs() * sizeof(double);

    Register doubleArgs = t0;
    masm.reserveStack(outParamOffset);
    masm.movePtr(StackPointer, doubleArgs);

    masm.setupAlignedABICall(f.argc());
    masm.passABIArg(cxreg);

    size_t argDisp = 0;
    size_t doubleArgDisp = 0;

    
    for (uint32_t explicitArg = 0; explicitArg < f.explicitArgs; explicitArg++) {
        MoveOperand from;
        switch (f.argProperties(explicitArg)) {
          case VMFunction::WordByValue:
            masm.passABIArg(MoveOperand(argsBase, argDisp), MoveOp::GENERAL);
            argDisp += sizeof(uint32_t);
            break;
          case VMFunction::DoubleByValue:
            
            
            MOZ_ASSERT(f.argPassedInFloatReg(explicitArg));
            masm.passABIArg(MoveOperand(argsBase, argDisp), MoveOp::DOUBLE);
            argDisp += sizeof(double);
            break;
          case VMFunction::WordByRef:
            masm.passABIArg(MoveOperand(argsBase, argDisp, MoveOperand::EFFECTIVE_ADDRESS),
                            MoveOp::GENERAL);
            argDisp += sizeof(uint32_t);
            break;
          case VMFunction::DoubleByRef:
            
            masm.ma_ld(ScratchDoubleReg, Address(argsBase, argDisp));
            masm.as_sd(ScratchDoubleReg, doubleArgs, doubleArgDisp);
            masm.passABIArg(MoveOperand(doubleArgs, doubleArgDisp, MoveOperand::EFFECTIVE_ADDRESS),
                            MoveOp::GENERAL);
            doubleArgDisp += sizeof(double);
            argDisp += sizeof(double);
            break;
        }
    }

    MOZ_ASSERT_IF(f.outParam != Type_Void,
                  doubleArgDisp + sizeof(double) == outParamOffset + outParamSize);

    
    if (f.outParam != Type_Void) {
        masm.passABIArg(MoveOperand(doubleArgs, outParamOffset, MoveOperand::EFFECTIVE_ADDRESS),
                            MoveOp::GENERAL);
    }

    masm.callWithABI(f.wrapped);

    
    switch (f.failType()) {
      case Type_Object:
        masm.branchTestPtr(Assembler::Zero, v0, v0, masm.failureLabel());
        break;
      case Type_Bool:
        
        masm.branchIfFalseBool(v0, masm.failureLabel());
        break;
      default:
        MOZ_CRASH("unknown failure kind");
    }

    masm.freeStack(outParamOffset);

    
    switch (f.outParam) {
      case Type_Handle:
        masm.popRooted(f.outParamRootType, ReturnReg, JSReturnOperand);
        break;

      case Type_Value:
        masm.loadValue(Address(StackPointer, 0), JSReturnOperand);
        masm.freeStack(sizeof(Value));
        break;

      case Type_Int32:
        MOZ_ASSERT(sizeof(uintptr_t) == sizeof(uint32_t));
      case Type_Pointer:
        masm.load32(Address(StackPointer, 0), ReturnReg);
        masm.freeStack(sizeof(uintptr_t));
        break;

      case Type_Bool:
        masm.load8ZeroExtend(Address(StackPointer, 0), ReturnReg);
        masm.freeStack(sizeof(uintptr_t));
        break;

      case Type_Double:
        if (cx->runtime()->jitSupportsFloatingPoint) {
            masm.as_ld(ReturnDoubleReg, StackPointer, 0);
        } else {
            masm.assumeUnreachable("Unable to load into float reg, with no FP support.");
        }
        masm.freeStack(sizeof(double));
        break;

      default:
        MOZ_ASSERT(f.outParam == Type_Void);
        break;
    }

    masm.restoreStackPointer();

    masm.leaveExitFrame();
    masm.retn(Imm32(sizeof(ExitFrameLayout) +
                    f.explicitStackSlots() * sizeof(uintptr_t) +
                    f.extraValuesToPop * sizeof(Value)));

    Linker linker(masm);
    AutoFlushICache afc("VMWrapper");
    JitCode* wrapper = linker.newCode<NoGC>(cx, OTHER_CODE);
    if (!wrapper)
        return nullptr;

    
    
    if (!functionWrappers_->relookupOrAdd(p, &f, wrapper))
        return nullptr;

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(wrapper, "VMWrapper");
#endif

    return wrapper;
}

JitCode*
JitRuntime::generatePreBarrier(JSContext* cx, MIRType type)
{
    MacroAssembler masm(cx);

    LiveRegisterSet save;
    if (cx->runtime()->jitSupportsFloatingPoint) {
        save.set() = RegisterSet(GeneralRegisterSet(Registers::VolatileMask),
                           FloatRegisterSet(FloatRegisters::VolatileMask));
    } else {
        save.set() = RegisterSet(GeneralRegisterSet(Registers::VolatileMask),
                           FloatRegisterSet());
    }
    masm.PushRegsInMask(save);

    MOZ_ASSERT(PreBarrierReg == a1);
    masm.movePtr(ImmPtr(cx->runtime()), a0);

    masm.setupUnalignedABICall(2, a2);
    masm.passABIArg(a0);
    masm.passABIArg(a1);
    masm.callWithABI(IonMarkFunction(type));

    masm.PopRegsInMask(save);
    masm.ret();

    Linker linker(masm);
    AutoFlushICache afc("PreBarrier");
    JitCode* code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "PreBarrier");
#endif

    return code;
}

typedef bool (*HandleDebugTrapFn)(JSContext*, BaselineFrame*, uint8_t*, bool*);
static const VMFunction HandleDebugTrapInfo = FunctionInfo<HandleDebugTrapFn>(HandleDebugTrap);

JitCode*
JitRuntime::generateDebugTrapHandler(JSContext* cx)
{
    MacroAssembler masm(cx);

    Register scratch1 = t0;
    Register scratch2 = t1;

    
    masm.movePtr(s5, scratch1);
    masm.subPtr(Imm32(BaselineFrame::Size()), scratch1);

    
    
    
    masm.movePtr(ImmPtr(nullptr), BaselineStubReg);
    EmitEnterStubFrame(masm, scratch2);

    JitCode* code = cx->runtime()->jitRuntime()->getVMWrapper(HandleDebugTrapInfo);
    if (!code)
        return nullptr;

    masm.subPtr(Imm32(2 * sizeof(uintptr_t)), StackPointer);
    masm.storePtr(ra, Address(StackPointer, sizeof(uintptr_t)));
    masm.storePtr(scratch1, Address(StackPointer, 0));

    EmitCallVM(code, masm);

    EmitLeaveStubFrame(masm);

    
    
    
    Label forcedReturn;
    masm.branchTest32(Assembler::NonZero, ReturnReg, ReturnReg, &forcedReturn);

    
    masm.branch(ra);

    masm.bind(&forcedReturn);
    masm.loadValue(Address(s5, BaselineFrame::reverseOffsetOfReturnValue()),
                   JSReturnOperand);
    masm.movePtr(s5, StackPointer);
    masm.pop(s5);

    
    
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
    JitCode* codeDbg = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(codeDbg, "DebugTrapHandler");
#endif

    return codeDbg;
}


JitCode*
JitRuntime::generateExceptionTailStub(JSContext* cx, void* handler)
{
    MacroAssembler masm;

    masm.handleFailureWithHandlerTail(handler);

    Linker linker(masm);
    AutoFlushICache afc("ExceptionTailStub");
    JitCode* code = linker.newCode<NoGC>(cx, OTHER_CODE);

#ifdef JS_ION_PERF
    writePerfSpewerJitCodeProfile(code, "ExceptionTailStub");
#endif

    return code;
}

JitCode*
JitRuntime::generateBailoutTailStub(JSContext* cx)
{
    MacroAssembler masm;

    masm.generateBailoutTail(a1, a2);

    Linker linker(masm);
    AutoFlushICache afc("BailoutTailStub");
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

    Register scratch1 = t0;
    Register scratch2 = t1;
    Register scratch3 = t2;
    Register scratch4 = t3;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
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

    
    
    
    masm.ma_and(scratch2, scratch1, Imm32((1 << FRAMESIZE_SHIFT) - 1));
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

        
        
        masm.ma_addu(scratch2, StackPointer, scratch1);
        masm.ma_addu(scratch2, scratch2, Imm32(JitFrameLayout::Size()));
        masm.storePtr(scratch2, lastProfilingFrame);
        masm.ret();
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    masm.bind(&handle_BaselineStub);
    {
        masm.ma_addu(scratch3, StackPointer, scratch1);
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
        
        masm.ma_addu(scratch2, StackPointer, scratch1);
        masm.add32(Imm32(JitFrameLayout::Size()), scratch2);
        masm.loadPtr(Address(scratch2, RectifierFrameLayout::offsetOfDescriptor()), scratch3);
        masm.ma_srl(scratch1, scratch3, Imm32(FRAMESIZE_SHIFT));
        masm.and32(Imm32((1 << FRAMETYPE_BITS) - 1), scratch3);

        
        
        

        
        Label handle_Rectifier_BaselineStub;
        masm.branch32(Assembler::NotEqual, scratch3, Imm32(JitFrame_IonJS),
                      &handle_Rectifier_BaselineStub);

        
        
        masm.loadPtr(Address(scratch2, RectifierFrameLayout::offsetOfReturnAddress()), scratch3);
        masm.storePtr(scratch3, lastProfilingCallSite);

        
        masm.ma_addu(scratch3, scratch2, scratch1);
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
        masm.ma_addu(scratch3, scratch2, scratch1);
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
        
        masm.ma_addu(scratch2, StackPointer, scratch1);
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

        
        
        masm.ma_addu(scratch1, scratch2, scratch3);
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
