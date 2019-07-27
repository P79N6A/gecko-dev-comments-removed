





#ifndef vm_Stack_inl_h
#define vm_Stack_inl_h

#include "vm/Stack.h"

#include "mozilla/PodOperations.h"

#include "jscntxt.h"

#include "jit/BaselineFrame.h"
#include "jit/RematerializedFrame.h"
#include "vm/GeneratorObject.h"
#include "vm/ScopeObject.h"

#include "jsobjinlines.h"

#include "jit/BaselineFrame-inl.h"

namespace js {






static inline bool
IsCacheableNonGlobalScope(JSObject *obj)
{
    bool cacheable = (obj->is<CallObject>() || obj->is<BlockObject>() || obj->is<DeclEnvObject>());

    MOZ_ASSERT_IF(cacheable, !obj->getOps()->lookupProperty);
    return cacheable;
}

inline HandleObject
InterpreterFrame::scopeChain() const
{
    MOZ_ASSERT_IF(!(flags_ & HAS_SCOPECHAIN), isFunctionFrame());
    if (!(flags_ & HAS_SCOPECHAIN)) {
        scopeChain_ = callee().environment();
        flags_ |= HAS_SCOPECHAIN;
    }
    return HandleObject::fromMarkedLocation(&scopeChain_);
}

inline GlobalObject &
InterpreterFrame::global() const
{
    return scopeChain()->global();
}

inline JSObject &
InterpreterFrame::varObj()
{
    JSObject *obj = scopeChain();
    while (!obj->isQualifiedVarObj())
        obj = obj->enclosingScope();
    return *obj;
}

inline JSCompartment *
InterpreterFrame::compartment() const
{
    MOZ_ASSERT(scopeChain()->compartment() == script()->compartment());
    return scopeChain()->compartment();
}

inline void
InterpreterFrame::initCallFrame(JSContext *cx, InterpreterFrame *prev, jsbytecode *prevpc,
                                Value *prevsp, JSFunction &callee, JSScript *script, Value *argv,
                                uint32_t nactual, InterpreterFrame::Flags flagsArg)
{
    MOZ_ASSERT((flagsArg & ~CONSTRUCTING) == 0);
    MOZ_ASSERT(callee.nonLazyScript() == script);

    
    flags_ = FUNCTION | HAS_SCOPECHAIN | flagsArg;
    argv_ = argv;
    exec.fun = &callee;
    u.nactual = nactual;
    scopeChain_ = callee.environment();
    prev_ = prev;
    prevpc_ = prevpc;
    prevsp_ = prevsp;

    initLocals();
}

inline void
InterpreterFrame::initLocals()
{
    SetValueRangeToUndefined(slots(), script()->nfixedvars());

    
    
    
    
    
    
    
    Value *lexicalEnd = slots() + script()->fixedLexicalEnd();
    for (Value *lexical = slots() + script()->fixedLexicalBegin(); lexical != lexicalEnd; ++lexical)
        lexical->setMagic(JS_UNINITIALIZED_LEXICAL);
}

inline Value &
InterpreterFrame::unaliasedLocal(uint32_t i)
{
    MOZ_ASSERT(i < script()->nfixed());
    return slots()[i];
}

inline Value &
InterpreterFrame::unaliasedFormal(unsigned i, MaybeCheckAliasing checkAliasing)
{
    MOZ_ASSERT(i < numFormalArgs());
    MOZ_ASSERT_IF(checkAliasing, !script()->argsObjAliasesFormals());
    MOZ_ASSERT_IF(checkAliasing, !script()->formalIsAliased(i));
    return argv()[i];
}

inline Value &
InterpreterFrame::unaliasedActual(unsigned i, MaybeCheckAliasing checkAliasing)
{
    MOZ_ASSERT(i < numActualArgs());
    MOZ_ASSERT_IF(checkAliasing, !script()->argsObjAliasesFormals());
    MOZ_ASSERT_IF(checkAliasing && i < numFormalArgs(), !script()->formalIsAliased(i));
    return argv()[i];
}

template <class Op>
inline void
InterpreterFrame::unaliasedForEachActual(Op op)
{
    
    
    

    const Value *argsEnd = argv() + numActualArgs();
    for (const Value *p = argv(); p < argsEnd; ++p)
        op(*p);
}

struct CopyTo
{
    Value *dst;
    explicit CopyTo(Value *dst) : dst(dst) {}
    void operator()(const Value &src) { *dst++ = src; }
};

struct CopyToHeap
{
    HeapValue *dst;
    explicit CopyToHeap(HeapValue *dst) : dst(dst) {}
    void operator()(const Value &src) { dst->init(src); ++dst; }
};

inline ArgumentsObject &
InterpreterFrame::argsObj() const
{
    MOZ_ASSERT(script()->needsArgsObj());
    MOZ_ASSERT(flags_ & HAS_ARGS_OBJ);
    return *argsObj_;
}

inline void
InterpreterFrame::initArgsObj(ArgumentsObject &argsobj)
{
    MOZ_ASSERT(script()->needsArgsObj());
    flags_ |= HAS_ARGS_OBJ;
    argsObj_ = &argsobj;
}

inline ScopeObject &
InterpreterFrame::aliasedVarScope(ScopeCoordinate sc) const
{
    JSObject *scope = &scopeChain()->as<ScopeObject>();
    for (unsigned i = sc.hops(); i; i--)
        scope = &scope->as<ScopeObject>().enclosingScope();
    return scope->as<ScopeObject>();
}

inline void
InterpreterFrame::pushOnScopeChain(ScopeObject &scope)
{
    MOZ_ASSERT(*scopeChain() == scope.enclosingScope() ||
               *scopeChain() == scope.as<CallObject>().enclosingScope().as<DeclEnvObject>().enclosingScope());
    scopeChain_ = &scope;
    flags_ |= HAS_SCOPECHAIN;
}

inline void
InterpreterFrame::popOffScopeChain()
{
    MOZ_ASSERT(flags_ & HAS_SCOPECHAIN);
    scopeChain_ = &scopeChain_->as<ScopeObject>().enclosingScope();
}

bool
InterpreterFrame::hasCallObj() const
{
    MOZ_ASSERT(isStrictEvalFrame() || fun()->isHeavyweight());
    return flags_ & HAS_CALL_OBJ;
}

inline CallObject &
InterpreterFrame::callObj() const
{
    MOZ_ASSERT(fun()->isHeavyweight());

    JSObject *pobj = scopeChain();
    while (MOZ_UNLIKELY(!pobj->is<CallObject>()))
        pobj = pobj->enclosingScope();
    return pobj->as<CallObject>();
}



inline void
InterpreterStack::purge(JSRuntime *rt)
{
    rt->gc.freeUnusedLifoBlocksAfterSweeping(&allocator_);
}

uint8_t *
InterpreterStack::allocateFrame(JSContext *cx, size_t size)
{
    size_t maxFrames;
    if (cx->compartment()->principals == cx->runtime()->trustedPrincipals())
        maxFrames = MAX_FRAMES_TRUSTED;
    else
        maxFrames = MAX_FRAMES;

    if (MOZ_UNLIKELY(frameCount_ >= maxFrames)) {
        js_ReportOverRecursed(cx);
        return nullptr;
    }

    uint8_t *buffer = reinterpret_cast<uint8_t *>(allocator_.alloc(size));
    if (!buffer)
        return nullptr;

    frameCount_++;
    return buffer;
}

MOZ_ALWAYS_INLINE InterpreterFrame *
InterpreterStack::getCallFrame(JSContext *cx, const CallArgs &args, HandleScript script,
                               InterpreterFrame::Flags *flags, Value **pargv)
{
    JSFunction *fun = &args.callee().as<JSFunction>();

    MOZ_ASSERT(fun->nonLazyScript() == script);
    unsigned nformal = fun->nargs();
    unsigned nvals = script->nslots();

    if (args.length() >= nformal) {
        *pargv = args.array();
        uint8_t *buffer = allocateFrame(cx, sizeof(InterpreterFrame) + nvals * sizeof(Value));
        return reinterpret_cast<InterpreterFrame *>(buffer);
    }

    
    MOZ_ASSERT(args.length() < nformal);

    nvals += nformal + 2; 
    uint8_t *buffer = allocateFrame(cx, sizeof(InterpreterFrame) + nvals * sizeof(Value));
    if (!buffer)
        return nullptr;

    Value *argv = reinterpret_cast<Value *>(buffer);
    unsigned nmissing = nformal - args.length();

    mozilla::PodCopy(argv, args.base(), 2 + args.length());
    SetValueRangeToUndefined(argv + 2 + args.length(), nmissing);

    *pargv = argv + 2;
    return reinterpret_cast<InterpreterFrame *>(argv + 2 + nformal);
}

MOZ_ALWAYS_INLINE bool
InterpreterStack::pushInlineFrame(JSContext *cx, InterpreterRegs &regs, const CallArgs &args,
                                  HandleScript script, InitialFrameFlags initial)
{
    RootedFunction callee(cx, &args.callee().as<JSFunction>());
    MOZ_ASSERT(regs.sp == args.end());
    MOZ_ASSERT(callee->nonLazyScript() == script);

    script->ensureNonLazyCanonicalFunction(cx);

    InterpreterFrame *prev = regs.fp();
    jsbytecode *prevpc = regs.pc;
    Value *prevsp = regs.sp;
    MOZ_ASSERT(prev);

    LifoAlloc::Mark mark = allocator_.mark();

    InterpreterFrame::Flags flags = ToFrameFlags(initial);
    Value *argv;
    InterpreterFrame *fp = getCallFrame(cx, args, script, &flags, &argv);
    if (!fp)
        return false;

    fp->mark_ = mark;

    
    fp->initCallFrame(cx, prev, prevpc, prevsp, *callee, script, argv, args.length(), flags);

    regs.prepareToRun(*fp, script);
    return true;
}

MOZ_ALWAYS_INLINE bool
InterpreterStack::resumeGeneratorCallFrame(JSContext *cx, InterpreterRegs &regs,
                                           HandleFunction callee, HandleValue thisv,
                                           HandleObject scopeChain)
{
    MOZ_ASSERT(callee->isGenerator());
    RootedScript script(cx, callee->getOrCreateScript(cx));
    InterpreterFrame *prev = regs.fp();
    jsbytecode *prevpc = regs.pc;
    Value *prevsp = regs.sp;
    MOZ_ASSERT(prev);

    script->ensureNonLazyCanonicalFunction(cx);

    LifoAlloc::Mark mark = allocator_.mark();

    
    unsigned nformal = callee->nargs();
    unsigned nvals = 2 + nformal + script->nslots();

    uint8_t *buffer = allocateFrame(cx, sizeof(InterpreterFrame) + nvals * sizeof(Value));
    if (!buffer)
        return false;

    Value *argv = reinterpret_cast<Value *>(buffer) + 2;
    argv[-2] = ObjectValue(*callee);
    argv[-1] = thisv;
    SetValueRangeToUndefined(argv, nformal);

    InterpreterFrame *fp = reinterpret_cast<InterpreterFrame *>(argv + nformal);
    InterpreterFrame::Flags flags = ToFrameFlags(INITIAL_NONE);
    fp->mark_ = mark;
    fp->initCallFrame(cx, prev, prevpc, prevsp, *callee, script, argv, 0, flags);
    fp->resumeGeneratorFrame(scopeChain);

    regs.prepareToRun(*fp, script);
    return true;
}

MOZ_ALWAYS_INLINE void
InterpreterStack::popInlineFrame(InterpreterRegs &regs)
{
    InterpreterFrame *fp = regs.fp();
    regs.popInlineFrame();
    regs.sp[-1] = fp->returnValue();
    releaseFrame(fp);
    MOZ_ASSERT(regs.fp());
}

template <class Op>
inline void
FrameIter::unaliasedForEachActual(JSContext *cx, Op op)
{
    switch (data_.state_) {
      case DONE:
      case ASMJS:
        break;
      case INTERP:
        interpFrame()->unaliasedForEachActual(op);
        return;
      case JIT:
        if (data_.jitFrames_.isIonJS()) {
            jit::MaybeReadFallback recover(cx, activation()->asJit(), &data_.jitFrames_);
            ionInlineFrames_.unaliasedForEachActual(cx, op, jit::ReadFrame_Actuals, recover);
        } else if (data_.jitFrames_.isBailoutJS()) {
            
            
            
            
            
            
            jit::MaybeReadFallback fallback;
            ionInlineFrames_.unaliasedForEachActual(cx, op, jit::ReadFrame_Actuals, fallback);
        } else {
            MOZ_ASSERT(data_.jitFrames_.isBaselineJS());
            data_.jitFrames_.unaliasedForEachActual(op, jit::ReadFrame_Actuals);
        }
        return;
    }
    MOZ_CRASH("Unexpected state");
}

inline HandleValue
AbstractFramePtr::returnValue() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->returnValue();
    return asBaselineFrame()->returnValue();
}

