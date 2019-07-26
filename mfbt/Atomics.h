














#ifndef mozilla_Atomics_h
#define mozilla_Atomics_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Compiler.h"
#include "mozilla/TypeTraits.h"

#include <stdint.h>






#if defined(__clang__) || defined(__GNUC__)
   





#  if MOZ_USING_LIBSTDCXX && MOZ_LIBSTDCXX_VERSION_AT_LEAST(4, 7, 0)
#    define MOZ_HAVE_CXX11_ATOMICS
#  elif MOZ_USING_LIBCXX
#    define MOZ_HAVE_CXX11_ATOMICS
#  endif
#elif defined(_MSC_VER) && _MSC_VER >= 1700
#  define MOZ_HAVE_CXX11_ATOMICS
#endif

namespace mozilla {































enum MemoryOrdering {
  




























  Relaxed,
  



















  ReleaseAcquire,
  


























  SequentiallyConsistent,
};

} 


#ifdef MOZ_HAVE_CXX11_ATOMICS

#  include <atomic>

namespace mozilla {
namespace detail {





template<MemoryOrdering Order> struct AtomicOrderConstraints;

template<>
struct AtomicOrderConstraints<Relaxed>
{
    static const std::memory_order AtomicRMWOrder = std::memory_order_relaxed;
    static const std::memory_order LoadOrder = std::memory_order_relaxed;
    static const std::memory_order StoreOrder = std::memory_order_relaxed;
    static const std::memory_order CompareExchangeFailureOrder =
      std::memory_order_relaxed;
};

template<>
struct AtomicOrderConstraints<ReleaseAcquire>
{
    static const std::memory_order AtomicRMWOrder = std::memory_order_acq_rel;
    static const std::memory_order LoadOrder = std::memory_order_acquire;
    static const std::memory_order StoreOrder = std::memory_order_release;
    static const std::memory_order CompareExchangeFailureOrder =
      std::memory_order_acquire;
};

template<>
struct AtomicOrderConstraints<SequentiallyConsistent>
{
    static const std::memory_order AtomicRMWOrder = std::memory_order_seq_cst;
    static const std::memory_order LoadOrder = std::memory_order_seq_cst;
    static const std::memory_order StoreOrder = std::memory_order_seq_cst;
    static const std::memory_order CompareExchangeFailureOrder =
      std::memory_order_seq_cst;
};

template<typename T, MemoryOrdering Order>
struct IntrinsicBase
{
    typedef std::atomic<T> ValueType;
    typedef AtomicOrderConstraints<Order> OrderedOp;
};

template<typename T, MemoryOrdering Order>
struct IntrinsicMemoryOps : public IntrinsicBase<T, Order>
{
    typedef IntrinsicBase<T, Order> Base;
    static T load(const typename Base::ValueType& ptr) {
      return ptr.load(Base::OrderedOp::LoadOrder);
    }
    static void store(typename Base::ValueType& ptr, T val) {
      ptr.store(val, Base::OrderedOp::StoreOrder);
    }
    static T exchange(typename Base::ValueType& ptr, T val) {
      return ptr.exchange(val, Base::OrderedOp::AtomicRMWOrder);
    }
    static bool compareExchange(typename Base::ValueType& ptr, T oldVal, T newVal) {
      return ptr.compare_exchange_strong(oldVal, newVal,
                                         Base::OrderedOp::AtomicRMWOrder,
                                         Base::OrderedOp::CompareExchangeFailureOrder);
    }
};

template<typename T, MemoryOrdering Order>
struct IntrinsicAddSub : public IntrinsicBase<T, Order>
{
    typedef IntrinsicBase<T, Order> Base;
    static T add(typename Base::ValueType& ptr, T val) {
      return ptr.fetch_add(val, Base::OrderedOp::AtomicRMWOrder);
    }
    static T sub(typename Base::ValueType& ptr, T val) {
      return ptr.fetch_sub(val, Base::OrderedOp::AtomicRMWOrder);
    }
};

template<typename T, MemoryOrdering Order>
struct IntrinsicAddSub<T*, Order> : public IntrinsicBase<T*, Order>
{
    typedef IntrinsicBase<T*, Order> Base;
    static T* add(typename Base::ValueType& ptr, ptrdiff_t val) {
      return ptr.fetch_add(fixupAddend(val), Base::OrderedOp::AtomicRMWOrder);
    }
    static T* sub(typename Base::ValueType& ptr, ptrdiff_t val) {
      return ptr.fetch_sub(fixupAddend(val), Base::OrderedOp::AtomicRMWOrder);
    }
  private:
    




