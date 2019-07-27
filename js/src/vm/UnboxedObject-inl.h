





#ifndef vm_UnboxedObject_inl_h
#define vm_UnboxedObject_inl_h

#include "vm/UnboxedObject.h"

#include "vm/ArrayObject-inl.h"
#include "vm/NativeObject-inl.h"

namespace js {

static inline Value
GetUnboxedValue(uint8_t* p, JSValueType type, bool maybeUninitialized)
{
    switch (type) {
      case JSVAL_TYPE_BOOLEAN:
        return BooleanValue(*p != 0);

      case JSVAL_TYPE_INT32:
        return Int32Value(*reinterpret_cast<int32_t*>(p));

      case JSVAL_TYPE_DOUBLE: {
        
        
        
        
        double d = *reinterpret_cast<double*>(p);
        if (maybeUninitialized)
            return DoubleValue(JS::CanonicalizeNaN(d));
        return DoubleValue(d);
      }

      case JSVAL_TYPE_STRING:
        return StringValue(*reinterpret_cast<JSString**>(p));

      case JSVAL_TYPE_OBJECT:
        return ObjectOrNullValue(*reinterpret_cast<JSObject**>(p));

      default:
        MOZ_CRASH("Invalid type for unboxed value");
    }
}

static inline void
SetUnboxedValueNoTypeChange(JSObject* unboxedObject,
                            uint8_t* p, JSValueType type, const Value& v,
                            bool preBarrier)
{
    switch (type) {
      case JSVAL_TYPE_BOOLEAN:
        *p = v.toBoolean();
        return;

      case JSVAL_TYPE_INT32:
        *reinterpret_cast<int32_t*>(p) = v.toInt32();
        return;

      case JSVAL_TYPE_DOUBLE:
        *reinterpret_cast<double*>(p) = v.toNumber();
        return;

      case JSVAL_TYPE_STRING: {
        MOZ_ASSERT(!IsInsideNursery(v.toString()));
        JSString** np = reinterpret_cast<JSString**>(p);
        if (preBarrier)
            JSString::writeBarrierPre(*np);
        *np = v.toString();
        return;
      }

      case JSVAL_TYPE_OBJECT: {
        JSObject** np = reinterpret_cast<JSObject**>(p);

        
        
        
        JSObject* obj = v.toObjectOrNull();
        if (IsInsideNursery(obj) && !IsInsideNursery(unboxedObject)) {
            JSRuntime* rt = unboxedObject->runtimeFromMainThread();
            rt->gc.storeBuffer.putWholeCellFromMainThread(unboxedObject);
        }

        if (preBarrier)
            JSObject::writeBarrierPre(*np);
        *np = obj;
        return;
      }

      default:
        MOZ_CRASH("Invalid type for unboxed value");
    }
}

static inline bool
SetUnboxedValue(ExclusiveContext* cx, JSObject* unboxedObject, jsid id,
                uint8_t* p, JSValueType type, const Value& v, bool preBarrier)
{
    switch (type) {
      case JSVAL_TYPE_BOOLEAN:
        if (v.isBoolean()) {
            *p = v.toBoolean();
            return true;
        }
        return false;

      case JSVAL_TYPE_INT32:
        if (v.isInt32()) {
            *reinterpret_cast<int32_t*>(p) = v.toInt32();
            return true;
        }
        return false;

      case JSVAL_TYPE_DOUBLE:
        if (v.isNumber()) {
            *reinterpret_cast<double*>(p) = v.toNumber();
            return true;
        }
        return false;

      case JSVAL_TYPE_STRING:
        if (v.isString()) {
            MOZ_ASSERT(!IsInsideNursery(v.toString()));
            JSString** np = reinterpret_cast<JSString**>(p);
            if (preBarrier)
                JSString::writeBarrierPre(*np);
            *np = v.toString();
            return true;
        }
        return false;

      case JSVAL_TYPE_OBJECT:
        if (v.isObjectOrNull()) {
            JSObject** np = reinterpret_cast<JSObject**>(p);

            
            
            
            AddTypePropertyId(cx, unboxedObject, id, v);

            
            JSObject* obj = v.toObjectOrNull();
            if (IsInsideNursery(v.toObjectOrNull()) && !IsInsideNursery(unboxedObject)) {
                JSRuntime* rt = unboxedObject->runtimeFromMainThread();
                rt->gc.storeBuffer.putWholeCellFromMainThread(unboxedObject);
            }

            if (preBarrier)
                JSObject::writeBarrierPre(*np);
            *np = obj;
            return true;
        }
        return false;

      default:
        MOZ_CRASH("Invalid type for unboxed value");
    }
}





inline const UnboxedLayout&
UnboxedPlainObject::layout() const
{
    return group()->unboxedLayout();
}





inline const UnboxedLayout&
UnboxedArrayObject::layout() const
{
    return group()->unboxedLayout();
}

inline void
UnboxedArrayObject::setLength(ExclusiveContext* cx, uint32_t length)
{
    if (length > INT32_MAX) {
        
        MarkObjectGroupFlags(cx, this, OBJECT_FLAG_LENGTH_OVERFLOW);
    }

    length_ = length;
}

template <JSValueType Type>
inline bool
UnboxedArrayObject::setElementSpecific(ExclusiveContext* cx, size_t index, const Value& v)
{
    MOZ_ASSERT(index < initializedLength());
    MOZ_ASSERT(Type == elementType());
    uint8_t* p = elements() + index * UnboxedTypeSize(Type);
    return SetUnboxedValue(cx, this, JSID_VOID, p, elementType(), v,  true);
}

template <JSValueType Type>
inline void
UnboxedArrayObject::setElementNoTypeChangeSpecific(size_t index, const Value& v)
{
    MOZ_ASSERT(index < initializedLength());
    MOZ_ASSERT(Type == elementType());
    uint8_t* p = elements() + index * UnboxedTypeSize(Type);
    return SetUnboxedValueNoTypeChange(this, p, elementType(), v,  true);
}

template <JSValueType Type>
inline bool
UnboxedArrayObject::initElementSpecific(ExclusiveContext* cx, size_t index, const Value& v)
{
    MOZ_ASSERT(index < initializedLength());
    MOZ_ASSERT(Type == elementType());
    uint8_t* p = elements() + index * UnboxedTypeSize(Type);
    return SetUnboxedValue(cx, this, JSID_VOID, p, elementType(), v,  false);
}

template <JSValueType Type>
inline void
UnboxedArrayObject::initElementNoTypeChangeSpecific(size_t index, const Value& v)
{
    MOZ_ASSERT(index < initializedLength());
    MOZ_ASSERT(Type == elementType());
    uint8_t* p = elements() + index * UnboxedTypeSize(Type);
    return SetUnboxedValueNoTypeChange(this, p, elementType(), v,  false);
}

template <JSValueType Type>
inline Value
UnboxedArrayObject::getElementSpecific(size_t index)
{
    MOZ_ASSERT(index < initializedLength());
    MOZ_ASSERT(Type == elementType());
    uint8_t* p = elements() + index * UnboxedTypeSize(Type);
    return GetUnboxedValue(p, Type,  false);
}

template <JSValueType Type>
inline void
UnboxedArrayObject::triggerPreBarrier(size_t index)
{
    MOZ_ASSERT(UnboxedTypeNeedsPreBarrier(Type));

    uint8_t* p = elements() + index * UnboxedTypeSize(Type);

    switch (Type) {
      case JSVAL_TYPE_STRING: {
        JSString** np = reinterpret_cast<JSString**>(p);
        JSString::writeBarrierPre(*np);
        break;
      }

      case JSVAL_TYPE_OBJECT: {
        JSObject** np = reinterpret_cast<JSObject**>(p);
        JSObject::writeBarrierPre(*np);
        break;
      }

      default:
        MOZ_CRASH("Bad type");
    }
}





static inline size_t
GetAnyBoxedOrUnboxedInitializedLength(JSObject* obj)
{
    if (obj->isNative())
        return obj->as<NativeObject>().getDenseInitializedLength();
    if (obj->is<UnboxedArrayObject>())
        return obj->as<UnboxedArrayObject>().initializedLength();
    return 0;
}

static inline size_t
GetAnyBoxedOrUnboxedCapacity(JSObject* obj)
{
    if (obj->isNative())
        return obj->as<NativeObject>().getDenseCapacity();
    if (obj->is<UnboxedArrayObject>())
        return obj->as<UnboxedArrayObject>().capacity();
    return 0;
}

static inline Value
GetAnyBoxedOrUnboxedDenseElement(JSObject* obj, size_t index)
{
    if (obj->isNative())
        return obj->as<NativeObject>().getDenseElement(index);
    return obj->as<UnboxedArrayObject>().getElement(index);
}

static inline size_t
GetAnyBoxedOrUnboxedArrayLength(JSObject* obj)
{
    if (obj->is<ArrayObject>())
        return obj->as<ArrayObject>().length();
    return obj->as<UnboxedArrayObject>().length();
}

static inline void
SetAnyBoxedOrUnboxedArrayLength(JSContext* cx, JSObject* obj, size_t length)
{
    if (obj->is<ArrayObject>()) {
        MOZ_ASSERT(length >= obj->as<ArrayObject>().length());
        obj->as<ArrayObject>().setLength(cx, length);
    } else {
        MOZ_ASSERT(length >= obj->as<UnboxedArrayObject>().length());
        obj->as<UnboxedArrayObject>().setLength(cx, length);
    }
}





template <JSValueType Type>
static inline bool
HasBoxedOrUnboxedDenseElements(JSObject* obj)
{
    if (Type == JSVAL_TYPE_MAGIC)
        return obj->isNative();
    return obj->is<UnboxedArrayObject>() && obj->as<UnboxedArrayObject>().elementType() == Type;
}

template <JSValueType Type>
static inline size_t
GetBoxedOrUnboxedInitializedLength(JSObject* obj)
{
    if (Type == JSVAL_TYPE_MAGIC)
        return obj->as<NativeObject>().getDenseInitializedLength();
    return obj->as<UnboxedArrayObject>().initializedLength();
}

template <JSValueType Type>
static inline DenseElementResult
SetBoxedOrUnboxedInitializedLength(JSContext* cx, JSObject* obj, size_t initlen)
{
    size_t oldInitlen = GetBoxedOrUnboxedInitializedLength<Type>(obj);
    if (Type == JSVAL_TYPE_MAGIC) {
        obj->as<NativeObject>().setDenseInitializedLength(initlen);
        if (initlen < oldInitlen)
            obj->as<NativeObject>().shrinkElements(cx, initlen);
    } else {
        obj->as<UnboxedArrayObject>().setInitializedLength(initlen);
        if (initlen < oldInitlen)
            obj->as<UnboxedArrayObject>().shrinkElements(cx, initlen);
    }
    return DenseElementResult::Success;
}

template <JSValueType Type>
static inline size_t
GetBoxedOrUnboxedCapacity(JSObject* obj)
{
    if (Type == JSVAL_TYPE_MAGIC)
        return obj->as<NativeObject>().getDenseCapacity();
    return obj->as<UnboxedArrayObject>().capacity();
}

template <JSValueType Type>
static inline Value
GetBoxedOrUnboxedDenseElement(JSObject* obj, size_t index)
{
    if (Type == JSVAL_TYPE_MAGIC)
        return obj->as<NativeObject>().getDenseElement(index);
    return obj->as<UnboxedArrayObject>().getElementSpecific<Type>(index);
}

template <JSValueType Type>
static inline void
SetBoxedOrUnboxedDenseElementNoTypeChange(JSObject* obj, size_t index, const Value& value)
{
    if (Type == JSVAL_TYPE_MAGIC)
        obj->as<NativeObject>().setDenseElement(index, value);
    else
        obj->as<UnboxedArrayObject>().setElementNoTypeChangeSpecific<Type>(index, value);
}

template <JSValueType Type>
static inline bool
EnsureBoxedOrUnboxedDenseElements(JSContext* cx, JSObject* obj, size_t count)
{
    if (Type == JSVAL_TYPE_MAGIC) {
        if (!obj->as<ArrayObject>().ensureElements(cx, count))
            return false;
    } else {
        if (obj->as<UnboxedArrayObject>().capacity() < count) {
            if (!obj->as<UnboxedArrayObject>().growElements(cx, count))
                return false;
        }
    }
    return true;
}

enum ShouldUpdateTypes
{
    UpdateTypes = true,
    DontUpdateTypes = false
};

template <JSValueType Type>
static inline DenseElementResult
SetOrExtendBoxedOrUnboxedDenseElements(JSContext* cx, JSObject* obj,
                                       uint32_t start, const Value* vp, uint32_t count,
                                       ShouldUpdateTypes updateTypes)
{
    if (Type == JSVAL_TYPE_MAGIC) {
        NativeObject* nobj = &obj->as<NativeObject>();

        if (obj->is<ArrayObject>() &&
            !obj->as<ArrayObject>().lengthIsWritable() &&
            start + count >= obj->as<ArrayObject>().length())
        {
            return DenseElementResult::Incomplete;
        }

        DenseElementResult result = nobj->ensureDenseElements(cx, start, count);
        if (result != DenseElementResult::Success)
            return result;

        if (obj->is<ArrayObject>() && start + count >= obj->as<ArrayObject>().length())
            obj->as<ArrayObject>().setLengthInt32(start + count);

        if (updateTypes == DontUpdateTypes && !nobj->shouldConvertDoubleElements()) {
            nobj->copyDenseElements(start, vp, count);
        } else {
            for (size_t i = 0; i < count; i++)
                nobj->setDenseElementWithType(cx, start + i, vp[i]);
        }

        return DenseElementResult::Success;
    }

    UnboxedArrayObject* nobj = &obj->as<UnboxedArrayObject>();

    if (start > nobj->initializedLength())
        return DenseElementResult::Incomplete;

    if (start + count >= UnboxedArrayObject::MaximumCapacity)
        return DenseElementResult::Incomplete;

    if (start + count > nobj->capacity() && !nobj->growElements(cx, start + count))
        return DenseElementResult::Failure;

    size_t oldInitlen = nobj->initializedLength();

    
    
    
    
    
    
    size_t i = 0;
    if (updateTypes == DontUpdateTypes) {
        for (size_t j = start; i < count && j < oldInitlen; i++)
            nobj->setElementNoTypeChangeSpecific<Type>(j, vp[i]);
    } else {
        for (size_t j = start; i < count && j < oldInitlen; i++) {
            if (!nobj->setElementSpecific<Type>(cx, j, vp[i]))
                return DenseElementResult::Incomplete;
        }
    }

    if (i != count) {
        obj->as<UnboxedArrayObject>().setInitializedLength(start + count);
        if (updateTypes == DontUpdateTypes) {
            for (; i < count; i++)
                nobj->initElementNoTypeChangeSpecific<Type>(start + i, vp[i]);
        } else {
            for (; i < count; i++) {
                if (!nobj->initElementSpecific<Type>(cx, start + i, vp[i])) {
                    nobj->setInitializedLength(oldInitlen);
                    return DenseElementResult::Incomplete;
                }
            }
        }
    }

    if (start + count >= nobj->length())
        nobj->setLength(cx, start + count);

    return DenseElementResult::Success;
}

template <JSValueType Type>
static inline DenseElementResult
MoveBoxedOrUnboxedDenseElements(JSContext* cx, JSObject* obj, uint32_t dstStart, uint32_t srcStart,
                                uint32_t length)
{
    MOZ_ASSERT(HasBoxedOrUnboxedDenseElements<Type>(obj));

    if (Type == JSVAL_TYPE_MAGIC) {
        if (!obj->as<NativeObject>().maybeCopyElementsForWrite(cx))
            return DenseElementResult::Failure;
        obj->as<NativeObject>().moveDenseElements(dstStart, srcStart, length);
    } else {
        uint8_t* data = obj->as<UnboxedArrayObject>().elements();
        size_t elementSize = UnboxedTypeSize(Type);

        if (UnboxedTypeNeedsPreBarrier(Type)) {
            
            
            
            for (size_t i = 0; i < length; i++)
                obj->as<UnboxedArrayObject>().triggerPreBarrier<Type>(dstStart + i);
        }

        memmove(data + dstStart * elementSize,
                data + srcStart * elementSize,
                length * elementSize);
    }

    return DenseElementResult::Success;
}

template <JSValueType Type>
static inline DenseElementResult
CopyBoxedOrUnboxedDenseElements(JSContext* cx, JSObject* dst, JSObject* src,
                                uint32_t dstStart, uint32_t srcStart, uint32_t length)
{
    MOZ_ASSERT(HasBoxedOrUnboxedDenseElements<Type>(src));
    MOZ_ASSERT(HasBoxedOrUnboxedDenseElements<Type>(dst));
    MOZ_ASSERT(GetBoxedOrUnboxedInitializedLength<Type>(dst) == dstStart);
    MOZ_ASSERT(GetBoxedOrUnboxedCapacity<Type>(dst) >= length);

    SetBoxedOrUnboxedInitializedLength<Type>(cx, dst, dstStart + length);

    if (Type == JSVAL_TYPE_MAGIC) {
        const Value* vp = src->as<NativeObject>().getDenseElements() + srcStart;
        dst->as<NativeObject>().initDenseElements(dstStart, vp, length);
    } else {
        uint8_t* dstData = dst->as<UnboxedArrayObject>().elements();
        uint8_t* srcData = src->as<UnboxedArrayObject>().elements();
        size_t elementSize = UnboxedTypeSize(Type);

        memcpy(dstData + dstStart * elementSize,
               srcData + srcStart * elementSize,
               length * elementSize);

        
        if (UnboxedTypeNeedsPostBarrier(Type) && !IsInsideNursery(dst))
            dst->runtimeFromMainThread()->gc.storeBuffer.putWholeCellFromMainThread(dst);
    }

    return DenseElementResult::Success;
}






#ifdef _MSC_VER
# define DEPENDENT_TEMPLATE_HINT
#else
# define DEPENDENT_TEMPLATE_HINT template
#endif



template <typename F>
DenseElementResult
CallBoxedOrUnboxedSpecialization(F f, JSObject* obj)
{
    if (HasBoxedOrUnboxedDenseElements<JSVAL_TYPE_MAGIC>(obj))
        return f. DEPENDENT_TEMPLATE_HINT operator()<JSVAL_TYPE_MAGIC>();
    if (HasBoxedOrUnboxedDenseElements<JSVAL_TYPE_BOOLEAN>(obj))
        return f. DEPENDENT_TEMPLATE_HINT operator()<JSVAL_TYPE_BOOLEAN>();
    if (HasBoxedOrUnboxedDenseElements<JSVAL_TYPE_INT32>(obj))
        return f. DEPENDENT_TEMPLATE_HINT operator()<JSVAL_TYPE_INT32>();
    if (HasBoxedOrUnboxedDenseElements<JSVAL_TYPE_DOUBLE>(obj))
        return f. DEPENDENT_TEMPLATE_HINT operator()<JSVAL_TYPE_DOUBLE>();
    if (HasBoxedOrUnboxedDenseElements<JSVAL_TYPE_STRING>(obj))
        return f. DEPENDENT_TEMPLATE_HINT operator()<JSVAL_TYPE_STRING>();
    if (HasBoxedOrUnboxedDenseElements<JSVAL_TYPE_OBJECT>(obj))
        return f. DEPENDENT_TEMPLATE_HINT operator()<JSVAL_TYPE_OBJECT>();
    return DenseElementResult::Incomplete;
}

#undef DEPENDENT_TEMPLATE_HINT

#define DefineBoxedOrUnboxedFunctor1(Signature, A)                      \
struct Signature ## Functor {                                           \
    A a;                                                                \
    explicit Signature ## Functor(A a)                                  \
      : a(a)                                                            \
    {}                                                                  \
    template <JSValueType Type>                                         \
    DenseElementResult operator()() {                                   \
        return Signature<Type>(a);                                      \
    }                                                                   \
}

