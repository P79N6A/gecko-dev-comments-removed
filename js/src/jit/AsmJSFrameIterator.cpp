





#include "jit/AsmJSFrameIterator.h"

#include "jit/AsmJS.h"
#include "jit/AsmJSModule.h"
#include "jit/IonMacroAssembler.h"

using namespace js;
using namespace js::jit;




static void *
ReturnAddressFromFP(uint8_t *fp)
{
    return reinterpret_cast<AsmJSFrame*>(fp)->returnAddress;
}

AsmJSFrameIterator::AsmJSFrameIterator(const AsmJSActivation &activation)
  : module_(&activation.module()),
    fp_(activation.fp())
{
    if (!fp_)
        return;
    settle();
}

void
AsmJSFrameIterator::operator++()
{
    JS_ASSERT(!done());
    fp_ += callsite_->stackDepth();
    settle();
}

void
AsmJSFrameIterator::settle()
{
    void *returnAddress = ReturnAddressFromFP(fp_);

    const AsmJSModule::CodeRange *codeRange = module_->lookupCodeRange(returnAddress);
    JS_ASSERT(codeRange);
    codeRange_ = codeRange;

    switch (codeRange->kind()) {
      case AsmJSModule::CodeRange::Entry:
        fp_ = nullptr;
        JS_ASSERT(done());
        return;
      case AsmJSModule::CodeRange::Function:
        callsite_ = module_->lookupCallSite(returnAddress);
        JS_ASSERT(callsite_);
        break;
    }
}

JSAtom *
AsmJSFrameIterator::functionDisplayAtom() const
{
    JS_ASSERT(!done());
    return reinterpret_cast<const AsmJSModule::CodeRange*>(codeRange_)->functionName(*module_);
}

unsigned
AsmJSFrameIterator::computeLine(uint32_t *column) const
{
    JS_ASSERT(!done());
    if (column)
        *column = callsite_->column();
    return callsite_->line();
}




static void
PushRetAddr(MacroAssembler &masm)
{
#if defined(JS_CODEGEN_ARM)
    masm.push(lr);
#elif defined(JS_CODEGEN_MIPS)
    masm.push(ra);
#else
    
#endif
}

void
js::GenerateAsmJSFunctionPrologue(MacroAssembler &masm, unsigned framePushed,
                                  Label *maybeOverflowThunk, Label *overflowExit)
{
    
    
    
    
    
    
    PushRetAddr(masm);
    masm.subPtr(Imm32(framePushed + AsmJSFrameBytesAfterReturnAddress), StackPointer);
    masm.setFramePushed(framePushed);

    
    
    
    if (maybeOverflowThunk) {
        
        Label *target = framePushed ? maybeOverflowThunk : overflowExit;
        masm.branchPtr(Assembler::AboveOrEqual,
                       AsmJSAbsoluteAddress(AsmJSImm_StackLimit),
                       StackPointer,
                       target);
    }
}

void
js::GenerateAsmJSFunctionEpilogue(MacroAssembler &masm, unsigned framePushed,
                                  Label *maybeOverflowThunk, Label *overflowExit)
{
    
    JS_ASSERT(masm.framePushed() == framePushed);
    masm.addPtr(Imm32(framePushed + AsmJSFrameBytesAfterReturnAddress), StackPointer);
    masm.ret();
    masm.setFramePushed(0);

    if (maybeOverflowThunk && maybeOverflowThunk->used()) {
        
        
        
        masm.bind(maybeOverflowThunk);
        masm.addPtr(Imm32(framePushed), StackPointer);
        masm.jump(overflowExit);
    }
}

void
js::GenerateAsmJSStackOverflowExit(MacroAssembler &masm, Label *overflowExit, Label *throwLabel)
{
    masm.bind(overflowExit);

    
    
    
    
    
    
    Register activation = ABIArgGenerator::NonArgReturnVolatileReg0;
    masm.loadAsmJSActivation(activation);
    masm.storePtr(StackPointer, Address(activation, AsmJSActivation::offsetOfFP()));

    
    if (unsigned stackDec = StackDecrementForCall(sizeof(AsmJSFrame), ShadowStackSpace))
        masm.subPtr(Imm32(stackDec), StackPointer);

    
    masm.assertStackAlignment();
    masm.call(AsmJSImmPtr(AsmJSImm_ReportOverRecursed));
    masm.jump(throwLabel);
}

void
js::GenerateAsmJSEntryPrologue(MacroAssembler &masm)
{
    
    
    
    PushRetAddr(masm);
    masm.subPtr(Imm32(AsmJSFrameBytesAfterReturnAddress), StackPointer);
    masm.setFramePushed(0);
}

void
js::GenerateAsmJSEntryEpilogue(MacroAssembler &masm)
{
    
    JS_ASSERT(masm.framePushed() == 0);
    masm.addPtr(Imm32(AsmJSFrameBytesAfterReturnAddress), StackPointer);
    masm.ret();
    masm.setFramePushed(0);
}

void
js::GenerateAsmJSFFIExitPrologue(MacroAssembler &masm, unsigned framePushed)
{
    
    PushRetAddr(masm);

    Register activation = ABIArgGenerator::NonArgReturnVolatileReg0;
    masm.loadAsmJSActivation(activation);
    Address fp(activation, AsmJSActivation::offsetOfFP());
    masm.push(fp);
    masm.storePtr(StackPointer, fp);

    if (framePushed)
        masm.subPtr(Imm32(framePushed), StackPointer);

    masm.setFramePushed(framePushed);
}

void
js::GenerateAsmJSFFIExitEpilogue(MacroAssembler &masm, unsigned framePushed)
{
    
    JS_ASSERT(masm.framePushed() == framePushed);

    if (framePushed)
        masm.addPtr(Imm32(framePushed), StackPointer);

    Register activation = ABIArgGenerator::NonArgReturnVolatileReg0;
    masm.loadAsmJSActivation(activation);
#if defined(JS_CODEGEN_X86) || defined(JS_CODEGEN_X64)
    masm.pop(Operand(activation, AsmJSActivation::offsetOfFP()));
#else
    Register fp = ABIArgGenerator::NonArgReturnVolatileReg1;
    masm.pop(fp);
    masm.storePtr(fp, Address(activation, AsmJSActivation::offsetOfFP()));
#endif

    masm.ret();
    masm.setFramePushed(0);
}

