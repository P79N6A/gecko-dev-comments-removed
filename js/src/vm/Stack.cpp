





#include "vm/Stack.h"

#include "mozilla/PodOperations.h"

#include "jscntxt.h"

#include "gc/Marking.h"
#ifdef JS_ION
#include "ion/BaselineFrame.h"
#include "ion/IonCompartment.h"
#endif

#include "vm/Interpreter-inl.h"
#include "vm/ScopeObject-inl.h"
#include "vm/Stack-inl.h"
#include "vm/Probes-inl.h"

using namespace js;

using mozilla::PodCopy;



void
StackFrame::initExecuteFrame(JSContext *cx, JSScript *script, AbstractFramePtr evalInFramePrev,
                             const Value &thisv, JSObject &scopeChain, ExecuteType type)
{
    




    flags_ = type | HAS_SCOPECHAIN | HAS_BLOCKCHAIN;

    JSObject *callee = NULL;
    if (!(flags_ & (GLOBAL))) {
        if (evalInFramePrev) {
            JS_ASSERT(evalInFramePrev.isFunctionFrame() || evalInFramePrev.isGlobalFrame());
            if (evalInFramePrev.isFunctionFrame()) {
                callee = evalInFramePrev.callee();
                flags_ |= FUNCTION;
            } else {
                flags_ |= GLOBAL;
            }
        } else {
            ScriptFrameIter iter(cx);
            JS_ASSERT(iter.isFunctionFrame() || iter.isGlobalFrame());
            if (iter.isFunctionFrame()) {
                callee = iter.callee();
                flags_ |= FUNCTION;
            } else {
                flags_ |= GLOBAL;
            }
        }
    }

    Value *dstvp = (Value *)this - 2;
    dstvp[1] = thisv;

    if (isFunctionFrame()) {
        dstvp[0] = ObjectValue(*callee);
        exec.fun = &callee->as<JSFunction>();
        u.evalScript = script;
    } else {
        JS_ASSERT(isGlobalFrame());
        dstvp[0] = NullValue();
        exec.script = script;
#ifdef DEBUG
        u.evalScript = (JSScript *)0xbad;
#endif
    }

    scopeChain_ = &scopeChain;
    prev_ = NULL;
    prevpc_ = NULL;
    prevsp_ = NULL;
    blockChain_ = NULL;

    JS_ASSERT_IF(evalInFramePrev, isDebuggerFrame());
    evalInFramePrev_ = evalInFramePrev;

#ifdef DEBUG
    Debug_SetValueRangeToCrashOnTouch(&rval_, 1);
    hookData_ = (void *)0xbad;
#endif
}

template <StackFrame::TriggerPostBarriers doPostBarrier>
void
StackFrame::copyFrameAndValues(JSContext *cx, Value *vp, StackFrame *otherfp,
                               const Value *othervp, Value *othersp)
{
    JS_ASSERT(othervp == otherfp->generatorArgsSnapshotBegin());
    JS_ASSERT(othersp >= otherfp->slots());
    JS_ASSERT(othersp <= otherfp->generatorSlotsSnapshotBegin() + otherfp->script()->nslots);

    
    const Value *srcend = otherfp->generatorArgsSnapshotEnd();
    Value *dst = vp;
    for (const Value *src = othervp; src < srcend; src++, dst++) {
        *dst = *src;
        if (doPostBarrier)
            HeapValue::writeBarrierPost(*dst, dst);
    }

    *this = *otherfp;
    argv_ = vp + 2;
    unsetPushedSPSFrame();
    if (doPostBarrier)
        writeBarrierPost();

    srcend = othersp;
    dst = slots();
    for (const Value *src = otherfp->slots(); src < srcend; src++, dst++) {
        *dst = *src;
        if (doPostBarrier)
            HeapValue::writeBarrierPost(*dst, dst);
    }

    if (cx->compartment()->debugMode())
        DebugScopes::onGeneratorFrameChange(otherfp, this, cx);
}


template
void StackFrame::copyFrameAndValues<StackFrame::NoPostBarrier>(
                                    JSContext *, Value *, StackFrame *, const Value *, Value *);
template
void StackFrame::copyFrameAndValues<StackFrame::DoPostBarrier>(
                                    JSContext *, Value *, StackFrame *, const Value *, Value *);

void
StackFrame::writeBarrierPost()
{
    
    if (scopeChain_)
        JSObject::writeBarrierPost(scopeChain_, (void *)&scopeChain_);
    if (flags_ & HAS_ARGS_OBJ)
        JSObject::writeBarrierPost(argsObj_, (void *)&argsObj_);
    if (isFunctionFrame()) {
        JSFunction::writeBarrierPost(exec.fun, (void *)&exec.fun);
        if (isEvalFrame())
            JSScript::writeBarrierPost(u.evalScript, (void *)&u.evalScript);
    } else {
        JSScript::writeBarrierPost(exec.script, (void *)&exec.script);
    }
    if (hasReturnValue())
        HeapValue::writeBarrierPost(rval_, &rval_);
}

