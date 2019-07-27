







#ifndef jsinferinlines_h
#define jsinferinlines_h

#include "jsinfer.h"

#include "mozilla/PodOperations.h"

#include "builtin/SymbolObject.h"
#include "jit/BaselineJIT.h"
#include "vm/ArrayObject.h"
#include "vm/BooleanObject.h"
#include "vm/NumberObject.h"
#include "vm/SharedArrayObject.h"
#include "vm/SharedTypedArrayObject.h"
#include "vm/StringObject.h"
#include "vm/TypedArrayObject.h"
#include "vm/UnboxedObject.h"

#include "jscntxtinlines.h"

namespace js {
namespace types {





inline jit::IonScript *
CompilerOutput::ion() const
{
    
    
    
    MOZ_ASSERT(isValid());
    jit::IonScript *ion = script()->maybeIonScript();
    MOZ_ASSERT(ion != ION_COMPILING_SCRIPT);
    return ion;
}

inline CompilerOutput*
RecompileInfo::compilerOutput(TypeZone &types) const
{
    if (generation != types.generation) {
        if (!types.sweepCompilerOutputs || outputIndex >= types.sweepCompilerOutputs->length())
            return nullptr;
        CompilerOutput *output = &(*types.sweepCompilerOutputs)[outputIndex];
        if (!output->isValid())
            return nullptr;
        output = &(*types.compilerOutputs)[output->sweepIndex()];
        return output->isValid() ? output : nullptr;
    }

    if (!types.compilerOutputs || outputIndex >= types.compilerOutputs->length())
        return nullptr;
    CompilerOutput *output = &(*types.compilerOutputs)[outputIndex];
    return output->isValid() ? output : nullptr;
}

inline CompilerOutput*
RecompileInfo::compilerOutput(JSContext *cx) const
{
    return compilerOutput(cx->zone()->types);
}

inline bool
RecompileInfo::shouldSweep(TypeZone &types)
{
    CompilerOutput *output = compilerOutput(types);
    if (!output || !output->isValid())
        return true;

    
    
    MOZ_ASSERT_IF(generation == types.generation,
                  outputIndex == output - types.compilerOutputs->begin());

    
    outputIndex = output - types.compilerOutputs->begin();
    generation = types.generation;
    return false;
}





 inline TypeSetObjectKey *
TypeSetObjectKey::get(JSObject *obj)
{
    MOZ_ASSERT(obj);
    if (obj->isSingleton())
        return (TypeSetObjectKey *) (uintptr_t(obj) | 1);
    return (TypeSetObjectKey *) obj->group();
}

 inline TypeSetObjectKey *
TypeSetObjectKey::get(ObjectGroup *group)
{
    MOZ_ASSERT(group);
    if (group->singleton())
        return (TypeSetObjectKey *) (uintptr_t(group->singleton()) | 1);
    return (TypeSetObjectKey *) group;
}

inline ObjectGroup *
TypeSetObjectKey::groupNoBarrier()
{
    MOZ_ASSERT(isGroup());
    return (ObjectGroup *) this;
}

inline JSObject *
TypeSetObjectKey::singletonNoBarrier()
{
    MOZ_ASSERT(isSingleton());
    return (JSObject *) (uintptr_t(this) & ~1);
}

inline ObjectGroup *
TypeSetObjectKey::group()
{
    ObjectGroup *res = groupNoBarrier();
    ObjectGroup::readBarrier(res);
    return res;
}

inline JSObject *
TypeSetObjectKey::singleton()
{
    JSObject *res = singletonNoBarrier();
    JSObject::readBarrier(res);
    return res;
}

 inline Type
Type::ObjectType(JSObject *obj)
{
    if (obj->isSingleton())
        return Type(uintptr_t(obj) | 1);
    return Type(uintptr_t(obj->group()));
}

 inline Type
Type::ObjectType(ObjectGroup *group)
{
    if (group->singleton())
        return Type(uintptr_t(group->singleton()) | 1);
    return Type(uintptr_t(group));
}

