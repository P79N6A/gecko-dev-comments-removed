





#include "jsarray.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/MathAlgorithms.h"

#include <algorithm>

#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsfriendapi.h"
#include "jsfun.h"
#include "jsiter.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jstypes.h"
#include "jsutil.h"

#include "ds/Sort.h"
#include "gc/Heap.h"
#include "vm/ArgumentsObject.h"
#include "vm/ForkJoin.h"
#include "vm/Interpreter.h"
#include "vm/NumericConversions.h"
#include "vm/Shape.h"
#include "vm/StringBuffer.h"

#include "jsatominlines.h"

#include "vm/ArgumentsObject-inl.h"
#include "vm/ArrayObject-inl.h"
#include "vm/Interpreter-inl.h"
#include "vm/Runtime-inl.h"

using namespace js;
using namespace js::gc;
using namespace js::types;

using mozilla::Abs;
using mozilla::ArrayLength;
using mozilla::CeilingLog2;
using mozilla::DebugOnly;
using mozilla::IsNaN;
using mozilla::PointerRangeSize;

using JS::AutoCheckCannotGC;

bool
js::GetLengthProperty(JSContext *cx, HandleObject obj, uint32_t *lengthp)
{
    if (obj->is<ArrayObject>()) {
        *lengthp = obj->as<ArrayObject>().length();
        return true;
    }

    if (obj->is<ArgumentsObject>()) {
        ArgumentsObject &argsobj = obj->as<ArgumentsObject>();
        if (!argsobj.hasOverriddenLength()) {
            *lengthp = argsobj.initialLength();
            return true;
        }
    }

    RootedValue value(cx);
    if (!JSObject::getProperty(cx, obj, obj, cx->names().length, &value))
        return false;

    if (value.isInt32()) {
        *lengthp = uint32_t(value.toInt32()); 
        return true;
    }

    return ToUint32(cx, value, lengthp);
}




















template <typename CharT>
static bool
StringIsArrayIndex(const CharT *s, uint32_t length, uint32_t *indexp)
{
    const CharT *end = s + length;

    if (length == 0 || length > (sizeof("4294967294") - 1) || !JS7_ISDEC(*s))
        return false;

    uint32_t c = 0, previous = 0;
    uint32_t index = JS7_UNDEC(*s++);

    
    if (index == 0 && s != end)
        return false;

    for (; s < end; s++) {
        if (!JS7_ISDEC(*s))
            return false;

        previous = index;
        c = JS7_UNDEC(*s);
        index = 10 * index + c;
    }

    
    if (previous < (MAX_ARRAY_INDEX / 10) || (previous == (MAX_ARRAY_INDEX / 10) &&
        c <= (MAX_ARRAY_INDEX % 10))) {
        JS_ASSERT(index <= MAX_ARRAY_INDEX);
        *indexp = index;
        return true;
    }

    return false;
}

JS_FRIEND_API(bool)
js::StringIsArrayIndex(JSLinearString *str, uint32_t *indexp)
{
    AutoCheckCannotGC nogc;
    return str->hasLatin1Chars()
           ? ::StringIsArrayIndex(str->latin1Chars(nogc), str->length(), indexp)
           : ::StringIsArrayIndex(str->twoByteChars(nogc), str->length(), indexp);
}

static bool
ToId(JSContext *cx, double index, MutableHandleId id)
{
    if (index == uint32_t(index))
        return IndexToId(cx, uint32_t(index), id);

    Value tmp = DoubleValue(index);
    return ValueToId<CanGC>(cx, HandleValue::fromMarkedLocation(&tmp), id);
}

static bool
ToId(JSContext *cx, uint32_t index, MutableHandleId id)
{
    return IndexToId(cx, index, id);
}







template <typename IndexType>
static inline bool
DoGetElement(JSContext *cx, HandleObject obj, HandleObject receiver,
             IndexType index, bool *hole, MutableHandleValue vp)
{
    RootedId id(cx);
    if (!ToId(cx, index, &id))
        return false;

    RootedObject obj2(cx);
    RootedShape prop(cx);
    if (!JSObject::lookupGeneric(cx, obj, id, &obj2, &prop))
        return false;

    if (!prop) {
        vp.setUndefined();
        *hole = true;
    } else {
        if (!JSObject::getGeneric(cx, obj, receiver, id, vp))
            return false;
        *hole = false;
    }
    return true;
}

template <typename IndexType>
static void
AssertGreaterThanZero(IndexType index)
{
    JS_ASSERT(index >= 0);
    JS_ASSERT(index == floor(index));
}

template<>
void
AssertGreaterThanZero(uint32_t index)
{
}

template <typename IndexType>
static bool
GetElement(JSContext *cx, HandleObject obj, HandleObject receiver,
           IndexType index, bool *hole, MutableHandleValue vp)
{
    AssertGreaterThanZero(index);
    if (obj->isNative() && index < obj->getDenseInitializedLength()) {
        vp.set(obj->getDenseElement(uint32_t(index)));
        if (!vp.isMagic(JS_ELEMENTS_HOLE)) {
            *hole = false;
            return true;
        }
    }
    if (obj->is<ArgumentsObject>()) {
        if (obj->as<ArgumentsObject>().maybeGetElement(uint32_t(index), vp)) {
            *hole = false;
            return true;
        }
    }

    return DoGetElement(cx, obj, receiver, index, hole, vp);
}

template <typename IndexType>
static inline bool
GetElement(JSContext *cx, HandleObject obj, IndexType index, bool *hole, MutableHandleValue vp)
{
    return GetElement(cx, obj, obj, index, hole, vp);
}

static bool
GetElementsSlow(JSContext *cx, HandleObject aobj, uint32_t length, Value *vp)
{
    for (uint32_t i = 0; i < length; i++) {
        if (!JSObject::getElement(cx, aobj, aobj, i, MutableHandleValue::fromMarkedLocation(&vp[i])))
            return false;
    }

    return true;
}

bool
js::GetElements(JSContext *cx, HandleObject aobj, uint32_t length, Value *vp)
{
    if (aobj->is<ArrayObject>() && length <= aobj->getDenseInitializedLength() &&
        !ObjectMayHaveExtraIndexedProperties(aobj))
    {
        
        const Value *srcbeg = aobj->getDenseElements();
        const Value *srcend = srcbeg + length;
        const Value *src = srcbeg;
        for (Value *dst = vp; src < srcend; ++dst, ++src)
            *dst = src->isMagic(JS_ELEMENTS_HOLE) ? UndefinedValue() : *src;
        return true;
    }

    if (aobj->is<ArgumentsObject>()) {
        ArgumentsObject &argsobj = aobj->as<ArgumentsObject>();
        if (!argsobj.hasOverriddenLength()) {
            if (argsobj.maybeGetElements(0, length, vp))
                return true;
        }
    }

    return GetElementsSlow(cx, aobj, length, vp);
}




static bool
SetArrayElement(JSContext *cx, HandleObject obj, double index, HandleValue v)
{
    JS_ASSERT(index >= 0);

    if (obj->is<ArrayObject>() && !obj->isIndexed()) {
        Rooted<ArrayObject*> arr(cx, &obj->as<ArrayObject>());
        
        JSObject::EnsureDenseResult result = JSObject::ED_SPARSE;
        do {
            if (index > uint32_t(-1))
                break;
            uint32_t idx = uint32_t(index);
            if (idx >= arr->length() && !arr->lengthIsWritable()) {
                JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, js_GetErrorMessage, nullptr,
                                             JSMSG_CANT_REDEFINE_ARRAY_LENGTH);
                return false;
            }
            result = arr->ensureDenseElements(cx, idx, 1);
            if (result != JSObject::ED_OK)
                break;
            if (idx >= arr->length())
                arr->setLengthInt32(idx + 1);
            arr->setDenseElementWithType(cx, idx, v);
            return true;
        } while (false);

        if (result == JSObject::ED_FAILED)
            return false;
        JS_ASSERT(result == JSObject::ED_SPARSE);
    }

    RootedId id(cx);
    if (!ToId(cx, index, &id))
        return false;

    RootedValue tmp(cx, v);
    return JSObject::setGeneric(cx, obj, obj, id, &tmp, true);
}













static bool
DeleteArrayElement(JSContext *cx, HandleObject obj, double index, bool *succeeded)
{
    JS_ASSERT(index >= 0);
    JS_ASSERT(floor(index) == index);

    if (obj->is<ArrayObject>() && !obj->isIndexed()) {
        if (index <= UINT32_MAX) {
            uint32_t idx = uint32_t(index);
            if (idx < obj->getDenseInitializedLength()) {
                if (!obj->maybeCopyElementsForWrite(cx))
                    return false;
                if (idx+1 == obj->getDenseInitializedLength()) {
                    obj->setDenseInitializedLength(idx);
                } else {
                    obj->markDenseElementsNotPacked(cx);
                    obj->setDenseElement(idx, MagicValue(JS_ELEMENTS_HOLE));
                }
                if (!js_SuppressDeletedElement(cx, obj, idx))
                    return false;
            }
        }

        *succeeded = true;
        return true;
    }

    RootedId id(cx);
    if (!ToId(cx, index, &id))
        return false;
    return JSObject::deleteGeneric(cx, obj, id, succeeded);
}


static bool
DeletePropertyOrThrow(JSContext *cx, HandleObject obj, double index)
{
    bool succeeded;
    if (!DeleteArrayElement(cx, obj, index, &succeeded))
        return false;
    if (succeeded)
        return true;

    RootedId id(cx);
    RootedValue indexv(cx, NumberValue(index));
    if (!ValueToId<CanGC>(cx, indexv, &id))
        return false;
    return obj->reportNotConfigurable(cx, id, JSREPORT_ERROR);
}

bool
js::SetLengthProperty(JSContext *cx, HandleObject obj, double length)
{
    RootedValue v(cx, NumberValue(length));
    return JSObject::setProperty(cx, obj, obj, cx->names().length, &v, true);
}








static bool
array_length_getter(JSContext *cx, HandleObject obj_, HandleId id, MutableHandleValue vp)
{
    RootedObject obj(cx, obj_);
    do {
        if (obj->is<ArrayObject>()) {
            vp.setNumber(obj->as<ArrayObject>().length());
            return true;
        }
        if (!JSObject::getProto(cx, obj, &obj))
            return false;
    } while (obj);
    return true;
}

static bool
array_length_setter(JSContext *cx, HandleObject obj, HandleId id, bool strict, MutableHandleValue vp)
{
    if (!obj->is<ArrayObject>()) {
        return JSObject::defineProperty(cx, obj, cx->names().length, vp,
                                        nullptr, nullptr, JSPROP_ENUMERATE);
    }

    Rooted<ArrayObject*> arr(cx, &obj->as<ArrayObject>());
    MOZ_ASSERT(arr->lengthIsWritable(),
               "setter shouldn't be called if property is non-writable");
    return ArraySetLength<SequentialExecution>(cx, arr, id, JSPROP_PERMANENT, vp, strict);
}

