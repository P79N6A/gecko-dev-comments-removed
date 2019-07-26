






#include "builtin/ParallelArray.h"
#include "builtin/ParallelArray-inl.h"

#include "jsapi.h"
#include "jsobj.h"
#include "jsarray.h"
#include "jsprf.h"

#include "gc/Marking.h"
#include "vm/GlobalObject.h"
#include "vm/Stack.h"
#include "vm/StringBuffer.h"

#include "jsobjinlines.h"
#include "jsarrayinlines.h"

using namespace js;
using namespace js::types;





typedef ParallelArrayObject::IndexVector IndexVector;
typedef ParallelArrayObject::IndexInfo IndexInfo;

static bool
ReportMoreArgsNeeded(JSContext *cx, const char *name, const char *num, const char *p)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_MORE_ARGS_NEEDED, name, num, p);
    return false;
}

static bool
ReportBadArg(JSContext *cx, const char *s = "")
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_BAD_ARG, s);
    return false;
}

bool
ParallelArrayObject::IndexInfo::isInitialized() const
{
    return (dimensions.length() > 0 &&
            indices.capacity() >= dimensions.length() &&
            partialProducts.length() == dimensions.length());
}






static bool
MaybeGetParallelArrayObjectAndLength(JSContext *cx, HandleObject obj,
                                     MutableHandle<ParallelArrayObject *> pa,
                                     IndexInfo *iv, uint32_t *length)
{
    if (ParallelArrayObject::is(obj)) {
        pa.set(ParallelArrayObject::as(obj));
        if (!pa->isOneDimensional() && !iv->initialize(cx, pa, 1))
            return false;
        *length = pa->outermostDimension();
    } else if (!GetLengthProperty(cx, obj, length)) {
        return false;
    }

    return true;
}








static bool
GetElementFromArrayLikeObject(JSContext *cx, HandleObject obj, HandleParallelArrayObject pa,
                              IndexInfo &iv, uint32_t i, MutableHandleValue vp)
{
    
    
    
    if (pa && pa->getParallelArrayElement(cx, i, &iv, vp))
        return true;

    if (obj->isDenseArray() && i < obj->getDenseArrayInitializedLength() &&
        !js_PrototypeHasIndexedProperties(cx, obj))
    {
        vp.set(obj->getDenseArrayElement(i));
        if (vp.isMagic(JS_ARRAY_HOLE))
            vp.setUndefined();
        return true;
    }

    if (obj->isArguments()) {
        if (obj->asArguments().maybeGetElement(static_cast<uint32_t>(i), vp))
            return true;
    }

    
    
    return JSObject::getElement(cx, obj, obj, i, vp);
}

static inline bool
SetArrayNewType(JSContext *cx, HandleObject obj)
{
    RootedTypeObject newtype(cx, GetTypeCallerInitObject(cx, JSProto_Array));
    if (!newtype)
        return false;
    obj->setType(newtype);
    return true;
}

static JSObject *
NewDenseCopiedArrayWithType(JSContext *cx, uint32_t length, HandleObject source)
{
    JS_ASSERT(source);

    RootedObject buffer(cx, NewDenseAllocatedArray(cx, length));
    if (!buffer)
        return NULL;
    JS_ASSERT(buffer->getDenseArrayCapacity() >= length);
    buffer->setDenseArrayInitializedLength(length);

    uint32_t srclen;
    uint32_t copyUpTo;

    if (source->isDenseArray() && !js_PrototypeHasIndexedProperties(cx, source)) {
        
        
        
        
        const Value *srcvp = source->getDenseArrayElements();

        srclen = source->getDenseArrayInitializedLength();
        copyUpTo = Min(length, srclen);

        
        Value elem;
        for (uint32_t i = 0; i < copyUpTo; i++) {
            elem = srcvp[i].isMagic(JS_ARRAY_HOLE) ? UndefinedValue() : srcvp[i];
            JSObject::initDenseArrayElementWithType(cx, buffer, i, elem);
        }

        
        for (uint32_t i = copyUpTo; i < length; i++)
            JSObject::initDenseArrayElementWithType(cx, buffer, i, UndefinedValue());
    } else {
        
        
        
        for (uint32_t i = 0; i < length; i++)
            JSObject::initDenseArrayElementWithType(cx, buffer, i, UndefinedValue());

        IndexInfo siv(cx);
        RootedParallelArrayObject sourcePA(cx);

        if (!MaybeGetParallelArrayObjectAndLength(cx, source, &sourcePA, &siv, &srclen))
            return NULL;
        copyUpTo = Min(length, srclen);

        
        RootedValue elem(cx);
        for (uint32_t i = 0; i < copyUpTo; i++) {
            if (!GetElementFromArrayLikeObject(cx, source, sourcePA, siv, i, &elem))
                return NULL;
            JSObject::setDenseArrayElementWithType(cx, buffer, i, elem);
        }
    }

    if (!SetArrayNewType(cx, buffer))
        return NULL;

    return *buffer.address();
}

static inline JSObject *
NewDenseArrayWithType(JSContext *cx, uint32_t length)
{
    RootedObject buffer(cx, NewDenseAllocatedArray(cx, length));
    if (!buffer)
        return NULL;

    buffer->ensureDenseArrayInitializedLength(cx, length, 0);

    if (!SetArrayNewType(cx, buffer))
        return NULL;

    return *buffer.address();
}



static inline bool
ArrayLikeToIndexVector(JSContext *cx, HandleObject obj, IndexVector &indices)
{
    IndexInfo iv(cx);
    RootedParallelArrayObject pa(cx);
    uint32_t length;

    if (!MaybeGetParallelArrayObjectAndLength(cx, obj, &pa, &iv, &length))
        return false;

    if (!indices.resize(length))
        return false;

    RootedValue elem(cx);
    for (uint32_t i = 0; i < length; i++) {
        if (!GetElementFromArrayLikeObject(cx, obj, pa, iv, i, &elem) ||
            !ToUint32(cx, elem, &indices[i]))
        {
            return false;
        }
    }

    return true;
}

template <bool impl(JSContext *, CallArgs)>
static inline
JSBool NonGenericMethod(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, ParallelArrayObject::is, impl, args);
}













































































ParallelArrayObject::SequentialMode ParallelArrayObject::sequential;
ParallelArrayObject::ParallelMode ParallelArrayObject::parallel;
ParallelArrayObject::FallbackMode ParallelArrayObject::fallback;