 inline Type
Type::ObjectType(TypeSetObjectKey *obj)
{
    return Type(uintptr_t(obj));
}

inline Type
GetValueType(const Value &val)
{
    if (val.isDouble())
        return Type::DoubleType();
    if (val.isObject())
        return Type::ObjectType(&val.toObject());
    return Type::PrimitiveType(val.extractNonDoubleType());
}

inline bool
IsUntrackedValue(const Value &val)
{
    return val.isMagic() && (val.whyMagic() == JS_OPTIMIZED_OUT ||
                             val.whyMagic() == JS_UNINITIALIZED_LEXICAL);
}

inline Type
GetMaybeUntrackedValueType(const Value &val)
{
    return IsUntrackedValue(val) ? Type::UnknownType() : GetValueType(val);
}

inline TypeFlags
PrimitiveTypeFlag(JSValueType type)
{
    switch (type) {
      case JSVAL_TYPE_UNDEFINED:
        return TYPE_FLAG_UNDEFINED;
      case JSVAL_TYPE_NULL:
        return TYPE_FLAG_NULL;
      case JSVAL_TYPE_BOOLEAN:
        return TYPE_FLAG_BOOLEAN;
      case JSVAL_TYPE_INT32:
        return TYPE_FLAG_INT32;
      case JSVAL_TYPE_DOUBLE:
        return TYPE_FLAG_DOUBLE;
      case JSVAL_TYPE_STRING:
        return TYPE_FLAG_STRING;
      case JSVAL_TYPE_SYMBOL:
        return TYPE_FLAG_SYMBOL;
      case JSVAL_TYPE_MAGIC:
        return TYPE_FLAG_LAZYARGS;
      default:
        MOZ_CRASH("Bad JSValueType");
    }
}

inline JSValueType
TypeFlagPrimitive(TypeFlags flags)
{
    switch (flags) {
      case TYPE_FLAG_UNDEFINED:
        return JSVAL_TYPE_UNDEFINED;
      case TYPE_FLAG_NULL:
        return JSVAL_TYPE_NULL;
      case TYPE_FLAG_BOOLEAN:
        return JSVAL_TYPE_BOOLEAN;
      case TYPE_FLAG_INT32:
        return JSVAL_TYPE_INT32;
      case TYPE_FLAG_DOUBLE:
        return JSVAL_TYPE_DOUBLE;
      case TYPE_FLAG_STRING:
        return JSVAL_TYPE_STRING;
      case TYPE_FLAG_SYMBOL:
        return JSVAL_TYPE_SYMBOL;
      case TYPE_FLAG_LAZYARGS:
        return JSVAL_TYPE_MAGIC;
      default:
        MOZ_CRASH("Bad TypeFlags");
    }
}






inline jsid
IdToTypeId(jsid id)
{
    MOZ_ASSERT(!JSID_IS_EMPTY(id));

    
    
    return JSID_IS_INT(id) ? JSID_VOID : id;
}

const char * TypeIdStringImpl(jsid id);


static inline const char *
TypeIdString(jsid id)
{
#ifdef DEBUG
    return TypeIdStringImpl(id);
#else
    return "(missing)";
#endif
}










struct AutoEnterAnalysis
{
    
    gc::AutoSuppressGC suppressGC;

    
    AutoClearTypeInferenceStateOnOOM oom;

    
    RecompileInfoVector pendingRecompiles;

    FreeOp *freeOp;
    Zone *zone;

    explicit AutoEnterAnalysis(ExclusiveContext *cx)
      : suppressGC(cx), oom(cx->zone())
    {
        init(cx->defaultFreeOp(), cx->zone());
    }

    AutoEnterAnalysis(FreeOp *fop, Zone *zone)
      : suppressGC(zone->runtimeFromMainThread()), oom(zone)
    {
        init(fop, zone);
    }

