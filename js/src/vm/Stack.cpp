





#include "mozilla/DebugOnly.h"
#include "mozilla/PodOperations.h"

#include "jscntxt.h"
#include "gc/Marking.h"
#include "methodjit/MethodJIT.h"
#ifdef JS_ION
#include "ion/BaselineFrame.h"
#include "ion/IonFrames.h"
#include "ion/IonCompartment.h"
#include "ion/Bailouts.h"
#endif
#include "Stack.h"
#include "ForkJoin.h"

#include "jsgcinlines.h"
#include "jsobjinlines.h"
#include "jsinterpinlines.h"

#include "jsopcode.h"

#include "Stack-inl.h"


#ifdef XP_WIN
# include "jswin.h"
#elif defined(XP_OS2)
# define INCL_DOSMEMMGR
# include <os2.h>
#else
# include <unistd.h>
# include <sys/mman.h>
# if !defined(MAP_ANONYMOUS)
#  if defined(MAP_ANON)
#   define MAP_ANONYMOUS MAP_ANON
#  else
#   define MAP_ANONYMOUS 0
#  endif
# endif
#endif

using namespace js;

using mozilla::DebugOnly;
using mozilla::PodCopy;



void
StackFrame::initExecuteFrame(JSScript *script, StackFrame *prevLink, AbstractFramePtr prev,
                             FrameRegs *regs, const Value &thisv, JSObject &scopeChain,
                             ExecuteType type)
{
     



    JS_ASSERT_IF(prev.isStackFrame(), prev.asStackFrame() == prevLink);
    JS_ASSERT_IF(prev, prevLink != NULL);

    




    flags_ = type | HAS_SCOPECHAIN | HAS_BLOCKCHAIN | HAS_PREVPC;
    if (!(flags_ & GLOBAL)) {
        JS_ASSERT(prev.isFunctionFrame() || prev.isGlobalFrame());
        flags_ |= prev.isFunctionFrame() ? FUNCTION : GLOBAL;
    }

    Value *dstvp = (Value *)this - 2;
    dstvp[1] = thisv;

    if (isFunctionFrame()) {
        dstvp[0] = prev.calleev();
        exec.fun = prev.fun();
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
    prev_ = prevLink;
    prevpc_ = regs ? regs->pc : (jsbytecode *)0xbad;
    prevInline_ = regs ? regs->inlined() : NULL;
    blockChain_ = NULL;

#ifdef JS_ION
    
    JS_ASSERT_IF(isDebuggerFrame(), isEvalFrame());
    prevBaselineFrame_ = (isEvalFrame() && prev.isBaselineFrame()) ? prev.asBaselineFrame() : NULL;
#endif

#ifdef DEBUG
    ncode_ = (void *)0xbad;
    Debug_SetValueRangeToCrashOnTouch(&rval_, 1);
    hookData_ = (void *)0xbad;
#endif
}

template <StackFrame::TriggerPostBarriers doPostBarrier>
void
StackFrame::copyFrameAndValues(JSContext *cx, Value *vp, StackFrame *otherfp,
                               const Value *othervp, Value *othersp)
{
    JS_ASSERT(vp == (Value *)this - ((Value *)otherfp - othervp));
    JS_ASSERT(othervp == otherfp->generatorArgsSnapshotBegin());
    JS_ASSERT(othersp >= otherfp->slots());
    JS_ASSERT(othersp <= otherfp->generatorSlotsSnapshotBegin() + otherfp->script()->nslots);
    JS_ASSERT((Value *)this - vp == (Value *)otherfp - othervp);

    
    const Value *srcend = otherfp->generatorArgsSnapshotEnd();
    Value *dst = vp;
    for (const Value *src = othervp; src < srcend; src++, dst++) {
        *dst = *src;
        if (doPostBarrier)
            HeapValue::writeBarrierPost(*dst, dst);
    }

    *this = *otherfp;
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

    if (cx->compartment->debugMode())
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
    



    if (!isGeneratorFrame() || rt->stackSpace.containsFast(this))
        return NULL;

    



    char *vp = reinterpret_cast<char *>(generatorArgsSnapshotBegin());
    char *p = vp - offsetof(JSGenerator, stackSnapshot);
    JSGenerator *gen = reinterpret_cast<JSGenerator *>(p);
    JS_ASSERT(gen->fp == this);
    return gen;
}

jsbytecode *
StackFrame::prevpcSlow(InlinedSite **pinlined)
{
    JS_ASSERT(!(flags_ & HAS_PREVPC));
#if defined(JS_METHODJIT) && defined(JS_MONOIC)
    StackFrame *p = prev();
    mjit::JITScript *jit = p->script()->getJIT(p->isConstructing(),
                                               p->compartment()->zone()->compileBarriers());
    prevpc_ = jit->nativeToPC(ncode_, &prevInline_);
    flags_ |= HAS_PREVPC;
    if (pinlined)
        *pinlined = prevInline_;
    return prevpc_;
#else
    JS_NOT_REACHED("Unknown PC for frame");
    return NULL;
#endif
}

jsbytecode *
StackFrame::pcQuadratic(const ContextStack &stack, size_t maxDepth)
{
    StackSegment &seg = stack.space().containingSegment(this);
    FrameRegs &regs = seg.regs();

    



    if (regs.fp() == this)
        return regs.pc;

    




    if (StackFrame *next = seg.computeNextFrame(this, maxDepth))
        return next->prevpc();

    
    return regs.fp()->script()->code;
}

bool
StackFrame::copyRawFrameSlots(AutoValueVector *vec)
{
    if (!vec->resize(numFormalArgs() + script()->nfixed))
        return false;
    PodCopy(vec->begin(), formals(), numFormalArgs());
    PodCopy(vec->begin() + numFormalArgs(), slots(), script()->nfixed);
    return true;
}

static void
CleanupTornValue(StackFrame *fp, Value *vp)
{
    if (vp->isObject() && !vp->toGCThing())
        vp->setObject(fp->global());
    if (vp->isString() && !vp->toGCThing())
        vp->setString(fp->compartment()->rt->emptyString);
}

void
StackFrame::cleanupTornValues()
{
    for (size_t i = 0; i < numFormalArgs(); i++)
        CleanupTornValue(this, &formals()[i]);
    for (size_t i = 0; i < script()->nfixed; i++)
        CleanupTornValue(this, &slots()[i]);
}

static inline void
AssertDynamicScopeMatchesStaticScope(JSContext *cx, JSScript *script, JSObject *scope)
{
#ifdef DEBUG
    RootedObject enclosingScope(cx, script->enclosingStaticScope());
    for (StaticScopeIter i(cx, enclosingScope); !i.done(); i++) {
        if (i.hasDynamicScopeObject()) {
            



            while (scope->isWith())
                scope = &scope->asWith().enclosingScope();

            switch (i.type()) {
              case StaticScopeIter::BLOCK:
                JS_ASSERT(i.block() == scope->asClonedBlock().staticBlock());
                scope = &scope->asClonedBlock().enclosingScope();
                break;
              case StaticScopeIter::FUNCTION:
                JS_ASSERT(scope->asCall().callee().nonLazyScript() == i.funScript());
                scope = &scope->asCall().enclosingScope();
                break;
              case StaticScopeIter::NAMED_LAMBDA:
                scope = &scope->asDeclEnv().enclosingScope();
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
    JS_ASSERT(cx->regs().pc == script->code);

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
            JS_ASSERT_IF(hasCallObj(), scopeChain()->asCall().isForEval());
            if (cx->compartment->debugMode())
                DebugScopes::onPopStrictEvalScope(this);
        } else if (isDirectEvalFrame()) {
            if (isDebuggerFrame())
                JS_ASSERT(!scopeChain()->isScope());
        } else {
            






            if (isDebuggerFrame())
                JS_ASSERT(scopeChain()->isGlobal() || scopeChain()->enclosingScope()->isGlobal());
            else
                JS_ASSERT(scopeChain()->isGlobal());
        }
        return;
    }

    if (isGlobalFrame()) {
        JS_ASSERT(!scopeChain()->isScope());
        return;
    }

    JS_ASSERT(isNonEvalFunctionFrame());

    if (fun()->isHeavyweight())
        JS_ASSERT_IF(hasCallObj(), scopeChain()->asCall().callee().nonLazyScript() == script);
    else
        AssertDynamicScopeMatchesStaticScope(cx, script, scopeChain());

    if (cx->compartment->debugMode())
        DebugScopes::onPopCall(this, cx);

    if (isConstructing() && thisValue().isObject() && returnValue().isPrimitive())
        setReturnValue(ObjectValue(constructorThis()));
}

bool
StackFrame::jitStrictEvalPrologue(JSContext *cx)
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

    if (cx->compartment->debugMode())
        DebugScopes::onPopBlock(cx, this);

    if (blockChain_->needsClone()) {
        JS_ASSERT(scopeChain_->asClonedBlock().staticBlock() == *blockChain_);
        popOffScopeChain();
    }

    blockChain_ = blockChain_->enclosingBlock();
}

void
StackFrame::popWith(JSContext *cx)
{
    if (cx->compartment->debugMode())
        DebugScopes::onPopWith(this);

    JS_ASSERT(scopeChain()->isWith());
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



bool
StackSegment::contains(const StackFrame *fp) const
{
    
    return (Value *)fp >= slotsBegin() && (Value *)fp <= (Value *)maybefp();
}

bool
StackSegment::contains(const FrameRegs *regs) const
{
    return regs && contains(regs->fp());
}

StackFrame *
StackSegment::computeNextFrame(const StackFrame *f, size_t maxDepth) const
{
    JS_ASSERT(contains(f) && f != fp());

    StackFrame *next = fp();
    for (size_t i = 0; i <= maxDepth; ++i) {
        if (next->prev() == f)
            return next;
        next = next->prev();
    }

    return NULL;
}

Value *
StackSegment::end() const
{
    
    JS_ASSERT_IF(regs_, contains(regs_));
    Value *p = regs_ ? regs_->sp : slotsBegin();
    if (invokeArgsEnd_ > p)
        p = invokeArgsEnd_;
    JS_ASSERT(p >= slotsBegin());
    return p;
}

FrameRegs *
StackSegment::pushRegs(FrameRegs &regs)
{
    JS_ASSERT_IF(contains(regs_), regs.fp()->prev() == regs_->fp());
    FrameRegs *prev = regs_;
    regs_ = &regs;
    return prev;
}

void
StackSegment::popRegs(FrameRegs *regs)
{
    JS_ASSERT_IF(regs && contains(regs->fp()), regs->fp() == regs_->fp()->prev());
    regs_ = regs;
}



StackSpace::StackSpace()
  : seg_(NULL),
    base_(NULL),
    conservativeEnd_(NULL),
#ifdef XP_WIN
    commitEnd_(NULL),
#endif
    defaultEnd_(NULL),
    trustedEnd_(NULL)
{
    assertInvariants();
}

bool
StackSpace::init()
{
    void *p;
#ifdef XP_WIN
    p = VirtualAlloc(NULL, CAPACITY_BYTES, MEM_RESERVE, PAGE_READWRITE);
    if (!p)
        return false;
    void *check = VirtualAlloc(p, COMMIT_BYTES, MEM_COMMIT, PAGE_READWRITE);
    if (p != check)
        return false;
    base_ = reinterpret_cast<Value *>(p);
    conservativeEnd_ = commitEnd_ = base_ + COMMIT_VALS;
    trustedEnd_ = base_ + CAPACITY_VALS;
    defaultEnd_ = trustedEnd_ - BUFFER_VALS;
    Debug_SetValueRangeToCrashOnTouch(base_, commitEnd_);
#elif defined(XP_OS2)
    if (DosAllocMem(&p, CAPACITY_BYTES, PAG_COMMIT | PAG_READ | PAG_WRITE | OBJ_ANY) &&
        DosAllocMem(&p, CAPACITY_BYTES, PAG_COMMIT | PAG_READ | PAG_WRITE))
        return false;
    base_ = reinterpret_cast<Value *>(p);
    trustedEnd_ = base_ + CAPACITY_VALS;
    conservativeEnd_ = defaultEnd_ = trustedEnd_ - BUFFER_VALS;
    Debug_SetValueRangeToCrashOnTouch(base_, trustedEnd_);
#else
    JS_ASSERT(CAPACITY_BYTES % getpagesize() == 0);
    p = mmap(NULL, CAPACITY_BYTES, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED)
        return false;
    base_ = reinterpret_cast<Value *>(p);
    trustedEnd_ = base_ + CAPACITY_VALS;
    conservativeEnd_ = defaultEnd_ = trustedEnd_ - BUFFER_VALS;
    Debug_SetValueRangeToCrashOnTouch(base_, trustedEnd_);
#endif
    assertInvariants();
    return true;
}

StackSpace::~StackSpace()
{
    assertInvariants();
    JS_ASSERT(!seg_);
    if (!base_)
        return;
#ifdef XP_WIN
    VirtualFree(base_, (commitEnd_ - base_) * sizeof(Value), MEM_DECOMMIT);
    VirtualFree(base_, 0, MEM_RELEASE);
#elif defined(XP_OS2)
    DosFreeMem(base_);
#else
#ifdef SOLARIS
    munmap((caddr_t)base_, CAPACITY_BYTES);
#else
    munmap(base_, CAPACITY_BYTES);
#endif
#endif
}

StackSegment &
StackSpace::containingSegment(const StackFrame *target) const
{
    for (StackSegment *s = seg_; s; s = s->prevInMemory()) {
        if (s->contains(target))
            return *s;
    }
    JS_NOT_REACHED("frame not in stack space");
    return *(StackSegment *)NULL;
}

void
StackSpace::markFrame(JSTracer *trc, StackFrame *fp, Value *slotsEnd)
{
    





    Value *slotsBegin = fp->slots();
    gc::MarkValueRootRangeMaybeNullPayload(trc, slotsEnd - slotsBegin, slotsBegin, "vm_stack");
}

void
StackSpace::mark(JSTracer *trc)
{
    
    Value *nextSegEnd = firstUnused();
    for (StackSegment *seg = seg_; seg; seg = seg->prevInMemory()) {
        









        Value *slotsEnd = nextSegEnd;
        for (StackFrame *fp = seg->maybefp(); (Value *)fp > (Value *)seg; fp = fp->prev()) {
            
            markFrame(trc, fp, slotsEnd);

            fp->mark(trc);
            slotsEnd = (Value *)fp;

            InlinedSite *site;
            fp->prevpc(&site);
            JS_ASSERT_IF(fp->prev(), !site);
        }
        gc::MarkValueRootRange(trc, seg->slotsBegin(), slotsEnd, "vm_stack");
        nextSegEnd = (Value *)seg;
    }
}

void
StackSpace::markActiveCompartments()
{
    for (StackSegment *seg = seg_; seg; seg = seg->prevInMemory()) {
        for (StackFrame *fp = seg->maybefp(); (Value *)fp > (Value *)seg; fp = fp->prev())
            MarkCompartmentActive(fp);
    }
}

JS_FRIEND_API(bool)
StackSpace::ensureSpaceSlow(JSContext *cx, MaybeReportError report, Value *from, ptrdiff_t nvals) const
{
    assertInvariants();

    JSCompartment *dest = cx->compartment;
    bool trusted = dest->principals == cx->runtime->trustedPrincipals();
    Value *end = trusted ? trustedEnd_ : defaultEnd_;

    





    if (end - from < nvals) {
        if (report)
            js_ReportOverRecursed(cx);
        return false;
    }

#ifdef XP_WIN
    if (commitEnd_ - from < nvals) {
        Value *newCommit = commitEnd_;
        Value *request = from + nvals;

        
        JS_ASSERT((trustedEnd_ - newCommit) % COMMIT_VALS == 0);
        do {
            newCommit += COMMIT_VALS;
            JS_ASSERT((trustedEnd_ - newCommit) >= 0);
        } while (newCommit < request);

        
        int32_t size = static_cast<int32_t>(newCommit - commitEnd_) * sizeof(Value);

        if (!VirtualAlloc(commitEnd_, size, MEM_COMMIT, PAGE_READWRITE)) {
            if (report)
                js_ReportOverRecursed(cx);
            return false;
        }

        Debug_SetValueRangeToCrashOnTouch(commitEnd_, newCommit);

        commitEnd_ = newCommit;
        conservativeEnd_ = Min(commitEnd_, defaultEnd_);
        assertInvariants();
    }
#endif

    return true;
}

bool
StackSpace::tryBumpLimit(JSContext *cx, Value *from, unsigned nvals, Value **limit)
{
    if (!ensureSpace(cx, REPORT_ERROR, from, nvals))
        return false;
    *limit = conservativeEnd_;
    return true;
}

size_t
StackSpace::sizeOf()
{
#if defined(XP_UNIX)
    









    const int pageSize = getpagesize();
    size_t numBytes = (trustedEnd_ - base_) * sizeof(Value);
    size_t numPages = (numBytes + pageSize - 1) / pageSize;

    
#ifdef __linux__
    typedef unsigned char MincoreArgType;
#else
    typedef char MincoreArgType;
#endif

    MincoreArgType *vec = (MincoreArgType *) js_malloc(numPages);
    int result = mincore(base_, numBytes, vec);
    if (result) {
        js_free(vec);
        



        return (trustedEnd_ - base_) * sizeof(Value);
    }

    size_t residentBytes = 0;
    for (size_t i = 0; i < numPages; i++) {
        
        if (vec[i] & 0x1)
            residentBytes += pageSize;
    }
    js_free(vec);
    return residentBytes;

#elif defined(XP_WIN)
    return (commitEnd_ - base_) * sizeof(Value);
#else
    



    return (trustedEnd_ - base_) * sizeof(Value);
#endif
}

#ifdef DEBUG
bool
StackSpace::containsSlow(StackFrame *fp)
{
    if (!seg_)
        return false;
    for (AllFramesIter i(seg_->cx()->runtime); !i.done(); ++i) {
        



        if (i.isIon())
            continue;
        if (i.interpFrame() == fp)
            return true;
    }
    return false;
}
#endif



ContextStack::ContextStack(JSContext *cx)
  : seg_(NULL),
    space_(&cx->runtime->stackSpace),
    cx_(cx)
{}

ContextStack::~ContextStack()
{
    JS_ASSERT(!seg_);
}

bool
ContextStack::onTop() const
{
    return seg_ && seg_ == space().seg_;
}










Value *
ContextStack::ensureOnTop(JSContext *cx, MaybeReportError report, unsigned nvars,
                          MaybeExtend extend, bool *pushedSeg)
{
    Value *firstUnused = space().firstUnused();
    FrameRegs *regs = cx->maybeRegs();

#ifdef JS_METHODJIT
    












    if (regs && report != DONT_REPORT_ERROR) {
        RootedFunction fun(cx);
        if (InlinedSite *site = regs->inlined()) {
            mjit::JITChunk *chunk = regs->fp()->jit()->chunk(regs->pc);
            fun = chunk->inlineFrames()[site->inlineIndex].fun;
        } else {
            StackFrame *fp = regs->fp();
            if (fp->isFunctionFrame()) {
                JSFunction *f = fp->fun();
                if (f->isInterpreted())
                    fun = f;
            }
        }

        if (fun) {
            AutoCompartment ac(cx, fun);
            fun->nonLazyScript()->uninlineable = true;
            types::MarkTypeObjectFlags(cx, fun, types::OBJECT_FLAG_UNINLINEABLE);
        }
    }
    JS_ASSERT_IF(cx->hasfp(), !cx->regs().inlined());
#endif

    if (onTop() && extend) {
        if (!space().ensureSpace(cx, report, firstUnused, nvars))
            return NULL;
        return firstUnused;
    }

    if (!space().ensureSpace(cx, report, firstUnused, VALUES_PER_STACK_SEGMENT + nvars))
        return NULL;

    regs = (seg_ && extend) ? seg_->maybeRegs() : NULL;

    seg_ = new(firstUnused) StackSegment(cx, seg_, space().seg_, regs);
    space().seg_ = seg_;
    *pushedSeg = true;
    return seg_->slotsBegin();
}

void
ContextStack::popSegment()
{
    space().seg_ = seg_->prevInMemory();
    seg_ = seg_->prevInContext();

    if (!seg_)
        cx_->maybeMigrateVersionOverride();
}

bool
ContextStack::pushInvokeArgs(JSContext *cx, unsigned argc, InvokeArgsGuard *iag,
                             MaybeReportError report)
{
    JS_ASSERT(argc <= StackSpace::ARGS_LENGTH_MAX);

    unsigned nvars = 2 + argc;
    Value *firstUnused = ensureOnTop(cx, report, nvars, CAN_EXTEND, &iag->pushedSeg_);
    if (!firstUnused)
        return false;

    MakeRangeGCSafe(firstUnused, nvars);

    ImplicitCast<CallArgs>(*iag) = CallArgsFromVp(argc, firstUnused);

    seg_->pushInvokeArgsEnd(iag->end(), &iag->prevInvokeArgsEnd_);

    JS_ASSERT(space().firstUnused() == iag->end());
    iag->setPushed(*this);
    return true;
}

void
ContextStack::popInvokeArgs(const InvokeArgsGuard &iag)
{
    JS_ASSERT(iag.pushed());
    JS_ASSERT(onTop());
    JS_ASSERT(space().firstUnused() == seg_->invokeArgsEnd());

    Value *oldend = seg_->end();

    seg_->popInvokeArgsEnd(iag.prevInvokeArgsEnd_);

    if (iag.pushedSeg_)
        popSegment();

    Debug_SetValueRangeToCrashOnTouch(space().firstUnused(), oldend);
}

StackFrame *
ContextStack::pushInvokeFrame(JSContext *cx, MaybeReportError report,
                              const CallArgs &args, JSFunction *funArg,
                              InitialFrameFlags initial, FrameGuard *fg)
{
    JS_ASSERT(onTop());
    JS_ASSERT(space().firstUnused() == args.end());

    RootedFunction fun(cx, funArg);
    RootedScript script(cx, fun->nonLazyScript());

    StackFrame::Flags flags = ToFrameFlags(initial);
    StackFrame *fp = getCallFrame(cx, report, args, fun, script, &flags);
    if (!fp)
        return NULL;

    fp->initCallFrame(cx, *fun, script, args.length(), flags);
    fg->regs_.prepareToRun(*fp, script);

    fg->prevRegs_ = seg_->pushRegs(fg->regs_);
    JS_ASSERT(space().firstUnused() == fg->regs_.sp);
    fg->setPushed(*this);
    return fp;
}

bool
ContextStack::pushInvokeFrame(JSContext *cx, const CallArgs &args,
                              InitialFrameFlags initial, InvokeFrameGuard *ifg)
{
    JSObject &callee = args.callee();
    JSFunction *fun = callee.toFunction();
    if (!pushInvokeFrame(cx, REPORT_ERROR, args, fun, initial, ifg))
        return false;
    return true;
}

bool
ContextStack::pushExecuteFrame(JSContext *cx, HandleScript script, const Value &thisv,
                               HandleObject scopeChain, ExecuteType type,
                               AbstractFramePtr evalInFrame, ExecuteFrameGuard *efg)
{
    












    MaybeExtend extend;
    StackFrame *prevLink;
    AbstractFramePtr prev = NullFramePtr();
    if (evalInFrame) {
        
        AllFramesIter frameIter(cx->runtime);
        while (frameIter.isIonOptimizedJS() || frameIter.abstractFramePtr() != evalInFrame)
            ++frameIter;
        JS_ASSERT(frameIter.abstractFramePtr() == evalInFrame);

        StackSegment &seg = *frameIter.seg();

        StackIter iter(cx->runtime, seg);
        
        JS_ASSERT_IF(evalInFrame.isStackFrame(), !evalInFrame.asStackFrame()->runningInIon());
        JS_ASSERT_IF(evalInFrame.compartment() == iter.compartment(), !iter.isIonOptimizedJS());
        while (iter.isIonOptimizedJS() || iter.abstractFramePtr() != evalInFrame) {
            ++iter;
            JS_ASSERT_IF(evalInFrame.compartment() == iter.compartment(), !iter.isIonOptimizedJS());
        }
        JS_ASSERT(iter.abstractFramePtr() == evalInFrame);
        prevLink = iter.data_.fp_;
        prev = evalInFrame;
        extend = CANT_EXTEND;
    } else {
        prevLink = maybefp();
        extend = CAN_EXTEND;
        if (maybefp()) {
            ScriptFrameIter iter(cx);
            prev = iter.isIonOptimizedJS() ? maybefp() : iter.abstractFramePtr();
        }
    }

    unsigned nvars = 2  + VALUES_PER_STACK_FRAME + script->nslots;
    Value *firstUnused = ensureOnTop(cx, REPORT_ERROR, nvars, extend, &efg->pushedSeg_);
    if (!firstUnused)
        return false;

    StackFrame *fp = reinterpret_cast<StackFrame *>(firstUnused + 2);
    fp->initExecuteFrame(script, prevLink, prev, seg_->maybeRegs(), thisv, *scopeChain, type);
    fp->initVarsToUndefined();
    efg->regs_.prepareToRun(*fp, script);

    efg->prevRegs_ = seg_->pushRegs(efg->regs_);
    JS_ASSERT(space().firstUnused() == efg->regs_.sp);
    efg->setPushed(*this);
    return true;
}

#ifdef JS_ION
bool
ContextStack::pushBailoutArgs(JSContext *cx, const ion::IonBailoutIterator &it, InvokeArgsGuard *iag)
{
    unsigned argc = it.numActualArgs();

    if (!pushInvokeArgs(cx, argc, iag, DONT_REPORT_ERROR))
        return false;

    ion::SnapshotIterator s(it);
    JSFunction *fun = it.callee();
    iag->setCallee(ObjectValue(*fun));

    CopyTo dst(iag->array());
    Value *src = it.actualArgs();
    Value thisv = iag->thisv();
    s.readFrameArgs(dst, src, NULL, &thisv, 0, fun->nargs, argc, it.script());
    return true;
}

StackFrame *
ContextStack::pushBailoutFrame(JSContext *cx, const ion::IonBailoutIterator &it,
                               const CallArgs &args, BailoutFrameGuard *bfg)
{
    JSFunction *fun = it.callee();
    return pushInvokeFrame(cx, DONT_REPORT_ERROR, args, fun, INITIAL_NONE, bfg);
}
#endif

void
ContextStack::popFrame(const FrameGuard &fg)
{
    JS_ASSERT(fg.pushed());
    JS_ASSERT(onTop());
    JS_ASSERT(space().firstUnused() == fg.regs_.sp);
    JS_ASSERT(&fg.regs_ == &seg_->regs());

    Value *oldend = seg_->end();

    seg_->popRegs(fg.prevRegs_);
    if (fg.pushedSeg_)
        popSegment();

    Debug_SetValueRangeToCrashOnTouch(space().firstUnused(), oldend);
}

bool
ContextStack::pushGeneratorFrame(JSContext *cx, JSGenerator *gen, GeneratorFrameGuard *gfg)
{
    HeapValue *genvp = gen->stackSnapshot;
    JS_ASSERT(genvp == HeapValueify(gen->fp->generatorArgsSnapshotBegin()));
    unsigned vplen = HeapValueify(gen->fp->generatorArgsSnapshotEnd()) - genvp;

    unsigned nvars = vplen + VALUES_PER_STACK_FRAME + gen->fp->script()->nslots;
    Value *firstUnused = ensureOnTop(cx, REPORT_ERROR, nvars, CAN_EXTEND, &gfg->pushedSeg_);
    if (!firstUnused)
        return false;

    StackFrame *stackfp = reinterpret_cast<StackFrame *>(firstUnused + vplen);
    Value *stackvp = (Value *)stackfp - vplen;

    
    gfg->gen_ = gen;
    gfg->stackvp_ = stackvp;

    






    JS_ASSERT(gen->obj->getClass()->trace);
    JSObject::writeBarrierPre(gen->obj);

    
    stackfp->copyFrameAndValues<StackFrame::NoPostBarrier>(cx, stackvp, gen->fp,
                                                           Valueify(genvp), gen->regs.sp);
    stackfp->resetGeneratorPrev(cx);
    gfg->regs_.rebaseFromTo(gen->regs, *stackfp);

    gfg->prevRegs_ = seg_->pushRegs(gfg->regs_);
    JS_ASSERT(space().firstUnused() == gfg->regs_.sp);
    gfg->setPushed(*this);
    return true;
}

void
ContextStack::popGeneratorFrame(const GeneratorFrameGuard &gfg)
{
    JSGenerator *gen = gfg.gen_;
    HeapValue *genvp = gen->stackSnapshot;

    const FrameRegs &stackRegs = gfg.regs_;
    StackFrame *stackfp = stackRegs.fp();
    Value *stackvp = gfg.stackvp_;

    
    if (stackfp->isYielding()) {
        



        JS_ASSERT(!GeneratorHasMarkableFrame(gen));

        gen->regs.rebaseFromTo(stackRegs, *gen->fp);
        gen->fp->copyFrameAndValues<StackFrame::DoPostBarrier>(cx_, (Value *)genvp, stackfp,
                                                               stackvp, stackRegs.sp);
    }

    
    JS_ASSERT(ImplicitCast<const FrameGuard>(gfg).pushed());
}

bool
ContextStack::saveFrameChain()
{
    bool pushedSeg;
    if (!ensureOnTop(cx_, REPORT_ERROR, 0, CANT_EXTEND, &pushedSeg))
        return false;

    JS_ASSERT(pushedSeg);
    JS_ASSERT(!hasfp());
    JS_ASSERT(onTop());
    JS_ASSERT(seg_->isEmpty());
    return true;
}

void
ContextStack::restoreFrameChain()
{
    JS_ASSERT(!hasfp());
    JS_ASSERT(onTop());
    JS_ASSERT(seg_->isEmpty());

    popSegment();
}



void
StackIter::poisonRegs()
{
    data_.pc_ = (jsbytecode *)0xbad;
}

void
StackIter::popFrame()
{
    StackFrame *oldfp = data_.fp_;
    JS_ASSERT(data_.seg_->contains(oldfp));
    data_.fp_ = data_.fp_->prev();

    if (data_.seg_->contains(data_.fp_)) {
        InlinedSite *inline_;
        data_.pc_ = oldfp->prevpc(&inline_);
        JS_ASSERT(!inline_);
    } else {
        poisonRegs();
    }
}

void
StackIter::settleOnNewSegment()
{
    if (FrameRegs *regs = data_.seg_->maybeRegs())
        data_.pc_ = regs->pc;
    else
        poisonRegs();
}

void
StackIter::startOnSegment(StackSegment *seg)
{
    data_.seg_ = seg;
    data_.fp_ = data_.seg_->maybefp();
    settleOnNewSegment();
}
















#if defined(_MSC_VER)
# pragma optimize("g", off)
#endif
void
StackIter::settleOnNewState()
{
    



    while (true) {
        if (!data_.fp_) {
            if (data_.savedOption_ == GO_THROUGH_SAVED && data_.seg_->prevInContext()) {
                startOnSegment(data_.seg_->prevInContext());
                continue;
            }
            data_.state_ = DONE;
            return;
        }

        
        bool containsFrame = data_.seg_->contains(data_.fp_);
        while (!containsFrame) {
            
            data_.seg_ = data_.seg_->prevInMemory();
            containsFrame = data_.seg_->contains(data_.fp_);

            
            if (containsFrame && data_.seg_->fp() != data_.fp_) {
                
                StackIter tmp = *this;
                tmp.startOnSegment(data_.seg_);
                tmp.settleOnNewState();
                while (tmp.data_.fp_ != data_.fp_)
                    ++tmp;
                JS_ASSERT(!tmp.done() &&
                          tmp.data_.seg_ == data_.seg_ &&
                          tmp.data_.fp_ == data_.fp_);
                *this = tmp;
                return;
            }

            settleOnNewSegment();
        }

#ifdef JS_ION
        if (data_.fp_->beginsIonActivation()) {
            






            while (true) {
                ion::IonActivation *act = data_.ionActivations_.activation();
                while (!act->entryfp())
                    act = act->prev();
                if (act->entryfp() == data_.fp_)
                    break;

                ++data_.ionActivations_;
            }

            data_.ionFrames_ = ion::IonFrameIterator(data_.ionActivations_);

            while (!data_.ionFrames_.isScripted() && !data_.ionFrames_.done())
                ++data_.ionFrames_;

            
            
            if (data_.ionFrames_.done()) {
                data_.state_ = SCRIPTED;
                return;
            }

            data_.state_ = ION;
            nextIonFrame();
            return;
        }
#endif 

        data_.state_ = SCRIPTED;
        return;
    }
}
#if defined(_MSC_VER)
# pragma optimize("", on)
#endif

StackIter::Data::Data(JSContext *cx, PerThreadData *perThread, SavedOption savedOption)
  : perThread_(perThread),
    cx_(cx),
    savedOption_(savedOption)
#ifdef JS_ION
    , ionActivations_(cx),
    ionFrames_((uint8_t *)NULL)
#endif
{
}

StackIter::Data::Data(JSContext *cx, JSRuntime *rt, StackSegment *seg)
  : perThread_(&rt->mainThread),
    cx_(cx),
    savedOption_(STOP_AT_SAVED)
#ifdef JS_ION
    , ionActivations_(rt),
    ionFrames_((uint8_t *)NULL)
#endif
{
}

StackIter::Data::Data(const StackIter::Data &other)
  : perThread_(other.perThread_),
    cx_(other.cx_),
    savedOption_(other.savedOption_),
    state_(other.state_),
    fp_(other.fp_),
    seg_(other.seg_),
    pc_(other.pc_)
#ifdef JS_ION
    , ionActivations_(other.ionActivations_),
    ionFrames_(other.ionFrames_)
#endif
{
}

StackIter::StackIter(JSContext *cx, SavedOption savedOption)
  : data_(cx, &cx->runtime->mainThread, savedOption)
#ifdef JS_ION
    , ionInlineFrames_(cx, (js::ion::IonFrameIterator*) NULL)
#endif
{
#ifdef JS_METHODJIT
    for (ZonesIter zone(cx->runtime); !zone.done(); zone.next())
        mjit::ExpandInlineFrames(zone);
#endif

    if (StackSegment *seg = cx->stack.seg_) {
        startOnSegment(seg);
        settleOnNewState();
    } else {
        data_.state_ = DONE;
    }
}

StackIter::StackIter(JSRuntime *rt, StackSegment &seg)
  : data_(seg.cx(), rt, &seg)
#ifdef JS_ION
    , ionInlineFrames_(seg.cx(), (js::ion::IonFrameIterator*) NULL)
#endif
{
#ifdef JS_METHODJIT
    for (ZonesIter zone(rt); !zone.done(); zone.next())
        mjit::ExpandInlineFrames(zone);
#endif
    startOnSegment(&seg);
    settleOnNewState();
}

StackIter::StackIter(const StackIter &other)
  : data_(other.data_)
#ifdef JS_ION
    , ionInlineFrames_(other.data_.seg_->cx(),
                       data_.ionFrames_.isScripted() ? &other.ionInlineFrames_ : NULL)
#endif
{
}

StackIter::StackIter(const Data &data)
  : data_(data)
#ifdef JS_ION
    , ionInlineFrames_(data.cx_, data_.ionFrames_.isOptimizedJS() ? &data_.ionFrames_ : NULL)
#endif
{
    JS_ASSERT(data.cx_);
}

#ifdef JS_ION
void
StackIter::nextIonFrame()
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
StackIter::popIonFrame()
{
    
    poisonRegs();
    if (data_.ionFrames_.isOptimizedJS() && ionInlineFrames_.more()) {
        ++ionInlineFrames_;
        data_.pc_ = ionInlineFrames_.pc();
    } else {
        ++data_.ionFrames_;
        while (!data_.ionFrames_.done() && !data_.ionFrames_.isScripted())
            ++data_.ionFrames_;

        if (!data_.ionFrames_.done()) {
            nextIonFrame();
            return;
        }

        
        
        ion::IonActivation *activation = data_.ionActivations_.activation();
        if (!activation->entryfp()) {
            JS_ASSERT(activation->prevpc());
            JS_ASSERT(data_.fp_->beginsIonActivation());
            ++data_.ionActivations_;
            settleOnNewState();
            return;
        }

        if (data_.fp_->runningInIon()) {
            ++data_.ionActivations_;
            popFrame();
            settleOnNewState();
        } else {
            JS_ASSERT(data_.fp_->callingIntoIon());
            data_.state_ = SCRIPTED;
            data_.pc_ = data_.ionActivations_.activation()->prevpc();
            ++data_.ionActivations_;
        }
    }
}

void
StackIter::popBaselineDebuggerFrame()
{
    ion::BaselineFrame *prevBaseline = data_.fp_->prevBaselineFrame();

    popFrame();
    settleOnNewState();

    JS_ASSERT(data_.state_ == ION);
    while (!data_.ionFrames_.isBaselineJS() || data_.ionFrames_.baselineFrame() != prevBaseline)
        popIonFrame();
}
#endif

StackIter &
StackIter::operator++()
{
    switch (data_.state_) {
      case DONE:
        JS_NOT_REACHED("Unexpected state");
      case SCRIPTED:
#ifdef JS_ION
        if (data_.fp_->isDebuggerFrame() && data_.fp_->prevBaselineFrame()) {
            
            popBaselineDebuggerFrame();
            break;
        }
#endif
        popFrame();
        settleOnNewState();
        break;
      case ION:
#ifdef JS_ION
        popIonFrame();
        break;
#else
        JS_NOT_REACHED("Unexpected state");
#endif
    }
    return *this;
}

bool
StackIter::operator==(const StackIter &rhs) const
{
    return done() == rhs.done() &&
           (done() || data_.fp_ == rhs.data_.fp_);
}

StackIter::Data *
StackIter::copyData() const
{
#ifdef JS_ION
    



    JS_ASSERT(data_.ionFrames_.type() != ion::IonFrame_OptimizedJS);
#endif
    return data_.cx_->new_<Data>(data_);
}

JSCompartment *
StackIter::compartment() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        return data_.fp_->compartment();
      case ION:
#ifdef  JS_ION
        return data_.ionActivations_.activation()->compartment();
#else
        break;
#endif
    }
    JS_NOT_REACHED("Unexpected state");
    return NULL;
}

