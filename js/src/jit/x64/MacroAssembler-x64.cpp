





#include "jit/x64/MacroAssembler-x64.h"

#include "jit/Bailouts.h"
#include "jit/BaselineFrame.h"
#include "jit/JitCompartment.h"
#include "jit/JitFrames.h"
#include "jit/MoveEmitter.h"

using namespace js;
using namespace js::jit;

void
MacroAssemblerX64::loadConstantDouble(double d, FloatRegister dest)
{
    if (maybeInlineDouble(d, dest))
        return;

    if (!doubleMap_.initialized()) {
        enoughMemory_ &= doubleMap_.init();
        if (!enoughMemory_)
            return;
    }
    size_t doubleIndex;
    if (DoubleMap::AddPtr p = doubleMap_.lookupForAdd(d)) {
        doubleIndex = p->value();
    } else {
        doubleIndex = doubles_.length();
        enoughMemory_ &= doubles_.append(Double(d));
        enoughMemory_ &= doubleMap_.add(p, d, doubleIndex);
        if (!enoughMemory_)
            return;
    }
    Double &dbl = doubles_[doubleIndex];
    MOZ_ASSERT(!dbl.uses.bound());

    
    
    
    
    
    JmpSrc j = masm.movsd_ripr(dest.code());
    JmpSrc prev = JmpSrc(dbl.uses.use(j.offset()));
    masm.setNextJump(j, prev);
}

void
MacroAssemblerX64::loadConstantFloat32(float f, FloatRegister dest)
{
    if (maybeInlineFloat(f, dest))
        return;

    if (!floatMap_.initialized()) {
        enoughMemory_ &= floatMap_.init();
        if (!enoughMemory_)
            return;
    }
    size_t floatIndex;
    if (FloatMap::AddPtr p = floatMap_.lookupForAdd(f)) {
        floatIndex = p->value();
    } else {
        floatIndex = floats_.length();
        enoughMemory_ &= floats_.append(Float(f));
        enoughMemory_ &= floatMap_.add(p, f, floatIndex);
        if (!enoughMemory_)
            return;
    }
    Float &flt = floats_[floatIndex];
    MOZ_ASSERT(!flt.uses.bound());

    
    JmpSrc j = masm.movss_ripr(dest.code());
    JmpSrc prev = JmpSrc(flt.uses.use(j.offset()));
    masm.setNextJump(j, prev);
}

MacroAssemblerX64::SimdData *
MacroAssemblerX64::getSimdData(const SimdConstant &v)
{
    if (!simdMap_.initialized()) {
        enoughMemory_ &= simdMap_.init();
        if (!enoughMemory_)
            return nullptr;
    }

    size_t index;
    if (SimdMap::AddPtr p = simdMap_.lookupForAdd(v)) {
        index = p->value();
    } else {
        index = simds_.length();
        enoughMemory_ &= simds_.append(SimdData(v));
        enoughMemory_ &= simdMap_.add(p, v, index);
        if (!enoughMemory_)
            return nullptr;
    }
    return &simds_[index];
}

void
MacroAssemblerX64::loadConstantInt32x4(const SimdConstant &v, FloatRegister dest)
{
    MOZ_ASSERT(v.type() == SimdConstant::Int32x4);
    if (maybeInlineInt32x4(v, dest))
        return;

    SimdData *val = getSimdData(v);
    if (!val)
        return;

    MOZ_ASSERT(!val->uses.bound());
    MOZ_ASSERT(val->type() == SimdConstant::Int32x4);

    JmpSrc j = masm.movdqa_ripr(dest.code());
    JmpSrc prev = JmpSrc(val->uses.use(j.offset()));
    masm.setNextJump(j, prev);
}

void
MacroAssemblerX64::loadConstantFloat32x4(const SimdConstant&v, FloatRegister dest)
{
    MOZ_ASSERT(v.type() == SimdConstant::Float32x4);
    if (maybeInlineFloat32x4(v, dest))
        return;

    SimdData *val = getSimdData(v);
    if (!val)
        return;

    MOZ_ASSERT(!val->uses.bound());
    MOZ_ASSERT(val->type() == SimdConstant::Float32x4);

    JmpSrc j = masm.movaps_ripr(dest.code());
    JmpSrc prev = JmpSrc(val->uses.use(j.offset()));
    masm.setNextJump(j, prev);
}

