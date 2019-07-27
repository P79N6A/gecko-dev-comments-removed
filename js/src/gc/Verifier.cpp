





#ifdef MOZ_VALGRIND
# include <valgrind/memcheck.h>
#endif

#include "jscntxt.h"
#include "jsgc.h"
#include "jsprf.h"

#include "gc/GCInternals.h"
#include "gc/Zone.h"
#include "js/GCAPI.h"
#include "js/HashTable.h"

#include "jscntxtinlines.h"
#include "jsgcinlines.h"

using namespace js;
using namespace js::gc;

#ifdef JS_GC_ZEAL






























struct EdgeValue
{
    void *thing;
    JSGCTraceKind kind;
    const char *label;
};

struct VerifyNode
{
    void *thing;
    JSGCTraceKind kind;
    uint32_t count;
    EdgeValue edges[1];
};

typedef HashMap<void *, VerifyNode *, DefaultHasher<void *>, SystemAllocPolicy> NodeMap;














struct VerifyPreTracer : JSTracer
{
    JS::AutoDisableGenerationalGC noggc;

    
    uint64_t number;

    
    int count;

    
    VerifyNode *curnode;
    VerifyNode *root;
    char *edgeptr;
    char *term;
    NodeMap nodemap;

    VerifyPreTracer(JSRuntime *rt, JSTraceCallback callback)
      : JSTracer(rt, callback), noggc(rt), number(rt->gc.gcNumber()), count(0), root(nullptr)
    {}

    ~VerifyPreTracer() {
        js_free(root);
    }
};





static void
AccumulateEdge(JSTracer *jstrc, void **thingp, JSGCTraceKind kind)
{
    VerifyPreTracer *trc = (VerifyPreTracer *)jstrc;

    JS_ASSERT(!IsInsideNursery(*reinterpret_cast<Cell **>(thingp)));

    trc->edgeptr += sizeof(EdgeValue);
    if (trc->edgeptr >= trc->term) {
        trc->edgeptr = trc->term;
        return;
    }

    VerifyNode *node = trc->curnode;
    uint32_t i = node->count;

    node->edges[i].thing = *thingp;
    node->edges[i].kind = kind;
    node->edges[i].label = trc->tracingName("<unknown>");
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
            return nullptr;
        }

        node->thing = thing;
        node->count = 0;
        node->kind = kind;
        trc->nodemap.add(p, thing, node);
        return node;
    }
    return nullptr;
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
gc::GCRuntime::startVerifyPreBarriers()
{
    if (verifyPreData || incrementalState != NO_INCREMENTAL)
        return;

    





    if (verifyPostData)
        return;

    evictNursery();

    AutoPrepareForTracing prep(rt, WithAtoms);

    if (!IsIncrementalGCSafe(rt))
        return;

    for (GCChunkSet::Range r(chunkSet.all()); !r.empty(); r.popFront())
        r.front()->bitmap.clear();

    number++;

    VerifyPreTracer *trc = js_new<VerifyPreTracer>(rt, JSTraceCallback(nullptr));
    if (!trc)
        return;

    



    trc->setTraceCallback(AccumulateEdge);

    const size_t size = 64 * 1024 * 1024;
    trc->root = (VerifyNode *)js_malloc(size);
    if (!trc->root)
        goto oom;
    trc->edgeptr = (char *)trc->root;
    trc->term = trc->edgeptr + size;

    if (!trc->nodemap.init())
        goto oom;

    
    trc->curnode = MakeNode(trc, nullptr, JSGCTraceKind(0));

    
    incrementalState = MARK_ROOTS;

    
    markRuntime(trc);

    VerifyNode *node;
    node = trc->curnode;
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

    verifyPreData = trc;
    incrementalState = MARK;
    marker.start();

    rt->setNeedsIncrementalBarrier(true);
    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        PurgeJITCaches(zone);
        zone->setNeedsIncrementalBarrier(true, Zone::UpdateJit);
        zone->allocator.arenas.purge();
    }

    return;

oom:
    incrementalState = NO_INCREMENTAL;
    js_delete(trc);
    verifyPreData = nullptr;
}

