







































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



#ifdef DEBUG
JSObject *const StackFrame::sInvalidScopeChain = (JSObject *)0xbeef;
#endif

jsbytecode *
StackFrame::prevpcSlow()
{
    JS_ASSERT(!(flags_ & HAS_PREVPC));
#if defined(JS_METHODJIT) && defined(JS_MONOIC)
    StackFrame *p = prev();
    js::mjit::JITScript *jit = p->script()->getJIT(p->isConstructing());
    prevpc_ = jit->nativeToPC(ncode_);
    flags_ |= HAS_PREVPC;
    return prevpc_;
#else
    JS_NOT_REACHED("Unknown PC for frame");
    return NULL;
#endif
}

jsbytecode *
StackFrame::pcQuadratic(JSContext *cx)
{
    StackSegment &seg = cx->stack.space().containingSegment(this);
    FrameRegs &regs = seg.currentRegs();

    



    if (regs.fp() == this)
        return regs.pc;

    return seg.computeNextFrame(this)->prevpc();
}



JS_REQUIRES_STACK bool
StackSegment::contains(const StackFrame *fp) const
{
    JS_ASSERT(!empty());

    if (fp < initialFrame_)
        return false;

    StackFrame *start;
    if (isActive())
        start = stack_->fp();
    else
        start = suspendedRegs_->fp();

    if (fp > start)
        return false;

#ifdef DEBUG
    bool found = false;
    StackFrame *stop = initialFrame_->prev();
    for (StackFrame *f = start; !found && f != stop; f = f->prev()) {
        if (f == fp) {
            found = true;
            break;
        }
    }
    JS_ASSERT(found);
#endif

    return true;
}

StackFrame *
StackSegment::computeNextFrame(StackFrame *fp) const
{
    JS_ASSERT(contains(fp));
    JS_ASSERT(fp != currentFrame());

    StackFrame *next = currentFrame();
    StackFrame *prev;
    while ((prev = next->prev()) != fp)
        next = prev;
    return next;
}