void
MacroAssemblerX64::finish()
{
    if (!doubles_.empty())
        masm.align(sizeof(double));
    for (size_t i = 0; i < doubles_.length(); i++) {
        Double &dbl = doubles_[i];
        bind(&dbl.uses);
        masm.doubleConstant(dbl.value);
    }

    if (!floats_.empty())
        masm.align(sizeof(float));
    for (size_t i = 0; i < floats_.length(); i++) {
        Float &flt = floats_[i];
        bind(&flt.uses);
        masm.floatConstant(flt.value);
    }

    
    if (!simds_.empty())
        masm.align(SimdStackAlignment);
    for (size_t i = 0; i < simds_.length(); i++) {
        SimdData &v = simds_[i];
        bind(&v.uses);
        switch(v.type()) {
          case SimdConstant::Int32x4:   masm.int32x4Constant(v.value.asInt32x4());     break;
          case SimdConstant::Float32x4: masm.float32x4Constant(v.value.asFloat32x4()); break;
          default: MOZ_CRASH("unexpected SimdConstant type");
        }
    }

    MacroAssemblerX86Shared::finish();
}

void
MacroAssemblerX64::setupABICall(uint32_t args)
{
    MOZ_ASSERT(!inCall_);
    inCall_ = true;

    args_ = args;
    passedIntArgs_ = 0;
    passedFloatArgs_ = 0;
    stackForCall_ = ShadowStackSpace;
}

void
MacroAssemblerX64::setupAlignedABICall(uint32_t args)
{
    setupABICall(args);
    dynamicAlignment_ = false;
}

void
MacroAssemblerX64::setupUnalignedABICall(uint32_t args, Register scratch)
{
    setupABICall(args);
    dynamicAlignment_ = true;

    movq(rsp, scratch);
    andq(Imm32(~(ABIStackAlignment - 1)), rsp);
    push(scratch);
}

void
MacroAssemblerX64::passABIArg(const MoveOperand &from, MoveOp::Type type)
{
    MoveOperand to;
    switch (type) {
      case MoveOp::FLOAT32:
      case MoveOp::DOUBLE: {
        FloatRegister dest;
        if (GetFloatArgReg(passedIntArgs_, passedFloatArgs_++, &dest)) {
            if (from.isFloatReg() && from.floatReg() == dest) {
                
                return;
            }
            to = MoveOperand(dest);
        } else {
            to = MoveOperand(StackPointer, stackForCall_);
            switch (type) {
              case MoveOp::FLOAT32: stackForCall_ += sizeof(float);  break;
              case MoveOp::DOUBLE:  stackForCall_ += sizeof(double); break;
              default: MOZ_CRASH("Unexpected float register class argument type");
            }
        }
        break;
      }
      case MoveOp::GENERAL: {
        Register dest;
        if (GetIntArgReg(passedIntArgs_++, passedFloatArgs_, &dest)) {
            if (from.isGeneralReg() && from.reg() == dest) {
                
                return;
            }
            to = MoveOperand(dest);
        } else {
            to = MoveOperand(StackPointer, stackForCall_);
            stackForCall_ += sizeof(int64_t);
        }
        break;
      }
      default:
        MOZ_CRASH("Unexpected argument type");
    }

    enoughMemory_ = moveResolver_.addMove(from, to, type);
}

void
MacroAssemblerX64::passABIArg(Register reg)
{
    passABIArg(MoveOperand(reg), MoveOp::GENERAL);
}

void
MacroAssemblerX64::passABIArg(FloatRegister reg, MoveOp::Type type)
{
    passABIArg(MoveOperand(reg), type);
}

