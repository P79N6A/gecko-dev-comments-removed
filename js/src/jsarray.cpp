





#include "jsarray.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/CheckedInt.h"
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
#include "js/Class.h"
#include "js/Conversions.h"
#include "vm/ArgumentsObject.h"
#include "vm/Interpreter.h"
#include "vm/Shape.h"
#include "vm/StringBuffer.h"
#include "vm/TypedArrayCommon.h"

#include "jsatominlines.h"

#include "vm/ArgumentsObject-inl.h"
#include "vm/ArrayObject-inl.h"
#include "vm/Interpreter-inl.h"
#include "vm/NativeObject-inl.h"
#include "vm/Runtime-inl.h"

using namespace js;
using namespace js::gc;

using mozilla::Abs;
using mozilla::ArrayLength;
using mozilla::CeilingLog2;
using mozilla::CheckedInt;
using mozilla::DebugOnly;
using mozilla::IsNaN;
using mozilla::UniquePtr;

using JS::AutoCheckCannotGC;
using JS::ToUint32;

bool
js::GetLengthProperty(JSContext* cx, HandleObject obj, uint32_t* lengthp)
{
    if (obj->is<ArrayObject>()) {
        *lengthp = obj->as<ArrayObject>().length();
        return true;
    }

    if (obj->is<ArgumentsObject>()) {
        ArgumentsObject& argsobj = obj->as<ArgumentsObject>();
        if (!argsobj.hasOverriddenLength()) {
            *lengthp = argsobj.initialLength();
            return true;
        }
    }

    RootedValue value(cx);
    if (!GetProperty(cx, obj, obj, cx->names().length, &value))
        return false;

    if (value.isInt32()) {
        *lengthp = uint32_t(value.toInt32()); 
        return true;
    }

    return ToUint32(cx, value, lengthp);
}




















template <typename CharT>
static bool
StringIsArrayIndex(const CharT* s, uint32_t length, uint32_t* indexp)
{
    const CharT* end = s + length;

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
        MOZ_ASSERT(index <= MAX_ARRAY_INDEX);
        *indexp = index;
        return true;
    }

    return false;
}

JS_FRIEND_API(bool)
js::StringIsArrayIndex(JSLinearString* str, uint32_t* indexp)
{
    AutoCheckCannotGC nogc;
    return str->hasLatin1Chars()
           ? ::StringIsArrayIndex(str->latin1Chars(nogc), str->length(), indexp)
           : ::StringIsArrayIndex(str->twoByteChars(nogc), str->length(), indexp);
}

static bool
ToId(JSContext* cx, double index, MutableHandleId id)
{
    if (index == uint32_t(index))
        return IndexToId(cx, uint32_t(index), id);

    Value tmp = DoubleValue(index);
    return ValueToId<CanGC>(cx, HandleValue::fromMarkedLocation(&tmp), id);
}

static bool
ToId(JSContext* cx, uint32_t index, MutableHandleId id)
{
    return IndexToId(cx, index, id);
}







template <typename IndexType>
static inline bool
DoGetElement(JSContext* cx, HandleObject obj, HandleObject receiver,
             IndexType index, bool* hole, MutableHandleValue vp)
{
    RootedId id(cx);
    if (!ToId(cx, index, &id))
        return false;

    bool found;
    if (!HasProperty(cx, obj, id, &found))
        return false;

    if (found) {
        if (!GetProperty(cx, obj, receiver, id, vp))
            return false;
    } else {
        vp.setUndefined();
    }
    *hole = !found;
    return true;
}

template <typename IndexType>
static void
AssertGreaterThanZero(IndexType index)
{
    MOZ_ASSERT(index >= 0);
    MOZ_ASSERT(index == floor(index));
}

template<>
void
AssertGreaterThanZero(uint32_t index)
{
}