struct ReverseIndexComparator
{
    bool operator()(const uint32_t& a, const uint32_t& b, bool *lessOrEqualp) {
        MOZ_ASSERT(a != b, "how'd we get duplicate indexes?");
        *lessOrEqualp = b <= a;
        return true;
    }
};

template <ExecutionMode mode>
bool
js::CanonicalizeArrayLengthValue(typename ExecutionModeTraits<mode>::ContextType cx,
                                 HandleValue v, uint32_t *newLen)
{
    double d;

    if (mode == ParallelExecution) {
        if (v.isObject())
            return false;

        if (!NonObjectToUint32(cx, v, newLen))
            return false;

        if (!NonObjectToNumber(cx, v, &d))
            return false;
    } else {
        if (!ToUint32(cx->asJSContext(), v, newLen))
            return false;

        if (!ToNumber(cx->asJSContext(), v, &d))
            return false;
    }

    if (d == *newLen)
        return true;

    if (cx->isJSContext())
        JS_ReportErrorNumber(cx->asJSContext(), js_GetErrorMessage, nullptr,
                             JSMSG_BAD_ARRAY_LENGTH);
    return false;
}

template bool
js::CanonicalizeArrayLengthValue<SequentialExecution>(JSContext *cx,
                                                      HandleValue v, uint32_t *newLen);
template bool
js::CanonicalizeArrayLengthValue<ParallelExecution>(ForkJoinContext *cx,
                                                    HandleValue v, uint32_t *newLen);


template <ExecutionMode mode>
bool
js::ArraySetLength(typename ExecutionModeTraits<mode>::ContextType cxArg,
                   Handle<ArrayObject*> arr, HandleId id,
                   unsigned attrs, HandleValue value, bool setterIsStrict)
{
    MOZ_ASSERT(cxArg->isThreadLocal(arr));
    MOZ_ASSERT(id == NameToId(cxArg->names().length));

    if (!arr->maybeCopyElementsForWrite(cxArg))
        return false;

    

    
    uint32_t newLen;
    if (!CanonicalizeArrayLengthValue<mode>(cxArg, value, &newLen))
        return false;

    
    
    
    
    
    
    
    
    
    if (!(attrs & JSPROP_PERMANENT) || (attrs & JSPROP_ENUMERATE)) {
        if (!setterIsStrict)
            return true;
        
        
        if (mode == ParallelExecution)
            return false;
        return Throw(cxArg->asJSContext(), id, JSMSG_CANT_REDEFINE_PROP);
    }

    
    bool lengthIsWritable = arr->lengthIsWritable();
#ifdef DEBUG
    {
        RootedShape lengthShape(cxArg, arr->nativeLookupPure(id));
        MOZ_ASSERT(lengthShape);
        MOZ_ASSERT(lengthShape->writable() == lengthIsWritable);
    }
#endif

    uint32_t oldLen = arr->length();

    
    if (!lengthIsWritable) {
        if (newLen == oldLen)
            return true;

        if (!cxArg->isJSContext())
            return false;

        if (setterIsStrict) {
            return JS_ReportErrorFlagsAndNumber(cxArg->asJSContext(),
                                                JSREPORT_ERROR, js_GetErrorMessage, nullptr,
                                                JSMSG_CANT_REDEFINE_ARRAY_LENGTH);
        }

        return JSObject::reportReadOnly(cxArg->asJSContext(), id,
                                        JSREPORT_STRICT | JSREPORT_WARNING);
    }

    
    bool succeeded = true;
    do {
        
        
        
        if (newLen >= oldLen)
            break;

        
        
        
        
        
        
        
        
        
        if (!arr->isIndexed()) {
            if (!arr->maybeCopyElementsForWrite(cxArg))
                return false;

            uint32_t oldCapacity = arr->getDenseCapacity();
            uint32_t oldInitializedLength = arr->getDenseInitializedLength();
            MOZ_ASSERT(oldCapacity >= oldInitializedLength);
            if (oldInitializedLength > newLen)
                arr->setDenseInitializedLength(newLen);
            if (oldCapacity > newLen)
                arr->shrinkElements(cxArg, newLen);

            
            
            
            break;
        }

        
        
        if (mode == ParallelExecution)
            return false;

        JSContext *cx = cxArg->asJSContext();

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        uint32_t gap = oldLen - newLen;
        const uint32_t RemoveElementsFastLimit = 1 << 24;
        if (gap < RemoveElementsFastLimit) {
            
            
            while (newLen < oldLen) {
                
                oldLen--;

                
                bool deleteSucceeded;
                if (!JSObject::deleteElement(cx, arr, oldLen, &deleteSucceeded))
                    return false;
                if (!deleteSucceeded) {
                    newLen = oldLen + 1;
                    succeeded = false;
                    break;
                }
            }
        } else {
            
            
            
            
            
            
            
            
            
            
            

            Vector<uint32_t> indexes(cx);
            {
                AutoIdVector props(cx);
                if (!GetPropertyNames(cx, arr, JSITER_OWNONLY | JSITER_HIDDEN, &props))
                    return false;

                for (size_t i = 0; i < props.length(); i++) {
                    if (!CheckForInterrupt(cx))
                        return false;

                    uint32_t index;
                    if (!js_IdIsIndex(props[i], &index))
                        continue;

                    if (index >= newLen && index < oldLen) {
                        if (!indexes.append(index))
                            return false;
                    }
                }
            }

            uint32_t count = indexes.length();
            {
                
                
                Vector<uint32_t> scratch(cx);
                if (!scratch.resize(count))
                    return false;
                MOZ_ALWAYS_TRUE(MergeSort(indexes.begin(), count, scratch.begin(),
                                          ReverseIndexComparator()));
            }

            uint32_t index = UINT32_MAX;
            for (uint32_t i = 0; i < count; i++) {
                MOZ_ASSERT(indexes[i] < index, "indexes should never repeat");
                index = indexes[i];

                
                bool deleteSucceeded;
                if (!JSObject::deleteElement(cx, arr, index, &deleteSucceeded))
                    return false;
                if (!deleteSucceeded) {
                    newLen = index + 1;
                    succeeded = false;
                    break;
                }
            }
        }
    } while (false);

    

    
    
    
    
    RootedShape lengthShape(cxArg, mode == ParallelExecution
                            ? arr->nativeLookupPure(id)
                            : arr->nativeLookup(cxArg->asJSContext(), id));
    if (!JSObject::changeProperty<mode>(cxArg, arr, lengthShape, attrs,
                                        JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_SHARED,
                                        array_length_getter, array_length_setter))
    {
        return false;
    }

    if (mode == ParallelExecution) {
        
        if (newLen > INT32_MAX)
            return false;
        arr->setLengthInt32(newLen);
    } else {
        JSContext *cx = cxArg->asJSContext();
        arr->setLength(cx, newLen);
    }


    
    

    
    
    
    ObjectElements *header = arr->getElementsHeader();
    header->initializedLength = Min(header->initializedLength, newLen);

    if (attrs & JSPROP_READONLY) {
        header->setNonwritableArrayLength();

        
        
        
        
        
        
        if (arr->getDenseCapacity() > newLen) {
            arr->shrinkElements(cxArg, newLen);
            arr->getElementsHeader()->capacity = newLen;
        }
    }

    if (setterIsStrict && !succeeded) {
        
        
        JSContext *cx = cxArg->asJSContext();
        RootedId elementId(cx);
        if (!IndexToId(cx, newLen - 1, &elementId))
            return false;
        return arr->reportNotConfigurable(cx, elementId);
    }

    return true;
}

template bool
js::ArraySetLength<SequentialExecution>(JSContext *cx, Handle<ArrayObject*> arr,
                                        HandleId id, unsigned attrs, HandleValue value,
                                        bool setterIsStrict);
template bool
js::ArraySetLength<ParallelExecution>(ForkJoinContext *cx, Handle<ArrayObject*> arr,
                                      HandleId id, unsigned attrs, HandleValue value,
                                      bool setterIsStrict);

bool
js::WouldDefinePastNonwritableLength(ThreadSafeContext *cx,
                                     HandleObject obj, uint32_t index, bool strict,
                                     bool *definesPast)
{
    if (!obj->is<ArrayObject>()) {
        *definesPast = false;
        return true;
    }

    Rooted<ArrayObject*> arr(cx, &obj->as<ArrayObject>());
    uint32_t length = arr->length();
    if (index < length) {
        *definesPast = false;
        return true;
    }

    if (arr->lengthIsWritable()) {
        *definesPast = false;
        return true;
    }

    *definesPast = true;

    
    unsigned flags = strict ? JSREPORT_ERROR : (JSREPORT_STRICT | JSREPORT_WARNING);
    if (cx->isForkJoinContext())
        return cx->asForkJoinContext()->reportError(flags);

    if (!cx->isJSContext())
        return true;

    JSContext *ncx = cx->asJSContext();

    if (!strict && !ncx->compartment()->options().extraWarnings(ncx))
        return true;

    
    return JS_ReportErrorFlagsAndNumber(ncx, flags, js_GetErrorMessage, nullptr,
                                        JSMSG_CANT_DEFINE_PAST_ARRAY_LENGTH);
}

static bool
array_addProperty(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue vp)
{
    Rooted<ArrayObject*> arr(cx, &obj->as<ArrayObject>());

    uint32_t index;
    if (!js_IdIsIndex(id, &index))
        return true;

    uint32_t length = arr->length();
    if (index >= length) {
        MOZ_ASSERT(arr->lengthIsWritable(),
                   "how'd this element get added if length is non-writable?");
        arr->setLength(cx, index + 1);
    }
    return true;
}

bool
js::ObjectMayHaveExtraIndexedProperties(JSObject *obj)
{
    





    JS_ASSERT(obj->isNative());

    if (obj->isIndexed())
        return true;

    




    while ((obj = obj->getProto()) != nullptr) {
        




        if (!obj->isNative())
            return true;
        if (obj->isIndexed())
            return true;
        if (obj->getDenseInitializedLength() > 0)
            return true;
        if (obj->is<TypedArrayObject>())
            return true;
    }

    return false;
}

static bool
AddLengthProperty(ExclusiveContext *cx, HandleObject obj)
{
    






    RootedId lengthId(cx, NameToId(cx->names().length));
    JS_ASSERT(!obj->nativeLookup(cx, lengthId));

    return JSObject::addProperty(cx, obj, lengthId, array_length_getter, array_length_setter,
                                 SHAPE_INVALID_SLOT, JSPROP_PERMANENT | JSPROP_SHARED, 0,
                                  false);
}

#if JS_HAS_TOSOURCE

