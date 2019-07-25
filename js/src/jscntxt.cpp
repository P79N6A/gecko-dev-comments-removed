










































#include <new>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#ifdef ANDROID
# include <fstream>
# include <string>
#endif  

#include "jsstdint.h"

#include "jstypes.h"
#include "jsarena.h"
#include "jsutil.h"
#include "jsclist.h"
#include "jsprf.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsdbgapi.h"
#include "jsexn.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsmath.h"
#include "jsnativestack.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jspubtd.h"
#include "jsscan.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstaticcheck.h"
#include "jsstr.h"
#include "jstracer.h"

#ifdef JS_METHODJIT
# include "assembler/assembler/MacroAssembler.h"
#endif

#include "jscntxtinlines.h"
#include "jscompartment.h"
#include "jsinterpinlines.h"
#include "jsobjinlines.h"

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
using namespace js::gc;

static const size_t ARENA_HEADER_SIZE_HACK = 40;
static const size_t TEMP_POOL_CHUNK_SIZE = 4096 - ARENA_HEADER_SIZE_HACK;

static void
FreeContext(JSContext *cx);

#ifdef DEBUG
JS_REQUIRES_STACK bool
StackSegment::contains(const JSStackFrame *fp) const
{
    JS_ASSERT(inContext());
    JSStackFrame *start;
    JSStackFrame *stop;
    if (isActive()) {
        JS_ASSERT(cx->hasfp());
        start = cx->fp();
        stop = cx->activeSegment()->initialFrame->prev();
    } else {
        JS_ASSERT(suspendedRegs && suspendedRegs->fp);
        start = suspendedRegs->fp;
        stop = initialFrame->prev();
    }
    for (JSStackFrame *f = start; f != stop; f = f->prev()) {
        if (f == fp)
            return true;
    }
    return false;
}
#endif

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
    base = reinterpret_cast<Value *>(p);
    commitEnd = base + COMMIT_VALS;
    end = base + CAPACITY_VALS;
#elif defined(XP_OS2)
    if (DosAllocMem(&p, CAPACITY_BYTES, PAG_COMMIT | PAG_READ | PAG_WRITE | OBJ_ANY) &&
        DosAllocMem(&p, CAPACITY_BYTES, PAG_COMMIT | PAG_READ | PAG_WRITE))
        return false;
    base = reinterpret_cast<Value *>(p);
    end = base + CAPACITY_VALS;
#else
    JS_ASSERT(CAPACITY_BYTES % getpagesize() == 0);
    p = mmap(NULL, CAPACITY_BYTES, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED)
        return false;
    base = reinterpret_cast<Value *>(p);
    end = base + CAPACITY_VALS;
#endif
    return true;
}

void
StackSpace::finish()
{
#ifdef XP_WIN
    VirtualFree(base, (commitEnd - base) * sizeof(Value), MEM_DECOMMIT);
    VirtualFree(base, 0, MEM_RELEASE);
#elif defined(XP_OS2)
    DosFreeMem(base);
#else
#ifdef SOLARIS
    munmap((caddr_t)base, CAPACITY_BYTES);
#else
    munmap(base, CAPACITY_BYTES);
#endif
#endif
}

#ifdef XP_WIN
JS_FRIEND_API(bool)
StackSpace::bumpCommit(Value *from, ptrdiff_t nvals) const
{
    JS_ASSERT(end - from >= nvals);
    Value *newCommit = commitEnd;
    Value *request = from + nvals;

    
    JS_ASSERT((end - newCommit) % COMMIT_VALS == 0);
    do {
        newCommit += COMMIT_VALS;
        JS_ASSERT((end - newCommit) >= 0);
    } while (newCommit < request);

    
    int32 size = static_cast<int32>(newCommit - commitEnd) * sizeof(Value);

    if (!VirtualAlloc(commitEnd, size, MEM_COMMIT, PAGE_READWRITE))
        return false;
    commitEnd = newCommit;
    return true;
}
#endif

void
StackSpace::mark(JSTracer *trc)
{
    








    Value *end = firstUnused();
    for (StackSegment *seg = currentSegment; seg; seg = seg->getPreviousInMemory()) {
        STATIC_ASSERT(ubound(end) >= 0);
        if (seg->inContext()) {
            
            if (seg->hasInitialVarObj())
                MarkObject(trc, seg->getInitialVarObj(), "varobj");

            
            JSStackFrame *fp = seg->getCurrentFrame();
            MarkStackRangeConservatively(trc, fp->slots(), end);

            
            JSStackFrame *initial = seg->getInitialFrame();
            for (JSStackFrame *f = fp; f != initial; f = f->prev()) {
                js_TraceStackFrame(trc, f);
                MarkStackRangeConservatively(trc, f->prev()->slots(), (Value *)f);
            }

            
            js_TraceStackFrame(trc, initial);
            MarkStackRangeConservatively(trc, seg->valueRangeBegin(), (Value *)initial);
        } else {
            
            MarkValueRange(trc, seg->valueRangeBegin(), end, "stack");
        }
        end = (Value *)seg;
    }
}

bool
StackSpace::pushSegmentForInvoke(JSContext *cx, uintN argc, InvokeArgsGuard *ag)
{
    Value *start = firstUnused();
    ptrdiff_t nvals = VALUES_PER_STACK_SEGMENT + 2 + argc;
    if (!ensureSpace(cx, start, nvals))
        return false;

    StackSegment *seg = new(start) StackSegment;
    seg->setPreviousInMemory(currentSegment);
    currentSegment = seg;

    ag->cx = cx;
    ag->seg = seg;
    ag->argv_ = seg->valueRangeBegin() + 2;
    ag->argc_ = argc;

    
#ifdef DEBUG
    ag->prevInvokeSegment = invokeSegment;
    invokeSegment = seg;
    ag->prevInvokeFrame = invokeFrame;
    invokeFrame = NULL;
#endif
    ag->prevInvokeArgEnd = invokeArgEnd;
    invokeArgEnd = ag->argv() + ag->argc();
    return true;
}

void
StackSpace::popSegmentForInvoke(const InvokeArgsGuard &ag)
{
    JS_ASSERT(!currentSegment->inContext());
    JS_ASSERT(ag.seg == currentSegment);
    JS_ASSERT(invokeSegment == currentSegment);
    JS_ASSERT(invokeArgEnd == ag.argv() + ag.argc());

    currentSegment = currentSegment->getPreviousInMemory();

#ifdef DEBUG
    invokeSegment = ag.prevInvokeSegment;
    invokeFrame = ag.prevInvokeFrame;
#endif
    invokeArgEnd = ag.prevInvokeArgEnd;
}

bool
StackSpace::getSegmentAndFrame(JSContext *cx, uintN vplen, uintN nfixed,
                               FrameGuard *fg) const
{
    Value *start = firstUnused();
    uintN nvals = VALUES_PER_STACK_SEGMENT + vplen + VALUES_PER_STACK_FRAME + nfixed;
    if (!ensureSpace(cx, start, nvals))
        return false;

    fg->seg_ = new(start) StackSegment;
    fg->vp_ = start + VALUES_PER_STACK_SEGMENT;
    fg->fp_ = reinterpret_cast<JSStackFrame *>(fg->vp() + vplen);
    return true;
}

