





#ifndef nsCOMPtr_h___
#define nsCOMPtr_h___














#include "mozilla/AlreadyAddRefed.h"
#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Move.h"
#include "mozilla/TypeTraits.h"

#include "nsDebug.h" 
#include "nsISupportsUtils.h" 

#include "nsCycleCollectionNoteChild.h"










#ifdef _MSC_VER
  
  #define NSCAP_FEATURE_INLINE_STARTASSIGNMENT




  #pragma warning( disable: 4514 )
#endif

#define NSCAP_FEATURE_USE_BASE

#ifdef DEBUG
  #define NSCAP_FEATURE_TEST_DONTQUERY_CASES
  #undef NSCAP_FEATURE_USE_BASE

#endif

#ifdef __GNUC__
  
  
  
  
  

  #define NS_MAY_ALIAS_PTR(t)    t*  __attribute__((__may_alias__))
#else
  #define NS_MAY_ALIAS_PTR(t)    t*
#endif

#if defined(NSCAP_DISABLE_DEBUG_PTR_TYPES)
  #define NSCAP_FEATURE_USE_BASE
#endif












#ifndef NSCAP_ADDREF
  #define NSCAP_ADDREF(this, ptr)     (ptr)->AddRef()
#endif

#ifndef NSCAP_RELEASE
  #define NSCAP_RELEASE(this, ptr)    (ptr)->Release()
#endif


#ifdef NSCAP_LOG_ASSIGNMENT
  
  
  
  #define NSCAP_LOG_EXTERNAL_ASSIGNMENT
#else
    
  #define NSCAP_LOG_ASSIGNMENT(this, ptr)
#endif

#ifndef NSCAP_LOG_RELEASE
  #define NSCAP_LOG_RELEASE(this, ptr)
#endif

namespace mozilla {
namespace dom {
template<class T> class OwningNonNull;
} 
} 

template<class T>
inline already_AddRefed<T>
dont_AddRef(T* aRawPtr)
{
  return already_AddRefed<T>(aRawPtr);
}

template<class T>
inline already_AddRefed<T>&&
dont_AddRef(already_AddRefed<T>&& aAlreadyAddRefedPtr)
{
  return mozilla::Move(aAlreadyAddRefedPtr);
}


















class MOZ_STACK_CLASS nsCOMPtr_helper
{
public:
  virtual nsresult NS_FASTCALL operator()(const nsIID&, void**) const = 0;
};







class MOZ_STACK_CLASS nsQueryInterface final
{
public:
  explicit
  nsQueryInterface(nsISupports* aRawPtr) : mRawPtr(aRawPtr) {}

  nsresult NS_FASTCALL operator()(const nsIID& aIID, void**) const;

private:
  nsISupports* MOZ_OWNING_REF mRawPtr;
};

class nsQueryInterfaceWithError final
{
public:
  nsQueryInterfaceWithError(nsISupports* aRawPtr, nsresult* aError)
    : mRawPtr(aRawPtr)
    , mErrorPtr(aError)
  {
  }

  nsresult NS_FASTCALL operator()(const nsIID& aIID, void**) const;

private:
  nsISupports* MOZ_OWNING_REF mRawPtr;
  nsresult* mErrorPtr;
};

inline nsQueryInterface
do_QueryInterface(nsISupports* aRawPtr)
{
  return nsQueryInterface(aRawPtr);
}

inline nsQueryInterfaceWithError
do_QueryInterface(nsISupports* aRawPtr, nsresult* aError)
{
  return nsQueryInterfaceWithError(aRawPtr, aError);
}

template<class T>
inline void
do_QueryInterface(already_AddRefed<T>&)
{
  
  
  
}

template<class T>
inline void
do_QueryInterface(already_AddRefed<T>&, nsresult*)
{
  
  
  
}




class nsGetServiceByCID final
{
public:
  explicit nsGetServiceByCID(const nsCID& aCID) : mCID(aCID) {}

  nsresult NS_FASTCALL operator()(const nsIID&, void**) const;

private:
  const nsCID& mCID;
};

class nsGetServiceByCIDWithError final
{
public:
  nsGetServiceByCIDWithError(const nsCID& aCID, nsresult* aErrorPtr)
    : mCID(aCID)
    , mErrorPtr(aErrorPtr)
  {
  }

  nsresult NS_FASTCALL operator()(const nsIID&, void**) const;

private:
  const nsCID& mCID;
  nsresult* mErrorPtr;
};

class nsGetServiceByContractID final
{
public:
  explicit nsGetServiceByContractID(const char* aContractID)
    : mContractID(aContractID)
  {
  }

  nsresult NS_FASTCALL operator()(const nsIID&, void**) const;

private:
  const char* mContractID;
};

class nsGetServiceByContractIDWithError final
{
public:
  nsGetServiceByContractIDWithError(const char* aContractID, nsresult* aErrorPtr)
    : mContractID(aContractID)
    , mErrorPtr(aErrorPtr)
  {
  }