static bool
array_toSource(JSContext *cx, unsigned argc, Value *vp)
{
    JS_CHECK_RECURSION(cx, return false);
    CallArgs args = CallArgsFromVp(argc, vp);

    if (!args.thisv().isObject()) {
        ReportIncompatible(cx, args);
        return false;
    }

    Rooted<JSObject*> obj(cx, &args.thisv().toObject());
    RootedValue elt(cx);

    AutoCycleDetector detector(cx, obj);
    if (!detector.init())
        return false;

    StringBuffer sb(cx);

    if (detector.foundCycle()) {
        if (!sb.append("[]"))
            return false;
        goto make_string;
    }

    if (!sb.append('['))
        return false;

    uint32_t length;
    if (!GetLengthProperty(cx, obj, &length))
        return false;

    for (uint32_t index = 0; index < length; index++) {
        bool hole;
        if (!CheckForInterrupt(cx) ||
            !GetElement(cx, obj, index, &hole, &elt)) {
            return false;
        }

        
        JSString *str;
        if (hole) {
            str = cx->runtime()->emptyString;
        } else {
            str = ValueToSource(cx, elt);
            if (!str)
                return false;
        }

        
        if (!sb.append(str))
            return false;
        if (index + 1 != length) {
            if (!sb.append(", "))
                return false;
        } else if (hole) {
            if (!sb.append(','))
                return false;
        }
    }

    
    if (!sb.append(']'))
        return false;

  make_string:
    JSString *str = sb.finishString();
    if (!str)
        return false;

    args.rval().setString(str);
    return true;
}

#endif

struct EmptySeparatorOp
{
    bool operator()(JSContext *, StringBuffer &sb) { return true; }
};

template <typename CharT>
struct CharSeparatorOp
{
    const CharT sep;
    explicit CharSeparatorOp(CharT sep) : sep(sep) {}
    bool operator()(JSContext *, StringBuffer &sb) { return sb.append(sep); }
};

struct StringSeparatorOp
{
    HandleLinearString sep;

    explicit StringSeparatorOp(HandleLinearString sep) : sep(sep) {}

    bool operator()(JSContext *cx, StringBuffer &sb) {
        return sb.append(sep);
    }
};

template <bool Locale, typename SeparatorOp>
static bool
ArrayJoinKernel(JSContext *cx, SeparatorOp sepOp, HandleObject obj, uint32_t length,
               StringBuffer &sb)
{
    uint32_t i = 0;

    if (!Locale && obj->is<ArrayObject>() && !ObjectMayHaveExtraIndexedProperties(obj)) {
        
        
        
        uint32_t initLength = obj->getDenseInitializedLength();
        while (i < initLength) {
            if (!CheckForInterrupt(cx))
                return false;

            const Value &elem = obj->getDenseElement(i);

            if (elem.isString()) {
                if (!sb.append(elem.toString()))
                    return false;
            } else if (elem.isNumber()) {
                if (!NumberValueToStringBuffer(cx, elem, sb))
                    return false;
            } else if (elem.isBoolean()) {
                if (!BooleanToStringBuffer(elem.toBoolean(), sb))
                    return false;
            } else if (elem.isObject() || elem.isSymbol()) {
                







                break;
            } else {
                JS_ASSERT(elem.isMagic(JS_ELEMENTS_HOLE) || elem.isNullOrUndefined());
            }

            if (++i != length && !sepOp(cx, sb))
                return false;
        }
    }

    if (i != length) {
        RootedValue v(cx);
        while (i < length) {
            if (!CheckForInterrupt(cx))
                return false;

            bool hole;
            if (!GetElement(cx, obj, i, &hole, &v))
                return false;
            if (!hole && !v.isNullOrUndefined()) {
                if (Locale) {
                    JSObject *robj = ToObject(cx, v);
                    if (!robj)
                        return false;
                    RootedId id(cx, NameToId(cx->names().toLocaleString));
                    if (!robj->callMethod(cx, id, 0, nullptr, &v))
                        return false;
                }
                if (!ValueToStringBuffer(cx, v, sb))
                    return false;
            }

            if (++i != length && !sepOp(cx, sb))
                return false;
        }
    }

    return true;
}

template <bool Locale>
JSString *
js::ArrayJoin(JSContext *cx, HandleObject obj, HandleLinearString sepstr, uint32_t length)
{
    
    
    

    

    JS::Anchor<JSString*> anchor(sepstr);

    

    
    
    
    if (length == 1 && !Locale && obj->is<ArrayObject>() &&
        obj->getDenseInitializedLength() == 1)
    {
        const Value &elem0 = obj->getDenseElement(0);
        if (elem0.isString()) {
            return elem0.toString();
        }
    }

    StringBuffer sb(cx);
    if (sepstr->hasTwoByteChars() && !sb.ensureTwoByteChars())
        return nullptr;

    
    
    size_t seplen = sepstr->length();
    if (length > 0 && !sb.reserve(seplen * (length - 1)))
        return nullptr;

    
    if (seplen == 0) {
        EmptySeparatorOp op;
        if (!ArrayJoinKernel<Locale>(cx, op, obj, length, sb))
            return nullptr;
    } else if (seplen == 1) {
        char16_t c = sepstr->latin1OrTwoByteChar(0);
        if (c <= JSString::MAX_LATIN1_CHAR) {
            CharSeparatorOp<Latin1Char> op(c);
            if (!ArrayJoinKernel<Locale>(cx, op, obj, length, sb))
                return nullptr;
        } else {
            CharSeparatorOp<char16_t> op(c);
            if (!ArrayJoinKernel<Locale>(cx, op, obj, length, sb))
                return nullptr;
        }
    } else {
        StringSeparatorOp op(sepstr);
        if (!ArrayJoinKernel<Locale>(cx, op, obj, length, sb))
            return nullptr;
    }

    
    JSString *str = sb.finishString();
    if (!str)
        return nullptr;
    return str;
}

template <bool Locale>
bool
ArrayJoin(JSContext *cx, CallArgs &args)
{
    
    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    AutoCycleDetector detector(cx, obj);
    if (!detector.init())
        return false;

    if (detector.foundCycle()) {
        args.rval().setString(cx->names().empty);
        return true;
    }

    
    uint32_t length;
    if (!GetLengthProperty(cx, obj, &length))
        return false;

    
    RootedLinearString sepstr(cx);
    if (!Locale && args.hasDefined(0)) {
        JSString *s = ToString<CanGC>(cx, args[0]);
        if (!s)
            return false;
        sepstr = s->ensureLinear(cx);
        if (!sepstr)
            return false;
    } else {
        sepstr = cx->names().comma;
    }

    
    JSString *res = js::ArrayJoin<Locale>(cx, obj, sepstr, length);
    if (!res)
        return false;

    args.rval().setString(res);
    return true;
}


static bool
array_toString(JSContext *cx, unsigned argc, Value *vp)
{
    JS_CHECK_RECURSION(cx, return false);

    CallArgs args = CallArgsFromVp(argc, vp);
    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    RootedValue join(cx, args.calleev());
    if (!JSObject::getProperty(cx, obj, obj, cx->names().join, &join))
        return false;

    if (!IsCallable(join)) {
        JSString *str = JS_BasicObjectToString(cx, obj);
        if (!str)
            return false;
        args.rval().setString(str);
        return true;
    }

    InvokeArgs args2(cx);
    if (!args2.init(0))
        return false;

    args2.setCallee(join);
    args2.setThis(ObjectValue(*obj));

    
    if (!Invoke(cx, args2))
        return false;
    args.rval().set(args2.rval());
    return true;
}


static bool
array_toLocaleString(JSContext *cx, unsigned argc, Value *vp)
{
    JS_CHECK_RECURSION(cx, return false);

    CallArgs args = CallArgsFromVp(argc, vp);
    return ArrayJoin<true>(cx, args);
}


bool
js::array_join(JSContext *cx, unsigned argc, Value *vp)
{
    JS_CHECK_RECURSION(cx, return false);

    CallArgs args = CallArgsFromVp(argc, vp);
    return ArrayJoin<false>(cx, args);
}

static inline bool
InitArrayTypes(JSContext *cx, TypeObject *type, const Value *vector, unsigned count)
{
    if (!type->unknownProperties()) {
        AutoEnterAnalysis enter(cx);

        HeapTypeSet *types = type->getProperty(cx, JSID_VOID);
        if (!types)
            return false;

        for (unsigned i = 0; i < count; i++) {
            if (vector[i].isMagic(JS_ELEMENTS_HOLE))
                continue;
            Type valtype = GetValueType(vector[i]);
            types->addType(cx, valtype);
        }
    }
    return true;
}

enum ShouldUpdateTypes
{
    UpdateTypes = true,
    DontUpdateTypes = false
};


static bool
InitArrayElements(JSContext *cx, HandleObject obj, uint32_t start, uint32_t count, const Value *vector, ShouldUpdateTypes updateTypes)
{
    JS_ASSERT(count <= MAX_ARRAY_INDEX);

    if (count == 0)
        return true;

    types::TypeObject *type = obj->getType(cx);
    if (!type)
        return false;
    if (updateTypes && !InitArrayTypes(cx, type, vector, count))
        return false;

    




    do {
        if (!obj->is<ArrayObject>())
            break;
        if (ObjectMayHaveExtraIndexedProperties(obj))
            break;

        if (obj->shouldConvertDoubleElements())
            break;

        Rooted<ArrayObject*> arr(cx, &obj->as<ArrayObject>());

        if (!arr->lengthIsWritable() && start + count > arr->length())
            break;

        JSObject::EnsureDenseResult result = arr->ensureDenseElements(cx, start, count);
        if (result != JSObject::ED_OK) {
            if (result == JSObject::ED_FAILED)
                return false;
            JS_ASSERT(result == JSObject::ED_SPARSE);
            break;
        }

        uint32_t newlen = start + count;
        if (newlen > arr->length())
            arr->setLengthInt32(newlen);

        JS_ASSERT(count < UINT32_MAX / sizeof(Value));
        arr->copyDenseElements(start, vector, count);
        JS_ASSERT_IF(count != 0, !arr->getDenseElement(newlen - 1).isMagic(JS_ELEMENTS_HOLE));
        return true;
    } while (false);

    const Value* end = vector + count;
    while (vector < end && start <= MAX_ARRAY_INDEX) {
        if (!CheckForInterrupt(cx) ||
            !SetArrayElement(cx, obj, start++, HandleValue::fromMarkedLocation(vector++))) {
            return false;
        }
    }

    if (vector == end)
        return true;

    JS_ASSERT(start == MAX_ARRAY_INDEX + 1);
    RootedValue value(cx);
    RootedId id(cx);
    RootedValue indexv(cx);
    double index = MAX_ARRAY_INDEX + 1;
    do {
        value = *vector++;
        indexv = DoubleValue(index);
        if (!ValueToId<CanGC>(cx, indexv, &id) ||
            !JSObject::setGeneric(cx, obj, obj, id, &value, true))
        {
            return false;
        }
        index += 1;
    } while (vector != end);

    return true;
}

