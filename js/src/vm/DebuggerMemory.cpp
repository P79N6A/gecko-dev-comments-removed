





#include "vm/DebuggerMemory.h"

#include "mozilla/Maybe.h"
#include "mozilla/Move.h"
#include "mozilla/Vector.h"

#include <stdlib.h>

#include "jscompartment.h"

#include "gc/Marking.h"
#include "js/Debug.h"
#include "js/TracingAPI.h"
#include "js/UbiNode.h"
#include "js/UbiNodeTraverse.h"
#include "vm/Debugger.h"
#include "vm/GlobalObject.h"
#include "vm/SavedStacks.h"

#include "vm/Debugger-inl.h"
#include "vm/NativeObject-inl.h"

using namespace js;

using JS::ubi::BreadthFirst;
using JS::ubi::Edge;
using JS::ubi::Node;

using mozilla::Maybe;
using mozilla::Move;
using mozilla::Nothing;

 DebuggerMemory*
DebuggerMemory::create(JSContext* cx, Debugger* dbg)
{
    Value memoryProtoValue = dbg->object->getReservedSlot(Debugger::JSSLOT_DEBUG_MEMORY_PROTO);
    RootedObject memoryProto(cx, &memoryProtoValue.toObject());
    RootedNativeObject memory(cx, NewNativeObjectWithGivenProto(cx, &class_, memoryProto));
    if (!memory)
        return nullptr;

    dbg->object->setReservedSlot(Debugger::JSSLOT_DEBUG_MEMORY_INSTANCE, ObjectValue(*memory));
    memory->setReservedSlot(JSSLOT_DEBUGGER, ObjectValue(*dbg->object));

    return &memory->as<DebuggerMemory>();
}

Debugger*
DebuggerMemory::getDebugger()
{
    const Value& dbgVal = getReservedSlot(JSSLOT_DEBUGGER);
    return Debugger::fromJSObject(&dbgVal.toObject());
}

 bool
DebuggerMemory::construct(JSContext* cx, unsigned argc, Value* vp)
{
    JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_NO_CONSTRUCTOR,
                         "Debugger.Memory");
    return false;
}

 const Class DebuggerMemory::class_ = {
    "Memory",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(JSSLOT_COUNT)
};

 DebuggerMemory*
DebuggerMemory::checkThis(JSContext* cx, CallArgs& args, const char* fnName)
{
    const Value& thisValue = args.thisv();

    if (!thisValue.isObject()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_NOT_NONNULL_OBJECT, InformalValueTypeName(thisValue));
        return nullptr;
    }

    JSObject& thisObject = thisValue.toObject();
    if (!thisObject.is<DebuggerMemory>()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             class_.name, fnName, thisObject.getClass()->name);
        return nullptr;
    }

    
    
    
    
    if (thisObject.as<DebuggerMemory>().getReservedSlot(JSSLOT_DEBUGGER).isUndefined()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             class_.name, fnName, "prototype object");
        return nullptr;
    }

    return &thisObject.as<DebuggerMemory>();
}














#define THIS_DEBUGGER_MEMORY(cx, argc, vp, fnName, args, memory)        \
    CallArgs args = CallArgsFromVp(argc, vp);                           \
    Rooted<DebuggerMemory*> memory(cx, checkThis(cx, args, fnName));    \
    if (!memory)                                                        \
        return false

static bool
undefined(CallArgs& args)
{
    args.rval().setUndefined();
    return true;
}

 bool
DebuggerMemory::setTrackingAllocationSites(JSContext* cx, unsigned argc, Value* vp)
{
    THIS_DEBUGGER_MEMORY(cx, argc, vp, "(set trackingAllocationSites)", args, memory);
    if (!args.requireAtLeast(cx, "(set trackingAllocationSites)", 1))
        return false;

    Debugger* dbg = memory->getDebugger();
    bool enabling = ToBoolean(args[0]);

    if (enabling == dbg->trackingAllocationSites)
        return undefined(args);

    dbg->trackingAllocationSites = enabling;

    if (!dbg->enabled)
        return undefined(args);

    if (enabling) {
        if (!dbg->addAllocationsTrackingForAllDebuggees(cx)) {
            dbg->trackingAllocationSites = false;
            return false;
        }
    } else {
        dbg->removeAllocationsTrackingForAllDebuggees();
    }

    return undefined(args);
}

 bool