void
StackSpace::pushSegmentAndFrame(JSContext *cx, JSObject *initialVarObj,
                                JSFrameRegs *regs, FrameGuard *fg)
{
    
    JS_ASSERT(regs->fp == fg->fp());

    
    StackSegment *seg = fg->segment();
    seg->setPreviousInMemory(currentSegment);
    currentSegment = seg;

    
    cx->pushSegmentAndFrame(seg, *regs);
    seg->setInitialVarObj(initialVarObj);
    fg->cx_ = cx;
}

void
StackSpace::popSegmentAndFrame(JSContext *cx)
{
    JS_ASSERT(isCurrentAndActive(cx));
    JS_ASSERT(cx->hasActiveSegment());
    cx->popSegmentAndFrame();
    currentSegment = currentSegment->getPreviousInMemory();
}

FrameGuard::~FrameGuard()
{
    if (!pushed())
        return;
    JS_ASSERT(cx_->activeSegment() == segment());
    JS_ASSERT(cx_->maybefp() == fp());
    cx_->stack().popSegmentAndFrame(cx_);
}

bool
StackSpace::getExecuteFrame(JSContext *cx, JSScript *script, ExecuteFrameGuard *fg) const
{
    return getSegmentAndFrame(cx, 2, script->nfixed, fg);
}

void
StackSpace::pushExecuteFrame(JSContext *cx, JSObject *initialVarObj, ExecuteFrameGuard *fg)
{
    JSStackFrame *fp = fg->fp();
    JSScript *script = fp->script();
    fg->regs_.pc = script->code;
    fg->regs_.fp = fp;
    fg->regs_.sp = fp->base();
    pushSegmentAndFrame(cx, initialVarObj, &fg->regs_, fg);
}

bool
StackSpace::pushDummyFrame(JSContext *cx, JSObject &scopeChain, DummyFrameGuard *fg)
{
    if (!getSegmentAndFrame(cx, 0 , 0 , fg))
        return false;
    fg->fp()->initDummyFrame(cx, scopeChain);
    fg->regs_.fp = fg->fp();
    fg->regs_.pc = NULL;
    fg->regs_.sp = fg->fp()->slots();
    pushSegmentAndFrame(cx, NULL , &fg->regs_, fg);
    return true;
}

bool
StackSpace::getGeneratorFrame(JSContext *cx, uintN vplen, uintN nfixed, GeneratorFrameGuard *fg)
{
    return getSegmentAndFrame(cx, vplen, nfixed, fg);
}

void
StackSpace::pushGeneratorFrame(JSContext *cx, JSFrameRegs *regs, GeneratorFrameGuard *fg)
{
    JS_ASSERT(regs->fp == fg->fp());
    JS_ASSERT(regs->fp->prev() == cx->maybefp());
    pushSegmentAndFrame(cx, NULL , regs, fg);
}

bool
StackSpace::bumpCommitAndLimit(JSStackFrame *base, Value *sp, uintN nvals, Value **limit) const
{
    JS_ASSERT(sp >= firstUnused());
    JS_ASSERT(sp + nvals >= *limit);
#ifdef XP_WIN
    if (commitEnd <= *limit) {
        Value *quotaEnd = (Value *)base + STACK_QUOTA;
        if (sp + nvals < quotaEnd) {
            if (!ensureSpace(NULL, sp, nvals))
                return false;
            *limit = Min(quotaEnd, commitEnd);
            return true;
        }
    }
#endif
    return false;
}

void
FrameRegsIter::initSlow()
{
    if (!curseg) {
        curfp = NULL;
        cursp = NULL;
        curpc = NULL;
        return;
    }

    JS_ASSERT(curseg->isSuspended());
    curfp = curseg->getSuspendedFrame();
    cursp = curseg->getSuspendedRegs()->sp;
    curpc = curseg->getSuspendedRegs()->pc;
}







void
FrameRegsIter::incSlow(JSStackFrame *fp, JSStackFrame *prev)
{
    JS_ASSERT(prev);
    JS_ASSERT(curpc == curfp->pc(cx, fp));
    JS_ASSERT(fp == curseg->getInitialFrame());

    






    curseg = curseg->getPreviousInContext();
    cursp = curseg->getSuspendedRegs()->sp;
    JSStackFrame *f = curseg->getSuspendedFrame();
    while (f != prev) {
        if (f == curseg->getInitialFrame()) {
            curseg = curseg->getPreviousInContext();
            cursp = curseg->getSuspendedRegs()->sp;
            f = curseg->getSuspendedFrame();
        } else {
            cursp = f->formalArgsEnd();
            f = f->prev();
        }
    }
}

AllFramesIter::AllFramesIter(JSContext *cx)
  : curcs(cx->stack().getCurrentSegment()),
    curfp(curcs ? curcs->getCurrentFrame() : NULL)
{
}

AllFramesIter&
AllFramesIter::operator++()
{
    JS_ASSERT(!done());
    if (curfp == curcs->getInitialFrame()) {
        curcs = curcs->getPreviousInMemory();
        curfp = curcs ? curcs->getCurrentFrame() : NULL;
    } else {
        curfp = curfp->prev();
    }
    return *this;
}

bool
JSThreadData::init()
{
#ifdef DEBUG
    
    for (size_t i = 0; i != sizeof(*this); ++i)
        JS_ASSERT(reinterpret_cast<uint8*>(this)[i] == 0);
#endif
    if (!stackSpace.init())
        return false;
#ifdef JS_TRACER
    InitJIT(&traceMonitor);
#endif
    dtoaState = js_NewDtoaState();
    if (!dtoaState) {
        finish();
        return false;
    }
    nativeStackBase = GetNativeStackBase();
    return true;
}

MathCache *
JSThreadData::allocMathCache(JSContext *cx)
{
    JS_ASSERT(!mathCache);
    mathCache = new MathCache;
    if (!mathCache)
        js_ReportOutOfMemory(cx);
    return mathCache;
}

void
JSThreadData::finish()
{
#ifdef DEBUG
    for (size_t i = 0; i != JS_ARRAY_LENGTH(scriptsToGC); ++i)
        JS_ASSERT(!scriptsToGC[i]);
#endif

    if (dtoaState)
        js_DestroyDtoaState(dtoaState);

    js_FinishGSNCache(&gsnCache);
    propertyCache.~PropertyCache();
#if defined JS_TRACER
    FinishJIT(&traceMonitor);
#endif
    stackSpace.finish();
    delete mathCache;
}

void
JSThreadData::mark(JSTracer *trc)
{
    stackSpace.mark(trc);
}

void
JSThreadData::purge(JSContext *cx)
{
    js_PurgeGSNCache(&gsnCache);

    
    propertyCache.purge(cx);

#ifdef JS_TRACER
    



    if (cx->runtime->gcRegenShapes)
        traceMonitor.needFlush = JS_TRUE;
#endif

    
    js_DestroyScriptsToGC(cx, this);

    
    memset(cachedNativeIterators, 0, sizeof(cachedNativeIterators));
    lastNativeIterator = NULL;

    dtoaCache.s = NULL;
}

#ifdef JS_THREADSAFE

static JSThread *
NewThread(void *id)
{
    JS_ASSERT(js_CurrentThreadId() == id);
    JSThread *thread = (JSThread *) js_calloc(sizeof(JSThread));
    if (!thread)
        return NULL;
    JS_INIT_CLIST(&thread->contextList);
    thread->id = id;
    if (!thread->data.init()) {
        js_free(thread);
        return NULL;
    }
    return thread;
}

