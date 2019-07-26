





#include "ion/x86/MacroAssembler-x86.h"

#include "mozilla/Casting.h"

#include "ion/Bailouts.h"
#include "ion/BaselineFrame.h"
#include "ion/IonFrames.h"
#include "ion/MoveEmitter.h"

#include "jsscriptinlines.h"

using namespace js;
using namespace js::ion;

void
MacroAssemblerX86::loadConstantDouble(double d, const FloatRegister &dest)
{
    if (maybeInlineDouble(d, dest))
        return;

    if (!doubleMap_.initialized()) {
        enoughMemory_ &= doubleMap_.init();
        if (!enoughMemory_)
            return;
    }
    size_t doubleIndex;
    DoubleMap::AddPtr p = doubleMap_.lookupForAdd(d);
    if (p) {
        doubleIndex = p->value;
    } else {
        doubleIndex = doubles_.length();
        enoughMemory_ &= doubles_.append(Double(d));
        enoughMemory_ &= doubleMap_.add(p, d, doubleIndex);
        if (!enoughMemory_)
            return;
    }
    Double &dbl = doubles_[doubleIndex];
    JS_ASSERT(!dbl.uses.bound());

    masm.movsd_mr(reinterpret_cast<const void *>(dbl.uses.prev()), dest.code());
    dbl.uses.setPrev(masm.size());
}

void
MacroAssemblerX86::loadStaticDouble(const double *dp, const FloatRegister &dest) {
    if (maybeInlineDouble(*dp, dest))
        return;

    
    movsd(dp, dest);
}

void
MacroAssemblerX86::finish()
{
    if (doubles_.empty())
        return;

    masm.align(sizeof(double));
    for (size_t i = 0; i < doubles_.length(); i++) {
        CodeLabel cl(doubles_[i].uses);
        writeDoubleConstant(doubles_[i].value, cl.src());
        enoughMemory_ &= addCodeLabel(cl);
        if (!enoughMemory_)
            return;
    }
}

void
MacroAssemblerX86::setupABICall(uint32_t args)
{
    JS_ASSERT(!inCall_);
    inCall_ = true;

    args_ = args;
    passedArgs_ = 0;
    stackForCall_ = 0;
}

void
MacroAssemblerX86::setupAlignedABICall(uint32_t args)
{
    setupABICall(args);
    dynamicAlignment_ = false;
}

void
MacroAssemblerX86::setupUnalignedABICall(uint32_t args, const Register &scratch)
{
    setupABICall(args);
    dynamicAlignment_ = true;

    movl(esp, scratch);
    andl(Imm32(~(StackAlignment - 1)), esp);
    push(scratch);
}

void
MacroAssemblerX86::passABIArg(const MoveOperand &from)
{
    ++passedArgs_;
    MoveOperand to = MoveOperand(StackPointer, stackForCall_);
    if (from.isDouble()) {
        stackForCall_ += sizeof(double);
        enoughMemory_ &= moveResolver_.addMove(from, to, Move::DOUBLE);
    } else {
        stackForCall_ += sizeof(int32_t);
        enoughMemory_ &= moveResolver_.addMove(from, to, Move::GENERAL);
    }
}

void
MacroAssemblerX86::passABIArg(const Register &reg)
{
    passABIArg(MoveOperand(reg));
}

void
MacroAssemblerX86::passABIArg(const FloatRegister &reg)
{
    passABIArg(MoveOperand(reg));
}

void
MacroAssemblerX86::callWithABIPre(uint32_t *stackAdjust)
{
    JS_ASSERT(inCall_);
    JS_ASSERT(args_ == passedArgs_);

    if (dynamicAlignment_) {
        *stackAdjust = stackForCall_
                     + ComputeByteAlignment(stackForCall_ + STACK_SLOT_SIZE,
                                            StackAlignment);
    } else {
        *stackAdjust = stackForCall_
                     + ComputeByteAlignment(stackForCall_ + framePushed_,
                                            StackAlignment);
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
        testl(esp, Imm32(StackAlignment - 1));
        j(Equal, &good);
        breakpoint();
        bind(&good);
    }
#endif
}

void
MacroAssemblerX86::callWithABIPost(uint32_t stackAdjust, Result result)
{
    freeStack(stackAdjust);
    if (result == DOUBLE) {
        reserveStack(sizeof(double));
        fstp(Operand(esp, 0));
        movsd(Operand(esp, 0), ReturnFloatReg);
        freeStack(sizeof(double));
    }
    if (dynamicAlignment_)
        pop(esp);

    JS_ASSERT(inCall_);
    inCall_ = false;
}

void
MacroAssemblerX86::callWithABI(void *fun, Result result)
{
    uint32_t stackAdjust;
    callWithABIPre(&stackAdjust);
    call(ImmWord(fun));
    callWithABIPost(stackAdjust, result);
}