DebuggerMemory::getTrackingAllocationSites(JSContext* cx, unsigned argc, Value* vp)
{
    THIS_DEBUGGER_MEMORY(cx, argc, vp, "(get trackingAllocationSites)", args, memory);
    args.rval().setBoolean(memory->getDebugger()->trackingAllocationSites);
    return true;
}

 bool
DebuggerMemory::drainAllocationsLog(JSContext* cx, unsigned argc, Value* vp)
{
    THIS_DEBUGGER_MEMORY(cx, argc, vp, "drainAllocationsLog", args, memory);
    Debugger* dbg = memory->getDebugger();

    if (!dbg->trackingAllocationSites) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_NOT_TRACKING_ALLOCATIONS,
                             "drainAllocationsLog");
        return false;
    }

    size_t length = dbg->allocationsLogLength;

    RootedArrayObject result(cx, NewDenseFullyAllocatedArray(cx, length));
    if (!result)
        return false;
    result->ensureDenseInitializedLength(cx, 0, length);

    for (size_t i = 0; i < length; i++) {
        RootedPlainObject obj(cx, NewBuiltinClassInstance<PlainObject>(cx));
        if (!obj)
            return false;

        
        
        
        
        Debugger::AllocationSite* allocSite = dbg->allocationsLog.getFirst();
        RootedValue frame(cx, ObjectOrNullValue(allocSite->frame));
        if (!DefineProperty(cx, obj, cx->names().frame, frame))
            return false;

        RootedValue timestampValue(cx, NumberValue(allocSite->when));
        if (!DefineProperty(cx, obj, cx->names().timestamp, timestampValue))
            return false;

        RootedString className(cx, Atomize(cx, allocSite->className, strlen(allocSite->className)));
        if (!className)
            return false;
        RootedValue classNameValue(cx, StringValue(className));
        if (!DefineProperty(cx, obj, cx->names().class_, classNameValue))
            return false;

        result->setDenseElement(i, ObjectValue(*obj));

        
        
        
        MOZ_ALWAYS_TRUE(dbg->allocationsLog.popFirst() == allocSite);
        js_delete(allocSite);
    }

    dbg->allocationsLogOverflowed = false;
    dbg->allocationsLogLength = 0;
    args.rval().setObject(*result);
    return true;
}

 bool
DebuggerMemory::getMaxAllocationsLogLength(JSContext* cx, unsigned argc, Value* vp)
{
    THIS_DEBUGGER_MEMORY(cx, argc, vp, "(get maxAllocationsLogLength)", args, memory);
    args.rval().setInt32(memory->getDebugger()->maxAllocationsLogLength);
    return true;
}

 bool
DebuggerMemory::setMaxAllocationsLogLength(JSContext* cx, unsigned argc, Value* vp)
{
    THIS_DEBUGGER_MEMORY(cx, argc, vp, "(set maxAllocationsLogLength)", args, memory);
    if (!args.requireAtLeast(cx, "(set maxAllocationsLogLength)", 1))
        return false;

    int32_t max;
    if (!ToInt32(cx, args[0], &max))
        return false;

    if (max < 1) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_UNEXPECTED_TYPE,
                             "(set maxAllocationsLogLength)'s parameter",
                             "not a positive integer");
        return false;
    }

    Debugger* dbg = memory->getDebugger();
    dbg->maxAllocationsLogLength = max;

    while (dbg->allocationsLogLength > dbg->maxAllocationsLogLength) {
        js_delete(dbg->allocationsLog.getFirst());
        dbg->allocationsLogLength--;
    }

    args.rval().setUndefined();
    return true;
}

 bool
