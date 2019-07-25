








































#include "jsanalyze.h"
#include "jscompartment.h"
#include "jsinfer.h"
#include "jsprf.h"

#ifndef jsinferinlines_h___
#define jsinferinlines_h___





#ifdef JS_TYPE_INFERENCE

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
        JS_ASSERT(obj->typeObject);
        return (jstype) obj->typeObject;
      }
      default:
        JS_NOT_REACHED("Unknown value");
        return (jstype) 0;
    }
}






inline jsid
MakeTypeId(jsid id)
{
    if (JSID_IS_VOID(id))
        return JSID_VOID;

    



    if (JSID_IS_INT(id))
        return JSID_VOID;

    
    if (JSID_IS_OBJECT(id))
        return JSID_VOID;

    




    if (JSID_IS_STRING(id)) {
        JSString *str = JSID_TO_STRING(id);
        jschar *cp = str->chars();
        if (JS7_ISDEC(*cp) || *cp == '-') {
            cp++;
            while (JS7_ISDEC(*cp))
                cp++;
            if (unsigned(cp - str->chars()) == str->length())
                return JSID_VOID;
        }
        return id;
    }

    JS_NOT_REACHED("Unknown id");
    return JSID_VOID;
}


static inline const char *
TypeIdString(jsid id)
{
#ifdef DEBUG
    if (JSID_IS_VOID(id))
        return "(index)";
    return js_GetStringBytes(JSID_TO_ATOM(id));
#else
    return "(missing)";
#endif
}

} } 

#endif 





inline js::types::TypeObject *
JSContext::getTypeFunction(const char *name, js::types::TypeObject *prototype)
{
#ifdef JS_TYPE_INFERENCE
    if (!prototype)
        prototype = getFixedTypeObject(js::types::TYPE_OBJECT_FUNCTION_PROTOTYPE);
    return compartment->types.getTypeObject(this, NULL, name, true, prototype);
#else
    return NULL;
#endif
}

inline js::types::TypeObject *
JSContext::getTypeObject(const char *name, js::types::TypeObject *prototype)
{
#ifdef JS_TYPE_INFERENCE
    if (!prototype)
        prototype = getFixedTypeObject(js::types::TYPE_OBJECT_OBJECT_PROTOTYPE);
    return compartment->types.getTypeObject(this, NULL, name, false, prototype);
#else
    return NULL;
#endif
}

inline js::types::TypeObject *
JSContext::getGlobalTypeObject()
{
#ifdef JS_TYPE_INFERENCE
    if (!compartment->types.globalObject) {
        js::types::TypeObject *prototype = getFixedTypeObject(js::types::TYPE_OBJECT_OBJECT_PROTOTYPE);
        compartment->types.globalObject = getTypeObject("Global", prototype);
    }
    return compartment->types.globalObject;
#else
    return NULL;
#endif
}

inline void
JSContext::setTypeFunctionScript(JSFunction *fun, JSScript *script)
{
#ifdef JS_TYPE_INFERENCE
    char name[8];
    JS_snprintf(name, 16, "#%u", script->analysis->id);

    js::types::TypeObject *proto = getFixedTypeObject(js::types::TYPE_OBJECT_FUNCTION_PROTOTYPE);
    js::types::TypeFunction *typeFun =
        compartment->types.getTypeObject(this, script->analysis, name, true, proto)->asFunction();

    
    if (typeFun->script) {
        JS_ASSERT(typeFun->script == script);
        fun->typeObject = typeFun;
        return;
    }

    typeFun->script = script;
    fun->typeObject = typeFun;

    script->analysis->setFunction(this, fun);
#endif
}

inline js::types::TypeFunction *
JSContext::getTypeFunctionHandler(const char *name, JSTypeHandler handler,
                                  js::types::TypeObject *prototype)
{
#ifdef JS_TYPE_INFERENCE
    if (!prototype)
        prototype = getFixedTypeObject(js::types::TYPE_OBJECT_FUNCTION_PROTOTYPE);
    js::types::TypeFunction *typeFun =
        compartment->types.getTypeObject(this, NULL, name, true, prototype)->asFunction();

    if (typeFun->handler) {
        
        JS_ASSERT(typeFun->handler == handler);
        return typeFun;
    }

    typeFun->handler = handler;
    return typeFun;
#else
    return NULL;
#endif
}