  nsresult NS_FASTCALL operator()(const nsIID&, void**) const;

private:
  const char* mContractID;
  nsresult* mErrorPtr;
};










class nsCOMPtr_base
{
public:
  explicit nsCOMPtr_base(nsISupports* aRawPtr = 0) : mRawPtr(aRawPtr) {}

  NS_CONSTRUCTOR_FASTCALL ~nsCOMPtr_base()
  {
    NSCAP_LOG_RELEASE(this, mRawPtr);
    if (mRawPtr) {
      NSCAP_RELEASE(this, mRawPtr);
    }
  }

  void NS_FASTCALL
  assign_with_AddRef(nsISupports*);
  void NS_FASTCALL
  assign_from_qi(const nsQueryInterface, const nsIID&);
  void NS_FASTCALL
  assign_from_qi_with_error(const nsQueryInterfaceWithError&, const nsIID&);
  void NS_FASTCALL
  assign_from_gs_cid(const nsGetServiceByCID, const nsIID&);
  void NS_FASTCALL
  assign_from_gs_cid_with_error(const nsGetServiceByCIDWithError&, const nsIID&);
  void NS_FASTCALL
  assign_from_gs_contractid(const nsGetServiceByContractID, const nsIID&);
  void NS_FASTCALL
  assign_from_gs_contractid_with_error(const nsGetServiceByContractIDWithError&,
                                       const nsIID&);
  void NS_FASTCALL
  assign_from_helper(const nsCOMPtr_helper&, const nsIID&);
  void** NS_FASTCALL
  begin_assignment();

protected:
  NS_MAY_ALIAS_PTR(nsISupports) MOZ_OWNING_REF mRawPtr;

  void assign_assuming_AddRef(nsISupports* aNewPtr)
  {
    
    
    
    
    
    
    nsISupports* oldPtr = mRawPtr;
    mRawPtr = aNewPtr;
    NSCAP_LOG_ASSIGNMENT(this, aNewPtr);
    NSCAP_LOG_RELEASE(this, oldPtr);
    if (oldPtr) {
      NSCAP_RELEASE(this, oldPtr);
    }
  }
};



template<class T>
class nsCOMPtr final
#ifdef NSCAP_FEATURE_USE_BASE
  : private nsCOMPtr_base
