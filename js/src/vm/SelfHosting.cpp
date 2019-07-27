





#include "vm/SelfHosting.h"

#include "jscntxt.h"
#include "jscompartment.h"
#include "jsfriendapi.h"
#include "jshashutil.h"
#include "jsobj.h"
#include "jswrapper.h"
#include "selfhosted.out.h"

#include "builtin/Intl.h"
#include "builtin/Object.h"
#include "builtin/SelfHostingDefines.h"
#include "builtin/TypedObject.h"
#include "builtin/WeakSetObject.h"
#include "gc/Marking.h"
#include "vm/Compression.h"
#include "vm/ForkJoin.h"
#include "vm/Interpreter.h"
#include "vm/String.h"
#include "vm/TypedArrayCommon.h"

#include "jsfuninlines.h"
#include "jsscriptinlines.h"

#include "vm/BooleanObject-inl.h"
#include "vm/NumberObject-inl.h"
#include "vm/StringObject-inl.h"

using namespace js;
using namespace js::selfhosted;

using JS::AutoCheckCannotGC;

static void
selfHosting_ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
    PrintError(cx, stderr, message, report, true);
}

static const JSClass self_hosting_global_class = {
    "self-hosting-global", JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub,  JS_DeletePropertyStub,
    JS_PropertyStub,  JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub,
    JS_ConvertStub,   nullptr,
    nullptr, nullptr, nullptr,
    JS_GlobalObjectTraceHook
};

bool
js::intrinsic_ToObject(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedValue val(cx, args[0]);
    RootedObject obj(cx, ToObject(cx, val));
    if (!obj)
        return false;
    args.rval().setObject(*obj);
    return true;
}

bool
js::intrinsic_IsObject(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    Value val = args[0];
    bool isObject = val.isObject();
    args.rval().setBoolean(isObject);
    return true;
}

bool
js::intrinsic_ToInteger(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    double result;
    if (!ToInteger(cx, args[0], &result))
        return false;
    args.rval().setNumber(result);
    return true;
}

bool
js::intrinsic_ToString(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedString str(cx);
    str = ToString<CanGC>(cx, args[0]);
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}

bool
js::intrinsic_IsCallable(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setBoolean(IsCallable(args[0]));
    return true;
}

static bool
intrinsic_IsConstructor(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setBoolean(IsConstructor(args[0]));
    return true;
}

static bool
intrinsic_OwnPropertyKeys(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return GetOwnPropertyKeys(cx, args,
                              JSITER_OWNONLY | JSITER_HIDDEN | JSITER_SYMBOLS);
}

bool
js::intrinsic_ThrowError(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() >= 1);
    uint32_t errorNumber = args[0].toInt32();

#ifdef DEBUG
    const JSErrorFormatString *efs = js_GetErrorMessage(nullptr, errorNumber);
    JS_ASSERT(efs->argCount == args.length() - 1);
#endif

    JSAutoByteString errorArgs[3];
    for (unsigned i = 1; i < 4 && i < args.length(); i++) {
        RootedValue val(cx, args[i]);
        if (val.isInt32()) {
            JSString *str = ToString<CanGC>(cx, val);
            if (!str)
                return false;
            errorArgs[i - 1].encodeLatin1(cx, str);
        } else if (val.isString()) {
            errorArgs[i - 1].encodeLatin1(cx, val.toString());
        } else {
            errorArgs[i - 1].initBytes(DecompileValueGenerator(cx, JSDVG_SEARCH_STACK, val, NullPtr()));
        }
        if (!errorArgs[i - 1])
            return false;
    }

    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, errorNumber,
                         errorArgs[0].ptr(), errorArgs[1].ptr(), errorArgs[2].ptr());
    return false;
}





static bool
intrinsic_AssertionFailed(JSContext *cx, unsigned argc, Value *vp)
{
#ifdef DEBUG
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() > 0) {
        
        JSString *str = ToString<CanGC>(cx, args[0]);
        if (str) {
            fprintf(stderr, "Self-hosted JavaScript assertion info: ");
            str->dumpCharsNoNewline();
            fputc('\n', stderr);
        }
    }
#endif
    JS_ASSERT(false);
    return false;
}

static bool
intrinsic_MakeConstructible(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 2);
    JS_ASSERT(args[0].isObject());
    JS_ASSERT(args[0].toObject().is<JSFunction>());
    JS_ASSERT(args[1].isObject());

    
    
    RootedObject ctor(cx, &args[0].toObject());
    if (!JSObject::defineProperty(cx, ctor, cx->names().prototype, args[1],
                                  JS_PropertyStub, JS_StrictPropertyStub,
                                  JSPROP_READONLY | JSPROP_ENUMERATE | JSPROP_PERMANENT))
    {
        return false;
    }

    ctor->as<JSFunction>().setIsSelfHostedConstructor();
    args.rval().setUndefined();
    return true;
}









static bool
intrinsic_DecompileArg(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 2);

    RootedValue value(cx, args[1]);
    ScopedJSFreePtr<char> str(DecompileArgument(cx, args[0].toInt32(), value));
    if (!str)
        return false;
    RootedAtom atom(cx, Atomize(cx, str, strlen(str)));
    if (!atom)
        return false;
    args.rval().setString(atom);
    return true;
}
