    static ptrdiff_t fixupAddend(ptrdiff_t val) {
#if defined(__clang__) || defined(_MSC_VER)
      return val;
#elif defined(__GNUC__) && MOZ_GCC_VERSION_AT_LEAST(4, 6, 0) && \
      !MOZ_GCC_VERSION_AT_LEAST(4, 7, 0)
      return val * sizeof(T);
#else
      return val;
#endif
    }
};

template<typename T, MemoryOrdering Order>
struct IntrinsicIncDec : public IntrinsicAddSub<T, Order>
{
    typedef IntrinsicBase<T, Order> Base;
    static T inc(typename Base::ValueType& ptr) {
      return IntrinsicAddSub<T, Order>::add(ptr, 1);
    }
    static T dec(typename Base::ValueType& ptr) {
      return IntrinsicAddSub<T, Order>::sub(ptr, 1);
    }
};

template<typename T, MemoryOrdering Order>
struct AtomicIntrinsics : public IntrinsicMemoryOps<T, Order>,
                          public IntrinsicIncDec<T, Order>
{
    typedef IntrinsicBase<T, Order> Base;
    static T or_(typename Base::ValueType& ptr, T val) {
      return ptr.fetch_or(val, Base::OrderedOp::AtomicRMWOrder);
    }
    static T xor_(typename Base::ValueType& ptr, T val) {
      return ptr.fetch_xor(val, Base::OrderedOp::AtomicRMWOrder);
    }
    static T and_(typename Base::ValueType& ptr, T val) {
      return ptr.fetch_and(val, Base::OrderedOp::AtomicRMWOrder);
    }
};

template<typename T, MemoryOrdering Order>
struct AtomicIntrinsics<T*, Order>
  : public IntrinsicMemoryOps<T*, Order>, public IntrinsicIncDec<T*, Order>
{
};

} 
} 

#elif defined(__GNUC__)

namespace mozilla {
namespace detail {





















template<MemoryOrdering Order> struct Barrier;








template<>
struct Barrier<Relaxed>
{
    static void beforeLoad() {}
    static void afterLoad() {}
    static void beforeStore() {}
    static void afterStore() {}
};

template<>
struct Barrier<ReleaseAcquire>
{
    static void beforeLoad() {}
    static void afterLoad() { __sync_synchronize(); }
    static void beforeStore() { __sync_synchronize(); }
    static void afterStore() {}
};

template<>
struct Barrier<SequentiallyConsistent>
{
    static void beforeLoad() { __sync_synchronize(); }
    static void afterLoad() { __sync_synchronize(); }
    static void beforeStore() { __sync_synchronize(); }
    static void afterStore() { __sync_synchronize(); }
};

template<typename T, MemoryOrdering Order>
struct IntrinsicMemoryOps
{
    static T load(const T& ptr) {
      Barrier<Order>::beforeLoad();
      T val = ptr;
      Barrier<Order>::afterLoad();
      return val;
    }
    static void store(T& ptr, T val) {
      Barrier<Order>::beforeStore();
      ptr = val;
      Barrier<Order>::afterStore();
    }
    static T exchange(T& ptr, T val) {
      
      
      
      

      Barrier<Order>::beforeStore();
      return __sync_lock_test_and_set(&ptr, val);
    }
    static bool compareExchange(T& ptr, T oldVal, T newVal) {
      return __sync_bool_compare_and_swap(&ptr, oldVal, newVal);
    }
};

template<typename T>
struct IntrinsicAddSub
{
    typedef T ValueType;
    static T add(T& ptr, T val) {
      return __sync_fetch_and_add(&ptr, val);
    }
    static T sub(T& ptr, T val) {
      return __sync_fetch_and_sub(&ptr, val);
    }
};

template<typename T>
struct IntrinsicAddSub<T*>
{
    typedef T* ValueType;
    