JSGenerator *
StackFrame::maybeSuspendedGenerator(JSRuntime *rt)
{
    



    if (!isGeneratorFrame() || !isSuspended())
        return NULL;

    



    char *vp = reinterpret_cast<char *>(generatorArgsSnapshotBegin());
    char *p = vp - offsetof(JSGenerator, stackSnapshot);
    JSGenerator *gen = reinterpret_cast<JSGenerator *>(p);
    JS_ASSERT(gen->fp == this);
    return gen;
}

bool
StackFrame::copyRawFrameSlots(AutoValueVector *vec)
{
    if (!vec->resize(numFormalArgs() + script()->nfixed))
        return false;
    PodCopy(vec->begin(), argv(), numFormalArgs());
    PodCopy(vec->begin() + numFormalArgs(), slots(), script()->nfixed);
    return true;
}

JSObject *
StackFrame::createRestParameter(JSContext *cx)
{
    JS_ASSERT(fun()->hasRest());
    unsigned nformal = fun()->nargs - 1, nactual = numActualArgs();
    unsigned nrest = (nactual > nformal) ? nactual - nformal : 0;
    Value *restvp = argv() + nformal;
    RootedObject obj(cx, NewDenseCopiedArray(cx, nrest, restvp, NULL));
    if (!obj)
        return NULL;

    RootedTypeObject type(cx, types::GetTypeCallerInitObject(cx, JSProto_Array));
    if (!type)
        return NULL;
    obj->setType(type);

    
    for (unsigned i = 0; i < nrest; i++)
        types::AddTypePropertyId(cx, obj, JSID_VOID, restvp[i]);

    return obj;
}

static inline void
AssertDynamicScopeMatchesStaticScope(JSContext *cx, JSScript *script, JSObject *scope)
{
#ifdef DEBUG
    RootedObject enclosingScope(cx, script->enclosingStaticScope());
    for (StaticScopeIter i(cx, enclosingScope); !i.done(); i++) {
        if (i.hasDynamicScopeObject()) {
            



            while (scope->is<WithObject>())
                scope = &scope->as<WithObject>().enclosingScope();

            switch (i.type()) {
              case StaticScopeIter::BLOCK:
                JS_ASSERT(i.block() == scope->as<ClonedBlockObject>().staticBlock());
                scope = &scope->as<ClonedBlockObject>().enclosingScope();
                break;
              case StaticScopeIter::FUNCTION:
                JS_ASSERT(scope->as<CallObject>().callee().nonLazyScript() == i.funScript());
                scope = &scope->as<CallObject>().enclosingScope();
                break;
              case StaticScopeIter::NAMED_LAMBDA:
                scope = &scope->as<DeclEnvObject>().enclosingScope();
                break;
            }
        }
    }

    




#endif
}

bool
StackFrame::initFunctionScopeObjects(JSContext *cx)
{
    CallObject *callobj = CallObject::createForFunction(cx, this);
    if (!callobj)
        return false;
    pushOnScopeChain(*callobj);
    flags_ |= HAS_CALL_OBJ;
    return true;
}

bool
StackFrame::prologue(JSContext *cx)
{
    RootedScript script(cx, this->script());

    JS_ASSERT(!isGeneratorFrame());
    JS_ASSERT(cx->interpreterRegs().pc == script->code);

    if (isEvalFrame()) {
        if (script->strict) {
            CallObject *callobj = CallObject::createForStrictEval(cx, this);
            if (!callobj)
                return false;
            pushOnScopeChain(*callobj);
            flags_ |= HAS_CALL_OBJ;
        }
        Probes::enterScript(cx, script, NULL, this);
        return true;
    }

    if (isGlobalFrame()) {
        Probes::enterScript(cx, script, NULL, this);
        return true;
    }

    JS_ASSERT(isNonEvalFunctionFrame());
    AssertDynamicScopeMatchesStaticScope(cx, script, scopeChain());

    if (fun()->isHeavyweight() && !initFunctionScopeObjects(cx))
        return false;

    if (isConstructing()) {
        RootedObject callee(cx, &this->callee());
        JSObject *obj = CreateThisForFunction(cx, callee, useNewType());
        if (!obj)
            return false;
        functionThis() = ObjectValue(*obj);
    }

    Probes::enterScript(cx, script, script->function(), this);
    return true;
}

