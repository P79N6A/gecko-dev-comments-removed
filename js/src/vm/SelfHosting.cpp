





#include "vm/SelfHosting.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/Casting.h"
#include "mozilla/DebugOnly.h"

#include "jscntxt.h"
#include "jscompartment.h"
#include "jsdate.h"
#include "jsfriendapi.h"
#include "jshashutil.h"
#include "jsweakmap.h"
#include "jswrapper.h"
#include "selfhosted.out.h"

#include "builtin/Intl.h"
#include "builtin/Object.h"
#include "builtin/SelfHostingDefines.h"
#include "builtin/TypedObject.h"
#include "builtin/WeakSetObject.h"
#include "gc/Marking.h"
#include "vm/Compression.h"
#include "vm/GeneratorObject.h"
#include "vm/Interpreter.h"
#include "vm/String.h"
#include "vm/TypedArrayCommon.h"

#include "jsfuninlines.h"
#include "jsscriptinlines.h"

#include "vm/BooleanObject-inl.h"
#include "vm/NativeObject-inl.h"
#include "vm/NumberObject-inl.h"
#include "vm/StringObject-inl.h"

using namespace js;
using namespace js::selfhosted;

using JS::AutoCheckCannotGC;
using mozilla::IsInRange;
using mozilla::PodMove;
using mozilla::UniquePtr;

static void
selfHosting_ErrorReporter(JSContext* cx, const char* message, JSErrorReport* report)
{
    PrintError(cx, stderr, message, report, true);
}

bool
js::intrinsic_ToObject(JSContext* cx, unsigned argc, Value* vp)
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
js::intrinsic_IsObject(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    Value val = args[0];
    bool isObject = val.isObject();
    args.rval().setBoolean(isObject);
    return true;
}

bool
js::intrinsic_ToInteger(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    double result;
    if (!ToInteger(cx, args[0], &result))
        return false;
    args.rval().setNumber(result);
    return true;
}

bool
js::intrinsic_ToString(JSContext* cx, unsigned argc, Value* vp)
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
intrinsic_ToPropertyKey(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedId id(cx);
    if (!ValueToId<CanGC>(cx, args[0], &id))
        return false;

    args.rval().set(IdToValue(id));
    return true;
}

bool
js::intrinsic_IsCallable(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setBoolean(IsCallable(args[0]));
    return true;
}

static bool
intrinsic_IsConstructor(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setBoolean(IsConstructor(args[0]));
    return true;
}

bool
js::intrinsic_SubstringKernel(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args[0].isString());
    MOZ_ASSERT(args[1].isInt32());
    MOZ_ASSERT(args[2].isInt32());

    RootedString str(cx, args[0].toString());
    int32_t begin = args[1].toInt32();
    int32_t length = args[2].toInt32();

    JSString* substr = SubstringKernel(cx, str, begin, length);
    if (!substr)
        return false;

    args.rval().setString(substr);
    return true;
}

static bool
intrinsic_OwnPropertyKeys(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return GetOwnPropertyKeys(cx, args,
                              JSITER_OWNONLY | JSITER_HIDDEN | JSITER_SYMBOLS);
}

static void
ThrowErrorWithType(JSContext* cx, JSExnType type, const CallArgs& args)
{
    uint32_t errorNumber = args[0].toInt32();

#ifdef DEBUG
    const JSErrorFormatString* efs = GetErrorMessage(nullptr, errorNumber);
    MOZ_ASSERT(efs->argCount == args.length() - 1);
    MOZ_ASSERT(efs->exnType == type, "error-throwing intrinsic and error number are inconsistent");
#endif

    JSAutoByteString errorArgs[3];
    for (unsigned i = 1; i < 4 && i < args.length(); i++) {
        RootedValue val(cx, args[i]);
        if (val.isInt32()) {
            JSString* str = ToString<CanGC>(cx, val);
            if (!str)
                return;
            errorArgs[i - 1].encodeLatin1(cx, str);
        } else if (val.isString()) {
            errorArgs[i - 1].encodeLatin1(cx, val.toString());
        } else {
            UniquePtr<char[], JS::FreePolicy> bytes =
                DecompileValueGenerator(cx, JSDVG_SEARCH_STACK, val, NullPtr());
            if (!bytes)
                return;
            errorArgs[i - 1].initBytes(bytes.release());
        }
        if (!errorArgs[i - 1])
            return;
    }

    JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, errorNumber,
                         errorArgs[0].ptr(), errorArgs[1].ptr(), errorArgs[2].ptr());
}

bool
js::intrinsic_ThrowRangeError(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() >= 1);

    ThrowErrorWithType(cx, JSEXN_RANGEERR, args);
    return false;
}

bool
js::intrinsic_ThrowTypeError(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() >= 1);

    ThrowErrorWithType(cx, JSEXN_TYPEERR, args);
    return false;
}





static bool
intrinsic_AssertionFailed(JSContext* cx, unsigned argc, Value* vp)
{
#ifdef DEBUG
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() > 0) {
        
        JSString* str = ToString<CanGC>(cx, args[0]);
        if (str) {
            fprintf(stderr, "Self-hosted JavaScript assertion info: ");
            str->dumpCharsNoNewline();
            fputc('\n', stderr);
        }
    }
#endif
    MOZ_ASSERT(false);
    return false;
}

static bool
intrinsic_MakeConstructible(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 2);
    MOZ_ASSERT(args[0].isObject());
    MOZ_ASSERT(args[0].toObject().is<JSFunction>());
    MOZ_ASSERT(args[1].isObject());

    
    
    RootedObject ctor(cx, &args[0].toObject());
    if (!DefineProperty(cx, ctor, cx->names().prototype, args[1],
                        nullptr, nullptr,
                        JSPROP_READONLY | JSPROP_ENUMERATE | JSPROP_PERMANENT))
    {
        return false;
    }

    ctor->as<JSFunction>().setIsSelfHostedConstructor();
    args.rval().setUndefined();
    return true;
}









static bool
intrinsic_DecompileArg(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 2);

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





bool
js::intrinsic_NewDenseArray(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    if (!args[0].isInt32()) {
        JS_ReportError(cx, "Expected int32 as second argument");
        return false;
    }
    uint32_t length = args[0].toInt32();

    
    RootedArrayObject buffer(cx, NewDenseFullyAllocatedArray(cx, length));
    if (!buffer)
        return false;

    ObjectGroup* newgroup = ObjectGroup::callingAllocationSiteGroup(cx, JSProto_Array);
    if (!newgroup)
        return false;
    buffer->setGroup(newgroup);

    NativeObject::EnsureDenseResult edr = buffer->ensureDenseElements(cx, length, 0);
    switch (edr) {
      case NativeObject::ED_OK:
        args.rval().setObject(*buffer);
        return true;

      case NativeObject::ED_SPARSE: 
        MOZ_ASSERT(!"%EnsureDenseArrayElements() would yield sparse array");
        JS_ReportError(cx, "%EnsureDenseArrayElements() would yield sparse array");
        break;

      case NativeObject::ED_FAILED:
        break;
    }
    return false;
}













