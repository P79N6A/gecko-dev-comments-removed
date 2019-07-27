





#include "jit/arm64/MacroAssembler-arm64.h"

#include "jit/arm64/MoveEmitter-arm64.h"
#include "jit/arm64/SharedICRegisters-arm64.h"
#include "jit/Bailouts.h"
#include "jit/BaselineFrame.h"
#include "jit/MacroAssembler.h"

namespace js {
namespace jit {

void
MacroAssembler::clampDoubleToUint8(FloatRegister input, Register output)
{
    ARMRegister dest(output, 32);
    Fcvtns(dest, ARMFPRegister(input, 64));

    {
        vixl::UseScratchRegisterScope temps(this);
        const ARMRegister scratch32 = temps.AcquireW();

        Mov(scratch32, Operand(0xff));
        Cmp(dest, scratch32);
        Csel(dest, dest, scratch32, LessThan);
    }

    Cmp(dest, Operand(0));
    Csel(dest, dest, wzr, GreaterThan);
}

void
MacroAssemblerCompat::buildFakeExitFrame(Register scratch, uint32_t* offset)
{
    mozilla::DebugOnly<uint32_t> initialDepth = framePushed();
    uint32_t descriptor = MakeFrameDescriptor(framePushed(), JitFrame_IonJS);

    asMasm().Push(Imm32(descriptor)); 

    enterNoPool(3);
    Label fakeCallsite;
    Adr(ARMRegister(scratch, 64), &fakeCallsite);
    asMasm().Push(scratch);
    bind(&fakeCallsite);
    uint32_t pseudoReturnOffset = currentOffset();
    leaveNoPool();

    MOZ_ASSERT(framePushed() == initialDepth + ExitFrameLayout::Size());

    *offset = pseudoReturnOffset;
}

void
MacroAssemblerCompat::callWithExitFrame(JitCode* target)
{
    uint32_t descriptor = MakeFrameDescriptor(framePushed(), JitFrame_IonJS);
    asMasm().Push(Imm32(descriptor));
    call(target);
}

void
MacroAssembler::alignFrameForICArguments(MacroAssembler::AfterICSaveLive& aic)
{
    
}

void
MacroAssembler::restoreFrameAlignmentForICArguments(MacroAssembler::AfterICSaveLive& aic)
{
    
}

js::jit::MacroAssembler&
MacroAssemblerCompat::asMasm()
{
    return *static_cast<js::jit::MacroAssembler*>(this);
}

const js::jit::MacroAssembler&
MacroAssemblerCompat::asMasm() const
{
    return *static_cast<const js::jit::MacroAssembler*>(this);
}

vixl::MacroAssembler&
MacroAssemblerCompat::asVIXL()
{
    return *static_cast<vixl::MacroAssembler*>(this);
}

const vixl::MacroAssembler&
MacroAssemblerCompat::asVIXL() const
{
    return *static_cast<const vixl::MacroAssembler*>(this);
}

BufferOffset
MacroAssemblerCompat::movePatchablePtr(ImmPtr ptr, Register dest)
{
    const size_t numInst = 1; 
    const unsigned numPoolEntries = 2; 
    uint8_t* literalAddr = (uint8_t*)(&ptr.value); 

    
    
    
    
    
    
    
    uint32_t instructionScratch = 0;

    
    
    vixl::Assembler::ldr((Instruction*)&instructionScratch, ARMRegister(dest, 64), 0);

    
    
    return armbuffer_.allocEntry(numInst, numPoolEntries,
                                 (uint8_t*)&instructionScratch, literalAddr);
}

BufferOffset
MacroAssemblerCompat::movePatchablePtr(ImmWord ptr, Register dest)
{
    const size_t numInst = 1; 
    const unsigned numPoolEntries = 2; 
    uint8_t* literalAddr = (uint8_t*)(&ptr.value);

    
    
    
    
    
    
    
    uint32_t instructionScratch = 0;

    
    
    vixl::Assembler::ldr((Instruction*)&instructionScratch, ARMRegister(dest, 64), 0);

    
    
    return armbuffer_.allocEntry(numInst, numPoolEntries,
                                 (uint8_t*)&instructionScratch, literalAddr);
}

void
MacroAssemblerCompat::handleFailureWithHandlerTail(void* handler)
{
    
    int64_t size = (sizeof(ResumeFromException) + 7) & ~7;
    Sub(GetStackPointer64(), GetStackPointer64(), Operand(size));
    if (!GetStackPointer64().Is(sp))
        Mov(sp, GetStackPointer64());

    Mov(x0, GetStackPointer64());

    
    setupUnalignedABICall(1, r1);
    passABIArg(r0);
    callWithABI(handler);

    Label entryFrame;
    Label catch_;
    Label finally;
    Label return_;
    Label bailout;

    MOZ_ASSERT(GetStackPointer64().Is(x28)); 

    loadPtr(Address(r28, offsetof(ResumeFromException, kind)), r0);
    branch32(Assembler::Equal, r0, Imm32(ResumeFromException::RESUME_ENTRY_FRAME), &entryFrame);
    branch32(Assembler::Equal, r0, Imm32(ResumeFromException::RESUME_CATCH), &catch_);
    branch32(Assembler::Equal, r0, Imm32(ResumeFromException::RESUME_FINALLY), &finally);
    branch32(Assembler::Equal, r0, Imm32(ResumeFromException::RESUME_FORCED_RETURN), &return_);
    branch32(Assembler::Equal, r0, Imm32(ResumeFromException::RESUME_BAILOUT), &bailout);

    breakpoint(); 

    
    
    bind(&entryFrame);
    moveValue(MagicValue(JS_ION_ERROR), JSReturnOperand);
    loadPtr(Address(r28, offsetof(ResumeFromException, stackPointer)), r28);
    retn(Imm32(1 * sizeof(void*))); 

    
    
    bind(&catch_);
    loadPtr(Address(r28, offsetof(ResumeFromException, target)), r0);
    loadPtr(Address(r28, offsetof(ResumeFromException, framePointer)), BaselineFrameReg);
    loadPtr(Address(r28, offsetof(ResumeFromException, stackPointer)), r28);
    syncStackPtr();
    Br(x0);

    
    
    
    bind(&finally);
    ARMRegister exception = x1;
    Ldr(exception, MemOperand(GetStackPointer64(), offsetof(ResumeFromException, exception)));
    Ldr(x0, MemOperand(GetStackPointer64(), offsetof(ResumeFromException, target)));
    Ldr(ARMRegister(BaselineFrameReg, 64),
        MemOperand(GetStackPointer64(), offsetof(ResumeFromException, framePointer)));
    Ldr(GetStackPointer64(), MemOperand(GetStackPointer64(), offsetof(ResumeFromException, stackPointer)));
    syncStackPtr();
    pushValue(BooleanValue(true));
    push(exception);
    Br(x0);

    
    bind(&return_);
    loadPtr(Address(r28, offsetof(ResumeFromException, framePointer)), BaselineFrameReg);
    loadPtr(Address(r28, offsetof(ResumeFromException, stackPointer)), r28);
    loadValue(Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfReturnValue()),
              JSReturnOperand);
    movePtr(BaselineFrameReg, r28);
    vixl::MacroAssembler::Pop(ARMRegister(BaselineFrameReg, 64), vixl::lr);
    syncStackPtr();
    vixl::MacroAssembler::Ret(vixl::lr);

    
    
