






#include "jsapi.h"
#include "jscntxt.h"
#include "jsgc.h"
#include "jsprf.h"
#include "jsutil.h"
#include "jswatchpoint.h"

#include "mozilla/Util.h"

#include "js/HashTable.h"
#include "gc/GCInternals.h"

#include "jsobjinlines.h"
#include "jsgcinlines.h"

#ifdef MOZ_VALGRIND
# include <valgrind/memcheck.h>
#endif

using namespace js;
using namespace js::gc;
using namespace mozilla;

#if defined(DEBUG) && defined(JS_GC_ZEAL) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)

JS_ALWAYS_INLINE bool
CheckStackRootThing(uintptr_t *w, void *address, ThingRootKind kind)
{
    if (kind != THING_ROOT_BINDINGS)
        return address == static_cast<void*>(w);

    Bindings *bp = static_cast<Bindings*>(address);
    return w >= (uintptr_t*)bp && w < (uintptr_t*)(bp + 1);
}

struct Rooter {
    Rooted<void*> *rooter;
    ThingRootKind kind;
};

static void
CheckStackRoot(JSRuntime *rt, uintptr_t *w, Rooter *begin, Rooter *end)
{
    
#ifdef MOZ_VALGRIND
    VALGRIND_MAKE_MEM_DEFINED(&w, sizeof(w));
#endif

    void *thing = GetAddressableGCThing(rt, *w);
    if (!thing)
        return;

    
    if (static_cast<Cell *>(thing)->zone() == rt->atomsCompartment->zone())
        return;

    





    for (Rooter *p = begin; p != end; p++) {
        if (CheckStackRootThing(w, p->rooter->address(), p->kind))
            return;
    }

    SkipRoot *skip = TlsPerThreadData.get()->skipGCRooters;
    while (skip) {
        if (skip->contains(reinterpret_cast<uint8_t*>(w), sizeof(w)))
            return;
        skip = skip->previous();
    }
    for (ContextIter cx(rt); !cx.done(); cx.next()) {
        skip = cx->skipGCRooters;
        while (skip) {
            if (skip->contains(reinterpret_cast<uint8_t*>(w), sizeof(w)))
                return;
            skip = skip->previous();
        }
    }

    





    JS::PoisonPtr(w);
}

static void
CheckStackRootsRange(JSRuntime *rt, uintptr_t *begin, uintptr_t *end, Rooter *rbegin, Rooter *rend)
{
    JS_ASSERT(begin <= end);
    for (uintptr_t *i = begin; i != end; ++i)
        CheckStackRoot(rt, i, rbegin, rend);
}

static void
CheckStackRootsRangeAndSkipIon(JSRuntime *rt, uintptr_t *begin, uintptr_t *end, Rooter *rbegin, Rooter *rend)
{
    




    uintptr_t *i = begin;

#if JS_STACK_GROWTH_DIRECTION < 0 && defined(JS_ION)
    for (ion::IonActivationIterator ion(rt); ion.more(); ++ion) {
        uintptr_t *ionMin, *ionEnd;
        ion.ionStackRange(ionMin, ionEnd);

        uintptr_t *upto = Min(ionMin, end);
        if (upto > i)
            CheckStackRootsRange(rt, i, upto, rbegin, rend);
        else
            break;
        i = ionEnd;
    }
#endif

    
    if (i < end)
        CheckStackRootsRange(rt, i, end, rbegin, rend);
}

static int
CompareRooters(const void *vpA, const void *vpB)
{
    const Rooter *a = static_cast<const Rooter *>(vpA);
    const Rooter *b = static_cast<const Rooter *>(vpB);
    
    return (a->rooter < b->rooter) ? -1 : 1;
}











static bool
SuppressCheckRoots(Vector<Rooter, 0, SystemAllocPolicy> &rooters)
{
    static const unsigned int NumStackMemories = 6;
    static const size_t StackCheckDepth = 10;

    static uint32_t stacks[NumStackMemories];
    static unsigned int numMemories = 0;
    static unsigned int oldestMemory = 0;

    
    
    
    
    qsort(rooters.begin(), rooters.length(), sizeof(Rooter), CompareRooters);

    
    
    unsigned int pos;

    
    uint32_t hash = HashGeneric(&pos);
    for (unsigned int i = 0; i < Min(StackCheckDepth, rooters.length()); i++)
        hash = AddToHash(hash, rooters[rooters.length() - i - 1].rooter);

    
    for (pos = 0; pos < numMemories; pos++) {
        if (stacks[pos] == hash) {
            
            
            
            return true;
        }
    }

    
    stacks[oldestMemory] = hash;
    oldestMemory = (oldestMemory + 1) % NumStackMemories;
    if (numMemories < NumStackMemories)
        numMemories++;

    return false;
}