bool
js::intrinsic_UnsafePutElements(JSContext* cx, unsigned argc, Value* vp)
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

        MOZ_ASSERT(args[arri].isObject());
        MOZ_ASSERT(args[arri].toObject().isNative() || IsTypedObjectArray(args[arri].toObject()));
        MOZ_ASSERT(args[idxi].isInt32());

        RootedObject arrobj(cx, &args[arri].toObject());
        uint32_t idx = args[idxi].toInt32();

        if (IsAnyTypedArray(arrobj.get()) || arrobj->is<TypedObject>()) {
            MOZ_ASSERT_IF(IsAnyTypedArray(arrobj.get()), idx < AnyTypedArrayLength(arrobj.get()));
            MOZ_ASSERT_IF(arrobj->is<TypedObject>(), idx < uint32_t(arrobj->as<TypedObject>().length()));
            
            ObjectOpResult ignored;
            RootedValue receiver(cx, ObjectValue(*arrobj));
            if (!SetElement(cx, arrobj, idx, args[elemi], receiver, ignored))
                return false;
        } else {
            MOZ_ASSERT(idx < arrobj->as<ArrayObject>().getDenseInitializedLength());
            arrobj->as<ArrayObject>().setDenseElementWithType(cx, idx, args[elemi]);
        }
    }

    args.rval().setUndefined();
    return true;
}

bool
js::intrinsic_DefineDataProperty(JSContext* cx, unsigned argc, Value* vp)
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

    Rooted<PropertyDescriptor> desc(cx);
    unsigned attrs = 0;

    MOZ_ASSERT(bool(attributes & ATTR_ENUMERABLE) != bool(attributes & ATTR_NONENUMERABLE),
               "_DefineDataProperty must receive either ATTR_ENUMERABLE xor ATTR_NONENUMERABLE");
    if (attributes & ATTR_ENUMERABLE)
        attrs |= JSPROP_ENUMERATE;

    MOZ_ASSERT(bool(attributes & ATTR_CONFIGURABLE) != bool(attributes & ATTR_NONCONFIGURABLE),
               "_DefineDataProperty must receive either ATTR_CONFIGURABLE xor "
               "ATTR_NONCONFIGURABLE");
    if (attributes & ATTR_NONCONFIGURABLE)
        attrs |= JSPROP_PERMANENT;

    MOZ_ASSERT(bool(attributes & ATTR_WRITABLE) != bool(attributes & ATTR_NONWRITABLE),
               "_DefineDataProperty must receive either ATTR_WRITABLE xor ATTR_NONWRITABLE");
    if (attributes & ATTR_NONWRITABLE)
        attrs |= JSPROP_READONLY;

    desc.setDataDescriptor(value, attrs);
    return StandardDefineProperty(cx, obj, id, desc);
}

bool
js::intrinsic_UnsafeSetReservedSlot(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 3);
    MOZ_ASSERT(args[0].isObject());
    MOZ_ASSERT(args[1].isInt32());

    args[0].toObject().as<NativeObject>().setReservedSlot(args[1].toPrivateUint32(), args[2]);
    args.rval().setUndefined();
    return true;
}

bool
js::intrinsic_UnsafeGetReservedSlot(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 2);
    MOZ_ASSERT(args[0].isObject());
    MOZ_ASSERT(args[1].isInt32());

    args.rval().set(args[0].toObject().as<NativeObject>().getReservedSlot(args[1].toPrivateUint32()));
    return true;
}

bool
js::intrinsic_UnsafeGetObjectFromReservedSlot(JSContext* cx, unsigned argc, Value* vp)
{
    if (!intrinsic_UnsafeGetReservedSlot(cx, argc, vp))
        return false;
    MOZ_ASSERT(vp->isObject());
    return true;
}

bool
js::intrinsic_UnsafeGetInt32FromReservedSlot(JSContext* cx, unsigned argc, Value* vp)
{
    if (!intrinsic_UnsafeGetReservedSlot(cx, argc, vp))
        return false;
    MOZ_ASSERT(vp->isInt32());
    return true;
}

bool
js::intrinsic_UnsafeGetStringFromReservedSlot(JSContext* cx, unsigned argc, Value* vp)
{
    if (!intrinsic_UnsafeGetReservedSlot(cx, argc, vp))
        return false;
    MOZ_ASSERT(vp->isString());
    return true;
}

bool
js::intrinsic_UnsafeGetBooleanFromReservedSlot(JSContext* cx, unsigned argc, Value* vp)
{
    if (!intrinsic_UnsafeGetReservedSlot(cx, argc, vp))
        return false;
    MOZ_ASSERT(vp->isBoolean());
    return true;
}

bool
js::intrinsic_IsPackedArray(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());

    JSObject* obj = &args[0].toObject();
    bool isPacked = obj->is<ArrayObject>() && !obj->hasLazyGroup() &&
                    !obj->group()->hasAllFlags(OBJECT_FLAG_NON_PACKED) &&
                    obj->as<ArrayObject>().getDenseInitializedLength() ==
                        obj->as<ArrayObject>().length();

    args.rval().setBoolean(isPacked);
    return true;
}

static bool
intrinsic_GetIteratorPrototype(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 0);

    JSObject* obj = GlobalObject::getOrCreateIteratorPrototype(cx, cx->global());
    if (!obj)
        return false;

    args.rval().setObject(*obj);
    return true;
}

static bool
intrinsic_NewArrayIterator(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 0);

    RootedObject proto(cx, GlobalObject::getOrCreateArrayIteratorPrototype(cx, cx->global()));
    if (!proto)
        return false;

    JSObject* obj = NewObjectWithGivenProto(cx, &ArrayIteratorObject::class_, proto);
    if (!obj)
        return false;

    args.rval().setObject(*obj);
    return true;
}

bool
js::intrinsic_IsArrayIterator(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());

    args.rval().setBoolean(args[0].toObject().is<ArrayIteratorObject>());
    return true;
}

static bool
intrinsic_NewStringIterator(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 0);

    RootedObject proto(cx, GlobalObject::getOrCreateStringIteratorPrototype(cx, cx->global()));
    if (!proto)
        return false;

    JSObject* obj = NewObjectWithGivenProto(cx, &StringIteratorObject::class_, proto);
    if (!obj)
        return false;

    args.rval().setObject(*obj);
    return true;
}

bool
js::intrinsic_IsStringIterator(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());

    args.rval().setBoolean(args[0].toObject().is<StringIteratorObject>());
    return true;
}

static bool
intrinsic_IsStarGeneratorObject(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());

    args.rval().setBoolean(args[0].toObject().is<StarGeneratorObject>());
    return true;
}

static bool
intrinsic_StarGeneratorObjectIsClosed(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());

    StarGeneratorObject* genObj = &args[0].toObject().as<StarGeneratorObject>();
    args.rval().setBoolean(genObj->isClosed());
    return true;
}

bool
js::intrinsic_IsSuspendedStarGenerator(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);

    if (!args[0].isObject() || !args[0].toObject().is<StarGeneratorObject>()) {
        args.rval().setBoolean(false);
        return true;
    }

    StarGeneratorObject& genObj = args[0].toObject().as<StarGeneratorObject>();
    args.rval().setBoolean(!genObj.isClosed() && genObj.isSuspended());
    return true;
}

static bool
intrinsic_IsLegacyGeneratorObject(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());

    args.rval().setBoolean(args[0].toObject().is<LegacyGeneratorObject>());
    return true;
}