static bool
IsMarkedOrAllocated(TenuredCell *cell)
{
    return cell->isMarked() || cell->arenaHeader()->allocatedDuringIncremental;
}

static const uint32_t MAX_VERIFIER_EDGES = 1000;








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
            node->edges[i].thing = nullptr;
            return;
        }
    }
}

static void
AssertMarkedOrAllocated(const EdgeValue &edge)
{
    if (!edge.thing || IsMarkedOrAllocated(TenuredCell::fromPointer(edge.thing)))
        return;

    
    if (edge.kind == JSTRACE_STRING && static_cast<JSString *>(edge.thing)->isPermanentAtom())
        return;
    if (edge.kind == JSTRACE_SYMBOL && static_cast<JS::Symbol *>(edge.thing)->isWellKnownSymbol())
        return;

    char msgbuf[1024];
    const char *label = edge.label;

    JS_snprintf(msgbuf, sizeof(msgbuf), "[barrier verifier] Unmarked edge: %s", label);
    MOZ_ReportAssertionFailure(msgbuf, __FILE__, __LINE__);
    MOZ_CRASH();
}

bool
gc::GCRuntime::endVerifyPreBarriers()
{
    VerifyPreTracer *trc = (VerifyPreTracer *)verifyPreData;

    if (!trc)
        return false;

    JS_ASSERT(!JS::IsGenerationalGCEnabled(rt));

    AutoPrepareForTracing prep(rt, SkipAtoms);

    bool compartmentCreated = false;

    
    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        if (!zone->needsIncrementalBarrier())
            compartmentCreated = true;

        zone->setNeedsIncrementalBarrier(false, Zone::UpdateJit);
        PurgeJITCaches(zone);
    }
    rt->setNeedsIncrementalBarrier(false);

    



    JS_ASSERT(trc->number == number);
    number++;

    verifyPreData = nullptr;
    incrementalState = NO_INCREMENTAL;

    if (!compartmentCreated && IsIncrementalGCSafe(rt)) {
        trc->setTraceCallback(CheckEdge);

        
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

    marker.reset();
    marker.stop();

    js_delete(trc);
    return true;
}



struct VerifyPostTracer : JSTracer
{
    
    uint64_t number;

    
    int count;

    
    typedef HashSet<void **, PointerHasher<void **, 3>, SystemAllocPolicy> EdgeSet;
    EdgeSet *edges;

    VerifyPostTracer(JSRuntime *rt, JSTraceCallback callback)
      : JSTracer(rt, callback), number(rt->gc.gcNumber()), count(0)
    {}
};






void
gc::GCRuntime::startVerifyPostBarriers()
{
#ifdef JSGC_GENERATIONAL
    if (verifyPostData ||
        incrementalState != NO_INCREMENTAL)
    {
        return;
    }

    evictNursery();

    number++;

    VerifyPostTracer *trc = js_new<VerifyPostTracer>(rt, JSTraceCallback(nullptr));
    if (!trc)
        return;

    verifyPostData = trc;
#endif
}

#ifdef JSGC_GENERATIONAL
void
PostVerifierCollectStoreBufferEdges(JSTracer *jstrc, void **thingp, JSGCTraceKind kind)
{
    VerifyPostTracer *trc = (VerifyPostTracer *)jstrc;

    
    if (kind != JSTRACE_OBJECT)
        return;

    
    JSObject *dst = *reinterpret_cast<JSObject **>(thingp);
    if (trc->runtime()->gc.nursery.isInside(thingp) || !IsInsideNursery(dst))
        return;

    




    void **loc = trc->tracingLocation(thingp);

    trc->edges->put(loc);
}

static void
AssertStoreBufferContainsEdge(VerifyPostTracer::EdgeSet *edges, void **loc, JSObject *dst)
{
    if (edges->has(loc))
        return;

    char msgbuf[1024];
    JS_snprintf(msgbuf, sizeof(msgbuf), "[post-barrier verifier] Missing edge @ %p to %p",
                (void *)loc, (void *)dst);
    MOZ_ReportAssertionFailure(msgbuf, __FILE__, __LINE__);
    MOZ_CRASH();
}