void
MacroAssemblerX64::callWithABIPre(uint32_t *stackAdjust)
{
    MOZ_ASSERT(inCall_);
    MOZ_ASSERT(args_ == passedIntArgs_ + passedFloatArgs_);

    if (dynamicAlignment_) {
        *stackAdjust = stackForCall_
                     + ComputeByteAlignment(stackForCall_ + sizeof(intptr_t),
                                            ABIStackAlignment);
    } else {
        *stackAdjust = stackForCall_
                     + ComputeByteAlignment(stackForCall_ + framePushed_,
                                            ABIStackAlignment);
    }

    reserveStack(*stackAdjust);

    
    {
        enoughMemory_ &= moveResolver_.resolve();
        if (!enoughMemory_)
            return;

        MoveEmitter emitter(*this);
        emitter.emit(moveResolver_);
        emitter.finish();
    }

#ifdef DEBUG
    {
        Label good;
        testq(rsp, Imm32(ABIStackAlignment - 1));
        j(Equal, &good);
        breakpoint();
        bind(&good);
    }
#endif
}

void
MacroAssemblerX64::callWithABIPost(uint32_t stackAdjust, MoveOp::Type result)
{
    freeStack(stackAdjust);
    if (dynamicAlignment_)
        pop(rsp);

    MOZ_ASSERT(inCall_);
    inCall_ = false;
}

void
MacroAssemblerX64::callWithABI(void *fun, MoveOp::Type result)
{
    uint32_t stackAdjust;
    callWithABIPre(&stackAdjust);
    call(ImmPtr(fun));
    callWithABIPost(stackAdjust, result);
}

void
MacroAssemblerX64::callWithABI(AsmJSImmPtr imm, MoveOp::Type result)
{
    uint32_t stackAdjust;
    callWithABIPre(&stackAdjust);
    call(imm);
    callWithABIPost(stackAdjust, result);
}

static bool
IsIntArgReg(Register reg)
{
    for (uint32_t i = 0; i < NumIntArgRegs; i++) {
        if (IntArgRegs[i] == reg)
            return true;
    }

    return false;
}

void
MacroAssemblerX64::callWithABI(Address fun, MoveOp::Type result)
{
    if (IsIntArgReg(fun.base)) {
        
        
        moveResolver_.addMove(MoveOperand(fun.base), MoveOperand(r10), MoveOp::GENERAL);
        fun.base = r10;
    }

    MOZ_ASSERT(!IsIntArgReg(fun.base));

    uint32_t stackAdjust;
    callWithABIPre(&stackAdjust);
    call(Operand(fun));
    callWithABIPost(stackAdjust, result);
}

void
MacroAssemblerX64::callWithABI(Register fun, MoveOp::Type result)
{
    if (IsIntArgReg(fun)) {
        
        
        moveResolver_.addMove(MoveOperand(fun), MoveOperand(r10), MoveOp::GENERAL);
        fun = r10;
    }

    MOZ_ASSERT(!IsIntArgReg(fun));

    uint32_t stackAdjust;
    callWithABIPre(&stackAdjust);
    call(Operand(fun));
    callWithABIPost(stackAdjust, result);
}

void
MacroAssemblerX64::handleFailureWithHandler(void *handler)
{
    
    subq(Imm32(sizeof(ResumeFromException)), rsp);
    movq(rsp, rax);

    
    setupUnalignedABICall(1, rcx);
    passABIArg(rax);
    callWithABI(handler);

    JitCode *excTail = GetJitContext()->runtime->jitRuntime()->getExceptionTail();
    jmp(excTail);
}