static bool
intrinsic_LegacyGeneratorObjectIsClosed(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());

    LegacyGeneratorObject* genObj = &args[0].toObject().as<LegacyGeneratorObject>();
    args.rval().setBoolean(genObj->isClosed());
    return true;
}

static bool
intrinsic_CloseClosingLegacyGeneratorObject(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());

    LegacyGeneratorObject* genObj = &args[0].toObject().as<LegacyGeneratorObject>();
    MOZ_ASSERT(genObj->isClosing());
    genObj->setClosed();
    return true;
}

static bool
intrinsic_ThrowStopIteration(JSContext* cx, unsigned argc, Value* vp)
{
    MOZ_ASSERT(CallArgsFromVp(argc, vp).length() == 0);

    return ThrowStopIteration(cx);
}

static bool
intrinsic_GeneratorIsRunning(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());

    GeneratorObject* genObj = &args[0].toObject().as<GeneratorObject>();
    args.rval().setBoolean(genObj->isRunning() || genObj->isClosing());
    return true;
}

static bool
intrinsic_GeneratorSetClosed(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());

    GeneratorObject* genObj = &args[0].toObject().as<GeneratorObject>();
    genObj->setClosed();
    return true;
}

bool
js::intrinsic_IsArrayBuffer(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());

    args.rval().setBoolean(args[0].toObject().is<ArrayBufferObject>());
    return true;
}

bool
js::intrinsic_IsTypedArray(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());

    args.rval().setBoolean(args[0].toObject().is<TypedArrayObject>());
    return true;
}

bool
js::intrinsic_IsPossiblyWrappedTypedArray(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);

    bool isTypedArray = false;
    if (args[0].isObject()) {
        JSObject* obj = CheckedUnwrap(&args[0].toObject());
        if (!obj) {
            JS_ReportError(cx, "Permission denied to access object");
            return false;
        }

        isTypedArray = obj->is<TypedArrayObject>();
    }

    args.rval().setBoolean(isTypedArray);
    return true;
}

bool
js::intrinsic_TypedArrayBuffer(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(TypedArrayObject::is(args[0]));

    Rooted<TypedArrayObject*> tarray(cx, &args[0].toObject().as<TypedArrayObject>());
    if (!TypedArrayObject::ensureHasBuffer(cx, tarray))
        return false;

    args.rval().set(TypedArrayObject::bufferValue(tarray));
    return true;
}

bool
js::intrinsic_TypedArrayByteOffset(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(TypedArrayObject::is(args[0]));

    args.rval().set(TypedArrayObject::byteOffsetValue(&args[0].toObject().as<TypedArrayObject>()));
    return true;
}

bool
js::intrinsic_TypedArrayElementShift(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(TypedArrayObject::is(args[0]));

    unsigned shift = TypedArrayShift(args[0].toObject().as<TypedArrayObject>().type());
    MOZ_ASSERT(shift == 0 || shift == 1 || shift == 2 || shift == 3);

    args.rval().setInt32(mozilla::AssertedCast<int32_t>(shift));
    return true;
}


bool
js::intrinsic_TypedArrayLength(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);

    RootedObject obj(cx, &args[0].toObject());
    MOZ_ASSERT(obj->is<TypedArrayObject>());
    args.rval().setInt32(obj->as<TypedArrayObject>().length());
    return true;
}

bool
js::intrinsic_MoveTypedArrayElements(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 4);

    Rooted<TypedArrayObject*> tarray(cx, &args[0].toObject().as<TypedArrayObject>());
    uint32_t to = uint32_t(args[1].toInt32());
    uint32_t from = uint32_t(args[2].toInt32());
    uint32_t count = uint32_t(args[3].toInt32());

    MOZ_ASSERT(count > 0,
               "don't call this method if copying no elements, because then "
               "the not-neutered requirement is wrong");

    if (tarray->hasBuffer() && tarray->buffer()->isNeutered()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
        return false;
    }

    
    
    const size_t ElementShift = TypedArrayShift(tarray->type());

    MOZ_ASSERT((UINT32_MAX >> ElementShift) > to);
    uint32_t byteDest = to << ElementShift;

    MOZ_ASSERT((UINT32_MAX >> ElementShift) > from);
    uint32_t byteSrc = from << ElementShift;

    MOZ_ASSERT((UINT32_MAX >> ElementShift) >= count);
    uint32_t byteSize = count << ElementShift;

#ifdef DEBUG
    {
        uint32_t viewByteLength = tarray->byteLength();
        MOZ_ASSERT(byteSize <= viewByteLength);
        MOZ_ASSERT(byteDest < viewByteLength);
        MOZ_ASSERT(byteSrc < viewByteLength);
        MOZ_ASSERT(byteDest <= viewByteLength - byteSize);
        MOZ_ASSERT(byteSrc <= viewByteLength - byteSize);
    }
#endif

    uint8_t* data = static_cast<uint8_t*>(tarray->viewData());
    PodMove(&data[byteDest], &data[byteSrc], byteSize);

    args.rval().setUndefined();
    return true;
}












static TypedArrayObject*
DangerouslyUnwrapTypedArray(JSContext* cx, JSObject* obj)
{
    
    
    JSObject* unwrapped = CheckedUnwrap(obj);
    if (!unwrapped->is<TypedArrayObject>()) {
        
        
        
        
        
        
        
        
        
        
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_DEAD_OBJECT);
        return nullptr;
    }

    
    
    
    return &unwrapped->as<TypedArrayObject>();
}


bool
js::intrinsic_SetFromTypedArrayApproach(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 4);

    Rooted<TypedArrayObject*> target(cx, &args[0].toObject().as<TypedArrayObject>());
    MOZ_ASSERT(!target->hasBuffer() || !target->buffer()->isNeutered(),
               "something should have defended against a neutered target");

    
    
    Rooted<TypedArrayObject*> unsafeTypedArrayCrossCompartment(cx);
    unsafeTypedArrayCrossCompartment = DangerouslyUnwrapTypedArray(cx, &args[1].toObject());
    if (!unsafeTypedArrayCrossCompartment)
        return false;

    double doubleTargetOffset = args[2].toNumber();
    MOZ_ASSERT(doubleTargetOffset >= 0, "caller failed to ensure |targetOffset >= 0|");

    uint32_t targetLength = uint32_t(args[3].toInt32());

    
    
    

    
    if (unsafeTypedArrayCrossCompartment->hasBuffer() &&
        unsafeTypedArrayCrossCompartment->buffer()->isNeutered())
    {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_DETACHED);
        return false;
    }

    
    uint32_t unsafeSrcLengthCrossCompartment = unsafeTypedArrayCrossCompartment->length();
    if (unsafeSrcLengthCrossCompartment + doubleTargetOffset > targetLength) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_BAD_INDEX);
        return false;
    }

    
    uint32_t targetOffset = uint32_t(doubleTargetOffset);

    
    

    Scalar::Type targetType = target->type();
    Scalar::Type unsafeSrcTypeCrossCompartment = unsafeTypedArrayCrossCompartment->type();

    size_t targetElementSize = TypedArrayElemSize(targetType);
    uint8_t* targetData =
        static_cast<uint8_t*>(target->viewData()) + targetOffset * targetElementSize;

    uint8_t* unsafeSrcDataCrossCompartment =
        static_cast<uint8_t*>(unsafeTypedArrayCrossCompartment->viewData());

    uint32_t unsafeSrcElementSizeCrossCompartment =
        TypedArrayElemSize(unsafeSrcTypeCrossCompartment);
    uint32_t unsafeSrcByteLengthCrossCompartment =
        unsafeSrcLengthCrossCompartment * unsafeSrcElementSizeCrossCompartment;

    
    
    
    
    
    
    
    
    
    if (targetType == unsafeSrcTypeCrossCompartment) {
        PodMove(targetData, unsafeSrcDataCrossCompartment, unsafeSrcByteLengthCrossCompartment);
        args.rval().setInt32(JS_SETTYPEDARRAY_SAME_TYPE);
        return true;
    }

    
    
    

    uint8_t* unsafeSrcDataLimitCrossCompartment =
        unsafeSrcDataCrossCompartment + unsafeSrcByteLengthCrossCompartment;
    uint8_t* targetDataLimit =
        static_cast<uint8_t*>(target->viewData()) + targetLength * targetElementSize;

    
    bool overlap =
        IsInRange(targetData, unsafeSrcDataCrossCompartment, unsafeSrcDataLimitCrossCompartment) ||
        IsInRange(unsafeSrcDataCrossCompartment, targetData, targetDataLimit);

    args.rval().setInt32(overlap ? JS_SETTYPEDARRAY_OVERLAPPING : JS_SETTYPEDARRAY_DISJOINT);
    return true;
}

