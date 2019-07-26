





#include "jit/BaselineFrame-inl.h"

#include "jit/BaselineJIT.h"
#include "jit/Ion.h"
#include "vm/Debugger.h"
#include "vm/ScopeObject.h"

#include "jit/IonFrames-inl.h"
#include "vm/Stack-inl.h"

using namespace js;
using namespace js::jit;

void
BaselineFrame::trace(JSTracer *trc)
{
    replaceCalleeToken(MarkCalleeToken(trc, calleeToken()));

    gc::MarkValueRoot(trc, &thisValue(), "baseline-this");

    
    if (isNonEvalFunctionFrame()) {
        unsigned numArgs = js::Max(numActualArgs(), numFormalArgs());
        gc::MarkValueRootRange(trc, numArgs, argv(), "baseline-args");
    }

    
    if (scopeChain_)
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

    mozilla::PodCopy(vec->begin(), argv(), nformals);
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

bool
BaselineFrame::heavyweightFunPrologue(JSContext *cx)
{
    return initFunctionScopeObjects(cx);
}

bool
BaselineFrame::initFunctionScopeObjects(JSContext *cx)
{
    JS_ASSERT(isNonEvalFunctionFrame());
    JS_ASSERT(fun()->isHeavyweight());

    CallObject *callobj = CallObject::createForFunction(cx, this);
    if (!callobj)
        return false;

    pushOnScopeChain(*callobj);
    flags_ |= HAS_CALL_OBJ;
    return true;
}

bool
BaselineFrame::initForOsr(StackFrame *fp, uint32_t numStackValues)
{
    mozilla::PodZero(this);

    scopeChain_ = fp->scopeChain();

    if (fp->hasCallObjUnchecked())
        flags_ |= BaselineFrame::HAS_CALL_OBJ;

    if (fp->hasBlockChain()) {
        flags_ |= BaselineFrame::HAS_BLOCKCHAIN;
        blockChain_ = &fp->blockChain();
    }

    if (fp->isEvalFrame()) {
        flags_ |= BaselineFrame::EVAL;
        evalScript_ = fp->script();
    }

    if (fp->script()->needsArgsObj() && fp->hasArgsObj()) {
        flags_ |= BaselineFrame::HAS_ARGS_OBJ;
        argsObj_ = &fp->argsObj();
    }

    if (fp->hasHookData()) {
        flags_ |= BaselineFrame::HAS_HOOK_DATA;
        hookData_ = fp->hookData();
    }

    if (fp->hasReturnValue())
        setReturnValue(fp->returnValue());

    if (fp->hasPushedSPSFrame())
        flags_ |= BaselineFrame::HAS_PUSHED_SPS_FRAME;

    frameSize_ = BaselineFrame::FramePointerOffset +
        BaselineFrame::Size() +
        numStackValues * sizeof(Value);

    JS_ASSERT(numValueSlots() == numStackValues);

    for (uint32_t i = 0; i < numStackValues; i++)
        *valueSlot(i) = fp->slots()[i];

    JSContext *cx = GetIonContext()->cx;
    if (cx->compartment()->debugMode()) {
        
        

        
        
        
        
        IonFrameIterator iter(cx->mainThread().ionTop);
        JS_ASSERT(iter.returnAddress() == nullptr);
        BaselineScript *baseline = fp->script()->baselineScript();
        iter.current()->setReturnAddress(baseline->returnAddressForIC(baseline->icEntry(0)));

        if (!Debugger::handleBaselineOsr(cx, fp, this))
            return false;
    }

    return true;
}
