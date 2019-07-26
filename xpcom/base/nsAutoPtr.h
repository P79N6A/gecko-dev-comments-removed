





#ifndef nsAutoPtr_h___
#define nsAutoPtr_h___

#include "nsCOMPtr.h"

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
    Ptr(T* aPtr)
      : mPtr(aPtr)
    {
    }

    operator T*() const
    {
      return mPtr;
    }

  private:
    T* mPtr;
  };

private:
  T* mRawPtr;

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

  nsAutoPtr(Ptr aRawPtr)
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








#ifndef NSCAP_DONT_PROVIDE_NONCONST_OPEQ
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
#endif





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


#ifdef HAVE_CPP_TROUBLE_COMPARING_TO_ZERO




template <class T>
inline bool
operator==(const nsAutoPtr<T>& aLhs, int aRhs)

{
  return static_cast<const void*>(aLhs.get()) == reinterpret_cast<const void*>(aRhs);
}

template <class T>
inline bool
operator==(int aLhs, const nsAutoPtr<T>& aRhs)

{
  return reinterpret_cast<const void*>(aLhs) == static_cast<const void*>(aRhs.get());
}

#endif 





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
  T* mRawPtr;

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

  nsAutoArrayPtr(T* aRawPtr)
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








#ifndef NSCAP_DONT_PROVIDE_NONCONST_OPEQ
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
#endif





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


#ifdef HAVE_CPP_TROUBLE_COMPARING_TO_ZERO




template <class T>
inline bool
operator==(const nsAutoArrayPtr<T>& aLhs, int aRhs)

{
  return static_cast<const void*>(aLhs.get()) == reinterpret_cast<const void*>(aRhs);
}

template <class T>
inline bool
operator==(int aLhs, const nsAutoArrayPtr<T>& aRhs)

{
  return reinterpret_cast<const void*>(aLhs) == static_cast<const void*>(aRhs.get());
}

#endif 






template <class T>
class nsRefPtr
{
private:

  void
  assign_with_AddRef(T* aRawPtr)
  {
    if (aRawPtr) {
      aRawPtr->AddRef();
    }
    assign_assuming_AddRef(aRawPtr);
  }

  void**
  begin_assignment()
  {
    assign_assuming_AddRef(0);
    return reinterpret_cast<void**>(&mRawPtr);
  }

  void
  assign_assuming_AddRef(T* aNewPtr)
  {
    T* oldPtr = mRawPtr;
    mRawPtr = aNewPtr;
    if (oldPtr) {
      oldPtr->Release();
    }
  }

private:
  T* mRawPtr;

public:
  typedef T element_type;

  ~nsRefPtr()
  {
    if (mRawPtr) {
      mRawPtr->Release();
    }
  }

  

  nsRefPtr()
    : mRawPtr(0)
    
  {
  }

  nsRefPtr(const nsRefPtr<T>& aSmartPtr)
    : mRawPtr(aSmartPtr.mRawPtr)
    
  {
    if (mRawPtr) {
      mRawPtr->AddRef();
    }
  }

  nsRefPtr(nsRefPtr<T>&& aRefPtr)
    : mRawPtr(aRefPtr.mRawPtr)
  {
    aRefPtr.mRawPtr = nullptr;
  }

  

  nsRefPtr(T* aRawPtr)
    : mRawPtr(aRawPtr)
  {
    if (mRawPtr) {
      mRawPtr->AddRef();
    }
  }

  template <typename I>
  nsRefPtr(already_AddRefed<I>& aSmartPtr)
    : mRawPtr(aSmartPtr.take())
    
  {
  }

  template <typename I>
  nsRefPtr(already_AddRefed<I>&& aSmartPtr)
    : mRawPtr(aSmartPtr.take())
    
  {
  }

  nsRefPtr(const nsCOMPtr_helper& aHelper)
  {
    void* newRawPtr;
    if (NS_FAILED(aHelper(NS_GET_TEMPLATE_IID(T), &newRawPtr))) {
      newRawPtr = 0;
    }
    mRawPtr = static_cast<T*>(newRawPtr);
  }

  

  nsRefPtr<T>&
  operator=(const nsRefPtr<T>& aRhs)
  
  {
    assign_with_AddRef(aRhs.mRawPtr);
    return *this;
  }

  nsRefPtr<T>&
  operator=(T* aRhs)
  
  {
    assign_with_AddRef(aRhs);
    return *this;
  }

  template <typename I>
  nsRefPtr<T>&
  operator=(already_AddRefed<I>& aRhs)
  
  {
    assign_assuming_AddRef(aRhs.take());
    return *this;
  }

  template <typename I>
  nsRefPtr<T>&
  operator=(already_AddRefed<I> && aRhs)
  
  {
    assign_assuming_AddRef(aRhs.take());
    return *this;
  }

  nsRefPtr<T>&
  operator=(const nsCOMPtr_helper& aHelper)
  {
    void* newRawPtr;
    if (NS_FAILED(aHelper(NS_GET_TEMPLATE_IID(T), &newRawPtr))) {
      newRawPtr = 0;
    }
    assign_assuming_AddRef(static_cast<T*>(newRawPtr));
    return *this;
  }