DebuggerMemory::getAllocationSamplingProbability(JSContext* cx, unsigned argc, Value* vp)
{
    THIS_DEBUGGER_MEMORY(cx, argc, vp, "(get allocationSamplingProbability)", args, memory);
    args.rval().setDouble(memory->getDebugger()->allocationSamplingProbability);
    return true;
}

 bool
DebuggerMemory::setAllocationSamplingProbability(JSContext* cx, unsigned argc, Value* vp)
{
    THIS_DEBUGGER_MEMORY(cx, argc, vp, "(set allocationSamplingProbability)", args, memory);
    if (!args.requireAtLeast(cx, "(set allocationSamplingProbability)", 1))
        return false;

    double probability;
    if (!ToNumber(cx, args[0], &probability))
        return false;

    if (probability < 0.0 || probability > 1.0) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_UNEXPECTED_TYPE,
                             "(set allocationSamplingProbability)'s parameter",
                             "not a number between 0 and 1");
        return false;
    }

    memory->getDebugger()->allocationSamplingProbability = probability;
    args.rval().setUndefined();
    return true;
}

 bool
DebuggerMemory::getAllocationsLogOverflowed(JSContext* cx, unsigned argc, Value* vp)
{
    THIS_DEBUGGER_MEMORY(cx, argc, vp, "(get allocationsLogOverflowed)", args, memory);
    args.rval().setBoolean(memory->getDebugger()->allocationsLogOverflowed);
    return true;
}

 bool
DebuggerMemory::getOnGarbageCollection(JSContext* cx, unsigned argc, Value* vp)
{
    THIS_DEBUGGER_MEMORY(cx, argc, vp, "(get onGarbageCollection)", args, memory);
    return Debugger::getHookImpl(cx, args, *memory->getDebugger(), Debugger::OnGarbageCollection);
}

 bool
DebuggerMemory::setOnGarbageCollection(JSContext* cx, unsigned argc, Value* vp)
{
    THIS_DEBUGGER_MEMORY(cx, argc, vp, "(set onGarbageCollection)", args, memory);
    return Debugger::setHookImpl(cx, args, *memory->getDebugger(), Debugger::OnGarbageCollection);
}




JS_PUBLIC_API(void)
JS::dbg::SetDebuggerMallocSizeOf(JSRuntime* rt, mozilla::MallocSizeOf mallocSizeOf)
{
    rt->debuggerMallocSizeOf = mallocSizeOf;
}

JS_PUBLIC_API(mozilla::MallocSizeOf)
JS::dbg::GetDebuggerMallocSizeOf(JSRuntime* rt)
{
    return rt->debuggerMallocSizeOf;
}

namespace js {
namespace dbg {


struct Census {
    JSContext * const cx;
    JS::ZoneSet debuggeeZones;
    Zone* atomsZone;

    explicit Census(JSContext* cx) : cx(cx), atomsZone(nullptr) { }

    bool init() {
        AutoLockForExclusiveAccess lock(cx);
        atomsZone = cx->runtime()->atomsCompartment()->zone();
        return debuggeeZones.init();
    }
};




























class Tally {
    size_t total_;

  public:
    explicit Tally(Census& census) : total_(0) { }
    Tally(Tally&& rhs) : total_(rhs.total_) { }
    Tally& operator=(Tally&& rhs) { total_ = rhs.total_; return *this; }

    bool init(Census& census) { return true; }

    bool count(Census& census, const Node& node) {
        total_++;
        return true;
    }

    size_t total() const { return total_; }

    bool report(Census& census, MutableHandleValue report) {
        RootedPlainObject obj(census.cx, NewBuiltinClassInstance<PlainObject>(census.cx));
        RootedValue countValue(census.cx, NumberValue(total_));
        if (!obj ||
            !DefineProperty(census.cx, obj, census.cx->names().count, countValue))
        {
            return false;
        }
        report.setObject(*obj);
        return true;
    }
};







template<typename EachObject = Tally,
         typename EachScript = Tally,
         typename EachString = Tally,
         typename EachOther  = Tally>
class ByJSType {
    size_t total_;
    EachObject objects;
    EachScript scripts;
    EachString strings;
    EachOther other;