    bind(&bailout);
    Ldr(x2, MemOperand(GetStackPointer64(), offsetof(ResumeFromException, bailoutInfo)));
    Ldr(x1, MemOperand(GetStackPointer64(), offsetof(ResumeFromException, target)));
    Mov(x0, BAILOUT_RETURN_OK);
    Br(x1);
}

void
MacroAssemblerCompat::setupABICall(uint32_t args)
{
    MOZ_ASSERT(!inCall_);
    inCall_ = true;

    args_ = args;
    usedOutParam_ = false;
    passedIntArgs_ = 0;
    passedFloatArgs_ = 0;
    passedArgTypes_ = 0;
    stackForCall_ = ShadowStackSpace;
}

void
MacroAssemblerCompat::setupUnalignedABICall(uint32_t args, Register scratch)
{
    setupABICall(args);
    dynamicAlignment_ = true;

    int64_t alignment = ~(int64_t(ABIStackAlignment) - 1);
    ARMRegister scratch64(scratch, 64);

    
    push(lr);

    
    MOZ_ASSERT(!GetStackPointer64().Is(sp));

    
    Mov(scratch64, GetStackPointer64());

    
    Sub(GetStackPointer64(), GetStackPointer64(), Operand(8));
    And(GetStackPointer64(), GetStackPointer64(), Operand(alignment));

    
    syncStackPtr();

    
    Str(scratch64, MemOperand(GetStackPointer64(), 0));
}

void
MacroAssemblerCompat::passABIArg(const MoveOperand& from, MoveOp::Type type)
{
    if (!enoughMemory_)
        return;

    Register activeSP = Register::FromCode(GetStackPointer64().code());
    if (type == MoveOp::GENERAL) {
        Register dest;
        passedArgTypes_ = (passedArgTypes_ << ArgType_Shift) | ArgType_General;
        if (GetIntArgReg(passedIntArgs_++, passedFloatArgs_, &dest)) {
            if (!from.isGeneralReg() || from.reg() != dest)
                enoughMemory_ = moveResolver_.addMove(from, MoveOperand(dest), type);
            return;
        }

        enoughMemory_ = moveResolver_.addMove(from, MoveOperand(activeSP, stackForCall_), type);
        stackForCall_ += sizeof(int64_t);
        return;
    }

    MOZ_ASSERT(type == MoveOp::FLOAT32 || type == MoveOp::DOUBLE);
    if (type == MoveOp::FLOAT32)
        passedArgTypes_ = (passedArgTypes_ << ArgType_Shift) | ArgType_Float32;
    else
        passedArgTypes_ = (passedArgTypes_ << ArgType_Shift) | ArgType_Double;

    FloatRegister fdest;
    if (GetFloatArgReg(passedIntArgs_, passedFloatArgs_++, &fdest)) {
        if (!from.isFloatReg() || from.floatReg() != fdest)
            enoughMemory_ = moveResolver_.addMove(from, MoveOperand(fdest), type);
        return;
    }

    enoughMemory_ = moveResolver_.addMove(from, MoveOperand(activeSP, stackForCall_), type);
    switch (type) {
      case MoveOp::FLOAT32: stackForCall_ += sizeof(float);  break;
      case MoveOp::DOUBLE:  stackForCall_ += sizeof(double); break;
      default: MOZ_CRASH("Unexpected float register class argument type");
    }
}

void
MacroAssemblerCompat::passABIArg(Register reg)
{
    passABIArg(MoveOperand(reg), MoveOp::GENERAL);
}

void
MacroAssemblerCompat::passABIArg(FloatRegister reg, MoveOp::Type type)
{
    passABIArg(MoveOperand(reg), type);
}
void
MacroAssemblerCompat::passABIOutParam(Register reg)
{
    if (!enoughMemory_)
        return;
    MOZ_ASSERT(!usedOutParam_);
    usedOutParam_ = true;
    if (reg == r8)
        return;
    enoughMemory_ = moveResolver_.addMove(MoveOperand(reg), MoveOperand(r8), MoveOp::GENERAL);

}

void
MacroAssemblerCompat::callWithABIPre(uint32_t* stackAdjust)
{
    *stackAdjust = stackForCall_;
    
    
    *stackAdjust += ComputeByteAlignment(*stackAdjust, StackAlignment);
    asMasm().reserveStack(*stackAdjust);
    {
        moveResolver_.resolve();
        MoveEmitter emitter(asMasm());
        emitter.emit(moveResolver_);
        emitter.finish();
    }

    
    syncStackPtr();
}

void
MacroAssemblerCompat::callWithABIPost(uint32_t stackAdjust, MoveOp::Type result)
{
    
    if (!GetStackPointer64().Is(sp))
        Mov(GetStackPointer64(), sp);

    inCall_ = false;
    asMasm().freeStack(stackAdjust);

    
    if (dynamicAlignment_)
        Ldr(GetStackPointer64(), MemOperand(GetStackPointer64(), 0));

    
    pop(lr);

    
    
    syncStackPtr();

    
    
}

#if defined(DEBUG) && defined(JS_SIMULATOR_ARM64)
static void
AssertValidABIFunctionType(uint32_t passedArgTypes)
{
    switch (passedArgTypes) {
      case Args_General0:
      case Args_General1:
      case Args_General2:
      case Args_General3:
      case Args_General4:
      case Args_General5:
      case Args_General6:
      case Args_General7:
      case Args_General8:
      case Args_Double_None:
      case Args_Int_Double:
      case Args_Float32_Float32:
      case Args_Double_Double:
      case Args_Double_Int:
      case Args_Double_DoubleInt:
      case Args_Double_DoubleDouble:
      case Args_Double_DoubleDoubleDouble:
      case Args_Double_DoubleDoubleDoubleDouble:
      case Args_Double_IntDouble:
      case Args_Int_IntDouble:
        break;
      default:
        MOZ_CRASH("Unexpected type");
    }
}
#endif 

void
MacroAssemblerCompat::callWithABI(void* fun, MoveOp::Type result)
{
#ifdef JS_SIMULATOR_ARM64
    MOZ_ASSERT(passedIntArgs_ + passedFloatArgs_ <= 15);
    passedArgTypes_ <<= ArgType_Shift;
    switch (result) {
      case MoveOp::GENERAL: passedArgTypes_ |= ArgType_General; break;
      case MoveOp::DOUBLE:  passedArgTypes_ |= ArgType_Double;  break;
      case MoveOp::FLOAT32: passedArgTypes_ |= ArgType_Float32; break;
      default: MOZ_CRASH("Invalid return type");
    }
# ifdef DEBUG
    AssertValidABIFunctionType(passedArgTypes_);
# endif
    ABIFunctionType type = ABIFunctionType(passedArgTypes_);
    fun = vixl::Simulator::RedirectNativeFunction(fun, type);
#endif 

    uint32_t stackAdjust;
    callWithABIPre(&stackAdjust);
    call(ImmPtr(fun));
    callWithABIPost(stackAdjust, result);
}

void
MacroAssemblerCompat::callWithABI(Register fun, MoveOp::Type result)
{
    movePtr(fun, ip0);

    uint32_t stackAdjust;
    callWithABIPre(&stackAdjust);
    call(ip0);
    callWithABIPost(stackAdjust, result);
}

void
MacroAssemblerCompat::callWithABI(AsmJSImmPtr imm, MoveOp::Type result)
{
    uint32_t stackAdjust;
    callWithABIPre(&stackAdjust);
    call(imm);
    callWithABIPost(stackAdjust, result);
}

void
MacroAssemblerCompat::callWithABI(Address fun, MoveOp::Type result)
{
    loadPtr(fun, ip0);

    uint32_t stackAdjust;
    callWithABIPre(&stackAdjust);
    call(ip0);
    callWithABIPost(stackAdjust, result);
}

void
MacroAssemblerCompat::branchPtrInNurseryRange(Condition cond, Register ptr, Register temp,
                                              Label* label)
{
    MOZ_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);
    MOZ_ASSERT(ptr != temp);
    MOZ_ASSERT(ptr != ScratchReg && ptr != ScratchReg2); 
    MOZ_ASSERT(temp != ScratchReg && temp != ScratchReg2);