StackSpace::StackSpace()
  : base_(NULL),
    commitEnd_(NULL),
    end_(NULL),
    seg_(NULL)
{
    override_.top = NULL;
#ifdef DEBUG
    override_.seg = NULL;
    override_.frame = NULL;
#endif
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

Value *
StackSpace::firstUnused() const
{
    if (!seg_) {
        JS_ASSERT(override_.top == NULL);
        return base_;
    }
    if (!seg_->empty()) {
        Value *sp = seg_->currentRegs().sp;
        if (override_.top > sp) {
            JS_ASSERT(override_.seg == seg_);
            JS_ASSERT_IF(seg_->isActive(), override_.frame == seg_->stack().fp());
            JS_ASSERT_IF(!seg_->isActive(), override_.frame == seg_->suspendedFrame());
            return override_.top;
        }
        return sp;
    }
    JS_ASSERT(override_.seg == seg_);
    return override_.top;
}

StackSegment &
StackSpace::containingSegment(const StackFrame *target) const
{
    for (StackSegment *s = seg_; s; s = s->previousInMemory()) {
        if (s->contains(target))
            return *s;
    }
    JS_NOT_REACHED("frame not in stack space");
    return *(StackSegment *)NULL;
}

void
StackSpace::mark(JSTracer *trc)
{
    



    Value *end = firstUnused();
    for (StackSegment *seg = seg_; seg; seg = seg->previousInMemory()) {
        STATIC_ASSERT(ubound(end) >= 0);
        if (seg->empty()) {
            
            MarkStackRangeConservatively(trc, seg->valueRangeBegin(), end);
        } else {
            
            StackFrame *fp = seg->currentFrame();
            MarkStackRangeConservatively(trc, fp->slots(), end);

            
            StackFrame *initial = seg->initialFrame();
            for (StackFrame *f = fp; f != initial; f = f->prev()) {
                js_TraceStackFrame(trc, f);
                MarkStackRangeConservatively(trc, f->prev()->slots(), (Value *)f);
            }

            
            js_TraceStackFrame(trc, initial);
            MarkStackRangeConservatively(trc, seg->valueRangeBegin(), (Value *)initial);
        }
        end = (Value *)seg;
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

void
StackSpace::popSegment()
{
    JS_ASSERT(seg_->empty());
    seg_ = seg_->previousInMemory();
}

void
StackSpace::pushSegment(StackSegment &seg)
{
    JS_ASSERT(seg.empty());
    seg.setPreviousInMemory(seg_);
    seg_ = &seg;
}

size_t
StackSpace::committedSize()
{
    return (commitEnd_ - base_) * sizeof(Value);
}



ContextStack::ContextStack(JSContext *cx)
  : regs_(NULL),
    seg_(NULL),
    space_(&JS_THREAD_DATA(cx)->stackSpace),
    cx_(cx)
{
    threadReset();
}

ContextStack::~ContextStack()
{
    JS_ASSERT(!regs_);
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
ContextStack::assertSegmentsInSync() const
{
    if (regs_) {
        JS_ASSERT(seg_->isActive());
        if (StackSegment *prev = seg_->previousInContext())
            JS_ASSERT(!prev->isActive());
    } else {
        JS_ASSERT_IF(seg_, !seg_->isActive());
    }
}

void
ContextStack::assertSpaceInSync() const
{
    JS_ASSERT(space_);
    JS_ASSERT(space_ == &JS_THREAD_DATA(cx_)->stackSpace);
}

bool
ContextStack::contains(const StackFrame *fp) const
{
    return &space().containingSegment(fp).stack() == this;
}
#endif

void
ContextStack::saveActiveSegment()
{
    JS_ASSERT(regs_);
    seg_->save(*regs_);
    regs_ = NULL;
    cx_->resetCompartment();
}

void
ContextStack::restoreSegment()
{
    regs_ = &seg_->suspendedRegs();
    seg_->restore();
    cx_->resetCompartment();
}

StackFrame *
ContextStack::getSegmentAndFrame(JSContext *cx, uintN vplen, uintN nslots,
                                 FrameGuard *frameGuard) const
{
    Value *start = space().firstUnused();
    uintN nvals = VALUES_PER_STACK_SEGMENT + vplen + VALUES_PER_STACK_FRAME + nslots;
    if (!space().ensureSpace(cx, start, nvals))
        return NULL;

    StackSegment *seg = new(start) StackSegment;
    Value *vp = seg->valueRangeBegin();
    frameGuard->seg_ = seg;
    return reinterpret_cast<StackFrame *>(vp + vplen);
}

void
ContextStack::pushSegmentAndFrameImpl(FrameRegs &regs, StackSegment &seg)
{
    JS_ASSERT(&seg == space().currentSegment());

    if (regs_)
        seg_->suspend(*regs_);
    regs_ = &regs;

    seg.setPreviousInContext(seg_);
    seg_ = &seg;
    seg.joinContext(*this, *regs.fp());
}

void
ContextStack::pushSegmentAndFrame(FrameGuard *frameGuard)
{
    space().pushSegment(*frameGuard->seg_);
    pushSegmentAndFrameImpl(frameGuard->regs_, *frameGuard->seg_);
    frameGuard->stack_ = this;
}

void
ContextStack::popSegmentAndFrameImpl()
{
    JS_ASSERT(isCurrentAndActive());
    JS_ASSERT(&seg_->stack() == this);
    JS_ASSERT(seg_->initialFrame() == regs_->fp());

    regs_->fp()->putActivationObjects();

    seg_->leaveContext();
    seg_ = seg_->previousInContext();
    if (seg_) {
        if (seg_->isSaved()) {
            regs_ = NULL;
        } else {
            regs_ = &seg_->suspendedRegs();
            seg_->resume();
        }
    } else {
        JS_ASSERT(regs_->fp()->prev() == NULL);
        regs_ = NULL;
    }
}

void
ContextStack::popSegmentAndFrame()
{
    popSegmentAndFrameImpl();
    space().popSegment();
    notifyIfNoCodeRunning();
}

FrameGuard::~FrameGuard()
{
    if (!pushed())
        return;
    JS_ASSERT(stack_->currentSegment() == seg_);
    JS_ASSERT(stack_->currentSegment()->currentFrame() == regs_.fp());
    stack_->popSegmentAndFrame();
}

bool
ContextStack::pushExecuteFrame(JSContext *cx, JSScript *script, const Value &thisv,
                               JSObject &scopeChain, ExecuteType type,
                               StackFrame *evalInFrame, ExecuteFrameGuard *efg)
{
    StackFrame *fp = getSegmentAndFrame(cx, 2, script->nslots, efg);
    if (!fp)
        return false;

    
    StackFrame *prev = evalInFrame ? evalInFrame : maybefp();

    
    efg->regs_.prepareToRun(fp, script);
    SetValueRangeToNull(fp->slots(), script->nfixed);
    fp->initExecuteFrame(script, prev, thisv, scopeChain, type);

    pushSegmentAndFrame(efg);
    return true;
}

bool
ContextStack::pushGeneratorFrame(JSContext *cx, JSGenerator *gen, GeneratorFrameGuard *gfg)
{
    StackFrame *genfp = gen->floatingFrame();
    Value *genvp = gen->floatingStack;
    uintN vplen = (Value *)genfp - genvp;

    StackFrame *stackfp = getSegmentAndFrame(cx, vplen, genfp->numSlots(), gfg);
    if (!stackfp)
        return false;

    Value *stackvp = (Value *)stackfp - vplen;

    
    gfg->gen_ = gen;
    gfg->stackvp_ = stackvp;

    
    gfg->regs_.rebaseFromTo(gen->regs, stackfp);
    stackfp->stealFrameAndSlots(stackvp, genfp, genvp, gen->regs.sp);
    stackfp->resetGeneratorPrev(cx);
    stackfp->unsetFloatingGenerator();

    pushSegmentAndFrame(gfg);
    return true;
}

void
ContextStack::popGeneratorFrame(GeneratorFrameGuard *gfg)
{
    JSGenerator *gen = gfg->gen_;
    StackFrame *genfp = gen->floatingFrame();
    Value *genvp = gen->floatingStack;

    FrameRegs &stackRegs = gfg->regs_;
    StackFrame *stackfp = stackRegs.fp();
    Value *stackvp = gfg->stackvp_;

    
    gen->regs.rebaseFromTo(stackRegs, genfp);
    genfp->stealFrameAndSlots(genvp, stackfp, stackvp, stackRegs.sp);
    genfp->setFloatingGenerator();

    
}

GeneratorFrameGuard::~GeneratorFrameGuard()
{
    if (pushed())
        stack_->popGeneratorFrame(this);
}

bool
ContextStack::pushDummyFrame(JSContext *cx, JSObject &scopeChain,
                             DummyFrameGuard *dfg)
{
    StackFrame *fp = getSegmentAndFrame(cx, 0 , 0 , dfg);
    if (!fp)
        return false;

    fp->initDummyFrame(cx, scopeChain);
    dfg->regs_.initDummyFrame(fp);

    pushSegmentAndFrame(dfg);
    return true;
}

bool
ContextStack::pushInvokeArgsSlow(JSContext *cx, uintN argc,
                                 InvokeArgsGuard *argsGuard)
{
    




    JS_ASSERT(!isCurrentAndActive());

    Value *start = space().firstUnused();
    size_t vplen = 2 + argc;
    ptrdiff_t nvals = VALUES_PER_STACK_SEGMENT + vplen;
    if (!space().ensureSpace(cx, start, nvals))
        return false;

    StackSegment *seg = new(start) StackSegment;
    argsGuard->seg_ = seg;

    Value *vp = seg->valueRangeBegin();
    ImplicitCast<CallArgs>(*argsGuard) = CallArgsFromVp(argc, vp);

    



    space().pushSegment(*seg);
    space().pushOverride(vp + vplen, &argsGuard->prevOverride_);

    argsGuard->stack_ = this;
    return true;
}

void
ContextStack::popInvokeArgsSlow(const InvokeArgsGuard &argsGuard)
{
    JS_ASSERT(space().currentSegment()->empty());

    space().popOverride(argsGuard.prevOverride_);
    space().popSegment();
    notifyIfNoCodeRunning();
}

void
ContextStack::pushInvokeFrameSlow(InvokeFrameGuard *ifg)
{
    JS_ASSERT(space().seg_->empty());
    pushSegmentAndFrameImpl(ifg->regs_, *space().seg_);
    ifg->stack_ = this;
}

void
ContextStack::popInvokeFrameSlow(const InvokeFrameGuard &ifg)
{
    JS_ASSERT(ifg.regs_.fp() == seg_->initialFrame());
    popSegmentAndFrameImpl();
}





void
ContextStack::notifyIfNoCodeRunning()
{
    if (regs_)
        return;

    cx_->resetCompartment();
    cx_->maybeMigrateVersionOverride();
}



FrameRegsIter::FrameRegsIter(JSContext *cx)
  : cx_(cx)
{
    LeaveTrace(cx);
    seg_ = cx->stack.currentSegment();
    if (!seg_) {
        fp_ = NULL;
        sp_ = NULL;
        pc_ = NULL;
        return;
    }
    if (!seg_->isActive()) {
        JS_ASSERT(seg_->isSuspended());
        fp_ = seg_->suspendedFrame();
        sp_ = seg_->suspendedRegs().sp;
        pc_ = seg_->suspendedRegs().pc;
        return;
    }
    fp_ = cx->fp();
    sp_ = cx->regs().sp;
    pc_ = cx->regs().pc;
    return;
}

FrameRegsIter &
FrameRegsIter::operator++()
{
    StackFrame *oldfp = fp_;
    fp_ = fp_->prev();
    if (!fp_)
        return *this;

    if (oldfp != seg_->initialFrame()) {
        pc_ = oldfp->prevpc();
        sp_ = oldfp->formalArgsEnd();
        return *this;
    }

    JS_ASSERT(oldfp == seg_->initialFrame());
    JS_ASSERT(fp_ == oldfp->prev());

    





    seg_ = seg_->previousInContext();
    sp_ = seg_->suspendedRegs().sp;
    pc_ = seg_->suspendedRegs().pc;
    StackFrame *f = seg_->suspendedFrame();
    while (f != fp_) {
        if (f == seg_->initialFrame()) {
            seg_ = seg_->previousInContext();
            sp_ = seg_->suspendedRegs().sp;
            pc_ = seg_->suspendedRegs().pc;
            f = seg_->suspendedFrame();
        } else {
            sp_ = f->formalArgsEnd();
            pc_ = f->prevpc();
            f = f->prev();
        }
    }
    return *this;
}

bool
FrameRegsIter::operator==(const FrameRegsIter &rhs) const
{
    return done() == rhs.done() && (done() || fp_ == rhs.fp_);
}



AllFramesIter::AllFramesIter(JSContext *cx)
  : seg_(cx->stack.currentSegment()),
    fp_(seg_ ? seg_->currentFrame() : NULL)
{
}

AllFramesIter&
AllFramesIter::operator++()
{
    JS_ASSERT(!done());
    if (fp_ == seg_->initialFrame()) {
        seg_ = seg_->previousInMemory();
        fp_ = seg_ ? seg_->currentFrame() : NULL;
    } else {
        fp_ = fp_->prev();
    }
    return *this;
}

