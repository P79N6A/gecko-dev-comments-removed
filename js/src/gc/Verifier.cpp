





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
    void* thing;
    JS::TraceKind kind;
    const char* label;
};

struct VerifyNode
{
    void* thing;
    JS::TraceKind kind;
    uint32_t count;
    EdgeValue edges[1];
};

typedef HashMap<void*, VerifyNode*, DefaultHasher<void*>, SystemAllocPolicy> NodeMap;














class js::VerifyPreTracer : public JS::CallbackTracer
{
    JS::AutoDisableGenerationalGC noggc;

    void trace(void** thingp, JS::TraceKind kind) override;

  public:
    
    uint64_t number;

    
    int count;

    
    VerifyNode* curnode;
    VerifyNode* root;
    char* edgeptr;
    char* term;
    NodeMap nodemap;

    explicit VerifyPreTracer(JSRuntime* rt)
      : JS::CallbackTracer(rt), noggc(rt), number(rt->gc.gcNumber()), count(0), curnode(nullptr),
        root(nullptr), edgeptr(nullptr), term(nullptr)
    {}

    ~VerifyPreTracer() {
        js_free(root);
    }
};





void
VerifyPreTracer::trace(void** thingp, JS::TraceKind kind)
{
    MOZ_ASSERT(!IsInsideNursery(*reinterpret_cast<Cell**>(thingp)));

    edgeptr += sizeof(EdgeValue);
    if (edgeptr >= term) {
        edgeptr = term;
        return;
    }

    VerifyNode* node = curnode;
    uint32_t i = node->count;

    node->edges[i].thing = *thingp;
    node->edges[i].kind = kind;
    node->edges[i].label = contextName();
    node->count++;
}