static bool
intrinsic_SetScriptHints(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() >= 2);
    JS_ASSERT(args[0].isObject() && args[0].toObject().is<JSFunction>());
    JS_ASSERT(args[1].isObject());

    RootedFunction fun(cx, &args[0].toObject().as<JSFunction>());
    RootedScript funScript(cx, fun->getOrCreateScript(cx));
    if (!funScript)
        return false;
    RootedObject flags(cx, &args[1].toObject());

    RootedId id(cx);
    RootedValue propv(cx);

    id = AtomToId(Atomize(cx, "cloneAtCallsite", strlen("cloneAtCallsite")));
    if (!JSObject::getGeneric(cx, flags, flags, id, &propv))
        return false;
    if (ToBoolean(propv))
        funScript->setShouldCloneAtCallsite();

    id = AtomToId(Atomize(cx, "inline", strlen("inline")));
    if (!JSObject::getGeneric(cx, flags, flags, id, &propv))
        return false;
    if (ToBoolean(propv))
        funScript->setShouldInline();

    args.rval().setUndefined();
    return true;
}

#ifdef DEBUG



bool
intrinsic_Dump(ThreadSafeContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    js_DumpValue(args[0]);
    if (args[0].isObject()) {
        fprintf(stderr, "\n");
        js_DumpObject(&args[0].toObject());
    }
    args.rval().setUndefined();
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(intrinsic_Dump_jitInfo, intrinsic_Dump_jitInfo,
                                      intrinsic_Dump);

bool
intrinsic_ParallelSpew(ThreadSafeContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 1);
    JS_ASSERT(args[0].isString());

    AutoCheckCannotGC nogc;
    ScopedThreadSafeStringInspector inspector(args[0].toString());
    if (!inspector.ensureChars(cx, nogc))
        return false;

    ScopedJSFreePtr<char> bytes;
    if (inspector.hasLatin1Chars())
        bytes = JS::CharsToNewUTF8CharsZ(cx, inspector.latin1Range()).c_str();
    else
        bytes = JS::CharsToNewUTF8CharsZ(cx, inspector.twoByteRange()).c_str();

    parallel::Spew(parallel::SpewOps, bytes);

    args.rval().setUndefined();
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(intrinsic_ParallelSpew_jitInfo, intrinsic_ParallelSpew_jitInfo,
                                      intrinsic_ParallelSpew);
#endif










static bool
intrinsic_ForkJoin(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return ForkJoin(cx, args);
}





static bool
intrinsic_ForkJoinNumWorkers(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setInt32(cx->runtime()->threadPool.numWorkers());
    return true;
}












bool
js::intrinsic_ForkJoinGetSlice(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isInt32());
    args.rval().set(args[0]);
    return true;
}

static bool
intrinsic_ForkJoinGetSlicePar(ForkJoinContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isInt32());

    uint16_t sliceId;
    if (cx->getSlice(&sliceId))
        args.rval().setInt32(sliceId);
    else
        args.rval().setInt32(ThreadPool::MAX_SLICE_ID);

    return true;
}

JS_JITINFO_NATIVE_PARALLEL(intrinsic_ForkJoinGetSlice_jitInfo,
                           intrinsic_ForkJoinGetSlicePar);





bool
js::intrinsic_NewDenseArray(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    if (!args[0].isInt32()) {
        JS_ReportError(cx, "Expected int32 as second argument");
        return false;
    }
    uint32_t length = args[0].toInt32();

    
    RootedObject buffer(cx, NewDenseFullyAllocatedArray(cx, length));
    if (!buffer)
        return false;

    types::TypeObject *newtype = types::GetTypeCallerInitObject(cx, JSProto_Array);
    if (!newtype)
        return false;
    buffer->setType(newtype);

    JSObject::EnsureDenseResult edr = buffer->ensureDenseElements(cx, length, 0);
    switch (edr) {
      case JSObject::ED_OK:
        args.rval().setObject(*buffer);
        return true;

      case JSObject::ED_SPARSE: 
        JS_ASSERT(!"%EnsureDenseArrayElements() would yield sparse array");
        JS_ReportError(cx, "%EnsureDenseArrayElements() would yield sparse array");
        break;

      case JSObject::ED_FAILED:
        break;
    }
    return false;
}













bool
js::intrinsic_UnsafePutElements(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if ((args.length() % 3) != 0) {
        JS_ReportError(cx, "Incorrect number of arguments, not divisible by 3");
        return false;
    }

    for (uint32_t base = 0; base < args.length(); base += 3) {
        uint32_t arri = base;
        uint32_t idxi = base+1;
        uint32_t elemi = base+2;

        JS_ASSERT(args[arri].isObject());
        JS_ASSERT(args[arri].toObject().isNative() || IsTypedObjectArray(args[arri].toObject()));
        JS_ASSERT(args[idxi].isInt32());

        RootedObject arrobj(cx, &args[arri].toObject());
        uint32_t idx = args[idxi].toInt32();

        if (IsAnyTypedArray(arrobj.get()) || arrobj->is<TypedObject>()) {
            JS_ASSERT_IF(IsAnyTypedArray(arrobj.get()), idx < AnyTypedArrayLength(arrobj.get()));
            JS_ASSERT_IF(arrobj->is<TypedObject>(), idx < uint32_t(arrobj->as<TypedObject>().length()));
            RootedValue tmp(cx, args[elemi]);
            
            if (!JSObject::setElement(cx, arrobj, arrobj, idx, &tmp, false))
                return false;
        } else {
            JS_ASSERT(idx < arrobj->getDenseInitializedLength());
            arrobj->setDenseElementWithType(cx, idx, args[elemi]);
        }
    }

    args.rval().setUndefined();
    return true;
}