ParallelArrayObject::ExecutionStatus
ParallelArrayObject::SequentialMode::build(JSContext *cx, IndexInfo &iv,
                                           HandleObject elementalFun, HandleObject buffer)
{
    JS_ASSERT(iv.isInitialized());

    uint32_t length = iv.scalarLengthOfDimensions();

    InvokeArgsGuard args;
    if (!cx->stack.pushInvokeArgs(cx, iv.dimensions.length(), &args))
        return ExecutionFailed;

    for (uint32_t i = 0; i < length; i++) {
        args.setCallee(ObjectValue(*elementalFun));
        args.setThis(UndefinedValue());

        
        iv.fromScalar(i);
        for (size_t j = 0; j < iv.indices.length(); j++)
            args[j].setNumber(iv.indices[j]);

        if (!Invoke(cx, args))
            return ExecutionFailed;

        JSObject::setDenseArrayElementWithType(cx, buffer, i, args.rval());
    }

    return ExecutionSucceeded;
}

ParallelArrayObject::ExecutionStatus
ParallelArrayObject::SequentialMode::map(JSContext *cx, HandleParallelArrayObject source,
                                         HandleObject elementalFun, HandleObject buffer)
{
    JS_ASSERT(is(source));
    JS_ASSERT(source->outermostDimension() == buffer->getDenseArrayInitializedLength());
    JS_ASSERT(buffer->isDenseArray());

    uint32_t length = source->outermostDimension();

    IndexInfo iv(cx);
    if (!source->isOneDimensional() && !iv.initialize(cx, source, 1))
        return ExecutionFailed;

    InvokeArgsGuard args;
    if (!cx->stack.pushInvokeArgs(cx, 3, &args))
        return ExecutionFailed;

    RootedValue elem(cx);
    for (uint32_t i = 0; i < length; i++) {
        args.setCallee(ObjectValue(*elementalFun));
        args.setThis(UndefinedValue());

        if (!source->getParallelArrayElement(cx, i, &iv, &elem))
            return ExecutionFailed;

        
        args[0] = elem;
        args[1].setNumber(i);
        args[2].setObject(*source);

        if (!Invoke(cx, args))
            return ExecutionFailed;

        JSObject::setDenseArrayElementWithType(cx, buffer, i, args.rval());
    }

    return ExecutionSucceeded;
}

ParallelArrayObject::ExecutionStatus
ParallelArrayObject::SequentialMode::reduce(JSContext *cx, HandleParallelArrayObject source,
                                            HandleObject elementalFun, HandleObject buffer,
                                            MutableHandleValue vp)
{
    JS_ASSERT(is(source));
    JS_ASSERT_IF(buffer, buffer->isDenseArray());
    JS_ASSERT_IF(buffer, buffer->getDenseArrayInitializedLength() >= 1);

    uint32_t length = source->outermostDimension();

    
    
    
    
    
    RootedValue acc(cx);
    IndexInfo iv(cx);

    if (!source->isOneDimensional() && !iv.initialize(cx, source, 1))
        return ExecutionFailed;

    if (!source->getParallelArrayElement(cx, 0, &iv, &acc))
        return ExecutionFailed;

    if (buffer)
        JSObject::setDenseArrayElementWithType(cx, buffer, 0, acc);

    InvokeArgsGuard args;
    if (!cx->stack.pushInvokeArgs(cx, 2, &args))
        return ExecutionFailed;

    RootedValue elem(cx);
    for (uint32_t i = 1; i < length; i++) {
        args.setCallee(ObjectValue(*elementalFun));
        args.setThis(UndefinedValue());

        if (!source->getParallelArrayElement(cx, i, &iv, &elem))
            return ExecutionFailed;

        
        args[0] = acc;
        args[1] = elem;

        if (!Invoke(cx, args))
            return ExecutionFailed;

        
        acc = args.rval();
        if (buffer)
            JSObject::setDenseArrayElementWithType(cx, buffer, i, args.rval());
    }

    vp.set(acc);

    return ExecutionSucceeded;
}

ParallelArrayObject::ExecutionStatus
ParallelArrayObject::SequentialMode::scatter(JSContext *cx, HandleParallelArrayObject source,
                                             HandleObject targets, const Value &defaultValue,
                                             HandleObject conflictFun, HandleObject buffer)
{
    JS_ASSERT(buffer->isDenseArray());

    uint32_t length = buffer->getDenseArrayInitializedLength();

    IndexInfo iv(cx);
    if (!source->isOneDimensional() && !iv.initialize(cx, source, 1))
        return ExecutionFailed;

    
    
    IndexInfo tiv(cx);
    RootedParallelArrayObject targetsPA(cx);

    
    uint32_t targetsLength;
    if (!MaybeGetParallelArrayObjectAndLength(cx, targets, &targetsPA, &tiv, &targetsLength))
        return ExecutionFailed;

    
    
    RootedValue elem(cx);
    RootedValue telem(cx);
    RootedValue targetElem(cx);
    for (uint32_t i = 0; i < Min(targetsLength, source->outermostDimension()); i++) {
        uint32_t targetIndex;

        if (!GetElementFromArrayLikeObject(cx, targets, targetsPA, tiv, i, &telem) ||
            !ToUint32(cx, telem, &targetIndex))
        {
            return ExecutionFailed;
        }

        if (targetIndex >= length) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_SCATTER_BOUNDS);
            return ExecutionFailed;
        }

        if (!source->getParallelArrayElement(cx, i, &iv, &elem))
            return ExecutionFailed;

        targetElem = buffer->getDenseArrayElement(targetIndex);

        
        
        
        if (!targetElem.isMagic(JS_ARRAY_HOLE)) {
            if (conflictFun) {
                InvokeArgsGuard args;
                if (!cx->stack.pushInvokeArgs(cx, 2, &args))
                    return ExecutionFailed;

                args.setCallee(ObjectValue(*conflictFun));
                args.setThis(UndefinedValue());
                args[0] = elem;
                args[1] = targetElem;

                if (!Invoke(cx, args))
                    return ExecutionFailed;

                elem = args.rval();
            } else {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_PAR_ARRAY_SCATTER_CONFLICT);
                return ExecutionFailed;
            }
        }

        JSObject::setDenseArrayElementWithType(cx, buffer, targetIndex, elem);
    }

    
    for (uint32_t i = 0; i < length; i++) {
        if (buffer->getDenseArrayElement(i).isMagic(JS_ARRAY_HOLE))
            JSObject::setDenseArrayElementWithType(cx, buffer, i, defaultValue);
    }

    return ExecutionSucceeded;
}