    static ValueType add(ValueType& ptr, ptrdiff_t val) {
      ValueType amount = reinterpret_cast<ValueType>(val * sizeof(T));
      return __sync_fetch_and_add(&ptr, amount);
    }
    static ValueType sub(ValueType& ptr, ptrdiff_t val) {
      ValueType amount = reinterpret_cast<ValueType>(val * sizeof(T));
      return __sync_fetch_and_sub(&ptr, amount);
    }
};

template<typename T>
struct IntrinsicIncDec : public IntrinsicAddSub<T>
{
    static T inc(T& ptr) { return IntrinsicAddSub<T>::add(ptr, 1); }
    static T dec(T& ptr) { return IntrinsicAddSub<T>::sub(ptr, 1); }
};

template<typename T, MemoryOrdering Order>
struct AtomicIntrinsics : public IntrinsicMemoryOps<T, Order>,
                          public IntrinsicIncDec<T>
{
    static T or_(T& ptr, T val) {
      return __sync_fetch_and_or(&ptr, val);
    }
    static T xor_(T& ptr, T val) {
      return __sync_fetch_and_xor(&ptr, val);
    }
    static T and_(T& ptr, T val) {
      return __sync_fetch_and_and(&ptr, val);
    }
};

template<typename T, MemoryOrdering Order>
struct AtomicIntrinsics<T*, Order> : public IntrinsicMemoryOps<T*, Order>,
                                     public IntrinsicIncDec<T*>
{
};

} 
} 

#elif defined(_MSC_VER)













extern "C" {
long __cdecl _InterlockedExchangeAdd(long volatile* dst, long value);
long __cdecl _InterlockedOr(long volatile* dst, long value);
long __cdecl _InterlockedXor(long volatile* dst, long value);
long __cdecl _InterlockedAnd(long volatile* dst, long value);
long __cdecl _InterlockedExchange(long volatile *dst, long value);
long __cdecl _InterlockedCompareExchange(long volatile *dst, long newVal, long oldVal);
}

#  pragma intrinsic(_InterlockedExchangeAdd)
#  pragma intrinsic(_InterlockedOr)
#  pragma intrinsic(_InterlockedXor)
#  pragma intrinsic(_InterlockedAnd)
#  pragma intrinsic(_InterlockedExchange)
#  pragma intrinsic(_InterlockedCompareExchange)

namespace mozilla {
namespace detail {

#  if !defined(_M_IX86) && !defined(_M_X64)
     




#    error "Unknown CPU type"
#  endif







































template<size_t DataSize> struct PrimitiveIntrinsics;

template<>
struct PrimitiveIntrinsics<4>
{
    typedef long Type;

    static Type add(Type* ptr, Type val) {
      return _InterlockedExchangeAdd(ptr, val);
    }
    static Type sub(Type* ptr, Type val) {
      



      return _InterlockedExchangeAdd(ptr, -val);
    }
    static Type or_(Type* ptr, Type val) {
      return _InterlockedOr(ptr, val);
    }
    static Type xor_(Type* ptr, Type val) {
      return _InterlockedXor(ptr, val);
    }
    static Type and_(Type* ptr, Type val) {
      return _InterlockedAnd(ptr, val);
    }
    static void store(Type* ptr, Type val) {
      _InterlockedExchange(ptr, val);
    }
    static Type exchange(Type* ptr, Type val) {
      return _InterlockedExchange(ptr, val);
    }
    static bool compareExchange(Type* ptr, Type oldVal, Type newVal) {
      return _InterlockedCompareExchange(ptr, newVal, oldVal) == oldVal;
    }
};

#  if defined(_M_X64)

extern "C" {
long long __cdecl _InterlockedExchangeAdd64(long long volatile* dst,
                                            long long value);
long long __cdecl _InterlockedOr64(long long volatile* dst,
                                   long long value);
long long __cdecl _InterlockedXor64(long long volatile* dst,
                                    long long value);
long long __cdecl _InterlockedAnd64(long long volatile* dst,
                                    long long value);
long long __cdecl _InterlockedExchange64(long long volatile* dst,
                                         long long value);
long long __cdecl _InterlockedCompareExchange64(long long volatile* dst,
                                                long long newVal,
                                                long long oldVal);
}

#    pragma intrinsic(_InterlockedExchangeAdd64)
#    pragma intrinsic(_InterlockedOr64)
#    pragma intrinsic(_InterlockedXor64)
#    pragma intrinsic(_InterlockedAnd64)
#    pragma intrinsic(_InterlockedExchange64)
#    pragma intrinsic(_InterlockedCompareExchange64)

template <>
struct PrimitiveIntrinsics<8>
{
    typedef __int64 Type;