static void
GatherRooters(Vector<Rooter, 0, SystemAllocPolicy> &rooters,
              Rooted<void*> **thingGCRooters,
              unsigned thingRootKind)
{
    Rooted<void*> *rooter = thingGCRooters[thingRootKind];
    while (rooter) {
        Rooter r = { rooter, ThingRootKind(thingRootKind) };
        JS_ALWAYS_TRUE(rooters.append(r));
        rooter = rooter->previous();
    }
}

void
JS::CheckStackRoots(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    if (rt->gcZeal_ != ZealStackRootingValue)
        return;

    
    if (cx->compartment->activeAnalysis)
        return;

    if (rt->mainThread.suppressGC)
        return;

    
    if (IsAtomsCompartment(cx->compartment)) {
        for (CompartmentsIter c(rt); !c.done(); c.next()) {
            if (c.get()->activeAnalysis)
                return;
        }
    }

    AutoCopyFreeListToArenas copy(rt);

    ConservativeGCData *cgcd = &rt->conservativeGC;
    cgcd->recordStackTop();

    JS_ASSERT(cgcd->hasStackToScan());
    uintptr_t *stackMin, *stackEnd;
#if JS_STACK_GROWTH_DIRECTION > 0
    stackMin = rt->nativeStackBase;
    stackEnd = cgcd->nativeStackTop;
#else
    stackMin = cgcd->nativeStackTop + 1;
    stackEnd = reinterpret_cast<uintptr_t *>(rt->nativeStackBase);
#endif

    
    Vector<Rooter, 0, SystemAllocPolicy> rooters;
    for (unsigned i = 0; i < THING_ROOT_LIMIT; i++) {
        for (ContextIter cx(rt); !cx.done(); cx.next()) {
            GatherRooters(rooters, cx->thingGCRooters, i);
        }

        GatherRooters(rooters, rt->mainThread.thingGCRooters, i);
    }

    if (SuppressCheckRoots(rooters))
        return;

    
    
    
    void *firstScanned = NULL;
    for (Rooter *p = rooters.begin(); p != rooters.end(); p++) {
        if (p->rooter->scanned) {
            uintptr_t *addr = reinterpret_cast<uintptr_t*>(p->rooter);
#if JS_STACK_GROWTH_DIRECTION < 0
            if (stackEnd > addr)
#else
            if (stackEnd < addr)
#endif
            {
                stackEnd = addr;
                firstScanned = p->rooter;
            }
        }
    }

    
    
    Rooter *firstToScan = rooters.begin();
    if (firstScanned) {
        for (Rooter *p = rooters.begin(); p != rooters.end(); p++) {
#if JS_STACK_GROWTH_DIRECTION < 0
            if (p->rooter >= firstScanned)
#else
            if (p->rooter <= firstScanned)
#endif
            {
                Swap(*firstToScan, *p);
                ++firstToScan;
            }
        }
    }

    JS_ASSERT(stackMin <= stackEnd);
    CheckStackRootsRangeAndSkipIon(rt, stackMin, stackEnd, firstToScan, rooters.end());
    CheckStackRootsRange(rt, cgcd->registerSnapshot.words,
                         ArrayEnd(cgcd->registerSnapshot.words),
                         firstToScan, rooters.end());

    
    for (Rooter *p = rooters.begin(); p != rooters.end(); p++)
        p->rooter->scanned = true;
}

#endif 

#ifdef JS_GC_ZEAL

static void
DisableGGCForVerification(JSRuntime *rt)
{
#ifdef JSGC_GENERATIONAL
    if (rt->gcVerifyPreData || rt->gcVerifyPostData)
        return;

    if (rt->gcStoreBuffer.isEnabled())
        rt->gcStoreBuffer.disable();
#endif
}

static void
EnableGGCAfterVerification(JSRuntime *rt)
{
#ifdef JSGC_GENERATIONAL
    if (rt->gcVerifyPreData || rt->gcVerifyPostData)
        return;

    if (rt->gcGenerationalEnabled)
        rt->gcStoreBuffer.enable();
#endif
}






























struct EdgeValue
{
    void *thing;
    JSGCTraceKind kind;
    char *label;
};