ParallelArrayObject::ExecutionStatus
ParallelArrayObject::SequentialMode::filter(JSContext *cx, HandleParallelArrayObject source,
                                            HandleObject filters, HandleObject buffer)
{
    JS_ASSERT(buffer->isDenseArray());

    IndexInfo iv(cx);
    if (!source->isOneDimensional() && !iv.initialize(cx, source, 1))
        return ExecutionFailed;

    
    
    IndexInfo fiv(cx);
    RootedParallelArrayObject filtersPA(cx);

    
    uint32_t filtersLength;

    if (!MaybeGetParallelArrayObjectAndLength(cx, filters, &filtersPA, &fiv, &filtersLength))
        return ExecutionFailed;

    RootedValue elem(cx);
    RootedValue felem(cx);
    for (uint32_t i = 0, pos = 0; i < filtersLength; i++) {
        if (!GetElementFromArrayLikeObject(cx, filters, filtersPA, fiv, i, &felem))
            return ExecutionFailed;

        
        if (!ToBoolean(felem))
            continue;

        if (!source->getParallelArrayElement(cx, i, &iv, &elem))
            return ExecutionFailed;

        
        JSObject::EnsureDenseResult result = JSObject::ED_SPARSE;
        result = buffer->ensureDenseArrayElements(cx, pos, 1);
        if (result != JSObject::ED_OK)
            return ExecutionFailed;
        if (i >= buffer->getArrayLength())
            buffer->setDenseArrayLength(pos + 1);
        JSObject::setDenseArrayElementWithType(cx, buffer, pos, elem);

        
        pos++;
    }

    return ExecutionSucceeded;
}

ParallelArrayObject::ExecutionStatus
ParallelArrayObject::ParallelMode::build(JSContext *cx, IndexInfo &iv,
                                         HandleObject elementalFun, HandleObject buffer)
{
    return ExecutionFailed;
}

ParallelArrayObject::ExecutionStatus
ParallelArrayObject::ParallelMode::map(JSContext *cx, HandleParallelArrayObject source,
                                       HandleObject elementalFun, HandleObject buffer)
{
    return ExecutionFailed;
}

ParallelArrayObject::ExecutionStatus
ParallelArrayObject::ParallelMode::reduce(JSContext *cx, HandleParallelArrayObject source,
                                          HandleObject elementalFun, HandleObject buffer,
                                          MutableHandleValue vp)
{
    return ExecutionFailed;
}

ParallelArrayObject::ExecutionStatus
ParallelArrayObject::ParallelMode::scatter(JSContext *cx, HandleParallelArrayObject source,
                                           HandleObject targetsObj, const Value &defaultValue,
                                           HandleObject conflictFun, HandleObject buffer)
{
    return ExecutionFailed;
}

ParallelArrayObject::ExecutionStatus
ParallelArrayObject::ParallelMode::filter(JSContext *cx, HandleParallelArrayObject source,
                                          HandleObject filtersObj, HandleObject buffer)
{
    return ExecutionFailed;
}

ParallelArrayObject::ExecutionStatus
ParallelArrayObject::FallbackMode::build(JSContext *cx, IndexInfo &iv,
                                         HandleObject elementalFun, HandleObject buffer)
{
    if (parallel.build(cx, iv, elementalFun, buffer) ||
        sequential.build(cx, iv, elementalFun, buffer))
    {
        return ExecutionSucceeded;
    }
    return ExecutionFailed;
}

ParallelArrayObject::ExecutionStatus
ParallelArrayObject::FallbackMode::map(JSContext *cx, HandleParallelArrayObject source,
                                       HandleObject elementalFun, HandleObject buffer)
{
    if (parallel.map(cx, source, elementalFun, buffer) ||
        sequential.map(cx, source, elementalFun, buffer))
    {
        return ExecutionSucceeded;
    }
    return ExecutionFailed;
}

ParallelArrayObject::ExecutionStatus
ParallelArrayObject::FallbackMode::reduce(JSContext *cx, HandleParallelArrayObject source,
                                          HandleObject elementalFun, HandleObject buffer,
                                          MutableHandleValue vp)
{
    if (parallel.reduce(cx, source, elementalFun, buffer, vp) ||
        sequential.reduce(cx, source, elementalFun, buffer, vp))
    {
        return ExecutionSucceeded;
    }
    return ExecutionFailed;
}

ParallelArrayObject::ExecutionStatus
ParallelArrayObject::FallbackMode::scatter(JSContext *cx, HandleParallelArrayObject source,
                                           HandleObject targetsObj, const Value &defaultValue,
                                           HandleObject conflictFun, HandleObject buffer)
{
    if (parallel.scatter(cx, source, targetsObj, defaultValue, conflictFun, buffer) ||
        sequential.scatter(cx, source, targetsObj, defaultValue, conflictFun, buffer))
    {
        return ExecutionSucceeded;
    }
    return ExecutionFailed;
}

ParallelArrayObject::ExecutionStatus
ParallelArrayObject::FallbackMode::filter(JSContext *cx, HandleParallelArrayObject source,
                                          HandleObject filtersObj, HandleObject buffer)
{
    if (parallel.filter(cx, source, filtersObj, buffer) ||
        sequential.filter(cx, source, filtersObj, buffer))
    {
        return ExecutionSucceeded;
    }
    return ExecutionFailed;
}

#ifdef DEBUG

const char *
ParallelArrayObject::ExecutionStatusToString(ExecutionStatus ss)
{
    switch (ss) {
      case ExecutionFailed:
        return "failure";
      case ExecutionCompiled:
        return "compilation";
      case ExecutionSucceeded:
        return "success";
    }
    return "(unknown status)";
}

