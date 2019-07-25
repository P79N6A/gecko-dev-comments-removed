








































#include "MacroAssembler-x86.h"
#include "ion/MoveEmitter.h"
#include "ion/IonFrames.h"

using namespace js;
using namespace js::ion;

uint32
MacroAssemblerX86::setupABICall(uint32 args)
{
    JS_ASSERT(!inCall_);
    inCall_ = true;

    uint32 stackForArgs = args > NumArgRegs
                          ? (args - NumArgRegs) * sizeof(void *)
                          : 0;
    return stackForArgs;
}

static const uint32 StackAlignment = 16;

void
MacroAssemblerX86::setupAlignedABICall(uint32 args)
{
    
    
    
    uint32 stackForCall = setupABICall(args);
    uint32 displacement = stackForCall + framePushed_;
    stackAdjust_ = stackForCall + ComputeByteAlignment(displacement, StackAlignment);
    dynamicAlignment_ = false;
    reserveStack(stackAdjust_);
}

void
MacroAssemblerX86::setupUnalignedABICall(uint32 args, const Register &scratch)
{
    uint32 stackForCall = setupABICall(args);

    
    
    
    
    movl(esp, scratch);
    andl(Imm32(~(StackAlignment - 1)), esp);
    push(scratch);

    uint32 displacement = stackForCall + STACK_SLOT_SIZE;
    stackAdjust_ = stackForCall + ComputeByteAlignment(displacement, StackAlignment);
    dynamicAlignment_ = true;
    reserveStack(stackAdjust_);
}

void
MacroAssemblerX86::setABIArg(uint32 arg, const MoveOperand &from)
{
    uint32 disp = GetArgStackDisp(arg);
    MoveOperand to = MoveOperand(StackPointer, disp);
    enoughMemory_ &= moveResolver_.addMove(from, to, Move::GENERAL);
}

void
MacroAssemblerX86::setABIArg(uint32 arg, const Register &reg)
{
    setABIArg(arg, MoveOperand(reg));
}

void
MacroAssemblerX86::callWithABI(void *fun)
{
    JS_ASSERT(inCall_);

    
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
        movl(esp, eax);
        testl(eax, Imm32(StackAlignment - 1));
        j(Equal, &good);
        breakpoint();
        bind(&good);
    }
#endif

    call(fun);

    freeStack(stackAdjust_);
    if (dynamicAlignment_)
        pop(esp);

    JS_ASSERT(inCall_);
    inCall_ = false;
}

void
MacroAssemblerX86::handleException()
{
    
    subl(Imm32(sizeof(ResumeFromException)), esp);
    movl(esp, eax);

    
    setupUnalignedABICall(1, ecx);
    setABIArg(0, eax);
    callWithABI(JS_FUNC_TO_DATA_PTR(void *, ion::HandleException));
    
    
    moveValue(MagicValue(JS_ION_ERROR), JSReturnOperand);
    movl(Operand(esp, offsetof(ResumeFromException, stackPointer)), esp);
    ret();
}