struct VerifyNode
{
    void *thing;
    JSGCTraceKind kind;
    uint32_t count;
    EdgeValue edges[1];
};

typedef HashMap<void *, VerifyNode *, DefaultHasher<void *>, SystemAllocPolicy> NodeMap;














struct VerifyPreTracer : JSTracer {
    
    uint64_t number;

    
    int count;

    
    VerifyNode *curnode;
    VerifyNode *root;
    char *edgeptr;
    char *term;
    NodeMap nodemap;

    VerifyPreTracer() : root(NULL) {}
    ~VerifyPreTracer() { js_free(root); }
};





static void
AccumulateEdge(JSTracer *jstrc, void **thingp, JSGCTraceKind kind)
{
    VerifyPreTracer *trc = (VerifyPreTracer *)jstrc;

    trc->edgeptr += sizeof(EdgeValue);
    if (trc->edgeptr >= trc->term) {
        trc->edgeptr = trc->term;
        return;
    }

    VerifyNode *node = trc->curnode;
    uint32_t i = node->count;

    node->edges[i].thing = *thingp;
    node->edges[i].kind = kind;
    node->edges[i].label = trc->debugPrinter ? NULL : (char *)trc->debugPrintArg;
    node->count++;
}

static VerifyNode *
MakeNode(VerifyPreTracer *trc, void *thing, JSGCTraceKind kind)
{
    NodeMap::AddPtr p = trc->nodemap.lookupForAdd(thing);
    if (!p) {
        VerifyNode *node = (VerifyNode *)trc->edgeptr;
        trc->edgeptr += sizeof(VerifyNode) - sizeof(EdgeValue);
        if (trc->edgeptr >= trc->term) {
            trc->edgeptr = trc->term;
            return NULL;
        }

        node->thing = thing;
        node->count = 0;
        node->kind = kind;
        trc->nodemap.add(p, thing, node);
        return node;
    }
    return NULL;
}

static VerifyNode *
NextNode(VerifyNode *node)
{
    if (node->count == 0)
        return (VerifyNode *)((char *)node + sizeof(VerifyNode) - sizeof(EdgeValue));
    else
        return (VerifyNode *)((char *)node + sizeof(VerifyNode) +
                             sizeof(EdgeValue)*(node->count - 1));
}

void
gc::StartVerifyPreBarriers(JSRuntime *rt)
{
    if (rt->gcVerifyPreData ||
        rt->gcIncrementalState != NO_INCREMENTAL ||
        !IsIncrementalGCSafe(rt))
    {
        return;
    }

    DisableGGCForVerification(rt);

    AutoPrepareForTracing prep(rt);

    for (GCChunkSet::Range r(rt->gcChunkSet.all()); !r.empty(); r.popFront())
        r.front()->bitmap.clear();

    VerifyPreTracer *trc = js_new<VerifyPreTracer>();

    rt->gcNumber++;
    trc->number = rt->gcNumber;
    trc->count = 0;

    JS_TracerInit(trc, rt, AccumulateEdge);

    const size_t size = 64 * 1024 * 1024;
    trc->root = (VerifyNode *)js_malloc(size);
    JS_ASSERT(trc->root);
    trc->edgeptr = (char *)trc->root;
    trc->term = trc->edgeptr + size;

    if (!trc->nodemap.init())
        return;

    
    trc->curnode = MakeNode(trc, NULL, JSGCTraceKind(0));

    
    rt->gcIncrementalState = MARK_ROOTS;

    
    MarkRuntime(trc);

    VerifyNode *node = trc->curnode;
    if (trc->edgeptr == trc->term)
        goto oom;

    
    while ((char *)node < trc->edgeptr) {
        for (uint32_t i = 0; i < node->count; i++) {
            EdgeValue &e = node->edges[i];
            VerifyNode *child = MakeNode(trc, e.thing, e.kind);
            if (child) {
                trc->curnode = child;
                JS_TraceChildren(trc, e.thing, e.kind);
            }
            if (trc->edgeptr == trc->term)
                goto oom;
        }

        node = NextNode(node);
    }

    rt->gcVerifyPreData = trc;
    rt->gcIncrementalState = MARK;
    rt->gcMarker.start();

    for (ZonesIter zone(rt); !zone.done(); zone.next()) {
        PurgeJITCaches(zone);
        zone->setNeedsBarrier(true, Zone::UpdateIon);
        zone->allocator.arenas.purge();
    }

    return;

oom:
    rt->gcIncrementalState = NO_INCREMENTAL;
    js_delete(trc);
    rt->gcVerifyPreData = NULL;
    EnableGGCAfterVerification(rt);
}