bool
ParallelArrayObject::DebugOptions::init(JSContext *cx, const Value &v)
{
    RootedObject obj(cx, NonNullObject(cx, v));
    if (!obj)
        return false;

    RootedId id(cx);
    RootedValue propv(cx);
    JSString *propStr;
    JSBool match = false;
    bool ok;

    id = AtomToId(Atomize(cx, "mode", strlen("mode")));
    if (!JSObject::getGeneric(cx, obj, obj, id, &propv))
        return false;

    propStr = ToString(cx, propv);
    if (!propStr)
        return false;

    if ((ok = JS_StringEqualsAscii(cx, propStr, "par", &match)) && match)
        mode = &parallel;
    else if (ok && (ok = JS_StringEqualsAscii(cx, propStr, "seq", &match)) && match)
        mode = &sequential;
    else if (ok)
        return ReportBadArg(cx);
    else
        return false;

    id = AtomToId(Atomize(cx, "expect", strlen("expect")));
    if (!JSObject::getGeneric(cx, obj, obj, id, &propv))
        return false;

    propStr = ToString(cx, propv);
    if (!propStr)
        return false;

    if ((ok = JS_StringEqualsAscii(cx, propStr, "fail", &match)) && match)
        expect = ExecutionFailed;
    else if (ok && (ok = JS_StringEqualsAscii(cx, propStr, "bail", &match)) && match)
        expect = ExecutionCompiled;
    else if (ok && (ok = JS_StringEqualsAscii(cx, propStr, "success", &match)) && match)
        expect = ExecutionSucceeded;
    else if (ok)
        return ReportBadArg(cx);
    else
        return false;

    return true;
}

bool
ParallelArrayObject::DebugOptions::check(JSContext *cx, ExecutionStatus actual)
{
    if (expect != actual) {
        JS_ReportError(cx, "expected %s for %s execution, got %s",
                       ExecutionStatusToString(expect),
                       mode->toString(),
                       ExecutionStatusToString(actual));
        return false;
    }

    return true;
}

#endif 





JSFunctionSpec ParallelArrayObject::methods[] = {
    JS_FN("map",                 NonGenericMethod<map>,            1, 0),
    JS_FN("reduce",              NonGenericMethod<reduce>,         1, 0),
    JS_FN("scan",                NonGenericMethod<scan>,           1, 0),
    JS_FN("scatter",             NonGenericMethod<scatter>,        1, 0),
    JS_FN("filter",              NonGenericMethod<filter>,         1, 0),
    JS_FN("flatten",             NonGenericMethod<flatten>,        0, 0),
    JS_FN("partition",           NonGenericMethod<partition>,      1, 0),
    JS_FN("get",                 NonGenericMethod<get>,            1, 0),
    JS_FN(js_toString_str,       NonGenericMethod<toString>,       0, 0),
    JS_FN(js_toLocaleString_str, NonGenericMethod<toLocaleString>, 0, 0),
    JS_FS_END
};

Class ParallelArrayObject::protoClass = {
    "ParallelArray",
    JSCLASS_HAS_CACHED_PROTO(JSProto_ParallelArray),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

Class ParallelArrayObject::class_ = {
    "ParallelArray",
    Class::NON_NATIVE |
    JSCLASS_HAS_RESERVED_SLOTS(RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_ParallelArray),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    mark,                    
    JS_NULL_CLASS_EXT,
    {
        lookupGeneric,
        lookupProperty,
        lookupElement,
        lookupSpecial,
        defineGeneric,
        defineProperty,
        defineElement,
        defineSpecial,
        getGeneric,
        getProperty,
        getElement,
        NULL,                
        getSpecial,
        setGeneric,
        setProperty,
        setElement,
        setSpecial,
        getGenericAttributes,
        getPropertyAttributes,
        getElementAttributes,
        getSpecialAttributes,
        setGenericAttributes,
        setPropertyAttributes,
        setElementAttributes,
        setSpecialAttributes,
        deleteProperty,
        deleteElement,
        deleteSpecial,
        NULL,                
        NULL,                
        NULL,                
    }
};

JSObject *
ParallelArrayObject::initClass(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isNative());

    Rooted<GlobalObject *> global(cx, &obj->asGlobal());

    RootedObject proto(cx, global->createBlankPrototype(cx, &protoClass));
    if (!proto)
        return NULL;

    JSProtoKey key = JSProto_ParallelArray;
    RootedFunction ctor(cx);
    ctor = global->createConstructor(cx, construct, cx->names().ParallelArray, 0);
    if (!ctor ||
        !LinkConstructorAndPrototype(cx, ctor, proto) ||
        !DefinePropertiesAndBrand(cx, proto, NULL, methods) ||
        !DefineConstructorAndPrototype(cx, global, key, ctor, proto))
    {
        return NULL;
    }

    
    RootedId lengthId(cx, AtomToId(cx->names().length));
    RootedId shapeId(cx, AtomToId(cx->names().shape));
    unsigned flags = JSPROP_PERMANENT | JSPROP_SHARED | JSPROP_GETTER;

    RootedObject scriptedLength(cx, js_NewFunction(cx, NULL, NonGenericMethod<lengthGetter>,
                                                   0, 0, global, NULL));
    RootedObject scriptedShape(cx, js_NewFunction(cx, NULL, NonGenericMethod<dimensionsGetter>,
                                                  0, 0, global, NULL));

    RootedValue value(cx, UndefinedValue());
    if (!scriptedLength || !scriptedShape ||
        !DefineNativeProperty(cx, proto, lengthId, value,
                              JS_DATA_TO_FUNC_PTR(PropertyOp, scriptedLength.get()), NULL,
                              flags, 0, 0) ||
        !DefineNativeProperty(cx, proto, shapeId, value,
                              JS_DATA_TO_FUNC_PTR(PropertyOp, scriptedShape.get()), NULL,
                              flags, 0, 0))
    {
        return NULL;
    }

    return proto;
}

bool
ParallelArrayObject::getParallelArrayElement(JSContext *cx, IndexInfo &iv, MutableHandleValue vp)
{
    JS_ASSERT(iv.isInitialized());

    
    
    
    uint32_t d = iv.indices.length();
    uint32_t ndims = iv.dimensions.length();
    JS_ASSERT(d <= ndims);

    uint32_t base = bufferOffset();
    uint32_t end = base + iv.scalarLengthOfDimensions();

    
    
    if (d == ndims) {
        uint32_t index = base + iv.toScalar();
        if (index >= end)
            vp.setUndefined();
        else
            vp.set(buffer()->getDenseArrayElement(index));
        return true;
    }

    
    
    
    
    
    
    
    if (!iv.inBounds()) {
        vp.setUndefined();
        return true;
    }

    RootedObject buf(cx, buffer());
    IndexVector newDims(cx);
    return (newDims.append(iv.dimensions.begin() + d, iv.dimensions.end()) &&
            create(cx, buf, base + iv.toScalar(), newDims, vp));
}