  public:
    explicit ByJSType(Census& census)
      : total_(0),
        objects(census),
        scripts(census),
        strings(census),
        other(census)
    { }
    ByJSType(ByJSType&& rhs)
      : total_(rhs.total_),
        objects(Move(rhs.objects)),
        scripts(move(rhs.scripts)),
        strings(move(rhs.strings)),
        other(move(rhs.other))
    { }
    ByJSType& operator=(ByJSType&& rhs) {
        MOZ_ASSERT(&rhs != this);
        this->~ByJSType();
        new (this) ByJSType(Move(rhs));
        return *this;
    }

    bool init(Census& census) {
        return objects.init(census) &&
               scripts.init(census) &&
               strings.init(census) &&
               other.init(census);
    }

    bool count(Census& census, const Node& node) {
        total_++;
        if (node.is<JSObject>())
            return objects.count(census, node);
         if (node.is<JSScript>() || node.is<LazyScript>() || node.is<jit::JitCode>())
            return scripts.count(census, node);
        if (node.is<JSString>())
            return strings.count(census, node);
        return other.count(census, node);
    }

    bool report(Census& census, MutableHandleValue report) {
        JSContext* cx = census.cx;

        RootedPlainObject obj(cx, NewBuiltinClassInstance<PlainObject>(cx));
        if (!obj)
            return false;

        RootedValue objectsReport(cx);
        if (!objects.report(census, &objectsReport) ||
            !DefineProperty(cx, obj, cx->names().objects, objectsReport))
            return false;

        RootedValue scriptsReport(cx);
        if (!scripts.report(census, &scriptsReport) ||
            !DefineProperty(cx, obj, cx->names().scripts, scriptsReport))
            return false;

        RootedValue stringsReport(cx);
        if (!strings.report(census, &stringsReport) ||
            !DefineProperty(cx, obj, cx->names().strings, stringsReport))
            return false;

        RootedValue otherReport(cx);
        if (!other.report(census, &otherReport) ||
            !DefineProperty(cx, obj, cx->names().other, otherReport))
            return false;

        report.setObject(*obj);
        return true;
    }
};





template<typename EachClass = Tally,
         typename EachOther = Tally>
class ByObjectClass {
    size_t total_;

    
    struct HashPolicy {
        typedef const char* Lookup;
        static js::HashNumber hash(Lookup l) { return mozilla::HashString(l); }
        static bool match(const char* key, Lookup lookup) {
            return strcmp(key, lookup) == 0;
        }
    };

    
    
    
    
    
    typedef HashMap<const char*, EachClass, HashPolicy, SystemAllocPolicy> Table;
    typedef typename Table::Entry Entry;
    Table table;
    EachOther other;

    static int compareEntries(const void* lhsVoid, const void* rhsVoid) {
        size_t lhs = (*static_cast<const Entry * const*>(lhsVoid))->value().total();
        size_t rhs = (*static_cast<const Entry * const*>(rhsVoid))->value().total();

        
        
        
        if (lhs < rhs)
            return 1;
        if (lhs > rhs)
            return -1;
        return 0;
    }

  public:
    explicit ByObjectClass(Census& census) : total_(0), other(census) { }
    ByObjectClass(ByObjectClass&& rhs)
      : total_(rhs.total_), table(Move(rhs.table)), other(Move(rhs.other))
    { }
    ByObjectClass& operator=(ByObjectClass&& rhs) {
        MOZ_ASSERT(&rhs != this);
        this->~ByObjectClass();
        new (this) ByObjectClass(Move(rhs));
        return *this;
    }

    bool init(Census& census) { return table.init() && other.init(census); }