template <typename From, typename To>
static void
CopyValues(To* dest, const From* src, uint32_t count)
{
#ifdef DEBUG
    void* destVoid = static_cast<void*>(dest);
    void* destVoidEnd = static_cast<void*>(dest + count);
    const void* srcVoid = static_cast<const void*>(src);
    const void* srcVoidEnd = static_cast<const void*>(src + count);
    MOZ_ASSERT(!IsInRange(destVoid, srcVoid, srcVoidEnd));
    MOZ_ASSERT(!IsInRange(srcVoid, destVoid, destVoidEnd));
#endif

    for (; count > 0; count--)
        *dest++ = To(*src++);
}

struct DisjointElements
{
    template <typename To>
    static void
    copy(To* dest, const void* src, Scalar::Type fromType, uint32_t count) {
        switch (fromType) {
          case Scalar::Int8:
            CopyValues(dest, static_cast<const int8_t*>(src), count);
            return;

          case Scalar::Uint8:
            CopyValues(dest, static_cast<const uint8_t*>(src), count);
            return;

          case Scalar::Int16:
            CopyValues(dest, static_cast<const int16_t*>(src), count);
            return;

          case Scalar::Uint16:
            CopyValues(dest, static_cast<const uint16_t*>(src), count);
            return;

          case Scalar::Int32:
            CopyValues(dest, static_cast<const int32_t*>(src), count);
            return;

          case Scalar::Uint32:
            CopyValues(dest, static_cast<const uint32_t*>(src), count);
            return;

          case Scalar::Float32:
            CopyValues(dest, static_cast<const float*>(src), count);
            return;

          case Scalar::Float64:
            CopyValues(dest, static_cast<const double*>(src), count);
            return;

          case Scalar::Uint8Clamped:
            CopyValues(dest, static_cast<const uint8_clamped*>(src), count);
            return;

          default:
            MOZ_CRASH("NonoverlappingSet with bogus from-type");
        }
    }
};

static void
CopyToDisjointArray(TypedArrayObject* target, uint32_t targetOffset, const void* src,
                    Scalar::Type srcType, uint32_t count)
{
    Scalar::Type destType = target->type();
    void* dest =
        static_cast<uint8_t*>(target->viewData()) + targetOffset * TypedArrayElemSize(destType);

    switch (destType) {
      case Scalar::Int8: {
        int8_t* dst = reinterpret_cast<int8_t*>(dest);
        DisjointElements::copy(dst, src, srcType, count);
        break;
      }

      case Scalar::Uint8: {
        uint8_t* dst = reinterpret_cast<uint8_t*>(dest);
        DisjointElements::copy(dst, src, srcType, count);
        break;
      }

      case Scalar::Int16: {
        int16_t* dst = reinterpret_cast<int16_t*>(dest);
        DisjointElements::copy(dst, src, srcType, count);
        break;
      }

      case Scalar::Uint16: {
        uint16_t* dst = reinterpret_cast<uint16_t*>(dest);
        DisjointElements::copy(dst, src, srcType, count);
        break;
      }

      case Scalar::Int32: {
        int32_t* dst = reinterpret_cast<int32_t*>(dest);
        DisjointElements::copy(dst, src, srcType, count);
        break;
      }

      case Scalar::Uint32: {
        uint32_t* dst = reinterpret_cast<uint32_t*>(dest);
        DisjointElements::copy(dst, src, srcType, count);
        break;
      }

      case Scalar::Float32: {
        float* dst = reinterpret_cast<float*>(dest);
        DisjointElements::copy(dst, src, srcType, count);
        break;
      }

      case Scalar::Float64: {
        double* dst = reinterpret_cast<double*>(dest);
        DisjointElements::copy(dst, src, srcType, count);
        break;
      }

      case Scalar::Uint8Clamped: {
        uint8_clamped* dst = reinterpret_cast<uint8_clamped*>(dest);
        DisjointElements::copy(dst, src, srcType, count);
        break;
      }

      default:
        MOZ_CRASH("setFromAnyTypedArray with a typed array with bogus type");
    }
}





void
js::SetDisjointTypedElements(TypedArrayObject* target, uint32_t targetOffset,
                             TypedArrayObject* unsafeSrcCrossCompartment)
{
    Scalar::Type unsafeSrcTypeCrossCompartment = unsafeSrcCrossCompartment->type();

    const void* unsafeSrcDataCrossCompartment = unsafeSrcCrossCompartment->viewData();
    uint32_t count = unsafeSrcCrossCompartment->length();

    CopyToDisjointArray(target, targetOffset,
                        unsafeSrcDataCrossCompartment, unsafeSrcTypeCrossCompartment, count);
}

bool
js::intrinsic_SetDisjointTypedElements(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 3);

    Rooted<TypedArrayObject*> target(cx, &args[0].toObject().as<TypedArrayObject>());
    MOZ_ASSERT(!target->hasBuffer() || !target->buffer()->isNeutered(),
               "a neutered typed array has no elements to set, so "
               "it's nonsensical to be setting them");

    uint32_t targetOffset = uint32_t(args[1].toInt32());

    
    
    Rooted<TypedArrayObject*> unsafeSrcCrossCompartment(cx);
    unsafeSrcCrossCompartment = DangerouslyUnwrapTypedArray(cx, &args[2].toObject());
    if (!unsafeSrcCrossCompartment)
        return false;

    SetDisjointTypedElements(target, targetOffset, unsafeSrcCrossCompartment);

    args.rval().setUndefined();
    return true;
}