  nsRefPtr<T>&
  operator=(nsRefPtr<T> && aRefPtr)
  {
    assign_assuming_AddRef(aRefPtr.mRawPtr);
    aRefPtr.mRawPtr = nullptr;
    return *this;
  }

  

  void
  swap(nsRefPtr<T>& aRhs)
  
  {
    T* temp = aRhs.mRawPtr;
    aRhs.mRawPtr = mRawPtr;
    mRawPtr = temp;
  }

  void
  swap(T*& aRhs)
  
  {
    T* temp = aRhs;
    aRhs = mRawPtr;
    mRawPtr = temp;
  }

  already_AddRefed<T>
  forget()
  
  
  {
    T* temp = 0;
    swap(temp);
    return already_AddRefed<T>(temp);
  }

  template <typename I>
  void
  forget(I** aRhs)
  
  
  
  
  {
    NS_ASSERTION(aRhs, "Null pointer passed to forget!");
    *aRhs = mRawPtr;
    mRawPtr = 0;
  }

  T*
  get() const
  



  {
    return const_cast<T*>(mRawPtr);
  }

  operator T*() const
  







  {
    return get();
  }

  T*
  operator->() const
  {
    NS_PRECONDITION(mRawPtr != 0,
                    "You can't dereference a NULL nsRefPtr with operator->().");
    return get();
  }

  
  
  
#ifndef _MSC_VER
  template <class U, class V>
  U&
  operator->*(U V::* aMember)
  {
    NS_PRECONDITION(mRawPtr != 0,
                    "You can't dereference a NULL nsRefPtr with operator->*().");
    return get()->*aMember;
  }
#endif

  nsRefPtr<T>*
  get_address()
  
  
  {
    return this;
  }

  const nsRefPtr<T>*
  get_address() const
  
  
  {
    return this;
  }

public:
  T&
  operator*() const
  {
    NS_PRECONDITION(mRawPtr != 0,
                    "You can't dereference a NULL nsRefPtr with operator*().");
    return *get();
  }

  T**
  StartAssignment()
  {
#ifndef NSCAP_FEATURE_INLINE_STARTASSIGNMENT
    return reinterpret_cast<T**>(begin_assignment());
#else
    assign_assuming_AddRef(0);
    return reinterpret_cast<T**>(&mRawPtr);
#endif
  }
};

template <typename T>
inline void
ImplCycleCollectionUnlink(nsRefPtr<T>& aField)
{
  aField = nullptr;
}

template <typename T>
inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            nsRefPtr<T>& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  CycleCollectionNoteChild(aCallback, aField.get(), aName, aFlags);
}

template <class T>
inline nsRefPtr<T>*
address_of(nsRefPtr<T>& aPtr)
{
  return aPtr.get_address();
}

template <class T>
inline const nsRefPtr<T>*
address_of(const nsRefPtr<T>& aPtr)
{
  return aPtr.get_address();
}

template <class T>
class nsRefPtrGetterAddRefs

















{
public:
  explicit
  nsRefPtrGetterAddRefs(nsRefPtr<T>& aSmartPtr)
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
  nsRefPtr<T>& mTargetSmartPtr;
};

template <class T>
inline nsRefPtrGetterAddRefs<T>
getter_AddRefs(nsRefPtr<T>& aSmartPtr)




{
  return nsRefPtrGetterAddRefs<T>(aSmartPtr);
}





template <class T, class U>
inline bool
operator==(const nsRefPtr<T>& aLhs, const nsRefPtr<U>& aRhs)
{
  return static_cast<const T*>(aLhs.get()) == static_cast<const U*>(aRhs.get());
}


template <class T, class U>
inline bool
operator!=(const nsRefPtr<T>& aLhs, const nsRefPtr<U>& aRhs)
{
  return static_cast<const T*>(aLhs.get()) != static_cast<const U*>(aRhs.get());
}




template <class T, class U>
inline bool
operator==(const nsRefPtr<T>& aLhs, const U* aRhs)
{
  return static_cast<const T*>(aLhs.get()) == static_cast<const U*>(aRhs);
}

template <class T, class U>
inline bool
operator==(const U* aLhs, const nsRefPtr<T>& aRhs)
{
  return static_cast<const U*>(aLhs) == static_cast<const T*>(aRhs.get());
}

template <class T, class U>
inline bool
operator!=(const nsRefPtr<T>& aLhs, const U* aRhs)
{
  return static_cast<const T*>(aLhs.get()) != static_cast<const U*>(aRhs);
}

template <class T, class U>
inline bool
operator!=(const U* aLhs, const nsRefPtr<T>& aRhs)
{
  return static_cast<const U*>(aLhs) != static_cast<const T*>(aRhs.get());
}








