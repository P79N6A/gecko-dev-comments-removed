






#include "BaselineFrame.h"
#include "BaselineFrame-inl.h"
#include "vm/ScopeObject.h"

using namespace js;
using namespace js::ion;

void
BaselineFrame::trace(JSTracer *trc)
{
    
    gc::MarkObjectRoot(trc, &scopeChain_, "baseline-scopechain");

    
    if (hasReturnValue())
        gc::MarkValueRoot(trc, returnValue(), "baseline-rval");

    if (isEvalFrame())
        gc::MarkScriptRoot(trc, &evalScript_, "baseline-evalscript");

    if (hasArgsObj())
        gc::MarkObjectRoot(trc, &argsObj_, "baseline-args-obj");

    
    size_t nvalues = numValueSlots();
    if (nvalues > 0) {
        
        Value *last = valueSlot(nvalues - 1);
        gc::MarkValueRootRange(trc, nvalues, last, "baseline-stack");
    }
}

bool
BaselineFrame::copyRawFrameSlots(AutoValueVector *vec) const
{
    unsigned nfixed = script()->nfixed;
    unsigned nformals = numFormalArgs();

    if (!vec->resize(nformals + nfixed))
        return false;

    PodCopy(vec->begin(), formals(), nformals);
    for (unsigned i = 0; i < nfixed; i++)
        (*vec)[nformals + i] = *valueSlot(i);
    return true;
}

bool
BaselineFrame::strictEvalPrologue(JSContext *cx)
{
    JS_ASSERT(isStrictEvalFrame());

    CallObject *callobj = CallObject::createForStrictEval(cx, this);
    if (!callobj)
        return false;

    pushOnScopeChain(*callobj);
    flags_ |= HAS_CALL_OBJ;
    return true;
}