    ~AutoEnterAnalysis()
    {
        if (this != zone->types.activeAnalysis)
            return;

        zone->types.activeAnalysis = nullptr;

        if (!pendingRecompiles.empty())
            zone->types.processPendingRecompiles(freeOp, pendingRecompiles);
    }

  private:
    void init(FreeOp *fop, Zone *zone) {
        this->freeOp = fop;
        this->zone = zone;

        if (!zone->types.activeAnalysis)
            zone->types.activeAnalysis = this;
    }
};





void MarkIteratorUnknownSlow(JSContext *cx);

void TypeMonitorCallSlow(JSContext *cx, JSObject *callee, const CallArgs &args,
                         bool constructing);





inline void
TypeMonitorCall(JSContext *cx, const js::CallArgs &args, bool constructing)
{
    if (args.callee().is<JSFunction>()) {
        JSFunction *fun = &args.callee().as<JSFunction>();
        if (fun->isInterpreted() && fun->nonLazyScript()->types())
            TypeMonitorCallSlow(cx, &args.callee(), args, constructing);
    }
}

inline bool
TrackPropertyTypes(ExclusiveContext *cx, JSObject *obj, jsid id)
{
    if (obj->hasLazyGroup() || obj->group()->unknownProperties())
        return false;

    if (obj->isSingleton() && !obj->group()->maybeGetProperty(id))
        return false;

    return true;
}

inline void
EnsureTrackPropertyTypes(JSContext *cx, JSObject *obj, jsid id)
{
    id = IdToTypeId(id);

    if (obj->isSingleton()) {
        AutoEnterAnalysis enter(cx);
        if (obj->hasLazyGroup() && !obj->getGroup(cx)) {
            CrashAtUnhandlableOOM("Could not allocate ObjectGroup in EnsureTrackPropertyTypes");
            return;
        }
        if (!obj->group()->unknownProperties() && !obj->group()->getProperty(cx, id)) {
            MOZ_ASSERT(obj->group()->unknownProperties());
            return;
        }
    }

    MOZ_ASSERT(obj->group()->unknownProperties() || TrackPropertyTypes(cx, obj, id));
}

inline bool
CanHaveEmptyPropertyTypesForOwnProperty(JSObject *obj)
{
    
    
    
    return obj->is<GlobalObject>();
}

inline bool
PropertyHasBeenMarkedNonConstant(JSObject *obj, jsid id)
{
    
    if (!obj->isSingleton())
        return true;

    
    if (obj->group()->unknownProperties())
        return true;
    HeapTypeSet *types = obj->group()->maybeGetProperty(IdToTypeId(id));
    return types->nonConstantProperty();
}

inline bool
HasTypePropertyId(JSObject *obj, jsid id, Type type)
{
    if (obj->hasLazyGroup())
        return true;

    if (obj->group()->unknownProperties())
        return true;

    if (HeapTypeSet *types = obj->group()->maybeGetProperty(IdToTypeId(id)))
        return types->hasType(type);

    return false;
}

inline bool
HasTypePropertyId(JSObject *obj, jsid id, const Value &value)
{
    return HasTypePropertyId(obj, id, GetValueType(value));
}

void AddTypePropertyId(ExclusiveContext *cx, ObjectGroup *group, jsid id, Type type);
void AddTypePropertyId(ExclusiveContext *cx, ObjectGroup *group, jsid id, const Value &value);


inline void
AddTypePropertyId(ExclusiveContext *cx, JSObject *obj, jsid id, Type type)
{
    id = IdToTypeId(id);
    if (TrackPropertyTypes(cx, obj, id))
        AddTypePropertyId(cx, obj->group(), id, type);
}

inline void
AddTypePropertyId(ExclusiveContext *cx, JSObject *obj, jsid id, const Value &value)
{
    id = IdToTypeId(id);
    if (TrackPropertyTypes(cx, obj, id))
        AddTypePropertyId(cx, obj->group(), id, value);
}

inline void
MarkObjectGroupFlags(ExclusiveContext *cx, JSObject *obj, ObjectGroupFlags flags)
{
    if (!obj->hasLazyGroup() && !obj->group()->hasAllFlags(flags))
        obj->group()->setFlags(cx, flags);
}

inline void
MarkObjectGroupUnknownProperties(JSContext *cx, ObjectGroup *obj)
{
    if (!obj->unknownProperties())
        obj->markUnknown(cx);
}

inline void
MarkTypePropertyNonData(ExclusiveContext *cx, JSObject *obj, jsid id)
{
    id = IdToTypeId(id);
    if (TrackPropertyTypes(cx, obj, id))
        obj->group()->markPropertyNonData(cx, id);
}

inline void
MarkTypePropertyNonWritable(ExclusiveContext *cx, JSObject *obj, jsid id)
{
    id = IdToTypeId(id);
    if (TrackPropertyTypes(cx, obj, id))
        obj->group()->markPropertyNonWritable(cx, id);
}

inline bool
IsTypePropertyIdMarkedNonData(JSObject *obj, jsid id)
{
    return obj->group()->isPropertyNonData(id);
}

inline bool
IsTypePropertyIdMarkedNonWritable(JSObject *obj, jsid id)
{
    return obj->group()->isPropertyNonWritable(id);
}


inline void
MarkObjectStateChange(ExclusiveContext *cx, JSObject *obj)
{
    if (!obj->hasLazyGroup() && !obj->group()->unknownProperties())
        obj->group()->markStateChange(cx);
}


extern void TypeMonitorResult(JSContext *cx, JSScript *script, jsbytecode *pc,
                              const js::Value &rval);
extern void TypeDynamicResult(JSContext *cx, JSScript *script, jsbytecode *pc,
                              js::types::Type type);