#define DefineBoxedOrUnboxedFunctor3(Signature, A, B, C)                \
struct Signature ## Functor {                                           \
    A a; B b; C c;                                                      \
    Signature ## Functor(A a, B b, C c)                                 \
      : a(a), b(b), c(c)                                                \
    {}                                                                  \
    template <JSValueType Type>                                         \
    DenseElementResult operator()() {                                   \
        return Signature<Type>(a, b, c);                                \
    }                                                                   \
}

#define DefineBoxedOrUnboxedFunctor4(Signature, A, B, C, D)             \
struct Signature ## Functor {                                           \
    A a; B b; C c; D d;                                                 \
    Signature ## Functor(A a, B b, C c, D d)                            \
      : a(a), b(b), c(c), d(d)                                          \
    {}                                                                  \
    template <JSValueType Type>                                         \
    DenseElementResult operator()() {                                   \
        return Signature<Type>(a, b, c, d);                             \
    }                                                                   \
}

#define DefineBoxedOrUnboxedFunctor5(Signature, A, B, C, D, E)          \
struct Signature ## Functor {                                           \
    A a; B b; C c; D d; E e;                                            \
    Signature ## Functor(A a, B b, C c, D d, E e)                       \
      : a(a), b(b), c(c), d(d), e(e)                                    \
    {}                                                                  \
    template <JSValueType Type>                                         \
    DenseElementResult operator()() {                                   \
        return Signature<Type>(a, b, c, d, e);                          \
    }                                                                   \
}