inline void
AbstractFramePtr::setReturnValue(const Value &rval) const
{
    if (isInterpreterFrame()) {
        asInterpreterFrame()->setReturnValue(rval);
        return;
    }
    asBaselineFrame()->setReturnValue(rval);
}

inline JSObject *
AbstractFramePtr::scopeChain() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->scopeChain();
    if (isBaselineFrame())
        return asBaselineFrame()->scopeChain();
    return asRematerializedFrame()->scopeChain();
}

inline void
AbstractFramePtr::pushOnScopeChain(ScopeObject &scope)
{
    if (isInterpreterFrame()) {
        asInterpreterFrame()->pushOnScopeChain(scope);
        return;
    }
    asBaselineFrame()->pushOnScopeChain(scope);
}

inline CallObject &
AbstractFramePtr::callObj() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->callObj();
    if (isBaselineFrame())
        return asBaselineFrame()->callObj();
    return asRematerializedFrame()->callObj();
}

inline bool
AbstractFramePtr::initFunctionScopeObjects(JSContext *cx)
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->initFunctionScopeObjects(cx);
    return asBaselineFrame()->initFunctionScopeObjects(cx);
}

inline JSCompartment *
AbstractFramePtr::compartment() const
{
    return scopeChain()->compartment();
}

inline unsigned
AbstractFramePtr::numActualArgs() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->numActualArgs();
    if (isBaselineFrame())
        return asBaselineFrame()->numActualArgs();
    return asRematerializedFrame()->numActualArgs();
}