#endif
{

#ifdef NSCAP_FEATURE_USE_BASE
  #define NSCAP_CTOR_BASE(x) nsCOMPtr_base(x)
#else
  #define NSCAP_CTOR_BASE(x) mRawPtr(x)

private:
  void assign_with_AddRef(nsISupports*);
  void assign_from_qi(const nsQueryInterface, const nsIID&);
  void assign_from_qi_with_error(const nsQueryInterfaceWithError&, const nsIID&);
  void assign_from_gs_cid(const nsGetServiceByCID, const nsIID&);
  void assign_from_gs_cid_with_error(const nsGetServiceByCIDWithError&,
                                     const nsIID&);
  void assign_from_gs_contractid(const nsGetServiceByContractID, const nsIID&);
  void assign_from_gs_contractid_with_error(
    const nsGetServiceByContractIDWithError&, const nsIID&);
  void assign_from_helper(const nsCOMPtr_helper&, const nsIID&);
  void** begin_assignment();

  void assign_assuming_AddRef(T* aNewPtr)
  {
    T* oldPtr = mRawPtr;
    mRawPtr = aNewPtr;
    NSCAP_LOG_ASSIGNMENT(this, aNewPtr);
    NSCAP_LOG_RELEASE(this, oldPtr);
    if (oldPtr) {
      NSCAP_RELEASE(this, oldPtr);
    }
  }

private:
  T* MOZ_OWNING_REF mRawPtr;
#endif

public:
  typedef T element_type;

#ifndef NSCAP_FEATURE_USE_BASE
  ~nsCOMPtr()
  {
    NSCAP_LOG_RELEASE(this, mRawPtr);
    if (mRawPtr) {
      NSCAP_RELEASE(this, mRawPtr);
    }
  }
#endif

#ifdef NSCAP_FEATURE_TEST_DONTQUERY_CASES
  void Assert_NoQueryNeeded()
  {
    if (mRawPtr) {
      nsCOMPtr<T> query_result(do_QueryInterface(mRawPtr));
      NS_ASSERTION(query_result.get() == mRawPtr, "QueryInterface needed");
    }
  }

  #define NSCAP_ASSERT_NO_QUERY_NEEDED() Assert_NoQueryNeeded();
#else
  #define NSCAP_ASSERT_NO_QUERY_NEEDED()
#endif


  

  nsCOMPtr()
    : NSCAP_CTOR_BASE(0)
  {
    NSCAP_LOG_ASSIGNMENT(this, 0);
  }

  nsCOMPtr(const nsCOMPtr<T>& aSmartPtr)
    : NSCAP_CTOR_BASE(aSmartPtr.mRawPtr)
  {
    if (mRawPtr) {
      NSCAP_ADDREF(this, mRawPtr);
    }
    NSCAP_LOG_ASSIGNMENT(this, aSmartPtr.mRawPtr);
  }

  MOZ_IMPLICIT nsCOMPtr(T* aRawPtr)
    : NSCAP_CTOR_BASE(aRawPtr)
  {
    if (mRawPtr) {
      NSCAP_ADDREF(this, mRawPtr);
    }
    NSCAP_LOG_ASSIGNMENT(this, aRawPtr);
    NSCAP_ASSERT_NO_QUERY_NEEDED();
  }

  MOZ_IMPLICIT nsCOMPtr(already_AddRefed<T>& aSmartPtr)
    : NSCAP_CTOR_BASE(aSmartPtr.take())
  {
    NSCAP_LOG_ASSIGNMENT(this, mRawPtr);
    NSCAP_ASSERT_NO_QUERY_NEEDED();
  }

  
  MOZ_IMPLICIT nsCOMPtr(already_AddRefed<T>&& aSmartPtr)
    : NSCAP_CTOR_BASE(aSmartPtr.take())
  {
    NSCAP_LOG_ASSIGNMENT(this, mRawPtr);
    NSCAP_ASSERT_NO_QUERY_NEEDED();
  }

  
  template<typename U>
  MOZ_IMPLICIT nsCOMPtr(already_AddRefed<U>& aSmartPtr)
    : NSCAP_CTOR_BASE(static_cast<T*>(aSmartPtr.take()))
  {
    
    static_assert(mozilla::IsBaseOf<T, U>::value,
                  "U is not a subclass of T");
    NSCAP_LOG_ASSIGNMENT(this, static_cast<T*>(mRawPtr));
    NSCAP_ASSERT_NO_QUERY_NEEDED();
  }

  
  template<typename U>
  MOZ_IMPLICIT nsCOMPtr(already_AddRefed<U>&& aSmartPtr)
    : NSCAP_CTOR_BASE(static_cast<T*>(aSmartPtr.take()))
  {
    
    static_assert(mozilla::IsBaseOf<T, U>::value,
                  "U is not a subclass of T");
    NSCAP_LOG_ASSIGNMENT(this, static_cast<T*>(mRawPtr));
    NSCAP_ASSERT_NO_QUERY_NEEDED();
  }

  
  MOZ_IMPLICIT nsCOMPtr(const nsQueryInterface aQI)
    : NSCAP_CTOR_BASE(0)
  {
    NSCAP_LOG_ASSIGNMENT(this, 0);
    assign_from_qi(aQI, NS_GET_TEMPLATE_IID(T));
  }

  
  MOZ_IMPLICIT nsCOMPtr(const nsQueryInterfaceWithError& aQI)
    : NSCAP_CTOR_BASE(0)
  {
    NSCAP_LOG_ASSIGNMENT(this, 0);
    assign_from_qi_with_error(aQI, NS_GET_TEMPLATE_IID(T));
  }

  
  MOZ_IMPLICIT nsCOMPtr(const nsGetServiceByCID aGS)
    : NSCAP_CTOR_BASE(0)
  {
    NSCAP_LOG_ASSIGNMENT(this, 0);
    assign_from_gs_cid(aGS, NS_GET_TEMPLATE_IID(T));
  }

  
  MOZ_IMPLICIT nsCOMPtr(const nsGetServiceByCIDWithError& aGS)
    : NSCAP_CTOR_BASE(0)
  {
    NSCAP_LOG_ASSIGNMENT(this, 0);
    assign_from_gs_cid_with_error(aGS, NS_GET_TEMPLATE_IID(T));
  }

  
  MOZ_IMPLICIT nsCOMPtr(const nsGetServiceByContractID aGS)
    : NSCAP_CTOR_BASE(0)
  {
    NSCAP_LOG_ASSIGNMENT(this, 0);
    assign_from_gs_contractid(aGS, NS_GET_TEMPLATE_IID(T));
  }

  
  MOZ_IMPLICIT nsCOMPtr(const nsGetServiceByContractIDWithError& aGS)
    : NSCAP_CTOR_BASE(0)
  {
    NSCAP_LOG_ASSIGNMENT(this, 0);
    assign_from_gs_contractid_with_error(aGS, NS_GET_TEMPLATE_IID(T));
  }

  
  
  MOZ_IMPLICIT nsCOMPtr(const nsCOMPtr_helper& aHelper)
    : NSCAP_CTOR_BASE(0)
  {
    NSCAP_LOG_ASSIGNMENT(this, 0);
    assign_from_helper(aHelper, NS_GET_TEMPLATE_IID(T));
    NSCAP_ASSERT_NO_QUERY_NEEDED();
  }

  
  template<class U>
  MOZ_IMPLICIT nsCOMPtr(const mozilla::dom::OwningNonNull<U>& aOther);


  

  nsCOMPtr<T>& operator=(const nsCOMPtr<T>& aRhs)
  {
    assign_with_AddRef(aRhs.mRawPtr);
    return *this;
  }

  nsCOMPtr<T>& operator=(T* aRhs)
  {
    assign_with_AddRef(aRhs);
    NSCAP_ASSERT_NO_QUERY_NEEDED();
    return *this;
  }

  
  template<typename U>
  nsCOMPtr<T>& operator=(already_AddRefed<U>& aRhs)
  {
    
    static_assert(mozilla::IsBaseOf<T, U>::value,
                  "U is not a subclass of T");
    assign_assuming_AddRef(static_cast<T*>(aRhs.take()));
    NSCAP_ASSERT_NO_QUERY_NEEDED();
    return *this;
  }

  
  template<typename U>
  nsCOMPtr<T>& operator=(already_AddRefed<U>&& aRhs)
  {
    
    static_assert(mozilla::IsBaseOf<T, U>::value,
                  "U is not a subclass of T");
    assign_assuming_AddRef(static_cast<T*>(aRhs.take()));
    NSCAP_ASSERT_NO_QUERY_NEEDED();
    return *this;
  }

  
  nsCOMPtr<T>& operator=(const nsQueryInterface aRhs)
  {
    assign_from_qi(aRhs, NS_GET_TEMPLATE_IID(T));
    return *this;
  }

  
  nsCOMPtr<T>& operator=(const nsQueryInterfaceWithError& aRhs)
  {
    assign_from_qi_with_error(aRhs, NS_GET_TEMPLATE_IID(T));
    return *this;
  }

  
  nsCOMPtr<T>& operator=(const nsGetServiceByCID aRhs)
  {
    assign_from_gs_cid(aRhs, NS_GET_TEMPLATE_IID(T));
    return *this;
  }

  
  nsCOMPtr<T>& operator=(const nsGetServiceByCIDWithError& aRhs)
  {
    assign_from_gs_cid_with_error(aRhs, NS_GET_TEMPLATE_IID(T));
    return *this;
  }

  
  nsCOMPtr<T>& operator=(const nsGetServiceByContractID aRhs)
  {
    assign_from_gs_contractid(aRhs, NS_GET_TEMPLATE_IID(T));
    return *this;
  }

  
  nsCOMPtr<T>& operator=(const nsGetServiceByContractIDWithError& aRhs)
  {
    assign_from_gs_contractid_with_error(aRhs, NS_GET_TEMPLATE_IID(T));
    return *this;
  }

  
  
  nsCOMPtr<T>& operator=(const nsCOMPtr_helper& aRhs)
  {
    assign_from_helper(aRhs, NS_GET_TEMPLATE_IID(T));
    NSCAP_ASSERT_NO_QUERY_NEEDED();
    return *this;
  }

  
  template<class U>
  nsCOMPtr<T>& operator=(const mozilla::dom::OwningNonNull<U>& aOther);

  
  void swap(nsCOMPtr<T>& aRhs)
  {
#ifdef NSCAP_FEATURE_USE_BASE
    nsISupports* temp = aRhs.mRawPtr;
#else
    T* temp = aRhs.mRawPtr;
#endif
    NSCAP_LOG_ASSIGNMENT(&aRhs, mRawPtr);
    NSCAP_LOG_ASSIGNMENT(this, temp);
    NSCAP_LOG_RELEASE(this, mRawPtr);
    NSCAP_LOG_RELEASE(&aRhs, temp);
    aRhs.mRawPtr = mRawPtr;
    mRawPtr = temp;
    
  }

  
  void swap(T*& aRhs)
  {
#ifdef NSCAP_FEATURE_USE_BASE
    nsISupports* temp = aRhs;
#else
    T* temp = aRhs;
#endif
    NSCAP_LOG_ASSIGNMENT(this, temp);
    NSCAP_LOG_RELEASE(this, mRawPtr);
    aRhs = reinterpret_cast<T*>(mRawPtr);
    mRawPtr = temp;
    NSCAP_ASSERT_NO_QUERY_NEEDED();
  }


  

  
  
  already_AddRefed<T> forget()
  {
    T* temp = 0;
    swap(temp);
    return already_AddRefed<T>(temp);
  }

  
  
  
  template<typename I>
  void forget(I** aRhs)
  {
    NS_ASSERTION(aRhs, "Null pointer passed to forget!");
    NSCAP_LOG_RELEASE(this, mRawPtr);
    *aRhs = get();
    mRawPtr = 0;
  }

  
  
  
  T* get() const { return reinterpret_cast<T*>(mRawPtr); }

  
  
  
  
  
  
  operator T*() const { return get(); }

  T* operator->() const MOZ_NO_ADDREF_RELEASE_ON_RETURN
  {
    MOZ_ASSERT(mRawPtr != 0,
               "You can't dereference a NULL nsCOMPtr with operator->().");
    return get();
  }

  
  nsCOMPtr<T>* get_address() { return this; }
  const nsCOMPtr<T>* get_address() const { return this; }

public:
  T& operator*() const
  {
    MOZ_ASSERT(mRawPtr != 0,
               "You can't dereference a NULL nsCOMPtr with operator*().");
    return *get();
  }

  T** StartAssignment()
  {
#ifndef NSCAP_FEATURE_INLINE_STARTASSIGNMENT
    return reinterpret_cast<T**>(begin_assignment());
#else
    assign_assuming_AddRef(0);
    return reinterpret_cast<T**>(&mRawPtr);
#endif
  }
};