static void
DestroyThread(JSThread *thread)
{
    
    JS_ASSERT(JS_CLIST_IS_EMPTY(&thread->contextList));

    



    JS_ASSERT(!thread->data.conservativeGC.hasStackToScan());

    thread->data.finish();
    js_free(thread);
}

JSThread *
js_CurrentThread(JSRuntime *rt)
{
    void *id = js_CurrentThreadId();
    JS_LOCK_GC(rt);

    



    js_WaitForGC(rt);

    JSThread *thread;
    JSThread::Map::AddPtr p = rt->threads.lookupForAdd(id);
    if (p) {
        thread = p->value;

        



        if (JS_CLIST_IS_EMPTY(&thread->contextList))
            thread->data.nativeStackBase = GetNativeStackBase();
    } else {
        JS_UNLOCK_GC(rt);
        thread = NewThread(id);
        if (!thread)
            return NULL;
        JS_LOCK_GC(rt);
        js_WaitForGC(rt);
        if (!rt->threads.relookupOrAdd(p, id, thread)) {
            JS_UNLOCK_GC(rt);
            DestroyThread(thread);
            return NULL;
        }

        
        JS_ASSERT(p->value == thread);
    }
    JS_ASSERT(thread->id == id);
    JS_ASSERT(thread->data.nativeStackBase == GetNativeStackBase());

    return thread;
}

JSBool
js_InitContextThread(JSContext *cx)
{
    JSThread *thread = js_CurrentThread(cx->runtime);
    if (!thread)
        return false;

    JS_APPEND_LINK(&cx->threadLinks, &thread->contextList);
    cx->thread = thread;
    return true;
}

void
js_ClearContextThread(JSContext *cx)
{
    JS_ASSERT(CURRENT_THREAD_IS_ME(cx->thread));
    JS_REMOVE_AND_INIT_LINK(&cx->threadLinks);
    cx->thread = NULL;
}

#endif 

JSThreadData *
js_CurrentThreadData(JSRuntime *rt)
{
#ifdef JS_THREADSAFE
    JSThread *thread = js_CurrentThread(rt);
    if (!thread)
        return NULL;

    return &thread->data;
#else
    return &rt->threadData;
#endif
}

JSBool
js_InitThreads(JSRuntime *rt)
{
#ifdef JS_THREADSAFE
    if (!rt->threads.init(4))
        return false;
#else
    if (!rt->threadData.init())
        return false;
#endif
    return true;
}

void
js_FinishThreads(JSRuntime *rt)
{
#ifdef JS_THREADSAFE
    if (!rt->threads.initialized())
        return;
    for (JSThread::Map::Range r = rt->threads.all(); !r.empty(); r.popFront()) {
        JSThread *thread = r.front().value;
        JS_ASSERT(JS_CLIST_IS_EMPTY(&thread->contextList));
        DestroyThread(thread);
    }
    rt->threads.clear();
#else
    rt->threadData.finish();
#endif
}

void
js_PurgeThreads(JSContext *cx)
{
#ifdef JS_THREADSAFE
    for (JSThread::Map::Enum e(cx->runtime->threads);
         !e.empty();
         e.popFront()) {
        JSThread *thread = e.front().value;

        if (JS_CLIST_IS_EMPTY(&thread->contextList)) {
            JS_ASSERT(cx->thread != thread);
            js_DestroyScriptsToGC(cx, &thread->data);

            DestroyThread(thread);
            e.removeFront();
        } else {
            thread->data.purge(cx);
        }
    }
#else
    cx->runtime->threadData.purge(cx);
#endif
}

JSContext *
js_NewContext(JSRuntime *rt, size_t stackChunkSize)
{
    JSContext *cx;
    JSBool ok, first;
    JSContextCallback cxCallback;

    




    void *mem = js_calloc(sizeof *cx);
    if (!mem)
        return NULL;

    cx = new (mem) JSContext(rt);
    cx->debugHooks = &rt->globalDebugHooks;
#if JS_STACK_GROWTH_DIRECTION > 0
    cx->stackLimit = (jsuword) -1;
#endif
    cx->scriptStackQuota = JS_DEFAULT_SCRIPT_STACK_QUOTA;
    JS_STATIC_ASSERT(JSVERSION_DEFAULT == 0);
    JS_ASSERT(cx->findVersion() == JSVERSION_DEFAULT);
    VOUCH_DOES_NOT_REQUIRE_STACK();

    JS_InitArenaPool(&cx->tempPool, "temp", TEMP_POOL_CHUNK_SIZE, sizeof(jsdouble),
                     &cx->scriptStackQuota);
    JS_InitArenaPool(&cx->regExpPool, "regExp", TEMP_POOL_CHUNK_SIZE, sizeof(int),
                     &cx->scriptStackQuota);

    JS_ASSERT(cx->resolveFlags == 0);

#ifdef JS_THREADSAFE
    if (!js_InitContextThread(cx)) {
        FreeContext(cx);
        return NULL;
    }
#endif

    



    for (;;) {
        if (rt->state == JSRTS_UP) {
            JS_ASSERT(!JS_CLIST_IS_EMPTY(&rt->contextList));
            first = JS_FALSE;
            break;
        }
        if (rt->state == JSRTS_DOWN) {
            JS_ASSERT(JS_CLIST_IS_EMPTY(&rt->contextList));
            first = JS_TRUE;
            rt->state = JSRTS_LAUNCHING;
            break;
        }
        JS_WAIT_CONDVAR(rt->stateChange, JS_NO_TIMEOUT);

        






        js_WaitForGC(rt);
    }
    JS_APPEND_LINK(&cx->link, &rt->contextList);
    JS_UNLOCK_GC(rt);

    js_InitRandom(cx);

    







    if (first) {
#ifdef JS_THREADSAFE
        JS_BeginRequest(cx);
#endif
        ok = js_InitCommonAtoms(cx);

        




        if (ok && !rt->scriptFilenameTable)
            ok = js_InitRuntimeScriptState(rt);
        if (ok)
            ok = js_InitRuntimeNumberState(cx);
        if (ok) {
            





            uint32 shapeGen = rt->shapeGen;
            rt->shapeGen = 0;
            ok = Shape::initRuntimeState(cx);
            if (rt->shapeGen < shapeGen)
                rt->shapeGen = shapeGen;
        }

#ifdef JS_THREADSAFE
        JS_EndRequest(cx);
#endif
        if (!ok) {
            js_DestroyContext(cx, JSDCM_NEW_FAILED);
            return NULL;
        }

        AutoLockGC lock(rt);
        rt->state = JSRTS_UP;
        JS_NOTIFY_ALL_CONDVAR(rt->stateChange);
    }

    cxCallback = rt->cxCallback;
    if (cxCallback && !cxCallback(cx, JSCONTEXT_NEW)) {
        js_DestroyContext(cx, JSDCM_NEW_FAILED);
        return NULL;
    }

    
    if (!cx->busyArrays.init()) {
        FreeContext(cx);
        return NULL;
    }

    return cx;
}

#if defined DEBUG && defined XP_UNIX
# include <stdio.h>

class JSAutoFile {
public:
    JSAutoFile() : mFile(NULL) {}

    ~JSAutoFile() {
        if (mFile)
            fclose(mFile);
    }