    const Nursery& nursery = GetJitContext()->runtime->gcNursery();
    movePtr(ImmWord(-ptrdiff_t(nursery.start())), temp);
    addPtr(ptr, temp);
    branchPtr(cond == Assembler::Equal ? Assembler::Below : Assembler::AboveOrEqual,
              temp, ImmWord(nursery.nurserySize()), label);
}

void
MacroAssemblerCompat::branchValueIsNurseryObject(Condition cond, ValueOperand value, Register temp,
                                                 Label* label)
{
    MOZ_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);
    MOZ_ASSERT(temp != ScratchReg && temp != ScratchReg2); 

    
    const Nursery& nursery = GetJitContext()->runtime->gcNursery();
    Value start = ObjectValue(*reinterpret_cast<JSObject*>(nursery.start()));

    movePtr(ImmWord(-ptrdiff_t(start.asRawBits())), temp);
    addPtr(value.valueReg(), temp);
    branchPtr(cond == Assembler::Equal ? Assembler::Below : Assembler::AboveOrEqual,
              temp, ImmWord(nursery.nurserySize()), label);
}

void
MacroAssemblerCompat::callAndPushReturnAddress(Label* label)
{
    
    
    
    Label ret;
    {
        vixl::UseScratchRegisterScope temps(this);
        const ARMRegister scratch64 = temps.AcquireX();

        Adr(scratch64, &ret);
        asMasm().Push(scratch64.asUnsized());
    }

    Bl(label);
    bind(&ret);
}

