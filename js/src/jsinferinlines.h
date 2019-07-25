








































#include "jsanalyze.h"
#include "jscompartment.h"
#include "jsinfer.h"

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
      case JSVAL_TYPE_MAGIC:
        return (jstype) cx->getFixedTypeObject(TYPE_OBJECT_MAGIC);
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

} } 

#endif 





inline js::types::TypeObject *
JSContext::getTypeObject(const char *name, bool isFunction)
{
#ifdef JS_TYPE_INFERENCE
    return compartment->types.getTypeObject(this, NULL, name, isFunction);
#else
    return NULL;
#endif
}

inline js::types::TypeObject *
JSContext::getGlobalTypeObject()
{
#ifdef JS_TYPE_INFERENCE
    if (!compartment->types.globalObject)
        compartment->types.globalObject = getTypeObject("Global", false);
    return compartment->types.globalObject;
#else
    return NULL;
#endif
}

inline FILE *
JSContext::typeOut()
{
#ifdef JS_TYPE_INFERENCE
    return compartment->types.out;
#else
    JS_NOT_REACHED("Inference disabled");
    return NULL;
#endif
}

inline const char *
JSContext::getTypeId(jsid id)
{
#ifdef JS_TYPE_INFERENCE
    if (JSID_IS_VOID(id))
        return "(index)";

    JS_ASSERT(JSID_IS_STRING(id));
    JSString *str = JSID_TO_STRING(id);

    const jschar *chars;
    size_t length;

    str->getCharsAndLength(chars, length);
    if (length == 0)
        return "(blank)";

    unsigned size = js_GetDeflatedStringLength(this, chars, length);
    JS_ASSERT(size != unsigned(-1));

    char *scratchBuf = compartment->types.scratchBuf[0];
    size_t scratchLen = compartment->types.scratchLen[0];

    const unsigned GETID_COUNT = js::types::TypeCompartment::GETID_COUNT;

    for (unsigned i = 0; i < GETID_COUNT - 1; i++) {
        compartment->types.scratchBuf[i] = compartment->types.scratchBuf[i + 1];
        compartment->types.scratchLen[i] = compartment->types.scratchLen[i + 1];
    }

    if (size >= scratchLen) {
        scratchLen = js::Max(unsigned(100), size * 2);
        scratchBuf = js::ArenaArray<char>(compartment->types.pool, scratchLen);
    }

    compartment->types.scratchBuf[GETID_COUNT - 1] = scratchBuf;
    compartment->types.scratchLen[GETID_COUNT - 1] = scratchLen;

    js_DeflateStringToBuffer(this, chars, length, scratchBuf, &scratchLen);
    scratchBuf[size] = 0;

    return scratchBuf;
#else
    JS_NOT_REACHED("Inference disabled");
    return NULL;
#endif
}