#ifndef NSCAP_DONT_PROVIDE_NONCONST_OPEQ
template <class T, class U>
inline bool
operator==(const nsRefPtr<T>& aLhs, U* aRhs)
{
  return static_cast<const T*>(aLhs.get()) == const_cast<const U*>(aRhs);
}

template <class T, class U>
inline bool
operator==(U* aLhs, const nsRefPtr<T>& aRhs)
{
  return const_cast<const U*>(aLhs) == static_cast<const T*>(aRhs.get());
}

template <class T, class U>
inline bool
operator!=(const nsRefPtr<T>& aLhs, U* aRhs)
{
  return static_cast<const T*>(aLhs.get()) != const_cast<const U*>(aRhs);
}

template <class T, class U>
inline bool
operator!=(U* aLhs, const nsRefPtr<T>& aRhs)
{
  return const_cast<const U*>(aLhs) != static_cast<const T*>(aRhs.get());
}
#endif





template <class T>
inline bool
operator==(const nsRefPtr<T>& aLhs, NSCAP_Zero* aRhs)

{
  return static_cast<const void*>(aLhs.get()) == reinterpret_cast<const void*>(aRhs);
}

template <class T>
inline bool
operator==(NSCAP_Zero* aLhs, const nsRefPtr<T>& aRhs)

{
  return reinterpret_cast<const void*>(aLhs) == static_cast<const void*>(aRhs.get());
}

template <class T>
inline bool
operator!=(const nsRefPtr<T>& aLhs, NSCAP_Zero* aRhs)

{
  return static_cast<const void*>(aLhs.get()) != reinterpret_cast<const void*>(aRhs);
}

template <class T>
inline bool
operator!=(NSCAP_Zero* aLhs, const nsRefPtr<T>& aRhs)

{
  return reinterpret_cast<const void*>(aLhs) != static_cast<const void*>(aRhs.get());
}


#ifdef HAVE_CPP_TROUBLE_COMPARING_TO_ZERO




template <class T>
inline bool
operator==(const nsRefPtr<T>& aLhs, int aRhs)

{
  return static_cast<const void*>(aLhs.get()) == reinterpret_cast<const void*>(aRhs);
}

template <class T>
inline bool
operator==(int aLhs, const nsRefPtr<T>& aRhs)

{
  return reinterpret_cast<const void*>(aLhs) == static_cast<const void*>(aRhs.get());
}

#endif 

template <class SourceType, class DestinationType>
inline nsresult
CallQueryInterface(nsRefPtr<SourceType>& aSourcePtr, DestinationType** aDestPtr)
{
  return CallQueryInterface(aSourcePtr.get(), aDestPtr);
}



template<class T>
class nsQueryObject : public nsCOMPtr_helper
{
public:
  nsQueryObject(T* aRawPtr)
    : mRawPtr(aRawPtr)
  {
  }

  virtual nsresult NS_FASTCALL operator()(const nsIID& aIID,
                                          void** aResult) const
  {
    nsresult status = mRawPtr ? mRawPtr->QueryInterface(aIID, aResult)
                              : NS_ERROR_NULL_POINTER;
    return status;
  }
private:
  T* mRawPtr;
};

template<class T>
class nsQueryObjectWithError : public nsCOMPtr_helper
{
public:
  nsQueryObjectWithError(T* aRawPtr, nsresult* aErrorPtr)
    : mRawPtr(aRawPtr), mErrorPtr(aErrorPtr)
  {
  }

  virtual nsresult NS_FASTCALL operator()(const nsIID& aIID,
                                          void** aResult) const
  {
    nsresult status = mRawPtr ? mRawPtr->QueryInterface(aIID, aResult)
                              : NS_ERROR_NULL_POINTER;
    if (mErrorPtr) {
      *mErrorPtr = status;
    }
    return status;
  }
private:
  T* mRawPtr;
  nsresult* mErrorPtr;
};

template<class T>
inline nsQueryObject<T>
do_QueryObject(T* aRawPtr)
{
  return nsQueryObject<T>(aRawPtr);
}

template<class T>
inline nsQueryObject<T>
do_QueryObject(nsCOMPtr<T>& aRawPtr)
{
  return nsQueryObject<T>(aRawPtr);
}

template<class T>
inline nsQueryObject<T>
do_QueryObject(nsRefPtr<T>& aRawPtr)
{
  return nsQueryObject<T>(aRawPtr);
}

template<class T>
inline nsQueryObjectWithError<T>
do_QueryObject(T* aRawPtr, nsresult* aErrorPtr)
{
  return nsQueryObjectWithError<T>(aRawPtr, aErrorPtr);
}

template<class T>
inline nsQueryObjectWithError<T>
do_QueryObject(nsCOMPtr<T>& aRawPtr, nsresult* aErrorPtr)
{
  return nsQueryObjectWithError<T>(aRawPtr, aErrorPtr);
}

template<class T>
inline nsQueryObjectWithError<T>
do_QueryObject(nsRefPtr<T>& aRawPtr, nsresult* aErrorPtr)
{
  return nsQueryObjectWithError<T>(aRawPtr, aErrorPtr);
}



#endif 
