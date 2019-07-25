








































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
    



    if (JSID_IS_INT(id))
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
#ifdef DEBUG
    unsigned depth;
#endif

    AutoEnterTypeInference(JSContext *cx, bool compiling = false)
        : cx(cx)
    {
#ifdef DEBUG
        depth = cx->compartment->types.inferenceDepth;
#endif
        JS_ASSERT_IF(!compiling, cx->compartment->types.inferenceEnabled);
        if (cx->compartment->types.inferenceDepth++ == 0)
            cx->compartment->types.inferenceStartTime = cx->compartment->types.currentTime();
    }

    ~AutoEnterTypeInference()
    {
        
        JS_ASSERT(cx->compartment->types.inferenceDepth == depth);
    }
};

bool
TypeCompartment::checkPendingRecompiles(JSContext *cx)
{
    JS_ASSERT(inferenceDepth);
    if (--inferenceDepth != 0) {
        




        return true;
    }
    if (inferenceStartTime)
        analysisTime += currentTime() - inferenceStartTime;
    inferenceStartTime = 0;
    if (pendingNukeTypes)
        return nukeTypes(cx);
    else if (pendingRecompiles && !processPendingRecompiles(cx))
        return false;
    return true;
}

bool
UseNewType(JSContext *cx, JSScript *script, jsbytecode *pc);

} } 





inline bool
JSContext::typeInferenceEnabled()
{
    return compartment->types.inferenceEnabled;
}

inline js::types::TypeObject *
JSContext::getTypeNewObject(JSProtoKey key)
{
    JSObject *proto;
    if (!js_GetClassPrototype(this, NULL, key, &proto, NULL))
        return NULL;
    return proto->getNewType(this);
}

inline js::types::TypeObject *
JSContext::getTypeCallerInitObject(bool isArray)
{
    if (typeInferenceEnabled()) {
        JSStackFrame *caller = js_GetScriptedCaller(this, NULL);
        if (caller)
            return caller->script()->getTypeInitObject(this, caller->pc(this), isArray);
    }
    return getTypeNewObject(isArray ? JSProto_Array : JSProto_Object);
}

inline bool
JSContext::markTypeCallerUnexpected(js::types::jstype type)
{
    if (!typeInferenceEnabled())
        return true;

    








    JSStackFrame *caller = js_GetScriptedCaller(this, NULL);
    if (!caller)
        return true;

    switch ((JSOp)*caller->pc(this)) {
      case JSOP_CALL:
      case JSOP_EVAL:
      case JSOP_FUNCALL:
      case JSOP_FUNAPPLY:
      case JSOP_NEW:
        break;
      default:
        return true;
    }

    return caller->script()->typeMonitorResult(this, caller->pc(this), type);
}

inline bool
JSContext::markTypeCallerUnexpected(const js::Value &value)
{
    return markTypeCallerUnexpected(js::types::GetValueType(this, value));
}

inline bool
JSContext::markTypeCallerOverflow()
{
    return markTypeCallerUnexpected(js::types::TYPE_DOUBLE);
}

inline bool
JSContext::addTypeProperty(js::types::TypeObject *obj, const char *name, js::types::jstype type)
{
    if (typeInferenceEnabled() && !obj->unknownProperties) {
        jsid id = JSID_VOID;
        if (name) {
            JSAtom *atom = js_Atomize(this, name, strlen(name), 0);
            if (!atom)
                return false;
            id = ATOM_TO_JSID(atom);
        }
        return addTypePropertyId(obj, id, type);
    }
    return true;
}

inline bool
JSContext::addTypeProperty(js::types::TypeObject *obj, const char *name, const js::Value &value)
{
    if (typeInferenceEnabled() && !obj->unknownProperties)
        return addTypeProperty(obj, name, js::types::GetValueType(this, value));
    return true;
}

inline bool
JSContext::addTypePropertyId(js::types::TypeObject *obj, jsid id, js::types::jstype type)
{
    if (!typeInferenceEnabled() || obj->unknownProperties)
        return true;

    
    id = js::types::MakeTypeId(this, id);

    js::types::AutoEnterTypeInference enter(this);

    js::types::TypeSet *types = obj->getProperty(this, id, true);
    if (!types || types->hasType(type))
        return compartment->types.checkPendingRecompiles(this);

    js::types::InferSpew(js::types::ISpewOps, "externalType: property %s %s: %s",
                         obj->name(), js::types::TypeIdString(id),
                         js::types::TypeString(type));
    types->addType(this, type);

    return compartment->types.checkPendingRecompiles(this);
}