void
StackFrame::epilogue(JSContext *cx)
{
    JS_ASSERT(!isYielding());
    JS_ASSERT(!hasBlockChain());

    RootedScript script(cx, this->script());
    Probes::exitScript(cx, script, script->function(), this);

    if (isEvalFrame()) {
        if (isStrictEvalFrame()) {
            JS_ASSERT_IF(hasCallObj(), scopeChain()->as<CallObject>().isForEval());
            if (cx->compartment()->debugMode())
                DebugScopes::onPopStrictEvalScope(this);
        } else if (isDirectEvalFrame()) {
            if (isDebuggerFrame())
                JS_ASSERT(!scopeChain()->is<ScopeObject>());
        } else {
            






            if (isDebuggerFrame()) {
                JS_ASSERT(scopeChain()->is<GlobalObject>() ||
                          scopeChain()->enclosingScope()->is<GlobalObject>());
            } else {
                JS_ASSERT(scopeChain()->is<GlobalObject>());
            }
        }
        return;
    }

    if (isGlobalFrame()) {
        JS_ASSERT(!scopeChain()->is<ScopeObject>());
        return;
    }

    JS_ASSERT(isNonEvalFunctionFrame());

    if (fun()->isHeavyweight())
        JS_ASSERT_IF(hasCallObj(),
                     scopeChain()->as<CallObject>().callee().nonLazyScript() == script);
    else
        AssertDynamicScopeMatchesStaticScope(cx, script, scopeChain());

    if (cx->compartment()->debugMode())
        DebugScopes::onPopCall(this, cx);

    if (isConstructing() && thisValue().isObject() && returnValue().isPrimitive())
        setReturnValue(ObjectValue(constructorThis()));
}

bool
StackFrame::pushBlock(JSContext *cx, StaticBlockObject &block)
{
    JS_ASSERT_IF(hasBlockChain(), blockChain_ == block.enclosingBlock());

    if (block.needsClone()) {
        Rooted<StaticBlockObject *> blockHandle(cx, &block);
        ClonedBlockObject *clone = ClonedBlockObject::create(cx, blockHandle, this);
        if (!clone)
            return false;

        pushOnScopeChain(*clone);

        blockChain_ = blockHandle;
    } else {
        blockChain_ = &block;
    }

    flags_ |= HAS_BLOCKCHAIN;
    return true;
}

void
StackFrame::popBlock(JSContext *cx)
{
    JS_ASSERT(hasBlockChain());

    if (cx->compartment()->debugMode())
        DebugScopes::onPopBlock(cx, this);

    if (blockChain_->needsClone()) {
        JS_ASSERT(scopeChain_->as<ClonedBlockObject>().staticBlock() == *blockChain_);
        popOffScopeChain();
    }

    blockChain_ = blockChain_->enclosingBlock();
}

void
StackFrame::popWith(JSContext *cx)
{
    if (cx->compartment()->debugMode())
        DebugScopes::onPopWith(this);

    JS_ASSERT(scopeChain()->is<WithObject>());
    popOffScopeChain();
}

void
StackFrame::mark(JSTracer *trc)
{
    




    if (flags_ & HAS_SCOPECHAIN)
        gc::MarkObjectUnbarriered(trc, &scopeChain_, "scope chain");
    if (flags_ & HAS_ARGS_OBJ)
        gc::MarkObjectUnbarriered(trc, &argsObj_, "arguments");
    if (isFunctionFrame()) {
        gc::MarkObjectUnbarriered(trc, &exec.fun, "fun");
        if (isEvalFrame())
            gc::MarkScriptUnbarriered(trc, &u.evalScript, "eval script");
    } else {
        gc::MarkScriptUnbarriered(trc, &exec.script, "script");
    }
    if (IS_GC_MARKING_TRACER(trc))
        script()->compartment()->zone()->active = true;
    gc::MarkValueUnbarriered(trc, &returnValue(), "rval");
}

void
StackFrame::markValues(JSTracer *trc, Value *sp)
{
    JS_ASSERT(sp >= slots());
    gc::MarkValueRootRange(trc, sp - slots(), slots(), "vm_stack");
    if (hasArgs())
        gc::MarkValueRootRange(trc, js::Max(numActualArgs(), numFormalArgs()), argv_, "fp argv");
}

static void
MarkInterpreterActivation(JSTracer *trc, InterpreterActivation *act)
{
    for (InterpreterFrameIterator frames(act); !frames.done(); ++frames) {
        StackFrame *fp = frames.frame();
        fp->markValues(trc, frames.sp());
        fp->mark(trc);
    }
}