inline js::types::TypeObject *
JSContext::getTypeCallerInitObject(bool isArray)
{
#ifdef JS_TYPE_INFERENCE
    JSStackFrame *caller = js_GetScriptedCaller(this, NULL);
    return caller->script()->getTypeInitObject(this, caller->pc(this), isArray);
#else
    return NULL;
#endif
}

inline bool
JSContext::isTypeCallerMonitored()
{
#ifdef JS_TYPE_INFERENCE
    JSStackFrame *caller = js_GetScriptedCaller(this, NULL);
    js::analyze::Script *analysis = caller->script()->analysis;
    return analysis->failed() || analysis->getCode(caller->pc(this)).monitorNeeded;
#else
    return false;
#endif
}

inline void
JSContext::markTypeCallerUnexpected(js::types::jstype type)
{
#ifdef JS_TYPE_INFERENCE
    JSStackFrame *caller = js_GetScriptedCaller(this, NULL);
    caller->script()->typeMonitorResult(this, caller->pc(this), 0, type);
#endif
}

inline void
JSContext::markTypeCallerUnexpected(const js::Value &value)
{
#ifdef JS_TYPE_INFERENCE
    markTypeCallerUnexpected(js::types::GetValueType(this, value));
#endif
}

inline void
JSContext::markTypeCallerOverflow()
{
    markTypeCallerUnexpected(js::types::TYPE_DOUBLE);
}

inline void
JSContext::markTypeBuiltinFunction(js::types::TypeObject *fun)
{
#ifdef JS_TYPE_INFERENCE
    JS_ASSERT(fun->isFunction);
    fun->asFunction()->isBuiltin = true;
#endif
}

inline void
JSContext::setTypeFunctionPrototype(js::types::TypeObject *fun, js::types::TypeObject *proto)
{
#ifdef JS_TYPE_INFERENCE
    js::types::TypeFunction *nfun = fun->asFunction();
    JS_ASSERT(nfun->isBuiltin);

    if (nfun->prototypeObject) {
        JS_ASSERT(nfun->prototypeObject == proto);
        return;
    }

    nfun->prototypeObject = proto;
    addTypePropertyId(fun, ATOM_TO_JSID(runtime->atomState.classPrototypeAtom), (js::types::jstype) proto);
#endif
}

