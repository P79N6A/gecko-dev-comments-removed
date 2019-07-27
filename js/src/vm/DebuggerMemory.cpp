





#include "vm/DebuggerMemory.h"

#include "mozilla/Maybe.h"
#include "mozilla/Move.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/Vector.h"

#include <stdlib.h>

#include "jsalloc.h"
#include "jscompartment.h"

#include "builtin/MapObject.h"
#include "gc/Marking.h"
#include "js/Debug.h"
#include "js/TracingAPI.h"
#include "js/UbiNode.h"
#include "js/UbiNodeTraverse.h"
#include "js/Utility.h"
#include "vm/Debugger.h"
#include "vm/GlobalObject.h"
#include "vm/SavedStacks.h"

#include "vm/Debugger-inl.h"
#include "vm/NativeObject-inl.h"

using namespace js;

using JS::ubi::BreadthFirst;
using JS::ubi::Edge;
using JS::ubi::Node;

using mozilla::Forward;
using mozilla::Maybe;
using mozilla::Move;
using mozilla::Nothing;
using mozilla::UniquePtr;

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

        RootedValue ctorName(cx, NullValue());
        if (allocSite->ctorName)
            ctorName.setString(allocSite->ctorName);
        if (!DefineProperty(cx, obj, cx->names().constructor, ctorName))
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
    JSContext* const cx;
    JS::ZoneSet debuggeeZones;
    Zone* atomsZone;

    explicit Census(JSContext* cx) : cx(cx), atomsZone(nullptr) { }

    bool init() {
        AutoLockForExclusiveAccess lock(cx);
        atomsZone = cx->runtime()->atomsCompartment()->zone();
        return debuggeeZones.init();
    }

    
    
    
    template<typename T, typename... Args>
    T* new_(Args&&... args) MOZ_HEAP_ALLOCATOR {
        void* memory = js_malloc(sizeof(T));
        if (MOZ_UNLIKELY(!memory)) {
            memory = static_cast<T*>(cx->onOutOfMemory(AllocFunction::Malloc, sizeof(T)));
            if (!memory)
                return nullptr;
        }
        return new(memory) T(Forward<Args>(args)...);
    }
};

class CountBase;

struct CountDeleter {
    void operator()(CountBase*);
};

typedef UniquePtr<CountBase, CountDeleter> CountBasePtr;


struct CountType {
    explicit CountType(Census& census) : census(census) { }
    virtual ~CountType() { }

    
    
    virtual CountBasePtr makeCount() = 0;

    
    virtual void traceCount(CountBase& count, JSTracer* trc) = 0;

    
    virtual void destructCount(CountBase& count) = 0;

    
    
    virtual bool count(CountBase& count, const Node& node) = 0;

    
    
    virtual bool report(CountBase& count, MutableHandleValue report) = 0;

  protected:
    Census& census;
};

typedef UniquePtr<CountType, JS::DeletePolicy<CountType>> CountTypePtr;


class CountBase {
    
    
    
    
    CountType& type;

  protected:
    ~CountBase() { }

  public:
    explicit CountBase(CountType& type) : type(type), total_(0) { }

    
    bool count(const Node& node) { return type.count(*this, node); }

    
    
    
    bool report(MutableHandleValue report) { return type.report(*this, report); }

    
    
    void destruct() { return type.destructCount(*this); }

    
    void trace(JSTracer* trc) { type.traceCount(*this, trc); }

    size_t total_;
};

class RootedCount : JS::CustomAutoRooter {
    CountBasePtr count;

    void trace(JSTracer* trc) override { count->trace(trc); }

  public:
    RootedCount(JSContext* cx, CountBasePtr&& count)
        : CustomAutoRooter(cx),
          count(Move(count))
          { }
    CountBase* operator->() const { return count.get(); }
    explicit operator bool() const { return count.get(); }
    operator CountBasePtr&() { return count; }
};

void
CountDeleter::operator()(CountBase* ptr)
{
    if (!ptr)
        return;

    
    
    ptr->destruct();
    js_free(ptr);
}


class SimpleCount : public CountType {