static bool
array_reverse(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    uint32_t len;
    if (!GetLengthProperty(cx, obj, &len))
        return false;

    do {
        if (!obj->is<ArrayObject>())
            break;
        if (ObjectMayHaveExtraIndexedProperties(obj))
            break;

        
        if (len == 0 || obj->getDenseCapacity() == 0) {
            args.rval().setObject(*obj);
            return true;
        }

        








        JSObject::EnsureDenseResult result = obj->ensureDenseElements(cx, len, 0);
        if (result != JSObject::ED_OK) {
            if (result == JSObject::ED_FAILED)
                return false;
            JS_ASSERT(result == JSObject::ED_SPARSE);
            break;
        }

        
        obj->ensureDenseInitializedLength(cx, len, 0);

        RootedValue origlo(cx), orighi(cx);

        uint32_t lo = 0, hi = len - 1;
        for (; lo < hi; lo++, hi--) {
            origlo = obj->getDenseElement(lo);
            orighi = obj->getDenseElement(hi);
            obj->setDenseElement(lo, orighi);
            if (orighi.isMagic(JS_ELEMENTS_HOLE) &&
                !js_SuppressDeletedProperty(cx, obj, INT_TO_JSID(lo))) {
                return false;
            }
            obj->setDenseElement(hi, origlo);
            if (origlo.isMagic(JS_ELEMENTS_HOLE) &&
                !js_SuppressDeletedProperty(cx, obj, INT_TO_JSID(hi))) {
                return false;
            }
        }

        




        args.rval().setObject(*obj);
        return true;
    } while (false);

    RootedValue lowval(cx), hival(cx);
    for (uint32_t i = 0, half = len / 2; i < half; i++) {
        bool hole, hole2;
        if (!CheckForInterrupt(cx) ||
            !GetElement(cx, obj, i, &hole, &lowval) ||
            !GetElement(cx, obj, len - i - 1, &hole2, &hival))
        {
            return false;
        }

        if (!hole && !hole2) {
            if (!SetArrayElement(cx, obj, i, hival))
                return false;
            if (!SetArrayElement(cx, obj, len - i - 1, lowval))
                return false;
        } else if (hole && !hole2) {
            if (!SetArrayElement(cx, obj, i, hival))
                return false;
            if (!DeletePropertyOrThrow(cx, obj, len - i - 1))
                return false;
        } else if (!hole && hole2) {
            if (!DeletePropertyOrThrow(cx, obj, i))
                return false;
            if (!SetArrayElement(cx, obj, len - i - 1, lowval))
                return false;
        } else {
            
        }
    }
    args.rval().setObject(*obj);
    return true;
}

static inline bool
CompareStringValues(JSContext *cx, const Value &a, const Value &b, bool *lessOrEqualp)
{
    if (!CheckForInterrupt(cx))
        return false;

    JSString *astr = a.toString();
    JSString *bstr = b.toString();
    int32_t result;
    if (!CompareStrings(cx, astr, bstr, &result))
        return false;

    *lessOrEqualp = (result <= 0);
    return true;
}

static const uint64_t powersOf10[] = {
    1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000, 1000000000000ULL
};

static inline unsigned
NumDigitsBase10(uint32_t n)
{
    




    uint32_t log2 = CeilingLog2(n);
    uint32_t t = log2 * 1233 >> 12;
    return t - (n < powersOf10[t]) + 1;
}

static inline bool
CompareLexicographicInt32(const Value &a, const Value &b, bool *lessOrEqualp)
{
    int32_t aint = a.toInt32();
    int32_t bint = b.toInt32();

    






    if (aint == bint) {
        *lessOrEqualp = true;
    } else if ((aint < 0) && (bint >= 0)) {
        *lessOrEqualp = true;
    } else if ((aint >= 0) && (bint < 0)) {
        *lessOrEqualp = false;
    } else {
        uint32_t auint = Abs(aint);
        uint32_t buint = Abs(bint);

        





        unsigned digitsa = NumDigitsBase10(auint);
        unsigned digitsb = NumDigitsBase10(buint);
        if (digitsa == digitsb) {
            *lessOrEqualp = (auint <= buint);
        } else if (digitsa > digitsb) {
            JS_ASSERT((digitsa - digitsb) < ArrayLength(powersOf10));
            *lessOrEqualp = (uint64_t(auint) < uint64_t(buint) * powersOf10[digitsa - digitsb]);
        } else { 
            JS_ASSERT((digitsb - digitsa) < ArrayLength(powersOf10));
            *lessOrEqualp = (uint64_t(auint) * powersOf10[digitsb - digitsa] <= uint64_t(buint));
        }
    }

    return true;
}

template <typename Char1, typename Char2>
static inline bool
CompareSubStringValues(JSContext *cx, const Char1 *s1, size_t len1, const Char2 *s2, size_t len2,
                       bool *lessOrEqualp)
{
    if (!CheckForInterrupt(cx))
        return false;

    if (!s1 || !s2)
        return false;

    int32_t result = CompareChars(s1, len1, s2, len2);
    *lessOrEqualp = (result <= 0);
    return true;
}

namespace {

struct SortComparatorStrings
{
    JSContext   *const cx;

    explicit SortComparatorStrings(JSContext *cx)
      : cx(cx) {}

    bool operator()(const Value &a, const Value &b, bool *lessOrEqualp) {
        return CompareStringValues(cx, a, b, lessOrEqualp);
    }
};

struct SortComparatorLexicographicInt32
{
    bool operator()(const Value &a, const Value &b, bool *lessOrEqualp) {
        return CompareLexicographicInt32(a, b, lessOrEqualp);
    }
};

struct StringifiedElement
{
    size_t charsBegin;
    size_t charsEnd;
    size_t elementIndex;
};

struct SortComparatorStringifiedElements
{
    JSContext           *const cx;
    const StringBuffer  &sb;

    SortComparatorStringifiedElements(JSContext *cx, const StringBuffer &sb)
      : cx(cx), sb(sb) {}

    bool operator()(const StringifiedElement &a, const StringifiedElement &b, bool *lessOrEqualp) {
        size_t lenA = a.charsEnd - a.charsBegin;
        size_t lenB = b.charsEnd - b.charsBegin;

        if (sb.isUnderlyingBufferLatin1()) {
            return CompareSubStringValues(cx, sb.rawLatin1Begin() + a.charsBegin, lenA,
                                          sb.rawLatin1Begin() + b.charsBegin, lenB,
                                          lessOrEqualp);
        }

        return CompareSubStringValues(cx, sb.rawTwoByteBegin() + a.charsBegin, lenA,
                                      sb.rawTwoByteBegin() + b.charsBegin, lenB,
                                      lessOrEqualp);
    }
};

struct SortComparatorFunction
{
    JSContext          *const cx;
    const Value        &fval;
    FastInvokeGuard    &fig;

    SortComparatorFunction(JSContext *cx, const Value &fval, FastInvokeGuard &fig)
      : cx(cx), fval(fval), fig(fig) { }

    bool operator()(const Value &a, const Value &b, bool *lessOrEqualp);
};

bool
SortComparatorFunction::operator()(const Value &a, const Value &b, bool *lessOrEqualp)
{
    



    JS_ASSERT(!a.isMagic() && !a.isUndefined());
    JS_ASSERT(!a.isMagic() && !b.isUndefined());

    if (!CheckForInterrupt(cx))
        return false;

    InvokeArgs &args = fig.args();
    if (!args.init(2))
        return false;

    args.setCallee(fval);
    args.setThis(UndefinedValue());
    args[0].set(a);
    args[1].set(b);

    if (!fig.invoke(cx))
        return false;

    double cmp;
    if (!ToNumber(cx, args.rval(), &cmp))
        return false;

    




    *lessOrEqualp = (IsNaN(cmp) || cmp <= 0);
    return true;
}

struct NumericElement
{
    double dv;
    size_t elementIndex;
};

static bool
ComparatorNumericLeftMinusRight(const NumericElement &a, const NumericElement &b,
                                bool *lessOrEqualp)
{
    *lessOrEqualp = (a.dv <= b.dv);
    return true;
}

static bool
ComparatorNumericRightMinusLeft(const NumericElement &a, const NumericElement &b,
                                bool *lessOrEqualp)
{
    *lessOrEqualp = (b.dv <= a.dv);
    return true;
}

typedef bool (*ComparatorNumeric)(const NumericElement &a, const NumericElement &b,
                                  bool *lessOrEqualp);

static const ComparatorNumeric SortComparatorNumerics[] = {
    nullptr,
    nullptr,
    ComparatorNumericLeftMinusRight,
    ComparatorNumericRightMinusLeft
};

static bool
ComparatorInt32LeftMinusRight(const Value &a, const Value &b, bool *lessOrEqualp)
{
    *lessOrEqualp = (a.toInt32() <= b.toInt32());
    return true;
}

static bool
ComparatorInt32RightMinusLeft(const Value &a, const Value &b, bool *lessOrEqualp)
{
    *lessOrEqualp = (b.toInt32() <= a.toInt32());
    return true;
}

typedef bool (*ComparatorInt32)(const Value &a, const Value &b, bool *lessOrEqualp);

static const ComparatorInt32 SortComparatorInt32s[] = {
    nullptr,
    nullptr,
    ComparatorInt32LeftMinusRight,
    ComparatorInt32RightMinusLeft
};



enum ComparatorMatchResult {
    Match_Failure = 0,
    Match_None,
    Match_LeftMinusRight,
    Match_RightMinusLeft
};

} 





static ComparatorMatchResult
MatchNumericComparator(JSContext *cx, const Value &v)
{
    if (!v.isObject())
        return Match_None;

    JSObject &obj = v.toObject();
    if (!obj.is<JSFunction>())
        return Match_None;

    JSFunction *fun = &obj.as<JSFunction>();
    if (!fun->isInterpreted())
        return Match_None;

    JSScript *script = fun->getOrCreateScript(cx);
    if (!script)
        return Match_Failure;

    jsbytecode *pc = script->code();

    uint16_t arg0, arg1;
    if (JSOp(*pc) != JSOP_GETARG)
        return Match_None;
    arg0 = GET_ARGNO(pc);
    pc += JSOP_GETARG_LENGTH;

    if (JSOp(*pc) != JSOP_GETARG)
        return Match_None;
    arg1 = GET_ARGNO(pc);
    pc += JSOP_GETARG_LENGTH;

    if (JSOp(*pc) != JSOP_SUB)
        return Match_None;
    pc += JSOP_SUB_LENGTH;

    if (JSOp(*pc) != JSOP_RETURN)
        return Match_None;

    if (arg0 == 0 && arg1 == 1)
        return Match_LeftMinusRight;

    if (arg0 == 1 && arg1 == 0)
        return Match_RightMinusLeft;

    return Match_None;
}

template <typename K, typename C>
static inline bool
MergeSortByKey(K keys, size_t len, K scratch, C comparator, AutoValueVector *vec)
{
    MOZ_ASSERT(vec->length() >= len);

    
    if (!MergeSort(keys, len, scratch, comparator))
        return false;

    











    for (size_t i = 0; i < len; i++) {
        size_t j = keys[i].elementIndex;
        if (i == j)
            continue; 

        MOZ_ASSERT(j > i, "Everything less than |i| should be in the right place!");
        Value tv = (*vec)[j];
        do {
            size_t k = keys[j].elementIndex;
            keys[j].elementIndex = j;
            (*vec)[j].set((*vec)[k]);
            j = k;
        } while (j != i);

        
        
        
        (*vec)[i].set(tv);
    }

    return true;
}







