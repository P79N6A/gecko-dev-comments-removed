





#include "jit/AsmJSFrameIterator.h"

#include "jit/AsmJS.h"
#include "jit/AsmJSModule.h"

using namespace js;
using namespace js::jit;

static void *
ReturnAddressFromFP(uint8_t *fp)
{
    
    
    static_assert(AsmJSFrameSize == sizeof(void*), "Frame size mismatch");
    return *(uint8_t**)fp;
}

AsmJSFrameIterator::AsmJSFrameIterator(const AsmJSActivation &activation)
  : module_(&activation.module()),
    fp_(activation.exitFP())
{
    if (!fp_)
        return;
    settle(ReturnAddressFromFP(fp_));
}

void
AsmJSFrameIterator::operator++()
{
    JS_ASSERT(!done());
    fp_ += callsite_->stackDepth();
    settle(ReturnAddressFromFP(fp_));
}

void
AsmJSFrameIterator::settle(void *returnAddress)
{
    const AsmJSModule::CodeRange *codeRange = module_->lookupCodeRange(ReturnAddressFromFP(fp_));
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