inline bool
JSContext::addTypePropertyId(js::types::TypeObject *obj, jsid id, const js::Value &value)
{
    if (typeInferenceEnabled() && !obj->unknownProperties)
        return addTypePropertyId(obj, id, js::types::GetValueType(this, value));
    return true;
}

inline bool
JSContext::addTypePropertyId(js::types::TypeObject *obj, jsid id, js::types::ClonedTypeSet *set)
{
    if (obj->unknownProperties)
        return true;
    id = js::types::MakeTypeId(this, id);

    js::types::AutoEnterTypeInference enter(this);

    js::types::TypeSet *types = obj->getProperty(this, id, true);
    if (!types)
        return compartment->types.checkPendingRecompiles(this);

    js::types::InferSpew(js::types::ISpewOps, "externalType: property %s %s",
                         obj->name(), js::types::TypeIdString(id));
    types->addTypeSet(this, set);

    return compartment->types.checkPendingRecompiles(this);
}

inline js::types::TypeObject *
JSContext::getTypeEmpty()
{
    return &compartment->types.typeEmpty;
}

inline bool
JSContext::aliasTypeProperties(js::types::TypeObject *obj, jsid first, jsid second)
{
    if (!typeInferenceEnabled() || obj->unknownProperties)
        return true;

    js::types::AutoEnterTypeInference enter(this);

    first = js::types::MakeTypeId(this, first);
    second = js::types::MakeTypeId(this, second);

    js::types::TypeSet *firstTypes = obj->getProperty(this, first, true);
    js::types::TypeSet *secondTypes = obj->getProperty(this, second, true);
    if (!firstTypes || !secondTypes)
        return false;

    firstTypes->addBaseSubset(this, obj, secondTypes);
    secondTypes->addBaseSubset(this, obj, firstTypes);

    return compartment->types.checkPendingRecompiles(this);
}

inline bool
JSContext::markTypeArrayNotPacked(js::types::TypeObject *obj, bool notDense)
{
    if (!typeInferenceEnabled() || (notDense ? !obj->isDenseArray : !obj->isPackedArray))
        return true;
    js::types::AutoEnterTypeInference enter(this);

    obj->markNotPacked(this, notDense);

    return compartment->types.checkPendingRecompiles(this);
}

bool
JSContext::markTypeObjectUnknownProperties(js::types::TypeObject *obj)
{
    if (!typeInferenceEnabled() || obj->unknownProperties)
        return true;

    js::types::AutoEnterTypeInference enter(this);
    obj->markUnknown(this);
    return compartment->types.checkPendingRecompiles(this);
}

inline bool
JSContext::typeMonitorAssign(JSObject *obj, jsid id, const js::Value &rval)
{
    if (typeInferenceEnabled())
        return compartment->types.dynamicAssign(this, obj, id, rval);
    return true;
}

inline bool
JSContext::typeMonitorCall(const js::CallArgs &args, bool constructing)
{
    if (!typeInferenceEnabled() || !args.callee().isObject())
        return true;

    JSObject *callee = &args.callee().toObject();
    if (!callee->isFunction() || !callee->getFunctionPrivate()->isInterpreted())
        return true;

    return compartment->types.dynamicCall(this, callee, args, constructing);
}

inline bool
JSContext::fixArrayType(JSObject *obj)
{
    return !typeInferenceEnabled() || compartment->types.fixArrayType(this, obj);
}

inline bool
JSContext::fixObjectType(JSObject *obj)
{
    return !typeInferenceEnabled() || compartment->types.fixObjectType(this, obj);
}





inline bool
JSScript::ensureVarTypes(JSContext *cx)
{
    if (varTypes)
        return true;
    return makeVarTypes(cx);
}

inline js::types::TypeSet *
JSScript::returnTypes()
{
    JS_ASSERT(varTypes);
    return &varTypes[0];
}

inline js::types::TypeSet *
JSScript::thisTypes()
{
    JS_ASSERT(varTypes);
    return &varTypes[1];
}

inline js::types::TypeSet *
JSScript::argTypes(unsigned i)
{
    JS_ASSERT(varTypes && fun && i < fun->nargs);
    return &varTypes[2 + i];
}