    struct Count : CountBase {
        size_t totalBytes_;

        explicit Count(SimpleCount& count)
          : CountBase(count),
            totalBytes_(0)
        { }
    };

    UniquePtr<char16_t[], JS::FreePolicy> label;
    bool reportCount : 1;
    bool reportBytes : 1;

  public:
    SimpleCount(Census& census,
                UniquePtr<char16_t[], JS::FreePolicy>& label,
                bool reportCount=true,
                bool reportBytes=true)
      : CountType(census),
        label(Move(label)),
        reportCount(reportCount),
        reportBytes(reportBytes)
    { }

    explicit SimpleCount(Census& census)
        : CountType(census),
          label(nullptr),
          reportCount(true),
          reportBytes(true)
    { }

    CountBasePtr makeCount() override {
        return CountBasePtr(census.new_<Count>(*this));
    }

    void traceCount(CountBase& countBase, JSTracer* trc) override { }

    void destructCount(CountBase& countBase) override {
        Count& count = static_cast<Count&>(countBase);
        count.~Count();
    }

    bool count(CountBase& countBase, const Node& node) override {
        Count& count = static_cast<Count&>(countBase);
        count.total_++;
        if (reportBytes)
            count.totalBytes_ += node.size(census.cx->runtime()->debuggerMallocSizeOf);
        return true;
    }

    bool report(CountBase& countBase, MutableHandleValue report) override {
        Count& count = static_cast<Count&>(countBase);

        RootedPlainObject obj(census.cx, NewBuiltinClassInstance<PlainObject>(census.cx));
        if (!obj)
            return false;

        RootedValue countValue(census.cx, NumberValue(count.total_));
        if (reportCount && !DefineProperty(census.cx, obj, census.cx->names().count, countValue))
            return false;

        RootedValue bytesValue(census.cx, NumberValue(count.totalBytes_));
        if (reportBytes && !DefineProperty(census.cx, obj, census.cx->names().bytes, bytesValue))
            return false;

        if (label) {
            JSString* labelString = JS_NewUCStringCopyZ(census.cx, label.get());
            if (!labelString)
                return false;
            RootedValue labelValue(census.cx, StringValue(labelString));
            if (!DefineProperty(census.cx, obj, census.cx->names().label, labelValue))
                return false;
        }

        report.setObject(*obj);
        return true;
    }
};







class ByCoarseType : public CountType {
    CountTypePtr objects;
    CountTypePtr scripts;
    CountTypePtr strings;
    CountTypePtr other;

    struct Count : CountBase {
        Count(CountType& type,
              CountBasePtr& objects,
              CountBasePtr& scripts,
              CountBasePtr& strings,
              CountBasePtr& other)
          : CountBase(type),
            objects(Move(objects)),
            scripts(Move(scripts)),
            strings(Move(strings)),
            other(Move(other))
        { }

        CountBasePtr objects;
        CountBasePtr scripts;
        CountBasePtr strings;
        CountBasePtr other;
    };

  public:
    ByCoarseType(Census& census,
                 CountTypePtr& objects,
                 CountTypePtr& scripts,
                 CountTypePtr& strings,
                 CountTypePtr& other)
      : CountType(census),
        objects(Move(objects)),
        scripts(Move(scripts)),
        strings(Move(strings)),
        other(Move(other))
    { }

    CountBasePtr makeCount() override {
        CountBasePtr objectsCount(objects->makeCount());
        CountBasePtr scriptsCount(scripts->makeCount());
        CountBasePtr stringsCount(strings->makeCount());
        CountBasePtr otherCount(other->makeCount());

        if (!objectsCount || !scriptsCount || !stringsCount || !otherCount)
            return CountBasePtr(nullptr);

        return CountBasePtr(census.new_<Count>(*this,
                                               objectsCount,
                                               scriptsCount,
                                               stringsCount,
                                               otherCount));
    }

    void traceCount(CountBase& countBase, JSTracer* trc) override {
        Count& count = static_cast<Count&>(countBase);
        count.objects->trace(trc);
        count.scripts->trace(trc);
        count.strings->trace(trc);
        count.other->trace(trc);
    }