bool
ParallelArrayObject::getParallelArrayElement(JSContext *cx, uint32_t index, IndexInfo *maybeIV,
                                             MutableHandleValue vp)
{
    
    if (isOneDimensional()) {
        uint32_t base = bufferOffset();
        uint32_t end = base + outermostDimension();

        if (base + index >= end)
            vp.setUndefined();
        else
            vp.set(buffer()->getDenseArrayElement(base + index));

        return true;
    }

    
    JS_ASSERT(maybeIV);
    JS_ASSERT(maybeIV->isInitialized());
    JS_ASSERT(maybeIV->indices.length() == 1);

    maybeIV->indices[0] = index;
    return getParallelArrayElement(cx, *maybeIV, vp);
}

bool
ParallelArrayObject::getParallelArrayElement(JSContext *cx, uint32_t index, MutableHandleValue vp)
{
    if (isOneDimensional())
        return getParallelArrayElement(cx, index, NULL, vp);

    
    
    
    IndexInfo iv(cx);
    if (!getDimensions(cx, iv.dimensions) || !iv.initialize(1))
        return false;
    iv.indices[0] = index;
    return getParallelArrayElement(cx, iv, vp);
}

bool
ParallelArrayObject::create(JSContext *cx, MutableHandleValue vp)
{
    IndexVector dims(cx);
    if (!dims.append(0))
        return false;
    RootedObject buffer(cx, NewDenseArrayWithType(cx, 0));
    if (!buffer)
        return false;
    return create(cx, buffer, 0, dims, vp);
}

bool
ParallelArrayObject::create(JSContext *cx, HandleObject buffer, MutableHandleValue vp)
{
    IndexVector dims(cx);
    if (!dims.append(buffer->getArrayLength()))
        return false;
    return create(cx, buffer, 0, dims, vp);
}

bool
ParallelArrayObject::create(JSContext *cx, HandleObject buffer, uint32_t offset,
                            const IndexVector &dims, MutableHandleValue vp)
{
    JS_ASSERT(buffer->isDenseArray());

    RootedObject result(cx, NewBuiltinClassInstance(cx, &class_));
    if (!result)
        return false;

    
    if (cx->typeInferenceEnabled()) {
        AutoEnterTypeInference enter(cx);
        TypeObject *bufferType = buffer->getType(cx);
        TypeObject *resultType = result->getType(cx);
        if (!bufferType->unknownProperties() && !resultType->unknownProperties()) {
            HeapTypeSet *bufferIndexTypes = bufferType->getProperty(cx, JSID_VOID, false);
            HeapTypeSet *resultIndexTypes = resultType->getProperty(cx, JSID_VOID, true);
            bufferIndexTypes->addSubset(cx, resultIndexTypes);
        }
    }

    
    RootedObject dimArray(cx, NewDenseArrayWithType(cx, dims.length()));
    if (!dimArray)
        return false;

    for (uint32_t i = 0; i < dims.length(); i++)
        JSObject::setDenseArrayElementWithType(cx, dimArray, i,
                                               Int32Value(static_cast<int32_t>(dims[i])));

    result->setSlot(SLOT_DIMENSIONS, ObjectValue(*dimArray));

    
    result->setSlot(SLOT_BUFFER, ObjectValue(*buffer));
    result->setSlot(SLOT_BUFFER_OFFSET, Int32Value(static_cast<int32_t>(offset)));

    
    vp.setObject(*result);

    return true;
}

JSBool
ParallelArrayObject::construct(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    if (args.length() < 1)
        return create(cx, args.rval());

    
    if (args.length() == 1) {
        RootedObject source(cx, NonNullObject(cx, args[0]));
        if (!source)
            return false;

        
        IndexVector dims(cx);
        uint32_t length;
        if (!dims.resize(1) || !GetLengthProperty(cx, source, &length))
            return false;
        dims[0] = length;

        RootedObject buffer(cx, NewDenseCopiedArrayWithType(cx, length, source));
        if (!buffer)
            return false;

        return create(cx, buffer, 0, dims, args.rval());
    }

    
    
    
    
    
    
    
    IndexInfo iv(cx);
    if (args[0].isObject()) {
        RootedObject dimObj(cx, &(args[0].toObject()));
        if (!ArrayLikeToIndexVector(cx, dimObj, iv.dimensions))
            return false;
    } else {
        if (!iv.dimensions.resize(1) || !ToUint32(cx, args[0], &iv.dimensions[0]))
            return false;
    }

    
    
    if (iv.dimensions.length() == 0 && !iv.dimensions.append(0))
        return false;

    if (!iv.initialize(0))
        return false;

    
    RootedObject elementalFun(cx, ValueToCallable(cx, &args[1]));
    if (!elementalFun)
        return false;

    
    uint32_t length = iv.scalarLengthOfDimensions();

    
    RootedObject buffer(cx, NewDenseArrayWithType(cx, length));
    if (!buffer)
        return false;

#ifdef DEBUG
    if (args.length() > 2) {
        DebugOptions options;
        if (!options.init(cx, args[2]) ||
            !options.check(cx, options.mode->build(cx, iv, elementalFun, buffer)))
        {
            return false;
        }

        return create(cx, buffer, 0, iv.dimensions, args.rval());
    }
#endif

    if (fallback.build(cx, iv, elementalFun, buffer) != ExecutionSucceeded)
        return false;

    return create(cx, buffer, 0, iv.dimensions, args.rval());
}

bool
ParallelArrayObject::map(JSContext *cx, CallArgs args)
{
    if (args.length() < 1)
        return ReportMoreArgsNeeded(cx, "ParallelArray.prototype.map", "0", "s");

    RootedParallelArrayObject obj(cx, as(&args.thisv().toObject()));

    uint32_t outer = obj->outermostDimension();
    RootedObject buffer(cx, NewDenseArrayWithType(cx, outer));
    if (!buffer)
        return false;

    RootedObject elementalFun(cx, ValueToCallable(cx, &args[0]));
    if (!elementalFun)
        return false;

#ifdef DEBUG
    if (args.length() > 1) {
        DebugOptions options;
        if (!options.init(cx, args[1]) ||
            !options.check(cx, options.mode->map(cx, obj, elementalFun, buffer)))
        {
            return false;
        }

        return create(cx, buffer, args.rval());
    }
#endif

    if (fallback.map(cx, obj, elementalFun, buffer) != ExecutionSucceeded)
        return false;

    return create(cx, buffer, args.rval());
}