bool
js::intrinsic_DefineDataProperty(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    MOZ_ASSERT(args.length() == 4);
    MOZ_ASSERT(args[0].isObject());
    MOZ_ASSERT(args[3].isInt32());

    RootedObject obj(cx, &args[0].toObject());
    RootedId id(cx);
    if (!ValueToId<CanGC>(cx, args[1], &id))
        return false;
    RootedValue value(cx, args[2]);
    unsigned attributes = args[3].toInt32();

    Rooted<PropDesc> desc(cx);

    MOZ_ASSERT(bool(attributes & ATTR_ENUMERABLE) != bool(attributes & ATTR_NONENUMERABLE),
               "_DefineDataProperty must receive either ATTR_ENUMERABLE xor ATTR_NONENUMERABLE");
    PropDesc::Enumerability enumerable =
        PropDesc::Enumerability(bool(attributes & ATTR_ENUMERABLE));

    MOZ_ASSERT(bool(attributes & ATTR_CONFIGURABLE) != bool(attributes & ATTR_NONCONFIGURABLE),
               "_DefineDataProperty must receive either ATTR_CONFIGURABLE xor "
               "ATTR_NONCONFIGURABLE");
    PropDesc::Configurability configurable =
        PropDesc::Configurability(bool(attributes & ATTR_CONFIGURABLE));

    MOZ_ASSERT(bool(attributes & ATTR_WRITABLE) != bool(attributes & ATTR_NONWRITABLE),
               "_DefineDataProperty must receive either ATTR_WRITABLE xor ATTR_NONWRITABLE");
    PropDesc::Writability writable =
        PropDesc::Writability(bool(attributes & ATTR_WRITABLE));

    desc.set(PropDesc(value, writable, enumerable, configurable));

    bool result;
    return DefineProperty(cx, obj, id, desc, true, &result);
}

bool
js::intrinsic_UnsafeSetReservedSlot(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 3);
    JS_ASSERT(args[0].isObject());
    JS_ASSERT(args[1].isInt32());

    args[0].toObject().setReservedSlot(args[1].toPrivateUint32(), args[2]);
    args.rval().setUndefined();
    return true;
}

bool
js::intrinsic_UnsafeGetReservedSlot(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 2);
    JS_ASSERT(args[0].isObject());
    JS_ASSERT(args[1].isInt32());

    args.rval().set(args[0].toObject().getReservedSlot(args[1].toPrivateUint32()));
    return true;
}

bool
js::intrinsic_HaveSameClass(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 2);
    JS_ASSERT(args[0].isObject());
    JS_ASSERT(args[1].isObject());

    args.rval().setBoolean(args[0].toObject().getClass() == args[1].toObject().getClass());
    return true;
}

bool
js::intrinsic_IsPackedArray(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 1);
    JS_ASSERT(args[0].isObject());

    JSObject *obj = &args[0].toObject();
    bool isPacked = obj->is<ArrayObject>() && !obj->hasLazyType() &&
                    !obj->type()->hasAllFlags(types::OBJECT_FLAG_NON_PACKED) &&
                    obj->getDenseInitializedLength() == obj->as<ArrayObject>().length();

    args.rval().setBoolean(isPacked);
    return true;
}

static bool
intrinsic_GetIteratorPrototype(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 0);

    JSObject *obj = GlobalObject::getOrCreateIteratorPrototype(cx, cx->global());
    if (!obj)
        return false;

    args.rval().setObject(*obj);
    return true;
}

static bool
intrinsic_NewArrayIterator(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 0);

    RootedObject proto(cx, GlobalObject::getOrCreateArrayIteratorPrototype(cx, cx->global()));
    if (!proto)
        return false;

    JSObject *obj = NewObjectWithGivenProto(cx, proto->getClass(), proto, cx->global());
    if (!obj)
        return false;

    args.rval().setObject(*obj);
    return true;
}

static bool
intrinsic_IsArrayIterator(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 1);
    JS_ASSERT(args[0].isObject());

    args.rval().setBoolean(args[0].toObject().is<ArrayIteratorObject>());
    return true;
}

static bool
intrinsic_NewStringIterator(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 0);

    RootedObject proto(cx, GlobalObject::getOrCreateStringIteratorPrototype(cx, cx->global()));
    if (!proto)
        return false;

    JSObject *obj = NewObjectWithGivenProto(cx, &StringIteratorObject::class_, proto, cx->global());
    if (!obj)
        return false;

    args.rval().setObject(*obj);
    return true;
}

static bool
intrinsic_IsStringIterator(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 1);
    JS_ASSERT(args[0].isObject());

    args.rval().setBoolean(args[0].toObject().is<StringIteratorObject>());
    return true;
}

static bool
intrinsic_IsWeakSet(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 1);
    JS_ASSERT(args[0].isObject());

    args.rval().setBoolean(args[0].toObject().is<WeakSetObject>());
    return true;
}














static bool
intrinsic_ParallelTestsShouldPass(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setBoolean(ParallelTestsShouldPass(cx));
    return true;
}





