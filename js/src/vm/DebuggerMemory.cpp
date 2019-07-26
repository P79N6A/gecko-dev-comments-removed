





#include "vm/DebuggerMemory.h"

namespace js {

 bool
DebuggerMemory::construct(JSContext *cx, unsigned argc, Value *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NO_CONSTRUCTOR,
                         "Debugger.Memory");
    return false;
}

 const Class DebuggerMemory::class_ = {
    "Memory",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_DEBUGGER_MEMORY_COUNT),

    JS_PropertyStub,       
    JS_DeletePropertyStub, 
    JS_PropertyStub,       
    JS_StrictPropertyStub, 
    JS_EnumerateStub,      
    JS_ResolveStub,        
    JS_ConvertStub,        

    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr  
};

 const JSPropertySpec DebuggerMemory::properties[] = {
    JS_PS_END
};

 const JSFunctionSpec DebuggerMemory::methods[] = {
    JS_FS_END
};

} 
