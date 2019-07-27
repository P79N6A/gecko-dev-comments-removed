














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
#  elif MOZ_USING_LIBCXX && defined(__clang__)
#    define MOZ_HAVE_CXX11_ATOMICS
#  endif
#elif defined(_MSC_VER)
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

  static T load(const typename Base::ValueType& aPtr)
  {
    return aPtr.load(Base::OrderedOp::LoadOrder);
  }

  static void store(typename Base::ValueType& aPtr, T aVal)
  {
    aPtr.store(aVal, Base::OrderedOp::StoreOrder);
  }

  static T exchange(typename Base::ValueType& aPtr, T aVal)
  {
    return aPtr.exchange(aVal, Base::OrderedOp::AtomicRMWOrder);
  }

  static bool compareExchange(typename Base::ValueType& aPtr,
                              T aOldVal, T aNewVal)
  {
    return aPtr.compare_exchange_strong(aOldVal, aNewVal,
                                        Base::OrderedOp::AtomicRMWOrder,
                                        Base::OrderedOp::CompareExchangeFailureOrder);
  }
};

template<typename T, MemoryOrdering Order>
struct IntrinsicAddSub : public IntrinsicBase<T, Order>
{
  typedef IntrinsicBase<T, Order> Base;

  static T add(typename Base::ValueType& aPtr, T aVal)
  {
    return aPtr.fetch_add(aVal, Base::OrderedOp::AtomicRMWOrder);
  }

  static T sub(typename Base::ValueType& aPtr, T aVal)
  {
    return aPtr.fetch_sub(aVal, Base::OrderedOp::AtomicRMWOrder);
  }
};

template<typename T, MemoryOrdering Order>
struct IntrinsicAddSub<T*, Order> : public IntrinsicBase<T*, Order>
{
  typedef IntrinsicBase<T*, Order> Base;

  static T* add(typename Base::ValueType& aPtr, ptrdiff_t aVal)
  {
    return aPtr.fetch_add(fixupAddend(aVal), Base::OrderedOp::AtomicRMWOrder);
  }

  static T* sub(typename Base::ValueType& aPtr, ptrdiff_t aVal)
  {
    return aPtr.fetch_sub(fixupAddend(aVal), Base::OrderedOp::AtomicRMWOrder);
  }
private:
  




  static ptrdiff_t fixupAddend(ptrdiff_t aVal)
  {
#if defined(__clang__) || defined(_MSC_VER)
    return aVal;
#elif defined(__GNUC__) && !MOZ_GCC_VERSION_AT_LEAST(4, 7, 0)
    return aVal * sizeof(T);
#else
    return aVal;
#endif
  }
};

template<typename T, MemoryOrdering Order>
struct IntrinsicIncDec : public IntrinsicAddSub<T, Order>
{
  typedef IntrinsicBase<T, Order> Base;

  static T inc(typename Base::ValueType& aPtr)
  {
    return IntrinsicAddSub<T, Order>::add(aPtr, 1);
  }

  static T dec(typename Base::ValueType& aPtr)
  {
    return IntrinsicAddSub<T, Order>::sub(aPtr, 1);
  }
};

