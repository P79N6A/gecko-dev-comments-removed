





#ifndef nsAutoPtr_h
#define nsAutoPtr_h

#include "nsCOMPtr.h"
#include "nsRefPtr.h"

#include "nsCycleCollectionNoteChild.h"
#include "mozilla/MemoryReporting.h"





template <class T>
class nsAutoPtr
{
private:
  void**
  begin_assignment()
  {
    assign(0);
    return reinterpret_cast<void**>(&mRawPtr);
  }

  void
  assign(T* aNewPtr)
  {
    T* oldPtr = mRawPtr;

    if (aNewPtr && aNewPtr == oldPtr) {
      NS_RUNTIMEABORT("Logic flaw in the caller");
    }

    mRawPtr = aNewPtr;
    delete oldPtr;
  }

  
  
  
  
  
  class Ptr
  {
  public:
    MOZ_IMPLICIT Ptr(T* aPtr)
      : mPtr(aPtr)
    {
    }

    operator T*() const
    {
      return mPtr;
    }

  private:
    T* MOZ_NON_OWNING_REF mPtr;
  };

private:
  T* MOZ_OWNING_REF mRawPtr;

public:
  typedef T element_type;

  ~nsAutoPtr()
  {
    delete mRawPtr;
  }

  

  nsAutoPtr()
    : mRawPtr(0)
    
  {
  }

  MOZ_IMPLICIT nsAutoPtr(Ptr aRawPtr)
    : mRawPtr(aRawPtr)
    
  {
  }

  
  
  nsAutoPtr(nsAutoPtr<T>& aSmartPtr)
    : mRawPtr(aSmartPtr.forget())
    
  {
  }

  nsAutoPtr(nsAutoPtr<T>&& aSmartPtr)
    : mRawPtr(aSmartPtr.forget())
    
  {
  }

  

  nsAutoPtr<T>&
  operator=(T* aRhs)
  
  {
    assign(aRhs);
    return *this;
  }

  nsAutoPtr<T>& operator=(nsAutoPtr<T>& aRhs)
  
  {
    assign(aRhs.forget());
    return *this;
  }

  nsAutoPtr<T>& operator=(nsAutoPtr<T>&& aRhs)
  {
    assign(aRhs.forget());
    return *this;
  }

  

  T*
  get() const
  




  {
    return mRawPtr;
  }

  operator T*() const
  








  {
    return get();
  }

  T*
  forget()
  {
    T* temp = mRawPtr;
    mRawPtr = 0;
    return temp;
  }

  T*
  operator->() const
  {
    NS_PRECONDITION(mRawPtr != 0,
                    "You can't dereference a NULL nsAutoPtr with operator->().");
    return get();
  }

  
  
  
#ifndef _MSC_VER
  template <class U, class V>
  U&
  operator->*(U V::* aMember)
  {
    NS_PRECONDITION(mRawPtr != 0,
                    "You can't dereference a NULL nsAutoPtr with operator->*().");
    return get()->*aMember;
  }
#endif

  nsAutoPtr<T>*
  get_address()
  
  
  {
    return this;
  }

  const nsAutoPtr<T>*
  get_address() const
  
  
  {
    return this;
  }

public:
  T&
  operator*() const
  {
    NS_PRECONDITION(mRawPtr != 0,
                    "You can't dereference a NULL nsAutoPtr with operator*().");
    return *get();
  }

  T**
  StartAssignment()
  {
#ifndef NSCAP_FEATURE_INLINE_STARTASSIGNMENT
    return reinterpret_cast<T**>(begin_assignment());
#else
    assign(0);
    return reinterpret_cast<T**>(&mRawPtr);
#endif
  }
};

template <class T>
inline nsAutoPtr<T>*
address_of(nsAutoPtr<T>& aPtr)
{
  return aPtr.get_address();
}

template <class T>
inline const nsAutoPtr<T>*
address_of(const nsAutoPtr<T>& aPtr)
{
  return aPtr.get_address();
}

template <class T>
class nsAutoPtrGetterTransfers

















{
public:
  explicit
  nsAutoPtrGetterTransfers(nsAutoPtr<T>& aSmartPtr)
    : mTargetSmartPtr(aSmartPtr)
  {
    
  }

  operator void**()
  {
    return reinterpret_cast<void**>(mTargetSmartPtr.StartAssignment());
  }

  operator T**()
  {
    return mTargetSmartPtr.StartAssignment();
  }

  T*&
  operator*()
  {
    return *(mTargetSmartPtr.StartAssignment());
  }

private:
  nsAutoPtr<T>& mTargetSmartPtr;
};

template <class T>
inline nsAutoPtrGetterTransfers<T>
getter_Transfers(nsAutoPtr<T>& aSmartPtr)




{
  return nsAutoPtrGetterTransfers<T>(aSmartPtr);
}





template <class T, class U>
inline bool
operator==(const nsAutoPtr<T>& aLhs, const nsAutoPtr<U>& aRhs)
{
  return static_cast<const T*>(aLhs.get()) == static_cast<const U*>(aRhs.get());
}


