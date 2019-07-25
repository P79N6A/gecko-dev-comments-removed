








































#include "jsanalyze.h"
#include "jscompartment.h"
#include "jsinfer.h"
#include "jsprf.h"

#ifndef jsinferinlines_h___
#define jsinferinlines_h___





namespace js {
namespace types {

inline jstype
GetValueType(JSContext *cx, const Value &val)
{
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
    if (JSID_IS_VOID(id))
        return JSID_VOID;

    



    if (JSID_IS_INT(id))
        return JSID_VOID;

    
    if (JSID_IS_OBJECT(id))
        return JSID_VOID;

    



    if (JSID_IS_STRING(id)) {
        JSFlatString *str = JSID_TO_FLAT_STRING(id);
        const jschar *cp = str->getCharsZ(cx);
        if (JS7_ISDEC(*cp) || *cp == '-') {
            cp++;
            while (JS7_ISDEC(*cp))
                cp++;
            if (unsigned(cp - str->chars()) == str->length())
                return JSID_VOID;
        }
        
        return ATOM_TO_JSID(js_AtomizeString(cx, str, ATOM_PINNED));
    }

    JS_NOT_REACHED("Unknown id");
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

} } 





inline js::types::TypeObject *
JSContext::getTypeNewObject(JSProtoKey key)
{
    JSObject *proto;
    if (!js_GetClassPrototype(this, NULL, key, &proto, NULL))
        return NULL;
    return proto->getNewType(this);
}

inline js::types::TypeObject *
JSContext::emptyTypeObject()
{
    return &compartment->types.emptyObject;
}

inline void
JSContext::setTypeFunctionScript(JSFunction *fun, JSScript *script)
{
#ifdef JS_TYPE_INFERENCE
    js::types::TypeFunction *typeFun = fun->getType()->asFunction();

    typeFun->script = script;
    script->fun = fun;
#endif
}






inline js::types::TypeObject *
JSContext::getTypeCallerInitObject(bool isArray)
{
#ifdef JS_TYPE_INFERENCE
    JSStackFrame *caller = js_GetScriptedCaller(this, NULL);
    if (caller)
        return caller->script()->getTypeInitObject(this, caller->pc(this), isArray);
#endif
    return getTypeNewObject(isArray ? JSProto_Array : JSProto_Object);
}

inline bool
JSContext::isTypeCallerMonitored()
{
#ifdef JS_TYPE_INFERENCE
    JSStackFrame *caller = js_GetScriptedCaller(this, NULL);
    if (!caller)
        return true;
    JSScript *script = caller->script();
    return !script->types || script->types->monitored(caller->pc(this) - script->code);
#else
    return false;
#endif
}

inline void
JSContext::markTypeCallerUnexpected(js::types::jstype type)
{
#ifdef JS_TYPE_INFERENCE
    JSStackFrame *caller = js_GetScriptedCaller(this, NULL);
    if (!caller)
        return;
    caller->script()->typeMonitorResult(this, caller->pc(this), 0, type);
#endif
}

inline void
JSContext::markTypeCallerUnexpected(const js::Value &value)
{
    markTypeCallerUnexpected(js::types::GetValueType(this, value));
}

inline void
JSContext::markTypeCallerOverflow()
{
    markTypeCallerUnexpected(js::types::TYPE_DOUBLE);
}

inline void
JSContext::addTypeProperty(js::types::TypeObject *obj, const char *name, js::types::jstype type)
{
#ifdef JS_TYPE_INFERENCE
    
    jsid id = JSID_VOID;
    if (name)
        id = ATOM_TO_JSID(js_Atomize(this, name, strlen(name), ATOM_PINNED));
    addTypePropertyId(obj, id, type);
#endif
}

inline void
JSContext::addTypeProperty(js::types::TypeObject *obj, const char *name, const js::Value &value)
{
#ifdef JS_TYPE_INFERENCE
    addTypeProperty(obj, name, js::types::GetValueType(this, value));
#endif
}

inline void
JSContext::addTypePropertyId(js::types::TypeObject *obj, jsid id, js::types::jstype type)
{
#ifdef JS_TYPE_INFERENCE
    
    id = js::types::MakeTypeId(this, id);

    js::types::TypeSet *types = obj->getProperty(this, id, true);

    if (types->hasType(type))
        return;

    if (compartment->types.interpreting) {
        js::types::InferSpew(js::types::ISpewDynamic, "AddBuiltin: %s %s: %s",
                             obj->name(), js::types::TypeIdString(id),
                             js::types::TypeString(type));
        compartment->types.addDynamicType(this, types, type);
    } else {
        types->addType(this, type);
    }
#endif
}

inline void
JSContext::addTypePropertyId(js::types::TypeObject *obj, jsid id, const js::Value &value)
{
#ifdef JS_TYPE_INFERENCE
    addTypePropertyId(obj, id, js::types::GetValueType(this, value));
#endif
}

inline void
JSContext::markTypePropertyUnknown(js::types::TypeObject *obj, jsid id)
{
#ifdef JS_TYPE_INFERENCE
    
    id = js::types::MakeTypeId(this, id);

    js::types::TypeSet *types = obj->getProperty(this, id, true);

    if (types->unknown())
        return;

    if (compartment->types.interpreting) {
        js::types::InferSpew(js::types::ISpewDynamic, "AddUnknown: %s %s",
                             obj->name(), js::types::TypeIdString(id));
        compartment->types.addDynamicType(this, types, js::types::TYPE_UNKNOWN);
    } else {
        types->addType(this, js::types::TYPE_UNKNOWN);
    }
#endif
}

inline js::types::TypeObject *
JSContext::getTypeGetSet()
{
    if (!compartment->types.typeGetSet)
        compartment->types.typeGetSet = newTypeObject("GetSet", NULL);
    return compartment->types.typeGetSet;
}

inline void
JSContext::aliasTypeProperties(js::types::TypeObject *obj, jsid first, jsid second)
{
#ifdef JS_TYPE_INFERENCE
    first = js::types::MakeTypeId(this, first);
    second = js::types::MakeTypeId(this, second);

    js::types::TypeSet *firstTypes = obj->getProperty(this, first, true);
    js::types::TypeSet *secondTypes = obj->getProperty(this, second, true);

    firstTypes->addSubset(this, *obj->pool, secondTypes);
    secondTypes->addSubset(this, *obj->pool, firstTypes);
#endif
}

inline void
JSContext::markTypeArrayNotPacked(js::types::TypeObject *obj, bool notDense, bool dynamic)
{
#ifdef JS_TYPE_INFERENCE
    if (notDense) {
        if (!obj->isDenseArray)
            return;
        obj->isDenseArray = false;
    } else if (!obj->isPackedArray) {
        return;
    }
    obj->isPackedArray = false;

    if (dynamic) {
        js::types::InferSpew(js::types::ISpewDynamic, "%s: %s",
                             notDense ? "NonDenseArray" : "NonPackedArray", obj->name());
    }

    
    js::types::TypeSet *elementTypes = obj->getProperty(this, JSID_VOID, false);
    js::types::TypeConstraint *constraint = elementTypes->constraintList;
    while (constraint) {
        constraint->arrayNotPacked(this, notDense);
        constraint = constraint->next;
    }

    if (dynamic && compartment->types.hasPendingRecompiles())
        compartment->types.processPendingRecompiles(this);
#endif
}

void
JSContext::markTypeObjectUnknownProperties(js::types::TypeObject *obj)
{
#ifdef JS_TYPE_INFERENCE
    if (obj->unknownProperties)
        return;
    obj->markUnknown(this);
#endif
}

inline void
JSContext::typeMonitorCall(JSScript *caller, const jsbytecode *callerpc,
                           const js::CallArgs &args, bool constructing, bool force)
{
    JS_ASSERT_IF(caller == NULL, force);
#ifdef JS_TYPE_INFERENCE
    if (!args.callee().isObject() || !args.callee().toObject().isFunction())
        return;
    JSFunction *callee = args.callee().toObject().getFunctionPrivate();

    





    if (!callee->isInterpreted())
        return;

    JSScript *script = callee->script();
    typeMonitorEntry(script);

    if (!force && caller->types->monitored(callerpc - caller->code))
        force = true;

    
    if (!script->types || !force)
        return;

    js::types::jstype type;

    if (constructing) {
        js::Value protov;
        jsid id = ATOM_TO_JSID(runtime->atomState.classPrototypeAtom);
        if (!args.callee().toObject().getProperty(this, id, &protov))
            return;  
        if (protov.isObject()) {
            js::types::TypeObject *otype = protov.toObject().getNewType(this);
            if (!otype)
                return;  
            type = (js::types::jstype) otype;
        } else {
            type = (js::types::jstype) getTypeNewObject(JSProto_Object);
        }
    } else {
        type = js::types::GetValueType(this, args.thisv());
    }

    if (!script->types->thisTypes.hasType(type)) {
        js::types::InferSpew(js::types::ISpewDynamic, "AddThis: #%u: %s",
                             script->id(), js::types::TypeString(type));
        compartment->types.addDynamicType(this, &script->types->thisTypes, type);
    }

    




    unsigned arg = 0;
    for (; arg < args.argc() && arg < callee->nargs; arg++) {
        js::types::jstype type = js::types::GetValueType(this, args[arg]);
        js::types::TypeSet *types = script->types->argTypes(arg);
        if (!types->hasType(type)) {
            js::types::InferSpew(js::types::ISpewDynamic, "AddArg: #%u %u: %s",
                                 script->id(), arg, js::types::TypeString(type));
            compartment->types.addDynamicType(this, types, type);
        }
    }

    
    for (; arg < callee->nargs; arg++) {
        js::types::TypeSet *types = script->types->argTypes(arg);
        if (!types->hasType(js::types::TYPE_UNDEFINED)) {
            js::types::InferSpew(js::types::ISpewDynamic,
                                 "UndefinedArg: #%u %u:", script->id(), arg);
            compartment->types.addDynamicType(this, types, js::types::TYPE_UNDEFINED);
        }
    }
#endif
}

inline void
JSContext::typeMonitorEntry(JSScript *script)
{
#ifdef JS_TYPE_INFERENCE
    if (!script->types) {
        compartment->types.interpreting = false;
        uint64_t startTime = compartment->types.currentTime();

        js::types::InferSpew(js::types::ISpewDynamic, "EntryPoint: #%lu", script->id());

        js::types::AnalyzeTypes(this, script);

        uint64_t endTime = compartment->types.currentTime();
        compartment->types.analysisTime += (endTime - startTime);
        compartment->types.interpreting = true;

        if (compartment->types.hasPendingRecompiles())
            compartment->types.processPendingRecompiles(this);
    }
#endif
}

inline void
JSContext::typeMonitorEntry(JSScript *script, const js::Value &thisv)
{
#ifdef JS_TYPE_INFERENCE
    typeMonitorEntry(script);
    if (!script->types)
        return;

    js::types::jstype type = js::types::GetValueType(this, thisv);
    if (!script->types->thisTypes.hasType(type)) {
        js::types::InferSpew(js::types::ISpewDynamic, "AddThis: #%u: %s",
                             script->id(), js::types::TypeString(type));
        compartment->types.addDynamicType(this, &script->types->thisTypes, type);
    }

    typeMonitorEntry(script);
#endif
}





#ifdef JS_TYPE_INFERENCE

inline JSObject *
JSScript::getGlobal()
{
    JS_ASSERT(compileAndGo);
    if (global)
        return global;

    




    JSScript *nested = parent;
    while (true) {
        JS_ASSERT(nested->compileAndGo);
        if (nested->global) {
            global = nested->global;
            return global;
        }
        nested = nested->parent;
    }
    return NULL;
}

inline js::types::TypeObject *
JSScript::getGlobalType()
{
    return getGlobal()->getType();
}

inline js::types::TypeObject *
JSScript::getTypeNewObject(JSContext *cx, JSProtoKey key)
{
    JSObject *proto;
    if (!js_GetClassPrototype(cx, getGlobal(), key, &proto, NULL))
        return NULL;
    return proto->getNewType(cx);
}

#endif 

inline void
JSScript::setTypeNesting(JSScript *parent, const jsbytecode *pc)
{
#ifdef JS_TYPE_INFERENCE
    this->parent = parent;
#endif
}

inline void
JSScript::nukeUpvarTypes(JSContext *cx)
{
#ifdef JS_TYPE_INFERENCE
    if (this->parent) {
        if (!types)
            js::types::AnalyzeTypes(cx, this);
        types->nukeUpvarTypes(cx, this);
    }
#endif
}

inline js::types::TypeObject *
JSScript::getTypeInitObject(JSContext *cx, const jsbytecode *pc, bool isArray)
{
#ifdef JS_TYPE_INFERENCE
    if (!compileAndGo || !types)
        return cx->getTypeNewObject(isArray ? JSProto_Array : JSProto_Object);

    uint32 offset = pc - code;
    js::types::TypeObject *prev = NULL, *obj = types->objects;
    while (obj) {
        if (isArray ? obj->initializerArray : obj->initializerObject) {
            if (obj->initializerOffset == offset) {
                
                if (prev) {
                    prev->next = obj->next;
                    obj->next = types->objects;
                    types->objects = obj;
                }
                return obj;
            }
        }
        prev = obj;
        obj = obj->next;
    }

    return cx->compartment->types.newInitializerTypeObject(cx, this, offset, isArray);
#else
    return cx->getTypeNewObject(isArray ? JSProto_Array : JSProto_Object);
#endif
}

inline void
JSScript::typeMonitorResult(JSContext *cx, const jsbytecode *pc, unsigned index,
                            js::types::jstype type)
{
#ifdef JS_TYPE_INFERENCE
    if (!types)
        return;

    JS_ASSERT(index < js::analyze::GetDefCount(this, pc - code));
    js::types::TypeSet *types = this->types->pushed(pc - code, index);

    if (!types->hasType(type))
        cx->compartment->types.addDynamicPush(cx, this, pc - code, index, type);
#endif
}

inline void
JSScript::typeMonitorResult(JSContext *cx, const jsbytecode *pc, unsigned index,
                            const js::Value &rval)
{
#ifdef JS_TYPE_INFERENCE
    typeMonitorResult(cx, pc, index, js::types::GetValueType(cx, rval));
#endif
}

inline void
JSScript::typeMonitorOverflow(JSContext *cx, const jsbytecode *pc, unsigned index)
{
    typeMonitorResult(cx, pc, index, js::types::TYPE_DOUBLE);
}

inline void
JSScript::typeMonitorUndefined(JSContext *cx, const jsbytecode *pc, unsigned index)
{
    typeMonitorResult(cx, pc, index, js::types::TYPE_UNDEFINED);
}

inline void
JSScript::typeMonitorAssign(JSContext *cx, const jsbytecode *pc,
                            JSObject *obj, jsid id, const js::Value &rval, bool force)
{
#ifdef JS_TYPE_INFERENCE
    if (!force && types && !types->monitored(pc - code))
        return;

    if (!obj->getType()->unknownProperties || obj->isWith())
        cx->compartment->types.dynamicAssign(cx, obj, id, rval);
#endif
}

inline void
JSScript::typeSetArgument(JSContext *cx, unsigned arg, const js::Value &value)
{
#ifdef JS_TYPE_INFERENCE
    if (!types)
        return;
    js::types::TypeSet *argTypes = types->argTypes(arg);
    js::types::jstype type = js::types::GetValueType(cx, value);
    if (!argTypes->hasType(type)) {
        js::types::InferSpew(js::types::ISpewDynamic, "SetArgument: #%u %u: %s",
                             id(), arg, js::types::TypeString(type));
        cx->compartment->types.addDynamicType(cx, argTypes, type);
    }
#endif
}

#ifdef JS_TYPE_INFERENCE

namespace js {
namespace types {





inline void
TypeCompartment::addPending(JSContext *cx, TypeConstraint *constraint, TypeSet *source, jstype type)
{
    JS_ASSERT(this == &cx->compartment->types);
    JS_ASSERT(type);

    InferSpew(ISpewOps, "pending: C%u %s", constraint->id(), TypeString(type));

    if (pendingCount == pendingCapacity)
        growPendingArray();

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
        InferSpew(ISpewOps, "resolve: C%u %s",
                  pending.constraint->id(), TypeString(pending.type));
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
static U *&
HashSetInsertTry(JSContext *cx, U **&values, unsigned &count, T key)
{
    unsigned capacity = HashSetCapacity(count);
    unsigned insertpos = HashKey<T,KEY>(key) & (capacity - 1);

    
    bool converting = (count == SET_ARRAY_SIZE);

    if (!converting) {
        while (values[insertpos] != NULL) {
            if (KEY::getKey(values[insertpos]) == key)
                return values[insertpos];
            insertpos = (insertpos + 1) & (capacity - 1);
        }
    }

    count++;
    unsigned newCapacity = HashSetCapacity(count);

    if (newCapacity == capacity) {
        JS_ASSERT(!converting);
        return values[insertpos];
    }

    U **newValues = (U **) cx->calloc(newCapacity * sizeof(U*));

    for (unsigned i = 0; i < capacity; i++) {
        if (values[i]) {
            unsigned pos = HashKey<T,KEY>(KEY::getKey(values[i])) & (newCapacity - 1);
            while (newValues[pos] != NULL)
                pos = (pos + 1) & (newCapacity - 1);
            newValues[pos] = values[i];
        }
    }

    if (values)
        cx->free(values);
    values = newValues;

    insertpos = HashKey<T,KEY>(key) & (newCapacity - 1);
    while (values[insertpos] != NULL)
        insertpos = (insertpos + 1) & (newCapacity - 1);
    return values[insertpos];
}





template <class T, class U, class KEY>
static inline U *&
HashSetInsert(JSContext *cx, U **&values, unsigned &count, T key)
{
    if (count == 0) {
        JS_ASSERT(values == NULL);
        count++;
        U **pvalues = (U **) &values;
        return *pvalues;
    }

    if (count == 1) {
        U *oldData = (U*) values;
        if (KEY::getKey(oldData) == key) {
            U **pvalues = (U **) &values;
            return *pvalues;
        }

        values = (U **) cx->calloc(SET_ARRAY_SIZE * sizeof(U*));
        count++;

        values[0] = oldData;
        return values[1];
    }

    if (count <= SET_ARRAY_SIZE) {
        for (unsigned i = 0; i < count; i++) {
            if (KEY::getKey(values[i]) == key)
                return values[i];
        }

        if (count < SET_ARRAY_SIZE) {
            count++;
            return values[count - 1];
        }
    }

    return HashSetInsertTry<T,U,KEY>(cx, values, count, key);
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

inline bool
TypeSet::hasType(jstype type)
{
    if (unknown())
        return true;

    if (TypeIsPrimitive(type)) {
        return ((1 << type) & typeFlags) != 0;
    } else {
        return HashSetLookup<TypeObject*,TypeObject,TypeObjectKey>
            (objectSet, objectCount, (TypeObject *) type) != NULL;
    }
}

inline void
TypeSet::addType(JSContext *cx, jstype type)
{
    JS_ASSERT(type);
    JS_ASSERT_IF(unknown(), typeFlags == TYPE_FLAG_UNKNOWN);
    InferSpew(ISpewOps, "addType: T%u %s", id(), TypeString(type));

    if (unknown())
        return;

    if (type == TYPE_UNKNOWN) {
        typeFlags = TYPE_FLAG_UNKNOWN;
    } else if (TypeIsPrimitive(type)) {
        TypeFlags flag = 1 << type;
        if (typeFlags & flag)
            return;

        
        if (flag == TYPE_FLAG_DOUBLE)
            flag |= TYPE_FLAG_INT32;

        typeFlags |= flag;

#ifdef JS_TYPES_TEST_POLYMORPHISM
        
        if (!(flag & (TYPE_FLAG_UNDEFINED | TYPE_FLAG_NULL)) &&
            (typeFlags & ~flag & ~(TYPE_FLAG_UNDEFINED | TYPE_FLAG_NULL))) {
            typeFlags = TYPE_FLAG_UNKNOWN;
            type = TYPE_UNKNOWN;
        }
#endif
    } else {
        TypeObject *object = (TypeObject*) type;
        TypeObject *&entry = HashSetInsert<TypeObject *,TypeObject,TypeObjectKey>
                                 (cx, objectSet, objectCount, object);
        if (entry)
            return;
        entry = object;

        typeFlags |= TYPE_FLAG_OBJECT;

#ifdef JS_TYPES_TEST_POLYMORPHISM
        




        if ((typeFlags & ~(TYPE_FLAG_UNDEFINED | TYPE_FLAG_NULL | TYPE_FLAG_OBJECT)) ||
            (objectCount >= 2 &&
             (!UseDuplicateObjects(object) ||
              (objectCount == 2 && !UseDuplicateObjects(objectSet[0]))))) {
            typeFlags = TYPE_FLAG_UNKNOWN;
            type = TYPE_UNKNOWN;
        }
#endif
    }

    
    TypeConstraint *constraint = constraintList;
    while (constraint) {
        cx->compartment->types.addPending(cx, constraint, this, type);
        constraint = constraint->next;
    }

    cx->compartment->types.resolvePending(cx);
}

inline TypeSet *
TypeSet::make(JSContext *cx, JSArenaPool &pool, const char *name)
{
    TypeSet *res = ArenaNew<TypeSet>(pool, &pool);
    InferSpew(ISpewOps, "intermediate %s T%u", name, res->id());

    return res;
}





inline
TypeCallsite::TypeCallsite(JSScript *script, const jsbytecode *pc,
                           bool isNew, unsigned argumentCount)
    : script(script), pc(pc), isNew(isNew), argumentCount(argumentCount),
      thisTypes(NULL), thisType(0), returnTypes(NULL)
{
    argumentTypes = ArenaArray<TypeSet*>(script->types->pool, argumentCount);
}

inline void
TypeCallsite::forceThisTypes(JSContext *cx)
{
    if (thisTypes)
        return;
    thisTypes = TypeSet::make(cx, script->types->pool, "site_this");
    thisTypes->addType(cx, thisType);
}

inline void
TypeCallsite::forceReturnTypes(JSContext *cx)
{
    if (returnTypes)
        return;
    returnTypes = TypeSet::make(cx, script->types->pool, "site_return");
}

inline TypeObject *
TypeCallsite::getInitObject(JSContext *cx, bool isArray)
{
    return script->getTypeInitObject(cx, pc, isArray);
}

inline JSArenaPool &
TypeCallsite::pool()
{
    return script->types->pool;
}

inline bool
TypeCallsite::compileAndGo()
{
    return script->compileAndGo;
}





inline TypeSet *
TypeObject::getProperty(JSContext *cx, jsid id, bool assign)
{
    JS_ASSERT(JSID_IS_VOID(id) || JSID_IS_EMPTY(id) || JSID_IS_STRING(id));
    JS_ASSERT_IF(JSID_IS_STRING(id), JSID_TO_STRING(id) != NULL);

    Property *&prop = HashSetInsert<jsid,Property,Property>(cx, propertySet, propertyCount, id);
    if (!prop)
        addProperty(cx, id, prop);

    return assign ? &prop->ownTypes : &prop->types;
}





inline bool
TypeScript::monitored(uint32 offset)
{
    JS_ASSERT(offset < script->length);
    return 0x1 & (size_t) pushedArray[offset];
}

inline void
TypeScript::setMonitored(uint32 offset)
{
    JS_ASSERT(offset < script->length);
    pushedArray[offset] = (TypeSet *) (0x1 | (size_t) pushedArray[offset]);
}

inline TypeSet *
TypeScript::pushed(uint32 offset)
{
    JS_ASSERT(offset < script->length);
    return (TypeSet *) (~0x1 & (size_t) pushedArray[offset]);
}

inline TypeSet *
TypeScript::pushed(uint32 offset, uint32 index)
{
    JS_ASSERT(offset < script->length);
    JS_ASSERT(index < js::analyze::GetDefCount(script, offset));
    return pushed(offset) + index;
}

inline TypeSet *
TypeScript::argTypes(uint32 arg)
{
    JS_ASSERT(script->fun && arg < script->fun->nargs);
    return &argTypes_[arg];
}

inline TypeSet *
TypeScript::localTypes(uint32 local)
{
    JS_ASSERT(local < script->nfixed);
    return &localTypes_[local];
}

inline void
TypeScript::addType(JSContext *cx, uint32 offset, uint32 index, jstype type)
{
    TypeSet *types = pushed(offset, index);
    types->addType(cx, type);
}

} } 

#endif 

namespace js {
namespace types {

inline const char *
TypeObject::name()
{
#ifdef DEBUG
    return TypeIdString(name_);
#else
    return NULL;
#endif
}

inline TypeObject::TypeObject(JSArenaPool *pool, jsid name, JSObject *proto)
    : proto(proto), emptyShapes(NULL), isFunction(false), marked(false),
      initializerObject(false), initializerArray(false), initializerOffset(0),
      propertySet(NULL), propertyCount(0),
      instanceList(NULL), instanceNext(NULL), pool(pool), next(NULL), unknownProperties(false),
      isDenseArray(false), isPackedArray(false)
{
#ifdef DEBUG
    this->name_ = name;
#endif

#ifdef JS_TYPE_INFERENCE
    InferSpew(ISpewOps, "newObject: %s", this->name());
#endif

    if (proto) {
        TypeObject *prototype = proto->getType();
        if (prototype->unknownProperties) {
            unknownProperties = true;
        } else if (proto->isArray()) {
            








            isDenseArray = isPackedArray = true;
        }
        instanceNext = prototype->instanceList;
        prototype->instanceList = this;
    }
}

inline TypeFunction::TypeFunction(JSArenaPool *pool, jsid name, JSObject *proto)
    : TypeObject(pool, name, proto), handler(NULL), script(NULL),
      returnTypes(pool), isGeneric(false)
{
    isFunction = true;

#ifdef JS_TYPE_INFERENCE
    InferSpew(ISpewOps, "newFunction: %s return T%u", this->name(), returnTypes.id());
#endif
}

} } 

#endif 