bool
ParallelArrayObject::reduce(JSContext *cx, CallArgs args)
{
    if (args.length() < 1)
        return ReportMoreArgsNeeded(cx, "ParallelArray.prototype.reduce", "0", "s");

    RootedParallelArrayObject obj(cx, as(&args.thisv().toObject()));
    uint32_t outer = obj->outermostDimension();

    
    if (outer == 0) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_REDUCE_EMPTY);
        return false;
    }

    RootedObject elementalFun(cx, ValueToCallable(cx, &args[0]));
    if (!elementalFun)
        return false;

#ifdef DEBUG
    if (args.length() > 1) {
        DebugOptions options;
        if (!options.init(cx, args[1]))
            return false;

        return options.check(cx, options.mode->reduce(cx, obj, elementalFun, NullPtr(),
                                                      args.rval()));
    }
#endif

    
    return fallback.reduce(cx, obj, elementalFun, NullPtr(), args.rval()) == ExecutionSucceeded;
}

bool
ParallelArrayObject::scan(JSContext *cx, CallArgs args)
{
    if (args.length() < 1)
        return ReportMoreArgsNeeded(cx, "ParallelArray.prototype.scan", "0", "s");

    RootedParallelArrayObject obj(cx, as(&args.thisv().toObject()));
    uint32_t outer = obj->outermostDimension();

    
    if (outer == 0) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_REDUCE_EMPTY);
        return false;
    }

    RootedObject buffer(cx, NewDenseArrayWithType(cx, outer));
    if (!buffer)
        return false;

    RootedObject elementalFun(cx, ValueToCallable(cx, &args[0]));
    if (!elementalFun)
        return false;

    
    
    RootedValue dummy(cx);

#ifdef DEBUG
    if (args.length() > 1) {
        DebugOptions options;
        if (!options.init(cx, args[1]) ||
            !options.check(cx, options.mode->reduce(cx, obj, elementalFun, buffer, &dummy)))
        {
            return false;
        }

        return create(cx, buffer, args.rval());
    }
#endif

    if (fallback.reduce(cx, obj, elementalFun, buffer, &dummy) != ExecutionSucceeded)
        return false;

    return create(cx, buffer, args.rval());
}

bool
ParallelArrayObject::scatter(JSContext *cx, CallArgs args)
{
    if (args.length() < 1)
        return ReportMoreArgsNeeded(cx, "ParallelArray.prototype.scatter", "0", "s");

    RootedParallelArrayObject obj(cx, as(&args.thisv().toObject()));
    uint32_t outer = obj->outermostDimension();

    
    RootedObject targets(cx, NonNullObject(cx, args[0]));
    if (!targets)
        return false;

    
    Value defaultValue;
    if (args.length() >= 2)
        defaultValue = args[1];
    else
        defaultValue.setUndefined();

    
    RootedObject conflictFun(cx);
    if (args.length() >= 3 && !args[2].isUndefined()) {
        conflictFun = ValueToCallable(cx, &args[2]);
        if (!conflictFun)
            return false;
    }

    
    
    uint32_t resultLength;
    if (args.length() >= 4) {
        if (!ToUint32(cx, args[3], &resultLength))
            return false;
    } else {
        resultLength = outer;
    }

    
    RootedObject buffer(cx, NewDenseArrayWithType(cx, resultLength));
    if (!buffer)
        return false;

#ifdef DEBUG
    if (args.length() > 4) {
        DebugOptions options;
        if (!options.init(cx, args[4]) ||
            !options.check(cx, options.mode->scatter(cx, obj, targets, defaultValue,
                                                     conflictFun, buffer)))
        {
            return false;
        }

        return create(cx, buffer, args.rval());
    }
#endif

    if (fallback.scatter(cx, obj, targets, defaultValue,
                         conflictFun, buffer) != ExecutionSucceeded)
    {
        return false;
    }

    return create(cx, buffer, args.rval());
}

bool
ParallelArrayObject::filter(JSContext *cx, CallArgs args)
{
    if (args.length() < 1)
        return ReportMoreArgsNeeded(cx, "ParallelArray.prototype.filter", "0", "s");

    RootedParallelArrayObject obj(cx, as(&args.thisv().toObject()));

    
    RootedObject filters(cx, NonNullObject(cx, args[0]));
    if (!filters)
        return false;

    RootedObject buffer(cx, NewDenseArrayWithType(cx, 0));
    if (!buffer)
        return false;

#ifdef DEBUG
    if (args.length() > 1) {
        DebugOptions options;
        if (!options.init(cx, args[1]) ||
            !options.check(cx, options.mode->filter(cx, obj, filters, buffer)))
        {
            return false;
        }

        return create(cx, buffer, args.rval());
    }
#endif

    if (fallback.filter(cx, obj, filters, buffer) != ExecutionSucceeded)
        return false;

    return create(cx, buffer, args.rval());
}

bool
ParallelArrayObject::flatten(JSContext *cx, CallArgs args)
{
    RootedParallelArrayObject obj(cx, as(&args.thisv().toObject()));

    IndexVector dims(cx);
    if (!obj->getDimensions(cx, dims))
        return false;

    
    if (dims.length() == 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_ALREADY_FLAT);
        return false;
    }

    
    dims[1] *= dims[0];
    dims.erase(dims.begin());

    RootedObject buffer(cx, obj->buffer());
    return create(cx, buffer, obj->bufferOffset(), dims, args.rval());
}

bool
ParallelArrayObject::partition(JSContext *cx, CallArgs args)
{
    if (args.length() < 1)
        return ReportMoreArgsNeeded(cx, "ParallelArray.prototype.partition", "0", "s");

    uint32_t newDimension;
    if (!ToUint32(cx, args[0], &newDimension))
        return false;

    RootedParallelArrayObject obj(cx, as(&args.thisv().toObject()));

    
    uint32_t outer = obj->outermostDimension();
    if (newDimension == 0 || outer % newDimension) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_BAD_PARTITION);
        return false;
    }

    IndexVector dims(cx);
    if (!obj->getDimensions(cx, dims))
        return false;

    
    
    if (!dims.insert(dims.begin(), outer / newDimension))
        return false;

    
    dims[1] = newDimension;

    RootedObject buffer(cx, obj->buffer());
    return create(cx, buffer, obj->bufferOffset(), dims, args.rval());
}

bool
ParallelArrayObject::get(JSContext *cx, CallArgs args)
{
    if (args.length() < 1)
        return ReportMoreArgsNeeded(cx, "ParallelArray.prototype.get", "0", "s");

    RootedParallelArrayObject obj(cx, as(&args.thisv().toObject()));
    RootedObject indicesObj(cx, NonNullObject(cx, args[0]));
    if (!indicesObj)
        return false;

    IndexInfo iv(cx);
    if (!iv.initialize(cx, obj, 0))
        return false;
    if (!ArrayLikeToIndexVector(cx, indicesObj, iv.indices))
        return false;

    
    if (iv.indices.length() == 0 || iv.indices.length() > iv.dimensions.length())
        return ReportBadArg(cx, ".prototype.get");

    return obj->getParallelArrayElement(cx, iv, args.rval());
}