bool
js::intrinsic_SetOverlappingTypedElements(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 3);

    Rooted<TypedArrayObject*> target(cx, &args[0].toObject().as<TypedArrayObject>());
    MOZ_ASSERT(!target->hasBuffer() || !target->buffer()->isNeutered(),
               "shouldn't be setting elements if neutered");

    uint32_t targetOffset = uint32_t(args[1].toInt32());

    
    
    Rooted<TypedArrayObject*> unsafeSrcCrossCompartment(cx);
    unsafeSrcCrossCompartment = DangerouslyUnwrapTypedArray(cx, &args[2].toObject());
    if (!unsafeSrcCrossCompartment)
        return false;

    
    
    
    
    
    uint32_t count = unsafeSrcCrossCompartment->length();
    Scalar::Type unsafeSrcTypeCrossCompartment = unsafeSrcCrossCompartment->type();
    size_t sourceByteLen = count * TypedArrayElemSize(unsafeSrcTypeCrossCompartment);

    const void* unsafeSrcDataCrossCompartment = unsafeSrcCrossCompartment->viewData();

    auto copyOfSrcData = target->zone()->make_pod_array<uint8_t>(sourceByteLen);
    if (!copyOfSrcData)
        return false;

    mozilla::PodCopy(copyOfSrcData.get(),
                     static_cast<const uint8_t*>(unsafeSrcDataCrossCompartment),
                     sourceByteLen);

    CopyToDisjointArray(target, targetOffset, copyOfSrcData.get(),
                        unsafeSrcTypeCrossCompartment, count);

    args.rval().setUndefined();
    return true;
}

bool
CallSelfHostedNonGenericMethod(JSContext* cx, CallArgs args)
{
    
    
    
    

    MOZ_ASSERT(args.length() > 0);
    RootedPropertyName name(cx, args[args.length() - 1].toString()->asAtom().asPropertyName());

    RootedValue selfHostedFun(cx);
    if (!GlobalObject::getIntrinsicValue(cx, cx->global(), name, &selfHostedFun))
        return false;

    MOZ_ASSERT(selfHostedFun.toObject().is<JSFunction>());

    InvokeArgs args2(cx);
    if (!args2.init(args.length() - 1))
        return false;

    args2.setCallee(selfHostedFun);
    args2.setThis(args.thisv());

    for (size_t i = 0; i < args.length() - 1; i++)
        args2[i].set(args[i]);

    if (!Invoke(cx, args2))
        return false;

    args.rval().set(args2.rval());
    return true;
}

template<typename T>
bool
Is(HandleValue v)
{
    return v.isObject() && v.toObject().is<T>();
}

template<IsAcceptableThis Test>
static bool
CallNonGenericSelfhostedMethod(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<Test, CallSelfHostedNonGenericMethod>(cx, args);
}

static bool
intrinsic_IsWeakSet(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());

    args.rval().setBoolean(args[0].toObject().is<WeakSetObject>());
    return true;
}





static bool
intrinsic_RuntimeDefaultLocale(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    const char* locale = cx->runtime()->getDefaultLocale();
    if (!locale) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_DEFAULT_LOCALE_ERROR);
        return false;
    }

    RootedString jslocale(cx, JS_NewStringCopyZ(cx, locale));
    if (!jslocale)
        return false;

    args.rval().setString(jslocale);
    return true;
}

bool
js::intrinsic_IsConstructing(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 0);

    ScriptFrameIter iter(cx);
    bool isConstructing = iter.isConstructing();
    args.rval().setBoolean(isConstructing);
    return true;
}

static bool
intrinsic_ConstructorForTypedArray(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());
    MOZ_ASSERT(IsAnyTypedArray(&args[0].toObject()));

    RootedObject object(cx, &args[0].toObject());
    JSProtoKey protoKey = StandardProtoKeyOrNull(object);
    MOZ_ASSERT(protoKey);
    RootedValue ctor(cx, cx->global()->getConstructor(protoKey));
    MOZ_ASSERT(ctor.isObject());

    args.rval().set(ctor);
    return true;
}