template <class T, class U>
inline bool
operator!=(const nsAutoPtr<T>& aLhs, const nsAutoPtr<U>& aRhs)
{
  return static_cast<const T*>(aLhs.get()) != static_cast<const U*>(aRhs.get());
}




template <class T, class U>
inline bool
operator==(const nsAutoPtr<T>& aLhs, const U* aRhs)
{
  return static_cast<const T*>(aLhs.get()) == static_cast<const U*>(aRhs);
}

template <class T, class U>
inline bool
operator==(const U* aLhs, const nsAutoPtr<T>& aRhs)
{
  return static_cast<const U*>(aLhs) == static_cast<const T*>(aRhs.get());
}

template <class T, class U>
inline bool
operator!=(const nsAutoPtr<T>& aLhs, const U* aRhs)
{
  return static_cast<const T*>(aLhs.get()) != static_cast<const U*>(aRhs);
}

template <class T, class U>
inline bool
operator!=(const U* aLhs, const nsAutoPtr<T>& aRhs)
{
  return static_cast<const U*>(aLhs) != static_cast<const T*>(aRhs.get());
}

template <class T, class U>
inline bool
operator==(const nsAutoPtr<T>& aLhs, U* aRhs)
{
  return static_cast<const T*>(aLhs.get()) == const_cast<const U*>(aRhs);
}

template <class T, class U>
inline bool
operator==(U* aLhs, const nsAutoPtr<T>& aRhs)
{
  return const_cast<const U*>(aLhs) == static_cast<const T*>(aRhs.get());
}

template <class T, class U>
inline bool
operator!=(const nsAutoPtr<T>& aLhs, U* aRhs)
{
  return static_cast<const T*>(aLhs.get()) != const_cast<const U*>(aRhs);
}

template <class T, class U>
inline bool
operator!=(U* aLhs, const nsAutoPtr<T>& aRhs)
{
  return const_cast<const U*>(aLhs) != static_cast<const T*>(aRhs.get());
}





template <class T>
inline bool
operator==(const nsAutoPtr<T>& aLhs, NSCAP_Zero* aRhs)

{
  return static_cast<const void*>(aLhs.get()) == reinterpret_cast<const void*>(aRhs);
}

template <class T>
inline bool
operator==(NSCAP_Zero* aLhs, const nsAutoPtr<T>& aRhs)

{
  return reinterpret_cast<const void*>(aLhs) == static_cast<const void*>(aRhs.get());
}

template <class T>
inline bool
operator!=(const nsAutoPtr<T>& aLhs, NSCAP_Zero* aRhs)

{
  return static_cast<const void*>(aLhs.get()) != reinterpret_cast<const void*>(aRhs);
}

template <class T>
inline bool
operator!=(NSCAP_Zero* aLhs, const nsAutoPtr<T>& aRhs)

{
  return reinterpret_cast<const void*>(aLhs) != static_cast<const void*>(aRhs.get());
}






template <class T>
class nsAutoArrayPtr
{
private:
  void**
  begin_assignment()
  {
    assign(0);
    return reinterpret_cast<void**>(&mRawPtr);
  }

  void
  assign(T* aNewPtr)
  {
    T* oldPtr = mRawPtr;
    mRawPtr = aNewPtr;
    delete [] oldPtr;
  }

private:
  T* MOZ_OWNING_REF mRawPtr;

public:
  typedef T element_type;

  ~nsAutoArrayPtr()
  {
    delete [] mRawPtr;
  }

  

  nsAutoArrayPtr()
    : mRawPtr(0)
    
  {
  }

  MOZ_IMPLICIT nsAutoArrayPtr(T* aRawPtr)
    : mRawPtr(aRawPtr)
    
  {
  }

  nsAutoArrayPtr(nsAutoArrayPtr<T>& aSmartPtr)
    : mRawPtr(aSmartPtr.forget())
    
  {
  }


  

  nsAutoArrayPtr<T>&
  operator=(T* aRhs)
  
  {
    assign(aRhs);
    return *this;
  }

  nsAutoArrayPtr<T>& operator=(nsAutoArrayPtr<T>& aRhs)
  
  {
    assign(aRhs.forget());
    return *this;
  }

  

  T*
  get() const
  




  {
    return mRawPtr;
  }

  operator T*() const
  








  {
    return get();
  }

  T*
  forget()
  {
    T* temp = mRawPtr;
    mRawPtr = 0;
    return temp;
  }

  T*
  operator->() const
  {
    NS_PRECONDITION(mRawPtr != 0,
                    "You can't dereference a NULL nsAutoArrayPtr with operator->().");
    return get();
  }

  nsAutoArrayPtr<T>*
  get_address()
  
  
  {
    return this;
  }

  const nsAutoArrayPtr<T>*
  get_address() const
  
  
  {
    return this;
  }

public:
  T&
  operator*() const
  {
    NS_PRECONDITION(mRawPtr != 0,
                    "You can't dereference a NULL nsAutoArrayPtr with operator*().");
    return *get();
  }

