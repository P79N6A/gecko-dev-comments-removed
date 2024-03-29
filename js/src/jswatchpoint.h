





#ifndef jswatchpoint_h
#define jswatchpoint_h

#include "jsalloc.h"

#include "gc/Barrier.h"
#include "js/HashTable.h"

namespace js {

struct WeakMapTracer;

struct WatchKey {
    WatchKey() {}
    WatchKey(JSObject* obj, jsid id) : object(obj), id(id) {}
    WatchKey(const WatchKey& key) : object(key.object.get()), id(key.id.get()) {}
    PreBarrieredObject object;
    PreBarrieredId id;

    bool operator!=(const WatchKey& other) const {
        return object != other.object || id != other.id;
    }
};

typedef bool
(* JSWatchPointHandler)(JSContext* cx, JSObject* obj, jsid id, JS::Value old,
                        JS::Value* newp, void* closure);

struct Watchpoint {
    JSWatchPointHandler handler;
    PreBarrieredObject closure;  
    bool held;  
    Watchpoint(JSWatchPointHandler handler, JSObject* closure, bool held)
      : handler(handler), closure(closure), held(held) {}
};

template <>
struct DefaultHasher<WatchKey>
{
    typedef WatchKey Lookup;
    static inline js::HashNumber hash(const Lookup& key);

    static bool match(const WatchKey& k, const Lookup& l) {
        return k.object == l.object && k.id.get() == l.id.get();
    }

    static void rekey(WatchKey& k, const WatchKey& newKey) {
        k.object.unsafeSet(newKey.object);
        k.id.unsafeSet(newKey.id);
    }
};

class WatchpointMap {
  public:
    typedef HashMap<WatchKey, Watchpoint, DefaultHasher<WatchKey>, SystemAllocPolicy> Map;

    bool init();
    bool watch(JSContext* cx, HandleObject obj, HandleId id,
               JSWatchPointHandler handler, HandleObject closure);
    void unwatch(JSObject* obj, jsid id,
                 JSWatchPointHandler* handlerp, JSObject** closurep);
    void unwatchObject(JSObject* obj);
    void clear();

    bool triggerWatchpoint(JSContext* cx, HandleObject obj, HandleId id, MutableHandleValue vp);

    static bool markCompartmentIteratively(JSCompartment* c, JSTracer* trc);
    bool markIteratively(JSTracer* trc);
    void markAll(JSTracer* trc);
    static void sweepAll(JSRuntime* rt);
    void sweep();

    static void traceAll(WeakMapTracer* trc);
    void trace(WeakMapTracer* trc);

  private:
    Map map;
};

} 

#endif 