void
PostVerifierVisitEdge(JSTracer *jstrc, void **thingp, JSGCTraceKind kind)
{
    VerifyPostTracer *trc = (VerifyPostTracer *)jstrc;

    
    if (kind != JSTRACE_OBJECT)
        return;

    
    JS_ASSERT(!trc->runtime()->gc.nursery.isInside(thingp));
    JSObject *dst = *reinterpret_cast<JSObject **>(thingp);
    if (!IsInsideNursery(dst))
        return;

    





    void **loc = trc->tracingLocation(thingp);

    AssertStoreBufferContainsEdge(trc->edges, loc, dst);
}
#endif

bool
js::gc::GCRuntime::endVerifyPostBarriers()
{
#ifdef JSGC_GENERATIONAL
    VerifyPostTracer *trc = (VerifyPostTracer *)verifyPostData;
    if (!trc)
        return false;

    VerifyPostTracer::EdgeSet edges;
    AutoPrepareForTracing prep(rt, SkipAtoms);

    
    trc->setTraceCallback(PostVerifierCollectStoreBufferEdges);
    if (!edges.init())
        goto oom;
    trc->edges = &edges;
    storeBuffer.markAll(trc);

    
    trc->setTraceCallback(PostVerifierVisitEdge);
    for (GCZoneGroupIter zone(rt); !zone.done(); zone.next()) {
        for (size_t kind = 0; kind < FINALIZE_LIMIT; ++kind) {
            for (ZoneCellIterUnderGC cells(zone, AllocKind(kind)); !cells.done(); cells.next()) {
                Cell *src = cells.getCell();
                JS_TraceChildren(trc, src, MapAllocToTraceKind(AllocKind(kind)));
            }
        }
    }

oom:
    js_delete(trc);
    verifyPostData = nullptr;
    return true;
#else
    return false;
#endif
}



void
gc::GCRuntime::verifyPreBarriers()
{
    if (verifyPreData)
        endVerifyPreBarriers();
    else
        startVerifyPreBarriers();
}

void
gc::GCRuntime::verifyPostBarriers()
{
    if (verifyPostData)
        endVerifyPostBarriers();
    else
        startVerifyPostBarriers();
}

void
gc::VerifyBarriers(JSRuntime *rt, VerifierType type)
{
    if (type == PreBarrierVerifier)
        rt->gc.verifyPreBarriers();
    else
        rt->gc.verifyPostBarriers();
}

void
gc::GCRuntime::maybeVerifyPreBarriers(bool always)
{
    if (zealMode != ZealVerifierPreValue)
        return;

    if (rt->mainThread.suppressGC)
        return;

    if (VerifyPreTracer *trc = (VerifyPreTracer *)verifyPreData) {
        if (++trc->count < zealFrequency && !always)
            return;

        endVerifyPreBarriers();
    }

    startVerifyPreBarriers();
}

void
gc::GCRuntime::maybeVerifyPostBarriers(bool always)
{
#ifdef JSGC_GENERATIONAL
    if (zealMode != ZealVerifierPostValue)
        return;

    if (rt->mainThread.suppressGC || !storeBuffer.isEnabled())
        return;

    if (VerifyPostTracer *trc = (VerifyPostTracer *)verifyPostData) {
        if (++trc->count < zealFrequency && !always)
            return;

        endVerifyPostBarriers();
    }
    startVerifyPostBarriers();
#endif
}

void
js::gc::MaybeVerifyBarriers(JSContext *cx, bool always)
{
    GCRuntime *gc = &cx->runtime()->gc;
    gc->maybeVerifyPreBarriers(always);
    gc->maybeVerifyPostBarriers(always);
}

void
js::gc::GCRuntime::finishVerifier()
{
    if (VerifyPreTracer *trc = (VerifyPreTracer *)verifyPreData) {
        js_delete(trc);
        verifyPreData = nullptr;
    }
#ifdef JSGC_GENERATIONAL
    if (VerifyPostTracer *trc = (VerifyPostTracer *)verifyPostData) {
        js_delete(trc);
        verifyPostData = nullptr;
    }
#endif
}

#endif 