void
MacroAssemblerCompat::breakpoint()
{
    static int code = 0xA77;
    Brk((code++) & 0xffff);
}




void
MacroAssembler::reserveStack(uint32_t amount)
{
    
    
    vixl::MacroAssembler::Claim(Operand(amount));
    adjustFrame(amount);
}

void
MacroAssembler::PushRegsInMask(LiveRegisterSet set)
{
    for (GeneralRegisterBackwardIterator iter(set.gprs()); iter.more(); ) {
        vixl::CPURegister src[4] = { vixl::NoCPUReg, vixl::NoCPUReg, vixl::NoCPUReg, vixl::NoCPUReg };

        for (size_t i = 0; i < 4 && iter.more(); i++) {
            src[i] = ARMRegister(*iter, 64);
            ++iter;
            adjustFrame(8);
        }
        vixl::MacroAssembler::Push(src[0], src[1], src[2], src[3]);
    }

    for (FloatRegisterBackwardIterator iter(set.fpus().reduceSetForPush()); iter.more(); ) {
        vixl::CPURegister src[4] = { vixl::NoCPUReg, vixl::NoCPUReg, vixl::NoCPUReg, vixl::NoCPUReg };

        for (size_t i = 0; i < 4 && iter.more(); i++) {
            src[i] = ARMFPRegister(*iter, 64);
            ++iter;
            adjustFrame(8);
        }
        vixl::MacroAssembler::Push(src[0], src[1], src[2], src[3]);
    }
}

