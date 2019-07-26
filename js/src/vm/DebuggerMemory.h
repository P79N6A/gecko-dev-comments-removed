





#ifndef vm_DebuggerMemory_h
#define vm_DebuggerMemory_h

#include "jsapi.h"
#include "jscntxt.h"
#include "jsobj.h"
#include "js/Class.h"
#include "js/Value.h"

namespace js {

class DebuggerMemory : public JSObject {
    enum {
        JSSLOT_DEBUGGER_MEMORY_COUNT
    };

public:

    static bool construct(JSContext *cx, unsigned argc, Value *vp);

    static const Class          class_;
    static const JSPropertySpec properties[];
    static const JSFunctionSpec methods[];
};

} 

#endif 
