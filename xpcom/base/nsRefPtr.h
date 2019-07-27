





#ifndef nsRefPtr_h
#define nsRefPtr_h

#include "mozilla/AlreadyAddRefed.h"
#include "mozilla/Attributes.h"
#include "nsDebug.h"
#include "nsISupportsUtils.h"





class nsCOMPtr_helper;

namespace mozilla {
namespace dom {
template<class T> class OwningNonNull;
} 
} 

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
  T* MOZ_OWNING_REF mRawPtr;

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

  

  MOZ_IMPLICIT nsRefPtr(T* aRawPtr)
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

  template <typename I>
  nsRefPtr(nsRefPtr<I>&& aSmartPtr)
    : mRawPtr(aSmartPtr.forget().take())
    
  {
  }

  MOZ_IMPLICIT nsRefPtr(const nsCOMPtr_helper& aHelper);

  
  template<class U>
  MOZ_IMPLICIT nsRefPtr(const mozilla::dom::OwningNonNull<U>& aOther);

  

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

  nsRefPtr<T>& operator=(const nsCOMPtr_helper& aHelper);

  nsRefPtr<T>&
  operator=(nsRefPtr<T> && aRefPtr)
  {
    assign_assuming_AddRef(aRefPtr.mRawPtr);
    aRefPtr.mRawPtr = nullptr;
    return *this;
  }

  
  template<class U>
  nsRefPtr<T>&
  operator=(const mozilla::dom::OwningNonNull<U>& aOther);

  

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
  operator->() const MOZ_NO_ADDREF_RELEASE_ON_RETURN
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
    assign_assuming_AddRef(0);
    return reinterpret_cast<T**>(&mRawPtr);
  }
};

template <class T>
nsRefPtr<T>::nsRefPtr(const nsCOMPtr_helper& aHelper)
{
  void* newRawPtr;
  if (NS_FAILED(aHelper(NS_GET_TEMPLATE_IID(T), &newRawPtr))) {
    newRawPtr = 0;
  }
  mRawPtr = static_cast<T*>(newRawPtr);
}

template <class T>
nsRefPtr<T>&
nsRefPtr<T>::operator=(const nsCOMPtr_helper& aHelper)
{
  void* newRawPtr;
  if (NS_FAILED(aHelper(NS_GET_TEMPLATE_IID(T), &newRawPtr))) {
    newRawPtr = 0;
  }
  assign_assuming_AddRef(static_cast<T*>(newRawPtr));
  return *this;
}

class nsCycleCollectionTraversalCallback;
template <typename T>
void
CycleCollectionNoteChild(nsCycleCollectionTraversalCallback& aCallback,
                         T* aChild, const char* aName, uint32_t aFlags);

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

namespace detail {
class nsRefPtrZero;
}



template <class T>
inline bool
operator==(const nsRefPtr<T>& aLhs, ::detail::nsRefPtrZero* aRhs)

{
  return static_cast<const void*>(aLhs.get()) == reinterpret_cast<const void*>(aRhs);
}

template <class T>
inline bool
operator==(::detail::nsRefPtrZero* aLhs, const nsRefPtr<T>& aRhs)

{
  return reinterpret_cast<const void*>(aLhs) == static_cast<const void*>(aRhs.get());
}

template <class T>
inline bool
operator!=(const nsRefPtr<T>& aLhs, ::detail::nsRefPtrZero* aRhs)

{
  return static_cast<const void*>(aLhs.get()) != reinterpret_cast<const void*>(aRhs);
}

template <class T>
inline bool
operator!=(::detail::nsRefPtrZero* aLhs, const nsRefPtr<T>& aRhs)

{
  return reinterpret_cast<const void*>(aLhs) != static_cast<const void*>(aRhs.get());
}


template <class SourceType, class DestinationType>
inline nsresult
CallQueryInterface(nsRefPtr<SourceType>& aSourcePtr, DestinationType** aDestPtr)
{
  return CallQueryInterface(aSourcePtr.get(), aDestPtr);
}



#endif 