    FILE *open(const char *fname, const char *mode) {
        return mFile = fopen(fname, mode);
    }
    operator FILE *() {
        return mFile;
    }

private:
    FILE *mFile;
};

static void
DumpEvalCacheMeter(JSContext *cx)
{
    if (const char *filename = getenv("JS_EVALCACHE_STATFILE")) {
        struct {
            const char *name;
            ptrdiff_t  offset;
        } table[] = {
#define frob(x) { #x, offsetof(JSEvalCacheMeter, x) }
            EVAL_CACHE_METER_LIST(frob)
#undef frob
        };
        JSEvalCacheMeter *ecm = &JS_THREAD_DATA(cx)->evalCacheMeter;

        static JSAutoFile fp;
        if (!fp && !fp.open(filename, "w"))
            return;

        fprintf(fp, "eval cache meter (%p):\n",
#ifdef JS_THREADSAFE
                (void *) cx->thread
#else
                (void *) cx->runtime
#endif
                );
        for (uintN i = 0; i < JS_ARRAY_LENGTH(table); ++i) {
            fprintf(fp, "%-8.8s  %llu\n",
                    table[i].name,
                    (unsigned long long int) *(uint64 *)((uint8 *)ecm + table[i].offset));
        }
        fprintf(fp, "hit ratio %g%%\n", ecm->hit * 100. / ecm->probe);
        fprintf(fp, "avg steps %g\n", double(ecm->step) / ecm->probe);
        fflush(fp);
    }
}
# define DUMP_EVAL_CACHE_METER(cx) DumpEvalCacheMeter(cx)

static void
DumpFunctionCountMap(const char *title, JSRuntime::FunctionCountMap &map, FILE *fp)
{
    fprintf(fp, "\n%s count map:\n", title);

    for (JSRuntime::FunctionCountMap::Range r = map.all(); !r.empty(); r.popFront()) {
        JSFunction *fun = r.front().key;
        int32 count = r.front().value;

        fprintf(fp, "%10d %s:%u\n", count, fun->u.i.script->filename, fun->u.i.script->lineno);
    }
}

static void
DumpFunctionMeter(JSContext *cx)
{
    if (const char *filename = cx->runtime->functionMeterFilename) {
        struct {
            const char *name;
            ptrdiff_t  offset;
        } table[] = {
#define frob(x) { #x, offsetof(JSFunctionMeter, x) }
            FUNCTION_KIND_METER_LIST(frob)
#undef frob
        };
        JSFunctionMeter *fm = &cx->runtime->functionMeter;

        static JSAutoFile fp;
        if (!fp && !fp.open(filename, "w"))
            return;

        fprintf(fp, "function meter (%s):\n", cx->runtime->lastScriptFilename);
        for (uintN i = 0; i < JS_ARRAY_LENGTH(table); ++i)
            fprintf(fp, "%-19.19s %d\n", table[i].name, *(int32 *)((uint8 *)fm + table[i].offset));

        DumpFunctionCountMap("method read barrier", cx->runtime->methodReadBarrierCountMap, fp);
        DumpFunctionCountMap("unjoined function", cx->runtime->unjoinedFunctionCountMap, fp);

        putc('\n', fp);
        fflush(fp);
    }
}

# define DUMP_FUNCTION_METER(cx)   DumpFunctionMeter(cx)

#endif 

#ifndef DUMP_EVAL_CACHE_METER
# define DUMP_EVAL_CACHE_METER(cx) ((void) 0)
#endif

#ifndef DUMP_FUNCTION_METER
# define DUMP_FUNCTION_METER(cx)   ((void) 0)
#endif

void
js_DestroyContext(JSContext *cx, JSDestroyContextMode mode)
{
    JSRuntime *rt;
    JSContextCallback cxCallback;
    JSBool last;

    JS_ASSERT(!cx->enumerators);

    rt = cx->runtime;
#ifdef JS_THREADSAFE
    




    JS_ASSERT(cx->thread && CURRENT_THREAD_IS_ME(cx->thread));
    if (!cx->thread)
        JS_SetContextThread(cx);

    





    JS_ASSERT(cx->outstandingRequests <= cx->thread->data.requestDepth);
#endif

    if (mode != JSDCM_NEW_FAILED) {
        cxCallback = rt->cxCallback;
        if (cxCallback) {
            



#ifdef DEBUG
            JSBool callbackStatus =
#endif
            cxCallback(cx, JSCONTEXT_DESTROY);
            JS_ASSERT(callbackStatus);
        }
    }

    JS_LOCK_GC(rt);
    JS_ASSERT(rt->state == JSRTS_UP || rt->state == JSRTS_LAUNCHING);
#ifdef JS_THREADSAFE
    



    if (cx->thread->data.requestDepth == 0)
        js_WaitForGC(rt);
#endif
    JS_REMOVE_LINK(&cx->link);
    last = (rt->contextList.next == &rt->contextList);
    if (last)
        rt->state = JSRTS_LANDING;
    if (last || mode == JSDCM_FORCE_GC || mode == JSDCM_MAYBE_GC
#ifdef JS_THREADSAFE
        || cx->outstandingRequests != 0
#endif
        ) {
        JS_ASSERT(!rt->gcRunning);

        JS_UNLOCK_GC(rt);

        if (last) {
#ifdef JS_THREADSAFE
            









            if (cx->thread->data.requestDepth == 0)
                JS_BeginRequest(cx);
#endif

            Shape::finishRuntimeState(cx);
            js_FinishRuntimeNumberState(cx);

            
            js_FinishCommonAtoms(cx);

            
            JS_ClearAllTraps(cx);
            JS_ClearAllWatchPoints(cx);
        }

#ifdef JS_THREADSAFE
        






        while (cx->outstandingRequests != 0)
            JS_EndRequest(cx);
#endif

        if (last) {
            js_GC(cx, GC_LAST_CONTEXT);
            DUMP_EVAL_CACHE_METER(cx);
            DUMP_FUNCTION_METER(cx);

            
            JS_LOCK_GC(rt);
            rt->state = JSRTS_DOWN;
            JS_NOTIFY_ALL_CONDVAR(rt->stateChange);
        } else {
            if (mode == JSDCM_FORCE_GC)
                js_GC(cx, GC_NORMAL);
            else if (mode == JSDCM_MAYBE_GC)
                JS_MaybeGC(cx);
            JS_LOCK_GC(rt);
            js_WaitForGC(rt);
        }
    }
#ifdef JS_THREADSAFE
#ifdef DEBUG
    JSThread *t = cx->thread;
#endif
    js_ClearContextThread(cx);
    JS_ASSERT_IF(JS_CLIST_IS_EMPTY(&t->contextList), !t->data.requestDepth);
#endif
#ifdef JS_METER_DST_OFFSET_CACHING
    cx->dstOffsetCache.dumpStats();
#endif
    JS_UNLOCK_GC(rt);
    FreeContext(cx);
}

static void
FreeContext(JSContext *cx)
{
#ifdef JS_THREADSAFE
    JS_ASSERT(!cx->thread);
#endif

    
    VOUCH_DOES_NOT_REQUIRE_STACK();
    JS_FinishArenaPool(&cx->tempPool);
    JS_FinishArenaPool(&cx->regExpPool);

    if (cx->lastMessage)
        js_free(cx->lastMessage);

    
    JSArgumentFormatMap *map = cx->argumentFormatMap;
    while (map) {
        JSArgumentFormatMap *temp = map;
        map = map->next;
        cx->free(temp);
    }

    
    if (cx->resolvingTable) {
        JS_DHashTableDestroy(cx->resolvingTable);
        cx->resolvingTable = NULL;
    }

    
    cx->~JSContext();
    js_free(cx);
}