template<>
class nsCOMPtr<nsISupports>
  : private nsCOMPtr_base
{
public:
  typedef nsISupports element_type;

  

  nsCOMPtr()
    : nsCOMPtr_base(0)
  {
    NSCAP_LOG_ASSIGNMENT(this, 0);
  }

  nsCOMPtr(const nsCOMPtr<nsISupports>& aSmartPtr)
    : nsCOMPtr_base(aSmartPtr.mRawPtr)
  {
    if (mRawPtr) {
      NSCAP_ADDREF(this, mRawPtr);
    }
    NSCAP_LOG_ASSIGNMENT(this, aSmartPtr.mRawPtr);
  }

  MOZ_IMPLICIT nsCOMPtr(nsISupports* aRawPtr)
    : nsCOMPtr_base(aRawPtr)
  {
    if (mRawPtr) {
      NSCAP_ADDREF(this, mRawPtr);
    }
    NSCAP_LOG_ASSIGNMENT(this, aRawPtr);
  }

  
  MOZ_IMPLICIT nsCOMPtr(already_AddRefed<nsISupports>& aSmartPtr)
    : nsCOMPtr_base(aSmartPtr.take())
  {
    NSCAP_LOG_ASSIGNMENT(this, mRawPtr);
  }

  
  MOZ_IMPLICIT nsCOMPtr(already_AddRefed<nsISupports>&& aSmartPtr)
    : nsCOMPtr_base(aSmartPtr.take())
  {
    NSCAP_LOG_ASSIGNMENT(this, mRawPtr);
  }

  
  MOZ_IMPLICIT nsCOMPtr(const nsQueryInterface aQI)
    : nsCOMPtr_base(0)
  {
    NSCAP_LOG_ASSIGNMENT(this, 0);
    assign_from_qi(aQI, NS_GET_IID(nsISupports));
  }

  
  MOZ_IMPLICIT nsCOMPtr(const nsQueryInterfaceWithError& aQI)
    : nsCOMPtr_base(0)
  {
    NSCAP_LOG_ASSIGNMENT(this, 0);
    assign_from_qi_with_error(aQI, NS_GET_IID(nsISupports));
  }

  
  MOZ_IMPLICIT nsCOMPtr(const nsGetServiceByCID aGS)
    : nsCOMPtr_base(0)
  {
    NSCAP_LOG_ASSIGNMENT(this, 0);
    assign_from_gs_cid(aGS, NS_GET_IID(nsISupports));
  }

  
  MOZ_IMPLICIT nsCOMPtr(const nsGetServiceByCIDWithError& aGS)
    : nsCOMPtr_base(0)
  {
    NSCAP_LOG_ASSIGNMENT(this, 0);
    assign_from_gs_cid_with_error(aGS, NS_GET_IID(nsISupports));
  }

  
  MOZ_IMPLICIT nsCOMPtr(const nsGetServiceByContractID aGS)
    : nsCOMPtr_base(0)
  {
    NSCAP_LOG_ASSIGNMENT(this, 0);
    assign_from_gs_contractid(aGS, NS_GET_IID(nsISupports));
  }

  
  MOZ_IMPLICIT nsCOMPtr(const nsGetServiceByContractIDWithError& aGS)
    : nsCOMPtr_base(0)
  {
    NSCAP_LOG_ASSIGNMENT(this, 0);
    assign_from_gs_contractid_with_error(aGS, NS_GET_IID(nsISupports));
  }

  
  
  MOZ_IMPLICIT nsCOMPtr(const nsCOMPtr_helper& aHelper)
    : nsCOMPtr_base(0)
  {
    NSCAP_LOG_ASSIGNMENT(this, 0);
    assign_from_helper(aHelper, NS_GET_IID(nsISupports));
  }


  

  nsCOMPtr<nsISupports>& operator=(const nsCOMPtr<nsISupports>& aRhs)
  {
    assign_with_AddRef(aRhs.mRawPtr);
    return *this;
  }

  nsCOMPtr<nsISupports>& operator=(nsISupports* aRhs)
  {
    assign_with_AddRef(aRhs);
    return *this;
  }

  
  nsCOMPtr<nsISupports>& operator=(already_AddRefed<nsISupports>& aRhs)
  {
    assign_assuming_AddRef(aRhs.take());
    return *this;
  }

  
  nsCOMPtr<nsISupports>& operator=(already_AddRefed<nsISupports>&& aRhs)
  {
    assign_assuming_AddRef(aRhs.take());
    return *this;
  }

  
  nsCOMPtr<nsISupports>& operator=(const nsQueryInterface aRhs)
  {
    assign_from_qi(aRhs, NS_GET_IID(nsISupports));
    return *this;
  }

  
  nsCOMPtr<nsISupports>& operator=(const nsQueryInterfaceWithError& aRhs)
  {
    assign_from_qi_with_error(aRhs, NS_GET_IID(nsISupports));
    return *this;
  }

  
  nsCOMPtr<nsISupports>& operator=(const nsGetServiceByCID aRhs)
  {
    assign_from_gs_cid(aRhs, NS_GET_IID(nsISupports));
    return *this;
  }

  
  nsCOMPtr<nsISupports>& operator=(const nsGetServiceByCIDWithError& aRhs)
  {
    assign_from_gs_cid_with_error(aRhs, NS_GET_IID(nsISupports));
    return *this;
  }

  
  nsCOMPtr<nsISupports>& operator=(const nsGetServiceByContractID aRhs)
  {
    assign_from_gs_contractid(aRhs, NS_GET_IID(nsISupports));
    return *this;
  }

  
  nsCOMPtr<nsISupports>& operator=(const nsGetServiceByContractIDWithError& aRhs)
  {
    assign_from_gs_contractid_with_error(aRhs, NS_GET_IID(nsISupports));
    return *this;
  }

  
  
  nsCOMPtr<nsISupports>& operator=(const nsCOMPtr_helper& aRhs)
  {
    assign_from_helper(aRhs, NS_GET_IID(nsISupports));
    return *this;
  }

  
  void swap(nsCOMPtr<nsISupports>& aRhs)
  {
    nsISupports* temp = aRhs.mRawPtr;
    NSCAP_LOG_ASSIGNMENT(&aRhs, mRawPtr);
    NSCAP_LOG_ASSIGNMENT(this, temp);
    NSCAP_LOG_RELEASE(this, mRawPtr);
    NSCAP_LOG_RELEASE(&aRhs, temp);
    aRhs.mRawPtr = mRawPtr;
    mRawPtr = temp;
  }

  
  void swap(nsISupports*& aRhs)
  {
    nsISupports* temp = aRhs;
    NSCAP_LOG_ASSIGNMENT(this, temp);
    NSCAP_LOG_RELEASE(this, mRawPtr);
    aRhs = mRawPtr;
    mRawPtr = temp;
  }

  
  
  already_AddRefed<nsISupports> forget()
  {
    nsISupports* temp = 0;
    swap(temp);
    return already_AddRefed<nsISupports>(temp);
  }

  
  
  
  void forget(nsISupports** aRhs)
  {
    NS_ASSERTION(aRhs, "Null pointer passed to forget!");
    *aRhs = 0;
    swap(*aRhs);
  }

  

  
  
  
  nsISupports* get() const { return reinterpret_cast<nsISupports*>(mRawPtr); }

  
  
  
  
  
  
  operator nsISupports* () const { return get(); }

  nsISupports* operator->() const MOZ_NO_ADDREF_RELEASE_ON_RETURN
  {
    MOZ_ASSERT(mRawPtr != 0,
               "You can't dereference a NULL nsCOMPtr with operator->().");
    return get();
  }

  
  nsCOMPtr<nsISupports>* get_address() { return this; }
  const nsCOMPtr<nsISupports>* get_address() const { return this; }

public:

  nsISupports& operator*() const
  {
    MOZ_ASSERT(mRawPtr != 0,
               "You can't dereference a NULL nsCOMPtr with operator*().");
    return *get();
  }

  nsISupports** StartAssignment()
  {
#ifndef NSCAP_FEATURE_INLINE_STARTASSIGNMENT
    return reinterpret_cast<nsISupports**>(begin_assignment());
#else
    assign_assuming_AddRef(0);
    return reinterpret_cast<nsISupports**>(&mRawPtr);
#endif
  }
};