bool
StackIter::isFunctionFrame() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        return interpFrame()->isFunctionFrame();
      case ION:
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
StackIter::isGlobalFrame() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        return interpFrame()->isGlobalFrame();
      case ION:
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
StackIter::isEvalFrame() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        return interpFrame()->isEvalFrame();
      case ION:
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
StackIter::isNonEvalFunctionFrame() const
{
    JS_ASSERT(!done());
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        return interpFrame()->isNonEvalFunctionFrame();
      case ION:
        return !isEvalFrame() && isFunctionFrame();
    }
    JS_NOT_REACHED("Unexpected state");
    return false;
}

bool
StackIter::isGeneratorFrame() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        return interpFrame()->isGeneratorFrame();
      case ION:
        return false;
    }
    JS_NOT_REACHED("Unexpected state");
    return false;
}

bool
StackIter::isConstructing() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case ION:
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
StackIter::abstractFramePtr() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case ION:
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
StackIter::updatePcQuadratic()
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        data_.pc_ = interpFrame()->pcQuadratic(data_.cx_);
        return;
      case ION:
#ifdef JS_ION
        if (data_.ionFrames_.isBaselineJS()) {
            ion::BaselineFrame *frame = data_.ionFrames_.baselineFrame();
            ion::IonActivation *activation = data_.ionActivations_.activation();

            
            
            data_.ionActivations_ = ion::IonActivationIterator(data_.cx_);
            while (data_.ionActivations_.activation() != activation)
                ++data_.ionActivations_;

            
            data_.ionFrames_ = ion::IonFrameIterator(data_.ionActivations_);
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
StackIter::callee() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        JS_ASSERT(isFunctionFrame());
        return &interpFrame()->callee();
      case ION:
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
StackIter::calleev() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        JS_ASSERT(isFunctionFrame());
        return interpFrame()->calleev();
      case ION:
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
StackIter::numActualArgs() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        JS_ASSERT(isFunctionFrame());
        return interpFrame()->numActualArgs();
      case ION:
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
StackIter::unaliasedActual(unsigned i, MaybeCheckAliasing checkAliasing) const
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        return interpFrame()->unaliasedActual(i, checkAliasing);
      case ION:
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
StackIter::scopeChain() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case ION:
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
StackIter::callObj() const
{
    JS_ASSERT(callee()->isHeavyweight());

    JSObject *pobj = scopeChain();
    while (!pobj->isCall())
        pobj = pobj->enclosingScope();
    return pobj->asCall();
}

bool
StackIter::hasArgsObj() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case SCRIPTED:
        return interpFrame()->hasArgsObj();
      case ION:
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
StackIter::argsObj() const
{
    JS_ASSERT(hasArgsObj());

    switch (data_.state_) {
      case DONE:
        break;
      case ION:
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
StackIter::computeThis() const
{
    JS_ASSERT(!done());
    if (!isIonOptimizedJS()) {
        JS_ASSERT(data_.cx_);
        return ComputeThis(data_.cx_, abstractFramePtr());
    }
    return true;
}

Value
StackIter::thisv() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case ION:
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
StackIter::returnValue() const
{
    switch (data_.state_) {
      case DONE:
        break;
      case ION:
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
StackIter::setReturnValue(const Value &v)
{
    switch (data_.state_) {
      case DONE:
        break;
      case ION:
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
StackIter::numFrameSlots() const
{
    switch (data_.state_) {
      case DONE:
        break;
     case ION: {
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
        JS_ASSERT(data_.cx_->regs().spForStackDepth(0) == interpFrame()->base());
        return data_.cx_->regs().sp - interpFrame()->base();
    }
    JS_NOT_REACHED("Unexpected state");
    return 0;
}

Value
StackIter::frameSlotValue(size_t index) const
{
    switch (data_.state_) {
      case DONE:
        break;
      case ION:
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



AllFramesIter::AllFramesIter(JSRuntime *rt)
  : seg_(rt->stackSpace.seg_),
    fp_(seg_ ? seg_->maybefp() : NULL)
#ifdef JS_ION
    , ionActivations_(rt),
    ionFrames_((uint8_t *)NULL)
#endif
{
    settleOnNewState();
}

#ifdef JS_ION
void
AllFramesIter::popIonFrame()
{
    JS_ASSERT(state_ == ION);

    ++ionFrames_;
    while (!ionFrames_.done() && !ionFrames_.isScripted())
        ++ionFrames_;

    if (!ionFrames_.done())
        return;

    
    
    ion::IonActivation *activation = ionActivations_.activation();
    if (!activation->entryfp()) {
        JS_ASSERT(activation->prevpc());
        JS_ASSERT(fp_->beginsIonActivation());
        ++ionActivations_;
        settleOnNewState();
        return;
    }

    if (fp_->runningInIon()) {
        ++ionActivations_;
        fp_ = fp_->prev();
        settleOnNewState();
    } else {
        JS_ASSERT(fp_->callingIntoIon());
        state_ = SCRIPTED;
        ++ionActivations_;
    }
}
#endif

AllFramesIter&
AllFramesIter::operator++()
{
    switch (state_) {
      case SCRIPTED:
        fp_ = fp_->prev();
        settleOnNewState();
        break;
#ifdef JS_ION
      case ION:
        popIonFrame();
        break;
#endif
      case DONE:
      default:
        JS_NOT_REACHED("Unexpeced state");
    }
    return *this;
}

void
AllFramesIter::settleOnNewState()
{
    while (seg_ && (!fp_ || !seg_->contains(fp_))) {
        seg_ = seg_->prevInMemory();
        fp_ = seg_ ? seg_->maybefp() : NULL;
    }

    JS_ASSERT(!!seg_ == !!fp_);
    JS_ASSERT_IF(fp_, seg_->contains(fp_));

#ifdef JS_ION
    if (fp_ && fp_->beginsIonActivation()) {
        
        ionFrames_ = ion::IonFrameIterator(ionActivations_);
        while (!ionFrames_.isScripted() && !ionFrames_.done())
            ++ionFrames_;

        state_ = ionFrames_.done() ? SCRIPTED : ION;
        return;
    }
#endif

    state_ = fp_ ? SCRIPTED : DONE;
}

AbstractFramePtr
AllFramesIter::abstractFramePtr() const
{
    switch (state_) {
      case SCRIPTED:
        return AbstractFramePtr(interpFrame());
      case ION:
#ifdef JS_ION
        if (ionFrames_.isBaselineJS())
            return ionFrames_.baselineFrame();
#endif
        break;
      case DONE:
        break;
    }
    JS_NOT_REACHED("Unexpected state");
    return NullFramePtr();
}

JSObject *
AbstractFramePtr::evalPrevScopeChain(JSRuntime *rt) const
{
    
    AllFramesIter alliter(rt);
    while (alliter.isIonOptimizedJS() || alliter.abstractFramePtr() != *this)
        ++alliter;

    
    StackIter iter(rt, *alliter.seg());
    while (iter.isIonOptimizedJS() || iter.abstractFramePtr() != *this)
        ++iter;
    ++iter;
    return iter.scopeChain();
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
