





#include "jit/AsmJSFrameIterator.h"

#include "jit/AsmJS.h"
#include "jit/AsmJSModule.h"

using namespace js;
using namespace js::jit;

static uint8_t *
ReturnAddressForExitCall(uint8_t **psp)
{
    uint8_t *sp = *psp;
#if defined(JS_CODEGEN_X86) || defined(JS_CODEGEN_X64)
    
    
    
    return *(uint8_t**)(sp - sizeof(void*));
#elif defined(JS_CODEGEN_ARM)
    
    
    
    
    
    return *(uint8_t**)sp;
#elif defined(JS_CODEGEN_MIPS)
    
    
    
    
    if (uintptr_t(sp) & 0x1) {
        sp = *psp -= 0x1;  
        return *(uint8_t**)sp;
    }
    return *(uint8_t**)(sp + ShadowStackSpace);
#else
# error "Unknown architecture!"
#endif
}

static uint8_t *
ReturnAddressForJitCall(uint8_t *sp)
{
    
    
    return *(uint8_t**)(sp - sizeof(void*));
}

AsmJSFrameIterator::AsmJSFrameIterator(const AsmJSActivation *activation)
  : module_(nullptr)
{
    if (!activation || activation->isInterruptedSP())
        return;

    module_ = &activation->module();
    sp_ = activation->exitSP();

    settle(ReturnAddressForExitCall(&sp_));
}

void
AsmJSFrameIterator::operator++()
{
    settle(ReturnAddressForJitCall(sp_));
}

void
AsmJSFrameIterator::settle(uint8_t *returnAddress)
{
    callsite_ = module_->lookupCallSite(returnAddress);
    if (!callsite_ || callsite_->isEntry()) {
        module_ = nullptr;
        return;
    }

    if (callsite_->isEntry()) {
        module_ = nullptr;
        return;
    }

    sp_ += callsite_->stackDepth();

    if (callsite_->isExit())
        return settle(ReturnAddressForJitCall(sp_));

    JS_ASSERT(callsite_->isNormal());
}

JSAtom *
AsmJSFrameIterator::functionDisplayAtom() const
{
    JS_ASSERT(!done());
    return module_->functionName(callsite_->functionNameIndex());
}

unsigned
AsmJSFrameIterator::computeLine(uint32_t *column) const
{
    JS_ASSERT(!done());
    if (column)
        *column = callsite_->column();
    return callsite_->line();
}