static bool
SortLexicographically(JSContext *cx, AutoValueVector *vec, size_t len)
{
    JS_ASSERT(vec->length() >= len);

    StringBuffer sb(cx);
    Vector<StringifiedElement, 0, TempAllocPolicy> strElements(cx);

    
    if (!strElements.reserve(2 * len))
        return false;

    
    size_t cursor = 0;
    for (size_t i = 0; i < len; i++) {
        if (!CheckForInterrupt(cx))
            return false;

        if (!ValueToStringBuffer(cx, (*vec)[i], sb))
            return false;

        StringifiedElement el = { cursor, sb.length(), i };
        strElements.infallibleAppend(el);
        cursor = sb.length();
    }

    
    JS_ALWAYS_TRUE(strElements.resize(2 * len));

    
    return MergeSortByKey(strElements.begin(), len, strElements.begin() + len,
                          SortComparatorStringifiedElements(cx, sb), vec);
}







static bool
SortNumerically(JSContext *cx, AutoValueVector *vec, size_t len, ComparatorMatchResult comp)
{
    JS_ASSERT(vec->length() >= len);

    Vector<NumericElement, 0, TempAllocPolicy> numElements(cx);

    
    if (!numElements.reserve(2 * len))
        return false;

    
    for (size_t i = 0; i < len; i++) {
        if (!CheckForInterrupt(cx))
            return false;

        double dv;
        if (!ToNumber(cx, (*vec)[i], &dv))
            return false;

        NumericElement el = { dv, i };
        numElements.infallibleAppend(el);
    }

    
    JS_ALWAYS_TRUE(numElements.resize(2 * len));

    
    return MergeSortByKey(numElements.begin(), len, numElements.begin() + len,
                          SortComparatorNumerics[comp], vec);
}

bool
js::array_sort(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedValue fvalRoot(cx);
    Value &fval = fvalRoot.get();

    if (args.hasDefined(0)) {
        if (args[0].isPrimitive()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BAD_SORT_ARG);
            return false;
        }
        fval = args[0];     
    } else {
        fval.setNull();
    }

    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    uint32_t len;
    if (!GetLengthProperty(cx, obj, &len))
        return false;
    if (len < 2) {
        
        args.rval().setObject(*obj);
        return true;
    }

    





#if JS_BITS_PER_WORD == 32
    if (size_t(len) > size_t(-1) / (2 * sizeof(Value))) {
        js_ReportAllocationOverflow(cx);
        return false;
    }
#endif

    








    size_t n, undefs;
    {
        AutoValueVector vec(cx);
        if (!vec.reserve(2 * size_t(len)))
            return false;

        







        undefs = 0;
        bool allStrings = true;
        bool allInts = true;
        RootedValue v(cx);
        for (uint32_t i = 0; i < len; i++) {
            if (!CheckForInterrupt(cx))
                return false;

            
            bool hole;
            if (!GetElement(cx, obj, i, &hole, &v))
                return false;
            if (hole)
                continue;
            if (v.isUndefined()) {
                ++undefs;
                continue;
            }
            vec.infallibleAppend(v);
            allStrings = allStrings && v.isString();
            allInts = allInts && v.isInt32();
        }


        



        n = vec.length();
        if (n == 0 && undefs == 0) {
            args.rval().setObject(*obj);
            return true;
        }

        
        if (fval.isNull()) {
            



            if (allStrings) {
                JS_ALWAYS_TRUE(vec.resize(n * 2));
                if (!MergeSort(vec.begin(), n, vec.begin() + n, SortComparatorStrings(cx)))
                    return false;
            } else if (allInts) {
                JS_ALWAYS_TRUE(vec.resize(n * 2));
                if (!MergeSort(vec.begin(), n, vec.begin() + n,
                               SortComparatorLexicographicInt32())) {
                    return false;
                }
            } else {
                if (!SortLexicographically(cx, &vec, n))
                    return false;
            }
        } else {
            ComparatorMatchResult comp = MatchNumericComparator(cx, fval);
            if (comp == Match_Failure)
                return false;

            if (comp != Match_None) {
                if (allInts) {
                    JS_ALWAYS_TRUE(vec.resize(n * 2));
                    if (!MergeSort(vec.begin(), n, vec.begin() + n, SortComparatorInt32s[comp]))
                        return false;
                } else {
                    if (!SortNumerically(cx, &vec, n, comp))
                        return false;
                }
            } else {
                FastInvokeGuard fig(cx, fval);
                MOZ_ASSERT(!InParallelSection(),
                           "Array.sort() can't currently be used from parallel code");
                JS_ALWAYS_TRUE(vec.resize(n * 2));
                if (!MergeSort(vec.begin(), n, vec.begin() + n,
                               SortComparatorFunction(cx, fval, fig)))
                {
                    return false;
                }
            }
        }

        if (!InitArrayElements(cx, obj, 0, uint32_t(n), vec.begin(), DontUpdateTypes))
            return false;
    }

    
    while (undefs != 0) {
        --undefs;
        if (!CheckForInterrupt(cx) || !SetArrayElement(cx, obj, n++, UndefinedHandleValue))
            return false;
    }

    
    while (len > n) {
        if (!CheckForInterrupt(cx) || !DeletePropertyOrThrow(cx, obj, --len))
            return false;
    }
    args.rval().setObject(*obj);
    return true;
}

bool
js::NewbornArrayPush(JSContext *cx, HandleObject obj, const Value &v)
{
    Rooted<ArrayObject*> arr(cx, &obj->as<ArrayObject>());

    JS_ASSERT(!v.isMagic());
    JS_ASSERT(arr->lengthIsWritable());

    uint32_t length = arr->length();
    JS_ASSERT(length <= arr->getDenseCapacity());

    if (!arr->ensureElements(cx, length + 1))
        return false;

    arr->setDenseInitializedLength(length + 1);
    arr->setLengthInt32(length + 1);
    arr->initDenseElementWithType(cx, length, v);
    return true;
}


bool
js::array_push(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    
    uint32_t length;
    if (!GetLengthProperty(cx, obj, &length))
        return false;

    
    do {
        if (!obj->isNative() || obj->is<TypedArrayObject>())
            break;

        if (obj->is<ArrayObject>() && !obj->as<ArrayObject>().lengthIsWritable())
            break;

        if (ObjectMayHaveExtraIndexedProperties(obj))
            break;

        uint32_t argCount = args.length();
        JSObject::EnsureDenseResult result = obj->ensureDenseElements(cx, length, argCount);
        if (result == JSObject::ED_FAILED)
            return false;

        if (result == JSObject::ED_OK) {
            for (uint32_t i = 0, index = length; i < argCount; index++, i++)
                obj->setDenseElementWithType(cx, index, args[i]);
            uint32_t newlength = length + argCount;
            args.rval().setNumber(newlength);
            if (obj->is<ArrayObject>()) {
                obj->as<ArrayObject>().setLengthInt32(newlength);
                return true;
            }
            return SetLengthProperty(cx, obj, newlength);
        }

        MOZ_ASSERT(result == JSObject::ED_SPARSE);
    } while (false);

    
    if (!InitArrayElements(cx, obj, length, args.length(), args.array(), UpdateTypes))
        return false;

    
    double newlength = length + double(args.length());
    args.rval().setNumber(newlength);
    return SetLengthProperty(cx, obj, newlength);
}


bool
js::array_pop(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    
    uint32_t index;
    if (!GetLengthProperty(cx, obj, &index))
        return false;

    
    if (index == 0) {
        
        args.rval().setUndefined();
    } else {
        
        index--;

        
        bool hole;
        if (!GetElement(cx, obj, index, &hole, args.rval()))
            return false;

        
        if (!hole && !DeletePropertyOrThrow(cx, obj, index))
            return false;
    }

    
    return SetLengthProperty(cx, obj, index);
}

void
js::ArrayShiftMoveElements(JSObject *obj)
{
    JS_ASSERT(obj->is<ArrayObject>());
    JS_ASSERT(obj->as<ArrayObject>().lengthIsWritable());

    




    uint32_t initlen = obj->getDenseInitializedLength();
    obj->moveDenseElementsNoPreBarrier(0, 1, initlen);
}


bool
js::array_shift(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    
    uint32_t len;
    if (!GetLengthProperty(cx, obj, &len))
        return false;

    
    if (len == 0) {
        
        if (!SetLengthProperty(cx, obj, 0))
            return false;

        
        args.rval().setUndefined();
        return true;
    }

    uint32_t newlen = len - 1;

    
    if (obj->is<ArrayObject>() &&
        obj->getDenseInitializedLength() > 0 &&
        newlen < obj->getDenseCapacity() &&
        !ObjectMayHaveExtraIndexedProperties(obj))
    {
        args.rval().set(obj->getDenseElement(0));
        if (args.rval().isMagic(JS_ELEMENTS_HOLE))
            args.rval().setUndefined();

        if (!obj->maybeCopyElementsForWrite(cx))
            return false;

        obj->moveDenseElements(0, 1, obj->getDenseInitializedLength() - 1);
        obj->setDenseInitializedLength(obj->getDenseInitializedLength() - 1);

        if (!SetLengthProperty(cx, obj, newlen))
            return false;

        return js_SuppressDeletedProperty(cx, obj, INT_TO_JSID(newlen));
    }

    
    bool hole;
    if (!GetElement(cx, obj, uint32_t(0), &hole, args.rval()))
        return false;

    
    RootedValue value(cx);
    for (uint32_t i = 0; i < newlen; i++) {
        if (!CheckForInterrupt(cx))
            return false;
        if (!GetElement(cx, obj, i + 1, &hole, &value))
            return false;
        if (hole) {
            if (!DeletePropertyOrThrow(cx, obj, i))
                return false;
        } else {
            if (!SetArrayElement(cx, obj, i, value))
                return false;
        }
    }

    
    if (!DeletePropertyOrThrow(cx, obj, newlen))
        return false;

    
    return SetLengthProperty(cx, obj, newlen);
}

static bool
array_unshift(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    uint32_t length;
    if (!GetLengthProperty(cx, obj, &length))
        return false;

    double newlen = length;
    if (args.length() > 0) {
        
        if (length > 0) {
            bool optimized = false;
            do {
                if (!obj->is<ArrayObject>())
                    break;
                if (ObjectMayHaveExtraIndexedProperties(obj))
                    break;
                if (!obj->as<ArrayObject>().lengthIsWritable())
                    break;
                JSObject::EnsureDenseResult result = obj->ensureDenseElements(cx, length, args.length());
                if (result != JSObject::ED_OK) {
                    if (result == JSObject::ED_FAILED)
                        return false;
                    JS_ASSERT(result == JSObject::ED_SPARSE);
                    break;
                }
                obj->moveDenseElements(args.length(), 0, length);
                for (uint32_t i = 0; i < args.length(); i++)
                    obj->setDenseElement(i, MagicValue(JS_ELEMENTS_HOLE));
                optimized = true;
            } while (false);

            if (!optimized) {
                double last = length;
                double upperIndex = last + args.length();
                RootedValue value(cx);
                do {
                    --last, --upperIndex;
                    bool hole;
                    if (!CheckForInterrupt(cx))
                        return false;
                    if (!GetElement(cx, obj, last, &hole, &value))
                        return false;
                    if (hole) {
                        if (!DeletePropertyOrThrow(cx, obj, upperIndex))
                            return false;
                    } else {
                        if (!SetArrayElement(cx, obj, upperIndex, value))
                            return false;
                    }
                } while (last != 0);
            }
        }

        
        if (!InitArrayElements(cx, obj, 0, args.length(), args.array(), UpdateTypes))
            return false;

        newlen += args.length();
    }
    if (!SetLengthProperty(cx, obj, newlen))
        return false;

    
    args.rval().setNumber(newlen);
    return true;
}