template<typename T>
inline void
ImplCycleCollectionUnlink(nsCOMPtr<T>& aField)
{
  aField = nullptr;
}

template<typename T>
inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            nsCOMPtr<T>& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  CycleCollectionNoteChild(aCallback, aField.get(), aName, aFlags);
}

#ifndef NSCAP_FEATURE_USE_BASE
template<class T>
void
nsCOMPtr<T>::assign_with_AddRef(nsISupports* aRawPtr)
{
  if (aRawPtr) {
    NSCAP_ADDREF(this, aRawPtr);
  }
  assign_assuming_AddRef(reinterpret_cast<T*>(aRawPtr));
}

template<class T>
void
nsCOMPtr<T>::assign_from_qi(const nsQueryInterface aQI, const nsIID& aIID)
{
  void* newRawPtr;
  if (NS_FAILED(aQI(aIID, &newRawPtr))) {
    newRawPtr = 0;
  }
  assign_assuming_AddRef(static_cast<T*>(newRawPtr));
}

template<class T>
void
nsCOMPtr<T>::assign_from_qi_with_error(const nsQueryInterfaceWithError& aQI,
                                       const nsIID& aIID)
{
  void* newRawPtr;
  if (NS_FAILED(aQI(aIID, &newRawPtr))) {
    newRawPtr = 0;
  }
  assign_assuming_AddRef(static_cast<T*>(newRawPtr));
}