 inline unsigned
TypeScript::NumTypeSets(JSScript *script)
{
    size_t num = script->nTypeSets() + 1 ;
    if (JSFunction *fun = script->functionNonDelazifying())
        num += fun->nargs();
    return num;
}

 inline StackTypeSet *
TypeScript::ThisTypes(JSScript *script)
{
    TypeScript *types = script->types();
    return types ? types->typeArray() + script->nTypeSets() : nullptr;
}







 inline StackTypeSet *
TypeScript::ArgTypes(JSScript *script, unsigned i)
{
    MOZ_ASSERT(i < script->functionNonDelazifying()->nargs());
    TypeScript *types = script->types();
    return types ? types->typeArray() + script->nTypeSets() + 1 + i : nullptr;
}

template <typename TYPESET>
 inline TYPESET *
TypeScript::BytecodeTypes(JSScript *script, jsbytecode *pc, uint32_t *bytecodeMap,
                          uint32_t *hint, TYPESET *typeArray)
{
    MOZ_ASSERT(js_CodeSpec[*pc].format & JOF_TYPESET);
    uint32_t offset = script->pcToOffset(pc);

    
    if ((*hint + 1) < script->nTypeSets() && bytecodeMap[*hint + 1] == offset) {
        (*hint)++;
        return typeArray + *hint;
    }

    
    if (bytecodeMap[*hint] == offset)
        return typeArray + *hint;

    
    size_t bottom = 0;
    size_t top = script->nTypeSets() - 1;
    size_t mid = bottom + (top - bottom) / 2;
    while (mid < top) {
        if (bytecodeMap[mid] < offset)
            bottom = mid + 1;
        else if (bytecodeMap[mid] > offset)
            top = mid;
        else
            break;
        mid = bottom + (top - bottom) / 2;
    }

    
    
    
    MOZ_ASSERT(bytecodeMap[mid] == offset || mid == top);

    *hint = mid;
    return typeArray + *hint;
}

 inline StackTypeSet *
TypeScript::BytecodeTypes(JSScript *script, jsbytecode *pc)
{
    MOZ_ASSERT(CurrentThreadCanAccessRuntime(script->runtimeFromMainThread()));
    TypeScript *types = script->types();
    if (!types)
        return nullptr;
    uint32_t *hint = script->baselineScript()->bytecodeTypeMap() + script->nTypeSets();
    return BytecodeTypes(script, pc, script->baselineScript()->bytecodeTypeMap(),
                         hint, types->typeArray());
}

