





#include "jswatchpoint.h"

#include "jsatom.h"
#include "jscompartment.h"
#include "jsfriendapi.h"

#include "gc/Marking.h"

#include "jsgcinlines.h"

using namespace js;
using namespace js::gc;

inline HashNumber
DefaultHasher<WatchKey>::hash(const Lookup &key)
{
    return DefaultHasher<JSObject *>::hash(key.object.get()) ^ HashId(key.id.get());
}

namespace {

class AutoEntryHolder {
    typedef WatchpointMap::Map Map;
    Map &map;
    Map::Ptr p;
    uint32_t gen;
    RootedObject obj;
    RootedId id;

  public:
    AutoEntryHolder(JSContext *cx, Map &map, Map::Ptr p)
      : map(map), p(p), gen(map.generation()), obj(cx, p->key().object), id(cx, p->key().id)
    {
        JS_ASSERT(!p->value().held);
        p->value().held = true;
    }

    ~AutoEntryHolder() {
        if (gen != map.generation())
            p = map.lookup(WatchKey(obj, id));
        if (p)
            p->value().held = false;
    }
};

} 

bool
WatchpointMap::init()
{
    return map.init();
}

bool
WatchpointMap::watch(JSContext *cx, HandleObject obj, HandleId id,
                     JSWatchPointHandler handler, HandleObject closure)
{
    JS_ASSERT(JSID_IS_STRING(id) || JSID_IS_INT(id) || JSID_IS_SYMBOL(id));

    if (!obj->setWatched(cx))
        return false;

    Watchpoint w(handler, closure, false);
    if (!map.put(WatchKey(obj, id), w)) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    




    return true;
}

void
WatchpointMap::unwatch(JSObject *obj, jsid id,
                       JSWatchPointHandler *handlerp, JSObject **closurep)
{
    if (Map::Ptr p = map.lookup(WatchKey(obj, id))) {
        if (handlerp)
            *handlerp = p->value().handler;
        if (closurep) {
            
            
            JS::ExposeObjectToActiveJS(p->value().closure);
            *closurep = p->value().closure;
        }
        map.remove(p);
    }
}

void
WatchpointMap::unwatchObject(JSObject *obj)
{
    for (Map::Enum e(map); !e.empty(); e.popFront()) {
        Map::Entry &entry = e.front();
        if (entry.key().object == obj)
            e.removeFront();
    }
}

void
WatchpointMap::clear()
{
    map.clear();
}

bool
WatchpointMap::triggerWatchpoint(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue vp)
{
    Map::Ptr p = map.lookup(WatchKey(obj, id));
    if (!p || p->value().held)
        return true;

    AutoEntryHolder holder(cx, map, p);

    
    JSWatchPointHandler handler = p->value().handler;
    RootedObject closure(cx, p->value().closure);

    
    Value old;
    old.setUndefined();
    if (obj->isNative()) {
        if (Shape *shape = obj->nativeLookup(cx, id)) {
            if (shape->hasSlot())
                old = obj->nativeGetSlot(shape->slot());
        }
    }

    
    
    JS::ExposeObjectToActiveJS(closure);

    
    return handler(cx, obj, id, old, vp.address(), closure);
}

bool
WatchpointMap::markCompartmentIteratively(JSCompartment *c, JSTracer *trc)
{
    if (!c->watchpointMap)
        return false;
    return c->watchpointMap->markIteratively(trc);
}

bool
WatchpointMap::markIteratively(JSTracer *trc)
{
    bool marked = false;
    for (Map::Enum e(map); !e.empty(); e.popFront()) {
        Map::Entry &entry = e.front();
        JSObject *priorKeyObj = entry.key().object;
        jsid priorKeyId(entry.key().id.get());
        bool objectIsLive =
            IsObjectMarked(const_cast<PreBarrieredObject *>(&entry.key().object));
        if (objectIsLive || entry.value().held) {
            if (!objectIsLive) {
                MarkObject(trc, const_cast<PreBarrieredObject *>(&entry.key().object),
                           "held Watchpoint object");
                marked = true;
            }

            JS_ASSERT(JSID_IS_STRING(priorKeyId) ||
                      JSID_IS_INT(priorKeyId) ||
                      JSID_IS_SYMBOL(priorKeyId));
            MarkId(trc, const_cast<PreBarrieredId *>(&entry.key().id), "WatchKey::id");

            if (entry.value().closure && !IsObjectMarked(&entry.value().closure)) {
                MarkObject(trc, &entry.value().closure, "Watchpoint::closure");
                marked = true;
            }

            
            if (priorKeyObj != entry.key().object || priorKeyId != entry.key().id)
                e.rekeyFront(WatchKey(entry.key().object, entry.key().id));
        }
    }
    return marked;
}

void
WatchpointMap::markAll(JSTracer *trc)
{
    for (Map::Enum e(map); !e.empty(); e.popFront()) {
        Map::Entry &entry = e.front();
        WatchKey key = entry.key();
        WatchKey prior = key;
        JS_ASSERT(JSID_IS_STRING(prior.id) || JSID_IS_INT(prior.id) || JSID_IS_SYMBOL(prior.id));

        MarkObject(trc, const_cast<PreBarrieredObject *>(&key.object),
                   "held Watchpoint object");
        MarkId(trc, const_cast<PreBarrieredId *>(&key.id), "WatchKey::id");
        MarkObject(trc, &entry.value().closure, "Watchpoint::closure");

        if (prior.object != key.object || prior.id != key.id)
            e.rekeyFront(key);
    }
}

void
WatchpointMap::sweepAll(JSRuntime *rt)
{
    for (GCCompartmentsIter c(rt); !c.done(); c.next()) {
        if (WatchpointMap *wpmap = c->watchpointMap)
            wpmap->sweep();
    }
}

void
WatchpointMap::sweep()
{
    for (Map::Enum e(map); !e.empty(); e.popFront()) {
        Map::Entry &entry = e.front();
        JSObject *obj(entry.key().object);
        if (IsObjectAboutToBeFinalized(&obj)) {
            JS_ASSERT(!entry.value().held);
            e.removeFront();
        } else if (obj != entry.key().object) {
            e.rekeyFront(WatchKey(obj, entry.key().id));
        }
    }
}

void
WatchpointMap::traceAll(WeakMapTracer *trc)
{
    JSRuntime *rt = trc->runtime;
    for (CompartmentsIter comp(rt, SkipAtoms); !comp.done(); comp.next()) {
        if (WatchpointMap *wpmap = comp->watchpointMap)
            wpmap->trace(trc);
    }
}

void
WatchpointMap::trace(WeakMapTracer *trc)
{
    for (Map::Range r = map.all(); !r.empty(); r.popFront()) {
        Map::Entry &entry = r.front();
        trc->callback(trc, nullptr,
                      entry.key().object.get(), JSTRACE_OBJECT,
                      entry.value().closure.get(), JSTRACE_OBJECT);
    }
}