template <typename IndexType>
static bool
GetElement(JSContext* cx, HandleObject obj, HandleObject receiver,
           IndexType index, bool* hole, MutableHandleValue vp)
{
    AssertGreaterThanZero(index);
    if (obj->isNative() && index < obj->as<NativeObject>().getDenseInitializedLength()) {
        vp.set(obj->as<NativeObject>().getDenseElement(uint32_t(index)));
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
GetElement(JSContext* cx, HandleObject obj, IndexType index, bool* hole, MutableHandleValue vp)
{
    return GetElement(cx, obj, obj, index, hole, vp);
}

void
ElementAdder::append(JSContext* cx, HandleValue v)
{
    MOZ_ASSERT(index_ < length_);
    if (resObj_)
        resObj_->as<NativeObject>().setDenseElementWithType(cx, index_++, v);
    else
        vp_[index_++] = v;
}

void
ElementAdder::appendHole()
{
    MOZ_ASSERT(getBehavior_ == ElementAdder::CheckHasElemPreserveHoles);
    MOZ_ASSERT(index_ < length_);
    if (resObj_) {
        MOZ_ASSERT(resObj_->as<NativeObject>().getDenseElement(index_).isMagic(JS_ELEMENTS_HOLE));
        index_++;
    } else {
        vp_[index_++].setMagic(JS_ELEMENTS_HOLE);
    }
}

bool
js::GetElementsWithAdder(JSContext* cx, HandleObject obj, HandleObject receiver,
                         uint32_t begin, uint32_t end, ElementAdder* adder)
{
    MOZ_ASSERT(begin <= end);

    RootedValue val(cx);
    for (uint32_t i = begin; i < end; i++) {
        if (adder->getBehavior() == ElementAdder::CheckHasElemPreserveHoles) {
            bool hole;
            if (!GetElement(cx, obj, receiver, i, &hole, &val))
                return false;
            if (hole) {
                adder->appendHole();
                continue;
            }
        } else {
            MOZ_ASSERT(adder->getBehavior() == ElementAdder::GetElement);
            if (!GetElement(cx, obj, receiver, i, &val))
                return false;
        }
        adder->append(cx, val);
    }

    return true;
}

bool
js::GetElements(JSContext* cx, HandleObject aobj, uint32_t length, Value* vp)
{
    if (aobj->is<ArrayObject>() &&
        length <= aobj->as<ArrayObject>().getDenseInitializedLength() &&
        !ObjectMayHaveExtraIndexedProperties(aobj))
    {
        
        const Value* srcbeg = aobj->as<ArrayObject>().getDenseElements();
        const Value* srcend = srcbeg + length;
        const Value* src = srcbeg;
        for (Value* dst = vp; src < srcend; ++dst, ++src)
            *dst = src->isMagic(JS_ELEMENTS_HOLE) ? UndefinedValue() : *src;
        return true;
    }

    if (aobj->is<ArgumentsObject>()) {
        ArgumentsObject& argsobj = aobj->as<ArgumentsObject>();
        if (!argsobj.hasOverriddenLength()) {
            if (argsobj.maybeGetElements(0, length, vp))
                return true;
        }
    }

    if (js::GetElementsOp op = aobj->getOps()->getElements) {
        ElementAdder adder(cx, vp, length, ElementAdder::GetElement);
        return op(cx, aobj, 0, length, &adder);
    }

    for (uint32_t i = 0; i < length; i++) {
        if (!GetElement(cx, aobj, aobj, i, MutableHandleValue::fromMarkedLocation(&vp[i])))
            return false;
    }

    return true;
}




static bool
SetArrayElement(JSContext* cx, HandleObject obj, double index, HandleValue v)
{
    MOZ_ASSERT(index >= 0);

    if (obj->is<ArrayObject>() && !obj->isIndexed()) {
        Rooted<ArrayObject*> arr(cx, &obj->as<ArrayObject>());
        
        NativeObject::EnsureDenseResult result = NativeObject::ED_SPARSE;
        do {
            if (index > uint32_t(-1))
                break;
            uint32_t idx = uint32_t(index);
            if (idx >= arr->length() && !arr->lengthIsWritable()) {
                JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, GetErrorMessage, nullptr,
                                             JSMSG_CANT_REDEFINE_ARRAY_LENGTH);
                return false;
            }
            result = arr->ensureDenseElements(cx, idx, 1);
            if (result != NativeObject::ED_OK)
                break;
            if (idx >= arr->length())
                arr->setLengthInt32(idx + 1);
            arr->setDenseElementWithType(cx, idx, v);
            return true;
        } while (false);

        if (result == NativeObject::ED_FAILED)
            return false;
        MOZ_ASSERT(result == NativeObject::ED_SPARSE);
    }

    RootedId id(cx);
    if (!ToId(cx, index, &id))
        return false;

    return SetProperty(cx, obj, id, v);
}













static bool
DeleteArrayElement(JSContext* cx, HandleObject obj, double index, ObjectOpResult& result)
{
    MOZ_ASSERT(index >= 0);
    MOZ_ASSERT(floor(index) == index);

    if (obj->is<ArrayObject>() && !obj->isIndexed()) {
        ArrayObject* aobj = &obj->as<ArrayObject>();
        if (index <= UINT32_MAX) {
            uint32_t idx = uint32_t(index);
            if (idx < aobj->getDenseInitializedLength()) {
                if (!aobj->maybeCopyElementsForWrite(cx))
                    return false;
                if (idx+1 == aobj->getDenseInitializedLength()) {
                    aobj->setDenseInitializedLength(idx);
                } else {
                    aobj->markDenseElementsNotPacked(cx);
                    aobj->setDenseElement(idx, MagicValue(JS_ELEMENTS_HOLE));
                }
                if (!SuppressDeletedElement(cx, obj, idx))
                    return false;
            }
        }

        return result.succeed();
    }

    RootedId id(cx);
    if (!ToId(cx, index, &id))
        return false;
    return DeleteProperty(cx, obj, id, result);
}


static bool
DeletePropertyOrThrow(JSContext* cx, HandleObject obj, double index)
{
    ObjectOpResult success;
    if (!DeleteArrayElement(cx, obj, index, success))
        return false;
    if (!success) {
        RootedId id(cx);
        RootedValue indexv(cx, NumberValue(index));
        if (!ValueToId<CanGC>(cx, indexv, &id))
            return false;
        return success.reportError(cx, obj, id);
    }
    return true;
}

bool
js::SetLengthProperty(JSContext* cx, HandleObject obj, double length)
{
    RootedValue v(cx, NumberValue(length));
    return SetProperty(cx, obj, cx->names().length, v);
}








static bool
array_length_getter(JSContext* cx, HandleObject obj_, HandleId id, MutableHandleValue vp)
{
    RootedObject obj(cx, obj_);
    do {
        if (obj->is<ArrayObject>()) {
            vp.setNumber(obj->as<ArrayObject>().length());
            return true;
        }
        if (!GetPrototype(cx, obj, &obj))
            return false;
    } while (obj);
    return true;
}

static bool
array_length_setter(JSContext* cx, HandleObject obj, HandleId id, MutableHandleValue vp,
                    ObjectOpResult& result)
{
    if (!obj->is<ArrayObject>()) {
        
        
        
        const Class* clasp = obj->getClass();
        return DefineProperty(cx, obj, cx->names().length, vp,
                              clasp->getProperty, clasp->setProperty, JSPROP_ENUMERATE, result);
    }

    Rooted<ArrayObject*> arr(cx, &obj->as<ArrayObject>());
    MOZ_ASSERT(arr->lengthIsWritable(),
               "setter shouldn't be called if property is non-writable");

    return ArraySetLength(cx, arr, id, JSPROP_PERMANENT, vp, result);
}

struct ReverseIndexComparator
{
    bool operator()(const uint32_t& a, const uint32_t& b, bool* lessOrEqualp) {
        MOZ_ASSERT(a != b, "how'd we get duplicate indexes?");
        *lessOrEqualp = b <= a;
        return true;
    }
};

bool
js::CanonicalizeArrayLengthValue(JSContext* cx, HandleValue v, uint32_t* newLen)
{
    double d;

    if (!ToUint32(cx, v, newLen))
        return false;

    if (!ToNumber(cx, v, &d))
        return false;

    if (d == *newLen)
        return true;

    JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_BAD_ARRAY_LENGTH);
    return false;
}