static const JSFunctionSpec intrinsic_functions[] = {
    JS_FN("std_Array_join",                      array_join,                   1,0),
    JS_FN("std_Array_push",                      array_push,                   1,0),
    JS_FN("std_Array_pop",                       array_pop,                    0,0),
    JS_FN("std_Array_shift",                     array_shift,                  0,0),
    JS_FN("std_Array_unshift",                   array_unshift,                1,0),
    JS_FN("std_Array_slice",                     array_slice,                  2,0),
    JS_FN("std_Array_sort",                      array_sort,                   1,0),

    JS_FN("std_Date_now",                        date_now,                     0,0),
    JS_FN("std_Date_valueOf",                    date_valueOf,                 0,0),

    JS_FN("std_Function_bind",                   fun_bind,                     1,0),
    JS_FN("std_Function_apply",                  fun_apply,                    2,0),

    JS_FN("std_Math_floor",                      math_floor,                   1,0),
    JS_FN("std_Math_max",                        math_max,                     2,0),
    JS_FN("std_Math_min",                        math_min,                     2,0),
    JS_FN("std_Math_abs",                        math_abs,                     1,0),
    JS_FN("std_Math_imul",                       math_imul,                    2,0),
    JS_FN("std_Math_log2",                       math_log2,                    1,0),

    JS_FN("std_Map_has",                         MapObject::has,               1,0),
    JS_FN("std_Map_iterator",                    MapObject::entries,           0,0),

    JS_FN("std_Number_valueOf",                  num_valueOf,                  0,0),

    JS_FN("std_Object_create",                   obj_create,                   2,0),
    JS_FN("std_Object_propertyIsEnumerable",     obj_propertyIsEnumerable,     1,0),
    JS_FN("std_Object_defineProperty",           obj_defineProperty,           3,0),
    JS_FN("std_Object_getPrototypeOf",           obj_getPrototypeOf,           1,0),
    JS_FN("std_Object_getOwnPropertyNames",      obj_getOwnPropertyNames,      1,0),
    JS_FN("std_Object_getOwnPropertyDescriptor", obj_getOwnPropertyDescriptor, 2,0),
    JS_FN("std_Object_hasOwnProperty",           obj_hasOwnProperty,           1,0),

    JS_FN("std_Set_has",                         SetObject::has,               1,0),
    JS_FN("std_Set_iterator",                    SetObject::values,            0,0),

    JS_FN("std_String_fromCharCode",             str_fromCharCode,             1,0),
    JS_FN("std_String_charCodeAt",               str_charCodeAt,               1,0),
    JS_FN("std_String_indexOf",                  str_indexOf,                  1,0),
    JS_FN("std_String_lastIndexOf",              str_lastIndexOf,              1,0),
    JS_FN("std_String_match",                    str_match,                    1,0),
    JS_FN("std_String_replace",                  str_replace,                  2,0),
    JS_FN("std_String_split",                    str_split,                    2,0),
    JS_FN("std_String_startsWith",               str_startsWith,               1,0),
    JS_FN("std_String_toLowerCase",              str_toLowerCase,              0,0),
    JS_FN("std_String_toUpperCase",              str_toUpperCase,              0,0),

    JS_FN("std_WeakMap_has",                     WeakMap_has,                  1,0),
    JS_FN("std_WeakMap_get",                     WeakMap_get,                  2,0),
    JS_FN("std_WeakMap_set",                     WeakMap_set,                  2,0),
    JS_FN("std_WeakMap_delete",                  WeakMap_delete,               1,0),
    JS_FN("std_WeakMap_clear",                   WeakMap_clear,                0,0),

    
    JS_FN("ToObject",                intrinsic_ToObject,                1,0),
    JS_FN("IsObject",                intrinsic_IsObject,                1,0),
    JS_FN("ToInteger",               intrinsic_ToInteger,               1,0),
    JS_FN("ToString",                intrinsic_ToString,                1,0),
    JS_FN("ToPropertyKey",           intrinsic_ToPropertyKey,           1,0),
    JS_FN("IsCallable",              intrinsic_IsCallable,              1,0),
    JS_FN("IsConstructor",           intrinsic_IsConstructor,           1,0),
    JS_FN("OwnPropertyKeys",         intrinsic_OwnPropertyKeys,         1,0),
    JS_FN("ThrowRangeError",         intrinsic_ThrowRangeError,         4,0),
    JS_FN("ThrowTypeError",          intrinsic_ThrowTypeError,          4,0),
    JS_FN("AssertionFailed",         intrinsic_AssertionFailed,         1,0),
    JS_FN("MakeConstructible",       intrinsic_MakeConstructible,       2,0),
    JS_FN("_IsConstructing",         intrinsic_IsConstructing,          0,0),
    JS_FN("_ConstructorForTypedArray", intrinsic_ConstructorForTypedArray, 1,0),
    JS_FN("DecompileArg",            intrinsic_DecompileArg,            2,0),
    JS_FN("RuntimeDefaultLocale",    intrinsic_RuntimeDefaultLocale,    0,0),
    JS_FN("SubstringKernel",         intrinsic_SubstringKernel,         3,0),

    JS_FN("UnsafePutElements",       intrinsic_UnsafePutElements,       3,0),
    JS_FN("_DefineDataProperty",     intrinsic_DefineDataProperty,      4,0),
    JS_FN("UnsafeSetReservedSlot",   intrinsic_UnsafeSetReservedSlot,   3,0),
    JS_FN("UnsafeGetReservedSlot",   intrinsic_UnsafeGetReservedSlot,   2,0),
    JS_FN("UnsafeGetObjectFromReservedSlot",
          intrinsic_UnsafeGetObjectFromReservedSlot, 2, 0),
    JS_FN("UnsafeGetInt32FromReservedSlot",
          intrinsic_UnsafeGetInt32FromReservedSlot, 2, 0),
    JS_FN("UnsafeGetStringFromReservedSlot",
          intrinsic_UnsafeGetStringFromReservedSlot, 2, 0),
    JS_FN("UnsafeGetBooleanFromReservedSlot",
          intrinsic_UnsafeGetBooleanFromReservedSlot, 2, 0),
    JS_FN("IsPackedArray",           intrinsic_IsPackedArray,           1,0),

    JS_FN("GetIteratorPrototype",    intrinsic_GetIteratorPrototype,    0,0),

    JS_FN("NewArrayIterator",        intrinsic_NewArrayIterator,        0,0),
    JS_FN("IsArrayIterator",         intrinsic_IsArrayIterator,         1,0),
    JS_FN("CallArrayIteratorMethodIfWrapped",
          CallNonGenericSelfhostedMethod<Is<ArrayIteratorObject>>,      2,0),


    JS_FN("NewStringIterator",       intrinsic_NewStringIterator,       0,0),
    JS_FN("IsStringIterator",        intrinsic_IsStringIterator,        1,0),
    JS_FN("CallStringIteratorMethodIfWrapped",
          CallNonGenericSelfhostedMethod<Is<StringIteratorObject>>,     2,0),

    JS_FN("IsStarGeneratorObject",   intrinsic_IsStarGeneratorObject,   1,0),
    JS_FN("StarGeneratorObjectIsClosed", intrinsic_StarGeneratorObjectIsClosed, 1,0),
    JS_FN("IsSuspendedStarGenerator",intrinsic_IsSuspendedStarGenerator,1,0),

    JS_FN("IsLegacyGeneratorObject", intrinsic_IsLegacyGeneratorObject, 1,0),
    JS_FN("LegacyGeneratorObjectIsClosed", intrinsic_LegacyGeneratorObjectIsClosed, 1,0),
    JS_FN("CloseClosingLegacyGeneratorObject", intrinsic_CloseClosingLegacyGeneratorObject, 1,0),
    JS_FN("ThrowStopIteration",      intrinsic_ThrowStopIteration,      0,0),

    JS_FN("GeneratorIsRunning",      intrinsic_GeneratorIsRunning,      1,0),
    JS_FN("GeneratorSetClosed",      intrinsic_GeneratorSetClosed,      1,0),

    JS_FN("IsArrayBuffer",           intrinsic_IsArrayBuffer,           1,0),

    JS_FN("IsTypedArray",            intrinsic_IsTypedArray,            1,0),
    JS_FN("IsPossiblyWrappedTypedArray",intrinsic_IsPossiblyWrappedTypedArray,1,0),
    JS_FN("TypedArrayBuffer",        intrinsic_TypedArrayBuffer,        1,0),
    JS_FN("TypedArrayByteOffset",    intrinsic_TypedArrayByteOffset,    1,0),
    JS_FN("TypedArrayElementShift",  intrinsic_TypedArrayElementShift,  1,0),
    JS_FN("TypedArrayLength",        intrinsic_TypedArrayLength,        1,0),

    JS_FN("MoveTypedArrayElements",  intrinsic_MoveTypedArrayElements,  4,0),
    JS_FN("SetFromTypedArrayApproach",intrinsic_SetFromTypedArrayApproach, 4, 0),
    JS_FN("SetDisjointTypedElements",intrinsic_SetDisjointTypedElements,3,0),
    JS_FN("SetOverlappingTypedElements",intrinsic_SetOverlappingTypedElements,3,0),

    JS_FN("CallTypedArrayMethodIfWrapped",
          CallNonGenericSelfhostedMethod<Is<TypedArrayObject>>, 2, 0),

    JS_FN("CallLegacyGeneratorMethodIfWrapped",
          CallNonGenericSelfhostedMethod<Is<LegacyGeneratorObject>>, 2, 0),
    JS_FN("CallStarGeneratorMethodIfWrapped",
          CallNonGenericSelfhostedMethod<Is<StarGeneratorObject>>, 2, 0),

    JS_FN("IsWeakSet",               intrinsic_IsWeakSet,               1,0),

    JS_FN("NewDenseArray",           intrinsic_NewDenseArray,           1,0),

    
    JS_FN("NewOpaqueTypedObject",           js::NewOpaqueTypedObject, 1, 0),
    JS_FN("NewDerivedTypedObject",          js::NewDerivedTypedObject, 3, 0),
    JS_FN("TypedObjectBuffer",              TypedObject::GetBuffer, 1, 0),
    JS_FN("TypedObjectByteOffset",          TypedObject::GetByteOffset, 1, 0),
    JS_FN("AttachTypedObject",              js::AttachTypedObject, 3, 0),
    JS_FN("SetTypedObjectOffset",           js::SetTypedObjectOffset, 2, 0),
    JS_FN("ObjectIsTypeDescr"    ,          js::ObjectIsTypeDescr, 1, 0),
    JS_FN("ObjectIsTypedObject",            js::ObjectIsTypedObject, 1, 0),
    JS_FN("ObjectIsTransparentTypedObject", js::ObjectIsTransparentTypedObject, 1, 0),
    JS_FN("TypedObjectIsAttached",          js::TypedObjectIsAttached, 1, 0),
    JS_FN("TypedObjectTypeDescr",           js::TypedObjectTypeDescr, 1, 0),
    JS_FN("ObjectIsOpaqueTypedObject",      js::ObjectIsOpaqueTypedObject, 1, 0),
    JS_FN("TypeDescrIsArrayType",           js::TypeDescrIsArrayType, 1, 0),
    JS_FN("TypeDescrIsSimpleType",          js::TypeDescrIsSimpleType, 1, 0),
    JS_FN("ClampToUint8",                   js::ClampToUint8, 1, 0),
    JS_FN("GetTypedObjectModule",           js::GetTypedObjectModule, 0, 0),
    JS_FN("GetFloat32x4TypeDescr",          js::GetFloat32x4TypeDescr, 0, 0),
    JS_FN("GetFloat64x2TypeDescr",          js::GetFloat64x2TypeDescr, 0, 0),
    JS_FN("GetInt32x4TypeDescr",            js::GetInt32x4TypeDescr, 0, 0),

#define LOAD_AND_STORE_SCALAR_FN_DECLS(_constant, _type, _name)         \
    JS_FN("Store_" #_name, js::StoreScalar##_type::Func, 3, 0),         \
    JS_FN("Load_" #_name,  js::LoadScalar##_type::Func, 3, 0),
    JS_FOR_EACH_UNIQUE_SCALAR_TYPE_REPR_CTYPE(LOAD_AND_STORE_SCALAR_FN_DECLS)
#undef LOAD_AND_STORE_SCALAR_FN_DECLS

#define LOAD_AND_STORE_REFERENCE_FN_DECLS(_constant, _type, _name)      \
    JS_FN("Store_" #_name, js::StoreReference##_type::Func, 3, 0),      \
    JS_FN("Load_" #_name,  js::LoadReference##_type::Func, 3, 0),
    JS_FOR_EACH_REFERENCE_TYPE_REPR(LOAD_AND_STORE_REFERENCE_FN_DECLS)
#undef LOAD_AND_STORE_REFERENCE_FN_DECLS

    
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
    JS_FN("regexp_construct_no_statics", regexp_construct_no_statics, 2,0),

    JS_FS_END
};

void
js::FillSelfHostingCompileOptions(CompileOptions& options)
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

GlobalObject*
JSRuntime::createSelfHostingGlobal(JSContext* cx)
{
    MOZ_ASSERT(!cx->isExceptionPending());
    MOZ_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));

    JS::CompartmentOptions options;
    options.setDiscardSource(true);
    options.setZone(JS::FreshZone);

    JSCompartment* compartment = NewCompartment(cx, nullptr, nullptr, options);
    if (!compartment)
        return nullptr;

    static const Class shgClass = {
        "self-hosting-global", JSCLASS_GLOBAL_FLAGS,
        nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        JS_GlobalObjectTraceHook
    };

    AutoCompartment ac(cx, compartment);
    Rooted<GlobalObject*> shg(cx, GlobalObject::createInternal(cx, &shgClass));
    if (!shg)
        return nullptr;

    cx->runtime()->selfHostingGlobal_ = shg;
    compartment->isSelfHosting = true;
    compartment->setIsSystem(true);

    if (!GlobalObject::initSelfHostingBuiltins(cx, shg, intrinsic_functions))
        return nullptr;

    JS_FireOnNewGlobalObject(cx, shg);

    return shg;
}

bool
JSRuntime::initSelfHosting(JSContext* cx)
{
    MOZ_ASSERT(!selfHostingGlobal_);

    if (cx->runtime()->parentRuntime) {
        selfHostingGlobal_ = cx->runtime()->parentRuntime->selfHostingGlobal_;
        return true;
    }

    



    JS::AutoDisableGenerationalGC disable(cx->runtime());

    Rooted<GlobalObject*> shg(cx, JSRuntime::createSelfHostingGlobal(cx));
    if (!shg)
        return false;

    JSAutoCompartment ac(cx, shg);

    CompileOptions options(cx);
    FillSelfHostingCompileOptions(options);

    




    JSErrorReporter oldReporter = JS_SetErrorReporter(cx->runtime(), selfHosting_ErrorReporter);
    RootedValue rv(cx);
    bool ok = true;

    char* filename = getenv("MOZ_SELFHOSTEDJS");
    if (filename) {
        RootedScript script(cx);
        if (Compile(cx, options, filename, &script))
            ok = Execute(cx, script, *shg.get(), rv.address());
    } else {
        uint32_t srcLen = GetRawScriptsSize();

        const unsigned char* compressed = compressedSources;
        uint32_t compressedLen = GetCompressedSize();
        ScopedJSFreePtr<char> src(selfHostingGlobal_->zone()->pod_malloc<char>(srcLen));
        if (!src || !DecompressString(compressed, compressedLen,
                                      reinterpret_cast<unsigned char*>(src.get()), srcLen))
        {
            ok = false;
        }

        ok = ok && Evaluate(cx, options, src, srcLen, &rv);
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
JSRuntime::markSelfHostingGlobal(JSTracer* trc)
{
    if (selfHostingGlobal_ && !parentRuntime)
        TraceRoot(trc, &selfHostingGlobal_, "self-hosting global");
}

bool
JSRuntime::isSelfHostingCompartment(JSCompartment* comp)
{
    return selfHostingGlobal_->compartment() == comp;
}

bool
JSRuntime::isSelfHostingZone(JS::Zone* zone)
{
    return selfHostingGlobal_ && selfHostingGlobal_->zoneFromAnyThread() == zone;
}

static bool
CloneValue(JSContext* cx, HandleValue selfHostedValue, MutableHandleValue vp);

static bool
GetUnclonedValue(JSContext* cx, HandleNativeObject selfHostedObject,
                 HandleId id, MutableHandleValue vp)
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
        MOZ_ASSERT(selfHostedObject->is<GlobalObject>());
        RootedValue value(cx, IdToValue(id));
        return ReportValueErrorFlags(cx, JSREPORT_ERROR, JSMSG_NO_SUCH_SELF_HOSTED_PROP,
                                     JSDVG_IGNORE_STACK, value, NullPtr(), nullptr, nullptr);
    }

    RootedShape shape(cx, selfHostedObject->lookupPure(id));
    if (!shape) {
        RootedValue value(cx, IdToValue(id));
        return ReportValueErrorFlags(cx, JSREPORT_ERROR, JSMSG_NO_SUCH_SELF_HOSTED_PROP,
                                     JSDVG_IGNORE_STACK, value, NullPtr(), nullptr, nullptr);
    }

    MOZ_ASSERT(shape->hasSlot() && shape->hasDefaultGetter());
    vp.set(selfHostedObject->getSlot(shape->slot()));
    return true;
}

static bool
CloneProperties(JSContext* cx, HandleNativeObject selfHostedObject, HandleObject clone)
{
    AutoIdVector ids(cx);
    Vector<uint8_t, 16> attrs(cx);

    for (size_t i = 0; i < selfHostedObject->getDenseInitializedLength(); i++) {
        if (!selfHostedObject->getDenseElement(i).isMagic(JS_ELEMENTS_HOLE)) {
            if (!ids.append(INT_TO_JSID(i)))
                return false;
            if (!attrs.append(JSPROP_ENUMERATE))
                return false;
        }
    }

    AutoShapeVector shapes(cx);
    for (Shape::Range<NoGC> range(selfHostedObject->lastProperty()); !range.empty(); range.popFront()) {
        Shape& shape = range.front();
        if (shape.enumerable() && !shapes.append(&shape))
            return false;
    }

    
    Reverse(shapes.begin(), shapes.end());
    for (size_t i = 0; i < shapes.length(); ++i) {
        MOZ_ASSERT(!shapes[i]->isAccessorShape(),
                   "Can't handle cloning accessors here yet.");
        if (!ids.append(shapes[i]->propid()))
            return false;
        uint8_t shapeAttrs =
            shapes[i]->attributes() & (JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY);
        if (!attrs.append(shapeAttrs))
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
            !JS_DefinePropertyById(cx, clone, id, val, attrs[i]))
        {
            return false;
        }
    }

    return true;
}

static JSString*
CloneString(JSContext* cx, JSFlatString* selfHostedString)
{
    size_t len = selfHostedString->length();
    {
        JS::AutoCheckCannotGC nogc;
        JSString* clone;
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

static JSObject*
CloneObject(JSContext* cx, HandleNativeObject selfHostedObject)
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
        
        MOZ_ASSERT(!selfHostedFunction->isArrow());
        js::gc::AllocKind kind = hasName
                                 ? gc::AllocKind::FUNCTION_EXTENDED
                                 : selfHostedFunction->getAllocKind();
        clone = CloneFunctionObject(cx, selfHostedFunction, cx->global(), kind, TenuredObject);
        
        
        if (clone && hasName)
            clone->as<JSFunction>().setExtendedSlot(0, StringValue(selfHostedFunction->atom()));
    } else if (selfHostedObject->is<RegExpObject>()) {
        RegExpObject& reobj = selfHostedObject->as<RegExpObject>();
        RootedAtom source(cx, reobj.getSource());
        MOZ_ASSERT(source->isPermanentAtom());
        clone = RegExpObject::createNoStatics(cx, source, reobj.getFlags(), nullptr, cx->tempLifoAlloc());
    } else if (selfHostedObject->is<DateObject>()) {
        clone = JS_NewDateObjectMsec(cx, selfHostedObject->as<DateObject>().UTCTime().toNumber());
    } else if (selfHostedObject->is<BooleanObject>()) {
        clone = BooleanObject::create(cx, selfHostedObject->as<BooleanObject>().unbox());
    } else if (selfHostedObject->is<NumberObject>()) {
        clone = NumberObject::create(cx, selfHostedObject->as<NumberObject>().unbox());
    } else if (selfHostedObject->is<StringObject>()) {
        JSString* selfHostedString = selfHostedObject->as<StringObject>().unbox();
        if (!selfHostedString->isFlat())
            MOZ_CRASH();
        RootedString str(cx, CloneString(cx, &selfHostedString->asFlat()));
        if (!str)
            return nullptr;
        clone = StringObject::create(cx, str);
    } else if (selfHostedObject->is<ArrayObject>()) {
        clone = NewDenseEmptyArray(cx, NullPtr(), TenuredObject);
    } else {
        MOZ_ASSERT(selfHostedObject->isNative());
        clone = NewObjectWithGivenProto(cx, selfHostedObject->getClass(), NullPtr(),
                                        selfHostedObject->asTenured().getAllocKind(),
                                        SingletonObject);
    }
    if (!clone)
        return nullptr;
    if (!CloneProperties(cx, selfHostedObject, clone))
        return nullptr;
    return clone;
}

static bool
CloneValue(JSContext* cx, HandleValue selfHostedValue, MutableHandleValue vp)
{
    if (selfHostedValue.isObject()) {
        RootedNativeObject selfHostedObject(cx, &selfHostedValue.toObject().as<NativeObject>());
        JSObject* clone = CloneObject(cx, selfHostedObject);
        if (!clone)
            return false;
        vp.setObject(*clone);
    } else if (selfHostedValue.isBoolean() || selfHostedValue.isNumber() || selfHostedValue.isNullOrUndefined()) {
        
        vp.set(selfHostedValue);
    } else if (selfHostedValue.isString()) {
        if (!selfHostedValue.toString()->isFlat())
            MOZ_CRASH();
        JSFlatString* selfHostedString = &selfHostedValue.toString()->asFlat();
        JSString* clone = CloneString(cx, selfHostedString);
        if (!clone)
            return false;
        vp.setString(clone);
    } else if (selfHostedValue.isSymbol()) {
        
        mozilla::DebugOnly<JS::Symbol*> sym = selfHostedValue.toSymbol();
        MOZ_ASSERT(sym->isWellKnownSymbol());
        MOZ_ASSERT(cx->wellKnownSymbols().get(size_t(sym->code())) == sym);
        vp.set(selfHostedValue);
    } else {
        MOZ_CRASH("Self-hosting CloneValue can't clone given value.");
    }
    return true;
}

bool
JSRuntime::cloneSelfHostedFunctionScript(JSContext* cx, HandlePropertyName name,
                                         HandleFunction targetFun)
{
    RootedId id(cx, NameToId(name));
    RootedValue funVal(cx);
    if (!GetUnclonedValue(cx, HandleNativeObject::fromMarkedLocation(&selfHostingGlobal_), id, &funVal))
        return false;

    RootedFunction sourceFun(cx, &funVal.toObject().as<JSFunction>());
    
    
    MOZ_ASSERT(!sourceFun->isGenerator());
    RootedScript sourceScript(cx, sourceFun->getOrCreateScript(cx));
    if (!sourceScript)
        return false;
    MOZ_ASSERT(!sourceScript->enclosingStaticScope());
    JSScript* cscript = CloneScript(cx, NullPtr(), targetFun, sourceScript);
    if (!cscript)
        return false;
    cscript->setFunction(targetFun);

    MOZ_ASSERT(sourceFun->nargs() == targetFun->nargs());
    
    targetFun->setFlags((targetFun->flags() & ~JSFunction::INTERPRETED_LAZY) |
                        sourceFun->flags() | JSFunction::EXTENDED);
    targetFun->setScript(cscript);
    MOZ_ASSERT(targetFun->isExtended());
    return true;
}

bool
JSRuntime::cloneSelfHostedValue(JSContext* cx, HandlePropertyName name, MutableHandleValue vp)
{
    RootedId id(cx, NameToId(name));
    RootedValue selfHostedValue(cx);
    if (!GetUnclonedValue(cx, HandleNativeObject::fromMarkedLocation(&selfHostingGlobal_), id, &selfHostedValue))
        return false;

    




    if (cx->global() == selfHostingGlobal_) {
        vp.set(selfHostedValue);
        return true;
    }

    return CloneValue(cx, selfHostedValue, vp);
}

JSFunction*
js::SelfHostedFunction(JSContext* cx, HandlePropertyName propName)
{
    RootedValue func(cx);
    if (!GlobalObject::getIntrinsicValue(cx, cx->global(), propName, &func))
        return nullptr;

    MOZ_ASSERT(func.isObject());
    MOZ_ASSERT(func.toObject().is<JSFunction>());
    return &func.toObject().as<JSFunction>();
}

bool
js::IsSelfHostedFunctionWithName(JSFunction* fun, JSAtom* name)
{
    return fun->isSelfHostedBuiltin() && fun->getExtendedSlot(0).toString() == name;
}

static_assert(JSString::MAX_LENGTH <= INT32_MAX,
              "StringIteratorNext in builtin/String.js assumes the stored index "
              "into the string is an Int32Value");
