





#ifndef vm_DebuggerMemory_h
#define vm_DebuggerMemory_h

#include "jsapi.h"
#include "jscntxt.h"
#include "jsobj.h"
#include "js/Class.h"
#include "js/Value.h"

namespace js {

class DebuggerMemory : public JSObject {
    friend class Debugger;

    static DebuggerMemory *checkThis(JSContext *cx, CallArgs &args, const char *fnName);

    Debugger *getDebugger();

  public:
    static DebuggerMemory *create(JSContext *cx, Debugger *dbg);

    enum {
        JSSLOT_DEBUGGER,
        JSSLOT_COUNT
    };

    static bool construct(JSContext *cx, unsigned argc, Value *vp);
    static const Class          class_;
    static const JSPropertySpec properties[];
    static const JSFunctionSpec methods[];

    
    static bool setTrackingAllocationSites(JSContext *cx, unsigned argc, Value *vp);
    static bool getTrackingAllocationSites(JSContext *cx, unsigned argc, Value *vp);
    static bool setMaxAllocationsLogLength(JSContext*cx, unsigned argc, Value *vp);
    static bool getMaxAllocationsLogLength(JSContext*cx, unsigned argc, Value *vp);
    static bool setAllocationSamplingProbability(JSContext*cx, unsigned argc, Value *vp);
    static bool getAllocationSamplingProbability(JSContext*cx, unsigned argc, Value *vp);

    
    static bool takeCensus(JSContext *cx, unsigned argc, Value *vp);
    static bool drainAllocationsLog(JSContext *cx, unsigned argc, Value *vp);
};

} 

#endif 