inline unsigned
AbstractFramePtr::numFormalArgs() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->numFormalArgs();
    if (isBaselineFrame())
        return asBaselineFrame()->numFormalArgs();
    return asRematerializedFrame()->numFormalArgs();
}

inline Value &
AbstractFramePtr::unaliasedLocal(uint32_t i)
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->unaliasedLocal(i);
    if (isBaselineFrame())
        return asBaselineFrame()->unaliasedLocal(i);
    return asRematerializedFrame()->unaliasedLocal(i);
}

inline Value &
AbstractFramePtr::unaliasedFormal(unsigned i, MaybeCheckAliasing checkAliasing)
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->unaliasedFormal(i, checkAliasing);
    if (isBaselineFrame())
        return asBaselineFrame()->unaliasedFormal(i, checkAliasing);
    return asRematerializedFrame()->unaliasedFormal(i, checkAliasing);
}

inline Value &
AbstractFramePtr::unaliasedActual(unsigned i, MaybeCheckAliasing checkAliasing)
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->unaliasedActual(i, checkAliasing);
    if (isBaselineFrame())
        return asBaselineFrame()->unaliasedActual(i, checkAliasing);
    return asRematerializedFrame()->unaliasedActual(i, checkAliasing);
}

inline bool
AbstractFramePtr::hasCallObj() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->hasCallObj();
    if (isBaselineFrame())
        return asBaselineFrame()->hasCallObj();
    return asRematerializedFrame()->hasCallObj();
}