bool
js::intrinsic_ShouldForceSequential(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setBoolean(cx->runtime()->forkJoinWarmup ||
                           InParallelSection());
    return true;
}

bool
js::intrinsic_InParallelSection(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setBoolean(false);
    return true;
}

static bool
intrinsic_InParallelSectionPar(ForkJoinContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setBoolean(true);
    return true;
}

JS_JITINFO_NATIVE_PARALLEL(intrinsic_InParallelSection_jitInfo,
                           intrinsic_InParallelSectionPar);





bool
js::intrinsic_ObjectIsTypedObject(JSContext *cx, unsigned argc, Value *vp)
{
    return js::ObjectIsTypedObject(cx, argc, vp);
}

bool
js::intrinsic_ObjectIsTransparentTypedObject(JSContext *cx, unsigned argc, Value *vp)
{
    return js::ObjectIsTransparentTypedObject(cx, argc, vp);
}

bool
js::intrinsic_ObjectIsOpaqueTypedObject(JSContext *cx, unsigned argc, Value *vp)
{
    return js::ObjectIsOpaqueTypedObject(cx, argc, vp);
}

bool
js::intrinsic_ObjectIsTypeDescr(JSContext *cx, unsigned argc, Value *vp)
{
    return js::ObjectIsTypeDescr(cx, argc, vp);
}

bool
js::intrinsic_TypeDescrIsSimpleType(JSContext *cx, unsigned argc, Value *vp)
{
    return js::TypeDescrIsSimpleType(cx, argc, vp);
}

bool
js::intrinsic_TypeDescrIsArrayType(JSContext *cx, unsigned argc, Value *vp)
{
    return js::TypeDescrIsArrayType(cx, argc, vp);
}

bool
js::intrinsic_TypeDescrIsUnsizedArrayType(JSContext *cx, unsigned argc, Value *vp)
{
    return js::TypeDescrIsUnsizedArrayType(cx, argc, vp);
}

bool
js::intrinsic_TypeDescrIsSizedArrayType(JSContext *cx, unsigned argc, Value *vp)
{
    return js::TypeDescrIsSizedArrayType(cx, argc, vp);
}





static bool
intrinsic_RuntimeDefaultLocale(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    const char *locale = cx->runtime()->getDefaultLocale();
    if (!locale) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_DEFAULT_LOCALE_ERROR);
        return false;
    }

    RootedString jslocale(cx, JS_NewStringCopyZ(cx, locale));
    if (!jslocale)
        return false;

    args.rval().setString(jslocale);
    return true;
}

bool
js::intrinsic_IsConstructing(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 0);

    ScriptFrameIter iter(cx);
    bool isConstructing = iter.isConstructing();
    args.rval().setBoolean(isConstructing);
    return true;
}

