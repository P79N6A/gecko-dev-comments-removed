







#ifndef mozilla_UniquePtr_h
#define mozilla_UniquePtr_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Compiler.h"
#include "mozilla/Move.h"
#include "mozilla/Pair.h"
#include "mozilla/TypeTraits.h"

namespace mozilla {

template<typename T> class DefaultDelete;
template<typename T, class D = DefaultDelete<T>> class UniquePtr;

} 

namespace mozilla {






























































































































template<typename T, class D>
class UniquePtr
{
public:
  typedef T* Pointer;
  typedef T ElementType;
  typedef D DeleterType;

private:
  Pair<Pointer, DeleterType> mTuple;

  Pointer& ptr() { return mTuple.first(); }
  const Pointer& ptr() const { return mTuple.first(); }

  DeleterType& del() { return mTuple.second(); }
  const DeleterType& del() const { return mTuple.second(); }

public:
  


  MOZ_CONSTEXPR UniquePtr()
    : mTuple(static_cast<Pointer>(nullptr), DeleterType())
  {
    static_assert(!IsPointer<D>::value, "must provide a deleter instance");
    static_assert(!IsReference<D>::value, "must provide a deleter instance");
  }

  


  explicit UniquePtr(Pointer aPtr)
    : mTuple(aPtr, DeleterType())
  {
    static_assert(!IsPointer<D>::value, "must provide a deleter instance");
    static_assert(!IsReference<D>::value, "must provide a deleter instance");
  }

  UniquePtr(Pointer aPtr,
            typename Conditional<IsReference<D>::value,
                                 D,
                                 const D&>::Type aD1)
    : mTuple(aPtr, aD1)
  {}

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  UniquePtr(Pointer aPtr,
            typename RemoveReference<D>::Type&& aD2)
    : mTuple(aPtr, Move(aD2))
  {
    static_assert(!IsReference<D>::value,
                  "rvalue deleter can't be stored by reference");
  }

  UniquePtr(UniquePtr&& aOther)
    : mTuple(aOther.release(), Forward<DeleterType>(aOther.getDeleter()))
  {}

  MOZ_IMPLICIT
  UniquePtr(decltype(nullptr))
    : mTuple(nullptr, DeleterType())
  {
    static_assert(!IsPointer<D>::value, "must provide a deleter instance");
    static_assert(!IsReference<D>::value, "must provide a deleter instance");
  }

  template<typename U, class E>
  UniquePtr(UniquePtr<U, E>&& aOther,
            typename EnableIf<IsConvertible<typename UniquePtr<U, E>::Pointer,
                                            Pointer>::value &&
                              !IsArray<U>::value &&
                              (IsReference<D>::value
                               ? IsSame<D, E>::value
                               : IsConvertible<E, D>::value),
                              int>::Type aDummy = 0)
    : mTuple(aOther.release(), Forward<E>(aOther.getDeleter()))
  {
  }

  ~UniquePtr() { reset(nullptr); }

  UniquePtr& operator=(UniquePtr&& aOther)
  {
    reset(aOther.release());
    getDeleter() = Forward<DeleterType>(aOther.getDeleter());
    return *this;
  }

  template<typename U, typename E>
  UniquePtr& operator=(UniquePtr<U, E>&& aOther)
  {
    static_assert(IsConvertible<typename UniquePtr<U, E>::Pointer,
                                Pointer>::value,
                  "incompatible UniquePtr pointees");
    static_assert(!IsArray<U>::value,
                  "can't assign from UniquePtr holding an array");

    reset(aOther.release());
    getDeleter() = Forward<E>(aOther.getDeleter());
    return *this;
  }

  UniquePtr& operator=(decltype(nullptr))
  {
    reset(nullptr);
    return *this;
  }

  T& operator*() const { return *get(); }
  Pointer operator->() const
  {
    MOZ_ASSERT(get(), "dereferencing a UniquePtr containing nullptr");
    return get();
  }

  explicit operator bool() const { return get() != nullptr; }

  Pointer get() const { return ptr(); }

  DeleterType& getDeleter() { return del(); }
  const DeleterType& getDeleter() const { return del(); }

  Pointer release()
  {
    Pointer p = ptr();
    ptr() = nullptr;
    return p;
  }

  void reset(Pointer aPtr = Pointer())
  {
    Pointer old = ptr();
    ptr() = aPtr;
    if (old != nullptr) {
      getDeleter()(old);
    }
  }

  void swap(UniquePtr& aOther)
  {
    mTuple.swap(aOther.mTuple);
  }

private:
  UniquePtr(const UniquePtr& aOther) = delete; 
  void operator=(const UniquePtr& aOther) = delete; 
};





template<typename T, class D>
class UniquePtr<T[], D>
{
public:
  typedef T* Pointer;
  typedef T ElementType;
  typedef D DeleterType;

private:
  Pair<Pointer, DeleterType> mTuple;

public:
  


  MOZ_CONSTEXPR UniquePtr()
    : mTuple(static_cast<Pointer>(nullptr), DeleterType())
  {
    static_assert(!IsPointer<D>::value, "must provide a deleter instance");
    static_assert(!IsReference<D>::value, "must provide a deleter instance");
  }

  


  explicit UniquePtr(Pointer aPtr)
    : mTuple(aPtr, DeleterType())
  {
    static_assert(!IsPointer<D>::value, "must provide a deleter instance");
    static_assert(!IsReference<D>::value, "must provide a deleter instance");
  }

private:
  
  
  
  
  