inline bool
AbstractFramePtr::useNewType() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->useNewType();
    return false;
}

inline bool
AbstractFramePtr::isGeneratorFrame() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->isGeneratorFrame();
    return false;
}

inline bool
AbstractFramePtr::isFunctionFrame() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->isFunctionFrame();
    if (isBaselineFrame())
        return asBaselineFrame()->isFunctionFrame();
    return asRematerializedFrame()->isFunctionFrame();
}

inline bool
AbstractFramePtr::isGlobalFrame() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->isGlobalFrame();
    if (isBaselineFrame())
        return asBaselineFrame()->isGlobalFrame();
    return asRematerializedFrame()->isGlobalFrame();
}

inline bool
AbstractFramePtr::isEvalFrame() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->isEvalFrame();
    if (isBaselineFrame())
        return asBaselineFrame()->isEvalFrame();
    MOZ_ASSERT(isRematerializedFrame());
    return false;
}
inline bool
AbstractFramePtr::isDebuggerFrame() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->isDebuggerFrame();
    if (isBaselineFrame())
        return asBaselineFrame()->isDebuggerFrame();
    MOZ_ASSERT(isRematerializedFrame());
    return false;
}

inline bool
AbstractFramePtr::hasArgs() const {
    return isNonEvalFunctionFrame();
}