JSContext *
js_ContextIterator(JSRuntime *rt, JSBool unlocked, JSContext **iterp)
{
    JSContext *cx = *iterp;

    Conditionally<AutoLockGC> lockIf(!!unlocked, rt);
    cx = js_ContextFromLinkField(cx ? cx->link.next : rt->contextList.next);
    if (&cx->link == &rt->contextList)
        cx = NULL;
    *iterp = cx;
    return cx;
}

JS_FRIEND_API(JSContext *)
js_NextActiveContext(JSRuntime *rt, JSContext *cx)
{
    JSContext *iter = cx;
#ifdef JS_THREADSAFE
    while ((cx = js_ContextIterator(rt, JS_FALSE, &iter)) != NULL) {
        if (cx->outstandingRequests && cx->thread->data.requestDepth)
            break;
    }
    return cx;
#else
    return js_ContextIterator(rt, JS_FALSE, &iter);
#endif
}

static JSDHashNumber
resolving_HashKey(JSDHashTable *table, const void *ptr)
{
    const JSResolvingKey *key = (const JSResolvingKey *)ptr;

    return (JSDHashNumber(uintptr_t(key->obj)) >> JS_GCTHING_ALIGN) ^ JSID_BITS(key->id);
}

static JSBool
resolving_MatchEntry(JSDHashTable *table,
                     const JSDHashEntryHdr *hdr,
                     const void *ptr)
{
    const JSResolvingEntry *entry = (const JSResolvingEntry *)hdr;
    const JSResolvingKey *key = (const JSResolvingKey *)ptr;

    return entry->key.obj == key->obj && entry->key.id == key->id;
}

static const JSDHashTableOps resolving_dhash_ops = {
    JS_DHashAllocTable,
    JS_DHashFreeTable,
    resolving_HashKey,
    resolving_MatchEntry,
    JS_DHashMoveEntryStub,
    JS_DHashClearEntryStub,
    JS_DHashFinalizeStub,
    NULL
};

JSBool
js_StartResolving(JSContext *cx, JSResolvingKey *key, uint32 flag,
                  JSResolvingEntry **entryp)
{
    JSDHashTable *table;
    JSResolvingEntry *entry;

    table = cx->resolvingTable;
    if (!table) {
        table = JS_NewDHashTable(&resolving_dhash_ops, NULL,
                                 sizeof(JSResolvingEntry),
                                 JS_DHASH_MIN_SIZE);
        if (!table)
            goto outofmem;
        cx->resolvingTable = table;
    }

    entry = (JSResolvingEntry *)
            JS_DHashTableOperate(table, key, JS_DHASH_ADD);
    if (!entry)
        goto outofmem;

    if (entry->flags & flag) {
        
        entry = NULL;
    } else {
        
        if (!entry->key.obj)
            entry->key = *key;
        entry->flags |= flag;
    }
    *entryp = entry;
    return JS_TRUE;

outofmem:
    JS_ReportOutOfMemory(cx);
    return JS_FALSE;
}

void
js_StopResolving(JSContext *cx, JSResolvingKey *key, uint32 flag,
                 JSResolvingEntry *entry, uint32 generation)
{
    JSDHashTable *table;

    




    table = cx->resolvingTable;
    if (!entry || table->generation != generation) {
        entry = (JSResolvingEntry *)
                JS_DHashTableOperate(table, key, JS_DHASH_LOOKUP);
    }
    JS_ASSERT(JS_DHASH_ENTRY_IS_BUSY(&entry->hdr));
    entry->flags &= ~flag;
    if (entry->flags)
        return;

    





    if (table->removedCount < JS_DHASH_TABLE_SIZE(table) >> 2)
        JS_DHashTableRawRemove(table, &entry->hdr);
    else
        JS_DHashTableOperate(table, key, JS_DHASH_REMOVE);
}

static void
ReportError(JSContext *cx, const char *message, JSErrorReport *reportp,
            JSErrorCallback callback, void *userRef)
{
    





    JS_ASSERT(reportp);
    if ((!callback || callback == js_GetErrorMessage) &&
        reportp->errorNumber == JSMSG_UNCAUGHT_EXCEPTION)
        reportp->flags |= JSREPORT_EXCEPTION;

    







    if (!JS_IsRunning(cx) ||
        !js_ErrorToException(cx, message, reportp, callback, userRef)) {
        js_ReportErrorAgain(cx, message, reportp);
    } else if (cx->debugHooks->debugErrorHook && cx->errorReporter) {
        JSDebugErrorHook hook = cx->debugHooks->debugErrorHook;
        
        if (hook)
            hook(cx, message, reportp, cx->debugHooks->debugErrorHookData);
    }
}


static void
PopulateReportBlame(JSContext *cx, JSErrorReport *report)
{
    



    for (JSStackFrame *fp = js_GetTopStackFrame(cx); fp; fp = fp->prev()) {
        if (fp->pc(cx)) {
            report->filename = fp->script()->filename;
            report->lineno = js_FramePCToLineNumber(cx, fp);
            break;
        }
    }
}








void
js_ReportOutOfMemory(JSContext *cx)
{
#ifdef JS_TRACER
    



    if (JS_ON_TRACE(cx) && !cx->bailExit)
        return;
#endif

    JSErrorReport report;
    JSErrorReporter onError = cx->errorReporter;

    
    const JSErrorFormatString *efs =
        js_GetLocalizedErrorMessage(cx, NULL, NULL, JSMSG_OUT_OF_MEMORY);
    const char *msg = efs ? efs->format : "Out of memory";

    
    PodZero(&report);
    report.flags = JSREPORT_ERROR;
    report.errorNumber = JSMSG_OUT_OF_MEMORY;
    PopulateReportBlame(cx, &report);

    





    cx->throwing = JS_FALSE;
    if (onError) {
        JSDebugErrorHook hook = cx->debugHooks->debugErrorHook;
        if (hook &&
            !hook(cx, msg, &report, cx->debugHooks->debugErrorHookData)) {
            onError = NULL;
        }
    }

    if (onError)
        onError(cx, msg, &report);
}

void
js_ReportOutOfScriptQuota(JSContext *cx)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                         JSMSG_SCRIPT_STACK_QUOTA);
}

JS_FRIEND_API(void)
js_ReportOverRecursed(JSContext *cx)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OVER_RECURSED);
}

void
js_ReportAllocationOverflow(JSContext *cx)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_ALLOC_OVERFLOW);
}







static bool
checkReportFlags(JSContext *cx, uintN *flags)
{
    if (JSREPORT_IS_STRICT_MODE_ERROR(*flags)) {
        




        JSStackFrame *fp = js_GetScriptedCaller(cx, NULL);
        if (fp && fp->script()->strictModeCode)
            *flags &= ~JSREPORT_WARNING;
        else if (JS_HAS_STRICT_OPTION(cx))
            *flags |= JSREPORT_WARNING;
        else
            return true;
    } else if (JSREPORT_IS_STRICT(*flags)) {
        
        if (!JS_HAS_STRICT_OPTION(cx))
            return true;
    }

    
    if (JSREPORT_IS_WARNING(*flags) && JS_HAS_WERROR_OPTION(cx))
        *flags &= ~JSREPORT_WARNING;

    return false;
}