    static Type add(Type* ptr, Type val) {
      return _InterlockedExchangeAdd64(ptr, val);
    }
    static Type sub(Type* ptr, Type val) {
      


      return _InterlockedExchangeAdd64(ptr, -val);
    }
    static Type or_(Type* ptr, Type val) {
      return _InterlockedOr64(ptr, val);
    }
    static Type xor_(Type* ptr, Type val) {
      return _InterlockedXor64(ptr, val);
    }
    static Type and_(Type* ptr, Type val) {
      return _InterlockedAnd64(ptr, val);
    }
    static void store(Type* ptr, Type val) {
      _InterlockedExchange64(ptr, val);
    }
    static Type exchange(Type* ptr, Type val) {
      return _InterlockedExchange64(ptr, val);
    }
    static bool compareExchange(Type* ptr, Type oldVal, Type newVal) {
      return _InterlockedCompareExchange64(ptr, newVal, oldVal) == oldVal;
    }
};

#  endif

extern "C" { void _ReadWriteBarrier(); }

#  pragma intrinsic(_ReadWriteBarrier)

template<MemoryOrdering Order> struct Barrier;







template<>
struct Barrier<Relaxed>
{
    static void beforeLoad() {}
    static void afterLoad() {}
    static void beforeStore() {}
};

template<>
struct Barrier<ReleaseAcquire>
{
    static void beforeLoad() {}
    static void afterLoad() { _ReadWriteBarrier(); }
    static void beforeStore() { _ReadWriteBarrier(); }
};

template<>
struct Barrier<SequentiallyConsistent>
{
    static void beforeLoad() { _ReadWriteBarrier(); }
    static void afterLoad() { _ReadWriteBarrier(); }
    static void beforeStore() { _ReadWriteBarrier(); }
};

template<typename PrimType, typename T>
struct CastHelper
{
  static PrimType toPrimType(T val) { return static_cast<PrimType>(val); }
  static T fromPrimType(PrimType val) { return static_cast<T>(val); }
};

template<typename PrimType, typename T>
struct CastHelper<PrimType, T*>
{
  static PrimType toPrimType(T* val) { return reinterpret_cast<PrimType>(val); }
  static T* fromPrimType(PrimType val) { return reinterpret_cast<T*>(val); }
};

template<typename T>
struct IntrinsicBase
{
    typedef T ValueType;
    typedef PrimitiveIntrinsics<sizeof(T)> Primitives;
    typedef typename Primitives::Type PrimType;
    static_assert(sizeof(PrimType) == sizeof(T),
                  "Selection of PrimitiveIntrinsics was wrong");
    typedef CastHelper<PrimType, T> Cast;
};

template<typename T, MemoryOrdering Order>
struct IntrinsicMemoryOps : public IntrinsicBase<T>
{
    typedef typename IntrinsicBase<T>::ValueType ValueType;
    typedef typename IntrinsicBase<T>::Primitives Primitives;
    typedef typename IntrinsicBase<T>::PrimType PrimType;
    typedef typename IntrinsicBase<T>::Cast Cast;
    static ValueType load(const ValueType& ptr) {
      Barrier<Order>::beforeLoad();
      ValueType val = ptr;
      Barrier<Order>::afterLoad();
      return val;
    }
    static void store(ValueType& ptr, ValueType val) {
      
      
      
      if (Order == SequentiallyConsistent) {
        Primitives::store(reinterpret_cast<PrimType*>(&ptr),
                          Cast::toPrimType(val));
      } else {
        Barrier<Order>::beforeStore();
        ptr = val;
      }
    }
    static ValueType exchange(ValueType& ptr, ValueType val) {
      PrimType oldval =
        Primitives::exchange(reinterpret_cast<PrimType*>(&ptr),
                             Cast::toPrimType(val));
      return Cast::fromPrimType(oldval);
    }
    static bool compareExchange(ValueType& ptr, ValueType oldVal, ValueType newVal) {
      return Primitives::compareExchange(reinterpret_cast<PrimType*>(&ptr),
                                         Cast::toPrimType(oldVal),
                                         Cast::toPrimType(newVal));
    }
};

template<typename T>
struct IntrinsicApplyHelper : public IntrinsicBase<T>
{
    typedef typename IntrinsicBase<T>::ValueType ValueType;
    typedef typename IntrinsicBase<T>::PrimType PrimType;
    typedef typename IntrinsicBase<T>::Cast Cast;
    typedef PrimType (*BinaryOp)(PrimType*, PrimType);
    typedef PrimType (*UnaryOp)(PrimType*);