    void destructCount(CountBase& countBase) override {
        Count& count = static_cast<Count&>(countBase);
        count.~Count();
    }

    bool count(CountBase& countBase, const Node& node) override {
        Count& count = static_cast<Count&>(countBase);
        count.total_++;

        if (node.is<JSObject>())
            return count.objects->count(node);
        if (node.is<JSScript>() || node.is<LazyScript>() || node.is<jit::JitCode>())
            return count.scripts->count(node);
        if (node.is<JSString>())
            return count.strings->count(node);
        return count.other->count(node);
    }

    bool report(CountBase& countBase, MutableHandleValue report) override {
        Count& count = static_cast<Count&>(countBase);
        JSContext* cx = census.cx;

        RootedPlainObject obj(cx, NewBuiltinClassInstance<PlainObject>(cx));
        if (!obj)
            return false;

        RootedValue objectsReport(cx);
        if (!count.objects->report(&objectsReport) ||
            !DefineProperty(cx, obj, cx->names().objects, objectsReport))
            return false;

        RootedValue scriptsReport(cx);
        if (!count.scripts->report(&scriptsReport) ||
            !DefineProperty(cx, obj, cx->names().scripts, scriptsReport))
            return false;

        RootedValue stringsReport(cx);
        if (!count.strings->report(&stringsReport) ||
            !DefineProperty(cx, obj, cx->names().strings, stringsReport))
            return false;

        RootedValue otherReport(cx);
        if (!count.other->report(&otherReport) ||
            !DefineProperty(cx, obj, cx->names().other, otherReport))
            return false;

        report.setObject(*obj);
        return true;
    }
};




template<typename Entry>
static int compareEntries(const void* lhsVoid, const void* rhsVoid) {
    size_t lhs = (*static_cast<const Entry* const*>(lhsVoid))->value()->total_;
    size_t rhs = (*static_cast<const Entry* const*>(rhsVoid))->value()->total_;

    
    
    
    if (lhs < rhs)
        return 1;
    if (lhs > rhs)
        return -1;
    return 0;
}



class ByObjectClass : public CountType {
    
    struct HashPolicy {
        typedef const char* Lookup;
        static js::HashNumber hash(Lookup l) { return mozilla::HashString(l); }
        static bool match(const char* key, Lookup lookup) {
            return strcmp(key, lookup) == 0;
        }
    };

    
    
    
    
    
    typedef HashMap<const char*, CountBasePtr, HashPolicy, SystemAllocPolicy> Table;
    typedef Table::Entry Entry;

    struct Count : public CountBase {
        Table table;
        CountBasePtr other;

        Count(CountType& type, CountBasePtr& other)
          : CountBase(type),
            other(Move(other))
        { }

        bool init() { return table.init(); }
    };

    CountTypePtr classesType;
    CountTypePtr otherType;

  public:
    ByObjectClass(Census& census,
                  CountTypePtr& classesType,
                  CountTypePtr& otherType)
        : CountType(census),
          classesType(Move(classesType)),
          otherType(Move(otherType))
    { }

    CountBasePtr makeCount() override {
        CountBasePtr otherCount(otherType->makeCount());
        if (!otherCount)
            return nullptr;

        UniquePtr<Count> count(census.new_<Count>(*this, otherCount));
        if (!count || !count->init())
            return nullptr;

        return CountBasePtr(count.release());
    }

    void traceCount(CountBase& countBase, JSTracer* trc) override {
        Count& count = static_cast<Count&>(countBase);
        for (Table::Range r = count.table.all(); !r.empty(); r.popFront())
            r.front().value()->trace(trc);
        count.other->trace(trc);
    }

    void destructCount(CountBase& countBase) override {
        Count& count = static_cast<Count&>(countBase);
        count.~Count();
    }

    bool count(CountBase& countBase, const Node& node) override {
        Count& count = static_cast<Count&>(countBase);
        count.total_++;

        const char* className = node.jsObjectClassName();
        if (!className)
            return count.other->count(node);

        Table::AddPtr p = count.table.lookupForAdd(className);
        if (!p) {
            CountBasePtr classCount(classesType->makeCount());
            if (!classCount || !count.table.add(p, className, Move(classCount)))
                return false;
        }
        return p->value()->count(node);
    }