inline JSScript *
AbstractFramePtr::script() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->script();
    if (isBaselineFrame())
        return asBaselineFrame()->script();
    return asRematerializedFrame()->script();
}

inline JSFunction *
AbstractFramePtr::fun() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->fun();
    if (isBaselineFrame())
        return asBaselineFrame()->fun();
    return asRematerializedFrame()->fun();
}

inline JSFunction *
AbstractFramePtr::maybeFun() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->maybeFun();
    if (isBaselineFrame())
        return asBaselineFrame()->maybeFun();
    return asRematerializedFrame()->maybeFun();
}

inline JSFunction *
AbstractFramePtr::callee() const
{
    if (isInterpreterFrame())
        return &asInterpreterFrame()->callee();
    if (isBaselineFrame())
        return asBaselineFrame()->callee();
    return asRematerializedFrame()->callee();
}

inline Value
AbstractFramePtr::calleev() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->calleev();
    if (isBaselineFrame())
        return asBaselineFrame()->calleev();
    return asRematerializedFrame()->calleev();
}

inline bool
AbstractFramePtr::isNonEvalFunctionFrame() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->isNonEvalFunctionFrame();
    if (isBaselineFrame())
        return asBaselineFrame()->isNonEvalFunctionFrame();
    return asRematerializedFrame()->isNonEvalFunctionFrame();
}

inline bool
AbstractFramePtr::isNonStrictDirectEvalFrame() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->isNonStrictDirectEvalFrame();
    if (isBaselineFrame())
        return asBaselineFrame()->isNonStrictDirectEvalFrame();
    MOZ_ASSERT(isRematerializedFrame());
    return false;
}

inline bool
AbstractFramePtr::isStrictEvalFrame() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->isStrictEvalFrame();
    if (isBaselineFrame())
        return asBaselineFrame()->isStrictEvalFrame();
    MOZ_ASSERT(isRematerializedFrame());
    return false;
}

inline Value *
AbstractFramePtr::argv() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->argv();
    if (isBaselineFrame())
        return asBaselineFrame()->argv();
    return asRematerializedFrame()->argv();
}

inline bool
AbstractFramePtr::hasArgsObj() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->hasArgsObj();
    if (isBaselineFrame())
        return asBaselineFrame()->hasArgsObj();
    return asRematerializedFrame()->hasArgsObj();
}

inline ArgumentsObject &
AbstractFramePtr::argsObj() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->argsObj();
    if (isBaselineFrame())
        return asBaselineFrame()->argsObj();
    return asRematerializedFrame()->argsObj();
}

inline void
AbstractFramePtr::initArgsObj(ArgumentsObject &argsobj) const
{
    if (isInterpreterFrame()) {
        asInterpreterFrame()->initArgsObj(argsobj);
        return;
    }
    asBaselineFrame()->initArgsObj(argsobj);
}

inline bool
AbstractFramePtr::copyRawFrameSlots(AutoValueVector *vec) const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->copyRawFrameSlots(vec);
    return asBaselineFrame()->copyRawFrameSlots(vec);
}

inline bool
AbstractFramePtr::prevUpToDate() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->prevUpToDate();
    if (isBaselineFrame())
        return asBaselineFrame()->prevUpToDate();
    return asRematerializedFrame()->prevUpToDate();
}

inline void
AbstractFramePtr::setPrevUpToDate() const
{
    if (isInterpreterFrame()) {
        asInterpreterFrame()->setPrevUpToDate();
        return;
    }
    if (isBaselineFrame()) {
        asBaselineFrame()->setPrevUpToDate();
        return;
    }
    asRematerializedFrame()->setPrevUpToDate();
}

inline Value &
AbstractFramePtr::thisValue() const
{
    if (isInterpreterFrame())
        return asInterpreterFrame()->thisValue();
    if (isBaselineFrame())
        return asBaselineFrame()->thisValue();
    return asRematerializedFrame()->thisValue();
}