JSBool
js_ReportErrorVA(JSContext *cx, uintN flags, const char *format, va_list ap)
{
    char *message;
    jschar *ucmessage;
    size_t messagelen;
    JSErrorReport report;
    JSBool warning;

    if (checkReportFlags(cx, &flags))
        return JS_TRUE;

    message = JS_vsmprintf(format, ap);
    if (!message)
        return JS_FALSE;
    messagelen = strlen(message);

    PodZero(&report);
    report.flags = flags;
    report.errorNumber = JSMSG_USER_DEFINED_ERROR;
    report.ucmessage = ucmessage = js_InflateString(cx, message, &messagelen);
    PopulateReportBlame(cx, &report);

    warning = JSREPORT_IS_WARNING(report.flags);

    ReportError(cx, message, &report, NULL, NULL);
    js_free(message);
    cx->free(ucmessage);
    return warning;
}












JSBool
js_ExpandErrorArguments(JSContext *cx, JSErrorCallback callback,
                        void *userRef, const uintN errorNumber,
                        char **messagep, JSErrorReport *reportp,
                        bool charArgs, va_list ap)
{
    const JSErrorFormatString *efs;
    int i;
    int argCount;

    *messagep = NULL;

    
    if (!callback || callback == js_GetErrorMessage)
        efs = js_GetLocalizedErrorMessage(cx, userRef, NULL, errorNumber);
    else
        efs = callback(userRef, NULL, errorNumber);
    if (efs) {
        size_t totalArgsLength = 0;
        size_t argLengths[10]; 
        argCount = efs->argCount;
        JS_ASSERT(argCount <= 10);
        if (argCount > 0) {
            





            reportp->messageArgs = (const jschar **)
                cx->malloc(sizeof(jschar *) * (argCount + 1));
            if (!reportp->messageArgs)
                return JS_FALSE;
            reportp->messageArgs[argCount] = NULL;
            for (i = 0; i < argCount; i++) {
                if (charArgs) {
                    char *charArg = va_arg(ap, char *);
                    size_t charArgLength = strlen(charArg);
                    reportp->messageArgs[i]
                        = js_InflateString(cx, charArg, &charArgLength);
                    if (!reportp->messageArgs[i])
                        goto error;
                } else {
                    reportp->messageArgs[i] = va_arg(ap, jschar *);
                }
                argLengths[i] = js_strlen(reportp->messageArgs[i]);
                totalArgsLength += argLengths[i];
            }
            
            reportp->messageArgs[i] = NULL;
        }
        



        if (argCount > 0) {
            if (efs->format) {
                jschar *buffer, *fmt, *out;
                int expandedArgs = 0;
                size_t expandedLength;
                size_t len = strlen(efs->format);

                buffer = fmt = js_InflateString (cx, efs->format, &len);
                if (!buffer)
                    goto error;
                expandedLength = len
                                 - (3 * argCount)       
                                 + totalArgsLength;

                



                reportp->ucmessage = out = (jschar *)
                    cx->malloc((expandedLength + 1) * sizeof(jschar));
                if (!out) {
                    cx->free(buffer);
                    goto error;
                }
                while (*fmt) {
                    if (*fmt == '{') {
                        if (isdigit(fmt[1])) {
                            int d = JS7_UNDEC(fmt[1]);
                            JS_ASSERT(d < argCount);
                            js_strncpy(out, reportp->messageArgs[d],
                                       argLengths[d]);
                            out += argLengths[d];
                            fmt += 3;
                            expandedArgs++;
                            continue;
                        }
                    }
                    *out++ = *fmt++;
                }
                JS_ASSERT(expandedArgs == argCount);
                *out = 0;
                cx->free(buffer);
                *messagep =
                    js_DeflateString(cx, reportp->ucmessage,
                                     (size_t)(out - reportp->ucmessage));
                if (!*messagep)
                    goto error;
            }
        } else {
            



            if (efs->format) {
                size_t len;
                *messagep = JS_strdup(cx, efs->format);
                if (!*messagep)
                    goto error;
                len = strlen(*messagep);
                reportp->ucmessage = js_InflateString(cx, *messagep, &len);
                if (!reportp->ucmessage)
                    goto error;
            }
        }
    }
    if (*messagep == NULL) {
        
        const char *defaultErrorMessage
            = "No error message available for error number %d";
        size_t nbytes = strlen(defaultErrorMessage) + 16;
        *messagep = (char *)cx->malloc(nbytes);
        if (!*messagep)
            goto error;
        JS_snprintf(*messagep, nbytes, defaultErrorMessage, errorNumber);
    }
    return JS_TRUE;

error:
    if (reportp->messageArgs) {
        
        if (charArgs) {
            i = 0;
            while (reportp->messageArgs[i])
                cx->free((void *)reportp->messageArgs[i++]);
        }
        cx->free((void *)reportp->messageArgs);
        reportp->messageArgs = NULL;
    }
    if (reportp->ucmessage) {
        cx->free((void *)reportp->ucmessage);
        reportp->ucmessage = NULL;
    }
    if (*messagep) {
        cx->free((void *)*messagep);
        *messagep = NULL;
    }
    return JS_FALSE;
}

JSBool
js_ReportErrorNumberVA(JSContext *cx, uintN flags, JSErrorCallback callback,
                       void *userRef, const uintN errorNumber,
                       JSBool charArgs, va_list ap)
{
    JSErrorReport report;
    char *message;
    JSBool warning;

    if (checkReportFlags(cx, &flags))
        return JS_TRUE;
    warning = JSREPORT_IS_WARNING(flags);

    PodZero(&report);
    report.flags = flags;
    report.errorNumber = errorNumber;
    PopulateReportBlame(cx, &report);

    if (!js_ExpandErrorArguments(cx, callback, userRef, errorNumber,
                                 &message, &report, !!charArgs, ap)) {
        return JS_FALSE;
    }

    ReportError(cx, message, &report, callback, userRef);

    if (message)
        cx->free(message);
    if (report.messageArgs) {
        



        if (charArgs) {
            int i = 0;
            while (report.messageArgs[i])
                cx->free((void *)report.messageArgs[i++]);
        }
        cx->free((void *)report.messageArgs);
    }
    if (report.ucmessage)
        cx->free((void *)report.ucmessage);

    return warning;
}

JS_FRIEND_API(void)
js_ReportErrorAgain(JSContext *cx, const char *message, JSErrorReport *reportp)
{
    JSErrorReporter onError;

    if (!message)
        return;

    if (cx->lastMessage)
        js_free(cx->lastMessage);
    cx->lastMessage = JS_strdup(cx, message);
    if (!cx->lastMessage)
        return;
    onError = cx->errorReporter;

    



    if (onError) {
        JSDebugErrorHook hook = cx->debugHooks->debugErrorHook;
        if (hook &&
            !hook(cx, cx->lastMessage, reportp,
                  cx->debugHooks->debugErrorHookData)) {
            onError = NULL;
        }
    }
    if (onError)
        onError(cx, cx->lastMessage, reportp);
}

void
js_ReportIsNotDefined(JSContext *cx, const char *name)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_DEFINED, name);
}