 inline void
TypeScript::Monitor(JSContext *cx, JSScript *script, jsbytecode *pc, const js::Value &rval)
{
    TypeMonitorResult(cx, script, pc, rval);
}

 inline void
TypeScript::Monitor(JSContext *cx, const js::Value &rval)
{
    jsbytecode *pc;
    RootedScript script(cx, cx->currentScript(&pc));
    Monitor(cx, script, pc, rval);
}

 inline void
TypeScript::MonitorAssign(JSContext *cx, HandleObject obj, jsid id)
{
    if (!obj->isSingleton()) {
        







        uint32_t i;
        if (js_IdIsIndex(id, &i))
            return;

        
        
        
        ObjectGroup* group = obj->group();
        if (group->getPropertyCount() < 128)
            return;
        MarkObjectGroupUnknownProperties(cx, group);
    }
}

 inline void
TypeScript::SetThis(JSContext *cx, JSScript *script, Type type)
{
    StackTypeSet *types = ThisTypes(script);
    if (!types)
        return;

    if (!types->hasType(type)) {
        AutoEnterAnalysis enter(cx);

        InferSpew(ISpewOps, "externalType: setThis %p: %s",
                  script, TypeString(type));
        types->addType(cx, type);
    }
}

 inline void
TypeScript::SetThis(JSContext *cx, JSScript *script, const js::Value &value)
{
    SetThis(cx, script, GetValueType(value));
}

 inline void
TypeScript::SetArgument(JSContext *cx, JSScript *script, unsigned arg, Type type)
{
    StackTypeSet *types = ArgTypes(script, arg);
    if (!types)
        return;

    if (!types->hasType(type)) {
        AutoEnterAnalysis enter(cx);

        InferSpew(ISpewOps, "externalType: setArg %p %u: %s",
                  script, arg, TypeString(type));
        types->addType(cx, type);
    }
}