bool
js::ArraySetLength(JSContext* cx, Handle<ArrayObject*> arr, HandleId id,
                   unsigned attrs, HandleValue value, ObjectOpResult& result)
{
    MOZ_ASSERT(id == NameToId(cx->names().length));

    if (!arr->maybeCopyElementsForWrite(cx))
        return false;

    

    
    uint32_t newLen;
    if (!CanonicalizeArrayLengthValue(cx, value, &newLen))
        return false;

    
    
    
    
    
    
    
    
    
    if (!(attrs & JSPROP_PERMANENT) || (attrs & JSPROP_ENUMERATE))
        return result.fail(JSMSG_CANT_REDEFINE_PROP);

    
    bool lengthIsWritable = arr->lengthIsWritable();
#ifdef DEBUG
    {
        RootedShape lengthShape(cx, arr->lookupPure(id));
        MOZ_ASSERT(lengthShape);
        MOZ_ASSERT(lengthShape->writable() == lengthIsWritable);
    }
#endif

    uint32_t oldLen = arr->length();

    
    if (!lengthIsWritable) {
        if (newLen == oldLen)
            return result.succeed();

        return result.fail(JSMSG_CANT_REDEFINE_ARRAY_LENGTH);
    }

    
    bool succeeded = true;
    do {
        
        
        
        if (newLen >= oldLen)
            break;

        
        
        
        
        
        
        
        
        
        if (!arr->isIndexed()) {
            if (!arr->maybeCopyElementsForWrite(cx))
                return false;

            uint32_t oldCapacity = arr->getDenseCapacity();
            uint32_t oldInitializedLength = arr->getDenseInitializedLength();
            MOZ_ASSERT(oldCapacity >= oldInitializedLength);
            if (oldInitializedLength > newLen)
                arr->setDenseInitializedLength(newLen);
            if (oldCapacity > newLen)
                arr->shrinkElements(cx, newLen);

            
            
            
            break;
        }

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        uint32_t gap = oldLen - newLen;
        const uint32_t RemoveElementsFastLimit = 1 << 24;
        if (gap < RemoveElementsFastLimit) {
            
            
            while (newLen < oldLen) {
                
                oldLen--;

                
                ObjectOpResult deleteSucceeded;
                if (!DeleteElement(cx, arr, oldLen, deleteSucceeded))
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
                if (!GetPropertyKeys(cx, arr, JSITER_OWNONLY | JSITER_HIDDEN, &props))
                    return false;

                for (size_t i = 0; i < props.length(); i++) {
                    if (!CheckForInterrupt(cx))
                        return false;

                    uint32_t index;
                    if (!IdIsIndex(props[i], &index))
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

                
                ObjectOpResult deleteSucceeded;
                if (!DeleteElement(cx, arr, index, deleteSucceeded))
                    return false;
                if (!deleteSucceeded) {
                    newLen = index + 1;
                    succeeded = false;
                    break;
                }
            }
        }
    } while (false);

    

    
    
    
    
    RootedShape lengthShape(cx, arr->lookup(cx, id));
    if (!NativeObject::changeProperty(cx, arr, lengthShape, attrs,
                                      JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_SHARED,
                                      array_length_getter, array_length_setter))
    {
        return false;
    }

    arr->setLength(cx, newLen);

    
    

    
    
    
    ObjectElements* header = arr->getElementsHeader();
    header->initializedLength = Min(header->initializedLength, newLen);

    if (attrs & JSPROP_READONLY) {
        header->setNonwritableArrayLength();

        
        
        
        
        
        
        if (arr->getDenseCapacity() > newLen) {
            arr->shrinkElements(cx, newLen);
            arr->getElementsHeader()->capacity = newLen;
        }
    }

    if (!succeeded)
        return result.fail(JSMSG_CANT_TRUNCATE_ARRAY);

    return result.succeed();
}

bool
js::WouldDefinePastNonwritableLength(HandleNativeObject obj, uint32_t index)
{
    if (!obj->is<ArrayObject>())
        return false;

    ArrayObject* arr = &obj->as<ArrayObject>();
    return !arr->lengthIsWritable() && index >= arr->length();
}

static bool
array_addProperty(JSContext* cx, HandleObject obj, HandleId id, HandleValue v)
{
    Rooted<ArrayObject*> arr(cx, &obj->as<ArrayObject>());

    uint32_t index;
    if (!IdIsIndex(id, &index))
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
js::ObjectMayHaveExtraIndexedProperties(JSObject* obj)
{
    





    MOZ_ASSERT(obj->isNative());

    if (obj->isIndexed())
        return true;

    




    while ((obj = obj->getProto()) != nullptr) {
        




        if (!obj->isNative())
            return true;
        if (obj->isIndexed())
            return true;
        if (obj->as<NativeObject>().getDenseInitializedLength() > 0)
            return true;
        if (IsAnyTypedArray(obj))
            return true;
    }

    return false;
}

static bool
AddLengthProperty(ExclusiveContext* cx, HandleArrayObject obj)
{
    






    RootedId lengthId(cx, NameToId(cx->names().length));
    MOZ_ASSERT(!obj->lookup(cx, lengthId));

    return NativeObject::addProperty(cx, obj, lengthId, array_length_getter, array_length_setter,
                                     SHAPE_INVALID_SLOT, JSPROP_PERMANENT | JSPROP_SHARED, 0,
                                      false);
}

#if JS_HAS_TOSOURCE

static bool
array_toSource(JSContext* cx, unsigned argc, Value* vp)
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

        
        JSString* str;
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
    JSString* str = sb.finishString();
    if (!str)
        return false;

    args.rval().setString(str);
    return true;
}

#endif

struct EmptySeparatorOp
{
    bool operator()(JSContext*, StringBuffer& sb) { return true; }
};

template <typename CharT>
struct CharSeparatorOp
{
    const CharT sep;
    explicit CharSeparatorOp(CharT sep) : sep(sep) {}
    bool operator()(JSContext*, StringBuffer& sb) { return sb.append(sep); }
};

struct StringSeparatorOp
{
    HandleLinearString sep;

    explicit StringSeparatorOp(HandleLinearString sep) : sep(sep) {}

    bool operator()(JSContext* cx, StringBuffer& sb) {
        return sb.append(sep);
    }
};

template <bool Locale, typename SeparatorOp>
static bool
ArrayJoinKernel(JSContext* cx, SeparatorOp sepOp, HandleObject obj, uint32_t length,
               StringBuffer& sb)
{
    uint32_t i = 0;

    if (!Locale && obj->is<ArrayObject>() && !ObjectMayHaveExtraIndexedProperties(obj)) {
        
        
        
        uint32_t initLength = obj->as<ArrayObject>().getDenseInitializedLength();
        while (i < initLength) {
            if (!CheckForInterrupt(cx))
                return false;

            const Value& elem = obj->as<ArrayObject>().getDenseElement(i);

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
                MOZ_ASSERT(elem.isMagic(JS_ELEMENTS_HOLE) || elem.isNullOrUndefined());
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
                    JSObject* robj = ToObject(cx, v);
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
JSString*
js::ArrayJoin(JSContext* cx, HandleObject obj, HandleLinearString sepstr, uint32_t length)
{
    
    
    

    

    

    
    
    
    if (length == 1 && !Locale && obj->is<ArrayObject>() &&
        obj->as<ArrayObject>().getDenseInitializedLength() == 1)
    {
        const Value& elem0 = obj->as<ArrayObject>().getDenseElement(0);
        if (elem0.isString()) {
            return elem0.toString();
        }
    }

    StringBuffer sb(cx);
    if (sepstr->hasTwoByteChars() && !sb.ensureTwoByteChars())
        return nullptr;

    
    
    size_t seplen = sepstr->length();
    CheckedInt<uint32_t> res = CheckedInt<uint32_t>(seplen) * (length - 1);
    if (length > 0 && !res.isValid()) {
        ReportAllocationOverflow(cx);
        return nullptr;
    }

    if (length > 0 && !sb.reserve(res.value()))
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

    
    JSString* str = sb.finishString();
    if (!str)
        return nullptr;
    return str;
}

template <bool Locale>
bool
ArrayJoin(JSContext* cx, CallArgs& args)
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
        JSString* s = ToString<CanGC>(cx, args[0]);
        if (!s)
            return false;
        sepstr = s->ensureLinear(cx);
        if (!sepstr)
            return false;
    } else {
        sepstr = cx->names().comma;
    }

    
    JSString* res = js::ArrayJoin<Locale>(cx, obj, sepstr, length);
    if (!res)
        return false;

    args.rval().setString(res);
    return true;
}


static bool
array_toString(JSContext* cx, unsigned argc, Value* vp)
{
    JS_CHECK_RECURSION(cx, return false);

    CallArgs args = CallArgsFromVp(argc, vp);
    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    RootedValue join(cx, args.calleev());
    if (!GetProperty(cx, obj, obj, cx->names().join, &join))
        return false;

    if (!IsCallable(join)) {
        JSString* str = JS_BasicObjectToString(cx, obj);
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
array_toLocaleString(JSContext* cx, unsigned argc, Value* vp)
{
    JS_CHECK_RECURSION(cx, return false);

    CallArgs args = CallArgsFromVp(argc, vp);
    return ArrayJoin<true>(cx, args);
}


bool
js::array_join(JSContext* cx, unsigned argc, Value* vp)
{
    JS_CHECK_RECURSION(cx, return false);

    CallArgs args = CallArgsFromVp(argc, vp);
    return ArrayJoin<false>(cx, args);
}

static inline bool
InitArrayTypes(JSContext* cx, ObjectGroup* group, JSObject* obj,
               const Value* vector, unsigned count)
{
    if (!group->unknownProperties()) {
        AutoEnterAnalysis enter(cx);

        HeapTypeSet* types = group->getProperty(cx, obj, JSID_VOID);
        if (!types)
            return false;

        for (unsigned i = 0; i < count; i++) {
            if (vector[i].isMagic(JS_ELEMENTS_HOLE))
                continue;
            types->addType(cx, TypeSet::GetValueType(vector[i]));
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
InitArrayElements(JSContext* cx, HandleObject obj, uint32_t start, uint32_t count, const Value* vector, ShouldUpdateTypes updateTypes)
{
    MOZ_ASSERT(count <= MAX_ARRAY_INDEX);

    if (count == 0)
        return true;

    ObjectGroup* group = obj->getGroup(cx);
    if (!group)
        return false;
    if (updateTypes && !InitArrayTypes(cx, group, obj, vector, count))
        return false;

    




    do {
        if (!obj->is<ArrayObject>())
            break;
        if (ObjectMayHaveExtraIndexedProperties(obj))
            break;

        HandleArrayObject arr = obj.as<ArrayObject>();

        if (arr->shouldConvertDoubleElements())
            break;

        if (!arr->lengthIsWritable() && start + count > arr->length())
            break;

        NativeObject::EnsureDenseResult result = arr->ensureDenseElements(cx, start, count);
        if (result != NativeObject::ED_OK) {
            if (result == NativeObject::ED_FAILED)
                return false;
            MOZ_ASSERT(result == NativeObject::ED_SPARSE);
            break;
        }

        uint32_t newlen = start + count;
        if (newlen > arr->length())
            arr->setLengthInt32(newlen);

        MOZ_ASSERT(count < UINT32_MAX / sizeof(Value));
        arr->copyDenseElements(start, vector, count);
        MOZ_ASSERT_IF(count != 0, !arr->getDenseElement(newlen - 1).isMagic(JS_ELEMENTS_HOLE));
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

    MOZ_ASSERT(start == MAX_ARRAY_INDEX + 1);
    RootedValue value(cx);
    RootedId id(cx);
    RootedValue indexv(cx);
    double index = MAX_ARRAY_INDEX + 1;
    do {
        value = *vector++;
        indexv = DoubleValue(index);
        if (!ValueToId<CanGC>(cx, indexv, &id))
            return false;
        if (!SetProperty(cx, obj, id, value))
            return false;
        index += 1;
    } while (vector != end);

    return true;
}

static bool
array_reverse(JSContext* cx, unsigned argc, Value* vp)
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

        HandleArrayObject arr = obj.as<ArrayObject>();

        
        if (len == 0 || arr->getDenseCapacity() == 0) {
            args.rval().setObject(*obj);
            return true;
        }

        








        NativeObject::EnsureDenseResult result =
            arr->ensureDenseElements(cx, len, 0);
        if (result != NativeObject::ED_OK) {
            if (result == NativeObject::ED_FAILED)
                return false;
            MOZ_ASSERT(result == NativeObject::ED_SPARSE);
            break;
        }

        
        arr->ensureDenseInitializedLength(cx, len, 0);

        RootedValue origlo(cx), orighi(cx);

        uint32_t lo = 0, hi = len - 1;
        for (; lo < hi; lo++, hi--) {
            origlo = arr->getDenseElement(lo);
            orighi = arr->getDenseElement(hi);
            arr->setDenseElement(lo, orighi);
            if (orighi.isMagic(JS_ELEMENTS_HOLE) &&
                !SuppressDeletedProperty(cx, arr, INT_TO_JSID(lo)))
            {
                return false;
            }
            arr->setDenseElement(hi, origlo);
            if (origlo.isMagic(JS_ELEMENTS_HOLE) &&
                !SuppressDeletedProperty(cx, arr, INT_TO_JSID(hi)))
            {
                return false;
            }
        }

        




        args.rval().setObject(*arr);
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
CompareStringValues(JSContext* cx, const Value& a, const Value& b, bool* lessOrEqualp)
{
    if (!CheckForInterrupt(cx))
        return false;

    JSString* astr = a.toString();
    JSString* bstr = b.toString();
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
CompareLexicographicInt32(const Value& a, const Value& b, bool* lessOrEqualp)
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
            MOZ_ASSERT((digitsa - digitsb) < ArrayLength(powersOf10));
            *lessOrEqualp = (uint64_t(auint) < uint64_t(buint) * powersOf10[digitsa - digitsb]);
        } else { 
            MOZ_ASSERT((digitsb - digitsa) < ArrayLength(powersOf10));
            *lessOrEqualp = (uint64_t(auint) * powersOf10[digitsb - digitsa] <= uint64_t(buint));
        }
    }

    return true;
}

template <typename Char1, typename Char2>
static inline bool
CompareSubStringValues(JSContext* cx, const Char1* s1, size_t len1, const Char2* s2, size_t len2,
                       bool* lessOrEqualp)
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
    JSContext*  const cx;

    explicit SortComparatorStrings(JSContext* cx)
      : cx(cx) {}

    bool operator()(const Value& a, const Value& b, bool* lessOrEqualp) {
        return CompareStringValues(cx, a, b, lessOrEqualp);
    }
};

struct SortComparatorLexicographicInt32
{
    bool operator()(const Value& a, const Value& b, bool* lessOrEqualp) {
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
    JSContext*          const cx;
    const StringBuffer& sb;

    SortComparatorStringifiedElements(JSContext* cx, const StringBuffer& sb)
      : cx(cx), sb(sb) {}

    bool operator()(const StringifiedElement& a, const StringifiedElement& b, bool* lessOrEqualp) {
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
    JSContext*         const cx;
    const Value&       fval;
    FastInvokeGuard&   fig;

    SortComparatorFunction(JSContext* cx, const Value& fval, FastInvokeGuard& fig)
      : cx(cx), fval(fval), fig(fig) { }

    bool operator()(const Value& a, const Value& b, bool* lessOrEqualp);
};

bool
SortComparatorFunction::operator()(const Value& a, const Value& b, bool* lessOrEqualp)
{
    



    MOZ_ASSERT(!a.isMagic() && !a.isUndefined());
    MOZ_ASSERT(!a.isMagic() && !b.isUndefined());

    if (!CheckForInterrupt(cx))
        return false;

    InvokeArgs& args = fig.args();
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
ComparatorNumericLeftMinusRight(const NumericElement& a, const NumericElement& b,
                                bool* lessOrEqualp)
{
    *lessOrEqualp = (a.dv <= b.dv);
    return true;
}

static bool
ComparatorNumericRightMinusLeft(const NumericElement& a, const NumericElement& b,
                                bool* lessOrEqualp)
{
    *lessOrEqualp = (b.dv <= a.dv);
    return true;
}

typedef bool (*ComparatorNumeric)(const NumericElement& a, const NumericElement& b,
                                  bool* lessOrEqualp);

static const ComparatorNumeric SortComparatorNumerics[] = {
    nullptr,
    nullptr,
    ComparatorNumericLeftMinusRight,
    ComparatorNumericRightMinusLeft
};

static bool
ComparatorInt32LeftMinusRight(const Value& a, const Value& b, bool* lessOrEqualp)
{
    *lessOrEqualp = (a.toInt32() <= b.toInt32());
    return true;
}

static bool
ComparatorInt32RightMinusLeft(const Value& a, const Value& b, bool* lessOrEqualp)
{
    *lessOrEqualp = (b.toInt32() <= a.toInt32());
    return true;
}

typedef bool (*ComparatorInt32)(const Value& a, const Value& b, bool* lessOrEqualp);

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
MatchNumericComparator(JSContext* cx, const Value& v)
{
    if (!v.isObject())
        return Match_None;

    JSObject& obj = v.toObject();
    if (!obj.is<JSFunction>())
        return Match_None;

    JSFunction* fun = &obj.as<JSFunction>();
    if (!fun->isInterpreted())
        return Match_None;

    JSScript* script = fun->getOrCreateScript(cx);
    if (!script)
        return Match_Failure;

    jsbytecode* pc = script->code();

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
MergeSortByKey(K keys, size_t len, K scratch, C comparator, AutoValueVector* vec)
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
SortLexicographically(JSContext* cx, AutoValueVector* vec, size_t len)
{
    MOZ_ASSERT(vec->length() >= len);

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
SortNumerically(JSContext* cx, AutoValueVector* vec, size_t len, ComparatorMatchResult comp)
{
    MOZ_ASSERT(vec->length() >= len);

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
js::array_sort(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedValue fvalRoot(cx);
    Value& fval = fvalRoot.get();

    if (args.hasDefined(0)) {
        if (args[0].isPrimitive()) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_BAD_SORT_ARG);
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
        ReportAllocationOverflow(cx);
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
js::NewbornArrayPush(JSContext* cx, HandleObject obj, const Value& v)
{
    Rooted<ArrayObject*> arr(cx, &obj->as<ArrayObject>());

    MOZ_ASSERT(!v.isMagic());
    MOZ_ASSERT(arr->lengthIsWritable());

    uint32_t length = arr->length();
    MOZ_ASSERT(length <= arr->getDenseCapacity());

    if (!arr->ensureElements(cx, length + 1))
        return false;

    arr->setDenseInitializedLength(length + 1);
    arr->setLengthInt32(length + 1);
    arr->initDenseElementWithType(cx, length, v);
    return true;
}


bool
js::array_push(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    
    uint32_t length;
    if (!GetLengthProperty(cx, obj, &length))
        return false;

    
    do {
        if (!obj->isNative() || IsAnyTypedArray(obj.get()))
            break;

        if (obj->is<ArrayObject>() && !obj->as<ArrayObject>().lengthIsWritable())
            break;

        if (ObjectMayHaveExtraIndexedProperties(obj))
            break;

        uint32_t argCount = args.length();
        NativeObject::EnsureDenseResult result =
            obj->as<NativeObject>().ensureDenseElements(cx, length, argCount);
        if (result == NativeObject::ED_FAILED)
            return false;

        if (result == NativeObject::ED_OK) {
            for (uint32_t i = 0, index = length; i < argCount; index++, i++)
                obj->as<NativeObject>().setDenseElementWithType(cx, index, args[i]);
            uint32_t newlength = length + argCount;
            args.rval().setNumber(newlength);
            if (obj->is<ArrayObject>()) {
                obj->as<ArrayObject>().setLengthInt32(newlength);
                return true;
            }
            return SetLengthProperty(cx, obj, newlength);
        }

        MOZ_ASSERT(result == NativeObject::ED_SPARSE);
    } while (false);

    
    if (!InitArrayElements(cx, obj, length, args.length(), args.array(), UpdateTypes))
        return false;

    
    double newlength = length + double(args.length());
    args.rval().setNumber(newlength);
    return SetLengthProperty(cx, obj, newlength);
}


bool
js::array_pop(JSContext* cx, unsigned argc, Value* vp)
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
js::ArrayShiftMoveElements(ArrayObject* obj)
{
    MOZ_ASSERT(obj->lengthIsWritable());

    




    uint32_t initlen = obj->getDenseInitializedLength();
    obj->moveDenseElementsNoPreBarrier(0, 1, initlen);
}


bool
js::array_shift(JSContext* cx, unsigned argc, Value* vp)
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

    
    if (obj->is<ArrayObject>()) {
        ArrayObject* aobj = &obj->as<ArrayObject>();
        if (aobj->getDenseInitializedLength() > 0 &&
            newlen < aobj->getDenseCapacity() &&
            !ObjectMayHaveExtraIndexedProperties(aobj))
        {
            args.rval().set(aobj->getDenseElement(0));
            if (args.rval().isMagic(JS_ELEMENTS_HOLE))
                args.rval().setUndefined();

            if (!aobj->maybeCopyElementsForWrite(cx))
                return false;

            aobj->moveDenseElements(0, 1, aobj->getDenseInitializedLength() - 1);
            aobj->setDenseInitializedLength(aobj->getDenseInitializedLength() - 1);

            if (!SetLengthProperty(cx, obj, newlen))
                return false;

            return SuppressDeletedProperty(cx, obj, INT_TO_JSID(newlen));
        }
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

bool
js::array_unshift(JSContext* cx, unsigned argc, Value* vp)
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
                ArrayObject* aobj = &obj->as<ArrayObject>();
                if (!aobj->lengthIsWritable())
                    break;
                NativeObject::EnsureDenseResult result =
                    aobj->ensureDenseElements(cx, length, args.length());
                if (result != NativeObject::ED_OK) {
                    if (result == NativeObject::ED_FAILED)
                        return false;
                    MOZ_ASSERT(result == NativeObject::ED_SPARSE);
                    break;
                }
                aobj->moveDenseElements(args.length(), 0, length);
                for (uint32_t i = 0; i < args.length(); i++)
                    aobj->setDenseElement(i, MagicValue(JS_ELEMENTS_HOLE));
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
TryReuseArrayGroup(JSObject* obj, ArrayObject* narr)
{
    




    MOZ_ASSERT(ObjectGroup::hasDefaultNewGroup(narr->getProto(), &ArrayObject::class_, narr->group()));

    if (obj->is<ArrayObject>() && !obj->isSingleton() && obj->getProto() == narr->getProto())
        narr->setGroup(obj->group());
}








static inline bool
CanOptimizeForDenseStorage(HandleObject arr, uint32_t startingIndex, uint32_t count, JSContext* cx)
{
    
    if (UINT32_MAX - startingIndex < count)
        return false;

    
    if (!arr->is<ArrayObject>())
        return false;

    












    ObjectGroup* arrGroup = arr->getGroup(cx);
    if (MOZ_UNLIKELY(!arrGroup || arrGroup->hasAllFlags(OBJECT_FLAG_ITERATED)))
        return false;

    




    return !ObjectMayHaveExtraIndexedProperties(arr) &&
           startingIndex + count <= arr->as<ArrayObject>().getDenseInitializedLength();
}


bool
js::array_splice(JSContext* cx, unsigned argc, Value* vp)
{
    return array_splice_impl(cx, argc, vp, true);
}

bool
js::array_splice_impl(JSContext* cx, unsigned argc, Value* vp, bool returnValueIsUsed)
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

    MOZ_ASSERT(len - actualStart >= actualDeleteCount);

    
    Rooted<ArrayObject*> arr(cx);
    if (CanOptimizeForDenseStorage(obj, actualStart, actualDeleteCount, cx)) {
        if (returnValueIsUsed) {
            arr = NewDenseCopiedArray(cx, actualDeleteCount, obj.as<ArrayObject>(), actualStart);
            if (!arr)
                return false;
            TryReuseArrayGroup(obj, arr);
        }
    } else {
        arr = NewDenseFullyAllocatedArray(cx, actualDeleteCount);
        if (!arr)
            return false;
        TryReuseArrayGroup(obj, arr);

        RootedValue fromValue(cx);
        for (uint32_t k = 0; k < actualDeleteCount; k++) {
            bool hole;
            if (!CheckForInterrupt(cx) ||
                !GetElement(cx, obj, actualStart + k, &hole, &fromValue) ||
                (!hole && !DefineElement(cx, arr, k, fromValue)))
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
            ArrayObject* aobj = &obj->as<ArrayObject>();

            if (!aobj->maybeCopyElementsForWrite(cx))
                return false;

            
            aobj->moveDenseElements(targetIndex, sourceIndex, len - sourceIndex);

            



            aobj->setDenseInitializedLength(finalLength);

            
            aobj->shrinkElements(cx, finalLength);
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
                NativeObject::EnsureDenseResult res =
                    arr->ensureDenseElements(cx, arr->length(), itemCount - actualDeleteCount);
                if (res == NativeObject::ED_FAILED)
                    return false;
            }
        }

        if (CanOptimizeForDenseStorage(obj, len, itemCount - actualDeleteCount, cx)) {
            ArrayObject* aobj = &obj->as<ArrayObject>();
            if (!aobj->maybeCopyElementsForWrite(cx))
                return false;
            aobj->moveDenseElements(actualStart + itemCount,
                                    actualStart + actualDeleteCount,
                                    len - (actualStart + actualDeleteCount));
            aobj->setDenseInitializedLength(len + itemCount - actualDeleteCount);
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

    
    Value* items = args.array() + 2;

    
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
js::array_concat_dense(JSContext* cx, Handle<ArrayObject*> arr1, Handle<ArrayObject*> arr2,
                       Handle<ArrayObject*> result)
{
    uint32_t initlen1 = arr1->getDenseInitializedLength();
    MOZ_ASSERT(initlen1 == arr1->length());

    uint32_t initlen2 = arr2->getDenseInitializedLength();
    MOZ_ASSERT(initlen2 == arr2->length());

    
    uint32_t len = initlen1 + initlen2;

    if (!result->ensureElements(cx, len))
        return false;

    MOZ_ASSERT(!result->getDenseInitializedLength());
    result->setDenseInitializedLength(len);

    result->initDenseElements(0, arr1->getDenseElements(), initlen1);
    result->initDenseElements(initlen1, arr2->getDenseElements(), initlen2);
    result->setLengthInt32(len);
    return true;
}




bool
js::array_concat(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    Value* p = args.array() - 1;

    
    RootedObject aobj(cx, ToObject(cx, args.thisv()));
    if (!aobj)
        return false;

    Rooted<ArrayObject*> narr(cx);
    uint32_t length;
    if (aobj->is<ArrayObject>() && !aobj->isIndexed()) {
        length = aobj->as<ArrayObject>().length();
        uint32_t initlen = aobj->as<ArrayObject>().getDenseInitializedLength();
        narr = NewDenseCopiedArray(cx, initlen, aobj.as<ArrayObject>(), 0);
        if (!narr)
            return false;
        TryReuseArrayGroup(aobj, narr);
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
            
            if (IsArray(obj, cx)) {
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

struct SortComparatorIndexes
{
    bool operator()(uint32_t a, uint32_t b, bool* lessOrEqualp) {
        *lessOrEqualp = (a <= b);
        return true;
    }
};





static bool
GetIndexedPropertiesInRange(JSContext* cx, HandleObject obj, uint32_t begin, uint32_t end,
                            Vector<uint32_t>& indexes, bool* success)
{
    *success = false;

    
    
    JSObject* pobj = obj;
    do {
        if (!pobj->isNative() || pobj->getClass()->resolve || pobj->getOps()->lookupProperty)
            return true;
    } while ((pobj = pobj->getProto()));

    
    pobj = obj;
    do {
        
        NativeObject* nativeObj = &pobj->as<NativeObject>();
        uint32_t initLen = nativeObj->getDenseInitializedLength();
        for (uint32_t i = begin; i < initLen && i < end; i++) {
            if (nativeObj->getDenseElement(i).isMagic(JS_ELEMENTS_HOLE))
                continue;
            if (!indexes.append(i))
                return false;
        }

        
        if (IsAnyTypedArray(pobj)) {
            uint32_t len = AnyTypedArrayLength(pobj);
            for (uint32_t i = begin; i < len && i < end; i++) {
                if (!indexes.append(i))
                    return false;
            }
        }

        
        if (pobj->isIndexed()) {
            Shape::Range<NoGC> r(pobj->as<NativeObject>().lastProperty());
            for (; !r.empty(); r.popFront()) {
                Shape& shape = r.front();
                jsid id = shape.propid();
                if (!JSID_IS_INT(id))
                    continue;

                uint32_t i = uint32_t(JSID_TO_INT(id));
                if (!(begin <= i && i < end))
                    continue;

                
                if (!shape.hasDefaultGetter())
                    return true;

                if (!indexes.append(i))
                    return false;
            }
        }
    } while ((pobj = pobj->getProto()));

    
    Vector<uint32_t> tmp(cx);
    size_t n = indexes.length();
    if (!tmp.resize(n))
        return false;
    if (!MergeSort(indexes.begin(), n, tmp.begin(), SortComparatorIndexes()))
        return false;

    
    if (!indexes.empty()) {
        uint32_t last = 0;
        for (size_t i = 1, len = indexes.length(); i < len; i++) {
            uint32_t elem = indexes[i];
            if (indexes[last] != elem) {
                last++;
                indexes[last] = elem;
            }
        }
        if (!indexes.resize(last + 1))
            return false;
    }

    *success = true;
    return true;
}

static bool
SliceSlowly(JSContext* cx, HandleObject obj, HandleObject receiver,
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
        if (!hole && !DefineElement(cx, result, slot - begin, value))
            return false;
    }
    return true;
}

static bool
SliceSparse(JSContext* cx, HandleObject obj, uint32_t begin, uint32_t end, HandleObject result)
{
    MOZ_ASSERT(begin <= end);

    Vector<uint32_t> indexes(cx);
    bool success;
    if (!GetIndexedPropertiesInRange(cx, obj, begin, end, indexes, &success))
        return false;

    if (!success)
        return SliceSlowly(cx, obj, obj, begin, end, result);

    RootedValue value(cx);
    for (size_t i = 0, len = indexes.length(); i < len; i++) {
        uint32_t index = indexes[i];
        MOZ_ASSERT(begin <= index && index < end);

        bool hole;
        if (!GetElement(cx, obj, obj, index, &hole, &value))
            return false;

        if (!hole && !DefineElement(cx, result, index - begin, value))
            return false;
    }

    return true;
}

bool
js::array_slice(JSContext* cx, unsigned argc, Value* vp)
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

    if (obj->is<ArrayObject>() && !ObjectMayHaveExtraIndexedProperties(obj)) {
        narr = NewDenseFullyAllocatedArray(cx, end - begin);
        if (!narr)
            return false;
        TryReuseArrayGroup(obj, narr);

        ArrayObject* aobj = &obj->as<ArrayObject>();
        if (aobj->getDenseInitializedLength() > begin) {
            uint32_t numSourceElements = aobj->getDenseInitializedLength() - begin;
            uint32_t initLength = Min(numSourceElements, end - begin);
            narr->setDenseInitializedLength(initLength);
            narr->initDenseElements(0, &aobj->getDenseElement(begin), initLength);
        }
        args.rval().setObject(*narr);
        return true;
    }

    narr = NewDensePartlyAllocatedArray(cx, end - begin);
    if (!narr)
        return false;
    TryReuseArrayGroup(obj, narr);

    if (js::GetElementsOp op = obj->getOps()->getElements) {
        
        
        NativeObject::EnsureDenseResult result = narr->ensureDenseElements(cx, 0, end - begin);
        if (result == NativeObject::ED_FAILED)
             return false;

        if (result == NativeObject::ED_OK) {
            ElementAdder adder(cx, narr, end - begin, ElementAdder::CheckHasElemPreserveHoles);
            if (!op(cx, obj, begin, end, &adder))
                return false;

            args.rval().setObject(*narr);
            return true;
        }

        
        MOZ_ASSERT(result == NativeObject::ED_SPARSE);
    }

    if (obj->isNative() && obj->isIndexed() && end - begin > 1000) {
        if (!SliceSparse(cx, obj, begin, end, narr))
            return false;
    } else {
        if (!SliceSlowly(cx, obj, obj, begin, end, narr))
            return false;
    }

    args.rval().setObject(*narr);
    return true;
}


static bool
array_filter(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    
    uint32_t len;
    if (!GetLengthProperty(cx, obj, &len))
        return false;

    
    if (args.length() == 0) {
        ReportMissingArg(cx, args.calleev(), 0);
        return false;
    }
    RootedObject callable(cx, ValueToCallable(cx, args[0], args.length() - 1));
    if (!callable)
        return false;

    
    RootedValue thisv(cx, args.length() >= 2 ? args[1] : UndefinedValue());

    
    RootedObject arr(cx, NewDenseFullyAllocatedArray(cx, 0));
    if (!arr)
        return false;
    ObjectGroup* newGroup = ObjectGroup::callingAllocationSiteGroup(cx, JSProto_Array);
    if (!newGroup)
        return false;
    arr->setGroup(newGroup);

    
    uint32_t k = 0;

    
    uint32_t to = 0;

    
    FastInvokeGuard fig(cx, ObjectValue(*callable));
    InvokeArgs& args2 = fig.args();
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
array_isArray(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    bool isArray = false;
    if (args.get(0).isObject()) {
        RootedObject obj(cx, &args[0].toObject());
        isArray = IsArray(obj, cx);
    }
    args.rval().setBoolean(isArray);
    return true;
}

static bool
IsArrayConstructor(const Value& v)
{
    
    
    
    return v.isObject() &&
           v.toObject().is<JSFunction>() &&
           v.toObject().as<JSFunction>().isNative() &&
           v.toObject().as<JSFunction>().native() == ArrayConstructor;
}

static bool
ArrayFromCallArgs(JSContext* cx, HandleObjectGroup group, CallArgs& args)
{
    if (!InitArrayTypes(cx, group, nullptr, args.array(), args.length()))
        return false;
    JSObject* obj = (args.length() == 0)
        ? NewDenseEmptyArray(cx)
        : NewDenseCopiedArray(cx, args.length(), args.array());
    if (!obj)
        return false;
    obj->setGroup(group);
    args.rval().setObject(*obj);
    return true;
}

static bool
array_of(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (IsArrayConstructor(args.thisv()) || !IsConstructor(args.thisv())) {
        
        
        RootedObjectGroup group(cx, ObjectGroup::callingAllocationSiteGroup(cx, JSProto_Array));
        if (!group)
            return false;
        return ArrayFromCallArgs(cx, group, args);
    }

    
    RootedObject obj(cx);
    {
        RootedValue v(cx);
        Value argv[1] = {NumberValue(args.length())};
        if (!InvokeConstructor(cx, args.thisv(), 1, argv, &v))
            return false;
        obj = ToObject(cx, v);
        if (!obj)
            return false;
    }

    
    for (unsigned k = 0; k < args.length(); k++) {
        if (!DefineElement(cx, obj, k, args[k]))
            return false;
    }

    
    if (!SetLengthProperty(cx, obj, args.length()))
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

    
    JS_FN("join",               array_join,         1,JSFUN_GENERIC_NATIVE),
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

    
    JS_SELF_HOSTED_FN("find",        "ArrayFind",        1,0),
    JS_SELF_HOSTED_FN("findIndex",   "ArrayFindIndex",   1,0),
    JS_SELF_HOSTED_FN("copyWithin",  "ArrayCopyWithin",  3,0),

    JS_SELF_HOSTED_FN("fill",        "ArrayFill",        3,0),

    JS_SELF_HOSTED_SYM_FN(iterator,  "ArrayValues",      0,0),
    JS_SELF_HOSTED_FN("entries",     "ArrayEntries",     0,0),
    JS_SELF_HOSTED_FN("keys",        "ArrayKeys",        0,0),

    
#ifdef NIGHTLY_BUILD
    JS_SELF_HOSTED_FN("includes",    "ArrayIncludes",    2,0),
#endif

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

    JS_FS_END
};


bool
js::ArrayConstructor(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedObjectGroup group(cx, ObjectGroup::callingAllocationSiteGroup(cx, JSProto_Array));
    if (!group)
        return false;

    if (args.length() != 1 || !args[0].isNumber())
        return ArrayFromCallArgs(cx, group, args);

    uint32_t length;
    if (args[0].isInt32()) {
        int32_t i = args[0].toInt32();
        if (i < 0) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_BAD_ARRAY_LENGTH);
            return false;
        }
        length = uint32_t(i);
    } else {
        double d = args[0].toDouble();
        length = ToUint32(d);
        if (d != double(length)) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_BAD_ARRAY_LENGTH);
            return false;
        }
    }

    



    AllocatingBehaviour allocating = (length <= ArrayObject::EagerAllocationMaxLength)
                                   ? NewArray_FullyAllocating
                                   : NewArray_PartlyAllocating;
    RootedObject obj(cx, NewDenseArray(cx, length, group, allocating));
    if (!obj)
        return false;

    args.rval().setObject(*obj);
    return true;
}

ArrayObject*
js::ArrayConstructorOneArg(JSContext* cx, HandleObjectGroup group, int32_t lengthInt)
{
    if (lengthInt < 0) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_BAD_ARRAY_LENGTH);
        return nullptr;
    }

    uint32_t length = uint32_t(lengthInt);
    AllocatingBehaviour allocating = (length <= ArrayObject::EagerAllocationMaxLength)
                                   ? NewArray_FullyAllocating
                                   : NewArray_PartlyAllocating;
    return NewDenseArray(cx, length, group, allocating);
}

static JSObject*
CreateArrayPrototype(JSContext* cx, JSProtoKey key)
{
    MOZ_ASSERT(key == JSProto_Array);
    RootedObject proto(cx, cx->global()->getOrCreateObjectPrototype(cx));
    if (!proto)
        return nullptr;

    RootedObjectGroup group(cx, ObjectGroup::defaultNewGroup(cx, &ArrayObject::class_,
                                                             TaggedProto(proto)));
    if (!group)
        return nullptr;

    RootedShape shape(cx, EmptyShape::getInitialShape(cx, &ArrayObject::class_, TaggedProto(proto),
                                                      gc::AllocKind::OBJECT0));
    if (!shape)
        return nullptr;

    RootedArrayObject arrayProto(cx, ArrayObject::createArray(cx, gc::AllocKind::OBJECT4,
                                                              gc::TenuredHeap, shape, group, 0));
    if (!arrayProto ||
        !JSObject::setSingleton(cx, arrayProto) ||
        !AddLengthProperty(cx, arrayProto))
    {
        return nullptr;
    }

    





    if (!JSObject::setNewGroupUnknown(cx, &ArrayObject::class_, arrayProto))
        return nullptr;

    return arrayProto;
}

const Class ArrayObject::class_ = {
    "Array",
    JSCLASS_HAS_CACHED_PROTO(JSProto_Array),
    array_addProperty,
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    {
        GenericCreateConstructor<ArrayConstructor, 1, JSFunction::FinalizeKind>,
        CreateArrayPrototype,
        array_static_methods,
        nullptr,
        array_methods
    }
};





static inline bool
EnsureNewArrayElements(ExclusiveContext* cx, ArrayObject* obj, uint32_t length)
{
    



    DebugOnly<uint32_t> cap = obj->getDenseCapacity();

    if (!obj->ensureElements(cx, length))
        return false;

    MOZ_ASSERT_IF(cap, !obj->hasDynamicElements());

    return true;
}

static bool
NewArrayIsCachable(ExclusiveContext* cxArg, NewObjectKind newKind)
{
    return cxArg->isJSContext() && newKind == GenericObject;
}

template <uint32_t maxLength>
static MOZ_ALWAYS_INLINE ArrayObject*
NewArray(ExclusiveContext* cxArg, uint32_t length,
         HandleObject protoArg, NewObjectKind newKind = GenericObject)
{
    gc::AllocKind allocKind = GuessArrayGCKind(length);
    MOZ_ASSERT(CanBeFinalizedInBackground(allocKind, &ArrayObject::class_));
    allocKind = GetBackgroundAllocKind(allocKind);

    bool isCachable = NewArrayIsCachable(cxArg, newKind);
    if (isCachable) {
        JSContext* cx = cxArg->asJSContext();
        JSRuntime* rt = cx->runtime();
        NewObjectCache& cache = rt->newObjectCache;
        NewObjectCache::EntryIndex entry = -1;
        if (cache.lookupGlobal(&ArrayObject::class_, cx->global(), allocKind, &entry)) {
            gc::InitialHeap heap = GetInitialHeap(newKind, &ArrayObject::class_);
            JSObject* obj = cache.newObjectFromHit(cx, entry, heap);
            if (obj) {
                
                ArrayObject* arr = &obj->as<ArrayObject>();
                arr->setFixedElements();
                arr->setLength(cx, length);
                if (maxLength > 0 &&
                    !EnsureNewArrayElements(cx, arr, std::min(maxLength, length)))
                {
                    return nullptr;
                }
                return arr;
            }
        }
    }

    RootedObject proto(cxArg, protoArg);
    if (!proto && !GetBuiltinPrototype(cxArg, JSProto_Array, &proto))
        return nullptr;

    RootedObjectGroup group(cxArg, ObjectGroup::defaultNewGroup(cxArg, &ArrayObject::class_,
                                                                TaggedProto(proto)));
    if (!group)
        return nullptr;

    



    RootedShape shape(cxArg, EmptyShape::getInitialShape(cxArg, &ArrayObject::class_,
                                                         TaggedProto(proto),
                                                         gc::AllocKind::OBJECT0));
    if (!shape)
        return nullptr;

    RootedArrayObject arr(cxArg, ArrayObject::createArray(cxArg, allocKind,
                                                          GetInitialHeap(newKind, &ArrayObject::class_),
                                                          shape, group, length));
    if (!arr)
        return nullptr;

    if (shape->isEmptyShape()) {
        if (!AddLengthProperty(cxArg, arr))
            return nullptr;
        shape = arr->lastProperty();
        EmptyShape::insertInitialShape(cxArg, shape, proto);
    }

    if (newKind == SingletonObject && !JSObject::setSingleton(cxArg, arr))
        return nullptr;

    if (isCachable) {
        NewObjectCache& cache = cxArg->asJSContext()->runtime()->newObjectCache;
        NewObjectCache::EntryIndex entry = -1;
        cache.lookupGlobal(&ArrayObject::class_, cxArg->global(), allocKind, &entry);
        cache.fillGlobal(entry, &ArrayObject::class_, cxArg->global(), allocKind, arr);
    }

    if (maxLength > 0 && !EnsureNewArrayElements(cxArg, arr, std::min(maxLength, length)))
        return nullptr;

    probes::CreateObject(cxArg, arr);
    return arr;
}

ArrayObject * JS_FASTCALL
js::NewDenseEmptyArray(JSContext* cx, HandleObject proto ,
                       NewObjectKind newKind )
{
    return NewArray<0>(cx, 0, proto, newKind);
}

ArrayObject * JS_FASTCALL
js::NewDenseFullyAllocatedArray(ExclusiveContext* cx, uint32_t length,
                                HandleObject proto ,
                                NewObjectKind newKind )
{
    return NewArray<NativeObject::NELEMENTS_LIMIT>(cx, length, proto, newKind);
}

ArrayObject * JS_FASTCALL
js::NewDensePartlyAllocatedArray(ExclusiveContext* cx, uint32_t length,
                                 HandleObject proto ,
                                 NewObjectKind newKind )
{
    return NewArray<ArrayObject::EagerAllocationMaxLength>(cx, length, proto, newKind);
}

ArrayObject * JS_FASTCALL
js::NewDenseUnallocatedArray(ExclusiveContext* cx, uint32_t length,
                             HandleObject proto ,
                             NewObjectKind newKind )
{
    return NewArray<0>(cx, length, proto, newKind);
}

ArrayObject*
js::NewDenseArray(ExclusiveContext* cx, uint32_t length, HandleObjectGroup group,
                  AllocatingBehaviour allocating, bool convertDoubleElements)
{
    NewObjectKind newKind = !group ? SingletonObject : GenericObject;
    if (group && group->shouldPreTenure())
        newKind = TenuredObject;

    ArrayObject* arr;
    if (allocating == NewArray_Unallocating) {
        arr = NewDenseUnallocatedArray(cx, length, NullPtr(), newKind);
    } else if (allocating == NewArray_PartlyAllocating) {
        arr = NewDensePartlyAllocatedArray(cx, length, NullPtr(), newKind);
    } else {
        MOZ_ASSERT(allocating == NewArray_FullyAllocating);
        arr = NewDenseFullyAllocatedArray(cx, length, NullPtr(), newKind);
    }
    if (!arr)
        return nullptr;

    if (group)
        arr->setGroup(group);

    if (convertDoubleElements)
        arr->setShouldConvertDoubleElements();

    
    
    if (arr->length() > INT32_MAX)
        arr->setLength(cx, arr->length());

    return arr;
}

ArrayObject*
js::NewDenseCopiedArray(JSContext* cx, uint32_t length, HandleArrayObject src,
                        uint32_t elementOffset, HandleObject proto )
{
    MOZ_ASSERT(!src->isIndexed());

    ArrayObject* arr = NewArray<NativeObject::NELEMENTS_LIMIT>(cx, length, proto);
    if (!arr)
        return nullptr;

    MOZ_ASSERT(arr->getDenseCapacity() >= length);

    const Value* vp = src->getDenseElements() + elementOffset;
    arr->setDenseInitializedLength(vp ? length : 0);

    if (vp)
        arr->initDenseElements(0, vp, length);

    return arr;
}


ArrayObject*
js::NewDenseCopiedArray(JSContext* cx, uint32_t length, const Value* values,
                        HandleObject proto ,
                        NewObjectKind newKind )
{
    ArrayObject* arr = NewArray<NativeObject::NELEMENTS_LIMIT>(cx, length, proto);
    if (!arr)
        return nullptr;

    MOZ_ASSERT(arr->getDenseCapacity() >= length);

    arr->setDenseInitializedLength(values ? length : 0);

    if (values)
        arr->initDenseElements(0, values, length);

    return arr;
}

ArrayObject*
js::NewDenseFullyAllocatedArrayWithTemplate(JSContext* cx, uint32_t length, JSObject* templateObject)
{
    gc::AllocKind allocKind = GuessArrayGCKind(length);
    MOZ_ASSERT(CanBeFinalizedInBackground(allocKind, &ArrayObject::class_));
    allocKind = GetBackgroundAllocKind(allocKind);

    RootedObjectGroup group(cx, templateObject->group());
    RootedShape shape(cx, templateObject->as<ArrayObject>().lastProperty());

    gc::InitialHeap heap = GetInitialHeap(GenericObject, &ArrayObject::class_);
    Rooted<ArrayObject*> arr(cx, ArrayObject::createArray(cx, allocKind,
                                                           heap, shape, group, length));
    if (!arr)
        return nullptr;

    if (!EnsureNewArrayElements(cx, arr, length))
        return nullptr;

    probes::CreateObject(cx, arr);

    return arr;
}

JSObject*
js::NewDenseCopyOnWriteArray(JSContext* cx, HandleArrayObject templateObject, gc::InitialHeap heap)
{
    MOZ_ASSERT(!gc::IsInsideNursery(templateObject));

    ArrayObject* arr = ArrayObject::createCopyOnWriteArray(cx, heap, templateObject);
    if (!arr)
        return nullptr;

    probes::CreateObject(cx, arr);
    return arr;
}

#ifdef DEBUG
bool
js::ArrayInfo(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JSObject* obj;

    for (unsigned i = 0; i < args.length(); i++) {
        RootedValue arg(cx, args[i]);

        UniquePtr<char[], JS::FreePolicy> bytes =
            DecompileValueGenerator(cx, JSDVG_SEARCH_STACK, arg, NullPtr());
        if (!bytes)
            return false;
        if (arg.isPrimitive() ||
            !(obj = arg.toObjectOrNull())->is<ArrayObject>()) {
            fprintf(stderr, "%s: not array\n", bytes.get());
            continue;
        }
        fprintf(stderr, "%s: (len %u", bytes.get(), obj->as<ArrayObject>().length());
        fprintf(stderr, ", capacity %u", obj->as<ArrayObject>().getDenseCapacity());
        fputs(")\n", stderr);
    }

    args.rval().setUndefined();
    return true;
}
#endif