  T**
  StartAssignment()
  {
#ifndef NSCAP_FEATURE_INLINE_STARTASSIGNMENT
    return reinterpret_cast<T**>(begin_assignment());
#else
    assign(0);
    return reinterpret_cast<T**>(&mRawPtr);
#endif
  }

  size_t
  SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    return aMallocSizeOf(mRawPtr);
  }

  size_t
  SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }
};

template <class T>
inline nsAutoArrayPtr<T>*
address_of(nsAutoArrayPtr<T>& aPtr)
{
  return aPtr.get_address();
}

template <class T>
inline const nsAutoArrayPtr<T>*
address_of(const nsAutoArrayPtr<T>& aPtr)
{
  return aPtr.get_address();
}

template <class T>
class nsAutoArrayPtrGetterTransfers

















{
public:
  explicit
  nsAutoArrayPtrGetterTransfers(nsAutoArrayPtr<T>& aSmartPtr)
    : mTargetSmartPtr(aSmartPtr)
  {
    
  }

  operator void**()
  {
    return reinterpret_cast<void**>(mTargetSmartPtr.StartAssignment());
  }

  operator T**()
  {
    return mTargetSmartPtr.StartAssignment();
  }

  T*&
  operator*()
  {
    return *(mTargetSmartPtr.StartAssignment());
  }

private:
  nsAutoArrayPtr<T>& mTargetSmartPtr;
};

template <class T>
inline nsAutoArrayPtrGetterTransfers<T>
getter_Transfers(nsAutoArrayPtr<T>& aSmartPtr)




{
  return nsAutoArrayPtrGetterTransfers<T>(aSmartPtr);
}





template <class T, class U>
inline bool
operator==(const nsAutoArrayPtr<T>& aLhs, const nsAutoArrayPtr<U>& aRhs)
{
  return static_cast<const T*>(aLhs.get()) == static_cast<const U*>(aRhs.get());
}


template <class T, class U>
inline bool
operator!=(const nsAutoArrayPtr<T>& aLhs, const nsAutoArrayPtr<U>& aRhs)
{
  return static_cast<const T*>(aLhs.get()) != static_cast<const U*>(aRhs.get());
}




template <class T, class U>
inline bool
operator==(const nsAutoArrayPtr<T>& aLhs, const U* aRhs)
{
  return static_cast<const T*>(aLhs.get()) == static_cast<const U*>(aRhs);
}

template <class T, class U>
inline bool
operator==(const U* aLhs, const nsAutoArrayPtr<T>& aRhs)
{
  return static_cast<const U*>(aLhs) == static_cast<const T*>(aRhs.get());
}

template <class T, class U>
inline bool
operator!=(const nsAutoArrayPtr<T>& aLhs, const U* aRhs)
{
  return static_cast<const T*>(aLhs.get()) != static_cast<const U*>(aRhs);
}

template <class T, class U>
inline bool
operator!=(const U* aLhs, const nsAutoArrayPtr<T>& aRhs)
{
  return static_cast<const U*>(aLhs) != static_cast<const T*>(aRhs.get());
}

template <class T, class U>
inline bool
operator==(const nsAutoArrayPtr<T>& aLhs, U* aRhs)
{
  return static_cast<const T*>(aLhs.get()) == const_cast<const U*>(aRhs);
}

template <class T, class U>
inline bool
operator==(U* aLhs, const nsAutoArrayPtr<T>& aRhs)
{
  return const_cast<const U*>(aLhs) == static_cast<const T*>(aRhs.get());
}

template <class T, class U>
inline bool
operator!=(const nsAutoArrayPtr<T>& aLhs, U* aRhs)
{
  return static_cast<const T*>(aLhs.get()) != const_cast<const U*>(aRhs);
}

template <class T, class U>
inline bool
operator!=(U* aLhs, const nsAutoArrayPtr<T>& aRhs)
{
  return const_cast<const U*>(aLhs) != static_cast<const T*>(aRhs.get());
}





template <class T>
inline bool
operator==(const nsAutoArrayPtr<T>& aLhs, NSCAP_Zero* aRhs)

{
  return static_cast<const void*>(aLhs.get()) == reinterpret_cast<const void*>(aRhs);
}

template <class T>
inline bool
operator==(NSCAP_Zero* aLhs, const nsAutoArrayPtr<T>& aRhs)

{
  return reinterpret_cast<const void*>(aLhs) == static_cast<const void*>(aRhs.get());
}

template <class T>
inline bool
operator!=(const nsAutoArrayPtr<T>& aLhs, NSCAP_Zero* aRhs)

{
  return static_cast<const void*>(aLhs.get()) != reinterpret_cast<const void*>(aRhs);
}

template <class T>
inline bool
operator!=(NSCAP_Zero* aLhs, const nsAutoArrayPtr<T>& aRhs)

{
  return reinterpret_cast<const void*>(aLhs) != static_cast<const void*>(aRhs.get());
}




#endif 