static inline void
TryReuseArrayType(JSObject *obj, ArrayObject *narr)
{
    




    JS_ASSERT(narr->getProto()->hasNewType(&ArrayObject::class_, narr->type()));

    if (obj->is<ArrayObject>() && !obj->hasSingletonType() && obj->getProto() == narr->getProto())
        narr->setType(obj->type());
}








static inline bool
CanOptimizeForDenseStorage(HandleObject arr, uint32_t startingIndex, uint32_t count, JSContext *cx)
{
    
    if (UINT32_MAX - startingIndex < count)
        return false;

    
    if (!arr->is<ArrayObject>())
        return false;

    












    types::TypeObject *arrType = arr->getType(cx);
    if (MOZ_UNLIKELY(!arrType || arrType->hasAllFlags(OBJECT_FLAG_ITERATED)))
        return false;

    




    return !ObjectMayHaveExtraIndexedProperties(arr) &&
           startingIndex + count <= arr->getDenseInitializedLength();
}


bool
js::array_splice(JSContext *cx, unsigned argc, Value *vp)
{
    return array_splice_impl(cx, argc, vp, true);
}

bool
js::array_splice_impl(JSContext *cx, unsigned argc, Value *vp, bool returnValueIsUsed)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    
    uint32_t len;
    if (!GetLengthProperty(cx, obj, &len))
        return false;

    
    double relativeStart;
    if (!ToInteger(cx, args.get(0), &relativeStart))
        return false;

    
    uint32_t actualStart;
    if (relativeStart < 0)
        actualStart = Max(len + relativeStart, 0.0);
    else
        actualStart = Min(relativeStart, double(len));

    
    uint32_t actualDeleteCount;
    if (args.length() != 1) {
        double deleteCountDouble;
        RootedValue cnt(cx, args.length() >= 2 ? args[1] : Int32Value(0));
        if (!ToInteger(cx, cnt, &deleteCountDouble))
            return false;
        actualDeleteCount = Min(Max(deleteCountDouble, 0.0), double(len - actualStart));
    } else {
        



        actualDeleteCount = len - actualStart;
    }

    JS_ASSERT(len - actualStart >= actualDeleteCount);

    
    Rooted<ArrayObject*> arr(cx);
    if (CanOptimizeForDenseStorage(obj, actualStart, actualDeleteCount, cx)) {
        if (returnValueIsUsed) {
            arr = NewDenseCopiedArray(cx, actualDeleteCount, obj, actualStart);
            if (!arr)
                return false;
            TryReuseArrayType(obj, arr);
        }
    } else {
        arr = NewDenseFullyAllocatedArray(cx, actualDeleteCount);
        if (!arr)
            return false;
        TryReuseArrayType(obj, arr);

        RootedValue fromValue(cx);
        for (uint32_t k = 0; k < actualDeleteCount; k++) {
            bool hole;
            if (!CheckForInterrupt(cx) ||
                !GetElement(cx, obj, actualStart + k, &hole, &fromValue) ||
                (!hole && !JSObject::defineElement(cx, arr, k, fromValue)))
            {
                return false;
            }
        }
    }

    
    uint32_t itemCount = (args.length() >= 2) ? (args.length() - 2) : 0;

    if (itemCount < actualDeleteCount) {
        
        uint32_t sourceIndex = actualStart + actualDeleteCount;
        uint32_t targetIndex = actualStart + itemCount;
        uint32_t finalLength = len - actualDeleteCount + itemCount;

        if (CanOptimizeForDenseStorage(obj, 0, len, cx)) {
            if (!obj->maybeCopyElementsForWrite(cx))
                return false;

            
            obj->moveDenseElements(targetIndex, sourceIndex, len - sourceIndex);

            



            obj->setDenseInitializedLength(finalLength);

            
            obj->shrinkElements(cx, finalLength);
        } else {
            






            
            RootedValue fromValue(cx);
            for (uint32_t from = sourceIndex, to = targetIndex; from < len; from++, to++) {
                if (!CheckForInterrupt(cx))
                    return false;

                bool hole;
                if (!GetElement(cx, obj, from, &hole, &fromValue))
                    return false;
                if (hole) {
                    if (!DeletePropertyOrThrow(cx, obj, to))
                        return false;
                } else {
                    if (!SetArrayElement(cx, obj, to, fromValue))
                        return false;
                }
            }

            
            for (uint32_t k = len; k > finalLength; k--) {
                if (!DeletePropertyOrThrow(cx, obj, k - 1))
                    return false;
            }
        }
    } else if (itemCount > actualDeleteCount) {
        

        























        if (obj->is<ArrayObject>()) {
            Rooted<ArrayObject*> arr(cx, &obj->as<ArrayObject>());
            if (arr->lengthIsWritable()) {
                JSObject::EnsureDenseResult res =
                    arr->ensureDenseElements(cx, arr->length(), itemCount - actualDeleteCount);
                if (res == JSObject::ED_FAILED)
                    return false;
            }
        }

        if (CanOptimizeForDenseStorage(obj, len, itemCount - actualDeleteCount, cx)) {
            if (!obj->maybeCopyElementsForWrite(cx))
                return false;
            obj->moveDenseElements(actualStart + itemCount,
                                   actualStart + actualDeleteCount,
                                   len - (actualStart + actualDeleteCount));
            obj->setDenseInitializedLength(len + itemCount - actualDeleteCount);
        } else {
            RootedValue fromValue(cx);
            for (double k = len - actualDeleteCount; k > actualStart; k--) {
                if (!CheckForInterrupt(cx))
                    return false;

                double from = k + actualDeleteCount - 1;
                double to = k + itemCount - 1;

                bool hole;
                if (!GetElement(cx, obj, from, &hole, &fromValue))
                    return false;

                if (hole) {
                    if (!DeletePropertyOrThrow(cx, obj, to))
                        return false;
                } else {
                    if (!SetArrayElement(cx, obj, to, fromValue))
                        return false;
                }
            }
        }
    }

    
    Value *items = args.array() + 2;

    
    for (uint32_t k = actualStart, i = 0; i < itemCount; i++, k++) {
        if (!SetArrayElement(cx, obj, k, HandleValue::fromMarkedLocation(&items[i])))
            return false;
    }

    
    double finalLength = double(len) - actualDeleteCount + itemCount;
    if (!SetLengthProperty(cx, obj, finalLength))
        return false;

    
    if (returnValueIsUsed)
        args.rval().setObject(*arr);

    return true;
}

bool
js::array_concat_dense(JSContext *cx, Handle<ArrayObject*> arr1, Handle<ArrayObject*> arr2,
                       Handle<ArrayObject*> result)
{
    uint32_t initlen1 = arr1->getDenseInitializedLength();
    JS_ASSERT(initlen1 == arr1->length());

    uint32_t initlen2 = arr2->getDenseInitializedLength();
    JS_ASSERT(initlen2 == arr2->length());

    
    uint32_t len = initlen1 + initlen2;

    if (!result->ensureElements(cx, len))
        return false;

    JS_ASSERT(!result->getDenseInitializedLength());
    result->setDenseInitializedLength(len);

    result->initDenseElements(0, arr1->getDenseElements(), initlen1);
    result->initDenseElements(initlen1, arr2->getDenseElements(), initlen2);
    result->setLengthInt32(len);
    return true;
}




bool
js::array_concat(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    Value *p = args.array() - 1;

    
    RootedObject aobj(cx, ToObject(cx, args.thisv()));
    if (!aobj)
        return false;

    Rooted<ArrayObject*> narr(cx);
    uint32_t length;
    if (aobj->is<ArrayObject>() && !aobj->isIndexed()) {
        length = aobj->as<ArrayObject>().length();
        uint32_t initlen = aobj->getDenseInitializedLength();
        narr = NewDenseCopiedArray(cx, initlen, aobj, 0);
        if (!narr)
            return false;
        TryReuseArrayType(aobj, narr);
        narr->setLength(cx, length);
        args.rval().setObject(*narr);
        if (argc == 0)
            return true;
        argc--;
        p++;
    } else {
        narr = NewDenseEmptyArray(cx);
        if (!narr)
            return false;
        args.rval().setObject(*narr);
        length = 0;
    }

    
    for (unsigned i = 0; i <= argc; i++) {
        if (!CheckForInterrupt(cx))
            return false;
        HandleValue v = HandleValue::fromMarkedLocation(&p[i]);
        if (v.isObject()) {
            RootedObject obj(cx, &v.toObject());
            if (ObjectClassIs(obj, ESClass_Array, cx)) {
                uint32_t alength;
                if (!GetLengthProperty(cx, obj, &alength))
                    return false;
                RootedValue tmp(cx);
                for (uint32_t slot = 0; slot < alength; slot++) {
                    bool hole;
                    if (!CheckForInterrupt(cx) || !GetElement(cx, obj, slot, &hole, &tmp))
                        return false;

                    



                    if (!hole && !SetArrayElement(cx, narr, length + slot, tmp))
                        return false;
                }
                length += alength;
                continue;
            }
        }

        if (!SetArrayElement(cx, narr, length, v))
            return false;
        length++;
    }

    return SetLengthProperty(cx, narr, length);
}

static bool
array_slice(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    uint32_t length;
    if (!GetLengthProperty(cx, obj, &length))
        return false;

    uint32_t begin = 0;
    uint32_t end = length;
    if (args.length() > 0) {
        double d;
        if (!ToInteger(cx, args[0], &d))
            return false;
        if (d < 0) {
            d += length;
            if (d < 0)
                d = 0;
        } else if (d > length) {
            d = length;
        }
        begin = (uint32_t)d;

        if (args.hasDefined(1)) {
            if (!ToInteger(cx, args[1], &d))
                return false;
            if (d < 0) {
                d += length;
                if (d < 0)
                    d = 0;
            } else if (d > length) {
                d = length;
            }
            end = (uint32_t)d;
        }
    }

    if (begin > end)
        begin = end;

    Rooted<ArrayObject*> narr(cx);
    narr = NewDenseFullyAllocatedArray(cx, end - begin);
    if (!narr)
        return false;
    TryReuseArrayType(obj, narr);

    if (obj->is<ArrayObject>() && !ObjectMayHaveExtraIndexedProperties(obj)) {
        if (obj->getDenseInitializedLength() > begin) {
            uint32_t numSourceElements = obj->getDenseInitializedLength() - begin;
            uint32_t initLength = Min(numSourceElements, end - begin);
            narr->setDenseInitializedLength(initLength);
            narr->initDenseElements(0, &obj->getDenseElement(begin), initLength);
        }
        args.rval().setObject(*narr);
        return true;
    }

    if (js::SliceOp op = obj->getOps()->slice) {
        
        JSObject::EnsureDenseResult result = narr->ensureDenseElements(cx, 0, end - begin);
        if (result == JSObject::ED_FAILED)
             return false;

        if (result == JSObject::ED_OK) {
            if (!op(cx, obj, begin, end, narr))
                return false;

            args.rval().setObject(*narr);
            return true;
        }

        
        JS_ASSERT(result == JSObject::ED_SPARSE);
    }


    if (!SliceSlowly(cx, obj, obj, begin, end, narr))
        return false;

    args.rval().setObject(*narr);
    return true;
}