    static ValueType applyBinaryFunction(BinaryOp op, ValueType& ptr,
                                         ValueType val) {
      PrimType* primTypePtr = reinterpret_cast<PrimType*>(&ptr);
      PrimType primTypeVal = Cast::toPrimType(val);
      return Cast::fromPrimType(op(primTypePtr, primTypeVal));
    }

    static ValueType applyUnaryFunction(UnaryOp op, ValueType& ptr) {
      PrimType* primTypePtr = reinterpret_cast<PrimType*>(&ptr);
      return Cast::fromPrimType(op(primTypePtr));
    }
};

template<typename T>
struct IntrinsicAddSub : public IntrinsicApplyHelper<T>
{
    typedef typename IntrinsicApplyHelper<T>::ValueType ValueType;
    typedef typename IntrinsicBase<T>::Primitives Primitives;
    static ValueType add(ValueType& ptr, ValueType val) {
      return applyBinaryFunction(&Primitives::add, ptr, val);
    }
    static ValueType sub(ValueType& ptr, ValueType val) {
      return applyBinaryFunction(&Primitives::sub, ptr, val);
    }
};

template<typename T>
struct IntrinsicAddSub<T*> : public IntrinsicApplyHelper<T*>
{
    typedef typename IntrinsicApplyHelper<T*>::ValueType ValueType;
    static ValueType add(ValueType& ptr, ptrdiff_t amount) {
      return applyBinaryFunction(&Primitives::add, ptr,
                                 (ValueType)(amount * sizeof(ValueType)));
    }
    static ValueType sub(ValueType& ptr, ptrdiff_t amount) {
      return applyBinaryFunction(&Primitives::sub, ptr,
                                 (ValueType)(amount * sizeof(ValueType)));
    }
};

template<typename T>
struct IntrinsicIncDec : public IntrinsicAddSub<T>
{
    typedef typename IntrinsicAddSub<T>::ValueType ValueType;
    static ValueType inc(ValueType& ptr) { return add(ptr, 1); }
    static ValueType dec(ValueType& ptr) { return sub(ptr, 1); }
};

template<typename T, MemoryOrdering Order>
struct AtomicIntrinsics : public IntrinsicMemoryOps<T, Order>,
                          public IntrinsicIncDec<T>
{
    typedef typename IntrinsicIncDec<T>::ValueType ValueType;
    static ValueType or_(ValueType& ptr, T val) {
      return applyBinaryFunction(&Primitives::or_, ptr, val);
    }
    static ValueType xor_(ValueType& ptr, T val) {
      return applyBinaryFunction(&Primitives::xor_, ptr, val);
    }
    static ValueType and_(ValueType& ptr, T val) {
      return applyBinaryFunction(&Primitives::and_, ptr, val);
    }
};

template<typename T, MemoryOrdering Order>
struct AtomicIntrinsics<T*, Order> : public IntrinsicMemoryOps<T*, Order>,
                                     public IntrinsicIncDec<T*>
{
    typedef typename IntrinsicMemoryOps<T*, Order>::ValueType ValueType;
};

} 
} 

#else
# error "Atomic compiler intrinsics are not supported on your platform"
#endif

namespace mozilla {

namespace detail {

template<typename T, MemoryOrdering Order>
class AtomicBase
{
    
    
    static_assert(sizeof(T) == 4 || (sizeof(uintptr_t) == 8 && sizeof(T) == 8),
                  "mozilla/Atomics.h only supports 32-bit and pointer-sized types");

  protected:
    typedef typename detail::AtomicIntrinsics<T, Order> Intrinsics;
    typename Intrinsics::ValueType mValue;