inline js::types::TypeSet *
JSScript::localTypes(unsigned i)
{
    JS_ASSERT(varTypes && i < nfixed);
    if (fun)
        i += fun->nargs;
    return &varTypes[2 + i];
}

inline js::types::TypeSet *
JSScript::upvarTypes(unsigned i)
{
    JS_ASSERT(varTypes && i < bindings.countUpvars());
    if (fun)
        i += fun->nargs;
    return &varTypes[2 + nfixed + i];
}

inline JSObject *
JSScript::getGlobal()
{
    JS_ASSERT(compileAndGo && global);
    return global;
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

inline js::types::TypeObject *
JSScript::getTypeInitObject(JSContext *cx, const jsbytecode *pc, bool isArray)
{
    if (!cx->typeInferenceEnabled() || !compileAndGo)
        return cx->getTypeNewObject(isArray ? JSProto_Array : JSProto_Object);

    uint32 offset = pc - code;
    js::types::TypeObject *prev = NULL, *obj = typeObjects;
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

    return cx->compartment->types.newInitializerTypeObject(cx, this, offset, isArray);
}

inline bool
JSScript::typeMonitorResult(JSContext *cx, const jsbytecode *pc,
                            js::types::jstype type)
{
    if (cx->typeInferenceEnabled())
        return cx->compartment->types.dynamicPush(cx, this, pc - code, type);
    return true;
}

inline bool
JSScript::typeMonitorResult(JSContext *cx, const jsbytecode *pc, const js::Value &rval)
{
    if (cx->typeInferenceEnabled())
        return typeMonitorResult(cx, pc, js::types::GetValueType(cx, rval));
    return true;
}

inline bool
JSScript::typeMonitorOverflow(JSContext *cx, const jsbytecode *pc)
{
    return typeMonitorResult(cx, pc, js::types::TYPE_DOUBLE);
}

inline bool
JSScript::typeMonitorUndefined(JSContext *cx, const jsbytecode *pc)
{
    return typeMonitorResult(cx, pc, js::types::TYPE_UNDEFINED);
}

inline bool
JSScript::typeMonitorUnknown(JSContext *cx, const jsbytecode *pc)
{
    return typeMonitorResult(cx, pc, js::types::TYPE_UNKNOWN);
}

inline bool
JSScript::typeSetThis(JSContext *cx, js::types::jstype type)
{
    JS_ASSERT(cx->typeInferenceEnabled());
    if (!ensureVarTypes(cx))
        return false;

    
    bool analyze = !types && cx->hasRunOption(JSOPTION_METHODJIT_ALWAYS) && !isUncachedEval;

    if (!thisTypes()->hasType(type) || analyze) {
        js::types::AutoEnterTypeInference enter(cx);

        js::types::InferSpew(js::types::ISpewOps, "externalType: setThis #%u: %s",
                             id(), js::types::TypeString(type));
        thisTypes()->addType(cx, type);

        if (analyze && !types)
            js::types::AnalyzeScriptTypes(cx, this);

        return cx->compartment->types.checkPendingRecompiles(cx);
    }

    return true;
}

inline bool
JSScript::typeSetThis(JSContext *cx, const js::Value &value)
{
    if (cx->typeInferenceEnabled())
        return typeSetThis(cx, js::types::GetValueType(cx, value));
    return true;
}

inline bool
JSScript::typeSetThis(JSContext *cx, js::types::ClonedTypeSet *set)
{
    if (!ensureVarTypes(cx))
        return false;
    js::types::AutoEnterTypeInference enter(cx);

    js::types::InferSpew(js::types::ISpewOps, "externalType: setThis #%u: %s", id());
    thisTypes()->addTypeSet(cx, set);

    return cx->compartment->types.checkPendingRecompiles(cx);
}

inline bool
JSScript::typeSetNewCalled(JSContext *cx)
{
    if (!cx->typeInferenceEnabled() || calledWithNew)
        return true;
    calledWithNew = true;

    






    if (analyzed) {
        
        js::types::AutoEnterTypeInference enter(cx);
        js::types::AnalyzeScriptNew(cx, this);
        if (!cx->compartment->types.checkPendingRecompiles(cx))
            return false;
    }
    return true;
}

inline bool
JSScript::typeSetLocal(JSContext *cx, unsigned local, js::types::jstype type)
{
    if (!cx->typeInferenceEnabled())
        return true;
    if (!ensureVarTypes(cx))
        return false;
    if (!localTypes(local)->hasType(type)) {
        js::types::AutoEnterTypeInference enter(cx);

        js::types::InferSpew(js::types::ISpewOps, "externalType: setLocal #%u %u: %s",
                             id(), local, js::types::TypeString(type));
        localTypes(local)->addType(cx, type);

        return compartment->types.checkPendingRecompiles(cx);
    }
    return true;
}

inline bool
JSScript::typeSetLocal(JSContext *cx, unsigned local, const js::Value &value)
{
    if (cx->typeInferenceEnabled()) {
        js::types::jstype type = js::types::GetValueType(cx, value);
        return typeSetLocal(cx, local, type);
    }
    return true;
}

inline bool
JSScript::typeSetLocal(JSContext *cx, unsigned local, js::types::ClonedTypeSet *set)
{
    if (!ensureVarTypes(cx))
        return false;
    js::types::AutoEnterTypeInference enter(cx);

    js::types::InferSpew(js::types::ISpewOps, "externalType: setLocal #%u %u", id(), local);
    localTypes(local)->addTypeSet(cx, set);

    return compartment->types.checkPendingRecompiles(cx);
}

inline bool
JSScript::typeSetArgument(JSContext *cx, unsigned arg, js::types::jstype type)
{
    if (!cx->typeInferenceEnabled())
        return true;
    if (!ensureVarTypes(cx))
        return false;
    if (!argTypes(arg)->hasType(type)) {
        js::types::AutoEnterTypeInference enter(cx);

        js::types::InferSpew(js::types::ISpewOps, "externalType: setArg #%u %u: %s",
                             id(), arg, js::types::TypeString(type));
        argTypes(arg)->addType(cx, type);

        return cx->compartment->types.checkPendingRecompiles(cx);
    }
    return true;
}

inline bool
JSScript::typeSetArgument(JSContext *cx, unsigned arg, const js::Value &value)
{
    if (cx->typeInferenceEnabled()) {
        js::types::jstype type = js::types::GetValueType(cx, value);
        return typeSetArgument(cx, arg, type);
    }
    return true;
}

inline bool
JSScript::typeSetArgument(JSContext *cx, unsigned arg, js::types::ClonedTypeSet *set)
{
    if (!ensureVarTypes(cx))
        return false;
    js::types::AutoEnterTypeInference enter(cx);

    js::types::InferSpew(js::types::ISpewOps, "externalType: setArg #%u %u", id(), arg);
    argTypes(arg)->addTypeSet(cx, set);

    return cx->compartment->types.checkPendingRecompiles(cx);
}

inline bool
JSScript::typeSetUpvar(JSContext *cx, unsigned upvar, const js::Value &value)
{
    if (!cx->typeInferenceEnabled())
        return true;
    if (!ensureVarTypes(cx))
        return false;
    js::types::jstype type = js::types::GetValueType(cx, value);
    if (!upvarTypes(upvar)->hasType(type)) {
        js::types::AutoEnterTypeInference enter(cx);

        js::types::InferSpew(js::types::ISpewOps, "externalType: setUpvar #%u %u: %s",
                             id(), upvar, js::types::TypeString(type));
        upvarTypes(upvar)->addType(cx, type);

        return cx->compartment->types.checkPendingRecompiles(cx);
    }
    return true;
}

namespace js {
namespace types {





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
        ? ArenaArray<U*>(cx->compartment->types.pool, newCapacity)
        : (U **) ::js_malloc(newCapacity * sizeof(U*));
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
        ::js_free(values);
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
            ? ArenaArray<U*>(cx->compartment->types.pool, SET_ARRAY_SIZE)
            : (U **) ::js_malloc(SET_ARRAY_SIZE * sizeof(U*));
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
    JS_ASSERT(!(typeFlags & TYPE_FLAG_INTERMEDIATE_SET));
    if (objectCount >= 2)
        ::js_free(objectSet);
    while (constraintList) {
        TypeConstraint *next = constraintList->next;
        if (constraintList->condensed() || constraintList->baseSubset())
            ::js_free(constraintList);
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
    typeFlags = TYPE_FLAG_UNKNOWN | (typeFlags & TYPE_FLAG_INTERMEDIATE_SET);
    if (objectCount >= 2 && !(typeFlags & TYPE_FLAG_INTERMEDIATE_SET))
        cx->free(objectSet);
    objectCount = 0;
    objectSet = NULL;
}

inline void
TypeSet::addType(JSContext *cx, jstype type)
{
    JS_ASSERT(type);
    JS_ASSERT(cx->compartment->types.inferenceDepth);
    InferSpew(ISpewOps, "addType: T%p %s", this, TypeString(type));

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
                                  (cx, objectSet, objectCount, object,
                                   typeFlags & TYPE_FLAG_INTERMEDIATE_SET);
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

    
    TypeConstraint *constraint = constraintList;
    while (constraint) {
        cx->compartment->types.addPending(cx, constraint, this, type);
        constraint = constraint->next;
    }

    cx->compartment->types.resolvePending(cx);
}

inline TypeSet *
TypeSet::make(JSContext *cx, const char *name)
{
    JS_ASSERT(cx->compartment->types.inferenceDepth);

    TypeSet *res = ArenaNew<TypeSet>(cx->compartment->types.pool);
    if (!res) {
        cx->compartment->types.setPendingNukeTypes(cx);
        return NULL;
    }

    InferSpew(ISpewOps, "typeSet: T%p intermediate %s", res, name);
    res->setIntermediate();

    return res;
}





inline
TypeCallsite::TypeCallsite(JSContext *cx, JSScript *script, const jsbytecode *pc,
                           bool isNew, unsigned argumentCount)
    : script(script), pc(pc), isNew(isNew), argumentCount(argumentCount),
      thisTypes(NULL), thisType(0), returnTypes(NULL)
{
    
    argumentTypes = ArenaArray<TypeSet*>(cx->compartment->types.pool, argumentCount);
}

inline bool
TypeCallsite::forceThisTypes(JSContext *cx)
{
    if (thisTypes)
        return true;
    thisTypes = TypeSet::make(cx, "site_this");
    if (thisTypes)
        thisTypes->addType(cx, thisType);
    return thisTypes != NULL;
}

inline TypeObject *
TypeCallsite::getInitObject(JSContext *cx, bool isArray)
{
    TypeObject *type = script->getTypeInitObject(cx, pc, isArray);
    if (!type)
        cx->compartment->types.setPendingNukeTypes(cx);
    return type;
}

inline bool
TypeCallsite::compileAndGo()
{
    return script->compileAndGo;
}





inline TypeSet *
TypeObject::getProperty(JSContext *cx, jsid id, bool assign)
{
    JS_ASSERT(cx->compartment->types.inferenceDepth);
    JS_ASSERT(JSID_IS_VOID(id) || JSID_IS_EMPTY(id) || JSID_IS_STRING(id));
    JS_ASSERT_IF(JSID_IS_STRING(id), JSID_TO_STRING(id) != NULL);
    JS_ASSERT(!unknownProperties);

    Property **pprop = HashSetInsert<jsid,Property,Property>
                           (cx, propertySet, propertyCount, id, false);
    if (!pprop || (!*pprop && !addProperty(cx, id, pprop)))
        return NULL;

    return assign ? &(*pprop)->ownTypes : &(*pprop)->types;
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
    JS_ASSERT(script->compartment->types.inferenceDepth);
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

inline void
TypeScript::addType(JSContext *cx, uint32 offset, uint32 index, jstype type)
{
    TypeSet *types = pushed(offset, index);
    types->addType(cx, type);
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
    : proto(proto), emptyShapes(NULL), isFunction(false), marked(false),
      initializerObject(false), initializerArray(false), initializerOffset(0),
      contribution(0), propertySet(NULL), propertyCount(0),
      instanceList(NULL), instanceNext(NULL), next(NULL), unknownProperties(false),
      isDenseArray(false), isPackedArray(false)
{
#ifdef DEBUG
    this->name_ = name;
#endif

    InferSpew(ISpewOps, "newObject: %s", this->name());

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
            TypeObject *obj = (TypeObject *) types->objectSet;
            ::js_free(types->objectSet);
            types->objectSet = (TypeObject **) obj;
        } else if (types->objectCount == 0) {
            ::js_free(types->objectSet);
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

} } 

#endif 