JS_FRIEND_API(bool)
js::SliceSlowly(JSContext* cx, HandleObject obj, HandleObject receiver,
                uint32_t begin, uint32_t end, HandleObject result)
{
    RootedValue value(cx);
    for (uint32_t slot = begin; slot < end; slot++) {
        bool hole;
        if (!CheckForInterrupt(cx) ||
            !GetElement(cx, obj, receiver, slot, &hole, &value))
        {
            return false;
        }
        if (!hole && !JSObject::defineElement(cx, result, slot - begin, value))
            return false;
    }
    return true;
}


static bool
array_filter(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    
    uint32_t len;
    if (!GetLengthProperty(cx, obj, &len))
        return false;

    
    if (args.length() == 0) {
        js_ReportMissingArg(cx, args.calleev(), 0);
        return false;
    }
    RootedObject callable(cx, ValueToCallable(cx, args[0], args.length() - 1));
    if (!callable)
        return false;

    
    RootedValue thisv(cx, args.length() >= 2 ? args[1] : UndefinedValue());

    
    RootedObject arr(cx, NewDenseFullyAllocatedArray(cx, 0));
    if (!arr)
        return false;
    TypeObject *newtype = GetTypeCallerInitObject(cx, JSProto_Array);
    if (!newtype)
        return false;
    arr->setType(newtype);

    
    uint32_t k = 0;

    
    uint32_t to = 0;

    
    JS_ASSERT(!InParallelSection());
    FastInvokeGuard fig(cx, ObjectValue(*callable));
    InvokeArgs &args2 = fig.args();
    RootedValue kValue(cx);
    while (k < len) {
        if (!CheckForInterrupt(cx))
            return false;

        
        bool kNotPresent;
        if (!GetElement(cx, obj, k, &kNotPresent, &kValue))
            return false;

        
        if (!kNotPresent) {
            if (!args2.init(3))
                return false;
            args2.setCallee(ObjectValue(*callable));
            args2.setThis(thisv);
            args2[0].set(kValue);
            args2[1].setNumber(k);
            args2[2].setObject(*obj);
            if (!fig.invoke(cx))
                return false;

            if (ToBoolean(args2.rval())) {
                if (!SetArrayElement(cx, arr, to, kValue))
                    return false;
                to++;
            }
        }

        
        k++;
    }

    
    args.rval().setObject(*arr);
    return true;
}

static bool
array_isArray(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    bool isArray = args.length() > 0 && IsObjectWithClass(args[0], ESClass_Array, cx);
    args.rval().setBoolean(isArray);
    return true;
}

static bool
IsArrayConstructor(const Value &v)
{
    
    
    
    return v.isObject() &&
           v.toObject().is<JSFunction>() &&
           v.toObject().as<JSFunction>().isNative() &&
           v.toObject().as<JSFunction>().native() == js_Array;
}

static bool
ArrayFromCallArgs(JSContext *cx, RootedTypeObject &type, CallArgs &args)
{
    if (!InitArrayTypes(cx, type, args.array(), args.length()))
        return false;
    JSObject *obj = (args.length() == 0)
        ? NewDenseEmptyArray(cx)
        : NewDenseCopiedArray(cx, args.length(), args.array());
    if (!obj)
        return false;
    obj->setType(type);
    args.rval().setObject(*obj);
    return true;
}

static bool
array_of(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (IsArrayConstructor(args.thisv()) || !IsConstructor(args.thisv())) {
        
        
        RootedTypeObject type(cx, GetTypeCallerInitObject(cx, JSProto_Array));
        if (!type)
            return false;
        return ArrayFromCallArgs(cx, type, args);
    }

    
    RootedObject obj(cx);
    {
        RootedValue v(cx);
        Value argv[1] = {NumberValue(args.length())};
        if (!InvokeConstructor(cx, args.thisv(), 1, argv, v.address()))
            return false;
        obj = ToObject(cx, v);
        if (!obj)
            return false;
    }

    
    for (unsigned k = 0; k < args.length(); k++) {
        if (!JSObject::defineElement(cx, obj, k, args[k]))
            return false;
    }

    
    RootedValue v(cx, NumberValue(args.length()));
    if (!JSObject::setProperty(cx, obj, obj, cx->names().length, &v, true))
        return false;

    
    args.rval().setObject(*obj);
    return true;
}

#define GENERIC JSFUN_GENERIC_NATIVE

static const JSFunctionSpec array_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,      array_toSource,     0,0),
#endif
    JS_FN(js_toString_str,      array_toString,     0,0),
    JS_FN(js_toLocaleString_str,array_toLocaleString,0,0),

    
    JS_FN("join",               js::array_join,     1,JSFUN_GENERIC_NATIVE),
    JS_FN("reverse",            array_reverse,      0,JSFUN_GENERIC_NATIVE),
    JS_FN("sort",               array_sort,         1,JSFUN_GENERIC_NATIVE),
    JS_FN("push",               array_push,         1,JSFUN_GENERIC_NATIVE),
    JS_FN("pop",                array_pop,          0,JSFUN_GENERIC_NATIVE),
    JS_FN("shift",              array_shift,        0,JSFUN_GENERIC_NATIVE),
    JS_FN("unshift",            array_unshift,      1,JSFUN_GENERIC_NATIVE),
    JS_FN("splice",             array_splice,       2,JSFUN_GENERIC_NATIVE),

    
    JS_FN("concat",             array_concat,       1,JSFUN_GENERIC_NATIVE),
    JS_FN("slice",              array_slice,        2,JSFUN_GENERIC_NATIVE),

    JS_SELF_HOSTED_FN("lastIndexOf", "ArrayLastIndexOf", 1,0),
    JS_SELF_HOSTED_FN("indexOf",     "ArrayIndexOf",     1,0),
    JS_SELF_HOSTED_FN("forEach",     "ArrayForEach",     1,0),
    JS_SELF_HOSTED_FN("map",         "ArrayMap",         1,0),
    JS_SELF_HOSTED_FN("reduce",      "ArrayReduce",      1,0),
    JS_SELF_HOSTED_FN("reduceRight", "ArrayReduceRight", 1,0),
    JS_FN("filter",             array_filter,       1,JSFUN_GENERIC_NATIVE),
    JS_SELF_HOSTED_FN("some",        "ArraySome",        1,0),
    JS_SELF_HOSTED_FN("every",       "ArrayEvery",       1,0),

#ifdef ENABLE_PARALLEL_JS
    
    JS_SELF_HOSTED_FN("mapPar",      "ArrayMapPar",      2,0),
    JS_SELF_HOSTED_FN("reducePar",   "ArrayReducePar",   2,0),
    JS_SELF_HOSTED_FN("scanPar",     "ArrayScanPar",     2,0),
    JS_SELF_HOSTED_FN("scatterPar",  "ArrayScatterPar",  5,0),
    JS_SELF_HOSTED_FN("filterPar",   "ArrayFilterPar",   2,0),
#endif

    
    JS_SELF_HOSTED_FN("find",        "ArrayFind",        1,0),
    JS_SELF_HOSTED_FN("findIndex",   "ArrayFindIndex",   1,0),
    JS_SELF_HOSTED_FN("copyWithin",  "ArrayCopyWithin",  3,0),

    JS_SELF_HOSTED_FN("fill",        "ArrayFill",        3,0),

    JS_SELF_HOSTED_FN("@@iterator",  "ArrayValues",      0,0),
    JS_SELF_HOSTED_FN("entries",     "ArrayEntries",     0,0),
    JS_SELF_HOSTED_FN("keys",        "ArrayKeys",        0,0),
    JS_FS_END
};

static const JSFunctionSpec array_static_methods[] = {
    JS_FN("isArray",            array_isArray,      1,0),
    JS_SELF_HOSTED_FN("lastIndexOf", "ArrayStaticLastIndexOf", 2,0),
    JS_SELF_HOSTED_FN("indexOf",     "ArrayStaticIndexOf", 2,0),
    JS_SELF_HOSTED_FN("forEach",     "ArrayStaticForEach", 2,0),
    JS_SELF_HOSTED_FN("map",         "ArrayStaticMap",   2,0),
    JS_SELF_HOSTED_FN("every",       "ArrayStaticEvery", 2,0),
    JS_SELF_HOSTED_FN("some",        "ArrayStaticSome",  2,0),
    JS_SELF_HOSTED_FN("reduce",      "ArrayStaticReduce", 2,0),
    JS_SELF_HOSTED_FN("reduceRight", "ArrayStaticReduceRight", 2,0),
    JS_SELF_HOSTED_FN("from",        "ArrayFrom", 3,0),
    JS_FN("of",                 array_of,           0,0),

#ifdef ENABLE_PARALLEL_JS
    JS_SELF_HOSTED_FN("build",       "ArrayStaticBuild", 2,0),
    
    JS_SELF_HOSTED_FN("buildPar",    "ArrayStaticBuildPar", 3,0),
#endif

    JS_FS_END
};


bool
js_Array(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedTypeObject type(cx, GetTypeCallerInitObject(cx, JSProto_Array));
    if (!type)
        return false;

    if (args.length() != 1 || !args[0].isNumber())
        return ArrayFromCallArgs(cx, type, args);

    uint32_t length;
    if (args[0].isInt32()) {
        int32_t i = args[0].toInt32();
        if (i < 0) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BAD_ARRAY_LENGTH);
            return false;
        }
        length = uint32_t(i);
    } else {
        double d = args[0].toDouble();
        length = ToUint32(d);
        if (d != double(length)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BAD_ARRAY_LENGTH);
            return false;
        }
    }

    



    AllocatingBehaviour allocating = (length <= ArrayObject::EagerAllocationMaxLength)
                                   ? NewArray_FullyAllocating
                                   : NewArray_PartlyAllocating;
    RootedObject obj(cx, NewDenseArray(cx, length, type, allocating));
    if (!obj)
        return false;

    args.rval().setObject(*obj);
    return true;
}