bool
ParallelArrayObject::dimensionsGetter(JSContext *cx, CallArgs args)
{
    args.rval().setObject(*(as(&args.thisv().toObject())->dimensionArray()));
    return true;
}

bool
ParallelArrayObject::lengthGetter(JSContext *cx, CallArgs args)
{
    args.rval().setNumber(as(&args.thisv().toObject())->outermostDimension());
    return true;
}

bool
ParallelArrayObject::toStringBufferImpl(JSContext *cx, IndexInfo &iv, bool useLocale,
                                        HandleObject buffer, StringBuffer &sb)
{
    JS_ASSERT(iv.isInitialized());

    
    uint32_t d = iv.indices.length() + 1;

    
    if (d < iv.dimensions.length()) {
        if (!sb.append('<'))
            return false;

        iv.indices.infallibleAppend(0);
        uint32_t length = iv.dimensions[d - 1];
        for (size_t i = 0; i < length; i++) {
            iv.indices[d - 1] = i;
            if (!toStringBufferImpl(cx, iv, useLocale, buffer, sb) ||
                (i + 1 != length && !sb.append(',')))
            {
                return false;
            }
        }
        iv.indices.shrinkBy(1);

        if (!sb.append('>'))
            return false;

        return true;
    }

    
    if (!sb.append('<'))
        return false;

    uint32_t offset;
    uint32_t length;

    
    if (d == 1) {
        offset = bufferOffset();
        length = iv.dimensions[0];
    } else {
        offset = bufferOffset() + iv.toScalar();
        length = iv.partialProducts[d - 2];
    }

    RootedValue tmp(cx);
    RootedValue localeElem(cx);
    RootedId id(cx);

    const Value *start = buffer->getDenseArrayElements() + offset;
    const Value *end = start + length;
    const Value *elem;

    for (elem = start; elem < end; elem++) {
        
        JS_ASSERT(!elem->isMagic(JS_ARRAY_HOLE));

        if (!JS_CHECK_OPERATION_LIMIT(cx))
            return false;

        if (!elem->isNullOrUndefined()) {
            if (useLocale) {
                tmp = *elem;
                JSObject *robj = ToObject(cx, tmp);
                if (!robj)
                    return false;

                id = NameToId(cx->names().toLocaleString);
                if (!robj->callMethod(cx, id, 0, NULL, &localeElem) ||
                    !ValueToStringBuffer(cx, localeElem, sb))
                {
                    return false;
                }
            } else {
                if (!ValueToStringBuffer(cx, *elem, sb))
                    return false;
            }
        }

        if (elem + 1 != end && !sb.append(','))
            return false;
    }

    if (!sb.append('>'))
        return false;

    return true;
}

bool
ParallelArrayObject::toStringBuffer(JSContext *cx, bool useLocale, StringBuffer &sb)
{
    RootedParallelArrayObject self(cx, this);
    IndexInfo iv(cx);
    if (!iv.initialize(cx, self, 0))
        return false;
    RootedObject buffer(cx, this->buffer());
    return toStringBufferImpl(cx, iv, useLocale, buffer, sb);
}

bool
ParallelArrayObject::toString(JSContext *cx, CallArgs args)
{
    StringBuffer sb(cx);
    if (!as(&args.thisv().toObject())->toStringBuffer(cx, false, sb))
        return false;

    if (JSString *str = sb.finishString()) {
        args.rval().setString(str);
        return true;
    }

    return false;
}

bool
ParallelArrayObject::toLocaleString(JSContext *cx, CallArgs args)
{
    StringBuffer sb(cx);
    if (!as(&args.thisv().toObject())->toStringBuffer(cx, true, sb))
        return false;

    if (JSString *str = sb.finishString()) {
        args.rval().setString(str);
        return true;
    }

    return false;
}

void
ParallelArrayObject::mark(JSTracer *trc, JSObject *obj)
{
    gc::MarkSlot(trc, &obj->getSlotRef(SLOT_DIMENSIONS), "parallelarray.shape");
    gc::MarkSlot(trc, &obj->getSlotRef(SLOT_BUFFER), "parallelarray.buffer");
}

JSBool
ParallelArrayObject::lookupGeneric(JSContext *cx, HandleObject obj, HandleId id,
                                   MutableHandleObject objp, MutableHandleShape propp)
{
    uint32_t i;
    if (js_IdIsIndex(id, &i))
        return lookupElement(cx, obj, i, objp, propp);

    if (JSID_IS_ATOM(id, cx->names().length)) {
        MarkNonNativePropertyFound(obj, propp);
        objp.set(obj);
        return true;
    }

    RootedObject proto(cx, obj->getProto());
    if (proto)
        return JSObject::lookupGeneric(cx, proto, id, objp, propp);

    objp.set(NULL);
    propp.set(NULL);
    return true;
}

JSBool
ParallelArrayObject::lookupProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                    MutableHandleObject objp, MutableHandleShape propp)
{
    RootedId id(cx, NameToId(name));
    return lookupGeneric(cx, obj, id, objp, propp);
}

JSBool
ParallelArrayObject::lookupElement(JSContext *cx, HandleObject obj, uint32_t index,
                                   MutableHandleObject objp, MutableHandleShape propp)
{
    
    if (index < as(obj)->outermostDimension()) {
        MarkNonNativePropertyFound(obj, propp);
        objp.set(obj);
        return true;
    }

    objp.set(NULL);
    propp.set(NULL);
    return true;
}

JSBool
ParallelArrayObject::lookupSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                                   MutableHandleObject objp, MutableHandleShape propp)
{
    RootedId id(cx, SPECIALID_TO_JSID(sid));
    return lookupGeneric(cx, obj, id, objp, propp);
}

JSBool
ParallelArrayObject::defineGeneric(JSContext *cx, HandleObject obj, HandleId id, HandleValue Value,
                                   JSPropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_IMMUTABLE);
    return false;
}

JSBool
ParallelArrayObject::defineProperty(JSContext *cx, HandleObject obj,
                                    HandlePropertyName name, HandleValue value,
                                    JSPropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_IMMUTABLE);
    return false;
}