    bool report(CountBase& countBase, MutableHandleValue report) override {
        Count& count = static_cast<Count&>(countBase);
        JSContext* cx = census.cx;

        
        
        
        mozilla::Vector<Entry*> entries;
        if (!entries.reserve(count.table.count()))
            return false;
        for (Table::Range r = count.table.all(); !r.empty(); r.popFront())
            entries.infallibleAppend(&r.front());
        qsort(entries.begin(), entries.length(), sizeof(*entries.begin()), compareEntries<Entry>);

        
        RootedPlainObject obj(cx, NewBuiltinClassInstance<PlainObject>(cx));
        if (!obj)
            return false;
        for (Entry** entryPtr = entries.begin(); entryPtr < entries.end(); entryPtr++) {
            Entry& entry = **entryPtr;
            CountBasePtr& classCount = entry.value();
            RootedValue classReport(cx);
            if (!classCount->report(&classReport))
                return false;

            const char* name = entry.key();
            MOZ_ASSERT(name);
            JSAtom* atom = Atomize(cx, name, strlen(name));
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

            if (!DefineProperty(cx, obj, entryId, classReport))
                return false;
        }

        RootedValue otherReport(cx);
        if (!count.other->report(&otherReport) ||
            !DefineProperty(cx, obj, cx->names().other, otherReport))
            return false;

        report.setObject(*obj);
        return true;
    }
};



class ByUbinodeType : public CountType {
    
    
    
    typedef HashMap<const char16_t*, CountBasePtr, DefaultHasher<const char16_t*>,
                    SystemAllocPolicy> Table;
    typedef Table::Entry Entry;

    struct Count: public CountBase {
        Table table;

        explicit Count(CountType& type) : CountBase(type) { }

        bool init() { return table.init(); }
    };

    CountTypePtr entryType;

  public:
    ByUbinodeType(Census& census, CountTypePtr& entryType)
      : CountType(census),
        entryType(Move(entryType))
    { }

    CountBasePtr makeCount() override {
        UniquePtr<Count> count(census.new_<Count>(*this));
        if (!count || !count->init())
            return nullptr;

        return CountBasePtr(count.release());
    }

    void traceCount(CountBase& countBase, JSTracer* trc) override {
        Count& count = static_cast<Count&>(countBase);
        for (Table::Range r = count.table.all(); !r.empty(); r.popFront())
            r.front().value()->trace(trc);
    }

    void destructCount(CountBase& countBase) override {
        Count& count = static_cast<Count&>(countBase);
        count.~Count();
    }

    bool count(CountBase& countBase, const Node& node) {
        Count& count = static_cast<Count&>(countBase);
        count.total_++;

        const char16_t* key = node.typeName();
        Table::AddPtr p = count.table.lookupForAdd(key);
        if (!p) {
            CountBasePtr typesCount(entryType->makeCount());
            if (!typesCount || !count.table.add(p, key, Move(typesCount)))
                return false;
        }
        return p->value()->count(node);
    }

    bool report(CountBase& countBase, MutableHandleValue report) override {
        Count& count = static_cast<Count&>(countBase);
        JSContext* cx = census.cx;

        
        
        
        mozilla::Vector<Entry*> entries;
        if (!entries.reserve(count.table.count()))
            return false;
        for (Table::Range r = count.table.all(); !r.empty(); r.popFront())
            entries.infallibleAppend(&r.front());
        qsort(entries.begin(), entries.length(), sizeof(*entries.begin()), compareEntries<Entry>);

        
        RootedPlainObject obj(cx, NewBuiltinClassInstance<PlainObject>(cx));
        if (!obj)
            return false;
        for (Entry** entryPtr = entries.begin(); entryPtr < entries.end(); entryPtr++) {
            Entry& entry = **entryPtr;
            CountBasePtr& typeCount = entry.value();
            RootedValue typeReport(cx);
            if (!typeCount->report(&typeReport))
                return false;

            const char16_t* name = entry.key();
            MOZ_ASSERT(name);
            JSAtom* atom = AtomizeChars(cx, name, js_strlen(name));
            if (!atom)
                return false;
            RootedId entryId(cx, AtomToId(atom));

            if (!DefineProperty(cx, obj, entryId, typeReport))
                return false;
        }

        report.setObject(*obj);
        return true;
    }
};




