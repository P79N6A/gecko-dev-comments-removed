








































#include "jsarray.h"
#include "jsanalyze.h"
#include "jscompartment.h"
#include "jsinfer.h"
#include "jsprf.h"
#include "vm/GlobalObject.h"

#include "vm/Stack-inl.h"

#ifndef jsinferinlines_h___
#define jsinferinlines_h___





namespace js {
namespace types {

inline jstype
GetValueType(JSContext *cx, const Value &val)
{
    JS_ASSERT(cx->typeInferenceEnabled());
    if (val.isDouble())
        return TYPE_DOUBLE;
    switch (val.extractNonDoubleType()) {
      case JSVAL_TYPE_INT32:
        return TYPE_INT32;
      case JSVAL_TYPE_UNDEFINED:
        return TYPE_UNDEFINED;
      case JSVAL_TYPE_BOOLEAN:
        return TYPE_BOOLEAN;
      case JSVAL_TYPE_STRING:
        return TYPE_STRING;
      case JSVAL_TYPE_NULL:
        return TYPE_NULL;
      case JSVAL_TYPE_MAGIC:
        switch (val.whyMagic()) {
          case JS_LAZY_ARGUMENTS:
            return TYPE_LAZYARGS;
          default:
            JS_NOT_REACHED("Unknown value");
            return (jstype) 0;
        }
      case JSVAL_TYPE_OBJECT: {
        JSObject *obj = &val.toObject();
        JS_ASSERT(obj->type);
        return (jstype) obj->type;
      }
      default:
        JS_NOT_REACHED("Unknown value");
        return (jstype) 0;
    }
}






inline jsid
MakeTypeId(JSContext *cx, jsid id)
{
    JS_ASSERT(!JSID_IS_EMPTY(id));

    



    if (JSID_IS_INT(id))
        return JSID_VOID;

    



    if (JSID_IS_STRING(id)) {
        JSFlatString *str = JSID_TO_FLAT_STRING(id);
        const jschar *cp = str->getCharsZ(cx);
        if (JS7_ISDEC(*cp) || *cp == '-') {
            cp++;
            while (JS7_ISDEC(*cp))
                cp++;
            if (*cp == 0)
                return JSID_VOID;
        }
        return id;
    }

    return JSID_VOID;
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











struct AutoEnterTypeInference
{
    JSContext *cx;
    bool oldActiveAnalysis;
    bool oldActiveInference;

    AutoEnterTypeInference(JSContext *cx, bool compiling = false)
        : cx(cx), oldActiveAnalysis(cx->compartment->activeAnalysis),
          oldActiveInference(cx->compartment->activeInference)
    {
        JS_ASSERT_IF(!compiling, cx->compartment->types.inferenceEnabled);
        cx->compartment->activeAnalysis = true;
        cx->compartment->activeInference = true;
    }

    ~AutoEnterTypeInference()
    {
        cx->compartment->activeAnalysis = oldActiveAnalysis;
        cx->compartment->activeInference = oldActiveInference;

        





        if (!cx->compartment->activeInference) {
            TypeCompartment *types = &cx->compartment->types;
            if (types->pendingNukeTypes)
                types->nukeTypes(cx);
            else if (types->pendingRecompiles)
                types->processPendingRecompiles(cx);
        }
    }
};





struct AutoEnterCompilation
{
    JSContext *cx;
    JSScript *script;

    AutoEnterCompilation(JSContext *cx, JSScript *script)
        : cx(cx), script(script)
    {
        JS_ASSERT(!cx->compartment->types.compiledScript);
        cx->compartment->types.compiledScript = script;
    }

    ~AutoEnterCompilation()
    {
        JS_ASSERT(cx->compartment->types.compiledScript == script);
        cx->compartment->types.compiledScript = NULL;
    }
};


static inline bool
CanHaveReadBarrier(const jsbytecode *pc)
{
    JS_ASSERT(JSOp(*pc) != JSOP_TRAP);

    switch (JSOp(*pc)) {
      case JSOP_LENGTH:
      case JSOP_GETPROP:
      case JSOP_CALLPROP:
      case JSOP_GETXPROP:
      case JSOP_GETELEM:
      case JSOP_CALLELEM:
      case JSOP_NAME:
      case JSOP_CALLNAME:
      case JSOP_GETGNAME:
      case JSOP_CALLGNAME:
      case JSOP_GETGLOBAL:
      case JSOP_CALLGLOBAL:
        return true;
      default:
        return false;
    }
}
















inline TypeObject *
GetTypeNewObject(JSContext *cx, JSProtoKey key)
{
    JSObject *proto;
    if (!js_GetClassPrototype(cx, NULL, key, &proto, NULL))
        return NULL;
    return proto->getNewType(cx);
}


inline TypeObject *
GetTypeCallerInitObject(JSContext *cx, bool isArray)
{
    if (cx->typeInferenceEnabled()) {
        jsbytecode *pc;
        JSScript *script = cx->stack.currentScript(&pc);
        if (script && script->compartment == cx->compartment)
            return script->types.initObject(cx, pc, isArray);
    }
    return GetTypeNewObject(cx, isArray ? JSProto_Array : JSProto_Object);
}


inline void
MarkTypeCallerUnexpected(JSContext *cx, jstype type)
{
    extern void MarkTypeCallerUnexpectedSlow(JSContext *cx, jstype type);

    if (cx->typeInferenceEnabled())
        MarkTypeCallerUnexpectedSlow(cx, type);
}

inline void
MarkTypeCallerUnexpected(JSContext *cx, const Value &value)
{
    extern void MarkTypeCallerUnexpectedSlow(JSContext *cx, const Value &value);

    if (cx->typeInferenceEnabled())
        MarkTypeCallerUnexpectedSlow(cx, value);
}

inline void
MarkTypeCallerOverflow(JSContext *cx)
{
    MarkTypeCallerUnexpected(cx, TYPE_DOUBLE);
}





inline void
TypeMonitorCall(JSContext *cx, const js::CallArgs &args, bool constructing)
{
    extern void TypeMonitorCallSlow(JSContext *cx, JSObject *callee,
                                    const CallArgs &args, bool constructing);

    if (cx->typeInferenceEnabled()) {
        JSObject *callee = &args.callee();
        if (callee->isFunction() && callee->getFunctionPrivate()->isInterpreted())
            TypeMonitorCallSlow(cx, callee, args, constructing);
    }
}


inline void
AddTypePropertyId(JSContext *cx, TypeObject *obj, jsid id, jstype type)
{
    if (cx->typeInferenceEnabled() && !obj->unknownProperties())
        obj->addPropertyType(cx, id, type);
}

inline void
AddTypePropertyId(JSContext *cx, TypeObject *obj, jsid id, const Value &value)
{
    if (cx->typeInferenceEnabled() && !obj->unknownProperties())
        obj->addPropertyType(cx, id, value);
}

inline void
AddTypeProperty(JSContext *cx, TypeObject *obj, const char *name, jstype type)
{
    if (cx->typeInferenceEnabled() && !obj->unknownProperties())
        obj->addPropertyType(cx, name, type);
}

inline void
AddTypeProperty(JSContext *cx, TypeObject *obj, const char *name, const Value &value)
{
    if (cx->typeInferenceEnabled() && !obj->unknownProperties())
        obj->addPropertyType(cx, name, value);
}


inline void
AddTypePropertySet(JSContext *cx, TypeObject *obj, jsid id, ClonedTypeSet *set)
{
    if (cx->typeInferenceEnabled() && !obj->unknownProperties())
        obj->addPropertyTypeSet(cx, id, set);
}


inline TypeObject *
GetTypeEmpty(JSContext *cx)
{
    return &cx->compartment->types.typeEmpty;
}


inline void
AliasTypeProperties(JSContext *cx, TypeObject *obj, jsid first, jsid second)
{
    if (cx->typeInferenceEnabled() && !obj->unknownProperties())
        obj->aliasProperties(cx, first, second);
}


inline void
MarkTypeObjectFlags(JSContext *cx, TypeObject *obj, TypeObjectFlags flags)
{
    if (cx->typeInferenceEnabled() && !obj->hasAllFlags(flags))
        obj->setFlags(cx, flags);
}


inline void
MarkTypeObjectUnknownProperties(JSContext *cx, TypeObject *obj)
{
    if (cx->typeInferenceEnabled() && !obj->unknownProperties())
        obj->markUnknown(cx);
}





inline void
MarkTypePropertyConfigured(JSContext *cx, TypeObject *obj, jsid id)
{
    if (cx->typeInferenceEnabled() && !obj->unknownProperties())
        obj->markPropertyConfigured(cx, id);
}


inline void
MarkGlobalReallocation(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isGlobal());
    if (cx->typeInferenceEnabled() && !obj->getType()->unknownProperties())
        obj->getType()->markSlotReallocation(cx);
}






inline void
FixArrayType(JSContext *cx, JSObject *obj)
{
    if (cx->typeInferenceEnabled())
        cx->compartment->types.fixArrayType(cx, obj);
}

inline void
FixObjectType(JSContext *cx, JSObject *obj)
{
    if (cx->typeInferenceEnabled())
        cx->compartment->types.fixObjectType(cx, obj);
}


extern void TypeMonitorResult(JSContext *cx, JSScript *script, jsbytecode *pc, const js::Value &rval);
extern void TypeDynamicResult(JSContext *cx, JSScript *script, jsbytecode *pc, js::types::jstype type);





inline JSScript *
TypeScript::script()
{
    




    return (JSScript *)((char *)this - offsetof(JSScript, types));
}

inline unsigned
TypeScript::numTypeSets()
{
    return script()->nTypeSets + analyze::TotalSlots(script()) + script()->bindings.countUpvars();
}

inline bool
TypeScript::ensureTypeArray(JSContext *cx)
{
    if (typeArray)
        return true;
    return makeTypeArray(cx);
}

inline TypeSet *
TypeScript::bytecodeTypes(const jsbytecode *pc)
{
    JS_ASSERT(typeArray);

    JSOp op = JSOp(*pc);
    JS_ASSERT(op != JSOP_TRAP);
    JS_ASSERT(js_CodeSpec[op].format & JOF_TYPESET);

    
    const jsbytecode *npc = (op == JSOP_GETELEM || op == JSOP_CALLELEM) ? pc : pc + 2;
    JS_ASSERT(npc - pc + 3 == js_CodeSpec[op].length);

    uint16 index = GET_UINT16(npc);
    JS_ASSERT(index < script()->nTypeSets);

    return &typeArray[index];
}

inline TypeSet *
TypeScript::returnTypes()
{
    JS_ASSERT(typeArray);
    return &typeArray[script()->nTypeSets + js::analyze::CalleeSlot()];
}

inline TypeSet *
TypeScript::thisTypes()
{
    JS_ASSERT(typeArray);
    return &typeArray[script()->nTypeSets + js::analyze::ThisSlot()];
}







inline TypeSet *
TypeScript::argTypes(unsigned i)
{
    JS_ASSERT(typeArray && script()->fun && i < script()->fun->nargs);
    return &typeArray[script()->nTypeSets + js::analyze::ArgSlot(i)];
}

inline TypeSet *
TypeScript::localTypes(unsigned i)
{
    JS_ASSERT(typeArray && i < script()->nfixed);
    return &typeArray[script()->nTypeSets + js::analyze::LocalSlot(script(), i)];
}

inline TypeSet *
TypeScript::upvarTypes(unsigned i)
{
    JS_ASSERT(typeArray && i < script()->bindings.countUpvars());
    return &typeArray[script()->nTypeSets + js::analyze::TotalSlots(script()) + i];
}

inline TypeSet *
TypeScript::slotTypes(unsigned slot)
{
    JS_ASSERT(typeArray && slot < js::analyze::TotalSlots(script()));
    return &typeArray[script()->nTypeSets + slot];
}

inline TypeObject *
TypeScript::standardType(JSContext *cx, JSProtoKey key)
{
    JSObject *proto;
    if (!js_GetClassPrototype(cx, script()->global(), key, &proto, NULL))
        return NULL;
    return proto->getNewType(cx);
}

inline TypeObject *
TypeScript::initObject(JSContext *cx, const jsbytecode *pc, bool isArray)
{
    if (!cx->typeInferenceEnabled() || !script()->hasGlobal())
        return GetTypeNewObject(cx, isArray ? JSProto_Array : JSProto_Object);

    uint32 offset = pc - script()->code;
    TypeObject *prev = NULL, *obj = typeObjects;
    while (obj) {
        if (isArray ? obj->initializerArray : obj->initializerObject) {
            if (obj->initializerOffset == offset) {
                
                if (prev) {
                    prev->next = obj->next;
                    obj->next = typeObjects;
                    typeObjects = obj;
                }
                return obj;
            }
        }
        prev = obj;
        obj = obj->next;
    }

    return cx->compartment->types.newInitializerTypeObject(cx, script(), offset, isArray);
}

inline void
TypeScript::monitor(JSContext *cx, jsbytecode *pc, const js::Value &rval)
{
    if (cx->typeInferenceEnabled())
        TypeMonitorResult(cx, script(), pc, rval);
}

inline void
TypeScript::monitorOverflow(JSContext *cx, jsbytecode *pc)
{
    if (cx->typeInferenceEnabled())
        TypeDynamicResult(cx, script(), pc, TYPE_DOUBLE);
}

inline void
TypeScript::monitorString(JSContext *cx, jsbytecode *pc)
{
    if (cx->typeInferenceEnabled())
        TypeDynamicResult(cx, script(), pc, TYPE_STRING);
}

inline void
TypeScript::monitorUnknown(JSContext *cx, jsbytecode *pc)
{
    if (cx->typeInferenceEnabled())
        TypeDynamicResult(cx, script(), pc, TYPE_UNKNOWN);
}

inline void
TypeScript::monitorAssign(JSContext *cx, jsbytecode *pc,
                          JSObject *obj, jsid id, const js::Value &rval)
{
    if (cx->typeInferenceEnabled()) {
        





        uint32 i;
        if (js_IdIsIndex(id, &i))
            return;
        MarkTypeObjectUnknownProperties(cx, obj->getType());
    }
}

inline void
TypeScript::setThis(JSContext *cx, jstype type)
{
    JS_ASSERT(cx->typeInferenceEnabled());
    if (!ensureTypeArray(cx))
        return;

    
    bool analyze = cx->hasRunOption(JSOPTION_METHODJIT_ALWAYS) && !script()->isUncachedEval;

    if (!thisTypes()->hasType(type) || analyze) {
        AutoEnterTypeInference enter(cx);

        InferSpew(ISpewOps, "externalType: setThis #%u: %s",
                  script()->id(), TypeString(type));
        thisTypes()->addType(cx, type);

        if (analyze)
            script()->ensureRanInference(cx);
    }
}

inline void
TypeScript::setThis(JSContext *cx, const js::Value &value)
{
    if (cx->typeInferenceEnabled())
        setThis(cx, GetValueType(cx, value));
}

inline void
TypeScript::setThis(JSContext *cx, ClonedTypeSet *set)
{
    JS_ASSERT(cx->typeInferenceEnabled());
    if (!ensureTypeArray(cx))
        return;
    AutoEnterTypeInference enter(cx);

    InferSpew(ISpewOps, "externalType: setThis #%u", script()->id());
    thisTypes()->addTypeSet(cx, set);
}

inline void
TypeScript::setNewCalled(JSContext *cx)
{
    if (!cx->typeInferenceEnabled() || script()->calledWithNew)
        return;
    script()->calledWithNew = true;

    







    if (script()->ranInference) {
        AutoEnterTypeInference enter(cx);
        analyze::ScriptAnalysis *analysis = script()->analysis(cx);
        if (!analysis)
            return;
        analysis->analyzeTypesNew(cx);
    }
}

inline void
TypeScript::setLocal(JSContext *cx, unsigned local, jstype type)
{
    if (!cx->typeInferenceEnabled() || !ensureTypeArray(cx))
        return;
    if (!localTypes(local)->hasType(type)) {
        AutoEnterTypeInference enter(cx);

        InferSpew(ISpewOps, "externalType: setLocal #%u %u: %s",
                  script()->id(), local, TypeString(type));
        localTypes(local)->addType(cx, type);
    }
}

inline void
TypeScript::setLocal(JSContext *cx, unsigned local, const js::Value &value)
{
    if (cx->typeInferenceEnabled()) {
        jstype type = GetValueType(cx, value);
        setLocal(cx, local, type);
    }
}

inline void
TypeScript::setLocal(JSContext *cx, unsigned local, ClonedTypeSet *set)
{
    JS_ASSERT(cx->typeInferenceEnabled());
    if (!ensureTypeArray(cx))
        return;
    AutoEnterTypeInference enter(cx);

    InferSpew(ISpewOps, "externalType: setLocal #%u %u", script()->id(), local);
    localTypes(local)->addTypeSet(cx, set);
}

inline void
TypeScript::setArgument(JSContext *cx, unsigned arg, jstype type)
{
    if (!cx->typeInferenceEnabled() || !ensureTypeArray(cx))
        return;
    if (!argTypes(arg)->hasType(type)) {
        AutoEnterTypeInference enter(cx);

        InferSpew(ISpewOps, "externalType: setArg #%u %u: %s",
                  script()->id(), arg, TypeString(type));
        argTypes(arg)->addType(cx, type);
    }
}

inline void
TypeScript::setArgument(JSContext *cx, unsigned arg, const js::Value &value)
{
    if (cx->typeInferenceEnabled()) {
        jstype type = GetValueType(cx, value);
        setArgument(cx, arg, type);
    }
}

inline void
TypeScript::setArgument(JSContext *cx, unsigned arg, ClonedTypeSet *set)
{
    JS_ASSERT(cx->typeInferenceEnabled());
    if (!ensureTypeArray(cx))
        return;
    AutoEnterTypeInference enter(cx);

    InferSpew(ISpewOps, "externalType: setArg #%u %u", script()->id(), arg);
    argTypes(arg)->addTypeSet(cx, set);
}

inline void
TypeScript::setUpvar(JSContext *cx, unsigned upvar, const js::Value &value)
{
    if (!cx->typeInferenceEnabled() || !ensureTypeArray(cx))
        return;
    jstype type = GetValueType(cx, value);
    if (!upvarTypes(upvar)->hasType(type)) {
        AutoEnterTypeInference enter(cx);

        InferSpew(ISpewOps, "externalType: setUpvar #%u %u: %s",
                  script()->id(), upvar, TypeString(type));
        upvarTypes(upvar)->addType(cx, type);
    }
}





inline void
TypeCompartment::addPending(JSContext *cx, TypeConstraint *constraint, TypeSet *source, jstype type)
{
    JS_ASSERT(this == &cx->compartment->types);
    JS_ASSERT(type);
    JS_ASSERT(!cx->runtime->gcRunning);

    InferSpew(ISpewOps, "pending: C%p %s", constraint, TypeString(type));

    if (pendingCount == pendingCapacity)
        growPendingArray(cx);

    PendingWork &pending = pendingArray[pendingCount++];
    pending.constraint = constraint;
    pending.source = source;
    pending.type = type;
}

inline void
TypeCompartment::resolvePending(JSContext *cx)
{
    JS_ASSERT(this == &cx->compartment->types);

    if (resolving) {
        
        return;
    }

    resolving = true;

    
    while (pendingCount) {
        const PendingWork &pending = pendingArray[--pendingCount];
        InferSpew(ISpewOps, "resolve: C%p %s",
                  pending.constraint, TypeString(pending.type));
        pending.constraint->newType(cx, pending.source, pending.type);
    }

    resolving = false;
}













const unsigned SET_ARRAY_SIZE = 8;


static inline unsigned
HashSetCapacity(unsigned count)
{
    JS_ASSERT(count >= 2);

    if (count <= SET_ARRAY_SIZE)
        return SET_ARRAY_SIZE;

    unsigned log2;
    JS_FLOOR_LOG2(log2, count);
    return 1 << (log2 + 2);
}


template <class T, class KEY>
static inline uint32
HashKey(T v)
{
    uint32 nv = KEY::keyBits(v);

    uint32 hash = 84696351 ^ (nv & 0xff);
    hash = (hash * 16777619) ^ ((nv >> 8) & 0xff);
    hash = (hash * 16777619) ^ ((nv >> 16) & 0xff);
    return (hash * 16777619) ^ ((nv >> 24) & 0xff);
}





template <class T, class U, class KEY>
static U **
HashSetInsertTry(JSContext *cx, U **&values, unsigned &count, T key, bool pool)
{
    unsigned capacity = HashSetCapacity(count);
    unsigned insertpos = HashKey<T,KEY>(key) & (capacity - 1);

    
    bool converting = (count == SET_ARRAY_SIZE);

    if (!converting) {
        while (values[insertpos] != NULL) {
            if (KEY::getKey(values[insertpos]) == key)
                return &values[insertpos];
            insertpos = (insertpos + 1) & (capacity - 1);
        }
    }

    count++;
    unsigned newCapacity = HashSetCapacity(count);

    if (newCapacity == capacity) {
        JS_ASSERT(!converting);
        return &values[insertpos];
    }

    U **newValues = pool
        ? ArenaArray<U*>(cx->compartment->pool, newCapacity)
        : (U **) js::OffTheBooks::malloc_(newCapacity * sizeof(U*));
    if (!newValues) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return NULL;
    }
    PodZero(newValues, newCapacity);

