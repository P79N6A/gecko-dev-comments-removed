





#ifndef mozilla_StaticPtr_h
#define mozilla_StaticPtr_h

#include "mozilla/Assertions.h"
#include "mozilla/NullPtr.h"

namespace mozilla {























template<class T>
class StaticAutoPtr
{
public:
  
  
  
#ifdef DEBUG
  StaticAutoPtr()
  {
    MOZ_ASSERT(!mRawPtr);
  }
#endif

  StaticAutoPtr<T>& operator=(T* aRhs)
  {
    Assign(aRhs);
    return *this;
  }

  T* get() const { return mRawPtr; }

  operator T*() const { return get(); }

  T* operator->() const
  {
    MOZ_ASSERT(mRawPtr);
    return get();
  }

  T& operator*() const { return *get(); }

private:
  
  
  
  
#ifdef DEBUG
  StaticAutoPtr(StaticAutoPtr<T>& aOther);
#endif

  void Assign(T* aNewPtr)
  {
    MOZ_ASSERT(!aNewPtr || mRawPtr != aNewPtr);
    T* oldPtr = mRawPtr;
    mRawPtr = aNewPtr;
    delete oldPtr;
  }

  T* mRawPtr;
};

template<class T>
class StaticRefPtr
{
public:
  
  
  
#ifdef DEBUG
  StaticRefPtr()
  {
    MOZ_ASSERT(!mRawPtr);
  }
#endif

  StaticRefPtr<T>& operator=(T* aRhs)
  {
    AssignWithAddref(aRhs);
    return *this;
  }

  StaticRefPtr<T>& operator=(const StaticRefPtr<T>& aRhs)
  {
    return (this = aRhs.mRawPtr);
  }

  T* get() const { return mRawPtr; }

  operator T*() const { return get(); }

  T* operator->() const
  {
    MOZ_ASSERT(mRawPtr);
    return get();
  }

  T& operator*() const { return *get(); }

private:
  void AssignWithAddref(T* aNewPtr)
  {
    if (aNewPtr) {
      aNewPtr->AddRef();
    }
    AssignAssumingAddRef(aNewPtr);
  }

  void AssignAssumingAddRef(T* aNewPtr)
  {
    T* oldPtr = mRawPtr;
    mRawPtr = aNewPtr;
    if (oldPtr) {
      oldPtr->Release();
    }
  }

  T* mRawPtr;
};

namespace StaticPtr_internal {
class Zero;
} 

#define REFLEXIVE_EQUALITY_OPERATORS(type1, type2, eq_fn, ...) \
  template<__VA_ARGS__>                                        \
  inline bool                                                  \
  operator==(type1 lhs, type2 rhs)                             \
  {                                                            \
    return eq_fn;                                              \
  }                                                            \
                                                               \
  template<__VA_ARGS__>                                        \
  inline bool                                                  \
  operator==(type2 lhs, type1 rhs)                             \
  {                                                            \
    return rhs == lhs;                                         \
  }                                                            \
                                                               \
  template<__VA_ARGS__>                                        \
  inline bool                                                  \
  operator!=(type1 lhs, type2 rhs)                             \
  {                                                            \
    return !(lhs == rhs);                                      \
  }                                                            \
                                                               \
  template<__VA_ARGS__>                                        \
  inline bool                                                  \
  operator!=(type2 lhs, type1 rhs)                             \
  {                                                            \
    return !(lhs == rhs);                                      \
  }



template<class T, class U>
inline bool
operator==(const StaticAutoPtr<T>& aLhs, const StaticAutoPtr<U>& aRhs)
{
  return aLhs.get() == aRhs.get();
}

template<class T, class U>
inline bool
operator!=(const StaticAutoPtr<T>& aLhs, const StaticAutoPtr<U>& aRhs)
{
  return !(aLhs == aRhs);
}

REFLEXIVE_EQUALITY_OPERATORS(const StaticAutoPtr<T>&, const U*,
                             lhs.get() == rhs, class T, class U)

REFLEXIVE_EQUALITY_OPERATORS(const StaticAutoPtr<T>&, U*,
                             lhs.get() == rhs, class T, class U)


REFLEXIVE_EQUALITY_OPERATORS(const StaticAutoPtr<T>&, StaticPtr_internal::Zero*,
                             lhs.get() == nullptr, class T)



template<class T, class U>
inline bool
operator==(const StaticRefPtr<T>& aLhs, const StaticRefPtr<U>& aRhs)
{
  return aLhs.get() == aRhs.get();
}

template<class T, class U>
inline bool
operator!=(const StaticRefPtr<T>& aLhs, const StaticRefPtr<U>& aRhs)
{
  return !(aLhs == aRhs);
}

REFLEXIVE_EQUALITY_OPERATORS(const StaticRefPtr<T>&, const U*,
                             lhs.get() == rhs, class T, class U)

REFLEXIVE_EQUALITY_OPERATORS(const StaticRefPtr<T>&, U*,
                             lhs.get() == rhs, class T, class U)


REFLEXIVE_EQUALITY_OPERATORS(const StaticRefPtr<T>&, StaticPtr_internal::Zero*,
                             lhs.get() == nullptr, class T)

#undef REFLEXIVE_EQUALITY_OPERATORS

} 

#endif
