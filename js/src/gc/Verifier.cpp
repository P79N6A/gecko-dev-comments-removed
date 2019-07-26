





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
using namespace mozilla;

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
      : JSTracer(rt, callback), noggc(rt), number(rt->gcNumber), count(0), root(nullptr)
    {}

    ~VerifyPreTracer() {
        js_free(root);
    }
};





static void
AccumulateEdge(JSTracer *jstrc, void **thingp, JSGCTraceKind kind)
{
    VerifyPreTracer *trc = (VerifyPreTracer *)jstrc;

    JS_ASSERT(!IsInsideNursery(trc->runtime(), *(uintptr_t **)thingp));

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
gc::StartVerifyPreBarriers(JSRuntime *rt)
{
    if (rt->gcVerifyPreData || rt->gcIncrementalState != NO_INCREMENTAL)
        return;

    





    if (rt->gcVerifyPostData)
        return;

    MinorGC(rt, JS::gcreason::EVICT_NURSERY);

    AutoPrepareForTracing prep(rt, WithAtoms);

    if (!IsIncrementalGCSafe(rt))
        return;

    for (GCChunkSet::Range r(rt->gcChunkSet.all()); !r.empty(); r.popFront())
        r.front()->bitmap.clear();

    rt->gcNumber++;

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

    
    rt->gcIncrementalState = MARK_ROOTS;

    
    MarkRuntime(trc);

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

    rt->gcVerifyPreData = trc;
    rt->gcIncrementalState = MARK;
    rt->gcMarker.start();

    rt->setNeedsBarrier(true);
    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        PurgeJITCaches(zone);
        zone->setNeedsBarrier(true, Zone::UpdateIon);
        zone->allocator.arenas.purge();
    }

    return;

oom:
    rt->gcIncrementalState = NO_INCREMENTAL;
    js_delete(trc);
    rt->gcVerifyPreData = nullptr;
}

static bool
IsMarkedOrAllocated(Cell *cell)
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
    if (!edge.thing || IsMarkedOrAllocated(static_cast<Cell *>(edge.thing)))
        return;

    
    if (edge.kind == JSTRACE_STRING && static_cast<JSString *>(edge.thing)->isPermanentAtom())
        return;

    char msgbuf[1024];
    const char *label = edge.label;

    JS_snprintf(msgbuf, sizeof(msgbuf), "[barrier verifier] Unmarked edge: %s", label);
    MOZ_ReportAssertionFailure(msgbuf, __FILE__, __LINE__);
    MOZ_CRASH();
}

void
gc::EndVerifyPreBarriers(JSRuntime *rt)
{
    JS_ASSERT(!JS::IsGenerationalGCEnabled(rt));

    AutoPrepareForTracing prep(rt, SkipAtoms);

    VerifyPreTracer *trc = (VerifyPreTracer *)rt->gcVerifyPreData;

    if (!trc)
        return;

    bool compartmentCreated = false;

    
    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        if (!zone->needsBarrier())
            compartmentCreated = true;

        zone->setNeedsBarrier(false, Zone::UpdateIon);
        PurgeJITCaches(zone);
    }
    rt->setNeedsBarrier(false);

    



    JS_ASSERT(trc->number == rt->gcNumber);
    rt->gcNumber++;

    rt->gcVerifyPreData = nullptr;
    rt->gcIncrementalState = NO_INCREMENTAL;

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

    rt->gcMarker.reset();
    rt->gcMarker.stop();

    js_delete(trc);
}



struct VerifyPostTracer : JSTracer
{
    
    uint64_t number;

    
    int count;

    
    typedef HashSet<void **, PointerHasher<void **, 3>, SystemAllocPolicy> EdgeSet;
    EdgeSet *edges;

    VerifyPostTracer(JSRuntime *rt, JSTraceCallback callback)
      : JSTracer(rt, callback), number(rt->gcNumber), count(0)
    {}
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

    MinorGC(rt, JS::gcreason::EVICT_NURSERY);

    rt->gcNumber++;

    VerifyPostTracer *trc = js_new<VerifyPostTracer>(rt, JSTraceCallback(nullptr));
    if (!trc)
        return;

    rt->gcVerifyPostData = trc;
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
    if (trc->runtime()->gcNursery.isInside(thingp) || !trc->runtime()->gcNursery.isInside(dst))
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

    
    JS_ASSERT(!trc->runtime()->gcNursery.isInside(thingp));
    JSObject *dst = *reinterpret_cast<JSObject **>(thingp);
    if (!trc->runtime()->gcNursery.isInside(dst))
        return;

    





    void **loc = trc->tracingLocation(thingp);

    AssertStoreBufferContainsEdge(trc->edges, loc, dst);
}
#endif

void
js::gc::EndVerifyPostBarriers(JSRuntime *rt)
{
#ifdef JSGC_GENERATIONAL
    VerifyPostTracer::EdgeSet edges;
    AutoPrepareForTracing prep(rt, SkipAtoms);

    VerifyPostTracer *trc = (VerifyPostTracer *)rt->gcVerifyPostData;

    
    trc->setTraceCallback(PostVerifierCollectStoreBufferEdges);
    if (!edges.init())
        goto oom;
    trc->edges = &edges;
    rt->gcStoreBuffer.markAll(trc);

    
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
    rt->gcVerifyPostData = nullptr;
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
#ifdef JSGC_GENERATIONAL
    if (rt->gcZeal() != ZealVerifierPostValue)
        return;

    if (rt->mainThread.suppressGC || !rt->gcStoreBuffer.isEnabled())
        return;

    if (VerifyPostTracer *trc = (VerifyPostTracer *)rt->gcVerifyPostData) {
        if (++trc->count < rt->gcZealFrequency && !always)
            return;

        EndVerifyPostBarriers(rt);
    }
    StartVerifyPostBarriers(rt);
#endif
}

void
js::gc::MaybeVerifyBarriers(JSContext *cx, bool always)
{
    MaybeVerifyPreBarriers(cx->runtime(), always);
    MaybeVerifyPostBarriers(cx->runtime(), always);
}

void
js::gc::FinishVerifier(JSRuntime *rt)
{
    if (VerifyPreTracer *trc = (VerifyPreTracer *)rt->gcVerifyPreData) {
        js_delete(trc);
        rt->gcVerifyPreData = nullptr;
    }
#ifdef JSGC_GENERATIONAL
    if (VerifyPostTracer *trc = (VerifyPostTracer *)rt->gcVerifyPostData) {
        js_delete(trc);
        rt->gcVerifyPostData = nullptr;
    }
#endif
}

#endif 