template<typename T, MemoryOrdering Order>
struct AtomicIntrinsics : public IntrinsicMemoryOps<T, Order>,
                          public IntrinsicIncDec<T, Order>
{
  typedef IntrinsicBase<T, Order> Base;

  static T or_(typename Base::ValueType& aPtr, T aVal)
  {
    return aPtr.fetch_or(aVal, Base::OrderedOp::AtomicRMWOrder);
  }

  static T xor_(typename Base::ValueType& aPtr, T aVal)
  {
    return aPtr.fetch_xor(aVal, Base::OrderedOp::AtomicRMWOrder);
  }

  static T and_(typename Base::ValueType& aPtr, T aVal)
  {
    return aPtr.fetch_and(aVal, Base::OrderedOp::AtomicRMWOrder);
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
  static T load(const T& aPtr)
  {
    Barrier<Order>::beforeLoad();
    T val = aPtr;
    Barrier<Order>::afterLoad();
    return val;
  }

  static void store(T& aPtr, T aVal)
  {
    Barrier<Order>::beforeStore();
    aPtr = aVal;
    Barrier<Order>::afterStore();
  }

  static T exchange(T& aPtr, T aVal)
  {
    
    
    
    
    Barrier<Order>::beforeStore();
    return __sync_lock_test_and_set(&aPtr, aVal);
  }

  static bool compareExchange(T& aPtr, T aOldVal, T aNewVal)
  {
    return __sync_bool_compare_and_swap(&aPtr, aOldVal, aNewVal);
  }
};

template<typename T>
struct IntrinsicAddSub
{
  typedef T ValueType;

  static T add(T& aPtr, T aVal)
  {
    return __sync_fetch_and_add(&aPtr, aVal);
  }

  static T sub(T& aPtr, T aVal)
  {
    return __sync_fetch_and_sub(&aPtr, aVal);
  }
};

template<typename T>
struct IntrinsicAddSub<T*>
{
  typedef T* ValueType;

  






  static ValueType add(ValueType& aPtr, ptrdiff_t aVal)
  {
    ValueType amount = reinterpret_cast<ValueType>(aVal * sizeof(T));
    return __sync_fetch_and_add(&aPtr, amount);
  }

  static ValueType sub(ValueType& aPtr, ptrdiff_t aVal)
  {
    ValueType amount = reinterpret_cast<ValueType>(aVal * sizeof(T));
    return __sync_fetch_and_sub(&aPtr, amount);
  }
};

template<typename T>
struct IntrinsicIncDec : public IntrinsicAddSub<T>
{
  static T inc(T& aPtr) { return IntrinsicAddSub<T>::add(aPtr, 1); }
  static T dec(T& aPtr) { return IntrinsicAddSub<T>::sub(aPtr, 1); }
};

template<typename T, MemoryOrdering Order>
struct AtomicIntrinsics : public IntrinsicMemoryOps<T, Order>,
                          public IntrinsicIncDec<T>
{
  static T or_( T& aPtr, T aVal) { return __sync_fetch_and_or(&aPtr, aVal); }
  static T xor_(T& aPtr, T aVal) { return __sync_fetch_and_xor(&aPtr, aVal); }
  static T and_(T& aPtr, T aVal) { return __sync_fetch_and_and(&aPtr, aVal); }
};

template<typename T, MemoryOrdering Order>
struct AtomicIntrinsics<T*, Order> : public IntrinsicMemoryOps<T*, Order>,
                                     public IntrinsicIncDec<T*>
{
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
  explicit MOZ_CONSTEXPR AtomicBase(T aInit) : mValue(aInit) {}

  
  
  
  

  T operator=(T aVal)
  {
    Intrinsics::store(mValue, aVal);
    return aVal;
  }

  



  T exchange(T aVal)
  {
    return Intrinsics::exchange(mValue, aVal);
  }

  










  bool compareExchange(T aOldValue, T aNewValue)
  {
    return Intrinsics::compareExchange(mValue, aOldValue, aNewValue);
  }

private:
  template<MemoryOrdering AnyOrder>
  AtomicBase(const AtomicBase<T, AnyOrder>& aCopy) = delete;
};

template<typename T, MemoryOrdering Order>
class AtomicBaseIncDec : public AtomicBase<T, Order>
{
  typedef typename detail::AtomicBase<T, Order> Base;

public:
  MOZ_CONSTEXPR AtomicBaseIncDec() : Base() {}
  explicit MOZ_CONSTEXPR AtomicBaseIncDec(T aInit) : Base(aInit) {}

  using Base::operator=;

  operator T() const { return Base::Intrinsics::load(Base::mValue); }
  T operator++(int) { return Base::Intrinsics::inc(Base::mValue); }
  T operator--(int) { return Base::Intrinsics::dec(Base::mValue); }
  T operator++() { return Base::Intrinsics::inc(Base::mValue) + 1; }
  T operator--() { return Base::Intrinsics::dec(Base::mValue) - 1; }

private:
  template<MemoryOrdering AnyOrder>
  AtomicBaseIncDec(const AtomicBaseIncDec<T, AnyOrder>& aCopy) = delete;
};

} 


