inline void
JSContext::setTypeFunctionScript(JSFunction *fun, JSScript *script)
{
#ifdef JS_TYPE_INFERENCE
    char name[8];
    snprintf(name, 16, "#%u", script->analysis->id);

    js::types::TypeFunction *typeFun =
        compartment->types.getTypeObject(this, script->analysis, name, true)->asFunction();

    
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
JSContext::getTypeFunctionHandler(const char *name, JSTypeHandler handler)
{
#ifdef JS_TYPE_INFERENCE
    js::types::TypeFunction *typeFun =
        compartment->types.getTypeObject(this, NULL, name, true)->asFunction();

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
    return caller->script()->analysis->getCode(caller->pc(this)).monitorNeeded;
#else
    return false;
#endif
}

inline void
JSContext::markTypeCallerUnexpected(js::types::jstype type)
{
#ifdef JS_TYPE_INFERENCE
    JSStackFrame *caller = js_GetScriptedCaller(this, NULL);
    caller->script()->typeMonitorResult(this, caller->pc(this), 0, type, true);
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
JSContext::setTypeFunctionPrototype(js::types::TypeObject *fun,
                                    js::types::TypeObject *proto, bool inherit)
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

    
    if (nfun->newObject)
        addTypePrototype(nfun->newObject, proto);

    if (inherit) {
        getFixedTypeObject(js::types::TYPE_OBJECT_FUNCTION_PROTOTYPE)->addPropagate(this, fun);
        getFixedTypeObject(js::types::TYPE_OBJECT_OBJECT_PROTOTYPE)->addPropagate(this, proto);
    }
#endif
}

inline void
JSContext::addTypePrototype(js::types::TypeObject *obj, js::types::TypeObject *proto)
{
#ifdef JS_TYPE_INFERENCE
    proto->addPropagate(this, obj, true);
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

    js::types::TypeSet *types = obj->properties(this).getVariable(this, id);

    if (types->hasType(type))
        return;

    if (compartment->types.interpreting) {
        compartment->types.addDynamicType(this, types, type,
                                          "AddBuiltin: %s %s:",
                                          getTypeId(obj->name),
                                          getTypeId(id));
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

    js::types::TypeSet *firstTypes = obj->properties(this).getVariable(this, first);
    js::types::TypeSet *secondTypes = obj->properties(this).getVariable(this, second);

    firstTypes->addSubset(this, obj->pool(), secondTypes);
    secondTypes->addSubset(this, obj->pool(), firstTypes);
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
    js::types::TypeFunction *fun = args.callee().toObject().getTypeObject()->asFunction();

    





    if (!fun->script)
        return;
    js::analyze::Script *script = fun->script->analysis;

    if (!force) {
        if (caller->analysis->getCode(callerpc).monitorNeeded)
            force = true;
    }

    typeMonitorEntry(fun->script, args.thisv(), constructing, force);

    
    if (!force)
        return;

    unsigned arg = 0;
    for (; arg < args.argc(); arg++) {
        js::types::jstype type = js::types::GetValueType(this, args[arg]);

        jsid id = script->getArgumentId(arg);
        if (!JSID_IS_VOID(id)) {
            js::types::TypeSet *types = script->localTypes.getVariable(this, id);
            if (!types->hasType(type)) {
                compartment->types.addDynamicType(this, types, type,
                                                  "AddArg: #%u %u:", script->id, arg);
            }
        } else {
            




        }
    }

    
    for (; arg < script->argCount; arg++) {
        jsid id = script->getArgumentId(arg);
        JS_ASSERT(!JSID_IS_VOID(id));

        js::types::TypeSet *types = script->localTypes.getVariable(this, id);
        if (!types->hasType(js::types::TYPE_UNDEFINED)) {
            compartment->types.addDynamicType(this, types, js::types::TYPE_UNDEFINED,
                                              "AddArg: #%u %u:", script->id, arg);
        }
    }
#endif
}

inline void
JSContext::typeMonitorEntry(JSScript *script, const js::Value &thisv,
                            bool constructing, bool force)
{
#ifdef JS_TYPE_INFERENCE
    js::analyze::Script *analysis = script->analysis;
    JS_ASSERT(analysis);

    if (force) {
        js::types::jstype type;
        if (constructing)
            type = (js::types::jstype) analysis->function->getNewObject(this);
        else
            type = js::types::GetValueType(this, thisv);
        if (!analysis->thisTypes.hasType(type)) {
            compartment->types.addDynamicType(this, &analysis->thisTypes, type,
                                              "AddThis: #%u:", analysis->id);
        }
    }

    if (!analysis->hasAnalyzed()) {
        compartment->types.interpreting = false;
        uint64_t startTime = compartment->types.currentTime();

        analysis->analyze(this);

        uint64_t endTime = compartment->types.currentTime();
        compartment->types.analysisTime += (endTime - startTime);
        compartment->types.interpreting = true;
    }
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
    return analysis->getCode(pc).getInitObject(cx, isArray);
#else
    return NULL;
#endif
}

inline void
JSScript::typeMonitorResult(JSContext *cx, const jsbytecode *pc, unsigned index,
                            js::types::jstype type, bool force)
{
#ifdef JS_TYPE_INFERENCE
    js::analyze::Bytecode &code = analysis->getCode(pc);
    if (!force && !code.monitorNeeded)
        return;

    js::types::TypeSet *stackTypes = code.pushed(index);
    if (stackTypes->hasType(type))
        return;

    if (!stackTypes->hasType(type)) {
        const char *prefix = force ? "MonitorForce" : "MonitorResult";
        cx->compartment->types.addDynamicType(cx, stackTypes, type,
                                              "%s: #%u:%05u %u:", prefix,
                                              analysis->id, code.offset, index);
    }
#endif
}

inline void
JSScript::typeMonitorResult(JSContext *cx, const jsbytecode *pc, unsigned index,
                            const js::Value &rval, bool force)
{
#ifdef JS_TYPE_INFERENCE
    typeMonitorResult(cx, pc, index, js::types::GetValueType(cx, rval), force);
#endif
}

inline void
JSScript::typeMonitorAssign(JSContext *cx, const jsbytecode *pc,
                            JSObject *obj, jsid id, const js::Value &rval)
{
#ifdef JS_TYPE_INFERENCE
    js::analyze::Bytecode &code = analysis->getCode(pc);
    if (!code.monitorNeeded)
        return;

    js::types::TypeObject *object = obj->getTypeObject();
    js::types::jstype rvtype = js::types::GetValueType(cx, rval);

    id = js::types::MakeTypeId(id);
    js::types::TypeSet *assignTypes = object->properties(cx).getVariable(cx, id);

    
    if (id == ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom) &&
        js::types::TypeIsObject(rvtype) && object->isFunction) {
        cx->addTypePrototype(object->asFunction()->getNewObject(cx), (js::types::TypeObject *) rvtype);
    }

    
    if (id == ATOM_TO_JSID(cx->runtime->atomState.protoAtom) && js::types::TypeIsObject(rvtype))
        cx->addTypePrototype(object, (js::types::TypeObject *) rvtype);

    if (assignTypes->hasType(rvtype))
        return;

    cx->compartment->types.addDynamicType(cx, assignTypes, rvtype,
                                          "MonitorAssign: #%u:%05u %s %s:", analysis->id, code.offset,
                                          cx->getTypeId(object->name),
                                          cx->getTypeId(id));
#endif
}

inline void
JSScript::typeSetArgument(JSContext *cx, unsigned arg, const js::Value &value)
{
#ifdef JS_TYPE_INFERENCE
    jsid id = analysis->getArgumentId(arg);
    if (!JSID_IS_VOID(id)) {
        js::types::TypeSet *argTypes = analysis->localTypes.getVariable(cx, id);
        js::types::jstype type = js::types::GetValueType(cx, value);
        if (!argTypes->hasType(type)) {
            cx->compartment->types.addDynamicType(cx, argTypes, type,
                                                  "SetArgument: #%u %s:",
                                                  analysis->id, cx->getTypeId(id));
        }
    }
#endif
}





inline js::types::TypeObject *
JSObject::getTypeFunctionPrototype(JSContext *cx)
{
#ifdef JS_TYPE_INFERENCE
    js::types::TypeFunction *fun = getTypeObject()->asFunction();
    return fun->prototype(cx);
#else
    return NULL;
#endif
}

inline js::types::TypeObject *
JSObject::getTypeFunctionNewObject(JSContext *cx)
{
#ifdef JS_TYPE_INFERENCE
    js::types::TypeFunction *fun = getTypeObject()->asFunction();
    return fun->getNewObject(cx);
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
    if (!initObject) {
        char name[32];
        snprintf(name, 32, "#%u:%u", script->id, offset);
        initObject = cx->compartment->types.getTypeObject(cx, script, name, false);
        initObject->isInitObject = true;
    }

    




    if (isArray) {
        if (!initObject->hasArrayPropagation) {
            types::TypeObject *arrayProto = cx->getFixedTypeObject(types::TYPE_OBJECT_ARRAY_PROTOTYPE);
            arrayProto->addPropagate(cx, initObject);
        }
    } else {
        if (!initObject->hasObjectPropagation) {
            types::TypeObject *objectProto = cx->getFixedTypeObject(types::TYPE_OBJECT_OBJECT_PROTOTYPE);
            objectProto->addPropagate(cx, initObject);
        }
    }

    return initObject;
}





inline jsid
Script::getLocalId(unsigned index, types::TypeStack *stack)
{
    if (index >= script->nfixed) {
        




        stack = stack ? stack->group() : NULL;
        while (stack && (stack->stackDepth != index - script->nfixed)) {
            stack = stack->innerStack;
            stack = stack ? stack->group() : NULL;
        }

        if (stack && JSID_TO_STRING(stack->letVariable) != NULL)
            return stack->letVariable;

        




        return JSID_VOID;
    }

    if (!localNames[argCount + index])
        return JSID_VOID;

    return ATOM_TO_JSID(JS_LOCAL_NAME_TO_ATOM(localNames[argCount + index]));
}

inline jsid
Script::getArgumentId(unsigned index)
{
    JS_ASSERT(function);

    



    if (index >= argCount || !localNames[index])
        return JSID_VOID;

    return ATOM_TO_JSID(JS_LOCAL_NAME_TO_ATOM(localNames[index]));
}

} 