void
js::MarkInterpreterActivations(JSRuntime *rt, JSTracer *trc)
{
    for (ActivationIterator iter(rt); !iter.done(); ++iter) {
        Activation *act = iter.activation();
        if (act->isInterpreter())
            MarkInterpreterActivation(trc, act->asInterpreter());
    }

}



StackFrame *
InterpreterStack::pushInvokeFrame(JSContext *cx, const CallArgs &args, InitialFrameFlags initial,
                                  FrameGuard *fg)
{
    LifoAlloc::Mark mark = allocator_.mark();

    RootedFunction fun(cx, &args.callee().as<JSFunction>());
    RootedScript script(cx, fun->nonLazyScript());

    StackFrame::Flags flags = ToFrameFlags(initial);
    Value *argv;
    StackFrame *fp = getCallFrame(cx, args, script, &flags, &argv);
    if (!fp)
        return NULL;

    fp->mark_ = mark;
    fp->initCallFrame(cx, NULL, NULL, NULL, *fun, script, argv, args.length(), flags);
    fg->setPushed(*this, fp);
    return fp;
}

StackFrame *
InterpreterStack::pushExecuteFrame(JSContext *cx, HandleScript script, const Value &thisv,
                                   HandleObject scopeChain, ExecuteType type,
                                   AbstractFramePtr evalInFrame, FrameGuard *fg)
{
    LifoAlloc::Mark mark = allocator_.mark();

    unsigned nvars = 2  + script->nslots;
    uint8_t *buffer = allocateFrame(cx, sizeof(StackFrame) + nvars * sizeof(Value));
    if (!buffer)
        return NULL;

    StackFrame *fp = reinterpret_cast<StackFrame *>(buffer + 2 * sizeof(Value));
    fp->mark_ = mark;
    fp->initExecuteFrame(cx, script, evalInFrame, thisv, *scopeChain, type);
    fp->initVarsToUndefined();

    fg->setPushed(*this, fp);
    return fp;
}




#if defined(_MSC_VER)
# pragma optimize("g", off)
#endif

void
ScriptFrameIter::popActivation()
{
    ++data_.activations_;
    settleOnActivation();
}

void
ScriptFrameIter::popInterpreterFrame()
{
    JS_ASSERT(data_.state_ == SCRIPTED);

    ++data_.interpFrames_;

    if (data_.interpFrames_.done())
        popActivation();
    else
        data_.pc_ = data_.interpFrames_.pc();
}

void
ScriptFrameIter::settleOnActivation()
{
    while (true) {
        if (data_.activations_.done()) {
            data_.state_ = DONE;
            return;
        }

        Activation *activation = data_.activations_.activation();

        
        
        if (data_.savedOption_ == STOP_AT_SAVED && activation->hasSavedFrameChain()) {
            data_.state_ = DONE;
            return;
        }

        
        JS_ASSERT(activation->cx());
        JS_ASSERT(data_.cx_);
        if (data_.contextOption_ == CURRENT_CONTEXT && activation->cx() != data_.cx_) {
            ++data_.activations_;
            continue;
        }

#ifdef JS_ION
        if (activation->isJit()) {
            data_.ionFrames_ = ion::IonFrameIterator(data_.activations_);

            
            while (!data_.ionFrames_.isScripted() && !data_.ionFrames_.done())
                ++data_.ionFrames_;

            
            
            if (data_.ionFrames_.done()) {
                ++data_.activations_;
                continue;
            }

            nextJitFrame();
            data_.state_ = JIT;
            return;
        }
#endif

        JS_ASSERT(activation->isInterpreter());

        InterpreterActivation *interpAct = activation->asInterpreter();
        data_.interpFrames_ = InterpreterFrameIterator(interpAct);

        
        
        if (data_.interpFrames_.frame()->runningInJit()) {
            ++data_.interpFrames_;
            if (data_.interpFrames_.done()) {
                ++data_.activations_;
                continue;
            }
        }

        JS_ASSERT(!data_.interpFrames_.frame()->runningInJit());
        data_.pc_ = data_.interpFrames_.pc();
        data_.state_ = SCRIPTED;
        return;
    }
}

ScriptFrameIter::Data::Data(JSContext *cx, PerThreadData *perThread, SavedOption savedOption,
                            ContextOption contextOption)
  : perThread_(perThread),
    cx_(cx),
    savedOption_(savedOption),
    contextOption_(contextOption),
    pc_(NULL),
    interpFrames_(NULL),
    activations_(cx->runtime())
#ifdef JS_ION
  , ionFrames_((uint8_t *)NULL)
#endif
{
}

ScriptFrameIter::Data::Data(const ScriptFrameIter::Data &other)
  : perThread_(other.perThread_),
    cx_(other.cx_),
    savedOption_(other.savedOption_),
    contextOption_(other.contextOption_),
    state_(other.state_),
    pc_(other.pc_),
    interpFrames_(other.interpFrames_),
    activations_(other.activations_)