void
MacroAssemblerX64::handleFailureWithHandlerTail()
{
    Label entryFrame;
    Label catch_;
    Label finally;
    Label return_;
    Label bailout;

    loadPtr(Address(rsp, offsetof(ResumeFromException, kind)), rax);
    branch32(Assembler::Equal, rax, Imm32(ResumeFromException::RESUME_ENTRY_FRAME), &entryFrame);
    branch32(Assembler::Equal, rax, Imm32(ResumeFromException::RESUME_CATCH), &catch_);
    branch32(Assembler::Equal, rax, Imm32(ResumeFromException::RESUME_FINALLY), &finally);
    branch32(Assembler::Equal, rax, Imm32(ResumeFromException::RESUME_FORCED_RETURN), &return_);
    branch32(Assembler::Equal, rax, Imm32(ResumeFromException::RESUME_BAILOUT), &bailout);

    breakpoint(); 

    
    
    bind(&entryFrame);
    moveValue(MagicValue(JS_ION_ERROR), JSReturnOperand);
    loadPtr(Address(rsp, offsetof(ResumeFromException, stackPointer)), rsp);
    ret();

    
    
    bind(&catch_);
    loadPtr(Address(rsp, offsetof(ResumeFromException, target)), rax);
    loadPtr(Address(rsp, offsetof(ResumeFromException, framePointer)), rbp);
    loadPtr(Address(rsp, offsetof(ResumeFromException, stackPointer)), rsp);
    jmp(Operand(rax));

    
    
    
    bind(&finally);
    ValueOperand exception = ValueOperand(rcx);
    loadValue(Address(esp, offsetof(ResumeFromException, exception)), exception);

    loadPtr(Address(rsp, offsetof(ResumeFromException, target)), rax);
    loadPtr(Address(rsp, offsetof(ResumeFromException, framePointer)), rbp);
    loadPtr(Address(rsp, offsetof(ResumeFromException, stackPointer)), rsp);

    pushValue(BooleanValue(true));
    pushValue(exception);
    jmp(Operand(rax));

    
    bind(&return_);
    loadPtr(Address(rsp, offsetof(ResumeFromException, framePointer)), rbp);
    loadPtr(Address(rsp, offsetof(ResumeFromException, stackPointer)), rsp);
    loadValue(Address(rbp, BaselineFrame::reverseOffsetOfReturnValue()), JSReturnOperand);
    movq(rbp, rsp);
    pop(rbp);
    ret();

    
    
    bind(&bailout);
    loadPtr(Address(esp, offsetof(ResumeFromException, bailoutInfo)), r9);
    mov(ImmWord(BAILOUT_RETURN_OK), rax);
    jmp(Operand(rsp, offsetof(ResumeFromException, target)));
}

template <typename T>
void
MacroAssemblerX64::storeUnboxedValue(ConstantOrRegister value, MIRType valueType, const T &dest,
                                     MIRType slotType)
{
    if (valueType == MIRType_Double) {
        storeDouble(value.reg().typedReg().fpu(), dest);
        return;
    }

    
    
    if ((valueType == MIRType_Int32 || valueType == MIRType_Boolean) && slotType == valueType) {
        if (value.constant()) {
            Value val = value.value();
            if (valueType == MIRType_Int32)
                store32(Imm32(val.toInt32()), dest);
            else
                store32(Imm32(val.toBoolean() ? 1 : 0), dest);
        } else {
            store32(value.reg().typedReg().gpr(), dest);
        }
        return;
    }

    if (value.constant())
        storeValue(value.value(), dest);
    else
        storeValue(ValueTypeFromMIRType(valueType), value.reg().typedReg().gpr(), dest);
}

template void
MacroAssemblerX64::storeUnboxedValue(ConstantOrRegister value, MIRType valueType, const Address &dest,
                                     MIRType slotType);

template void
MacroAssemblerX64::storeUnboxedValue(ConstantOrRegister value, MIRType valueType, const BaseIndex &dest,
                                     MIRType slotType);

#ifdef JSGC_GENERATIONAL

void
MacroAssemblerX64::branchPtrInNurseryRange(Condition cond, Register ptr, Register temp, Label *label)
{
    MOZ_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);
    MOZ_ASSERT(ptr != temp);
    MOZ_ASSERT(ptr != ScratchReg);

    const Nursery &nursery = GetJitContext()->runtime->gcNursery();
    movePtr(ImmWord(-ptrdiff_t(nursery.start())), ScratchReg);
    addPtr(ptr, ScratchReg);
    branchPtr(cond == Assembler::Equal ? Assembler::Below : Assembler::AboveOrEqual,
              ScratchReg, Imm32(nursery.nurserySize()), label);
}

void
MacroAssemblerX64::branchValueIsNurseryObject(Condition cond, ValueOperand value, Register temp,
                                              Label *label)
{
    MOZ_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);

    
    const Nursery &nursery = GetJitContext()->runtime->gcNursery();
    Value start = ObjectValue(*reinterpret_cast<JSObject *>(nursery.start()));

    movePtr(ImmWord(-ptrdiff_t(start.asRawBits())), ScratchReg);
    addPtr(value.valueReg(), ScratchReg);
    branchPtr(cond == Assembler::Equal ? Assembler::Below : Assembler::AboveOrEqual,
              ScratchReg, Imm32(nursery.nurserySize()), label);
}

#endif