  template<typename U>
  UniquePtr(U&& aU,
            typename EnableIf<IsPointer<U>::value &&
                              IsConvertible<U, Pointer>::value,
                              int>::Type aDummy = 0)
  = delete;

public:
  UniquePtr(Pointer aPtr,
            typename Conditional<IsReference<D>::value,
                                 D,
                                 const D&>::Type aD1)
    : mTuple(aPtr, aD1)
  {}

  
  
  
  
  UniquePtr(Pointer aPtr,
            typename RemoveReference<D>::Type&& aD2)
    : mTuple(aPtr, Move(aD2))
  {
    static_assert(!IsReference<D>::value,
                  "rvalue deleter can't be stored by reference");
  }

private:
  
  template<typename U, typename V>
  UniquePtr(U&& aU, V&& aV,
            typename EnableIf<IsPointer<U>::value &&
                              IsConvertible<U, Pointer>::value,
                              int>::Type aDummy = 0)
  = delete;

public:
  UniquePtr(UniquePtr&& aOther)
    : mTuple(aOther.release(), Forward<DeleterType>(aOther.getDeleter()))
  {}

  MOZ_IMPLICIT
  UniquePtr(decltype(nullptr))
    : mTuple(nullptr, DeleterType())
  {
    static_assert(!IsPointer<D>::value, "must provide a deleter instance");
    static_assert(!IsReference<D>::value, "must provide a deleter instance");
  }

  ~UniquePtr() { reset(nullptr); }

  UniquePtr& operator=(UniquePtr&& aOther)
  {
    reset(aOther.release());
    getDeleter() = Forward<DeleterType>(aOther.getDeleter());
    return *this;
  }

  UniquePtr& operator=(decltype(nullptr))
  {
    reset();
    return *this;
  }

  explicit operator bool() const { return get() != nullptr; }

  T& operator[](decltype(sizeof(int)) aIndex) const { return get()[aIndex]; }
  Pointer get() const { return mTuple.first(); }

  DeleterType& getDeleter() { return mTuple.second(); }
  const DeleterType& getDeleter() const { return mTuple.second(); }

  Pointer release()
  {
    Pointer p = mTuple.first();
    mTuple.first() = nullptr;
    return p;
  }

  void reset(Pointer aPtr = Pointer())
  {
    Pointer old = mTuple.first();
    mTuple.first() = aPtr;
    if (old != nullptr) {
      mTuple.second()(old);
    }
  }

  void reset(decltype(nullptr))
  {
    Pointer old = mTuple.first();
    mTuple.first() = nullptr;
    if (old != nullptr) {
      mTuple.second()(old);
    }
  }

private:
  template<typename U>
  void reset(U) = delete;

public:
  void swap(UniquePtr& aOther) { mTuple.swap(aOther.mTuple); }

private:
  UniquePtr(const UniquePtr& aOther) = delete; 
  void operator=(const UniquePtr& aOther) = delete; 
};


template<typename T>
class DefaultDelete
{
public:
  MOZ_CONSTEXPR DefaultDelete() {}

  template<typename U>
  DefaultDelete(const DefaultDelete<U>& aOther,
                typename EnableIf<mozilla::IsConvertible<U*, T*>::value,
                                  int>::Type aDummy = 0)
  {}

  void operator()(T* aPtr) const
  {
    static_assert(sizeof(T) > 0, "T must be complete");
    delete aPtr;
  }
};


template<typename T>
class DefaultDelete<T[]>
{
public:
  MOZ_CONSTEXPR DefaultDelete() {}

  void operator()(T* aPtr) const
  {
    static_assert(sizeof(T) > 0, "T must be complete");
    delete[] aPtr;
  }

private:
  template<typename U>
  void operator()(U* aPtr) const = delete;
};

template<typename T, class D>
void
Swap(UniquePtr<T, D>& aX, UniquePtr<T, D>& aY)
{
  aX.swap(aY);
}

template<typename T, class D, typename U, class E>
bool
operator==(const UniquePtr<T, D>& aX, const UniquePtr<U, E>& aY)
{
  return aX.get() == aY.get();
}

template<typename T, class D, typename U, class E>
bool
operator!=(const UniquePtr<T, D>& aX, const UniquePtr<U, E>& aY)
{
  return aX.get() != aY.get();
}

template<typename T, class D>
bool
operator==(const UniquePtr<T, D>& aX, decltype(nullptr))
{
  return !aX;
}

template<typename T, class D>
bool
operator==(decltype(nullptr), const UniquePtr<T, D>& aX)
{
  return !aX;
}

template<typename T, class D>
bool
operator!=(const UniquePtr<T, D>& aX, decltype(nullptr))
{
  return bool(aX);
}

template<typename T, class D>
bool
operator!=(decltype(nullptr), const UniquePtr<T, D>& aX)
{
  return bool(aX);
}



namespace detail {

template<typename T>
struct UniqueSelector
{
  typedef UniquePtr<T> SingleObject;
};

template<typename T>
struct UniqueSelector<T[]>
{
  typedef UniquePtr<T[]> UnknownBound;
};

template<typename T, decltype(sizeof(int)) N>
struct UniqueSelector<T[N]>
{
  typedef UniquePtr<T[N]> KnownBound;
};

} 























































template<typename T, typename... Args>
typename detail::UniqueSelector<T>::SingleObject
MakeUnique(Args&&... aArgs)
{
  return UniquePtr<T>(new T(Forward<Args>(aArgs)...));
}

template<typename T>
typename detail::UniqueSelector<T>::UnknownBound
MakeUnique(decltype(sizeof(int)) aN)
{
  typedef typename RemoveExtent<T>::Type ArrayType;
  return UniquePtr<T>(new ArrayType[aN]());
}

template<typename T, typename... Args>
typename detail::UniqueSelector<T>::KnownBound
MakeUnique(Args&&... aArgs) = delete;

} 

#endif 