 inline void
TypeScript::SetArgument(JSContext *cx, JSScript *script, unsigned arg, const js::Value &value)
{
    Type type = GetValueType(value);
    SetArgument(cx, script, arg, type);
}













const unsigned SET_ARRAY_SIZE = 8;
const unsigned SET_CAPACITY_OVERFLOW = 1u << 30;


static inline unsigned
HashSetCapacity(unsigned count)
{
    MOZ_ASSERT(count >= 2);
    MOZ_ASSERT(count < SET_CAPACITY_OVERFLOW);

    if (count <= SET_ARRAY_SIZE)
        return SET_ARRAY_SIZE;

    return 1u << (mozilla::FloorLog2(count) + 2);
}


template <class T, class KEY>
static inline uint32_t
HashKey(T v)
{
    uint32_t nv = KEY::keyBits(v);

    uint32_t hash = 84696351 ^ (nv & 0xff);
    hash = (hash * 16777619) ^ ((nv >> 8) & 0xff);
    hash = (hash * 16777619) ^ ((nv >> 16) & 0xff);
    return (hash * 16777619) ^ ((nv >> 24) & 0xff);
}





template <class T, class U, class KEY>
static U **
HashSetInsertTry(LifoAlloc &alloc, U **&values, unsigned &count, T key)
{
    unsigned capacity = HashSetCapacity(count);
    unsigned insertpos = HashKey<T,KEY>(key) & (capacity - 1);

    
    bool converting = (count == SET_ARRAY_SIZE);

    if (!converting) {
        while (values[insertpos] != nullptr) {
            if (KEY::getKey(values[insertpos]) == key)
                return &values[insertpos];
            insertpos = (insertpos + 1) & (capacity - 1);
        }
    }

    if (count >= SET_CAPACITY_OVERFLOW)
        return nullptr;

    count++;
    unsigned newCapacity = HashSetCapacity(count);

    if (newCapacity == capacity) {
        MOZ_ASSERT(!converting);
        return &values[insertpos];
    }

    U **newValues = alloc.newArray<U*>(newCapacity);
    if (!newValues)
        return nullptr;
    mozilla::PodZero(newValues, newCapacity);

    for (unsigned i = 0; i < capacity; i++) {
        if (values[i]) {
            unsigned pos = HashKey<T,KEY>(KEY::getKey(values[i])) & (newCapacity - 1);
            while (newValues[pos] != nullptr)
                pos = (pos + 1) & (newCapacity - 1);
            newValues[pos] = values[i];
        }
    }

    values = newValues;

    insertpos = HashKey<T,KEY>(key) & (newCapacity - 1);
    while (values[insertpos] != nullptr)
        insertpos = (insertpos + 1) & (newCapacity - 1);
    return &values[insertpos];
}





template <class T, class U, class KEY>
static inline U **
HashSetInsert(LifoAlloc &alloc, U **&values, unsigned &count, T key)
{
    if (count == 0) {
        MOZ_ASSERT(values == nullptr);
        count++;
        return (U **) &values;
    }

    if (count == 1) {
        U *oldData = (U*) values;
        if (KEY::getKey(oldData) == key)
            return (U **) &values;

        values = alloc.newArray<U*>(SET_ARRAY_SIZE);
        if (!values) {
            values = (U **) oldData;
            return nullptr;
        }
        mozilla::PodZero(values, SET_ARRAY_SIZE);
        count++;

        values[0] = oldData;
        return &values[1];
    }

    if (count <= SET_ARRAY_SIZE) {
        for (unsigned i = 0; i < count; i++) {
            if (KEY::getKey(values[i]) == key)
                return &values[i];
        }

        if (count < SET_ARRAY_SIZE) {
            count++;
            return &values[count - 1];
        }
    }

    return HashSetInsertTry<T,U,KEY>(alloc, values, count, key);
}


template <class T, class U, class KEY>
static inline U *
HashSetLookup(U **values, unsigned count, T key)
{
    if (count == 0)
        return nullptr;

    if (count == 1)
        return (KEY::getKey((U *) values) == key) ? (U *) values : nullptr;

    if (count <= SET_ARRAY_SIZE) {
        for (unsigned i = 0; i < count; i++) {
            if (KEY::getKey(values[i]) == key)
                return values[i];
        }
        return nullptr;
    }

    unsigned capacity = HashSetCapacity(count);
    unsigned pos = HashKey<T,KEY>(key) & (capacity - 1);

    while (values[pos] != nullptr) {
        if (KEY::getKey(values[pos]) == key)
            return values[pos];
        pos = (pos + 1) & (capacity - 1);
    }

    return nullptr;
}

inline TypeSetObjectKey *
Type::objectKey() const
{
    MOZ_ASSERT(isObject());
    return (TypeSetObjectKey *) data;
}

inline JSObject *
Type::singleton() const
{
    return objectKey()->singleton();
}

inline ObjectGroup *
Type::group() const
{
    return objectKey()->group();
}

inline JSObject *
Type::singletonNoBarrier() const
{
    return objectKey()->singletonNoBarrier();
}

inline ObjectGroup *
Type::groupNoBarrier() const
{
    return objectKey()->groupNoBarrier();
}

inline bool
TypeSet::hasType(Type type) const
{
    if (unknown())
        return true;

    if (type.isUnknown()) {
        return false;
    } else if (type.isPrimitive()) {
        return !!(flags & PrimitiveTypeFlag(type.primitive()));
    } else if (type.isAnyObject()) {
        return !!(flags & TYPE_FLAG_ANYOBJECT);
    } else {
        return !!(flags & TYPE_FLAG_ANYOBJECT) ||
            HashSetLookup<TypeSetObjectKey*,TypeSetObjectKey,TypeSetObjectKey>
            (objectSet, baseObjectCount(), type.objectKey()) != nullptr;
    }
}

inline void
TypeSet::setBaseObjectCount(uint32_t count)
{
    MOZ_ASSERT(count <= TYPE_FLAG_DOMOBJECT_COUNT_LIMIT);
    flags = (flags & ~TYPE_FLAG_OBJECT_COUNT_MASK)
          | (count << TYPE_FLAG_OBJECT_COUNT_SHIFT);
}

inline void
HeapTypeSet::newPropertyState(ExclusiveContext *cxArg)
{
    
    if (JSContext *cx = cxArg->maybeJSContext()) {
        TypeConstraint *constraint = constraintList;
        while (constraint) {
            constraint->newPropertyState(cx, this);
            constraint = constraint->next;
        }
    } else {
        MOZ_ASSERT(!constraintList);
    }
}

inline void
HeapTypeSet::setNonDataProperty(ExclusiveContext *cx)
{
    if (flags & TYPE_FLAG_NON_DATA_PROPERTY)
        return;

    flags |= TYPE_FLAG_NON_DATA_PROPERTY;
    newPropertyState(cx);
}

inline void
HeapTypeSet::setNonWritableProperty(ExclusiveContext *cx)
{
    if (flags & TYPE_FLAG_NON_WRITABLE_PROPERTY)
        return;

    flags |= TYPE_FLAG_NON_WRITABLE_PROPERTY;
    newPropertyState(cx);
}

inline void
HeapTypeSet::setNonConstantProperty(ExclusiveContext *cx)
{
    if (flags & TYPE_FLAG_NON_CONSTANT_PROPERTY)
        return;

    flags |= TYPE_FLAG_NON_CONSTANT_PROPERTY;
    newPropertyState(cx);
}

inline unsigned
TypeSet::getObjectCount() const
{
    MOZ_ASSERT(!unknownObject());
    uint32_t count = baseObjectCount();
    if (count > SET_ARRAY_SIZE)
        return HashSetCapacity(count);
    return count;
}

inline TypeSetObjectKey *
TypeSet::getObject(unsigned i) const
{
    MOZ_ASSERT(i < getObjectCount());
    if (baseObjectCount() == 1) {
        MOZ_ASSERT(i == 0);
        return (TypeSetObjectKey *) objectSet;
    }
    return objectSet[i];
}

inline JSObject *
TypeSet::getSingleton(unsigned i) const
{
    TypeSetObjectKey *key = getObject(i);
    return (key && key->isSingleton()) ? key->singleton() : nullptr;
}

inline ObjectGroup *
TypeSet::getGroup(unsigned i) const
{
    TypeSetObjectKey *key = getObject(i);
    return (key && key->isGroup()) ? key->group() : nullptr;
}

inline JSObject *
TypeSet::getSingletonNoBarrier(unsigned i) const
{
    TypeSetObjectKey *key = getObject(i);
    return (key && key->isSingleton()) ? key->singletonNoBarrier() : nullptr;
}

inline ObjectGroup *
TypeSet::getGroupNoBarrier(unsigned i) const
{
    TypeSetObjectKey *key = getObject(i);
    return (key && key->isGroup()) ? key->groupNoBarrier() : nullptr;
}

inline const Class *
TypeSet::getObjectClass(unsigned i) const
{
    if (JSObject *object = getSingleton(i))
        return object->getClass();
    if (ObjectGroup *group = getGroup(i))
        return group->clasp();
    return nullptr;
}





inline void
TypeNewScript::writeBarrierPre(TypeNewScript *newScript)
{
    if (!newScript->function()->runtimeFromAnyThread()->needsIncrementalBarrier())
        return;

    JS::Zone *zone = newScript->function()->zoneFromAnyThread();
    if (zone->needsIncrementalBarrier())
        newScript->trace(zone->barrierTracer());
}

} 





inline uint32_t
ObjectGroup::basePropertyCount()
{
    return (flags() & OBJECT_FLAG_PROPERTY_COUNT_MASK) >> OBJECT_FLAG_PROPERTY_COUNT_SHIFT;
}

inline void
ObjectGroup::setBasePropertyCount(uint32_t count)
{
    
    MOZ_ASSERT(count <= OBJECT_FLAG_PROPERTY_COUNT_LIMIT);
    flags_ = (flags() & ~OBJECT_FLAG_PROPERTY_COUNT_MASK)
           | (count << OBJECT_FLAG_PROPERTY_COUNT_SHIFT);
}

inline types::HeapTypeSet *
ObjectGroup::getProperty(ExclusiveContext *cx, jsid id)
{
    MOZ_ASSERT(JSID_IS_VOID(id) || JSID_IS_EMPTY(id) || JSID_IS_STRING(id) || JSID_IS_SYMBOL(id));
    MOZ_ASSERT_IF(!JSID_IS_EMPTY(id), id == types::IdToTypeId(id));
    MOZ_ASSERT(!unknownProperties());

    if (types::HeapTypeSet *types = maybeGetProperty(id))
        return types;

    types::Property *base = cx->typeLifoAlloc().new_<types::Property>(id);
    if (!base) {
        markUnknown(cx);
        return nullptr;
    }

    uint32_t propertyCount = basePropertyCount();
    types::Property **pprop = types::HashSetInsert<jsid,types::Property,types::Property>
        (cx->typeLifoAlloc(), propertySet, propertyCount, id);
    if (!pprop) {
        markUnknown(cx);
        return nullptr;
    }

    MOZ_ASSERT(!*pprop);

    setBasePropertyCount(propertyCount);
    *pprop = base;

    updateNewPropertyTypes(cx, id, &base->types);

    if (propertyCount == OBJECT_FLAG_PROPERTY_COUNT_LIMIT) {
        
        
        
        markUnknown(cx);
    }

    return &base->types;
}

inline types::HeapTypeSet *
ObjectGroup::maybeGetProperty(jsid id)
{
    MOZ_ASSERT(JSID_IS_VOID(id) || JSID_IS_EMPTY(id) || JSID_IS_STRING(id) || JSID_IS_SYMBOL(id));
    MOZ_ASSERT_IF(!JSID_IS_EMPTY(id), id == types::IdToTypeId(id));
    MOZ_ASSERT(!unknownProperties());

    types::Property *prop = types::HashSetLookup<jsid,types::Property,types::Property>
        (propertySet, basePropertyCount(), id);

    return prop ? &prop->types : nullptr;
}

inline unsigned
ObjectGroup::getPropertyCount()
{
    uint32_t count = basePropertyCount();
    if (count > types::SET_ARRAY_SIZE)
        return types::HashSetCapacity(count);
    return count;
}

inline types::Property *
ObjectGroup::getProperty(unsigned i)
{
    MOZ_ASSERT(i < getPropertyCount());
    if (basePropertyCount() == 1) {
        MOZ_ASSERT(i == 0);
        return (types::Property *) propertySet;
    }
    return propertySet[i];
}

template <>
struct GCMethods<const types::Type>
{
    static types::Type initial() { return types::Type::UnknownType(); }
    static bool poisoned(const types::Type &v) {
        return (v.isGroup() && IsPoisonedPtr(v.group()))
            || (v.isSingleton() && IsPoisonedPtr(v.singleton()));
    }
};

template <>
struct GCMethods<types::Type>
{
    static types::Type initial() { return types::Type::UnknownType(); }
    static bool poisoned(const types::Type &v) {
        return (v.isGroup() && IsPoisonedPtr(v.group()))
            || (v.isSingleton() && IsPoisonedPtr(v.singleton()));
    }
};

} 

inline js::types::TypeScript *
JSScript::types()
{
    maybeSweepTypes(nullptr);
    return types_;
}

inline bool
JSScript::ensureHasTypes(JSContext *cx)
{
    return types() || makeTypes(cx);
}

#endif 