void
MacroAssemblerX86::callWithABI(const Address &fun, Result result)
{
    uint32_t stackAdjust;
    callWithABIPre(&stackAdjust);
    call(Operand(fun));
    callWithABIPost(stackAdjust, result);
}

void
MacroAssemblerX86::handleFailureWithHandler(void *handler)
{
    
    subl(Imm32(sizeof(ResumeFromException)), esp);
    movl(esp, eax);

    
    setupUnalignedABICall(1, ecx);
    passABIArg(eax);
    callWithABI(handler);

    IonCode *excTail = GetIonContext()->runtime->ionRuntime()->getExceptionTail();
    jmp(excTail);
}

void
MacroAssemblerX86::handleFailureWithHandlerTail()
{
    Label entryFrame;
    Label catch_;
    Label finally;
    Label return_;
    Label bailout;

    loadPtr(Address(esp, offsetof(ResumeFromException, kind)), eax);
    branch32(Assembler::Equal, eax, Imm32(ResumeFromException::RESUME_ENTRY_FRAME), &entryFrame);
    branch32(Assembler::Equal, eax, Imm32(ResumeFromException::RESUME_CATCH), &catch_);
    branch32(Assembler::Equal, eax, Imm32(ResumeFromException::RESUME_FINALLY), &finally);
    branch32(Assembler::Equal, eax, Imm32(ResumeFromException::RESUME_FORCED_RETURN), &return_);
    branch32(Assembler::Equal, eax, Imm32(ResumeFromException::RESUME_BAILOUT), &bailout);

    breakpoint(); 

    
    
    bind(&entryFrame);
    moveValue(MagicValue(JS_ION_ERROR), JSReturnOperand);
    movl(Operand(esp, offsetof(ResumeFromException, stackPointer)), esp);
    ret();

    
    
    bind(&catch_);
    movl(Operand(esp, offsetof(ResumeFromException, target)), eax);
    movl(Operand(esp, offsetof(ResumeFromException, framePointer)), ebp);
    movl(Operand(esp, offsetof(ResumeFromException, stackPointer)), esp);
    jmp(Operand(eax));

    
    
    
    bind(&finally);
    ValueOperand exception = ValueOperand(ecx, edx);
    loadValue(Operand(esp, offsetof(ResumeFromException, exception)), exception);

    movl(Operand(esp, offsetof(ResumeFromException, target)), eax);
    movl(Operand(esp, offsetof(ResumeFromException, framePointer)), ebp);
    movl(Operand(esp, offsetof(ResumeFromException, stackPointer)), esp);

    pushValue(BooleanValue(true));
    pushValue(exception);
    jmp(Operand(eax));

    
    bind(&return_);
    movl(Operand(esp, offsetof(ResumeFromException, framePointer)), ebp);
    movl(Operand(esp, offsetof(ResumeFromException, stackPointer)), esp);
    loadValue(Address(ebp, BaselineFrame::reverseOffsetOfReturnValue()), JSReturnOperand);
    movl(ebp, esp);
    pop(ebp);
    ret();

    
    
    bind(&bailout);
    movl(Operand(esp, offsetof(ResumeFromException, bailoutInfo)), ecx);
    movl(Imm32(BAILOUT_RETURN_OK), eax);
    jmp(Operand(esp, offsetof(ResumeFromException, target)));
}

void
MacroAssemblerX86::branchTestValue(Condition cond, const ValueOperand &value, const Value &v, Label *label)
{
    jsval_layout jv = JSVAL_TO_IMPL(v);
    if (v.isMarkable())
        cmpl(value.payloadReg(), ImmGCPtr(reinterpret_cast<gc::Cell *>(v.toGCThing())));
    else
        cmpl(value.payloadReg(), Imm32(jv.s.payload.i32));

    if (cond == Equal) {
        Label done;
        j(NotEqual, &done);
        {
            cmpl(value.typeReg(), Imm32(jv.s.tag));
            j(Equal, label);
        }
        bind(&done);
    } else {
        JS_ASSERT(cond == NotEqual);
        j(NotEqual, label);

        cmpl(value.typeReg(), Imm32(jv.s.tag));
        j(NotEqual, label);
    }
}

Assembler::Condition
MacroAssemblerX86::testNegativeZero(const FloatRegister &reg, const Register &scratch)
{
    
    

    Label nonZero;

    
    xorpd(ScratchFloatReg, ScratchFloatReg);
    
    branchDouble(DoubleNotEqual, reg, ScratchFloatReg, &nonZero);

    
    movmskpd(reg, scratch);
    
    cmpl(scratch, Imm32(1));

    bind(&nonZero);
    return Zero;
}