class ByAllocationStack : public CountType {
    typedef HashMap<SavedFrame*, CountBasePtr, DefaultHasher<SavedFrame*>,
                    SystemAllocPolicy> Table;
    typedef Table::Entry Entry;

    struct Count : public CountBase {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        Table table;
        CountBasePtr noStack;

        Count(CountType& type, CountBasePtr& noStack)
          : CountBase(type),
            noStack(Move(noStack))
        { }
        bool init() { return table.init(); }
    };

    CountTypePtr entryType;
    CountTypePtr noStackType;

  public:
    ByAllocationStack(Census& census, CountTypePtr& entryType, CountTypePtr& noStackType)
      : CountType(census),
        entryType(Move(entryType)),
        noStackType(Move(noStackType))
    { }

    CountBasePtr makeCount() override {
        CountBasePtr noStackCount(noStackType->makeCount());
        if (!noStackCount)
            return nullptr;

        UniquePtr<Count> count(census.new_<Count>(*this, noStackCount));
        if (!count || !count->init())
            return nullptr;
        return CountBasePtr(count.release());
    }

    void traceCount(CountBase& countBase, JSTracer* trc) override {
        Count& count= static_cast<Count&>(countBase);
        for (Table::Range r = count.table.all(); !r.empty(); r.popFront()) {
            
            r.front().value()->trace(trc);

            
            
            SavedFrame** keyPtr = const_cast<SavedFrame**>(&r.front().key());
            TraceRoot(trc, keyPtr, "Debugger.Memory.prototype.census byAllocationStack count key");
        }
        count.noStack->trace(trc);
    }

    void destructCount(CountBase& countBase) override {
        Count& count = static_cast<Count&>(countBase);
        count.~Count();
    }

    bool count(CountBase& countBase, const Node& node) {
        Count& count = static_cast<Count&>(countBase);
        count.total_++;

        SavedFrame* allocationStack = nullptr;
        if (node.is<JSObject>()) {
            JSObject* metadata = GetObjectMetadata(node.as<JSObject>());
            if (metadata && metadata->is<SavedFrame>())
                allocationStack = &metadata->as<SavedFrame>();
        }
        
        

        
        
        if (allocationStack) {
            Table::AddPtr p = count.table.lookupForAdd(allocationStack);
            if (!p) {
                CountBasePtr stackCount(entryType->makeCount());
                if (!stackCount || !count.table.add(p, allocationStack, Move(stackCount)))
                    return false;
            }
            return p->value()->count(node);
        }

        
        return count.noStack->count(node);
    }

    bool report(CountBase& countBase, MutableHandleValue report) override {
        Count& count = static_cast<Count&>(countBase);
        JSContext* cx = census.cx;

#ifdef DEBUG
        
        uint32_t generation = count.table.generation();
#endif

        
        
        
        mozilla::Vector<Entry*> entries;
        if (!entries.reserve(count.table.count()))
            return false;
        for (Table::Range r = count.table.all(); !r.empty(); r.popFront())
            entries.infallibleAppend(&r.front());
        qsort(entries.begin(), entries.length(), sizeof(*entries.begin()), compareEntries<Entry>);

        
        Rooted<MapObject*> map(cx, MapObject::create(cx));
        if (!map)
            return false;
        for (Entry** entryPtr = entries.begin(); entryPtr < entries.end(); entryPtr++) {
            Entry& entry = **entryPtr;

            MOZ_ASSERT(entry.key());
            RootedValue stack(cx, ObjectValue(*entry.key()));
            if (!cx->compartment()->wrap(cx, &stack))
                return false;

            CountBasePtr& stackCount = entry.value();
            RootedValue stackReport(cx);
            if (!stackCount->report(&stackReport))
                return false;

            if (!MapObject::set(cx, map, stack, stackReport))
                return false;
        }

        if (count.noStack->total_ > 0) {
            RootedValue noStackReport(cx);
            if (!count.noStack->report(&noStackReport))
                return false;
            RootedValue noStack(cx, StringValue(cx->names().noStack));
            if (!MapObject::set(cx, map, noStack, noStackReport))
                return false;
        }

        MOZ_ASSERT(generation == count.table.generation());

        report.setObject(*map);
        return true;
    }
};