    for (unsigned i = 0; i < capacity; i++) {
        if (values[i]) {
            unsigned pos = HashKey<T,KEY>(KEY::getKey(values[i])) & (newCapacity - 1);
            while (newValues[pos] != NULL)
                pos = (pos + 1) & (newCapacity - 1);
            newValues[pos] = values[i];
        }
    }

    if (values && !pool)
        Foreground::free_(values);
    values = newValues;

    insertpos = HashKey<T,KEY>(key) & (newCapacity - 1);
    while (values[insertpos] != NULL)
        insertpos = (insertpos + 1) & (newCapacity - 1);
    return &values[insertpos];
}





template <class T, class U, class KEY>
static inline U **
HashSetInsert(JSContext *cx, U **&values, unsigned &count, T key, bool pool)
{
    if (count == 0) {
        JS_ASSERT(values == NULL);
        count++;
        return (U **) &values;
    }

    if (count == 1) {
        U *oldData = (U*) values;
        if (KEY::getKey(oldData) == key)
            return (U **) &values;

        values = pool
            ? ArenaArray<U*>(cx->compartment->pool, SET_ARRAY_SIZE)
            : (U **) js::OffTheBooks::malloc_(SET_ARRAY_SIZE * sizeof(U*));
        if (!values) {
            values = (U **) oldData;
            cx->compartment->types.setPendingNukeTypes(cx);
            return NULL;
        }
        PodZero(values, SET_ARRAY_SIZE);
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

    return HashSetInsertTry<T,U,KEY>(cx, values, count, key, pool);
}


template <class T, class U, class KEY>
static inline U *
HashSetLookup(U **values, unsigned count, T key)
{
    if (count == 0)
        return NULL;

    if (count == 1)
        return (KEY::getKey((U *) values) == key) ? (U *) values : NULL;

    if (count <= SET_ARRAY_SIZE) {
        for (unsigned i = 0; i < count; i++) {
            if (KEY::getKey(values[i]) == key)
                return values[i];
        }
        return NULL;
    }

    unsigned capacity = HashSetCapacity(count);
    unsigned pos = HashKey<T,KEY>(key) & (capacity - 1);

    while (values[pos] != NULL) {
        if (KEY::getKey(values[pos]) == key)
            return values[pos];
        pos = (pos + 1) & (capacity - 1);
    }

    return NULL;
}

struct TypeObjectKey {
    static intptr_t keyBits(TypeObject *obj) { return (intptr_t) obj; }
    static TypeObject *getKey(TypeObject *obj) { return obj; }
};

inline void
TypeSet::destroy(JSContext *cx)
{
    JS_ASSERT(!intermediate());
    if (objectCount >= 2)
        Foreground::free_(objectSet);
    while (constraintList) {
        TypeConstraint *next = constraintList->next;
        if (constraintList->condensed() || constraintList->persistentObject())
            Foreground::free_(constraintList);
        constraintList = next;
    }
}

inline bool
TypeSet::hasType(jstype type)
{
    if (unknown())
        return true;

    if (type == TYPE_UNKNOWN) {
        return false;
    } else if (TypeIsPrimitive(type)) {
        return ((1 << type) & typeFlags) != 0;
    } else {
        return HashSetLookup<TypeObject*,TypeObject,TypeObjectKey>
            (objectSet, objectCount, (TypeObject *) type) != NULL;
    }
}

inline void
TypeSet::markUnknown(JSContext *cx)
{
    typeFlags = TYPE_FLAG_UNKNOWN | (typeFlags & ~baseFlags());
    if (objectCount >= 2 && !intermediate())
        cx->free_(objectSet);
    objectCount = 0;
    objectSet = NULL;
}

inline void
TypeSet::addType(JSContext *cx, jstype type)
{
    JS_ASSERT(type);
    JS_ASSERT(cx->compartment->activeInference);

    if (unknown())
        return;

    if (type == TYPE_UNKNOWN) {
        markUnknown(cx);
    } else if (TypeIsPrimitive(type)) {
        TypeFlags flag = 1 << type;
        if (typeFlags & flag)
            return;

        
        if (flag == TYPE_FLAG_DOUBLE)
            flag |= TYPE_FLAG_INT32;

        typeFlags |= flag;
    } else {
        TypeObject *object = (TypeObject*) type;
        TypeObject **pentry = HashSetInsert<TypeObject *,TypeObject,TypeObjectKey>
                                  (cx, objectSet, objectCount, object, intermediate());
        if (!pentry || *pentry)
            return;
        *pentry = object;

        object->contribution += objectCount;
        if (object->contribution >= TypeObject::CONTRIBUTION_LIMIT) {
            InferSpew(ISpewOps, "limitUnknown: T%p", this);
            type = TYPE_UNKNOWN;
            markUnknown(cx);
        }
    }

    InferSpew(ISpewOps, "addType: T%p %s", this, TypeString(type));

    
    TypeConstraint *constraint = constraintList;
    while (constraint) {
        JS_ASSERT_IF(!constraint->persistentObject(),
                     constraint->script->compartment == cx->compartment);
        cx->compartment->types.addPending(cx, constraint, this, type);
        constraint = constraint->next;
    }

    cx->compartment->types.resolvePending(cx);
}

inline void
TypeSet::setOwnProperty(JSContext *cx, bool configured)
{
    TypeFlags nflags = TYPE_FLAG_OWN_PROPERTY | (configured ? TYPE_FLAG_CONFIGURED_PROPERTY : 0);

    if ((typeFlags & nflags) == nflags)
        return;

    typeFlags |= nflags;

    
    TypeConstraint *constraint = constraintList;
    while (constraint) {
        JS_ASSERT_IF(!constraint->persistentObject(),
                     constraint->script->compartment == cx->compartment);
        constraint->newPropertyState(cx, this);
        constraint = constraint->next;
    }
}

inline unsigned
TypeSet::getObjectCount()
{
    JS_ASSERT(!unknown());
    if (objectCount > SET_ARRAY_SIZE)
        return HashSetCapacity(objectCount);
    return objectCount;
}

inline TypeObject *
TypeSet::getObject(unsigned i)
{
    if (objectCount == 1) {
        JS_ASSERT(i == 0);
        return (TypeObject *) objectSet;
    }
    return objectSet[i];
}

inline TypeObject *
TypeSet::getSingleObject()
{
    if (!baseFlags() && objectCount == 1)
        return getObject(0);
    return NULL;
}

inline TypeSet *
TypeSet::make(JSContext *cx, const char *name)
{
    JS_ASSERT(cx->compartment->activeInference);

    TypeSet *res = ArenaNew<TypeSet>(cx->compartment->pool);
    if (!res) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return NULL;
    }

    InferSpew(ISpewOps, "typeSet: T%p intermediate %s", res, name);
    res->setIntermediate();

    return res;
}





inline
TypeCallsite::TypeCallsite(JSContext *cx, JSScript *script, jsbytecode *pc,
                           bool isNew, unsigned argumentCount)
    : script(script), pc(pc), isNew(isNew), argumentCount(argumentCount),
      thisTypes(NULL), returnTypes(NULL)
{
    
    argumentTypes = ArenaArray<TypeSet*>(cx->compartment->pool, argumentCount);
}

inline TypeObject *
TypeCallsite::getInitObject(JSContext *cx, bool isArray)
{
    TypeObject *type = script->types.initObject(cx, pc, isArray);
    if (!type)
        cx->compartment->types.setPendingNukeTypes(cx);
    return type;
}





inline TypeSet *
TypeObject::getProperty(JSContext *cx, jsid id, bool assign)
{
    JS_ASSERT(cx->compartment->activeInference);
    JS_ASSERT(JSID_IS_VOID(id) || JSID_IS_EMPTY(id) || JSID_IS_STRING(id));
    JS_ASSERT_IF(!JSID_IS_EMPTY(id), id == MakeTypeId(cx, id));
    JS_ASSERT_IF(JSID_IS_STRING(id), JSID_TO_STRING(id) != NULL);
    JS_ASSERT(!unknownProperties());

    Property **pprop = HashSetInsert<jsid,Property,Property>
                           (cx, propertySet, propertyCount, id, false);
    if (!pprop || (!*pprop && !addProperty(cx, id, pprop)))
        return NULL;

    if (assign)
        (*pprop)->types.setOwnProperty(cx, false);

    return &(*pprop)->types;
}

inline unsigned
TypeObject::getPropertyCount()
{
    if (propertyCount > SET_ARRAY_SIZE)
        return HashSetCapacity(propertyCount);
    return propertyCount;
}

inline Property *
TypeObject::getProperty(unsigned i)
{
    if (propertyCount == 1) {
        JS_ASSERT(i == 0);
        return (Property *) propertySet;
    }
    return propertySet[i];
}





inline const char *
TypeObject::name()
{
#ifdef DEBUG
    return TypeIdString(name_);
#else
    return NULL;
#endif
}

inline TypeObject::TypeObject(jsid name, JSObject *proto)
    : proto(proto), emptyShapes(NULL),
      flags(0), isFunction(false), marked(false), newScriptCleared(false),
      newScript(NULL), initializerObject(false), initializerArray(false), initializerOffset(0),
      contribution(0), propertySet(NULL), propertyCount(0),
      instanceList(NULL), instanceNext(NULL), next(NULL),
      singleton(NULL)
{
#ifdef DEBUG
    this->name_ = name;
#endif

    InferSpew(ISpewOps, "newObject: %s", this->name());
}

inline TypeFunction::TypeFunction(jsid name, JSObject *proto)
    : TypeObject(name, proto), handler(NULL), script(NULL), isGeneric(false)
{
    isFunction = true;
}

inline void
SweepClonedTypes(ClonedTypeSet *types)
{
    if (types->objectCount >= 2) {
        for (unsigned i = 0; i < types->objectCount; i++) {
            if (!types->objectSet[i]->marked)
                types->objectSet[i--] = types->objectSet[--types->objectCount];
        }
        if (types->objectCount == 1) {
            TypeObject *obj = types->objectSet[0];
            Foreground::free_(types->objectSet);
            types->objectSet = (TypeObject **) obj;
        } else if (types->objectCount == 0) {
            Foreground::free_(types->objectSet);
            types->objectSet = NULL;
        }
    } else if (types->objectCount == 1) {
        TypeObject *obj = (TypeObject *) types->objectSet;
        if (!obj->marked) {
            types->objectSet = NULL;
            types->objectCount = 0;
        }
    }
}

class AutoTypeRooter : private AutoGCRooter {
  public:
    AutoTypeRooter(JSContext *cx, TypeObject *type
                   JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, TYPE), type(type)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);
    friend void MarkRuntime(JSTracer *trc);

  private:
    TypeObject *type;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

} } 

inline bool
JSScript::isAboutToBeFinalized(JSContext *cx)
{
    return isCachedEval ||
        (u.object && IsAboutToBeFinalized(cx, u.object)) ||
        (fun && IsAboutToBeFinalized(cx, fun));
}

inline bool
JSScript::ensureRanInference(JSContext *cx)
{
    js::analyze::ScriptAnalysis *analysis = this->analysis(cx);
    if (analysis && !analysis->ranInference()) {
        js::types::AutoEnterTypeInference enter(cx);
        analysis->analyzeTypes(cx);
    }
    return analysis && !analysis->OOM();
}

inline void
js::analyze::ScriptAnalysis::addPushedType(JSContext *cx, uint32 offset, uint32 which,
                                           js::types::jstype type)
{
    js::types::TypeSet *pushed = pushedTypes(offset, which);
    pushed->addType(cx, type);
}

#endif 
