





#include "vm/DebuggerMemory.h"

#include "mozilla/Maybe.h"
#include "mozilla/Move.h"

#include "jscompartment.h"

#include "gc/Marking.h"
#include "js/UbiNode.h"
#include "js/UbiNodeTraverse.h"
#include "vm/Debugger.h"
#include "vm/GlobalObject.h"
#include "vm/SavedStacks.h"

#include "vm/Debugger-inl.h"

using namespace js;

using JS::ubi::BreadthFirst;
using JS::ubi::Edge;
using JS::ubi::Node;

using mozilla::Maybe;
using mozilla::Move;

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

Debugger *
DebuggerMemory::getDebugger()
{
    const Value &dbgVal = getReservedSlot(JSSLOT_DEBUGGER);
    return Debugger::fromJSObject(&dbgVal.toObject());
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

    if (!enabling)
        dbg->emptyAllocationsLog();

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

 bool
DebuggerMemory::drainAllocationsLog(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER_MEMORY(cx, argc, vp, "drainAllocationsLog", args, memory);
    Debugger* dbg = memory->getDebugger();

    if (!dbg->trackingAllocationSites) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NOT_TRACKING_ALLOCATIONS,
                             "drainAllocationsLog");
        return false;
    }

    size_t length = dbg->allocationsLogLength;

    RootedObject result(cx, NewDenseAllocatedArray(cx, length));
    if (!result)
        return false;
    result->ensureDenseInitializedLength(cx, 0, length);

    for (size_t i = 0; i < length; i++) {
        Debugger::AllocationSite *allocSite = dbg->allocationsLog.popFirst();
        result->setDenseElement(i, ObjectValue(*allocSite->frame));
        js_delete(allocSite);
    }

    dbg->allocationsLogLength = 0;
    args.rval().setObject(*result);
    return true;
}

 bool
DebuggerMemory::getMaxAllocationsLogLength(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER_MEMORY(cx, argc, vp, "(get maxAllocationsLogLength)", args, memory);
    args.rval().setInt32(memory->getDebugger()->maxAllocationsLogLength);
    return true;
}

 bool
DebuggerMemory::setMaxAllocationsLogLength(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER_MEMORY(cx, argc, vp, "(set maxAllocationsLogLength)", args, memory);
    if (!args.requireAtLeast(cx, "(set maxAllocationsLogLength)", 1))
        return false;

    int32_t max;
    if (!ToInt32(cx, args[0], &max))
        return false;

    if (max < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNEXPECTED_TYPE,
                             "(set maxAllocationsLogLength)'s parameter",
                             "not a positive integer");
        return false;
    }

    Debugger *dbg = memory->getDebugger();
    dbg->maxAllocationsLogLength = max;

    while (dbg->allocationsLogLength > dbg->maxAllocationsLogLength) {
        js_delete(dbg->allocationsLog.getFirst());
        dbg->allocationsLogLength--;
    }

    args.rval().setUndefined();
    return true;
}





namespace js {
namespace dbg {


struct Census {
    JSContext * const cx;
    Zone::ZoneSet debuggeeZones;
    Zone *atomsZone;

    Census(JSContext *cx) : cx(cx), atomsZone(nullptr) { }

    bool init() {
        AutoLockForExclusiveAccess lock(cx);
        atomsZone = cx->runtime()->atomsCompartment()->zone();
        return debuggeeZones.init();
    }
};

























class Tally {
    size_t counter;

  public:
    Tally(Census &census) : counter(0) { }
    Tally(Tally &&rhs) : counter(rhs.counter) { }
    Tally &operator=(Tally &&rhs) { counter = rhs.counter; return *this; }

    bool init(Census &census) { return true; }

    bool count(Census &census, const Node &node) {
        counter++;
        return true;
    }

    bool report(Census &census, MutableHandleValue report) {
        RootedObject obj(census.cx, NewBuiltinClassInstance(census.cx, &JSObject::class_));
        RootedValue countValue(census.cx, NumberValue(counter));
        if (!obj ||
            !JSObject::defineProperty(census.cx, obj, census.cx->names().count, countValue))
        {
            return false;
        }
        report.setObject(*obj);
        return true;
    }
};



template<typename Assorter>
class CensusHandler {
    Census &census;
    Assorter assorter;

  public:
    CensusHandler(Census &census) : census(census), assorter(census) { }

    bool init(Census &census) { return assorter.init(census); }
    bool report(Census &census, MutableHandleValue report) {
        return assorter.report(census, report);
    }

    
    class NodeData { };

    bool operator() (BreadthFirst<CensusHandler> &traversal,
                     Node origin, const Edge &edge,
                     NodeData *referentData, bool first)
    {
        
        
        if (!first)
            return true;

        
        
        
        
        
        
        const Node &referent = edge.referent;
        Zone *zone = referent.zone();

        if (census.debuggeeZones.has(zone)) {
            return assorter.count(census, referent);
        }

        if (zone == census.atomsZone) {
            traversal.abandonReferent();
            return assorter.count(census, referent);
        }

        traversal.abandonReferent();
        return true;
    }
};



typedef CensusHandler<Tally> TallyingHandler;
typedef BreadthFirst<TallyingHandler> TallyingTraversal;

} 
} 

bool
DebuggerMemory::takeCensus(JSContext *cx, unsigned argc, Value *vp)
{
    THIS_DEBUGGER_MEMORY(cx, argc, vp, "Debugger.Memory.prototype.census", args, memory);
    Debugger *debugger = memory->getDebugger();

    dbg::Census census(cx);
    if (!census.init())
        return false;
    dbg::TallyingHandler handler(census);
    if (!handler.init(census))
        return false;

    {
        JS::AutoCheckCannotGC noGC;

        dbg::TallyingTraversal traversal(cx, handler, noGC);
        if (!traversal.init())
            return false;

        
        
        
        for (GlobalObjectSet::Range r = debugger->debuggees.all(); !r.empty(); r.popFront()) {
            if (!census.debuggeeZones.put(r.front()->zone()) ||
                !traversal.addStart(static_cast<JSObject *>(r.front())))
                return false;
        }

        if (!traversal.traverse())
            return false;
    }

    return handler.report(census, args.rval());
}






 const JSPropertySpec DebuggerMemory::properties[] = {
    JS_PSGS("trackingAllocationSites", getTrackingAllocationSites, setTrackingAllocationSites, 0),
    JS_PSGS("maxAllocationsLogLength", getMaxAllocationsLogLength, setMaxAllocationsLogLength, 0),
    JS_PS_END
};

 const JSFunctionSpec DebuggerMemory::methods[] = {
    JS_FN("drainAllocationsLog", DebuggerMemory::drainAllocationsLog, 0, 0),
    JS_FN("takeCensus", takeCensus, 0, 0),
    JS_FS_END
};
