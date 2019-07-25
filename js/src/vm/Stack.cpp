







































#include "jsgcmark.h"
#include "methodjit/MethodJIT.h"
#include "Stack.h"

#include "jsgcinlines.h"
#include "jsobjinlines.h"

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



void
StackFrame::initExecuteFrame(JSScript *script, StackFrame *prev, FrameRegs *regs,
                             const Value &thisv, JSObject &scopeChain, ExecuteType type)
{
    




    flags_ = type | HAS_SCOPECHAIN | HAS_PREVPC;
    if (!(flags_ & GLOBAL))
        flags_ |= (prev->flags_ & (FUNCTION | GLOBAL));

    Value *dstvp = (Value *)this - 2;
    dstvp[1] = thisv;

    if (isFunctionFrame()) {
        dstvp[0] = prev->calleev();
        exec = prev->exec;
        args.script = script;
    } else {
        JS_ASSERT(isGlobalFrame());
        dstvp[0] = NullValue();
        exec.script = script;
#ifdef DEBUG
        args.script = (JSScript *)0xbad;
#endif
    }

    scopeChain_ = &scopeChain;
    prev_ = prev;
    prevpc_ = regs ? regs->pc : (jsbytecode *)0xbad;

#ifdef DEBUG
    ncode_ = (void *)0xbad;
    Debug_SetValueRangeToCrashOnTouch(&rval_, 1);
    hookData_ = (void *)0xbad;
    annotation_ = (void *)0xbad;
#endif

    if (prev && prev->annotation())
        setAnnotation(prev->annotation());
}

void
StackFrame::initDummyFrame(JSContext *cx, JSObject &chain)
{
    PodZero(this);
    flags_ = DUMMY | HAS_PREVPC | HAS_SCOPECHAIN;
    initPrev(cx);
    JS_ASSERT(chain.isGlobal());
    setScopeChainNoCallObj(chain);
}

void
StackFrame::stealFrameAndSlots(Value *vp, StackFrame *otherfp,
                               Value *othervp, Value *othersp)
{
    JS_ASSERT(vp == (Value *)this - ((Value *)otherfp - othervp));
    JS_ASSERT(othervp == otherfp->actualArgs() - 2);
    JS_ASSERT(othersp >= otherfp->slots());
    JS_ASSERT(othersp <= otherfp->base() + otherfp->numSlots());

    PodCopy(vp, othervp, othersp - othervp);
    JS_ASSERT(vp == this->actualArgs() - 2);

    
    if (otherfp->hasOverflowArgs())
        Debug_SetValueRangeToCrashOnTouch(othervp, othervp + 2 + otherfp->numFormalArgs());

    





    if (hasCallObj()) {
        JSObject &obj = callObj();
        obj.setPrivate(this);
        otherfp->flags_ &= ~HAS_CALL_OBJ;
        if (js_IsNamedLambda(fun())) {
            JSObject *env = obj.getParent();
            JS_ASSERT(env->getClass() == &js_DeclEnvClass);
            env->setPrivate(this);
        }
    }
    if (hasArgsObj()) {
        ArgumentsObject &argsobj = argsObj();
        if (argsobj.isNormalArguments())
            argsobj.setPrivate(this);
        else
            JS_ASSERT(!argsobj.getPrivate());
        otherfp->flags_ &= ~HAS_ARGS_OBJ;
    }
}

#ifdef DEBUG
JSObject *const StackFrame::sInvalidScopeChain = (JSObject *)0xbeef;
#endif

jsbytecode *
StackFrame::pcQuadratic(JSContext *cx) const
{
    if (hasImacropc())
        return imacropc();
    StackSegment &seg = cx->stack.space().findContainingSegment(this);
    FrameRegs &regs = seg.regs();
    if (regs.fp() == this)
        return regs.pc;
    return seg.computeNextFrame(this)->prevpc();
}