template<typename T,
         MemoryOrdering Order = SequentiallyConsistent,
         typename Enable = void>
class Atomic;









template<typename T, MemoryOrdering Order>
class Atomic<T, Order, typename EnableIf<IsIntegral<T>::value &&
                       !IsSame<T, bool>::value>::Type>
  : public detail::AtomicBaseIncDec<T, Order>
{
  typedef typename detail::AtomicBaseIncDec<T, Order> Base;

public:
  MOZ_CONSTEXPR Atomic() : Base() {}
  explicit MOZ_CONSTEXPR Atomic(T aInit) : Base(aInit) {}

  using Base::operator=;

  T operator+=(T aDelta)
  {
    return Base::Intrinsics::add(Base::mValue, aDelta) + aDelta;
  }

  T operator-=(T aDelta)
  {
    return Base::Intrinsics::sub(Base::mValue, aDelta) - aDelta;
  }

  T operator|=(T aVal)
  {
    return Base::Intrinsics::or_(Base::mValue, aVal) | aVal;
  }

  T operator^=(T aVal)
  {
    return Base::Intrinsics::xor_(Base::mValue, aVal) ^ aVal;
  }

  T operator&=(T aVal)
  {
    return Base::Intrinsics::and_(Base::mValue, aVal) & aVal;
  }

private:
  Atomic(Atomic<T, Order>& aOther) = delete;
};









template<typename T, MemoryOrdering Order>
class Atomic<T*, Order> : public detail::AtomicBaseIncDec<T*, Order>
{
  typedef typename detail::AtomicBaseIncDec<T*, Order> Base;

public:
  MOZ_CONSTEXPR Atomic() : Base() {}
  explicit MOZ_CONSTEXPR Atomic(T* aInit) : Base(aInit) {}

  using Base::operator=;

  T* operator+=(ptrdiff_t aDelta)
  {
    return Base::Intrinsics::add(Base::mValue, aDelta) + aDelta;
  }

  T* operator-=(ptrdiff_t aDelta)
  {
    return Base::Intrinsics::sub(Base::mValue, aDelta) - aDelta;
  }

private:
  Atomic(Atomic<T*, Order>& aOther) = delete;
};






template<typename T, MemoryOrdering Order>
class Atomic<T, Order, typename EnableIf<IsEnum<T>::value>::Type>
  : public detail::AtomicBase<T, Order>
{
  typedef typename detail::AtomicBase<T, Order> Base;

public:
  MOZ_CONSTEXPR Atomic() : Base() {}
  explicit MOZ_CONSTEXPR Atomic(T aInit) : Base(aInit) {}

  operator T() const { return Base::Intrinsics::load(Base::mValue); }

  using Base::operator=;

private:
  Atomic(Atomic<T, Order>& aOther) = delete;
};

















template<MemoryOrdering Order>
class Atomic<bool, Order>
  : protected detail::AtomicBase<uint32_t, Order>
{
  typedef typename detail::AtomicBase<uint32_t, Order> Base;

public:
  MOZ_CONSTEXPR Atomic() : Base() {}
  explicit MOZ_CONSTEXPR Atomic(bool aInit) : Base(aInit) {}

  
  operator bool() const
  {
    return Base::Intrinsics::load(Base::mValue);
  }

  bool operator=(bool aVal)
  {
    return Base::operator=(aVal);
  }

  bool exchange(bool aVal)
  {
    return Base::exchange(aVal);
  }

  bool compareExchange(bool aOldValue, bool aNewValue)
  {
    return Base::compareExchange(aOldValue, aNewValue);
  }

private:
  Atomic(Atomic<bool, Order>& aOther) = delete;
};

} 

#endif 