inline void
JSContext::addTypeProperty(js::types::TypeObject *obj, const char *name, js::types::jstype type)
{
#ifdef JS_TYPE_INFERENCE
    jsid id = JSID_VOID;
    if (name)
        id = ATOM_TO_JSID(js_Atomize(this, name, strlen(name), 0));
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
    
    id = js::types::MakeTypeId(id);

    js::types::TypeSet *types = obj->getProperty(this, id, true);

    if (types->hasType(type))
        return;

    if (compartment->types.interpreting) {
        js::types::InferSpew(js::types::ISpewDynamic, "AddBuiltin: %s %s: %s",
                             js::types::TypeIdString(obj->name),
                             js::types::TypeIdString(id),
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
JSContext::aliasTypeProperties(js::types::TypeObject *obj, jsid first, jsid second)
{
#ifdef JS_TYPE_INFERENCE
    first = js::types::MakeTypeId(first);
    second = js::types::MakeTypeId(second);

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
                             notDense ? "NonDenseArray" : "NonPackedArray",
                             js::types::TypeIdString(obj->name));
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
    JSObject *callee = &args.callee().toObject();
    js::types::TypeFunction *fun = callee->getTypeObject()->asFunction();

    




    if (!fun->script)
        return;
    js::analyze::Script *script = fun->script->analysis;

    if (!force) {
        if (caller->analysis->failed() || caller->analysis->getCode(callerpc).monitorNeeded)
            force = true;
    }

    typeMonitorEntry(fun->script);

    
    if (!force)
        return;

    js::types::jstype type;
    if (constructing) {
        js::Value protov;
        if (!callee->getProperty(this, ATOM_TO_JSID(runtime->atomState.classPrototypeAtom),
                                 &protov)) {
            JS_NOT_REACHED("FIXME");
            return;
        }
        JSObject *proto = protov.isObjectOrNull() ? protov.toObjectOrNull() : NULL;
        js::types::TypeObject *protoType = proto
            ? proto->getTypeObject()
            : getFixedTypeObject(js::types::TYPE_OBJECT_OBJECT_PROTOTYPE);
        type = (js::types::jstype) protoType->getNewObject(this);
    } else {
        type = js::types::GetValueType(this, args.thisv());
    }
    if (!script->thisTypes.hasType(type)) {
        js::types::InferSpew(js::types::ISpewDynamic, "AddThis: #%u: %s",
                             script->id, js::types::TypeString(type));
        compartment->types.addDynamicType(this, &script->thisTypes, type);
    }

    unsigned arg = 0;
    for (; arg < args.argc(); arg++) {
        js::types::jstype type = js::types::GetValueType(this, args[arg]);

        jsid id = script->getArgumentId(arg);
        if (!JSID_IS_VOID(id)) {
            js::types::TypeSet *types = script->getVariable(this, id);
            if (!types->hasType(type)) {
                js::types::InferSpew(js::types::ISpewDynamic, "AddArg: #%u %u: %s",
                                     script->id, arg, js::types::TypeString(type));
                compartment->types.addDynamicType(this, types, type);
            }
        } else {
            




        }
    }

    
    for (; arg < script->argCount(); arg++) {
        jsid id = script->getArgumentId(arg);
        JS_ASSERT(!JSID_IS_VOID(id));

        js::types::TypeSet *types = script->getVariable(this, id);
        if (!types->hasType(js::types::TYPE_UNDEFINED)) {
            js::types::InferSpew(js::types::ISpewDynamic,
                                 "UndefinedArg: #%u %u:", script->id, arg);
            compartment->types.addDynamicType(this, types, js::types::TYPE_UNDEFINED);
        }
    }
#endif
}

inline void
JSContext::typeMonitorEntry(JSScript *script)
{
#ifdef JS_TYPE_INFERENCE
    js::analyze::Script *analysis = script->analysis;
    JS_ASSERT(analysis);

    if (!analysis->hasAnalyzed()) {
        compartment->types.interpreting = false;
        uint64_t startTime = compartment->types.currentTime();

        js::types::InferSpew(js::types::ISpewDynamic, "EntryPoint: #%lu", analysis->id);

        analysis->analyze(this);

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
    js::analyze::Script *analysis = script->analysis;
    JS_ASSERT(analysis);

    js::types::jstype type = js::types::GetValueType(this, thisv);
    if (!analysis->thisTypes.hasType(type)) {
        js::types::InferSpew(js::types::ISpewDynamic, "AddThis: #%u: %s",
                             analysis->id, js::types::TypeString(type));
        compartment->types.addDynamicType(this, &analysis->thisTypes, type);
    }

    typeMonitorEntry(script);
#endif
}





inline void
JSScript::setTypeNesting(JSScript *parent, const jsbytecode *pc)
{
#ifdef JS_TYPE_INFERENCE
    analysis->parent = parent;
    analysis->parentpc = pc;
#endif
}

inline js::types::TypeObject *
JSScript::getTypeInitObject(JSContext *cx, const jsbytecode *pc, bool isArray)
{
#ifdef JS_TYPE_INFERENCE
    if (analysis->failed()) {
        return cx->getFixedTypeObject(isArray
                                      ? js::types::TYPE_OBJECT_UNKNOWN_ARRAY
                                      : js::types::TYPE_OBJECT_UNKNOWN_OBJECT);
    }
    return analysis->getCode(pc).getInitObject(cx, isArray);
#else
    return NULL;
#endif
}

inline void
JSScript::typeMonitorResult(JSContext *cx, const jsbytecode *pc, unsigned index,
                            js::types::jstype type)
{
#ifdef JS_TYPE_INFERENCE
    if (analysis->failed())
        return;

    js::analyze::Bytecode &code = analysis->getCode(pc);
    js::types::TypeSet *stackTypes = code.pushed(index);
    if (stackTypes->hasType(type))
        return;

    if (!stackTypes->hasType(type))
        cx->compartment->types.addDynamicPush(cx, code, index, type);
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
                            JSObject *obj, jsid id, const js::Value &rval)
{
#ifdef JS_TYPE_INFERENCE
    if (!analysis->failed()) {
        js::analyze::Bytecode &code = analysis->getCode(pc);
        if (!code.monitorNeeded)
            return;
    }

    if (!obj->getTypeObject()->unknownProperties)
        cx->compartment->types.dynamicAssign(cx, obj, id, rval);
#endif
}

inline void
JSScript::typeSetArgument(JSContext *cx, unsigned arg, const js::Value &value)
{
#ifdef JS_TYPE_INFERENCE
    jsid id = analysis->getArgumentId(arg);
    if (!JSID_IS_VOID(id)) {
        js::types::TypeSet *argTypes = analysis->getVariable(cx, id);
        js::types::jstype type = js::types::GetValueType(cx, value);
        if (!argTypes->hasType(type)) {
            js::types::InferSpew(js::types::ISpewDynamic, "SetArgument: #%u %s: %s",
                                 analysis->id, js::types::TypeIdString(id),
                                 js::types::TypeString(type));
            cx->compartment->types.addDynamicType(cx, argTypes, type);
        }
    }
#endif
}





inline js::types::TypeObject *
JSObject::getTypePrototypeNewObject(JSContext *cx)
{
#ifdef JS_TYPE_INFERENCE
    if (typeObject->newObject)
        return typeObject->newObject;
    return typeObject->getNewObject(cx);
#else
    return NULL;
#endif
}

inline js::types::TypeObject *
JSObject::getTypePrototype(JSContext *cx)
{
#ifdef JS_TYPE_INFERENCE
    if (!typeObject->asFunction()->prototypeObject)
        typeObject->getProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom), false);
    return typeObject->asFunction()->prototypeObject;
#else
    return NULL;
#endif
}





#ifdef JS_TYPE_INFERENCE

namespace js {
namespace analyze {

inline JSArenaPool &
Bytecode::pool()
{
    return script->pool;
}

inline types::TypeSet *
Bytecode::popped(unsigned num)
{
    JS_ASSERT(num < GetUseCount(script->script, offset));
    types::TypeStack *stack = inStack->group();
    for (unsigned i = 0; i < num; i++)
        stack = stack->innerStack->group();
    JS_ASSERT(stack);
    return &stack->types;
}

inline types::TypeSet *
Bytecode::pushed(unsigned num)
{
    JS_ASSERT(num < GetDefCount(script->script, offset));
    return &pushedArray[num].group()->types;
}

inline void
Bytecode::setFixed(JSContext *cx, unsigned num, types::jstype type)
{
    pushed(num)->addType(cx, type);
}

inline types::TypeObject *
Bytecode::getInitObject(JSContext *cx, bool isArray)
{
    if (isArray) {
        if (!initArray) {
            char name[32];
            JS_snprintf(name, 32, "#%u:%u:Array", script->id, offset);
            types::TypeObject *proto = cx->getFixedTypeObject(types::TYPE_OBJECT_ARRAY_PROTOTYPE);
            initArray = cx->compartment->types.getTypeObject(cx, script, name, false, proto);
        }
        return initArray;
    }
    if (!initObject) {
        char name[32];
        JS_snprintf(name, 32, "#%u:%u:Object", script->id, offset);
        types::TypeObject *proto = cx->getFixedTypeObject(types::TYPE_OBJECT_OBJECT_PROTOTYPE);
        initObject = cx->compartment->types.getTypeObject(cx, script, name, false, proto);
    }
    return initObject;
}





inline jsid
Script::getLocalId(unsigned index, Bytecode *code)
{
    if (index >= script->nfixed) {
        if (!code)
            return JSID_VOID;

        JS_ASSERT(index - script->nfixed < code->stackDepth);
        unsigned diff = code->stackDepth - (index - script->nfixed);
        types::TypeStack *stack = code->inStack;
        for (unsigned i = 1; i < diff; i++)
            stack = stack->group()->innerStack;
        JS_ASSERT(stack);

        if (stack && JSID_TO_STRING(stack->letVariable) != NULL)
            return stack->letVariable;

        




        return JSID_VOID;
    }

    if (!localNames || !localNames[argCount() + index])
        return JSID_VOID;

    return ATOM_TO_JSID(JS_LOCAL_NAME_TO_ATOM(localNames[argCount() + index]));
}

inline jsid
Script::getArgumentId(unsigned index)
{
    JS_ASSERT(fun);

    



    if (index >= argCount() || !localNames[index])
        return JSID_VOID;

    return ATOM_TO_JSID(JS_LOCAL_NAME_TO_ATOM(localNames[index]));
}

inline types::TypeSet*
Script::getStackTypes(unsigned index, Bytecode *code)
{
    JS_ASSERT(index >= script->nfixed);
    JS_ASSERT(index - script->nfixed < code->stackDepth);

    types::TypeStack *stack = code->inStack;
    unsigned diff = code->stackDepth - (index - script->nfixed) - 1;
    for (unsigned i = 0; i < diff; i++)
        stack = stack->group()->innerStack;
    return &stack->group()->types;
}

inline JSValueType
Script::knownArgumentTypeTag(JSContext *cx, JSScript *script, unsigned arg)
{
    jsid id = getArgumentId(arg);
    if (!JSID_IS_VOID(id) && !argEscapes(arg)) {
        types::TypeSet *types = getVariable(cx, id);
        return types->getKnownTypeTag(cx, script);
    }
    return JSVAL_TYPE_UNKNOWN;
}

inline JSValueType
Script::knownLocalTypeTag(JSContext *cx, JSScript *script, unsigned local)
{
    jsid id = getLocalId(local, NULL);
    if (!localHasUseBeforeDef(local) && !JSID_IS_VOID(id)) {
        JS_ASSERT(!localEscapes(local));
        types::TypeSet *types = getVariable(cx, id);
        return types->getKnownTypeTag(cx, script);
    }
    return JSVAL_TYPE_UNKNOWN;
}

} 





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
    static uint32 keyBits(TypeObject *obj) { return (uint32) obj; }
    static TypeObject *getKey(TypeObject *obj) { return obj; }
};

inline bool
TypeSet::hasType(jstype type)
{
    if (typeFlags & TYPE_FLAG_UNKNOWN)
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
    JS_ASSERT_IF(typeFlags & TYPE_FLAG_UNKNOWN, typeFlags == TYPE_FLAG_UNKNOWN);
    InferSpew(ISpewOps, "addType: T%u %s", id(), TypeString(type));

    if (typeFlags & TYPE_FLAG_UNKNOWN)
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





inline TypeStack *
TypeStack::group()
{
    TypeStack *res = this;
    while (res->mergedGroup)
        res = res->mergedGroup;
    if (mergedGroup && mergedGroup != res)
        mergedGroup = res;
    return res;
}

inline void
TypeStack::setInnerStack(TypeStack *inner)
{
    JS_ASSERT(!mergedGroup);
    innerStack = inner;
}





inline
TypeCallsite::TypeCallsite(analyze::Bytecode *code, bool isNew, unsigned argumentCount)
    : code(code), isNew(isNew), argumentCount(argumentCount),
      thisTypes(NULL), thisType(0), returnTypes(NULL)
{
    argumentTypes = ArenaArray<TypeSet*>(code->pool(), argumentCount);
}

inline void
TypeCallsite::forceThisTypes(JSContext *cx)
{
    if (thisTypes)
        return;
    thisTypes = TypeSet::make(cx, code->pool(), "site_this");
    thisTypes->addType(cx, thisType);
}

inline void
TypeCallsite::forceReturnTypes(JSContext *cx)
{
    if (returnTypes)
        return;
    returnTypes = TypeSet::make(cx, code->pool(), "site_return");
}

inline TypeObject *
TypeCallsite::getInitObject(JSContext *cx, bool isArray)
{
    return code->getInitObject(cx, isArray);
}

inline JSArenaPool &
TypeCallsite::pool()
{
    return code->pool();
}





inline TypeSet *
TypeObject::getProperty(JSContext *cx, jsid id, bool assign)
{
    JS_ASSERT(JSID_IS_VOID(id) || JSID_IS_STRING(id));
    JS_ASSERT_IF(JSID_IS_STRING(id), JSID_TO_STRING(id) != NULL);

    Property *&prop = HashSetInsert<jsid,Property,Property>(cx, propertySet, propertyCount, id);
    if (!prop)
        addProperty(cx, id, prop);

    return assign ? &prop->ownTypes : &prop->types;
}

inline bool
TypeObject::isArray(JSContext *cx)
{
    TypeObject *array = cx->getFixedTypeObject(TYPE_OBJECT_ARRAY_PROTOTYPE);
    TypeObject *obj = prototype;
    while (obj) {
        if (obj == array)
            return true;
        obj = obj->prototype;
    }
    return false;
}

} 

inline types::TypeSet *
analyze::Script::getVariable(JSContext *cx, jsid id)
{
    JS_ASSERT(JSID_IS_STRING(id) && JSID_TO_STRING(id) != NULL);

    types::Variable *&var = types::HashSetInsert<jsid,types::Variable,types::Variable>
        (cx, variableSet, variableCount, id);
    if (!var)
        addVariable(cx, id, var);

    return &var->types;
}

} 

#endif 

#endif 
