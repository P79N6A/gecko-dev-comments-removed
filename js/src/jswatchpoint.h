






































#ifndef jswatchpoint_h___
#define jswatchpoint_h___

#include "jsalloc.h"
#include "jsprvtd.h"
#include "jsapi.h"

#include "js/HashTable.h"

namespace js {

struct WatchKey {
    WatchKey() {}
    WatchKey(JSObject *obj, jsid id) : object(obj), id(id) {}
    JSObject *object;
    jsid id;
};

struct Watchpoint {
    JSWatchPointHandler handler;
    JSObject *closure;
    bool held;  
};

template <>
struct DefaultHasher<WatchKey> {
    typedef WatchKey Lookup;
    static inline js::HashNumber hash(const Lookup &key);

    static bool match(const WatchKey &k, const Lookup &l) {
        return k.object == l.object && k.id == l.id;
    }
};

class WatchpointMap {
  public:
    typedef HashMap<WatchKey, Watchpoint, DefaultHasher<WatchKey>, SystemAllocPolicy> Map;

    bool init();
    bool watch(JSContext *cx, JSObject *obj, jsid id,
               JSWatchPointHandler handler, JSObject *closure);
    void unwatch(JSObject *obj, jsid id,
                 JSWatchPointHandler *handlerp, JSObject **closurep);
    void unwatchObject(JSObject *obj);
    void clear();

    bool triggerWatchpoint(JSContext *cx, JSObject *obj, jsid id, Value *vp);

    static bool markAllIteratively(JSTracer *trc);
    bool markIteratively(JSTracer *trc);
    static void sweepAll(JSContext *cx);
    void sweep(JSContext *cx);

  private:
    Map map;
};

}

#endif 