  public:
    MOZ_CONSTEXPR AtomicBase() : mValue() {}
    MOZ_CONSTEXPR AtomicBase(T aInit) : mValue(aInit) {}

    operator T() const { return Intrinsics::load(mValue); }

    T operator=(T aValue) {
      Intrinsics::store(mValue, aValue);
      return aValue;
    }

    



    T exchange(T aValue) {
      return Intrinsics::exchange(mValue, aValue);
    }

    










    bool compareExchange(T aOldValue, T aNewValue) {
      return Intrinsics::compareExchange(mValue, aOldValue, aNewValue);
    }

  private:
    template<MemoryOrdering AnyOrder>
    AtomicBase(const AtomicBase<T, AnyOrder>& aCopy) MOZ_DELETE;
};

template<typename T, MemoryOrdering Order>
class AtomicBaseIncDec : public AtomicBase<T, Order>
{
    typedef typename detail::AtomicBase<T, Order> Base;

  public:
    MOZ_CONSTEXPR AtomicBaseIncDec() : Base() {}
    MOZ_CONSTEXPR AtomicBaseIncDec(T aInit) : Base(aInit) {}

    using Base::operator=;

    T operator++(int) { return Base::Intrinsics::inc(Base::mValue); }
    T operator--(int) { return Base::Intrinsics::dec(Base::mValue); }
    T operator++() { return Base::Intrinsics::inc(Base::mValue) + 1; }
    T operator--() { return Base::Intrinsics::dec(Base::mValue) - 1; }

  private:
    template<MemoryOrdering AnyOrder>
    AtomicBaseIncDec(const AtomicBaseIncDec<T, AnyOrder>& aCopy) MOZ_DELETE;
};

} 


















template<typename T,
         MemoryOrdering Order = SequentiallyConsistent,
         typename Enable = void>
class Atomic;









template<typename T, MemoryOrdering Order>
class Atomic<T, Order, typename EnableIf<IsIntegral<T>::value>::Type>
  : public detail::AtomicBaseIncDec<T, Order>
{
    typedef typename detail::AtomicBaseIncDec<T, Order> Base;

  public:
    MOZ_CONSTEXPR Atomic() : Base() {}
    MOZ_CONSTEXPR Atomic(T aInit) : Base(aInit) {}

    using Base::operator=;

    T operator+=(T delta) { return Base::Intrinsics::add(Base::mValue, delta) + delta; }
    T operator-=(T delta) { return Base::Intrinsics::sub(Base::mValue, delta) - delta; }
    T operator|=(T val) { return Base::Intrinsics::or_(Base::mValue, val) | val; }
    T operator^=(T val) { return Base::Intrinsics::xor_(Base::mValue, val) ^ val; }
    T operator&=(T val) { return Base::Intrinsics::and_(Base::mValue, val) & val; }

  private:
    Atomic(Atomic<T, Order>& aOther) MOZ_DELETE;
};









template<typename T, MemoryOrdering Order>
class Atomic<T*, Order> : public detail::AtomicBaseIncDec<T*, Order>
{
    typedef typename detail::AtomicBaseIncDec<T*, Order> Base;

  public:
    MOZ_CONSTEXPR Atomic() : Base() {}
    MOZ_CONSTEXPR Atomic(T* aInit) : Base(aInit) {}

    using Base::operator=;

    T* operator+=(ptrdiff_t delta) {
      return Base::Intrinsics::add(Base::mValue, delta) + delta;
    }
    T* operator-=(ptrdiff_t delta) {
      return Base::Intrinsics::sub(Base::mValue, delta) - delta;
    }

  private:
    Atomic(Atomic<T*, Order>& aOther) MOZ_DELETE;
};






template<typename T, MemoryOrdering Order>
class Atomic<T, Order, typename EnableIf<IsEnum<T>::value>::Type>
  : public detail::AtomicBase<T, Order>
{
    typedef typename detail::AtomicBase<T, Order> Base;

  public:
    MOZ_CONSTEXPR Atomic() : Base() {}
    MOZ_CONSTEXPR Atomic(T aInit) : Base(aInit) {}

    using Base::operator=;

  private:
    Atomic(Atomic<T, Order>& aOther) MOZ_DELETE;
};

} 

#endif 