jsbytecode *
StackFrame::prevpcSlow()
{
    JS_ASSERT(!(flags_ & HAS_PREVPC));
#if defined(JS_METHODJIT) && defined(JS_MONOIC)
    StackFrame *p = prev();
    mjit::JITScript *jit = p->script()->getJIT(p->isConstructing());
    prevpc_ = jit->nativeToPC(ncode_);
    flags_ |= HAS_PREVPC;
    return prevpc_;
#else
    JS_NOT_REACHED("Unknown PC for frame");
    return NULL;
#endif
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

bool
StackSegment::contains(const CallArgsList *call) const
{
    if (!call || !calls_)
        return false;

    
    Value *vp = call->argv();
    bool ret = vp > slotsBegin() && vp <= calls_->argv();

#ifdef DEBUG
    bool found = false;
    for (CallArgsList *c = maybeCalls(); c->argv() > slotsBegin(); c = c->prev()) {
        if (c == call) {
            found = true;
            break;
        }
    }
    JS_ASSERT(found == ret);
#endif

    return ret;
}

StackFrame *
StackSegment::computeNextFrame(const StackFrame *f) const
{
    JS_ASSERT(contains(f) && f != fp());

    StackFrame *next = fp();
    StackFrame *prev;
    while ((prev = next->prev()) != f)
        next = prev;
    return next;
}

Value *
StackSegment::end() const
{
    
    JS_ASSERT_IF(calls_ || regs_, contains(calls_) || contains(regs_));
    Value *p = calls_
               ? regs_
                 ? Max(regs_->sp, calls_->end())
                 : calls_->end()
               : regs_
                 ? regs_->sp
                 : slotsBegin();
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

void
StackSegment::pushCall(CallArgsList &callList)
{
    callList.prev_ = calls_;
    calls_ = &callList;
}

void
StackSegment::pointAtCall(CallArgsList &callList)
{
    calls_ = &callList;
}

void
StackSegment::popCall()
{
    calls_ = calls_->prev_;
}



StackSpace::StackSpace()
  : base_(NULL),
    commitEnd_(NULL),
    end_(NULL),
    seg_(NULL)
{}

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
    commitEnd_ = base_ + COMMIT_VALS;
    end_ = base_ + CAPACITY_VALS;
#elif defined(XP_OS2)
    if (DosAllocMem(&p, CAPACITY_BYTES, PAG_COMMIT | PAG_READ | PAG_WRITE | OBJ_ANY) &&
        DosAllocMem(&p, CAPACITY_BYTES, PAG_COMMIT | PAG_READ | PAG_WRITE))
        return false;
    base_ = reinterpret_cast<Value *>(p);
    end_ = commitEnd_ = base_ + CAPACITY_VALS;
#else
    JS_ASSERT(CAPACITY_BYTES % getpagesize() == 0);
    p = mmap(NULL, CAPACITY_BYTES, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED)
        return false;
    base_ = reinterpret_cast<Value *>(p);
    end_ = commitEnd_ = base_ + CAPACITY_VALS;
#endif
    return true;
}

StackSpace::~StackSpace()
{
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
StackSpace::findContainingSegment(const StackFrame *target) const
{
    for (StackSegment *s = seg_; s; s = s->prevInMemory()) {
        if (s->contains(target))
            return *s;
    }
    JS_NOT_REACHED("frame not in stack space");
    return *(StackSegment *)NULL;
}

void
StackSpace::mark(JSTracer *trc)
{
    




    
    Value *nextSegEnd = firstUnused();
    for (StackSegment *seg = seg_; seg; seg = seg->prevInMemory()) {
        










        Value *slotsEnd = nextSegEnd;
        for (StackFrame *fp = seg->maybefp(); (Value *)fp > (Value *)seg; fp = fp->prev()) {
            MarkStackRangeConservatively(trc, fp->slots(), slotsEnd);
            js_TraceStackFrame(trc, fp);
            slotsEnd = (Value *)fp;
        }
        MarkStackRangeConservatively(trc, seg->slotsBegin(), slotsEnd);
        nextSegEnd = (Value *)seg;
    }
}

#ifdef XP_WIN
JS_FRIEND_API(bool)
StackSpace::bumpCommit(JSContext *maybecx, Value *from, ptrdiff_t nvals) const
{
    if (end_ - from < nvals) {
        js_ReportOverRecursed(maybecx);
        return false;
    }

    Value *newCommit = commitEnd_;
    Value *request = from + nvals;

    
    JS_ASSERT((end_ - newCommit) % COMMIT_VALS == 0);
    do {
        newCommit += COMMIT_VALS;
        JS_ASSERT((end_ - newCommit) >= 0);
    } while (newCommit < request);

    
    int32 size = static_cast<int32>(newCommit - commitEnd_) * sizeof(Value);

    if (!VirtualAlloc(commitEnd_, size, MEM_COMMIT, PAGE_READWRITE)) {
        js_ReportOverRecursed(maybecx);
        return false;
    }

    commitEnd_ = newCommit;
    return true;
}
#endif

bool
StackSpace::tryBumpLimit(JSContext *maybecx, Value *from, uintN nvals, Value **limit)
{
    if (!ensureSpace(maybecx, from, nvals))
        return false;
#ifdef XP_WIN
    *limit = commitEnd_;
#else
    *limit = end_;
#endif
    return true;
}

size_t
StackSpace::committedSize()
{
    return (commitEnd_ - base_) * sizeof(Value);
}



ContextStack::ContextStack(JSContext *cx)
  : seg_(NULL),
    space_(&JS_THREAD_DATA(cx)->stackSpace),
    cx_(cx)
{
    threadReset();
}

ContextStack::~ContextStack()
{
    JS_ASSERT(!seg_);
}

void
ContextStack::threadReset()
{
#ifdef JS_THREADSAFE
    if (cx_->thread())
        space_ = &JS_THREAD_DATA(cx_)->stackSpace;
    else
        space_ = NULL;
#else
    space_ = &JS_THREAD_DATA(cx_)->stackSpace;
#endif
}

#ifdef DEBUG
void
ContextStack::assertSpaceInSync() const
{
    JS_ASSERT(space_);
    JS_ASSERT(space_ == &JS_THREAD_DATA(cx_)->stackSpace);
}
#endif

bool
ContextStack::onTop() const
{
    return seg_ && seg_ == space().seg_;
}

bool
ContextStack::containsSlow(const StackFrame *target) const
{
    for (StackSegment *s = seg_; s; s = s->prevInContext()) {
        if (s->contains(target))
            return true;
    }
    return false;
}










Value *
ContextStack::ensureOnTop(JSContext *cx, uintN nvars, MaybeExtend extend, bool *pushedSeg)
{
    Value *firstUnused = space().firstUnused();

    if (onTop() && extend) {
        if (!space().ensureSpace(cx, firstUnused, nvars))
            return NULL;
        return firstUnused;
    }

    if (!space().ensureSpace(cx, firstUnused, VALUES_PER_STACK_SEGMENT + nvars))
        return NULL;

    FrameRegs *regs;
    CallArgsList *calls;
    if (seg_ && extend) {
        regs = seg_->maybeRegs();
        calls = seg_->maybeCalls();
    } else {
        regs = NULL;
        calls = NULL;
    }

    seg_ = new(firstUnused) StackSegment(seg_, space().seg_, regs, calls);
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
ContextStack::pushInvokeArgs(JSContext *cx, uintN argc, InvokeArgsGuard *iag)
{
    uintN nvars = 2 + argc;
    Value *firstUnused = ensureOnTop(cx, nvars, CAN_EXTEND, &iag->pushedSeg_);
    if (!firstUnused)
        return false;

    ImplicitCast<CallArgs>(*iag) = CallArgsFromVp(argc, firstUnused);

    seg_->pushCall(*iag);
    JS_ASSERT(space().firstUnused() == iag->end());
    iag->setPushed(*this);
    return true;
}

void
ContextStack::popInvokeArgs(const InvokeArgsGuard &iag)
{
    JS_ASSERT(iag.pushed());
    JS_ASSERT(onTop());
    JS_ASSERT(space().firstUnused() == seg_->calls().end());

    seg_->popCall();
    if (iag.pushedSeg_)
        popSegment();
}

bool
ContextStack::pushInvokeFrame(JSContext *cx, const CallArgs &args,
                              MaybeConstruct construct, InvokeFrameGuard *ifg)
{
    JS_ASSERT(onTop());
    JS_ASSERT(space().firstUnused() == args.end());

    JSObject &callee = args.callee();
    JSFunction *fun = callee.getFunctionPrivate();
    JSScript *script = fun->script();

    StackFrame::Flags flags = ToFrameFlags(construct);
    StackFrame *fp = getCallFrame(cx, args, fun, script, &flags, OOMCheck());
    if (!fp)
        return false;

    fp->initCallFrame(cx, callee, fun, script, args.argc(), flags);
    ifg->regs_.prepareToRun(*fp, script);

    ifg->prevRegs_ = seg_->pushRegs(ifg->regs_);
    JS_ASSERT(space().firstUnused() == ifg->regs_.sp);
    ifg->setPushed(*this);
    return true;
}

bool
ContextStack::pushExecuteFrame(JSContext *cx, JSScript *script, const Value &thisv,
                               JSObject &scopeChain, ExecuteType type,
                               StackFrame *evalInFrame, ExecuteFrameGuard *efg)
{
    











    CallArgsList *evalInFrameCalls = NULL;  
    StackFrame *prev;
    MaybeExtend extend;
    if (evalInFrame) {
        
        StackIter iter(cx, StackIter::GO_THROUGH_SAVED);
        while (!iter.isScript() || iter.fp() != evalInFrame)
            ++iter;
        evalInFrameCalls = iter.calls_;
        prev = evalInFrame;
        extend = CANT_EXTEND;
    } else {
        prev = maybefp();
        extend = CAN_EXTEND;
    }

    uintN nvars = 2  + VALUES_PER_STACK_FRAME + script->nslots;
    Value *firstUnused = ensureOnTop(cx, nvars, extend, &efg->pushedSeg_);
    if (!firstUnused)
        return NULL;

    StackFrame *fp = reinterpret_cast<StackFrame *>(firstUnused + 2);
    fp->initExecuteFrame(script, prev, seg_->maybeRegs(), thisv, scopeChain, type);
    SetValueRangeToUndefined(fp->slots(), script->nfixed);
    efg->regs_.prepareToRun(*fp, script);

    
    if (evalInFrame && evalInFrameCalls)
        seg_->pointAtCall(*evalInFrameCalls);

    efg->prevRegs_ = seg_->pushRegs(efg->regs_);
    JS_ASSERT(space().firstUnused() == efg->regs_.sp);
    efg->setPushed(*this);
    return true;
}

bool
ContextStack::pushDummyFrame(JSContext *cx, JSObject &scopeChain, DummyFrameGuard *dfg)
{
    uintN nvars = VALUES_PER_STACK_FRAME;
    Value *firstUnused = ensureOnTop(cx, nvars, CAN_EXTEND, &dfg->pushedSeg_);
    if (!firstUnused)
        return NULL;

    StackFrame *fp = reinterpret_cast<StackFrame *>(firstUnused);
    fp->initDummyFrame(cx, scopeChain);
    dfg->regs_.initDummyFrame(*fp);

    dfg->prevRegs_ = seg_->pushRegs(dfg->regs_);
    JS_ASSERT(space().firstUnused() == dfg->regs_.sp);
    dfg->setPushed(*this);
    return true;
}

void
ContextStack::popFrame(const FrameGuard &fg)
{
    JS_ASSERT(fg.pushed());
    JS_ASSERT(onTop());
    JS_ASSERT(space().firstUnused() == fg.regs_.sp);
    JS_ASSERT(&fg.regs_ == &seg_->regs());

    fg.regs_.fp()->putActivationObjects();

    seg_->popRegs(fg.prevRegs_);
    if (fg.pushedSeg_)
        popSegment();

    



    if (!hasfp())
        cx_->resetCompartment();
}

bool
ContextStack::pushGeneratorFrame(JSContext *cx, JSGenerator *gen, GeneratorFrameGuard *gfg)
{
    StackFrame *genfp = gen->floatingFrame();
    Value *genvp = gen->floatingStack;
    uintN vplen = (Value *)genfp - genvp;

    uintN nvars = vplen + VALUES_PER_STACK_FRAME + genfp->numSlots();
    Value *firstUnused = ensureOnTop(cx, nvars, CAN_EXTEND, &gfg->pushedSeg_);
    if (!firstUnused)
        return false;

    StackFrame *stackfp = reinterpret_cast<StackFrame *>(firstUnused + vplen);
    Value *stackvp = (Value *)stackfp - vplen;

    
    gfg->gen_ = gen;
    gfg->stackvp_ = stackvp;

    
    stackfp->stealFrameAndSlots(stackvp, genfp, genvp, gen->regs.sp);
    stackfp->resetGeneratorPrev(cx);
    stackfp->unsetFloatingGenerator();
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
    StackFrame *genfp = gen->floatingFrame();
    Value *genvp = gen->floatingStack;

    const FrameRegs &stackRegs = gfg.regs_;
    StackFrame *stackfp = stackRegs.fp();
    Value *stackvp = gfg.stackvp_;

    
    gen->regs.rebaseFromTo(stackRegs, *genfp);
    genfp->stealFrameAndSlots(genvp, stackfp, stackvp, stackRegs.sp);
    genfp->setFloatingGenerator();

    
    JS_ASSERT(ImplicitCast<const FrameGuard>(gfg).pushed());
}

bool
ContextStack::saveFrameChain()
{
    bool pushedSeg;
    if (!ensureOnTop(cx_, 0, CANT_EXTEND, &pushedSeg))
        return false;
    JS_ASSERT(pushedSeg);
    JS_ASSERT(!hasfp());
    cx_->resetCompartment();

    JS_ASSERT(onTop() && seg_->isEmpty());
    return true;
}

void
ContextStack::restoreFrameChain()
{
    JS_ASSERT(onTop() && seg_->isEmpty());

    popSegment();
    cx_->resetCompartment();
}



void
StackIter::poisonRegs()
{
    sp_ = (Value *)0xbad;
    pc_ = (jsbytecode *)0xbad;
}

void
StackIter::popFrame()
{
    StackFrame *oldfp = fp_;
    JS_ASSERT(seg_->contains(oldfp));
    fp_ = fp_->prev();
    if (seg_->contains(fp_)) {
        pc_ = oldfp->prevpc();

        






        if (oldfp->isGeneratorFrame()) {
            
            sp_ = (Value *)oldfp->actualArgs() - 2;
        } else if (oldfp->isNonEvalFunctionFrame()) {
            







            sp_ = oldfp->actualArgsEnd();
        } else if (oldfp->isFramePushedByExecute()) {
            
            sp_ = (Value *)oldfp - 2;
        } else {
            
            JS_ASSERT(oldfp->isDummyFrame());
            sp_ = (Value *)oldfp;
        }
    } else {
        poisonRegs();
    }
}

void
StackIter::popCall()
{
    CallArgsList *oldCall = calls_;
    JS_ASSERT(seg_->contains(oldCall));
    calls_ = calls_->prev();
    if (seg_->contains(fp_)) {
        
        sp_ = oldCall->base();
    } else {
        poisonRegs();
    }
}

void
StackIter::settleOnNewSegment()
{
    if (FrameRegs *regs = seg_->maybeRegs()) {
        sp_ = regs->sp;
        pc_ = regs->pc;
    } else {
        poisonRegs();
    }
}

void
StackIter::startOnSegment(StackSegment *seg)
{
    seg_ = seg;
    fp_ = seg_->maybefp();
    calls_ = seg_->maybeCalls();
    settleOnNewSegment();
}

void
StackIter::settleOnNewState()
{
    



    while (true) {
        if (!fp_ && !calls_) {
            if (savedOption_ == GO_THROUGH_SAVED && seg_->prevInContext()) {
                startOnSegment(seg_->prevInContext());
                continue;
            }
            state_ = DONE;
            return;
        }

        
        bool containsFrame = seg_->contains(fp_);
        bool containsCall = seg_->contains(calls_);
        while (!containsFrame && !containsCall) {
            seg_ = seg_->prevInContext();
            containsFrame = seg_->contains(fp_);
            containsCall = seg_->contains(calls_);

            
            if (containsFrame && seg_->fp() != fp_) {
                
                StackIter tmp = *this;
                tmp.startOnSegment(seg_);
                while (!tmp.isScript() || tmp.fp() != fp_)
                    ++tmp;
                JS_ASSERT(tmp.state_ == SCRIPTED && tmp.seg_ == seg_ && tmp.fp_ == fp_);
                *this = tmp;
                return;
            }
            
            JS_ASSERT_IF(containsCall, &seg_->calls() == calls_);
            settleOnNewSegment();
        }

        



        if (containsFrame && (!containsCall || (Value *)fp_ >= calls_->argv())) {
            
            if (fp_->isDummyFrame()) {
                popFrame();
                continue;
            }

            





















            JSOp op = js_GetOpcode(cx_, fp_->script(), pc_);
            if (op == JSOP_CALL || op == JSOP_FUNCALL) {
                uintN argc = GET_ARGC(pc_);
                DebugOnly<uintN> spoff = sp_ - fp_->base();
                JS_ASSERT(spoff == js_ReconstructStackDepth(cx_, fp_->script(), pc_));
                Value *vp = sp_ - (2 + argc);

                if (IsNativeFunction(*vp)) {
                    state_ = IMPLICIT_NATIVE;
                    args_ = CallArgsFromVp(argc, vp);
                    return;
                }
            } else if (op == JSOP_FUNAPPLY) {
                uintN argc = GET_ARGC(pc_);
                uintN spoff = js_ReconstructStackDepth(cx_, fp_->script(), pc_);
                Value *sp = fp_->base() + spoff;
                Value *vp = sp - (2 + argc);

                if (IsNativeFunction(*vp)) {
                    if (sp_ != sp) {
                        JS_ASSERT(argc == 2);
                        JS_ASSERT(vp[0].toObject().getFunctionPrivate()->native() == js_fun_apply);
                        JS_ASSERT(sp_ >= vp + 3);
                        argc = sp_ - (vp + 2);
                    }
                    state_ = IMPLICIT_NATIVE;
                    args_ = CallArgsFromVp(argc, vp);
                    return;
                }
            }

            state_ = SCRIPTED;
            JS_ASSERT(sp_ >= fp_->base() && sp_ <= fp_->slots() + fp_->script()->nslots);
            DebugOnly<JSScript *> script = fp_->script();
            JS_ASSERT_IF(!fp_->hasImacropc(),
                         pc_ >= script->code && pc_ < script->code + script->length);
            return;
        }

        









        if (calls_->active() && IsNativeFunction(calls_->calleev())) {
            state_ = NATIVE;
            args_ = *calls_;
            return;
        }

        
        popCall();
    }
}

StackIter::StackIter(JSContext *cx, SavedOption savedOption)
  : cx_(cx),
    savedOption_(savedOption)
{
    if (StackSegment *seg = cx->stack.seg_) {
        startOnSegment(seg);
        settleOnNewState();
    } else {
        state_ = DONE;
    }
}

StackIter &
StackIter::operator++()
{
    JS_ASSERT(!done());
    switch (state_) {
      case DONE:
        JS_NOT_REACHED("");
      case SCRIPTED:
        popFrame();
        settleOnNewState();
        break;
      case NATIVE:
        popCall();
        settleOnNewState();
        break;
      case IMPLICIT_NATIVE:
        state_ = SCRIPTED;
        break;
    }
    return *this;
}

bool
StackIter::operator==(const StackIter &rhs) const
{
    return done() == rhs.done() &&
           (done() ||
            (isScript() == rhs.isScript() &&
             ((isScript() && fp() == rhs.fp()) ||
              (!isScript() && nativeArgs().base() == rhs.nativeArgs().base()))));
}



AllFramesIter::AllFramesIter(StackSpace &space)
  : seg_(space.seg_),
    fp_(seg_ ? seg_->maybefp() : NULL)
{}

AllFramesIter&
AllFramesIter::operator++()
{
    JS_ASSERT(!done());
    fp_ = fp_->prev();
    if (!seg_->contains(fp_)) {
        seg_ = seg_->prevInMemory();
        while (seg_) {
            fp_ = seg_->maybefp();
            if (fp_)
                return *this;
            seg_ = seg_->prevInMemory();
        }
        JS_ASSERT(!fp_);
    }
    return *this;
}
