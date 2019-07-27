





#include "jit/BaselineFrame-inl.h"

#include "jit/BaselineJIT.h"
#include "jit/Ion.h"
#include "vm/Debugger.h"
#include "vm/ScopeObject.h"

#include "jit/JitFrames-inl.h"
#include "vm/Stack-inl.h"

using namespace js;
using namespace js::jit;

static void
MarkLocals(BaselineFrame* frame, JSTracer* trc, unsigned start, unsigned end)
{
    if (start < end) {
        
        Value* last = frame->valueSlot(end - 1);
        TraceRootRange(trc, end - start, last, "baseline-stack");
    }
}

void
BaselineFrame::trace(JSTracer* trc, JitFrameIterator& frameIterator)
{
    replaceCalleeToken(MarkCalleeToken(trc, calleeToken()));

    TraceRoot(trc, &thisValue(), "baseline-this");

    
    if (isNonEvalFunctionFrame()) {
        unsigned numArgs = js::Max(numActualArgs(), numFormalArgs());
        TraceRootRange(trc, numArgs, argv(), "baseline-args");
    }

    
    if (scopeChain_)
        gc::MarkObjectRoot(trc, &scopeChain_, "baseline-scopechain");

    
    if (hasReturnValue())
        TraceRoot(trc, returnValue().address(), "baseline-rval");

    if (isEvalFrame())
        TraceRoot(trc, &evalScript_, "baseline-evalscript");

    if (hasArgsObj())
        gc::MarkObjectRoot(trc, &argsObj_, "baseline-args-obj");

    
    JSScript* script = this->script();
    size_t nfixed = script->nfixed();
    size_t nlivefixed = script->nbodyfixed();

    if (nfixed != nlivefixed) {
        jsbytecode* pc;
        frameIterator.baselineScriptAndPc(nullptr, &pc);

        NestedScopeObject* staticScope = script->getStaticBlockScope(pc);
        while (staticScope && !staticScope->is<StaticBlockObject>())
            staticScope = staticScope->enclosingNestedScope();

        if (staticScope) {
            StaticBlockObject& blockObj = staticScope->as<StaticBlockObject>();
            nlivefixed = blockObj.localOffset() + blockObj.numVariables();
        }
    }

    MOZ_ASSERT(nlivefixed <= nfixed);
    MOZ_ASSERT(nlivefixed >= script->nbodyfixed());

    
    
    if (numValueSlots() == 0)
        return;

    MOZ_ASSERT(nfixed <= numValueSlots());

    if (nfixed == nlivefixed) {
        
        MarkLocals(this, trc, 0, numValueSlots());
    } else {
        
        MarkLocals(this, trc, nfixed, numValueSlots());

        
        while (nfixed > nlivefixed)
            unaliasedLocal(--nfixed).setMagic(JS_UNINITIALIZED_LEXICAL);

        
        MarkLocals(this, trc, 0, nlivefixed);
    }
}

bool
BaselineFrame::copyRawFrameSlots(AutoValueVector* vec) const
{
    unsigned nfixed = script()->nfixed();
    unsigned nformals = numFormalArgs();

    if (!vec->resize(nformals + nfixed))
        return false;

    mozilla::PodCopy(vec->begin(), argv(), nformals);
    for (unsigned i = 0; i < nfixed; i++)
        (*vec)[nformals + i].set(*valueSlot(i));
    return true;
}

bool
BaselineFrame::strictEvalPrologue(JSContext* cx)
{
    MOZ_ASSERT(isStrictEvalFrame());

    CallObject* callobj = CallObject::createForStrictEval(cx, this);
    if (!callobj)
        return false;

    pushOnScopeChain(*callobj);
    flags_ |= HAS_CALL_OBJ;
    return true;
}

bool
BaselineFrame::heavyweightFunPrologue(JSContext* cx)
{
    return initFunctionScopeObjects(cx);
}

bool
BaselineFrame::initFunctionScopeObjects(JSContext* cx)
{
    MOZ_ASSERT(isNonEvalFunctionFrame());
    MOZ_ASSERT(fun()->isHeavyweight());

    CallObject* callobj = CallObject::createForFunction(cx, this);
    if (!callobj)
        return false;

    pushOnScopeChain(*callobj);
    flags_ |= HAS_CALL_OBJ;
    return true;
}

bool
BaselineFrame::initForOsr(InterpreterFrame* fp, uint32_t numStackValues)
{
    mozilla::PodZero(this);

    scopeChain_ = fp->scopeChain();

    if (fp->hasCallObjUnchecked())
        flags_ |= BaselineFrame::HAS_CALL_OBJ;

    if (fp->isEvalFrame()) {
        flags_ |= BaselineFrame::EVAL;
        evalScript_ = fp->script();
    }

    if (fp->script()->needsArgsObj() && fp->hasArgsObj()) {
        flags_ |= BaselineFrame::HAS_ARGS_OBJ;
        argsObj_ = &fp->argsObj();
    }

    if (fp->hasReturnValue())
        setReturnValue(fp->returnValue());

    frameSize_ = BaselineFrame::FramePointerOffset +
        BaselineFrame::Size() +
        numStackValues * sizeof(Value);

    MOZ_ASSERT(numValueSlots() == numStackValues);

    for (uint32_t i = 0; i < numStackValues; i++)
        *valueSlot(i) = fp->slots()[i];

    if (fp->isDebuggee()) {
        JSContext* cx = GetJSContextFromJitCode();

        
        

        
        
        
        
        JitFrameIterator iter(cx);
        MOZ_ASSERT(iter.returnAddress() == nullptr);
        BaselineScript* baseline = fp->script()->baselineScript();
        iter.current()->setReturnAddress(baseline->returnAddressForIC(baseline->icEntry(0)));

        if (!Debugger::handleBaselineOsr(cx, fp, this))
            return false;

        setIsDebuggee();
    }

    return true;
}