void
MacroAssembler::PopRegsInMaskIgnore(LiveRegisterSet set, LiveRegisterSet ignore)
{
    
    uint32_t offset = 0;

    for (FloatRegisterIterator iter(set.fpus().reduceSetForPush()); iter.more(); ) {
        vixl::CPURegister dest[2] = { vixl::NoCPUReg, vixl::NoCPUReg };
        uint32_t nextOffset = offset;

        for (size_t i = 0; i < 2 && iter.more(); i++) {
            if (!ignore.has(*iter))
                dest[i] = ARMFPRegister(*iter, 64);
            ++iter;
            nextOffset += sizeof(double);
        }

        if (!dest[0].IsNone() && !dest[1].IsNone())
            Ldp(dest[0], dest[1], MemOperand(GetStackPointer64(), offset));
        else if (!dest[0].IsNone())
            Ldr(dest[0], MemOperand(GetStackPointer64(), offset));
        else if (!dest[1].IsNone())
            Ldr(dest[1], MemOperand(GetStackPointer64(), offset + sizeof(double)));

        offset = nextOffset;
    }

    MOZ_ASSERT(offset == set.fpus().getPushSizeInBytes());

    for (GeneralRegisterIterator iter(set.gprs()); iter.more(); ) {
        vixl::CPURegister dest[2] = { vixl::NoCPUReg, vixl::NoCPUReg };
        uint32_t nextOffset = offset;

        for (size_t i = 0; i < 2 && iter.more(); i++) {
            if (!ignore.has(*iter))
                dest[i] = ARMRegister(*iter, 64);
            ++iter;
            nextOffset += sizeof(uint64_t);
        }

        if (!dest[0].IsNone() && !dest[1].IsNone())
            Ldp(dest[0], dest[1], MemOperand(GetStackPointer64(), offset));
        else if (!dest[0].IsNone())
            Ldr(dest[0], MemOperand(GetStackPointer64(), offset));
        else if (!dest[1].IsNone())
            Ldr(dest[1], MemOperand(GetStackPointer64(), offset + sizeof(uint64_t)));

        offset = nextOffset;
    }

    size_t bytesPushed = set.gprs().size() * sizeof(uint64_t) + set.fpus().getPushSizeInBytes();
    MOZ_ASSERT(offset == bytesPushed);
    freeStack(bytesPushed);
}

void
MacroAssembler::Push(Register reg)
{
    push(reg);
    adjustFrame(sizeof(intptr_t));
}

void
MacroAssembler::Push(const Imm32 imm)
{
    push(imm);
    adjustFrame(sizeof(intptr_t));
}

void
MacroAssembler::Push(const ImmWord imm)
{
    push(imm);
    adjustFrame(sizeof(intptr_t));
}

void
MacroAssembler::Push(const ImmPtr imm)
{
    push(imm);
    adjustFrame(sizeof(intptr_t));
}

void
MacroAssembler::Push(const ImmGCPtr ptr)
{
    push(ptr);
    adjustFrame(sizeof(intptr_t));
}

void
MacroAssembler::Push(FloatRegister f)
{
    push(f);
    adjustFrame(sizeof(double));
}

void
MacroAssembler::Pop(const Register reg)
{
    pop(reg);
    adjustFrame(-1 * int64_t(sizeof(int64_t)));
}

void
MacroAssembler::Pop(const ValueOperand& val)
{
    pop(val);
    adjustFrame(-1 * int64_t(sizeof(int64_t)));
}

} 
} 