template<class T>
void
nsCOMPtr<T>::assign_from_gs_cid(const nsGetServiceByCID aGS, const nsIID& aIID)
{
  void* newRawPtr;
  if (NS_FAILED(aGS(aIID, &newRawPtr))) {
    newRawPtr = 0;
  }
  assign_assuming_AddRef(static_cast<T*>(newRawPtr));
}

template<class T>
void
nsCOMPtr<T>::assign_from_gs_cid_with_error(const nsGetServiceByCIDWithError& aGS,
                                           const nsIID& aIID)
{
  void* newRawPtr;
  if (NS_FAILED(aGS(aIID, &newRawPtr))) {
    newRawPtr = 0;
  }
  assign_assuming_AddRef(static_cast<T*>(newRawPtr));
}

template<class T>
void
nsCOMPtr<T>::assign_from_gs_contractid(const nsGetServiceByContractID aGS,
                                       const nsIID& aIID)
{
  void* newRawPtr;
  if (NS_FAILED(aGS(aIID, &newRawPtr))) {
    newRawPtr = 0;
  }
  assign_assuming_AddRef(static_cast<T*>(newRawPtr));
}

template<class T>
void
nsCOMPtr<T>::assign_from_gs_contractid_with_error(
    const nsGetServiceByContractIDWithError& aGS, const nsIID& aIID)
{
  void* newRawPtr;
  if (NS_FAILED(aGS(aIID, &newRawPtr))) {
    newRawPtr = 0;
  }
  assign_assuming_AddRef(static_cast<T*>(newRawPtr));
}

