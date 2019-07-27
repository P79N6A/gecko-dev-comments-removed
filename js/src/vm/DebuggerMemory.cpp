





#include "vm/DebuggerMemory.h"

#include "jscompartment.h"
#include "vm/Debugger.h"
#include "vm/GlobalObject.h"
#include "vm/SavedStacks.h"

#include "vm/Debugger-inl.h"

using namespace js;

 DebuggerMemory *
DebuggerMemory::create(JSContext *cx, Debugger *dbg)
{

    Value memoryProto = dbg->object->getReservedSlot(Debugger::JSSLOT_DEBUG_MEMORY_PROTO);
    RootedObject memory(cx, NewObjectWithGivenProto(cx, &class_,
                                                    &memoryProto.toObject(), nullptr));
    if (!memory)
        return nullptr;

    dbg->object->setReservedSlot(Debugger::JSSLOT_DEBUG_MEMORY_INSTANCE, ObjectValue(*memory));
    memory->setReservedSlot(JSSLOT_DEBUGGER, ObjectValue(*dbg->object));

    return &memory->as<DebuggerMemory>();
}

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
    JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_COUNT),

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

 DebuggerMemory *
DebuggerMemory::checkThis(JSContext *cx, CallArgs &args, const char *fnName)
{
    const Value &thisValue = args.thisv();

    if (!thisValue.isObject()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NOT_NONNULL_OBJECT);
        return nullptr;
    }

    JSObject &thisObject = thisValue.toObject();
    if (!thisObject.is<DebuggerMemory>()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             class_.name, fnName, thisObject.getClass()->name);
        return nullptr;
    }

    
    
    
    
    if (thisObject.getReservedSlot(JSSLOT_DEBUGGER).isUndefined()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             class_.name, fnName, "prototype object");
        return nullptr;
    }

    return &thisObject.as<DebuggerMemory>();
}














#define THIS_DEBUGGER_MEMORY(cx, argc, vp, fnName, args, memory)        \
    CallArgs args = CallArgsFromVp(argc, vp);                           \
    Rooted<DebuggerMemory *> memory(cx, checkThis(cx, args, fnName));   \
    if (!memory)                                                        \
        return false

Debugger *
DebuggerMemory::getDebugger()
{
    return Debugger::fromJSObject(&getReservedSlot(JSSLOT_DEBUGGER).toObject());
}

 bool
DebuggerMemory::setTrackingAllocationSites(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER_MEMORY(cx, argc, vp, "(set trackingAllocationSites)", args, memory);
    if (!args.requireAtLeast(cx, "(set trackingAllocationSites)", 1))
        return false;

    Debugger *dbg = memory->getDebugger();
    bool enabling = ToBoolean(args[0]);

    if (enabling == dbg->trackingAllocationSites) {
        
        args.rval().setUndefined();
        return true;
    }

    if (enabling) {
        for (GlobalObjectSet::Range r = dbg->debuggees.all(); !r.empty(); r.popFront()) {
            JSCompartment *compartment = r.front()->compartment();
            if (compartment->hasObjectMetadataCallback()) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                     JSMSG_OBJECT_METADATA_CALLBACK_ALREADY_SET);
                return false;
            }
        }
    }

    for (GlobalObjectSet::Range r = dbg->debuggees.all(); !r.empty(); r.popFront()) {
        if (enabling) {
            r.front()->compartment()->setObjectMetadataCallback(SavedStacksMetadataCallback);
        } else {
            r.front()->compartment()->forgetObjectMetadataCallback();
        }
    }

    dbg->trackingAllocationSites = enabling;
    args.rval().setUndefined();
    return true;
}

 bool
DebuggerMemory::getTrackingAllocationSites(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER_MEMORY(cx, argc, vp, "(get trackingAllocationSites)", args, memory);
    args.rval().setBoolean(memory->getDebugger()->trackingAllocationSites);
    return true;
}

 const JSPropertySpec DebuggerMemory::properties[] = {
    JS_PSGS("trackingAllocationSites", getTrackingAllocationSites, setTrackingAllocationSites, 0),
    JS_PS_END
};

 const JSFunctionSpec DebuggerMemory::methods[] = {
    JS_FS_END
};