static bool
IsMarkedOrAllocated(Cell *cell)
{
    return cell->isMarked() || cell->arenaHeader()->allocatedDuringIncremental;
}

const static uint32_t MAX_VERIFIER_EDGES = 1000;








static void
CheckEdge(JSTracer *jstrc, void **thingp, JSGCTraceKind kind)
{
    VerifyPreTracer *trc = (VerifyPreTracer *)jstrc;
    VerifyNode *node = trc->curnode;

    
    if (node->count > MAX_VERIFIER_EDGES)
        return;

    for (uint32_t i = 0; i < node->count; i++) {
        if (node->edges[i].thing == *thingp) {
            JS_ASSERT(node->edges[i].kind == kind);
            node->edges[i].thing = NULL;
            return;
        }
    }
}

static void
AssertMarkedOrAllocated(const EdgeValue &edge)
{
    if (!edge.thing || IsMarkedOrAllocated(static_cast<Cell *>(edge.thing)))
        return;

    char msgbuf[1024];
    const char *label = edge.label ? edge.label : "<unknown>";

    JS_snprintf(msgbuf, sizeof(msgbuf), "[barrier verifier] Unmarked edge: %s", label);
    MOZ_ReportAssertionFailure(msgbuf, __FILE__, __LINE__);
    MOZ_CRASH();
}

void
gc::EndVerifyPreBarriers(JSRuntime *rt)
{
    AutoPrepareForTracing prep(rt);

    VerifyPreTracer *trc = (VerifyPreTracer *)rt->gcVerifyPreData;

    if (!trc)
        return;

    bool compartmentCreated = false;

    
    for (ZonesIter zone(rt); !zone.done(); zone.next()) {
        if (!zone->needsBarrier())
            compartmentCreated = true;

        zone->setNeedsBarrier(false, Zone::UpdateIon);
        PurgeJITCaches(zone);
    }

    



    JS_ASSERT(trc->number == rt->gcNumber);
    rt->gcNumber++;

    rt->gcVerifyPreData = NULL;
    rt->gcIncrementalState = NO_INCREMENTAL;

    if (!compartmentCreated && IsIncrementalGCSafe(rt)) {
        JS_TracerInit(trc, rt, CheckEdge);

        
        VerifyNode *node = NextNode(trc->root);
        while ((char *)node < trc->edgeptr) {
            trc->curnode = node;
            JS_TraceChildren(trc, node->thing, node->kind);

            if (node->count <= MAX_VERIFIER_EDGES) {
                for (uint32_t i = 0; i < node->count; i++)
                    AssertMarkedOrAllocated(node->edges[i]);
            }

            node = NextNode(node);
        }
    }

    rt->gcMarker.reset();
    rt->gcMarker.stop();

    js_delete(trc);

    EnableGGCAfterVerification(rt);
}



struct VerifyPostTracer : JSTracer {
    
    uint64_t number;

    
    int count;
};






void
gc::StartVerifyPostBarriers(JSRuntime *rt)
{
#ifdef JSGC_GENERATIONAL
    if (rt->gcVerifyPostData ||
        rt->gcIncrementalState != NO_INCREMENTAL)
    {
        return;
    }

    DisableGGCForVerification(rt);

    VerifyPostTracer *trc = js_new<VerifyPostTracer>();
    rt->gcVerifyPostData = trc;
    rt->gcNumber++;
    trc->number = rt->gcNumber;
    trc->count = 0;

    if (!rt->gcVerifierNursery.enable())
        goto oom;

    if (!rt->gcStoreBuffer.enable())
        goto oom;

    return;
oom:
    js_delete(trc);
    rt->gcVerifyPostData = NULL;
    EnableGGCAfterVerification(rt);
#endif
}

#ifdef JSGC_GENERATIONAL
static void
AssertStoreBufferContainsEdge(StoreBuffer *storebuf, void *loc, void *dst)
{
    if (storebuf->containsEdgeAt(loc))
        return;

    char msgbuf[1024];
    JS_snprintf(msgbuf, sizeof(msgbuf), "[post-barrier verifier] Missing edge @ %p to %p",
                loc, dst);
    MOZ_ReportAssertionFailure(msgbuf, __FILE__, __LINE__);
    MOZ_CRASH();
}