#ifdef JS_ION
  , ionFrames_(other.ionFrames_)
#endif
{
}

ScriptFrameIter::ScriptFrameIter(JSContext *cx, SavedOption savedOption)
  : data_(cx, &cx->runtime()->mainThread, savedOption, CURRENT_CONTEXT)
#ifdef JS_ION
    , ionInlineFrames_(cx, (js::ion::IonFrameIterator*) NULL)
#endif
{
    settleOnActivation();
}

ScriptFrameIter::ScriptFrameIter(JSContext *cx, ContextOption contextOption, SavedOption savedOption)
  : data_(cx, &cx->runtime()->mainThread, savedOption, contextOption)
#ifdef JS_ION
    , ionInlineFrames_(cx, (js::ion::IonFrameIterator*) NULL)
#endif
{
    settleOnActivation();
}

ScriptFrameIter::ScriptFrameIter(const ScriptFrameIter &other)
  : data_(other.data_)
#ifdef JS_ION
    , ionInlineFrames_(other.data_.cx_,
                       data_.ionFrames_.isScripted() ? &other.ionInlineFrames_ : NULL)
#endif
{
}

ScriptFrameIter::ScriptFrameIter(const Data &data)
  : data_(data)
#ifdef JS_ION
    , ionInlineFrames_(data.cx_, data_.ionFrames_.isOptimizedJS() ? &data_.ionFrames_ : NULL)
#endif
{
    JS_ASSERT(data.cx_);
}

#ifdef JS_ION
void
ScriptFrameIter::nextJitFrame()
{
    if (data_.ionFrames_.isOptimizedJS()) {
        ionInlineFrames_.resetOn(&data_.ionFrames_);
        data_.pc_ = ionInlineFrames_.pc();
    } else {
        JS_ASSERT(data_.ionFrames_.isBaselineJS());
        data_.ionFrames_.baselineScriptAndPc(NULL, &data_.pc_);
    }
}

void
ScriptFrameIter::popJitFrame()
{
    JS_ASSERT(data_.state_ == JIT);

    if (data_.ionFrames_.isOptimizedJS() && ionInlineFrames_.more()) {
        ++ionInlineFrames_;
        data_.pc_ = ionInlineFrames_.pc();
        return;
    }

    ++data_.ionFrames_;
    while (!data_.ionFrames_.done() && !data_.ionFrames_.isScripted())
        ++data_.ionFrames_;

    if (!data_.ionFrames_.done()) {
        nextJitFrame();
        return;
    }

    popActivation();
}
#endif

ScriptFrameIter &
ScriptFrameIter::operator++()
{
    switch (data_.state_) {
      case DONE:
        JS_NOT_REACHED("Unexpected state");
      case SCRIPTED:
        if (interpFrame()->isDebuggerFrame() && interpFrame()->evalInFramePrev()) {
            AbstractFramePtr eifPrev = interpFrame()->evalInFramePrev();

            
            
            ContextOption prevContextOption = data_.contextOption_;
            SavedOption prevSavedOption = data_.savedOption_;
            data_.contextOption_ = ALL_CONTEXTS;
            data_.savedOption_ = GO_THROUGH_SAVED;

            popInterpreterFrame();

            while (isIon() || abstractFramePtr() != eifPrev) {
                if (data_.state_ == JIT) {
#ifdef JS_ION
                    popJitFrame();
#else
                    JS_NOT_REACHED("Invalid state");
#endif
                } else {
                    popInterpreterFrame();
                }
            }

            data_.contextOption_ = prevContextOption;
            data_.savedOption_ = prevSavedOption;
            data_.cx_ = data_.activations_.activation()->cx();
            break;
        }
        popInterpreterFrame();
        break;
      case JIT:
#ifdef JS_ION
        popJitFrame();
        break;
#else
        JS_NOT_REACHED("Unexpected state");
#endif
    }
    return *this;
}

ScriptFrameIter::Data *
ScriptFrameIter::copyData() const
{
#ifdef JS_ION
    



    JS_ASSERT(data_.ionFrames_.type() != ion::IonFrame_OptimizedJS);
#endif
    return data_.cx_->new_<Data>(data_);
}

JSCompartment *
ScriptFrameIter::compartment() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
      case JIT:
        return data_.activations_.activation()->compartment();
    }
    JS_NOT_REACHED("Unexpected state");
    return NULL;
}

bool
ScriptFrameIter::isFunctionFrame() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        return interpFrame()->isFunctionFrame();
      case JIT:
#ifdef JS_ION
        JS_ASSERT(data_.ionFrames_.isScripted());
        if (data_.ionFrames_.isBaselineJS())
            return data_.ionFrames_.isFunctionFrame();
        return ionInlineFrames_.isFunctionFrame();
#else
        break;
#endif
    }
    JS_NOT_REACHED("Unexpected state");
    return false;
}

bool
ScriptFrameIter::isGlobalFrame() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        return interpFrame()->isGlobalFrame();
      case JIT:
#ifdef JS_ION
        if (data_.ionFrames_.isBaselineJS())
            return data_.ionFrames_.baselineFrame()->isGlobalFrame();
        JS_ASSERT(!script()->isForEval());
        return !script()->function();
#else
        break;
#endif
    }
    JS_NOT_REACHED("Unexpected state");
    return false;
}

bool
ScriptFrameIter::isEvalFrame() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        return interpFrame()->isEvalFrame();
      case JIT:
#ifdef JS_ION
        if (data_.ionFrames_.isBaselineJS())
            return data_.ionFrames_.baselineFrame()->isEvalFrame();
        JS_ASSERT(!script()->isForEval());
        return false;
#else
        break;
#endif
    }
    JS_NOT_REACHED("Unexpected state");
    return false;
}

bool
ScriptFrameIter::isNonEvalFunctionFrame() const
{
    JS_ASSERT(!done());
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        return interpFrame()->isNonEvalFunctionFrame();
      case JIT:
        return !isEvalFrame() && isFunctionFrame();
    }
    JS_NOT_REACHED("Unexpected state");
    return false;
}

bool
ScriptFrameIter::isGeneratorFrame() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        return interpFrame()->isGeneratorFrame();
      case JIT:
        return false;
    }
    JS_NOT_REACHED("Unexpected state");
    return false;
}

bool
ScriptFrameIter::isConstructing() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case JIT:
#ifdef JS_ION
        if (data_.ionFrames_.isOptimizedJS())
            return ionInlineFrames_.isConstructing();
        JS_ASSERT(data_.ionFrames_.isBaselineJS());
        return data_.ionFrames_.isConstructing();
#else
        break;
#endif        
      case SCRIPTED:
        return interpFrame()->isConstructing();
    }
    JS_NOT_REACHED("Unexpected state");
    return false;
}

AbstractFramePtr
ScriptFrameIter::abstractFramePtr() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case JIT:
#ifdef JS_ION
        if (data_.ionFrames_.isBaselineJS())
            return data_.ionFrames_.baselineFrame();
#endif
        break;
      case SCRIPTED:
        JS_ASSERT(interpFrame());
        return AbstractFramePtr(interpFrame());
    }
    JS_NOT_REACHED("Unexpected state");
    return NullFramePtr();
}

void
ScriptFrameIter::updatePcQuadratic()
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED: {
        StackFrame *frame = interpFrame();
        InterpreterActivation *activation = data_.activations_.activation()->asInterpreter();

        
        data_.interpFrames_ = InterpreterFrameIterator(activation);
        while (data_.interpFrames_.frame() != frame)
            ++data_.interpFrames_;

        
        JS_ASSERT(data_.interpFrames_.frame() == frame);
        data_.pc_ = data_.interpFrames_.pc();
        return;
      }
      case JIT:
#ifdef JS_ION
        if (data_.ionFrames_.isBaselineJS()) {
            ion::BaselineFrame *frame = data_.ionFrames_.baselineFrame();
            ion::JitActivation *activation = data_.activations_.activation()->asJit();

            
            
            data_.activations_ = ActivationIterator(data_.cx_->runtime());
            while (data_.activations_.activation() != activation)
                ++data_.activations_;

            
            data_.ionFrames_ = ion::IonFrameIterator(data_.activations_);
            while (!data_.ionFrames_.isBaselineJS() || data_.ionFrames_.baselineFrame() != frame)
                ++data_.ionFrames_;

            
            JS_ASSERT(data_.ionFrames_.baselineFrame() == frame);
            data_.ionFrames_.baselineScriptAndPc(NULL, &data_.pc_);
            return;
        }
#endif
        break;
    }
    JS_NOT_REACHED("Unexpected state");
}

JSFunction *
ScriptFrameIter::callee() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        JS_ASSERT(isFunctionFrame());
        return &interpFrame()->callee();
      case JIT:
#ifdef JS_ION
        if (data_.ionFrames_.isBaselineJS())
            return data_.ionFrames_.callee();
        JS_ASSERT(data_.ionFrames_.isOptimizedJS());
        return ionInlineFrames_.callee();
#else
        break;
#endif
    }
    JS_NOT_REACHED("Unexpected state");
    return NULL;
}

