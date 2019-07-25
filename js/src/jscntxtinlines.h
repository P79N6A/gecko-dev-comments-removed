






































#ifndef jscntxtinlines_h___
#define jscntxtinlines_h___

#include "jscntxt.h"
#include "jsparse.h"
#include "jsxml.h"

inline bool
JSContext::ensureGeneratorStackSpace()
{
    bool ok = genStack.reserve(genStack.length() + 1);
    if (!ok)
        js_ReportOutOfMemory(this);
    return ok;
}

namespace js {

JS_REQUIRES_STACK JS_ALWAYS_INLINE JSStackFrame *
CallStack::getCurrentFrame() const
{
    JS_ASSERT(inContext());
    return isSuspended() ? getSuspendedFrame() : cx->fp;
}

JS_REQUIRES_STACK inline jsval *
StackSpace::firstUnused() const
{
    CallStack *ccs = currentCallStack;
    if (!ccs)
        return base;
    if (JSContext *cx = ccs->maybeContext()) {
        if (!ccs->isSuspended())
            return cx->regs->sp;
        return ccs->getSuspendedRegs()->sp;
    }
    return ccs->getInitialArgEnd();
}


JS_ALWAYS_INLINE void
StackSpace::assertIsCurrent(JSContext *cx) const
{
#ifdef DEBUG
    JS_ASSERT(cx == currentCallStack->maybeContext());
    JS_ASSERT(cx->getCurrentCallStack() == currentCallStack);
    cx->assertCallStacksInSync();
#endif
}

JS_ALWAYS_INLINE bool
StackSpace::ensureSpace(JSContext *maybecx, jsval *from, ptrdiff_t nvals) const
{
    JS_ASSERT(from == firstUnused());
#ifdef XP_WIN
    JS_ASSERT(from <= commitEnd);
    if (commitEnd - from >= nvals)
        return true;
    if (end - from < nvals) {
        if (maybecx)
            js_ReportOutOfScriptQuota(maybecx);
        return false;
    }
    if (!bumpCommit(from, nvals)) {
        if (maybecx)
            js_ReportOutOfScriptQuota(maybecx);
        return false;
    }
    return true;
#else
    if (end - from < nvals) {
        if (maybecx)
            js_ReportOutOfScriptQuota(maybecx);
        return false;
    }
    return true;
#endif
}

JS_ALWAYS_INLINE bool
StackSpace::ensureEnoughSpaceToEnterTrace()
{
#ifdef XP_WIN
    return ensureSpace(NULL, firstUnused(), MAX_TRACE_SPACE_VALS);
#endif
    return end - firstUnused() > MAX_TRACE_SPACE_VALS;
}

JS_REQUIRES_STACK JS_ALWAYS_INLINE JSStackFrame *
StackSpace::getInlineFrame(JSContext *cx, jsval *sp,
                           uintN nmissing, uintN nfixed) const
{
    assertIsCurrent(cx);
    JS_ASSERT(cx->hasActiveCallStack());
    JS_ASSERT(cx->regs->sp == sp);

    ptrdiff_t nvals = nmissing + VALUES_PER_STACK_FRAME + nfixed;
    if (!ensureSpace(cx, sp, nvals))
        return NULL;

    JSStackFrame *fp = reinterpret_cast<JSStackFrame *>(sp + nmissing);
    return fp;
}

JS_REQUIRES_STACK JS_ALWAYS_INLINE void
StackSpace::pushInlineFrame(JSContext *cx, JSStackFrame *fp, jsbytecode *pc,
                            JSStackFrame *newfp)
{
    assertIsCurrent(cx);
    JS_ASSERT(cx->hasActiveCallStack());
    JS_ASSERT(cx->fp == fp && cx->regs->pc == pc);

    fp->savedPC = pc;
    newfp->down = fp;
#ifdef DEBUG
    newfp->savedPC = JSStackFrame::sInvalidPC;
#endif
    cx->setCurrentFrame(newfp);
}

JS_REQUIRES_STACK JS_ALWAYS_INLINE void
StackSpace::popInlineFrame(JSContext *cx, JSStackFrame *up, JSStackFrame *down)
{
    assertIsCurrent(cx);
    JS_ASSERT(cx->hasActiveCallStack());
    JS_ASSERT(cx->fp == up && up->down == down);
    JS_ASSERT(up->savedPC == JSStackFrame::sInvalidPC);

    JSFrameRegs *regs = cx->regs;
    regs->pc = down->savedPC;
    regs->sp = up->argv - 1;
#ifdef DEBUG
    down->savedPC = JSStackFrame::sInvalidPC;
#endif
    cx->setCurrentFrame(down);
}

void
AutoIdArray::trace(JSTracer *trc) {
    JS_ASSERT(tag == IDARRAY);
    js::TraceValues(trc, idArray->length, idArray->vector, "JSAutoIdArray.idArray");
}

class AutoNamespaces : protected AutoGCRooter {
  protected:
    AutoNamespaces(JSContext *cx) : AutoGCRooter(cx, NAMESPACES) {
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  public:
    JSXMLArray array;
};

}

#endif 
