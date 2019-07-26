





#ifndef mozilla_StaticPtr_h
#define mozilla_StaticPtr_h

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

    StaticAutoPtr<T>& operator=(T* rhs)
    {
      Assign(rhs);
      return *this;
    }

    T* get() const
    {
      return mRawPtr;
    }

    operator T*() const
    {
      return get();
    }

    T* operator->() const
    {
      MOZ_ASSERT(mRawPtr);
      return get();
    }

    T& operator*() const
    {
      return *get();
    }

  private:
    
    
    
    
#ifdef DEBUG
    StaticAutoPtr(StaticAutoPtr<T> &other);
#endif

    void Assign(T* newPtr)
    {
      MOZ_ASSERT(!newPtr || mRawPtr != newPtr);
      T* oldPtr = mRawPtr;
      mRawPtr = newPtr;
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

  StaticRefPtr<T>& operator=(T* rhs)
  {
    AssignWithAddref(rhs);
    return *this;
  }

  StaticRefPtr<T>& operator=(const StaticRefPtr<T>& rhs)
  {
    return (this = rhs.mRawPtr);
  }

  T* get() const
  {
    return mRawPtr;
  }

  operator T*() const
  {
    return get();
  }

  T* operator->() const
  {
    MOZ_ASSERT(mRawPtr);
    return get();
  }

  T& operator*() const
  {
    return *get();
  }

private:
  void AssignWithAddref(T* newPtr)
  {
    if (newPtr) {
      newPtr->AddRef();
    }
    AssignAssumingAddRef(newPtr);
  }

  void AssignAssumingAddRef(T* newPtr)
  {
    T* oldPtr = mRawPtr;
    mRawPtr = newPtr;
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
operator==(const StaticAutoPtr<T>& lhs, const StaticAutoPtr<U>& rhs)
{
  return lhs.get() == rhs.get();
}

template<class T, class U>
inline bool
operator!=(const StaticAutoPtr<T>& lhs, const StaticAutoPtr<U>& rhs)
{
  return !(lhs == rhs);
}

REFLEXIVE_EQUALITY_OPERATORS(const StaticAutoPtr<T>&, const U*,
                             lhs.get() == rhs, class T, class U)

REFLEXIVE_EQUALITY_OPERATORS(const StaticAutoPtr<T>&, U*,
                             lhs.get() == rhs, class T, class U)


REFLEXIVE_EQUALITY_OPERATORS(const StaticAutoPtr<T>&, StaticPtr_internal::Zero*,
                             lhs.get() == NULL, class T)



template<class T, class U>
inline bool
operator==(const StaticRefPtr<T>& lhs, const StaticRefPtr<U>& rhs)
{
  return lhs.get() == rhs.get();
}

template<class T, class U>
inline bool
operator!=(const StaticRefPtr<T>& lhs, const StaticRefPtr<U>& rhs)
{
  return !(lhs == rhs);
}

REFLEXIVE_EQUALITY_OPERATORS(const StaticRefPtr<T>&, const U*,
                             lhs.get() == rhs, class T, class U)

REFLEXIVE_EQUALITY_OPERATORS(const StaticRefPtr<T>&, U*,
                             lhs.get() == rhs, class T, class U)


REFLEXIVE_EQUALITY_OPERATORS(const StaticRefPtr<T>&, StaticPtr_internal::Zero*,
                             lhs.get() == NULL, class T)

#undef REFLEXIVE_EQUALITY_OPERATORS

} 

#endif