Value
ScriptFrameIter::calleev() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        JS_ASSERT(isFunctionFrame());
        return interpFrame()->calleev();
      case JIT:
#ifdef JS_ION
        return ObjectValue(*callee());
#else
        break;
#endif
    }
    JS_NOT_REACHED("Unexpected state");
    return Value();
}

unsigned
ScriptFrameIter::numActualArgs() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        JS_ASSERT(isFunctionFrame());
        return interpFrame()->numActualArgs();
      case JIT:
#ifdef JS_ION
        if (data_.ionFrames_.isOptimizedJS())
            return ionInlineFrames_.numActualArgs();

        JS_ASSERT(data_.ionFrames_.isBaselineJS());
        return data_.ionFrames_.numActualArgs();
#else
        break;
#endif
    }
    JS_NOT_REACHED("Unexpected state");
    return 0;
}

Value
ScriptFrameIter::unaliasedActual(unsigned i, MaybeCheckAliasing checkAliasing) const
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        return interpFrame()->unaliasedActual(i, checkAliasing);
      case JIT:
#ifdef JS_ION
        JS_ASSERT(data_.ionFrames_.isBaselineJS());
        return data_.ionFrames_.baselineFrame()->unaliasedActual(i, checkAliasing);
#else
        break;
#endif
    }
    JS_NOT_REACHED("Unexpected state");
    return NullValue();
}

JSObject *
ScriptFrameIter::scopeChain() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case JIT:
#ifdef JS_ION
        if (data_.ionFrames_.isOptimizedJS())
            return ionInlineFrames_.scopeChain();
        return data_.ionFrames_.baselineFrame()->scopeChain();
#else
        break;
#endif
      case SCRIPTED:
        return interpFrame()->scopeChain();
    }
    JS_NOT_REACHED("Unexpected state");
    return NULL;
}

CallObject &
ScriptFrameIter::callObj() const
{
    JS_ASSERT(callee()->isHeavyweight());

    JSObject *pobj = scopeChain();
    while (!pobj->is<CallObject>())
        pobj = pobj->enclosingScope();
    return pobj->as<CallObject>();
}

bool
ScriptFrameIter::hasArgsObj() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        return interpFrame()->hasArgsObj();
      case JIT:
#ifdef JS_ION
        JS_ASSERT(data_.ionFrames_.isBaselineJS());
        return data_.ionFrames_.baselineFrame()->hasArgsObj();
#else
        break;
#endif
    }
    JS_NOT_REACHED("Unexpected state");
    return false;
}

ArgumentsObject &
ScriptFrameIter::argsObj() const
{
    JS_ASSERT(hasArgsObj());

    switch (data_.state_) {
      case DONE:
        break;
      case JIT:
#ifdef JS_ION
        JS_ASSERT(data_.ionFrames_.isBaselineJS());
        return data_.ionFrames_.baselineFrame()->argsObj();
#else
        break;
#endif
      case SCRIPTED:
        return interpFrame()->argsObj();
    }
    JS_NOT_REACHED("Unexpected state");
    return interpFrame()->argsObj();
}

bool
ScriptFrameIter::computeThis() const
{
    JS_ASSERT(!done());
    if (!isIon()) {
        JS_ASSERT(data_.cx_);
        return ComputeThis(data_.cx_, abstractFramePtr());
    }
    return true;
}

Value
ScriptFrameIter::thisv() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case JIT:
#ifdef JS_ION
        if (data_.ionFrames_.isOptimizedJS())
            return ObjectValue(*ionInlineFrames_.thisObject());
        return data_.ionFrames_.baselineFrame()->thisValue();
#else
        break;
#endif
      case SCRIPTED:
        return interpFrame()->thisValue();
    }
    JS_NOT_REACHED("Unexpected state");
    return NullValue();
}

Value
ScriptFrameIter::returnValue() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case JIT:
#ifdef JS_ION
        if (data_.ionFrames_.isBaselineJS())
            return *data_.ionFrames_.baselineFrame()->returnValue();
#endif
        break;
      case SCRIPTED:
        return interpFrame()->returnValue();
    }
    JS_NOT_REACHED("Unexpected state");
    return NullValue();
}

void
ScriptFrameIter::setReturnValue(const Value &v)
{
    switch (data_.state_) {
      case DONE:
        break;
      case JIT:
#ifdef JS_ION
        if (data_.ionFrames_.isBaselineJS()) {
            data_.ionFrames_.baselineFrame()->setReturnValue(v);
            return;
        }
#endif
        break;
      case SCRIPTED:
        interpFrame()->setReturnValue(v);
        return;
    }
    JS_NOT_REACHED("Unexpected state");
}