#define DefineBoxedOrUnboxedFunctor6(Signature, A, B, C, D, E, F)       \
struct Signature ## Functor {                                           \
    A a; B b; C c; D d; E e; F f;                                       \
    Signature ## Functor(A a, B b, C c, D d, E e, F f)                  \
      : a(a), b(b), c(c), d(d), e(e), f(f)                              \
    {}                                                                  \
    template <JSValueType Type>                                         \
    DenseElementResult operator()() {                                   \
        return Signature<Type>(a, b, c, d, e, f);                       \
    }                                                                   \
}

DenseElementResult
SetOrExtendAnyBoxedOrUnboxedDenseElements(JSContext* cx, JSObject* obj,
                                          uint32_t start, const Value* vp, uint32_t count,
                                          ShouldUpdateTypes updateTypes);

DenseElementResult
MoveAnyBoxedOrUnboxedDenseElements(JSContext* cx, JSObject* obj,
                                   uint32_t dstStart, uint32_t srcStart, uint32_t length);

DenseElementResult
CopyAnyBoxedOrUnboxedDenseElements(JSContext* cx, JSObject* dst, JSObject* src,
                                   uint32_t dstStart, uint32_t srcStart, uint32_t length);

void
SetAnyBoxedOrUnboxedInitializedLength(JSContext* cx, JSObject* obj, size_t initlen);

} 

#endif 