class CensusHandler {
    Census& census;
    CountBasePtr& rootCount;

  public:
    CensusHandler(Census& census, CountBasePtr& rootCount)
      : census(census),
        rootCount(rootCount)
    { }

    bool report(MutableHandleValue report) {
        return rootCount->report(report);
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
            return rootCount->count(referent);
        }

        if (zone == census.atomsZone) {
            traversal.abandonReferent();
            return rootCount->count(referent);
        }

        traversal.abandonReferent();
        return true;
    }
};

typedef BreadthFirst<CensusHandler> CensusTraversal;

static CountTypePtr ParseBreakdown(Census& census, HandleValue breakdownValue);

static CountTypePtr
ParseChildBreakdown(Census& census, HandleObject breakdown, PropertyName* prop)
{
    JSContext* cx = census.cx;
    RootedValue v(cx);
    if (!GetProperty(cx, breakdown, breakdown, prop, &v))
        return nullptr;
    return ParseBreakdown(census, v);
}

static CountTypePtr
ParseBreakdown(Census& census, HandleValue breakdownValue)
{
    JSContext* cx = census.cx;

    if (breakdownValue.isUndefined()) {
        
        CountTypePtr simple(census.new_<SimpleCount>(census));
        return simple;
    }

    RootedObject breakdown(cx, ToObject(cx, breakdownValue));
    if (!breakdown)
        return nullptr;

    RootedValue byValue(cx);
    if (!GetProperty(cx, breakdown, breakdown, cx->names().by, &byValue))
        return nullptr;
    RootedString byString(cx, ToString(cx, byValue));
    if (!byString)
        return nullptr;
    RootedLinearString by(cx, byString->ensureLinear(cx));
    if (!by)
        return nullptr;

    if (StringEqualsAscii(by, "count")) {
        RootedValue countValue(cx), bytesValue(cx);
        if (!GetProperty(cx, breakdown, breakdown, cx->names().count, &countValue) ||
            !GetProperty(cx, breakdown, breakdown, cx->names().bytes, &bytesValue))
            return nullptr;

        
        
        if (countValue.isUndefined()) countValue.setBoolean(true);
        if (bytesValue.isUndefined()) bytesValue.setBoolean(true);

        
        
        
        RootedValue label(cx);
        if (!GetProperty(cx, breakdown, breakdown, cx->names().label, &label))
            return nullptr;

        UniquePtr<char16_t[], JS::FreePolicy> labelUnique(nullptr);
        if (!label.isUndefined()) {
            RootedString labelString(cx, ToString(cx, label));
            if (!labelString)
                return nullptr;

            JSFlatString* flat = labelString->ensureFlat(cx);
            if (!flat)
                return nullptr;

            AutoStableStringChars chars(cx);
            if (!chars.initTwoByte(cx, flat))
                return nullptr;

            
            
            
            labelUnique = DuplicateString(cx, chars.twoByteChars());
            if (!labelUnique)
                return nullptr;
        }

        CountTypePtr simple(census.new_<SimpleCount>(census,
                                                     labelUnique,
                                                     ToBoolean(countValue),
                                                     ToBoolean(bytesValue)));
        return simple;
    }

    if (StringEqualsAscii(by, "objectClass")) {
        CountTypePtr thenType(ParseChildBreakdown(census, breakdown, cx->names().then));
        if (!thenType)
            return nullptr;

        CountTypePtr otherType(ParseChildBreakdown(census, breakdown, cx->names().other));
        if (!otherType)
            return nullptr;

        return CountTypePtr(census.new_<ByObjectClass>(census, thenType, otherType));
    }

    if (StringEqualsAscii(by, "coarseType")) {
        CountTypePtr objectsType(ParseChildBreakdown(census, breakdown, cx->names().objects));
        if (!objectsType)
            return nullptr;
        CountTypePtr scriptsType(ParseChildBreakdown(census, breakdown, cx->names().scripts));
        if (!scriptsType)
            return nullptr;
        CountTypePtr stringsType(ParseChildBreakdown(census, breakdown, cx->names().strings));
        if (!stringsType)
            return nullptr;
        CountTypePtr otherType(ParseChildBreakdown(census, breakdown, cx->names().other));
        if (!otherType)
            return nullptr;

        return CountTypePtr(census.new_<ByCoarseType>(census,
                                                      objectsType,
                                                      scriptsType,
                                                      stringsType,
                                                      otherType));
    }

    if (StringEqualsAscii(by, "internalType")) {
        CountTypePtr thenType(ParseChildBreakdown(census, breakdown, cx->names().then));
        if (!thenType)
            return nullptr;

        return CountTypePtr(census.new_<ByUbinodeType>(census, thenType));
    }

    if (StringEqualsAscii(by, "allocationStack")) {
        CountTypePtr thenType(ParseChildBreakdown(census, breakdown, cx->names().then));
        if (!thenType)
            return nullptr;
        CountTypePtr noStackType(ParseChildBreakdown(census, breakdown, cx->names().noStack));
        if (!noStackType)
            return nullptr;

        return CountTypePtr(census.new_<ByAllocationStack>(census, thenType, noStackType));
    }

    
    RootedString bySource(cx, ValueToSource(cx, byValue));
    if (!bySource)
        return nullptr;

    JSAutoByteString byBytes(cx, bySource);
    if (!byBytes)
        return nullptr;

    JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_DEBUG_CENSUS_BREAKDOWN,
                         byBytes.ptr());
    return nullptr;
}

} 
} 