size_t
ScriptFrameIter::numFrameSlots() const
{
    switch (data_.state_) {
      case DONE:
        break;
     case JIT: {
#ifdef JS_ION
        if (data_.ionFrames_.isOptimizedJS())
            return ionInlineFrames_.snapshotIterator().slots() - ionInlineFrames_.script()->nfixed;
        ion::BaselineFrame *frame = data_.ionFrames_.baselineFrame();
        return frame->numValueSlots() - data_.ionFrames_.script()->nfixed;
#else
        break;
#endif
      }
      case SCRIPTED:
        JS_ASSERT(data_.cx_);
        JS_ASSERT(data_.cx_->interpreterRegs().spForStackDepth(0) == interpFrame()->base());
        return data_.cx_->interpreterRegs().sp - interpFrame()->base();
    }
    JS_NOT_REACHED("Unexpected state");
    return 0;
}

Value
ScriptFrameIter::frameSlotValue(size_t index) const
{
    switch (data_.state_) {
      case DONE:
        break;
      case JIT:
#ifdef JS_ION
        if (data_.ionFrames_.isOptimizedJS()) {
            ion::SnapshotIterator si(ionInlineFrames_.snapshotIterator());
            index += ionInlineFrames_.script()->nfixed;
            return si.maybeReadSlotByIndex(index);
        }

        index += data_.ionFrames_.script()->nfixed;
        return *data_.ionFrames_.baselineFrame()->valueSlot(index);
#else
        break;
#endif
      case SCRIPTED:
          return interpFrame()->base()[index];
    }
    JS_NOT_REACHED("Unexpected state");
    return NullValue();
}

#if defined(_MSC_VER)
# pragma optimize("", on)
#endif



JSObject *
AbstractFramePtr::evalPrevScopeChain(JSContext *cx) const
{
    
    AllFramesIter iter(cx);
    while (iter.isIon() || iter.abstractFramePtr() != *this)
        ++iter;
    ++iter;
    return iter.scopeChain();
}

bool
AbstractFramePtr::hasPushedSPSFrame() const
{
    if (isStackFrame())
        return asStackFrame()->hasPushedSPSFrame();
#ifdef JS_ION
    return asBaselineFrame()->hasPushedSPSFrame();
#else
    JS_NOT_REACHED("Invalid frame");
    return false;
#endif
}

#ifdef DEBUG
void
js::CheckLocalUnaliased(MaybeCheckAliasing checkAliasing, JSScript *script,
                        StaticBlockObject *maybeBlock, unsigned i)
{
    if (!checkAliasing)
        return;

    JS_ASSERT(i < script->nslots);
    if (i < script->nfixed) {
        JS_ASSERT(!script->varIsAliased(i));
    } else {
        unsigned depth = i - script->nfixed;
        for (StaticBlockObject *b = maybeBlock; b; b = b->enclosingBlock()) {
            if (b->containsVarAtDepth(depth)) {
                JS_ASSERT(!b->isAliased(depth - b->stackDepth()));
                break;
            }
        }
    }
}
#endif

ion::JitActivation::JitActivation(JSContext *cx, bool firstFrameIsConstructing, bool active)
  : Activation(cx, Jit, active),
    prevIonTop_(cx->mainThread().ionTop),
    prevIonJSContext_(cx->mainThread().ionJSContext),
    firstFrameIsConstructing_(firstFrameIsConstructing)
{
    cx->mainThread().ionJSContext = cx;
}

ion::JitActivation::~JitActivation()
{
    cx_->mainThread().ionTop = prevIonTop_;
    cx_->mainThread().ionJSContext = prevIonJSContext_;
}

InterpreterFrameIterator &
InterpreterFrameIterator::operator++()
{
    JS_ASSERT(!done());
    if (fp_ != activation_->entry_) {
        pc_ = fp_->prevpc();
        sp_ = fp_->prevsp();
        fp_ = fp_->prev();
    } else {
        pc_ = NULL;
        sp_ = NULL;
        fp_ = NULL;
    }
    return *this;
}

ActivationIterator::ActivationIterator(JSRuntime *rt)
  : jitTop_(rt->mainThread.ionTop),
    activation_(rt->mainThread.activation_)
{
    settle();
}

ActivationIterator &
ActivationIterator::operator++()
{
    JS_ASSERT(activation_);
    if (activation_->isJit())
        jitTop_ = activation_->asJit()->prevIonTop();
    activation_ = activation_->prev();
    settle();
    return *this;
}

void
ActivationIterator::settle()
{
    while (!done() && !activation_->isActive()) {
        if (activation_->isJit())
            jitTop_ = activation_->asJit()->prevIonTop();
        activation_ = activation_->prev();
    }
}