    bool count(Census& census, const Node& node) {
        total_++;
        const char* className = node.jsObjectClassName();

        if (!className)
            return other.count(census, node);

        typename Table::AddPtr p = table.lookupForAdd(className);
        if (!p) {
            if (!table.add(p, className, EachClass(census)))
                return false;
            if (!p->value().init(census))
                return false;
        }
        return p->value().count(census, node);
    }

    size_t total() const { return total_; }

    bool report(Census& census, MutableHandleValue report) {
        JSContext* cx = census.cx;

        
        
        
        mozilla::Vector<Entry*> entries;
        if (!entries.reserve(table.count()))
            return false;
        for (typename Table::Range r = table.all(); !r.empty(); r.popFront())
            entries.infallibleAppend(&r.front());
        qsort(entries.begin(), entries.length(), sizeof(*entries.begin()), compareEntries);

        
        RootedPlainObject obj(cx, NewBuiltinClassInstance<PlainObject>(cx));
        if (!obj)
            return false;
        for (Entry** entryPtr = entries.begin(); entryPtr < entries.end(); entryPtr++) {
            Entry& entry = **entryPtr;
            EachClass& assorter = entry.value();
            RootedValue assorterReport(cx);
            if (!assorter.report(census, &assorterReport))
                return false;

            const char* name = entry.key();
            MOZ_ASSERT(name);
            JSAtom* atom = Atomize(census.cx, name, strlen(name));
            if (!atom)
                return false;
            RootedId entryId(cx, AtomToId(atom));

#ifdef DEBUG
            
            
            
            
            bool has;
            if (!HasOwnProperty(cx, obj, entryId, &has))
                return false;
            if (has) {
                fprintf(stderr, "already has own property '%s'\n", name);
                MOZ_ASSERT(!has);
            }
#endif

            if (!DefineProperty(cx, obj, entryId, assorterReport))
                return false;
        }

        report.setObject(*obj);
        return true;
    }
};



template<typename EachType = Tally>
class ByUbinodeType {
    size_t total_;

    
    
    
    typedef HashMap<const char16_t*, EachType, DefaultHasher<const char16_t*>,
                    SystemAllocPolicy> Table;
    typedef typename Table::Entry Entry;
    Table table;

  public:
    explicit ByUbinodeType(Census& census) : total_(0) { }
    ByUbinodeType(ByUbinodeType&& rhs) : total_(rhs.total_), table(Move(rhs.table)) { }
    ByUbinodeType& operator=(ByUbinodeType&& rhs) {
        MOZ_ASSERT(&rhs != this);
        this->~ByUbinodeType();
        new (this) ByUbinodeType(Move(rhs));
        return *this;
    }

    bool init(Census& census) { return table.init(); }

    bool count(Census& census, const Node& node) {
        total_++;
        const char16_t* key = node.typeName();
        typename Table::AddPtr p = table.lookupForAdd(key);
        if (!p) {
            if (!table.add(p, key, EachType(census)))
                return false;
            if (!p->value().init(census))
                return false;
        }
        return p->value().count(census, node);
    }

    size_t total() const { return total_; }

    static int compareEntries(const void* lhsVoid, const void* rhsVoid) {
        size_t lhs = (*static_cast<const Entry * const*>(lhsVoid))->value().total();
        size_t rhs = (*static_cast<const Entry * const*>(rhsVoid))->value().total();

        
        
        
        if (lhs < rhs)
            return 1;
        if (lhs > rhs)
            return -1;
        return 0;
    }