JSBool
js_ReportIsNullOrUndefined(JSContext *cx, intN spindex, const Value &v,
                           JSString *fallback)
{
    char *bytes;
    JSBool ok;

    bytes = DecompileValueGenerator(cx, spindex, v, fallback);
    if (!bytes)
        return JS_FALSE;

    if (strcmp(bytes, js_undefined_str) == 0 ||
        strcmp(bytes, js_null_str) == 0) {
        ok = JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR,
                                          js_GetErrorMessage, NULL,
                                          JSMSG_NO_PROPERTIES, bytes,
                                          NULL, NULL);
    } else if (v.isUndefined()) {
        ok = JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR,
                                          js_GetErrorMessage, NULL,
                                          JSMSG_UNEXPECTED_TYPE, bytes,
                                          js_undefined_str, NULL);
    } else {
        JS_ASSERT(v.isNull());
        ok = JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR,
                                          js_GetErrorMessage, NULL,
                                          JSMSG_UNEXPECTED_TYPE, bytes,
                                          js_null_str, NULL);
    }

    cx->free(bytes);
    return ok;
}

void
js_ReportMissingArg(JSContext *cx, const Value &v, uintN arg)
{
    char argbuf[11];
    char *bytes;
    JSAtom *atom;

    JS_snprintf(argbuf, sizeof argbuf, "%u", arg);
    bytes = NULL;
    if (IsFunctionObject(v)) {
        atom = GET_FUNCTION_PRIVATE(cx, &v.toObject())->atom;
        bytes = DecompileValueGenerator(cx, JSDVG_SEARCH_STACK,
                                        v, ATOM_TO_STRING(atom));
        if (!bytes)
            return;
    }
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                         JSMSG_MISSING_FUN_ARG, argbuf,
                         bytes ? bytes : "");
    cx->free(bytes);
}

JSBool
js_ReportValueErrorFlags(JSContext *cx, uintN flags, const uintN errorNumber,
                         intN spindex, const Value &v, JSString *fallback,
                         const char *arg1, const char *arg2)
{
    char *bytes;
    JSBool ok;

    JS_ASSERT(js_ErrorFormatString[errorNumber].argCount >= 1);
    JS_ASSERT(js_ErrorFormatString[errorNumber].argCount <= 3);
    bytes = DecompileValueGenerator(cx, spindex, v, fallback);
    if (!bytes)
        return JS_FALSE;

    ok = JS_ReportErrorFlagsAndNumber(cx, flags, js_GetErrorMessage,
                                      NULL, errorNumber, bytes, arg1, arg2);
    cx->free(bytes);
    return ok;
}

#if defined DEBUG && defined XP_UNIX

void js_logon(JSContext *cx)  { cx->logfp = stderr; cx->logPrevPc = NULL; }
void js_logoff(JSContext *cx) { cx->logfp = NULL; }
#endif

JSErrorFormatString js_ErrorFormatString[JSErr_Limit] = {
#define MSG_DEF(name, number, count, exception, format) \
    { format, count, exception } ,
#include "js.msg"
#undef MSG_DEF
};

JS_FRIEND_API(const JSErrorFormatString *)
js_GetErrorMessage(void *userRef, const char *locale, const uintN errorNumber)
{
    if ((errorNumber > 0) && (errorNumber < JSErr_Limit))
        return &js_ErrorFormatString[errorNumber];
    return NULL;
}

JSBool
js_InvokeOperationCallback(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;
    JSThreadData *td = JS_THREAD_DATA(cx);

    JS_ASSERT_REQUEST_DEPTH(cx);
    JS_ASSERT(td->interruptFlags != 0);

    




    JS_LOCK_GC(rt);
    td->interruptFlags = 0;
#ifdef JS_THREADSAFE
    JS_ATOMIC_DECREMENT(&rt->interruptCounter);
#endif
    JS_UNLOCK_GC(rt);

    






    if (rt->gcIsNeeded) {
        js_GC(cx, GC_NORMAL);

        



        bool delayedOutOfMemory;
        JS_LOCK_GC(rt);
        delayedOutOfMemory = (rt->gcBytes > rt->gcMaxBytes);
        JS_UNLOCK_GC(rt);
        if (delayedOutOfMemory) {
            js_ReportOutOfMemory(cx);
            return false;
        }
    }
#ifdef JS_THREADSAFE
    else {
        JS_YieldRequest(cx);
    }
#endif

    JSOperationCallback cb = cx->operationCallback;

    





    return !cb || cb(cx);
}

JSBool
js_HandleExecutionInterrupt(JSContext *cx)
{
    JSBool result = JS_TRUE;
    if (JS_THREAD_DATA(cx)->interruptFlags)
        result = js_InvokeOperationCallback(cx) && result;
    return result;
}

namespace js {

void
TriggerOperationCallback(JSContext *cx)
{
    





    JSThreadData *td;
#ifdef JS_THREADSAFE
    JSThread *thread = cx->thread;
    if (!thread)
        return;
    td = &thread->data;
#else
    td = JS_THREAD_DATA(cx);
#endif
    td->triggerOperationCallback(cx->runtime);
}

void
TriggerAllOperationCallbacks(JSRuntime *rt)
{
    for (ThreadDataIter i(rt); !i.empty(); i.popFront())
        i.threadData()->triggerOperationCallback(rt);
}

} 

JSStackFrame *
js_GetScriptedCaller(JSContext *cx, JSStackFrame *fp)
{
    if (!fp)
        fp = js_GetTopStackFrame(cx);
    while (fp && fp->isDummyFrame())
        fp = fp->prev();
    JS_ASSERT_IF(fp, fp->isScriptFrame());
    return fp;
}

jsbytecode*
js_GetCurrentBytecodePC(JSContext* cx)
{
    jsbytecode *pc, *imacpc;

#ifdef JS_TRACER
    if (JS_ON_TRACE(cx)) {
        pc = cx->bailExit->pc;
        imacpc = cx->bailExit->imacpc;
    } else
#endif
    {
        JS_ASSERT_NOT_ON_TRACE(cx);  
        pc = cx->regs ? cx->regs->pc : NULL;
        if (!pc)
            return NULL;
        imacpc = cx->fp()->maybeImacropc();
    }

    




    return (*pc == JSOP_CALL && imacpc) ? imacpc : pc;
}

bool
js_CurrentPCIsInImacro(JSContext *cx)
{
#ifdef JS_TRACER
    VOUCH_DOES_NOT_REQUIRE_STACK();
    if (JS_ON_TRACE(cx))
        return cx->bailExit->imacpc != NULL;
    return cx->fp()->hasImacropc();
#else
    return false;
#endif
}

void
DSTOffsetCache::purge()
{
    




    offsetMilliseconds = 0;
    rangeStartSeconds = rangeEndSeconds = INT64_MIN;
    oldOffsetMilliseconds = 0;
    oldRangeStartSeconds = oldRangeEndSeconds = INT64_MIN;

#ifdef JS_METER_DST_OFFSET_CACHING
    totalCalculations = 0;
    hit = 0;
    missIncreasing = missDecreasing = 0;
    missIncreasingOffsetChangeExpand = missIncreasingOffsetChangeUpper = 0;
    missDecreasingOffsetChangeExpand = missDecreasingOffsetChangeLower = 0;
    missLargeIncrease = missLargeDecrease = 0;
#endif

    sanityCheck();
}