template<class T>
void
nsCOMPtr<T>::assign_from_helper(const nsCOMPtr_helper& helper, const nsIID& aIID)
{
  void* newRawPtr;
  if (NS_FAILED(helper(aIID, &newRawPtr))) {
    newRawPtr = 0;
  }
  assign_assuming_AddRef(static_cast<T*>(newRawPtr));
}

template<class T>
void**
nsCOMPtr<T>::begin_assignment()
{
  assign_assuming_AddRef(0);
  union
  {
    T** mT;
    void** mVoid;
  } result;
  result.mT = &mRawPtr;
  return result.mVoid;
}
#endif

template<class T>
inline nsCOMPtr<T>*
address_of(nsCOMPtr<T>& aPtr)
{
  return aPtr.get_address();
}

template<class T>
inline const nsCOMPtr<T>*
address_of(const nsCOMPtr<T>& aPtr)
{
  return aPtr.get_address();
}
















template<class T>
class nsGetterAddRefs
{
public:
  explicit nsGetterAddRefs(nsCOMPtr<T>& aSmartPtr)
    : mTargetSmartPtr(aSmartPtr)
  {
  }

#if defined(NSCAP_FEATURE_TEST_DONTQUERY_CASES) || defined(NSCAP_LOG_EXTERNAL_ASSIGNMENT)
  ~nsGetterAddRefs()
  {
#ifdef NSCAP_LOG_EXTERNAL_ASSIGNMENT
    NSCAP_LOG_ASSIGNMENT(reinterpret_cast<void*>(address_of(mTargetSmartPtr)),
                         mTargetSmartPtr.get());
#endif

#ifdef NSCAP_FEATURE_TEST_DONTQUERY_CASES
    mTargetSmartPtr.Assert_NoQueryNeeded();
#endif
  }
#endif

  operator void**()
  {
    return reinterpret_cast<void**>(mTargetSmartPtr.StartAssignment());
  }

  operator T**() { return mTargetSmartPtr.StartAssignment(); }
  T*& operator*() { return *(mTargetSmartPtr.StartAssignment()); }

private:
  nsCOMPtr<T>& mTargetSmartPtr;
};