JSBool
ParallelArrayObject::defineElement(JSContext *cx, HandleObject obj,
                                   uint32_t index, HandleValue value,
                                   PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_IMMUTABLE);
    return false;
}

JSBool
ParallelArrayObject::defineSpecial(JSContext *cx, HandleObject obj,
                                   HandleSpecialId sid, HandleValue value,
                                   PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_IMMUTABLE);
    return false;
}

JSBool
ParallelArrayObject::getGeneric(JSContext *cx, HandleObject obj, HandleObject receiver,
                                HandleId id, MutableHandleValue vp)
{
    Value idval = IdToValue(id);

    uint32_t index;
    if (IsDefinitelyIndex(idval, &index))
        return getElement(cx, obj, receiver, index, vp);

    JSAtom *atom = ToAtom(cx, idval);
    if (!atom)
        return false;

    if (atom->isIndex(&index))
        return getElement(cx, obj, receiver, index, vp);

    Rooted<PropertyName*> name(cx, atom->asPropertyName());
    return getProperty(cx, obj, receiver, name, vp);
}

JSBool
ParallelArrayObject::getProperty(JSContext *cx, HandleObject obj, HandleObject receiver,
                                 HandlePropertyName name, MutableHandleValue vp)
{
    if (name == cx->names().length) {
        vp.setNumber(as(obj)->outermostDimension());
        return true;
    }

    RootedObject proto(cx, obj->getProto());
    if (proto)
        return JSObject::getProperty(cx, proto, receiver, name, vp);

    vp.setUndefined();
    return true;
}

JSBool
ParallelArrayObject::getElement(JSContext *cx, HandleObject obj, HandleObject receiver,
                                uint32_t index, MutableHandleValue vp)
{
    
    
    return as(obj)->getParallelArrayElement(cx, index, vp);
}

JSBool
ParallelArrayObject::getSpecial(JSContext *cx, HandleObject obj, HandleObject receiver,
                                HandleSpecialId sid, MutableHandleValue vp)
{
    if (!obj->getProto()) {
        vp.setUndefined();
        return true;
    }

    RootedId id(cx, SPECIALID_TO_JSID(sid));
    return baseops::GetProperty(cx, obj, receiver, id, vp);
}

JSBool
ParallelArrayObject::setGeneric(JSContext *cx, HandleObject obj, HandleId id,
                                MutableHandleValue vp, JSBool strict)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_IMMUTABLE);
    return false;
}

JSBool
ParallelArrayObject::setProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                 MutableHandleValue vp, JSBool strict)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_IMMUTABLE);
    return false;
}

JSBool
ParallelArrayObject::setElement(JSContext *cx, HandleObject obj, uint32_t index,
                                MutableHandleValue vp, JSBool strict)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_IMMUTABLE);
    return false;
}

JSBool
ParallelArrayObject::setSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                                MutableHandleValue vp, JSBool strict)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_IMMUTABLE);
    return false;
}

JSBool
ParallelArrayObject::getGenericAttributes(JSContext *cx, HandleObject obj, HandleId id,
                                          unsigned *attrsp)
{
    if (JSID_IS_ATOM(id, cx->names().length))
        *attrsp = JSPROP_PERMANENT | JSPROP_READONLY;
    else
        *attrsp = JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE;

    return true;
}

JSBool
ParallelArrayObject::getPropertyAttributes(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                           unsigned *attrsp)
{
    if (name == cx->names().length)
        *attrsp = JSPROP_PERMANENT | JSPROP_READONLY;
    return true;
}

JSBool
ParallelArrayObject::getElementAttributes(JSContext *cx, HandleObject obj, uint32_t index,
                                          unsigned *attrsp)
{
    *attrsp = JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE;
    return true;
}

JSBool
ParallelArrayObject::getSpecialAttributes(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                                          unsigned *attrsp)
{
    *attrsp = JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_ENUMERATE;
    return true;
}

JSBool
ParallelArrayObject::setGenericAttributes(JSContext *cx, HandleObject obj, HandleId id,
                                          unsigned *attrsp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_IMMUTABLE);
    return false;
}

JSBool
ParallelArrayObject::setPropertyAttributes(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                           unsigned *attrsp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_IMMUTABLE);
    return false;
}

JSBool
ParallelArrayObject::setElementAttributes(JSContext *cx, HandleObject obj, uint32_t index,
                                          unsigned *attrsp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_IMMUTABLE);
    return false;
}

JSBool
ParallelArrayObject::setSpecialAttributes(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                                          unsigned *attrsp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_IMMUTABLE);
    return false;
}

JSBool
ParallelArrayObject::deleteGeneric(JSContext *cx, HandleObject obj, HandleId id,
                                   MutableHandleValue rval, JSBool strict)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_IMMUTABLE);
    return false;
}

JSBool
ParallelArrayObject::deleteProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                    MutableHandleValue rval, JSBool strict)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_IMMUTABLE);
    return false;
}

JSBool
ParallelArrayObject::deleteElement(JSContext *cx, HandleObject obj, uint32_t index,
                                   MutableHandleValue rval, JSBool strict)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_IMMUTABLE);
    return false;
}

JSBool
ParallelArrayObject::deleteSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                                   MutableHandleValue rval, JSBool strict)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PAR_ARRAY_IMMUTABLE);
    return false;
}

bool
ParallelArrayObject::enumerate(JSContext *cx, HandleObject obj, unsigned flags,
                               AutoIdVector *props)
{
    RootedParallelArrayObject source(cx, as(obj));

    if (flags & JSITER_HIDDEN && !props->append(NameToId(cx->names().length)))
        return false;

    
    if (source->outermostDimension() > 0) {
        for (uint32_t i = 0; i < source->outermostDimension(); i++)
            props->append(INT_TO_JSID(i));
    }

    if (flags & JSITER_OWNONLY)
        return true;

    RootedObject proto(cx, obj->getProto());
    if (proto) {
        AutoIdVector protoProps(cx);
        if (!GetPropertyNames(cx, proto, flags, &protoProps))
            return false;

        
        
        uint32_t dummy;
        for (uint32_t i = 0; i < protoProps.length(); i++) {
            if (!js_IdIsIndex(protoProps[i], &dummy) && !props->append(protoProps[i]))
                return false;
        }
    }

    return true;
}

JSObject *
js_InitParallelArrayClass(JSContext *cx, JSObject *obj)
{
    return ParallelArrayObject::initClass(cx, obj);
}