DSTOffsetCache::DSTOffsetCache()
{
    purge();
}

JSContext::JSContext(JSRuntime *rt)
  : runtime(rt),
    compartment(rt->defaultCompartment),
    regs(NULL),
    busyArrays(thisInInitializer())
{}

void
JSContext::resetCompartment()
{
    JSObject *scopeobj;
    if (hasfp()) {
        scopeobj = &fp()->scopeChain();
    } else {
        scopeobj = globalObject;
        if (!scopeobj) {
            compartment = runtime->defaultCompartment;
            return;
        }

        



        OBJ_TO_INNER_OBJECT(this, scopeobj);
        if (!scopeobj) {
            



            JS_ASSERT(0);
            compartment = NULL;
            return;
        }
    }
    compartment = scopeobj->getCompartment();
}

void
JSContext::pushSegmentAndFrame(js::StackSegment *newseg, JSFrameRegs &newregs)
{
    JS_ASSERT(regs != &newregs);
    if (hasActiveSegment())
        currentSegment->suspend(regs);
    newseg->setPreviousInContext(currentSegment);
    currentSegment = newseg;
    setCurrentRegs(&newregs);
    newseg->joinContext(this, newregs.fp);
}

void
JSContext::popSegmentAndFrame()
{
    JS_ASSERT(currentSegment->maybeContext() == this);
    JS_ASSERT(currentSegment->getInitialFrame() == regs->fp);
    currentSegment->leaveContext();
    currentSegment = currentSegment->getPreviousInContext();
    if (currentSegment) {
        if (currentSegment->isSaved()) {
            setCurrentRegs(NULL);
        } else {
            setCurrentRegs(currentSegment->getSuspendedRegs());
            currentSegment->resume();
        }
    } else {
        JS_ASSERT(regs->fp->prev() == NULL);
        setCurrentRegs(NULL);
    }
}

void
JSContext::saveActiveSegment()
{
    JS_ASSERT(hasActiveSegment());
    currentSegment->save(regs);
    setCurrentRegs(NULL);
}

void
JSContext::restoreSegment()
{
    js::StackSegment *ccs = currentSegment;
    setCurrentRegs(ccs->getSuspendedRegs());
    ccs->restore();
}

JSGenerator *
JSContext::generatorFor(JSStackFrame *fp) const
{
    JS_ASSERT(stack().contains(fp) && fp->isGeneratorFrame());
    JS_ASSERT(!fp->isFloatingGenerator());
    JS_ASSERT(!genStack.empty());

    if (JS_LIKELY(fp == genStack.back()->liveFrame()))
        return genStack.back();

    
    for (size_t i = 0; i < genStack.length(); ++i) {
        if (genStack[i]->liveFrame() == fp)
            return genStack[i];
    }
    JS_NOT_REACHED("no matching generator");
    return NULL;
}

StackSegment *
JSContext::containingSegment(const JSStackFrame *target)
{
    
    StackSegment *seg = currentSegment;
    if (!seg)
        return NULL;

    
    if (regs) {
        JS_ASSERT(regs->fp);
        JS_ASSERT(activeSegment() == seg);
        JSStackFrame *f = regs->fp;
        JSStackFrame *stop = seg->getInitialFrame()->prev();
        for (; f != stop; f = f->prev()) {
            if (f == target)
                return seg;
        }
        seg = seg->getPreviousInContext();
    }

    
    for (; seg; seg = seg->getPreviousInContext()) {
        JSStackFrame *f = seg->getSuspendedFrame();
        JSStackFrame *stop = seg->getInitialFrame()->prev();
        for (; f != stop; f = f->prev()) {
            if (f == target)
                return seg;
        }
    }

    return NULL;
}

JS_FRIEND_API(void)
JSRuntime::onTooMuchMalloc()
{
#ifdef JS_THREADSAFE
    AutoLockGC lock(this);

    



    js_WaitForGC(this);
#endif
    TriggerGC(this);
}

JS_FRIEND_API(void *)
JSRuntime::onOutOfMemory(void *p, size_t nbytes, JSContext *cx)
{
#ifdef JS_THREADSAFE
    gcHelperThread.waitBackgroundSweepEnd(this);
    if (!p)
        p = ::js_malloc(nbytes);
    else if (p == reinterpret_cast<void *>(1))
        p = ::js_calloc(nbytes);
    else
      p = ::js_realloc(p, nbytes);
    if (p)
        return p;
#endif
    if (cx)
        js_ReportOutOfMemory(cx);
    return NULL;
}





inline void
FreeOldArenas(JSRuntime *rt, JSArenaPool *pool)
{
    JSArena *a = pool->current;
    if (a == pool->first.next && a->avail == a->base + sizeof(int64)) {
        int64 age = JS_Now() - *(int64 *) a->base;
        if (age > int64(rt->gcEmptyArenaPoolLifespan) * 1000)
            JS_FreeArenaPool(pool);
    }
}

void
JSContext::purge()
{
    FreeOldArenas(runtime, &regExpPool);
}

static bool
ComputeIsJITBroken()
{
#ifndef ANDROID
    return false;
#else  
    if (getenv("JS_IGNORE_JIT_BROKENNESS")) {
        return false;
    }

    std::string line;

    
    std::ifstream osrelease("/proc/sys/kernel/osrelease");
    std::getline(osrelease, line);
    if (line.npos == line.find("2.6.29")) {
        
        return false;
    }

    
    bool broken = false;
    std::ifstream cpuinfo("/proc/cpuinfo");
    do {
        if (0 == line.find("Hardware")) {
            const char* blacklist[] = {
                "SGH-T959",     
                "SGH-I897",     
                "SCH-I500",     
                "SPH-D700",     
                "GT-I9000",     
                NULL
            };
            for (const char** hw = &blacklist[0]; *hw; ++hw) {
                if (line.npos != line.find(*hw)) {
                    broken = true;
                    break;
                }
            }
            break;
        }
        std::getline(cpuinfo, line);
    } while(!cpuinfo.fail() && !cpuinfo.eof());
    return broken;
#endif  
}

static bool
IsJITBrokenHere()
{
    static bool computedIsBroken = false;
    static bool isBroken = false;
    if (!computedIsBroken) {
        isBroken = ComputeIsJITBroken();
        computedIsBroken = true;
    }
    return isBroken;
}

void
JSContext::updateJITEnabled()
{
#ifdef JS_TRACER
    traceJitEnabled = ((options & JSOPTION_JIT) &&
                       !IsJITBrokenHere() &&
                       (debugHooks == &js_NullDebugHooks ||
                        (debugHooks == &runtime->globalDebugHooks &&
                         !runtime->debuggerInhibitsJIT())));
#endif
#ifdef JS_METHODJIT
    methodJitEnabled = (options & JSOPTION_METHODJIT) &&
                       !IsJITBrokenHere()
# if defined JS_CPU_X86 || defined JS_CPU_X64
                       && JSC::MacroAssemblerX86Common::getSSEState() >=
                          JSC::MacroAssemblerX86Common::HasSSE2
# endif
                        ;
#ifdef JS_TRACER
    profilingEnabled = (options & JSOPTION_PROFILING) && traceJitEnabled;
#endif
#endif
}

namespace js {

void
SetPendingException(JSContext *cx, const Value &v)
{
    cx->throwing = JS_TRUE;
    cx->exception = v;
}

} 