static VerifyNode*
MakeNode(VerifyPreTracer* trc, void* thing, JS::TraceKind kind)
{
    NodeMap::AddPtr p = trc->nodemap.lookupForAdd(thing);
    if (!p) {
        VerifyNode* node = (VerifyNode*)trc->edgeptr;
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

static VerifyNode*
NextNode(VerifyNode* node)
{
    if (node->count == 0)
        return (VerifyNode*)((char*)node + sizeof(VerifyNode) - sizeof(EdgeValue));
    else
        return (VerifyNode*)((char*)node + sizeof(VerifyNode) +
                             sizeof(EdgeValue)*(node->count - 1));
}

void
gc::GCRuntime::startVerifyPreBarriers()
{
    if (verifyPreData || isIncrementalGCInProgress())
        return;

    evictNursery();

    AutoPrepareForTracing prep(rt, WithAtoms);

    if (!IsIncrementalGCSafe(rt))
        return;

    for (auto chunk = allNonEmptyChunks(); !chunk.done(); chunk.next())
        chunk->bitmap.clear();

    number++;

    VerifyPreTracer* trc = js_new<VerifyPreTracer>(rt);
    if (!trc)
        return;

    gcstats::AutoPhase ap(stats, gcstats::PHASE_TRACE_HEAP);

    const size_t size = 64 * 1024 * 1024;
    trc->root = (VerifyNode*)js_malloc(size);
    if (!trc->root)
        goto oom;
    trc->edgeptr = (char*)trc->root;
    trc->term = trc->edgeptr + size;

    if (!trc->nodemap.init())
        goto oom;

    
    trc->curnode = MakeNode(trc, nullptr, JS::TraceKind(0));

    incrementalState = MARK_ROOTS;

    
    markRuntime(trc);

    VerifyNode* node;
    node = trc->curnode;
    if (trc->edgeptr == trc->term)
        goto oom;

    
    while ((char*)node < trc->edgeptr) {
        for (uint32_t i = 0; i < node->count; i++) {
            EdgeValue& e = node->edges[i];
            VerifyNode* child = MakeNode(trc, e.thing, e.kind);
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

    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        PurgeJITCaches(zone);
        zone->setNeedsIncrementalBarrier(true, Zone::UpdateJit);
        zone->arenas.purge();
    }

    return;

oom:
    incrementalState = NO_INCREMENTAL;
    js_delete(trc);
    verifyPreData = nullptr;
}

static bool
IsMarkedOrAllocated(TenuredCell* cell)
{
    return cell->isMarked() || cell->arenaHeader()->allocatedDuringIncremental;
}

struct CheckEdgeTracer : public JS::CallbackTracer {
    VerifyNode* node;
    explicit CheckEdgeTracer(JSRuntime* rt) : JS::CallbackTracer(rt), node(nullptr) {}
    void trace(void** thingp, JS::TraceKind kind) override;
};

static const uint32_t MAX_VERIFIER_EDGES = 1000;








void
CheckEdgeTracer::trace(void** thingp, JS::TraceKind kind)
{
    
    if (node->count > MAX_VERIFIER_EDGES)
        return;

    for (uint32_t i = 0; i < node->count; i++) {
        if (node->edges[i].thing == *thingp) {
            MOZ_ASSERT(node->edges[i].kind == kind);
            node->edges[i].thing = nullptr;
            return;
        }
    }
}

static void
AssertMarkedOrAllocated(const EdgeValue& edge)
{
    if (!edge.thing || IsMarkedOrAllocated(TenuredCell::fromPointer(edge.thing)))
        return;

    
    if (edge.kind == JS::TraceKind::String && static_cast<JSString*>(edge.thing)->isPermanentAtom())
        return;
    if (edge.kind == JS::TraceKind::Symbol && static_cast<JS::Symbol*>(edge.thing)->isWellKnownSymbol())
        return;

    char msgbuf[1024];
    JS_snprintf(msgbuf, sizeof(msgbuf), "[barrier verifier] Unmarked edge: %s", edge.label);
    MOZ_ReportAssertionFailure(msgbuf, __FILE__, __LINE__);
    MOZ_CRASH();
}

bool
gc::GCRuntime::endVerifyPreBarriers()
{
    VerifyPreTracer* trc = verifyPreData;

    if (!trc)
        return false;

    MOZ_ASSERT(!JS::IsGenerationalGCEnabled(rt));

    AutoPrepareForTracing prep(rt, SkipAtoms);

    bool compartmentCreated = false;

    
    for (ZonesIter zone(rt, WithAtoms); !zone.done(); zone.next()) {
        if (!zone->needsIncrementalBarrier())
            compartmentCreated = true;

        zone->setNeedsIncrementalBarrier(false, Zone::UpdateJit);
        PurgeJITCaches(zone);
    }

    



    MOZ_ASSERT(trc->number == number);
    number++;

    verifyPreData = nullptr;
    incrementalState = NO_INCREMENTAL;

    if (!compartmentCreated && IsIncrementalGCSafe(rt)) {
        CheckEdgeTracer cetrc(rt);

        
        VerifyNode* node = NextNode(trc->root);
        while ((char*)node < trc->edgeptr) {
            cetrc.node = node;
            JS_TraceChildren(&cetrc, node->thing, node->kind);

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



void
gc::GCRuntime::verifyPreBarriers()
{
    if (verifyPreData)
        endVerifyPreBarriers();
    else
        startVerifyPreBarriers();
}

void
gc::VerifyBarriers(JSRuntime* rt, VerifierType type)
{
    if (type == PreBarrierVerifier)
        rt->gc.verifyPreBarriers();
}

void
gc::GCRuntime::maybeVerifyPreBarriers(bool always)
{
    if (zealMode != ZealVerifierPreValue)
        return;

    if (rt->mainThread.suppressGC)
        return;

    if (verifyPreData) {
        if (++verifyPreData->count < zealFrequency && !always)
            return;

        endVerifyPreBarriers();
    }

    startVerifyPreBarriers();
}

void
js::gc::MaybeVerifyBarriers(JSContext* cx, bool always)
{
    GCRuntime* gc = &cx->runtime()->gc;
    gc->maybeVerifyPreBarriers(always);
}

void
js::gc::GCRuntime::finishVerifier()
{
    if (verifyPreData) {
        js_delete(verifyPreData);
        verifyPreData = nullptr;
    }
}

#endif 