using dbg::Census;
using dbg::CountTypePtr;
using dbg::CountBasePtr;

bool
DebuggerMemory::takeCensus(JSContext* cx, unsigned argc, Value* vp)
{
    THIS_DEBUGGER_MEMORY(cx, argc, vp, "Debugger.Memory.prototype.census", args, memory);

    Census census(cx);
    if (!census.init())
        return false;
    CountTypePtr rootType;

    RootedObject options(cx);
    if (args.get(0).isObject())
        options = &args[0].toObject();

    {
        RootedValue breakdown(cx, UndefinedValue());
        if (options && !GetProperty(cx, options, options, cx->names().breakdown, &breakdown))
            return false;
        if (!breakdown.isUndefined()) {
            rootType = ParseBreakdown(census, breakdown);
        } else {
            
            
            
            
            
            CountTypePtr byClass(census.new_<dbg::SimpleCount>(census));
            CountTypePtr byClassElse(census.new_<dbg::SimpleCount>(census));
            CountTypePtr objects(census.new_<dbg::ByObjectClass>(census,
                                                                 byClass,
                                                                 byClassElse));

            CountTypePtr scripts(census.new_<dbg::SimpleCount>(census));
            CountTypePtr strings(census.new_<dbg::SimpleCount>(census));

            CountTypePtr byType(census.new_<dbg::SimpleCount>(census));
            CountTypePtr other(census.new_<dbg::ByUbinodeType>(census, byType));

            rootType = CountTypePtr(census.new_<dbg::ByCoarseType>(census,
                                                                   objects,
                                                                   scripts,
                                                                   strings,
                                                                   other));
        }
    }

    if (!rootType)
        return false;

    dbg::RootedCount rootCount(cx, rootType->makeCount());
    if (!rootCount)
        return false;
    dbg::CensusHandler handler(census, rootCount);

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

        dbg::CensusTraversal traversal(cx, handler, maybeNoGC.ref());
        if (!traversal.init())
            return false;
        traversal.wantNames = false;

        if (!traversal.addStart(JS::ubi::Node(&rootList)) ||
            !traversal.traverse())
        {
            return false;
        }
    }

    return handler.report(args.rval());
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