void
PostVerifierVisitEdge(JSTracer *jstrc, void **thingp, JSGCTraceKind kind)
{
    VerifyPostTracer *trc = (VerifyPostTracer *)jstrc;
    Cell *dst = (Cell *)*thingp;
    JSRuntime *rt = dst->runtime();

    
    if (!rt->gcVerifierNursery.isInside(dst))
        return;

    




    Cell *loc = (Cell *)(trc->realLocation != NULL ? trc->realLocation : thingp);

    AssertStoreBufferContainsEdge(&rt->gcStoreBuffer, loc, dst);
}
#endif

void
js::gc::EndVerifyPostBarriers(JSRuntime *rt)
{
#ifdef JSGC_GENERATIONAL
    AutoPrepareForTracing prep(rt);

    VerifyPostTracer *trc = (VerifyPostTracer *)rt->gcVerifyPostData;
    JS_TracerInit(trc, rt, PostVerifierVisitEdge);
    trc->count = 0;

    if (rt->gcStoreBuffer.hasOverflowed())
        goto oom;

    if (!rt->gcStoreBuffer.coalesceForVerification())
        goto oom;

    
    for (CompartmentsIter comp(rt); !comp.done(); comp.next()) {
        if (comp->watchpointMap)
            comp->watchpointMap->markAll(trc);
    }
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        for (size_t kind = 0; kind < FINALIZE_LIMIT; ++kind) {
            for (CellIterUnderGC cells(zone, AllocKind(kind)); !cells.done(); cells.next()) {
                Cell *src = cells.getCell();
                if (!rt->gcVerifierNursery.isInside(src))
                    JS_TraceChildren(trc, src, MapAllocToTraceKind(AllocKind(kind)));
            }
        }
    }

oom:
    js_delete(trc);
    rt->gcVerifyPostData = NULL;
    rt->gcVerifierNursery.disable();
    rt->gcStoreBuffer.disable();
    rt->gcStoreBuffer.releaseVerificationData();
    EnableGGCAfterVerification(rt);
#endif
}



static void
VerifyPreBarriers(JSRuntime *rt)
{
    if (rt->gcVerifyPreData)
        EndVerifyPreBarriers(rt);
    else
        StartVerifyPreBarriers(rt);
}

static void
VerifyPostBarriers(JSRuntime *rt)
{
    if (rt->gcVerifyPostData)
        EndVerifyPostBarriers(rt);
    else
        StartVerifyPostBarriers(rt);
}

void
gc::VerifyBarriers(JSRuntime *rt, VerifierType type)
{
    if (type == PreBarrierVerifier)
        VerifyPreBarriers(rt);
    else
        VerifyPostBarriers(rt);
}

static void
MaybeVerifyPreBarriers(JSRuntime *rt, bool always)
{
    if (rt->gcZeal() != ZealVerifierPreValue)
        return;

    if (rt->mainThread.suppressGC)
        return;

    if (VerifyPreTracer *trc = (VerifyPreTracer *)rt->gcVerifyPreData) {
        if (++trc->count < rt->gcZealFrequency && !always)
            return;

        EndVerifyPreBarriers(rt);
    }
    StartVerifyPreBarriers(rt);
}

static void
MaybeVerifyPostBarriers(JSRuntime *rt, bool always)
{
    if (rt->gcZeal() != ZealVerifierPostValue)
        return;

    if (rt->mainThread.suppressGC)
        return;

    if (VerifyPostTracer *trc = (VerifyPostTracer *)rt->gcVerifyPostData) {
        if (++trc->count < rt->gcZealFrequency && !always)
            return;

        EndVerifyPostBarriers(rt);
    }
    StartVerifyPostBarriers(rt);
}

void
js::gc::MaybeVerifyBarriers(JSContext *cx, bool always)
{
    MaybeVerifyPreBarriers(cx->runtime, always);
    MaybeVerifyPostBarriers(cx->runtime, always);
}

void
js::gc::FinishVerifier(JSRuntime *rt)
{
    if (VerifyPreTracer *trc = (VerifyPreTracer *)rt->gcVerifyPreData) {
        js_delete(trc);
        rt->gcVerifyPreData = NULL;
    }
#ifdef JSGC_GENERATIONAL
    if (VerifyPostTracer *trc = (VerifyPostTracer *)rt->gcVerifyPostData) {
        js_delete(trc);
        rt->gcVerifierNursery.disable();
        rt->gcStoreBuffer.disable();
        rt->gcStoreBuffer.releaseVerificationData();
        rt->gcVerifyPostData = NULL;
    }
#endif
    EnableGGCAfterVerification(rt);
}

#endif 