static const JSFunctionSpec intrinsic_functions[] = {
    JS_FN("ToObject",                intrinsic_ToObject,                1,0),
    JS_FN("IsObject",                intrinsic_IsObject,                1,0),
    JS_FN("ToInteger",               intrinsic_ToInteger,               1,0),
    JS_FN("ToString",                intrinsic_ToString,                1,0),
    JS_FN("IsCallable",              intrinsic_IsCallable,              1,0),
    JS_FN("IsConstructor",           intrinsic_IsConstructor,           1,0),
    JS_FN("OwnPropertyKeys",         intrinsic_OwnPropertyKeys,         1,0),
    JS_FN("ThrowError",              intrinsic_ThrowError,              4,0),
    JS_FN("AssertionFailed",         intrinsic_AssertionFailed,         1,0),
    JS_FN("SetScriptHints",          intrinsic_SetScriptHints,          2,0),
    JS_FN("MakeConstructible",       intrinsic_MakeConstructible,       1,0),
    JS_FN("_IsConstructing",         intrinsic_IsConstructing,          0,0),
    JS_FN("DecompileArg",            intrinsic_DecompileArg,            2,0),
    JS_FN("RuntimeDefaultLocale",    intrinsic_RuntimeDefaultLocale,    0,0),

    JS_FN("UnsafePutElements",       intrinsic_UnsafePutElements,       3,0),
    JS_FN("_DefineDataProperty",     intrinsic_DefineDataProperty,      4,0),
    JS_FN("UnsafeSetReservedSlot",   intrinsic_UnsafeSetReservedSlot,   3,0),
    JS_FN("UnsafeGetReservedSlot",   intrinsic_UnsafeGetReservedSlot,   2,0),
    JS_FN("HaveSameClass",           intrinsic_HaveSameClass,           2,0),
    JS_FN("IsPackedArray",           intrinsic_IsPackedArray,           1,0),

    JS_FN("GetIteratorPrototype",    intrinsic_GetIteratorPrototype,    0,0),

    JS_FN("NewArrayIterator",        intrinsic_NewArrayIterator,        0,0),
    JS_FN("IsArrayIterator",         intrinsic_IsArrayIterator,         1,0),

    JS_FN("NewStringIterator",       intrinsic_NewStringIterator,       0,0),
    JS_FN("IsStringIterator",        intrinsic_IsStringIterator,        1,0),

    JS_FN("IsWeakSet",               intrinsic_IsWeakSet,               1,0),

    JS_FN("ForkJoin",                intrinsic_ForkJoin,                5,0),
    JS_FN("ForkJoinNumWorkers",      intrinsic_ForkJoinNumWorkers,      0,0),
    JS_FN("NewDenseArray",           intrinsic_NewDenseArray,           1,0),
    JS_FN("ShouldForceSequential",   intrinsic_ShouldForceSequential,   0,0),
    JS_FN("ParallelTestsShouldPass", intrinsic_ParallelTestsShouldPass, 0,0),
    JS_FNINFO("ClearThreadLocalArenas",
              intrinsic_ClearThreadLocalArenas,
              &intrinsic_ClearThreadLocalArenasInfo, 0,0),
    JS_FNINFO("SetForkJoinTargetRegion",
              intrinsic_SetForkJoinTargetRegion,
              &intrinsic_SetForkJoinTargetRegionInfo, 2, 0),
    JS_FNINFO("ForkJoinGetSlice",
              intrinsic_ForkJoinGetSlice,
              &intrinsic_ForkJoinGetSlice_jitInfo, 1, 0),
    JS_FNINFO("InParallelSection",
              intrinsic_InParallelSection,
              &intrinsic_InParallelSection_jitInfo, 0, 0),

    
    JS_FN("NewOpaqueTypedObject",
          js::NewOpaqueTypedObject,
          1, 0),
    JS_FN("NewDerivedTypedObject",
          js::NewDerivedTypedObject,
          3, 0),
    JS_FNINFO("AttachTypedObject",
              JSNativeThreadSafeWrapper<js::AttachTypedObject>,
              &js::AttachTypedObjectJitInfo, 3, 0),
    JS_FNINFO("SetTypedObjectOffset",
              intrinsic_SetTypedObjectOffset,
              &js::intrinsic_SetTypedObjectOffsetJitInfo, 2, 0),
    JS_FNINFO("ObjectIsTypeDescr",
              intrinsic_ObjectIsTypeDescr,
              &js::ObjectIsTypeDescrJitInfo, 1, 0),
    JS_FNINFO("ObjectIsTypedObject",
              intrinsic_ObjectIsTypedObject,
              &js::ObjectIsTypedObjectJitInfo, 1, 0),
    JS_FNINFO("ObjectIsTransparentTypedObject",
              intrinsic_ObjectIsTransparentTypedObject,
              &js::ObjectIsTransparentTypedObjectJitInfo, 1, 0),
    JS_FNINFO("TypedObjectIsAttached",
              JSNativeThreadSafeWrapper<js::TypedObjectIsAttached>,
              &js::TypedObjectIsAttachedJitInfo, 1, 0),
    JS_FNINFO("ObjectIsOpaqueTypedObject",
              intrinsic_ObjectIsOpaqueTypedObject,
              &js::ObjectIsOpaqueTypedObjectJitInfo, 1, 0),
    JS_FNINFO("TypeDescrIsArrayType",
              intrinsic_TypeDescrIsArrayType,
              &js::TypeDescrIsArrayTypeJitInfo, 1, 0),
    JS_FNINFO("TypeDescrIsUnsizedArrayType",
              intrinsic_TypeDescrIsUnsizedArrayType,
              &js::TypeDescrIsUnsizedArrayTypeJitInfo, 1, 0),
    JS_FNINFO("TypeDescrIsSizedArrayType",
              intrinsic_TypeDescrIsSizedArrayType,
              &js::TypeDescrIsSizedArrayTypeJitInfo, 1, 0),
    JS_FNINFO("TypeDescrIsSimpleType",
              intrinsic_TypeDescrIsSimpleType,
              &js::TypeDescrIsSimpleTypeJitInfo, 1, 0),
    JS_FNINFO("ClampToUint8",
              JSNativeThreadSafeWrapper<js::ClampToUint8>,
              &js::ClampToUint8JitInfo, 1, 0),
    JS_FNINFO("Memcpy",
              JSNativeThreadSafeWrapper<js::Memcpy>,
              &js::MemcpyJitInfo, 5, 0),
    JS_FN("GetTypedObjectModule", js::GetTypedObjectModule, 0, 0),
    JS_FN("GetFloat32x4TypeDescr", js::GetFloat32x4TypeDescr, 0, 0),
    JS_FN("GetInt32x4TypeDescr", js::GetInt32x4TypeDescr, 0, 0),

#define LOAD_AND_STORE_SCALAR_FN_DECLS(_constant, _type, _name)               \
    JS_FNINFO("Store_" #_name,                                                \
              JSNativeThreadSafeWrapper<js::StoreScalar##_type::Func>,        \
              &js::StoreScalar##_type::JitInfo, 3, 0),                        \
    JS_FNINFO("Load_" #_name,                                                 \
              JSNativeThreadSafeWrapper<js::LoadScalar##_type::Func>,         \
              &js::LoadScalar##_type::JitInfo, 3, 0),
    JS_FOR_EACH_UNIQUE_SCALAR_TYPE_REPR_CTYPE(LOAD_AND_STORE_SCALAR_FN_DECLS)

#define LOAD_AND_STORE_REFERENCE_FN_DECLS(_constant, _type, _name)              \
    JS_FNINFO("Store_" #_name,                                                  \
              JSNativeThreadSafeWrapper<js::StoreReference##_type::Func>,       \
              &js::StoreReference##_type::JitInfo, 3, 0),                       \
    JS_FNINFO("Load_" #_name,                                                   \
              JSNativeThreadSafeWrapper<js::LoadReference##_type::Func>,        \
              &js::LoadReference##_type::JitInfo, 3, 0),
    JS_FOR_EACH_REFERENCE_TYPE_REPR(LOAD_AND_STORE_REFERENCE_FN_DECLS)

    
    JS_FN("intl_availableCalendars", intl_availableCalendars, 1,0),
    JS_FN("intl_availableCollations", intl_availableCollations, 1,0),
    JS_FN("intl_Collator", intl_Collator, 2,0),
    JS_FN("intl_Collator_availableLocales", intl_Collator_availableLocales, 0,0),
    JS_FN("intl_CompareStrings", intl_CompareStrings, 3,0),
    JS_FN("intl_DateTimeFormat", intl_DateTimeFormat, 2,0),
    JS_FN("intl_DateTimeFormat_availableLocales", intl_DateTimeFormat_availableLocales, 0,0),
    JS_FN("intl_FormatDateTime", intl_FormatDateTime, 2,0),
    JS_FN("intl_FormatNumber", intl_FormatNumber, 2,0),
    JS_FN("intl_NumberFormat", intl_NumberFormat, 2,0),
    JS_FN("intl_NumberFormat_availableLocales", intl_NumberFormat_availableLocales, 0,0),
    JS_FN("intl_numberingSystem", intl_numberingSystem, 1,0),
    JS_FN("intl_patternForSkeleton", intl_patternForSkeleton, 2,0),

    
    JS_FN("regexp_exec_no_statics", regexp_exec_no_statics, 2,0),
    JS_FN("regexp_test_no_statics", regexp_test_no_statics, 2,0),

#ifdef DEBUG
    JS_FNINFO("Dump",
              JSNativeThreadSafeWrapper<intrinsic_Dump>,
              &intrinsic_Dump_jitInfo, 1,0),

    JS_FNINFO("ParallelSpew",
              JSNativeThreadSafeWrapper<intrinsic_ParallelSpew>,
              &intrinsic_ParallelSpew_jitInfo, 1,0),
#endif

    JS_FS_END
};

void
js::FillSelfHostingCompileOptions(CompileOptions &options)
{
    













    options.setIntroductionType("self-hosted");
    options.setFileAndLine("self-hosted", 1);
    options.setSelfHostingMode(true);
    options.setCanLazilyParse(false);
    options.setVersion(JSVERSION_LATEST);
    options.werrorOption = true;
    options.strictOption = true;

#ifdef DEBUG
    options.extraWarningsOption = true;
#endif
}

bool
JSRuntime::initSelfHosting(JSContext *cx)
{
    JS_ASSERT(!selfHostingGlobal_);

    if (cx->runtime()->parentRuntime) {
        selfHostingGlobal_ = cx->runtime()->parentRuntime->selfHostingGlobal_;
        return true;
    }

    



    JS::AutoDisableGenerationalGC disable(cx->runtime());

    JS::CompartmentOptions compartmentOptions;
    compartmentOptions.setDiscardSource(true);
    if (!(selfHostingGlobal_ = JS_NewGlobalObject(cx, &self_hosting_global_class,
                                                  nullptr, JS::DontFireOnNewGlobalHook,
                                                  compartmentOptions)))
        return false;
    JSAutoCompartment ac(cx, selfHostingGlobal_);
    Rooted<GlobalObject*> shg(cx, &selfHostingGlobal_->as<GlobalObject>());
    selfHostingGlobal_->compartment()->isSelfHosting = true;
    selfHostingGlobal_->compartment()->isSystem = true;
    




    if (!GlobalObject::initStandardClasses(cx, shg))
        return false;

    if (!JS_DefineFunctions(cx, shg, intrinsic_functions))
        return false;

    JS_FireOnNewGlobalObject(cx, shg);

    CompileOptions options(cx);
    FillSelfHostingCompileOptions(options);

    




    JSErrorReporter oldReporter = JS_SetErrorReporter(cx->runtime(), selfHosting_ErrorReporter);
    RootedValue rv(cx);
    bool ok = false;

    char *filename = getenv("MOZ_SELFHOSTEDJS");
    if (filename) {
        RootedScript script(cx);
        if (Compile(cx, shg, options, filename, &script))
            ok = Execute(cx, script, *shg.get(), rv.address());
    } else {
        uint32_t srcLen = GetRawScriptsSize();

        const unsigned char *compressed = compressedSources;
        uint32_t compressedLen = GetCompressedSize();
        ScopedJSFreePtr<char> src(selfHostingGlobal_->pod_malloc<char>(srcLen));
        if (!src || !DecompressString(compressed, compressedLen,
                                      reinterpret_cast<unsigned char *>(src.get()), srcLen))
        {
            return false;
        }

        ok = Evaluate(cx, shg, options, src, srcLen, &rv);
    }
    JS_SetErrorReporter(cx->runtime(), oldReporter);
    return ok;
}

void
JSRuntime::finishSelfHosting()
{
    selfHostingGlobal_ = nullptr;
}

void
JSRuntime::markSelfHostingGlobal(JSTracer *trc)
{
    if (selfHostingGlobal_ && !parentRuntime)
        MarkObjectRoot(trc, &selfHostingGlobal_, "self-hosting global");
}

bool
JSRuntime::isSelfHostingCompartment(JSCompartment *comp)
{
    return selfHostingGlobal_->compartment() == comp;
}

static bool
CloneValue(JSContext *cx, HandleValue selfHostedValue, MutableHandleValue vp);

static bool
GetUnclonedValue(JSContext *cx, HandleObject selfHostedObject, HandleId id, MutableHandleValue vp)
{
    vp.setUndefined();

    if (JSID_IS_INT(id)) {
        size_t index = JSID_TO_INT(id);
        if (index < selfHostedObject->getDenseInitializedLength() &&
            !selfHostedObject->getDenseElement(index).isMagic(JS_ELEMENTS_HOLE))
        {
            vp.set(selfHostedObject->getDenseElement(JSID_TO_INT(id)));
            return true;
        }
    }

    
    
    
    
    if (JSID_IS_STRING(id) && !JSID_TO_STRING(id)->isPermanentAtom()) {
        JS_ASSERT(selfHostedObject->is<GlobalObject>());
        RootedValue value(cx, IdToValue(id));
        return js_ReportValueErrorFlags(cx, JSREPORT_ERROR, JSMSG_NO_SUCH_SELF_HOSTED_PROP,
                                        JSDVG_IGNORE_STACK, value, NullPtr(), nullptr, nullptr);
    }

    RootedShape shape(cx, selfHostedObject->nativeLookupPure(id));
    if (!shape) {
        RootedValue value(cx, IdToValue(id));
        return js_ReportValueErrorFlags(cx, JSREPORT_ERROR, JSMSG_NO_SUCH_SELF_HOSTED_PROP,
                                        JSDVG_IGNORE_STACK, value, NullPtr(), nullptr, nullptr);
    }

    JS_ASSERT(shape->hasSlot() && shape->hasDefaultGetter());
    vp.set(selfHostedObject->getSlot(shape->slot()));
    return true;
}

static bool
CloneProperties(JSContext *cx, HandleObject selfHostedObject, HandleObject clone)
{
    AutoIdVector ids(cx);

    for (size_t i = 0; i < selfHostedObject->getDenseInitializedLength(); i++) {
        if (!selfHostedObject->getDenseElement(i).isMagic(JS_ELEMENTS_HOLE)) {
            if (!ids.append(INT_TO_JSID(i)))
                return false;
        }
    }

    for (Shape::Range<NoGC> range(selfHostedObject->lastProperty()); !range.empty(); range.popFront()) {
        Shape &shape = range.front();
        if (shape.enumerable() && !ids.append(shape.propid()))
            return false;
    }

    RootedId id(cx);
    RootedValue val(cx);
    RootedValue selfHostedValue(cx);
    for (uint32_t i = 0; i < ids.length(); i++) {
        id = ids[i];
        if (!GetUnclonedValue(cx, selfHostedObject, id, &selfHostedValue))
            return false;
        if (!CloneValue(cx, selfHostedValue, &val) ||
            !JS_DefinePropertyById(cx, clone, id, val, 0))
        {
            return false;
        }
    }

    return true;
}

static JSString *
CloneString(JSContext *cx, JSFlatString *selfHostedString)
{
    size_t len = selfHostedString->length();
    {
        JS::AutoCheckCannotGC nogc;
        JSString *clone;
        if (selfHostedString->hasLatin1Chars())
            clone = NewStringCopyN<NoGC>(cx, selfHostedString->latin1Chars(nogc), len);
        else
            clone = NewStringCopyNDontDeflate<NoGC>(cx, selfHostedString->twoByteChars(nogc), len);
        if (clone)
            return clone;
    }

    AutoStableStringChars chars(cx);
    if (!chars.init(cx, selfHostedString))
        return nullptr;

    return chars.isLatin1()
           ? NewStringCopyN<CanGC>(cx, chars.latin1Range().start().get(), len)
           : NewStringCopyNDontDeflate<CanGC>(cx, chars.twoByteRange().start().get(), len);
}

static JSObject *
CloneObject(JSContext *cx, HandleObject selfHostedObject)
{
    AutoCycleDetector detect(cx, selfHostedObject);
    if (!detect.init())
        return nullptr;
    if (detect.foundCycle()) {
        JS_ReportError(cx, "SelfHosted cloning cannot handle cyclic object graphs.");
        return nullptr;
    }

    RootedObject clone(cx);
    if (selfHostedObject->is<JSFunction>()) {
        RootedFunction selfHostedFunction(cx, &selfHostedObject->as<JSFunction>());
        bool hasName = selfHostedFunction->atom() != nullptr;
        
        JS_ASSERT(!selfHostedFunction->isArrow());
        js::gc::AllocKind kind = hasName
                                 ? JSFunction::ExtendedFinalizeKind
                                 : selfHostedFunction->getAllocKind();
        clone = CloneFunctionObject(cx, selfHostedFunction, cx->global(), kind, TenuredObject);
        
        
        if (clone && hasName)
            clone->as<JSFunction>().setExtendedSlot(0, StringValue(selfHostedFunction->atom()));
    } else if (selfHostedObject->is<RegExpObject>()) {
        RegExpObject &reobj = selfHostedObject->as<RegExpObject>();
        RootedAtom source(cx, reobj.getSource());
        JS_ASSERT(source->isPermanentAtom());
        clone = RegExpObject::createNoStatics(cx, source, reobj.getFlags(), nullptr, cx->tempLifoAlloc());
    } else if (selfHostedObject->is<DateObject>()) {
        clone = JS_NewDateObjectMsec(cx, selfHostedObject->as<DateObject>().UTCTime().toNumber());
    } else if (selfHostedObject->is<BooleanObject>()) {
        clone = BooleanObject::create(cx, selfHostedObject->as<BooleanObject>().unbox());
    } else if (selfHostedObject->is<NumberObject>()) {
        clone = NumberObject::create(cx, selfHostedObject->as<NumberObject>().unbox());
    } else if (selfHostedObject->is<StringObject>()) {
        JSString *selfHostedString = selfHostedObject->as<StringObject>().unbox();
        if (!selfHostedString->isFlat())
            MOZ_CRASH();
        RootedString str(cx, CloneString(cx, &selfHostedString->asFlat()));
        if (!str)
            return nullptr;
        clone = StringObject::create(cx, str);
    } else if (selfHostedObject->is<ArrayObject>()) {
        clone = NewDenseEmptyArray(cx, nullptr, TenuredObject);
    } else {
        JS_ASSERT(selfHostedObject->isNative());
        clone = NewObjectWithGivenProto(cx, selfHostedObject->getClass(), TaggedProto(nullptr), cx->global(),
                                        selfHostedObject->tenuredGetAllocKind(),
                                        SingletonObject);
    }
    if (!clone)
        return nullptr;
    if (!CloneProperties(cx, selfHostedObject, clone))
        return nullptr;
    return clone;
}

static bool
CloneValue(JSContext *cx, HandleValue selfHostedValue, MutableHandleValue vp)
{
    if (selfHostedValue.isObject()) {
        RootedObject selfHostedObject(cx, &selfHostedValue.toObject());
        JSObject *clone = CloneObject(cx, selfHostedObject);
        if (!clone)
            return false;
        vp.setObject(*clone);
    } else if (selfHostedValue.isBoolean() || selfHostedValue.isNumber() || selfHostedValue.isNullOrUndefined()) {
        
        vp.set(selfHostedValue);
    } else if (selfHostedValue.isString()) {
        if (!selfHostedValue.toString()->isFlat())
            MOZ_CRASH();
        JSFlatString *selfHostedString = &selfHostedValue.toString()->asFlat();
        JSString *clone = CloneString(cx, selfHostedString);
        if (!clone)
            return false;
        vp.setString(clone);
    } else if (selfHostedValue.isSymbol()) {
        
        JS::Symbol *sym = selfHostedValue.toSymbol();
        MOZ_ASSERT(sym->isWellKnownSymbol());
        MOZ_ASSERT(cx->wellKnownSymbols().get(size_t(sym->code())) == sym);
        vp.set(selfHostedValue);
    } else {
        MOZ_CRASH("Self-hosting CloneValue can't clone given value.");
    }
    return true;
}

bool
JSRuntime::cloneSelfHostedFunctionScript(JSContext *cx, HandlePropertyName name,
                                         HandleFunction targetFun)
{
    RootedId id(cx, NameToId(name));
    RootedValue funVal(cx);
    if (!GetUnclonedValue(cx, HandleObject::fromMarkedLocation(&selfHostingGlobal_), id, &funVal))
        return false;

    RootedFunction sourceFun(cx, &funVal.toObject().as<JSFunction>());
    
    
    JS_ASSERT(!sourceFun->isGenerator());
    RootedScript sourceScript(cx, sourceFun->getOrCreateScript(cx));
    if (!sourceScript)
        return false;
    JS_ASSERT(!sourceScript->enclosingStaticScope());
    JSScript *cscript = CloneScript(cx, NullPtr(), targetFun, sourceScript);
    if (!cscript)
        return false;
    cscript->setFunction(targetFun);

    JS_ASSERT(sourceFun->nargs() == targetFun->nargs());
    
    targetFun->setFlags((targetFun->flags() & ~JSFunction::INTERPRETED_LAZY) |
                        sourceFun->flags() | JSFunction::EXTENDED);
    targetFun->setScript(cscript);
    JS_ASSERT(targetFun->isExtended());
    return true;
}

bool
JSRuntime::cloneSelfHostedValue(JSContext *cx, HandlePropertyName name, MutableHandleValue vp)
{
    RootedId id(cx, NameToId(name));
    RootedValue selfHostedValue(cx);
    if (!GetUnclonedValue(cx, HandleObject::fromMarkedLocation(&selfHostingGlobal_), id, &selfHostedValue))
        return false;

    




    if (cx->global() == selfHostingGlobal_) {
        vp.set(selfHostedValue);
        return true;
    }

    return CloneValue(cx, selfHostedValue, vp);
}

JSFunction *
js::SelfHostedFunction(JSContext *cx, HandlePropertyName propName)
{
    RootedValue func(cx);
    if (!GlobalObject::getIntrinsicValue(cx, cx->global(), propName, &func))
        return nullptr;

    JS_ASSERT(func.isObject());
    JS_ASSERT(func.toObject().is<JSFunction>());
    return &func.toObject().as<JSFunction>();
}

bool
js::IsSelfHostedFunctionWithName(JSFunction *fun, JSAtom *name)
{
    return fun->isSelfHostedBuiltin() && fun->getExtendedSlot(0).toString() == name;
}