static JSObject *
CreateArrayPrototype(JSContext *cx, JSProtoKey key)
{
    JS_ASSERT(key == JSProto_Array);
    RootedObject proto(cx, cx->global()->getOrCreateObjectPrototype(cx));
    if (!proto)
        return nullptr;

    RootedTypeObject type(cx, cx->getNewType(&ArrayObject::class_, TaggedProto(proto)));
    if (!type)
        return nullptr;

    JSObject *metadata = nullptr;
    if (!NewObjectMetadata(cx, &metadata))
        return nullptr;

    RootedShape shape(cx, EmptyShape::getInitialShape(cx, &ArrayObject::class_, TaggedProto(proto),
                                                      proto->getParent(), metadata,
                                                      gc::FINALIZE_OBJECT0));
    if (!shape)
        return nullptr;

    RootedObject arrayProto(cx, JSObject::createArray(cx, gc::FINALIZE_OBJECT4, gc::TenuredHeap, shape, type, 0));
    if (!arrayProto || !JSObject::setSingletonType(cx, arrayProto) || !AddLengthProperty(cx, arrayProto))
        return nullptr;

    





    if (!JSObject::setNewTypeUnknown(cx, &ArrayObject::class_, arrayProto))
        return nullptr;

    return arrayProto;
}

const Class ArrayObject::class_ = {
    "Array",
    JSCLASS_HAS_CACHED_PROTO(JSProto_Array),
    array_addProperty,
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    nullptr,
    nullptr,        
    nullptr,        
    nullptr,        
    nullptr,        
    {
        GenericCreateConstructor<js_Array, 1, JSFunction::FinalizeKind>,
        CreateArrayPrototype,
        array_static_methods,
        array_methods
    }
};





static inline bool
EnsureNewArrayElements(ExclusiveContext *cx, JSObject *obj, uint32_t length)
{
    



    DebugOnly<uint32_t> cap = obj->getDenseCapacity();

    if (!obj->ensureElements(cx, length))
        return false;

    JS_ASSERT_IF(cap, !obj->hasDynamicElements());

    return true;
}

template <uint32_t maxLength>
static MOZ_ALWAYS_INLINE ArrayObject *
NewArray(ExclusiveContext *cxArg, uint32_t length,
         JSObject *protoArg, NewObjectKind newKind = GenericObject)
{
    gc::AllocKind allocKind = GuessArrayGCKind(length);
    JS_ASSERT(CanBeFinalizedInBackground(allocKind, &ArrayObject::class_));
    allocKind = GetBackgroundAllocKind(allocKind);

    NewObjectCache::EntryIndex entry = -1;
    if (JSContext *cx = cxArg->maybeJSContext()) {
        NewObjectCache &cache = cx->runtime()->newObjectCache;
        if (newKind == GenericObject &&
            !cx->compartment()->hasObjectMetadataCallback() &&
            cache.lookupGlobal(&ArrayObject::class_, cx->global(), allocKind, &entry))
        {
            gc::InitialHeap heap = GetInitialHeap(newKind, &ArrayObject::class_);
            JSObject *obj = cache.newObjectFromHit<NoGC>(cx, entry, heap);
            if (obj) {
                
                ArrayObject *arr = &obj->as<ArrayObject>();
                arr->setFixedElements();
                arr->setLength(cx, length);
                if (maxLength > 0 &&
                    !EnsureNewArrayElements(cx, arr, std::min(maxLength, length)))
                {
                    return nullptr;
                }
                return arr;
            } else {
                RootedObject proto(cxArg, protoArg);
                obj = cache.newObjectFromHit<CanGC>(cx, entry, heap);
                JS_ASSERT(!obj);
                protoArg = proto;
            }
        }
    }

    RootedObject proto(cxArg, protoArg);
    if (protoArg)
        JS::PoisonPtr(&protoArg);

    if (!proto && !GetBuiltinPrototype(cxArg, JSProto_Array, &proto))
        return nullptr;

    RootedTypeObject type(cxArg, cxArg->getNewType(&ArrayObject::class_, TaggedProto(proto)));
    if (!type)
        return nullptr;

    JSObject *metadata = nullptr;
    if (!NewObjectMetadata(cxArg, &metadata))
        return nullptr;

    



    RootedShape shape(cxArg, EmptyShape::getInitialShape(cxArg, &ArrayObject::class_,
                                                         TaggedProto(proto), cxArg->global(),
                                                         metadata, gc::FINALIZE_OBJECT0));
    if (!shape)
        return nullptr;

    Rooted<ArrayObject*> arr(cxArg, JSObject::createArray(cxArg, allocKind,
                                                          GetInitialHeap(newKind, &ArrayObject::class_),
                                                          shape, type, length));
    if (!arr)
        return nullptr;

    if (shape->isEmptyShape()) {
        if (!AddLengthProperty(cxArg, arr))
            return nullptr;
        shape = arr->lastProperty();
        EmptyShape::insertInitialShape(cxArg, shape, proto);
    }

    if (newKind == SingletonObject && !JSObject::setSingletonType(cxArg, arr))
        return nullptr;

    if (entry != -1) {
        cxArg->asJSContext()->runtime()->newObjectCache.fillGlobal(entry, &ArrayObject::class_,
                                                                   cxArg->global(), allocKind, arr);
    }

    if (maxLength > 0 && !EnsureNewArrayElements(cxArg, arr, std::min(maxLength, length)))
        return nullptr;

    probes::CreateObject(cxArg, arr);
    return arr;
}

ArrayObject * JS_FASTCALL
js::NewDenseEmptyArray(JSContext *cx, JSObject *proto ,
                       NewObjectKind newKind )
{
    return NewArray<0>(cx, 0, proto, newKind);
}

ArrayObject * JS_FASTCALL
js::NewDenseFullyAllocatedArray(ExclusiveContext *cx, uint32_t length,
                                JSObject *proto ,
                                NewObjectKind newKind )
{
    return NewArray<JSObject::NELEMENTS_LIMIT>(cx, length, proto, newKind);
}

ArrayObject * JS_FASTCALL
js::NewDensePartlyAllocatedArray(ExclusiveContext *cx, uint32_t length,
                                 JSObject *proto ,
                                 NewObjectKind newKind )
{
    return NewArray<ArrayObject::EagerAllocationMaxLength>(cx, length, proto, newKind);
}

ArrayObject * JS_FASTCALL
js::NewDenseUnallocatedArray(ExclusiveContext *cx, uint32_t length, JSObject *proto ,
                             NewObjectKind newKind )
{
    return NewArray<0>(cx, length, proto, newKind);
}

ArrayObject * JS_FASTCALL
js::NewDenseArray(ExclusiveContext *cx, uint32_t length, HandleTypeObject type,
                  AllocatingBehaviour allocating)
{
    NewObjectKind newKind = !type ? SingletonObject : GenericObject;
    if (type && type->shouldPreTenure())
        newKind = TenuredObject;

    ArrayObject *arr;
    if (allocating == NewArray_Unallocating) {
        arr = NewDenseUnallocatedArray(cx, length, nullptr, newKind);
    } else if (allocating == NewArray_PartlyAllocating) {
        arr = NewDensePartlyAllocatedArray(cx, length, nullptr, newKind);
    } else {
        JS_ASSERT(allocating == NewArray_FullyAllocating);
        arr = NewDenseFullyAllocatedArray(cx, length, nullptr, newKind);
    }
    if (!arr)
        return nullptr;

    if (type)
        arr->setType(type);

    
    
    if (arr->length() > INT32_MAX)
        arr->setLength(cx, arr->length());

    return arr;
}

ArrayObject *
js::NewDenseCopiedArray(JSContext *cx, uint32_t length, HandleObject src, uint32_t elementOffset,
                        JSObject *proto )
{
    JS_ASSERT(!src->isIndexed());

    ArrayObject* arr = NewArray<JSObject::NELEMENTS_LIMIT>(cx, length, proto);
    if (!arr)
        return nullptr;

    JS_ASSERT(arr->getDenseCapacity() >= length);

    const Value* vp = src->getDenseElements() + elementOffset;
    arr->setDenseInitializedLength(vp ? length : 0);

    if (vp)
        arr->initDenseElements(0, vp, length);

    return arr;
}


ArrayObject *
js::NewDenseCopiedArray(JSContext *cx, uint32_t length, const Value *values,
                        JSObject *proto , NewObjectKind newKind )
{
    ArrayObject* arr = NewArray<JSObject::NELEMENTS_LIMIT>(cx, length, proto);
    if (!arr)
        return nullptr;

    JS_ASSERT(arr->getDenseCapacity() >= length);

    arr->setDenseInitializedLength(values ? length : 0);

    if (values)
        arr->initDenseElements(0, values, length);

    return arr;
}

ArrayObject *
js::NewDenseFullyAllocatedArrayWithTemplate(JSContext *cx, uint32_t length, JSObject *templateObject)
{
    gc::AllocKind allocKind = GuessArrayGCKind(length);
    JS_ASSERT(CanBeFinalizedInBackground(allocKind, &ArrayObject::class_));
    allocKind = GetBackgroundAllocKind(allocKind);

    RootedTypeObject type(cx, templateObject->type());
    RootedShape shape(cx, templateObject->lastProperty());

    gc::InitialHeap heap = GetInitialHeap(GenericObject, &ArrayObject::class_);
    Rooted<ArrayObject *> arr(cx, JSObject::createArray(cx, allocKind, heap, shape, type, length));
    if (!arr)
        return nullptr;

    if (!EnsureNewArrayElements(cx, arr, length))
        return nullptr;

    probes::CreateObject(cx, arr);

    return arr;
}

JSObject *
js::NewDenseCopyOnWriteArray(JSContext *cx, HandleObject templateObject, gc::InitialHeap heap)
{
    RootedShape shape(cx, templateObject->lastProperty());

    JS_ASSERT(!gc::IsInsideNursery(templateObject));

    JSObject *metadata = nullptr;
    if (!NewObjectMetadata(cx, &metadata))
        return nullptr;
    if (metadata) {
        shape = Shape::setObjectMetadata(cx, metadata, templateObject->getTaggedProto(), shape);
        if (!shape)
            return nullptr;
    }

    Rooted<ArrayObject *> arr(cx, JSObject::createCopyOnWriteArray(cx, heap, shape, templateObject));
    if (!arr)
        return nullptr;

    probes::CreateObject(cx, arr);
    return arr;
}

#ifdef DEBUG
bool
js_ArrayInfo(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JSObject *obj;

    for (unsigned i = 0; i < args.length(); i++) {
        RootedValue arg(cx, args[i]);

        char *bytes = DecompileValueGenerator(cx, JSDVG_SEARCH_STACK, arg, NullPtr());
        if (!bytes)
            return false;
        if (arg.isPrimitive() ||
            !(obj = arg.toObjectOrNull())->is<ArrayObject>()) {
            fprintf(stderr, "%s: not array\n", bytes);
            js_free(bytes);
            continue;
        }
        fprintf(stderr, "%s: (len %u", bytes, obj->as<ArrayObject>().length());
        fprintf(stderr, ", capacity %u", obj->getDenseCapacity());
        fputs(")\n", stderr);
        js_free(bytes);
    }

    args.rval().setUndefined();
    return true;
}
#endif