template<>
class nsGetterAddRefs<nsISupports>
{
public:
  explicit nsGetterAddRefs(nsCOMPtr<nsISupports>& aSmartPtr)
    : mTargetSmartPtr(aSmartPtr)
  {
  }

#ifdef NSCAP_LOG_EXTERNAL_ASSIGNMENT
  ~nsGetterAddRefs()
  {
    NSCAP_LOG_ASSIGNMENT(reinterpret_cast<void*>(address_of(mTargetSmartPtr)),
                         mTargetSmartPtr.get());
  }
#endif

  operator void**()
  {
    return reinterpret_cast<void**>(mTargetSmartPtr.StartAssignment());
  }

  operator nsISupports**() { return mTargetSmartPtr.StartAssignment(); }
  nsISupports*& operator*() { return *(mTargetSmartPtr.StartAssignment()); }

private:
  nsCOMPtr<nsISupports>& mTargetSmartPtr;
};

template<class T>
inline nsGetterAddRefs<T>
getter_AddRefs(nsCOMPtr<T>& aSmartPtr)
{
  return nsGetterAddRefs<T>(aSmartPtr);
}

template<class T, class DestinationType>
inline nsresult
CallQueryInterface(T* aSource, nsGetterAddRefs<DestinationType> aDestination)
{
  return CallQueryInterface(aSource,
                            static_cast<DestinationType**>(aDestination));
}




template<class T, class U>
inline bool
operator==(const nsCOMPtr<T>& aLhs, const nsCOMPtr<U>& aRhs)
{
  return static_cast<const T*>(aLhs.get()) == static_cast<const U*>(aRhs.get());
}


template<class T, class U>
inline bool
operator!=(const nsCOMPtr<T>& aLhs, const nsCOMPtr<U>& aRhs)
{
  return static_cast<const T*>(aLhs.get()) != static_cast<const U*>(aRhs.get());
}




template<class T, class U>
inline bool
operator==(const nsCOMPtr<T>& aLhs, const U* aRhs)
{
  return static_cast<const T*>(aLhs.get()) == aRhs;
}

template<class T, class U>
inline bool
operator==(const U* aLhs, const nsCOMPtr<T>& aRhs)
{
  return aLhs == static_cast<const T*>(aRhs.get());
}

template<class T, class U>
inline bool
operator!=(const nsCOMPtr<T>& aLhs, const U* aRhs)
{
  return static_cast<const T*>(aLhs.get()) != aRhs;
}

template<class T, class U>
inline bool
operator!=(const U* aLhs, const nsCOMPtr<T>& aRhs)
{
  return aLhs != static_cast<const T*>(aRhs.get());
}

template<class T, class U>
inline bool
operator==(const nsCOMPtr<T>& aLhs, U* aRhs)
{
  return static_cast<const T*>(aLhs.get()) == const_cast<const U*>(aRhs);
}

template<class T, class U>
inline bool
operator==(U* aLhs, const nsCOMPtr<T>& aRhs)
{
  return const_cast<const U*>(aLhs) == static_cast<const T*>(aRhs.get());
}

template<class T, class U>
inline bool
operator!=(const nsCOMPtr<T>& aLhs, U* aRhs)
{
  return static_cast<const T*>(aLhs.get()) != const_cast<const U*>(aRhs);
}

template<class T, class U>
inline bool
operator!=(U* aLhs, const nsCOMPtr<T>& aRhs)
{
  return const_cast<const U*>(aLhs) != static_cast<const T*>(aRhs.get());
}





class NSCAP_Zero;


template<class T>
inline bool
operator==(const nsCOMPtr<T>& aLhs, NSCAP_Zero* aRhs)
{
  return static_cast<const void*>(aLhs.get()) == reinterpret_cast<const void*>(aRhs);
}


template<class T>
inline bool
operator==(NSCAP_Zero* aLhs, const nsCOMPtr<T>& aRhs)
{
  return reinterpret_cast<const void*>(aLhs) == static_cast<const void*>(aRhs.get());
}


template<class T>
inline bool
operator!=(const nsCOMPtr<T>& aLhs, NSCAP_Zero* aRhs)
{
  return static_cast<const void*>(aLhs.get()) != reinterpret_cast<const void*>(aRhs);
}


template<class T>
inline bool
operator!=(NSCAP_Zero* aLhs, const nsCOMPtr<T>& aRhs)
{
  return reinterpret_cast<const void*>(aLhs) != static_cast<const void*>(aRhs.get());
}




inline bool
SameCOMIdentity(nsISupports* aLhs, nsISupports* aRhs)
{
  return nsCOMPtr<nsISupports>(do_QueryInterface(aLhs)) ==
    nsCOMPtr<nsISupports>(do_QueryInterface(aRhs));
}



template<class SourceType, class DestinationType>
inline nsresult
CallQueryInterface(nsCOMPtr<SourceType>& aSourcePtr, DestinationType** aDestPtr)
{
  return CallQueryInterface(aSourcePtr.get(), aDestPtr);
}

#endif 