namespace types {

inline void
TypeCompartment::addPending(JSContext *cx, TypeConstraint *constraint, TypeSet *source, jstype type)
{
    JS_ASSERT(this == &cx->compartment->types);
    JS_ASSERT(type);

#ifdef JS_TYPES_DEBUG_SPEW
    fprintf(out, "pending: C%u", constraint->id);
    PrintType(cx, type);
#endif

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

#ifdef JS_TYPES_DEBUG_SPEW
        fprintf(out, "resolve: C%u ", pending.constraint->id);
        PrintType(cx, pending.type);
#endif

        pending.constraint->newType(cx, pending.source, pending.type);
    }

    resolving = false;
}

inline void
TypeCompartment::recompileScript(analyze::Bytecode *code)
{
    JS_ASSERT(code->script->compiled);

    if (!code->script->recompileNeeded) {
#ifdef DEBUG
        fprintf(out, "MarkJIT: #%u:%05u\n", code->script->id, code->offset);
#endif
        code->script->recompileNeeded = true;
    }
}

inline void
TypeCompartment::monitorBytecode(analyze::Bytecode *code)
{
    if (code->monitorNeeded)
        return;

    if (code->script->compiled)
        recompileScript(code);

#ifdef JS_TYPES_DEBUG_SPEW
    fprintf(out, "addMonitorNeeded: #%u:%05u\n", code->script->id, code->offset);
#endif

    code->monitorNeeded = true;
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


template <class T>
static inline uint32
HashPointer(T *v)
{
    uint32 nv = (uint32)(uint64) v;

    uint32 hash = 84696351 ^ (nv & 0xff);
    hash = (hash * 16777619) ^ ((nv >> 8) & 0xff);
    hash = (hash * 16777619) ^ ((nv >> 16) & 0xff);
    return (hash * 16777619) ^ ((nv >> 24) & 0xff);
}





template <class T>
static bool
HashSetInsertTry(JSContext *cx, T **&values, unsigned &count, T *data, bool contains)
{
    unsigned capacity = HashSetCapacity(count);

    if (!contains) {
        unsigned pos = HashPointer(data) & (capacity - 1);

        while (values[pos] != NULL) {
            if (values[pos] == data)
                return false;
            pos = (pos + 1) & (capacity - 1);
        }

        values[pos] = data;
    }

    count++;
    unsigned newCapacity = HashSetCapacity(count);

    if (newCapacity > capacity) {
        T **newValues = (T **) cx->calloc(newCapacity * sizeof(T*));

        for (unsigned i = 0; i < capacity; i++) {
            if (values[i]) {
                unsigned pos = HashPointer(values[i]) & (newCapacity - 1);
                while (newValues[pos] != NULL)
                    pos = (pos + 1) & (newCapacity - 1);
                newValues[pos] = values[i];
            }
        }

        if (values)
            cx->free(values);
        values = newValues;
    }

    if (contains) {
        unsigned pos = HashPointer(data) & (newCapacity - 1);
        while (values[pos] != NULL)
            pos = (pos + 1) & (newCapacity - 1);
        values[pos] = data;
    }

    return true;
}





template <class T>
static inline bool
HashSetInsert(JSContext *cx, T **&values, unsigned &count, T *data)
{
    if (count == 0) {
        values = (T**) data;
        count++;
        return true;
    }

    if (count == 1) {
        T *oldData = (T*) values;
        if (oldData == data)
            return false;

        values = (T **) cx->calloc(SET_ARRAY_SIZE * sizeof(T*));

        values[0] = oldData;
        values[1] = data;
        count++;
        return true;
    }

    if (count <= SET_ARRAY_SIZE) {
        for (unsigned i = 0; i < count; i++) {
            if (values[i] == data)
                return false;
        }

        if (count < SET_ARRAY_SIZE) {
            values[count++] = data;
            return true;
        }

        HashSetInsertTry(cx, values, count, data, true);
        return true;
    }

    return HashSetInsertTry(cx, values, count, data, false);
}


template <class T>
static inline bool
HashSetContains(T **values, unsigned count, T *data)
{
    if (count == 0)
        return false;

    if (count == 1)
        return (data == (T*) values);

    if (count <= SET_ARRAY_SIZE) {
        for (unsigned i = 0; i < count; i++) {
            if (values[i] == data)
                return true;
        }
        return false;
    }

    unsigned capacity = HashSetCapacity(count);
    unsigned pos = HashPointer(data) & (capacity - 1);

    while (values[pos] != NULL) {
        if (values[pos] == data)
            return true;
        pos = (pos + 1) & (capacity - 1);
    }

    return false;
}

inline bool
TypeSet::hasType(jstype type)
{
    if (typeFlags & TYPE_FLAG_UNKNOWN)
        return true;

    if (TypeIsPrimitive(type))
        return ((1 << type) & typeFlags) != 0;
    else
        return HashSetContains(objectSet, objectCount, (TypeObject*) type);
}





static inline bool
UseDuplicateObjects(TypeObject *object)
{
    
    return object->isInitObject ||
        (object->isFunction && object->asFunction()->script != NULL);
}


const unsigned OBJECT_THRESHOLD = unsigned(-1);
const unsigned TYPESET_THRESHOLD = unsigned(-1);

inline void
TypeSet::addType(JSContext *cx, jstype type)
{
    JS_ASSERT(type);
    JS_ASSERT_IF(typeFlags & TYPE_FLAG_UNKNOWN, typeFlags == TYPE_FLAG_UNKNOWN);

#ifdef JS_TYPES_DEBUG_SPEW
    JS_ASSERT(id);
    fprintf(cx->typeOut(), "addType: T%u ", id);
    PrintType(cx, type);
#endif

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
        if (!HashSetInsert(cx, objectSet, objectCount, object))
            return;

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

#ifdef JS_TYPES_DEBUG_SPEW
    fprintf(cx->typeOut(), "intermediate %s T%u\n", name, res->id);
#endif

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
    stackDepth = inner ? (inner->group()->stackDepth + 1) : 0;
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
VariableSet::getVariable(JSContext *cx, jsid id)
{
    JS_ASSERT(JSID_IS_VOID(id) || JSID_IS_STRING(id));
    JS_ASSERT_IF(JSID_IS_STRING(id), JSID_TO_STRING(id) != NULL);

    Variable *res = variables;
    while (res) {
        if (res->id == id)
            return &res->types;
        res = res->next;
    }

    
    res = ArenaNew<Variable>(*pool, pool, id);
    res->next = variables;
    variables = res;

#ifdef JS_TYPES_DEBUG_SPEW
    fprintf(cx->typeOut(), "addVariable: %s %s T%u\n",
            cx->getTypeId(name), cx->getTypeId(id), res->types.id);
#endif

    
    if (propagateCount >= 2) {
        unsigned capacity = HashSetCapacity(propagateCount);
        for (unsigned i = 0; i < capacity; i++) {
            VariableSet *target = propagateSet[i];
            if (target) {
                TypeSet *targetTypes = target->getVariable(cx, id);
                res->types.addSubset(cx, *pool, targetTypes);
            }
        }
    } else if (propagateCount == 1) {
        TypeSet *targetTypes = ((VariableSet*)propagateSet)->getVariable(cx, id);
        res->types.addSubset(cx, *pool, targetTypes);
    }

    return &res->types;
}





inline TypeSet *
TypeObject::indexTypes(JSContext *cx)
{
    return properties(cx).getVariable(cx, JSID_VOID);
}

inline TypeObject *
TypeFunction::getNewObject(JSContext *cx)
{
    if (newObject)
        return newObject;

    const char *baseName = cx->getTypeId(name);

    unsigned len = strlen(baseName) + 10;
    char *newName = (char *) alloca(len);
    snprintf(newName, len, "%s:new", baseName);
    newObject = cx->compartment->types.getTypeObject(cx, script ? script->analysis : NULL,
                                                     newName, false);

    properties(cx);
    JS_ASSERT_IF(!prototypeObject, isBuiltin);

    



    if (prototypeObject)
        cx->addTypePrototype(newObject, prototypeObject);

    return newObject;
}

} 
} 

#endif 

#endif 