    bool report(Census& census, MutableHandleValue report) {
        JSContext* cx = census.cx;

        
        
        
        mozilla::Vector<Entry*> entries;
        if (!entries.reserve(table.count()))
            return false;
        for (typename Table::Range r = table.all(); !r.empty(); r.popFront())
            entries.infallibleAppend(&r.front());
        qsort(entries.begin(), entries.length(), sizeof(*entries.begin()), compareEntries);

        
        RootedPlainObject obj(cx, NewBuiltinClassInstance<PlainObject>(cx));
        if (!obj)
            return false;
        for (Entry** entryPtr = entries.begin(); entryPtr < entries.end(); entryPtr++) {
            Entry& entry = **entryPtr;
            EachType& assorter = entry.value();
            RootedValue assorterReport(cx);
            if (!assorter.report(census, &assorterReport))
                return false;

            const char16_t* name = entry.key();
            MOZ_ASSERT(name);
            JSAtom* atom = AtomizeChars(cx, name, js_strlen(name));
            if (!atom)
                return false;
            RootedId entryId(cx, AtomToId(atom));

            if (!DefineProperty(cx, obj, entryId, assorterReport))
                return false;
        }

        report.setObject(*obj);
        return true;
    }
};




template<typename Assorter>
class CensusHandler {
    Census& census;
    Assorter assorter;

  public:
    explicit CensusHandler(Census& census) : census(census), assorter(census) { }

    bool init(Census& census) { return assorter.init(census); }
    bool report(Census& census, MutableHandleValue report) {
        return assorter.report(census, report);
    }

    
    class NodeData { };

    bool operator() (BreadthFirst<CensusHandler>& traversal,
                     Node origin, const Edge& edge,
                     NodeData* referentData, bool first)
    {
        
        
        if (!first)
            return true;

        
        
        
        
        
        
        const Node& referent = edge.referent;
        Zone* zone = referent.zone();

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





typedef ByJSType<ByObjectClass<Tally>, Tally, Tally, ByUbinodeType<Tally> > DefaultAssorter;


typedef CensusHandler<DefaultAssorter> DefaultCensusHandler;
typedef BreadthFirst<DefaultCensusHandler> DefaultCensusTraversal;

} 
} 

bool
DebuggerMemory::takeCensus(JSContext* cx, unsigned argc, Value* vp)
{
    THIS_DEBUGGER_MEMORY(cx, argc, vp, "Debugger.Memory.prototype.census", args, memory);

    dbg::Census census(cx);
    if (!census.init())
        return false;

    dbg::DefaultCensusHandler handler(census);
    if (!handler.init(census))
        return false;

    Debugger* dbg = memory->getDebugger();
    RootedObject dbgObj(cx, dbg->object);

    
    for (WeakGlobalObjectSet::Range r = dbg->allDebuggees(); !r.empty(); r.popFront()) {
        if (!census.debuggeeZones.put(r.front()->zone()))
            return false;
    }

    {
        Maybe<JS::AutoCheckCannotGC> maybeNoGC;
        JS::ubi::RootList rootList(cx, maybeNoGC);
        if (!rootList.init(dbgObj))
            return false;

        dbg::DefaultCensusTraversal traversal(cx, handler, maybeNoGC.ref());
        if (!traversal.init())
            return false;
        traversal.wantNames = false;

        if (!traversal.addStart(JS::ubi::Node(&rootList)) ||
            !traversal.traverse())
        {
            return false;
        }
    }

    return handler.report(census, args.rval());
}






 const JSPropertySpec DebuggerMemory::properties[] = {
    JS_PSGS("trackingAllocationSites", getTrackingAllocationSites, setTrackingAllocationSites, 0),
    JS_PSGS("maxAllocationsLogLength", getMaxAllocationsLogLength, setMaxAllocationsLogLength, 0),
    JS_PSGS("allocationSamplingProbability", getAllocationSamplingProbability, setAllocationSamplingProbability, 0),
    JS_PSG("allocationsLogOverflowed", getAllocationsLogOverflowed, 0),
    JS_PSGS("onGarbageCollection", getOnGarbageCollection, setOnGarbageCollection, 0),
    JS_PS_END
};

 const JSFunctionSpec DebuggerMemory::methods[] = {
    JS_FN("drainAllocationsLog", DebuggerMemory::drainAllocationsLog, 0, 0),
    JS_FN("takeCensus", takeCensus, 0, 0),
    JS_FS_END
};