inline void
AbstractFramePtr::popBlock(JSContext *cx) const
{
    if (isInterpreterFrame()) {
        asInterpreterFrame()->popBlock(cx);
        return;
    }
    asBaselineFrame()->popBlock(cx);
}

inline void
AbstractFramePtr::popWith(JSContext *cx) const
{
    if (isInterpreterFrame()) {
        asInterpreterFrame()->popWith(cx);
        return;
    }
    asBaselineFrame()->popWith(cx);
}

Activation::Activation(ThreadSafeContext *cx, Kind kind)
  : cx_(cx),
    compartment_(cx->compartment_),
    prev_(cx->perThreadData->activation_),
    prevProfiling_(prev_ ? prev_->mostRecentProfiling() : nullptr),
    savedFrameChain_(0),
    hideScriptedCallerCount_(0),
    kind_(kind)
{
    cx->perThreadData->activation_ = this;
}

Activation::~Activation()
{
    MOZ_ASSERT_IF(isProfiling(), this != cx_->perThreadData->profilingActivation_);
    MOZ_ASSERT(cx_->perThreadData->activation_ == this);
    MOZ_ASSERT(hideScriptedCallerCount_ == 0);
    cx_->perThreadData->activation_ = prev_;
}

bool
Activation::isProfiling() const
{
    if (isInterpreter())
        return asInterpreter()->isProfiling();

    if (isJit())
        return asJit()->isProfiling();

    if (isForkJoin())
        return asForkJoin()->isProfiling();

    MOZ_ASSERT(isAsmJS());
    return asAsmJS()->isProfiling();
}

Activation *
Activation::mostRecentProfiling()
{
    if (isProfiling())
        return this;
    return prevProfiling_;
}

InterpreterActivation::InterpreterActivation(RunState &state, JSContext *cx,
                                             InterpreterFrame *entryFrame)
  : Activation(cx, Interpreter),
    entryFrame_(entryFrame),
    opMask_(0)
#ifdef DEBUG
  , oldFrameCount_(cx->runtime()->interpreterStack().frameCount_)
#endif
{
    regs_.prepareToRun(*entryFrame, state.script());
    MOZ_ASSERT(regs_.pc == state.script()->code());
    MOZ_ASSERT_IF(entryFrame_->isEvalFrame(), state.script()->isActiveEval());
}

InterpreterActivation::~InterpreterActivation()
{
    
    while (regs_.fp() != entryFrame_)
        popInlineFrame(regs_.fp());

    JSContext *cx = cx_->asJSContext();
    MOZ_ASSERT(oldFrameCount_ == cx->runtime()->interpreterStack().frameCount_);
    MOZ_ASSERT_IF(oldFrameCount_ == 0, cx->runtime()->interpreterStack().allocator_.used() == 0);

    if (entryFrame_)
        cx->runtime()->interpreterStack().releaseFrame(entryFrame_);
}

inline bool
InterpreterActivation::pushInlineFrame(const CallArgs &args, HandleScript script,
                                       InitialFrameFlags initial)
{
    JSContext *cx = cx_->asJSContext();
    if (!cx->runtime()->interpreterStack().pushInlineFrame(cx, regs_, args, script, initial))
        return false;
    MOZ_ASSERT(regs_.fp()->script()->compartment() == compartment());
    return true;
}

inline void
InterpreterActivation::popInlineFrame(InterpreterFrame *frame)
{
    (void)frame; 
    MOZ_ASSERT(regs_.fp() == frame);
    MOZ_ASSERT(regs_.fp() != entryFrame_);

    cx_->asJSContext()->runtime()->interpreterStack().popInlineFrame(regs_);
}

inline bool
InterpreterActivation::resumeGeneratorFrame(HandleFunction callee, HandleValue thisv,
                                            HandleObject scopeChain)
{
    InterpreterStack &stack = cx_->asJSContext()->runtime()->interpreterStack();
    if (!stack.resumeGeneratorCallFrame(cx_->asJSContext(), regs_, callee, thisv, scopeChain))
        return false;

    MOZ_ASSERT(regs_.fp()->script()->compartment() == compartment_);
    return true;
}

inline JSContext *
AsmJSActivation::cx()
{
    return cx_->asJSContext();
}

} 

#endif 
