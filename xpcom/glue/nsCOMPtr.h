






































#ifndef nsCOMPtr_h___
#define nsCOMPtr_h___















  
#ifndef nsDebug_h___
#include "nsDebug.h"
  
#endif

#ifndef nsISupportsUtils_h__
#include "nsISupportsUtils.h"
  
#endif

#ifndef nscore_h___
#include "nscore.h"
  
#endif










#ifdef _MSC_VER
  #define NSCAP_FEATURE_INLINE_STARTASSIGNMENT





  #pragma warning( disable: 4514 )
#endif

#define NSCAP_FEATURE_USE_BASE

#ifdef NS_DEBUG
  #define NSCAP_FEATURE_TEST_DONTQUERY_CASES
  #undef NSCAP_FEATURE_USE_BASE

#endif

  





#ifdef NSCAP_DISABLE_TEST_DONTQUERY_CASES
  #undef NSCAP_FEATURE_TEST_DONTQUERY_CASES
#endif

#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
  
  
  
  
  

  #define NS_MAY_ALIAS_PTR(t)    t*  __attribute__((__may_alias__))
#else
  #define NS_MAY_ALIAS_PTR(t)    t*
#endif

#if defined(NSCAP_DISABLE_DEBUG_PTR_TYPES)
  #define NSCAP_FEATURE_USE_BASE
#endif


#ifdef HAVE_CPP_BOOL
  typedef bool NSCAP_BOOL;
#else
  typedef PRBool NSCAP_BOOL;
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

template <class T>
struct already_AddRefed
    














  {
    already_AddRefed( T* aRawPtr )
        : mRawPtr(aRawPtr)
      {
        
      }

    T* get() const { return mRawPtr; }

    T* mRawPtr;
  };

template <class T>
inline
const already_AddRefed<T>
getter_AddRefs( T* aRawPtr )
    



  {
    return already_AddRefed<T>(aRawPtr);
  }

template <class T>
inline
const already_AddRefed<T>
getter_AddRefs( const already_AddRefed<T> aAlreadyAddRefedPtr )
  {
    return aAlreadyAddRefedPtr;
  }

template <class T>
inline
const already_AddRefed<T>
dont_AddRef( T* aRawPtr )
  {
    return already_AddRefed<T>(aRawPtr);
  }

template <class T>
inline
const already_AddRefed<T>
dont_AddRef( const already_AddRefed<T> aAlreadyAddRefedPtr )
  {
    return aAlreadyAddRefedPtr;
  }



class nsCOMPtr_helper
    














  {
    public:
      virtual nsresult NS_FASTCALL operator()( const nsIID&, void** ) const = 0;
  };








class
  NS_COM_GLUE
  NS_STACK_CLASS
  NS_FINAL_CLASS
nsQueryInterface
  {
    public:
      explicit
      nsQueryInterface( nsISupports* aRawPtr )
          : mRawPtr(aRawPtr)
        {
          
        }

      nsresult NS_FASTCALL operator()( const nsIID& aIID, void** ) const;

    private:
      nsISupports*  mRawPtr;
  };

class NS_COM_GLUE nsQueryInterfaceWithError
  {
    public:
      nsQueryInterfaceWithError( nsISupports* aRawPtr, nsresult* error )
          : mRawPtr(aRawPtr),
            mErrorPtr(error)
        {
          
        }

      nsresult NS_FASTCALL operator()( const nsIID& aIID, void** ) const;

    private:
      nsISupports*  mRawPtr;
      nsresult*     mErrorPtr;
  };

inline
nsQueryInterface
do_QueryInterface( nsISupports* aRawPtr )
  {
    return nsQueryInterface(aRawPtr);
  }

inline
nsQueryInterfaceWithError
do_QueryInterface( nsISupports* aRawPtr, nsresult* error )
  {
    return nsQueryInterfaceWithError(aRawPtr, error);
  }

template <class T>
inline
void
do_QueryInterface( already_AddRefed<T>& )
  {
    
    
    
  }

template <class T>
inline
void
do_QueryInterface( already_AddRefed<T>&, nsresult* )
  {
    
    
    
  }




class NS_COM_GLUE nsGetServiceByCID
{
 public:
    explicit nsGetServiceByCID(const nsCID& aCID)
        : mCID(aCID)
        {
            
        }
    
    nsresult NS_FASTCALL operator()( const nsIID&, void** ) const;
    
 private:
    const nsCID&                mCID;
};

class NS_COM_GLUE nsGetServiceByCIDWithError
{
 public:
    nsGetServiceByCIDWithError( const nsCID& aCID, nsresult* aErrorPtr )
        : mCID(aCID),
          mErrorPtr(aErrorPtr)
        {
            
        }
    
    nsresult NS_FASTCALL operator()( const nsIID&, void** ) const;
    
 private:
    const nsCID&                mCID;
    nsresult*                   mErrorPtr;
};

class NS_COM_GLUE nsGetServiceByContractID
{
 public:
    explicit nsGetServiceByContractID(const char* aContractID)
        : mContractID(aContractID)
        {
            
        }
    
    nsresult NS_FASTCALL operator()( const nsIID&, void** ) const;
    
 private:
    const char*                 mContractID;
};

class NS_COM_GLUE nsGetServiceByContractIDWithError
{
 public:
    nsGetServiceByContractIDWithError(const char* aContractID, nsresult* aErrorPtr)
        : mContractID(aContractID),
          mErrorPtr(aErrorPtr)
        {
            
        }
    
    nsresult NS_FASTCALL operator()( const nsIID&, void** ) const;
    
 private:
    const char*                 mContractID;
    nsresult*                   mErrorPtr;
};

class
nsCOMPtr_base
    











  {
    public:

      nsCOMPtr_base( nsISupports* rawPtr = 0 )
          : mRawPtr(rawPtr)
        {
          
        }

      NS_COM_GLUE NS_CONSTRUCTOR_FASTCALL ~nsCOMPtr_base();

      NS_COM_GLUE void NS_FASTCALL   assign_with_AddRef( nsISupports* );
      NS_COM_GLUE void NS_FASTCALL   assign_from_qi( const nsQueryInterface, const nsIID& );
      NS_COM_GLUE void NS_FASTCALL   assign_from_qi_with_error( const nsQueryInterfaceWithError&, const nsIID& );
      NS_COM_GLUE void NS_FASTCALL   assign_from_gs_cid( const nsGetServiceByCID, const nsIID& );
      NS_COM_GLUE void NS_FASTCALL   assign_from_gs_cid_with_error( const nsGetServiceByCIDWithError&, const nsIID& );
      NS_COM_GLUE void NS_FASTCALL   assign_from_gs_contractid( const nsGetServiceByContractID, const nsIID& );
      NS_COM_GLUE void NS_FASTCALL   assign_from_gs_contractid_with_error( const nsGetServiceByContractIDWithError&, const nsIID& );
      NS_COM_GLUE void NS_FASTCALL   assign_from_helper( const nsCOMPtr_helper&, const nsIID& );
      NS_COM_GLUE void** NS_FASTCALL begin_assignment();

    protected:
      NS_MAY_ALIAS_PTR(nsISupports) mRawPtr;

      void
      assign_assuming_AddRef( nsISupports* newPtr )
        {
            







          nsISupports* oldPtr = mRawPtr;
          mRawPtr = newPtr;
          NSCAP_LOG_ASSIGNMENT(this, newPtr);
          NSCAP_LOG_RELEASE(this, oldPtr);
          if ( oldPtr )
            NSCAP_RELEASE(this, oldPtr);
        }
  };



template <class T>
class
  NS_FINAL_CLASS
nsCOMPtr
#ifdef NSCAP_FEATURE_USE_BASE
    : private nsCOMPtr_base
#endif
  {

#ifdef NSCAP_FEATURE_USE_BASE
  #define NSCAP_CTOR_BASE(x) nsCOMPtr_base(x)
#else
  #define NSCAP_CTOR_BASE(x) mRawPtr(x)

    private:
      void    assign_with_AddRef( nsISupports* );
      void    assign_from_qi( const nsQueryInterface, const nsIID& );
      void    assign_from_qi_with_error( const nsQueryInterfaceWithError&, const nsIID& );
      void    assign_from_gs_cid( const nsGetServiceByCID, const nsIID& );
      void    assign_from_gs_cid_with_error( const nsGetServiceByCIDWithError&, const nsIID& );
      void    assign_from_gs_contractid( const nsGetServiceByContractID, const nsIID& );
      void    assign_from_gs_contractid_with_error( const nsGetServiceByContractIDWithError&, const nsIID& );
      void    assign_from_helper( const nsCOMPtr_helper&, const nsIID& );
      void**  begin_assignment();

      void
      assign_assuming_AddRef( T* newPtr )
        {
          T* oldPtr = mRawPtr;
          mRawPtr = newPtr;
          NSCAP_LOG_ASSIGNMENT(this, newPtr);
          NSCAP_LOG_RELEASE(this, oldPtr);
          if ( oldPtr )
            NSCAP_RELEASE(this, oldPtr);
        }

    private:
      T* mRawPtr;
#endif

    public:
      typedef T element_type;
      
#ifndef NSCAP_FEATURE_USE_BASE
     ~nsCOMPtr()
        {
          NSCAP_LOG_RELEASE(this, mRawPtr);
          if ( mRawPtr )
            NSCAP_RELEASE(this, mRawPtr);
        }
#endif

#ifdef NSCAP_FEATURE_TEST_DONTQUERY_CASES
      void
      Assert_NoQueryNeeded()
        {
          if ( mRawPtr )
            {
              nsCOMPtr<T> query_result( do_QueryInterface(mRawPtr) );
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

      nsCOMPtr( const nsCOMPtr<T>& aSmartPtr )
            : NSCAP_CTOR_BASE(aSmartPtr.mRawPtr)
          
        {
          if ( mRawPtr )
            NSCAP_ADDREF(this, mRawPtr);
          NSCAP_LOG_ASSIGNMENT(this, aSmartPtr.mRawPtr);
        }

      nsCOMPtr( T* aRawPtr )
            : NSCAP_CTOR_BASE(aRawPtr)
          
        {
          if ( mRawPtr )
            NSCAP_ADDREF(this, mRawPtr);
          NSCAP_LOG_ASSIGNMENT(this, aRawPtr);
          NSCAP_ASSERT_NO_QUERY_NEEDED();
        }

      nsCOMPtr( const already_AddRefed<T>& aSmartPtr )
            : NSCAP_CTOR_BASE(aSmartPtr.mRawPtr)
          
        {
          NSCAP_LOG_ASSIGNMENT(this, aSmartPtr.mRawPtr);
          NSCAP_ASSERT_NO_QUERY_NEEDED();
        }

      nsCOMPtr( const nsQueryInterface qi )
            : NSCAP_CTOR_BASE(0)
          
        {
          NSCAP_LOG_ASSIGNMENT(this, 0);
          assign_from_qi(qi, NS_GET_TEMPLATE_IID(T));
        }

      nsCOMPtr( const nsQueryInterfaceWithError& qi )
            : NSCAP_CTOR_BASE(0)
          
        {
          NSCAP_LOG_ASSIGNMENT(this, 0);
          assign_from_qi_with_error(qi, NS_GET_TEMPLATE_IID(T));
        }

      nsCOMPtr( const nsGetServiceByCID gs )
            : NSCAP_CTOR_BASE(0)
          
        {
          NSCAP_LOG_ASSIGNMENT(this, 0);
          assign_from_gs_cid(gs, NS_GET_TEMPLATE_IID(T));
        }

      nsCOMPtr( const nsGetServiceByCIDWithError& gs )
            : NSCAP_CTOR_BASE(0)
          
        {
          NSCAP_LOG_ASSIGNMENT(this, 0);
          assign_from_gs_cid_with_error(gs, NS_GET_TEMPLATE_IID(T));
        }

      nsCOMPtr( const nsGetServiceByContractID gs )
            : NSCAP_CTOR_BASE(0)
          
        {
          NSCAP_LOG_ASSIGNMENT(this, 0);
          assign_from_gs_contractid(gs, NS_GET_TEMPLATE_IID(T));
        }

      nsCOMPtr( const nsGetServiceByContractIDWithError& gs )
            : NSCAP_CTOR_BASE(0)
          
        {
          NSCAP_LOG_ASSIGNMENT(this, 0);
          assign_from_gs_contractid_with_error(gs, NS_GET_TEMPLATE_IID(T));
        }

      nsCOMPtr( const nsCOMPtr_helper& helper )
            : NSCAP_CTOR_BASE(0)
          
          
        {
          NSCAP_LOG_ASSIGNMENT(this, 0);
          assign_from_helper(helper, NS_GET_TEMPLATE_IID(T));
          NSCAP_ASSERT_NO_QUERY_NEEDED();
        }


        

      nsCOMPtr<T>&
      operator=( const nsCOMPtr<T>& rhs )
          
        {
          assign_with_AddRef(rhs.mRawPtr);
          return *this;
        }

      nsCOMPtr<T>&
      operator=( T* rhs )
          
        {
          assign_with_AddRef(rhs);
          NSCAP_ASSERT_NO_QUERY_NEEDED();
          return *this;
        }

      nsCOMPtr<T>&
      operator=( const already_AddRefed<T>& rhs )
          
        {
          assign_assuming_AddRef(rhs.mRawPtr);
          NSCAP_ASSERT_NO_QUERY_NEEDED();
          return *this;
        }

      nsCOMPtr<T>&
      operator=( const nsQueryInterface rhs )
          
        {
          assign_from_qi(rhs, NS_GET_TEMPLATE_IID(T));
          return *this;
        }

      nsCOMPtr<T>&
      operator=( const nsQueryInterfaceWithError& rhs )
          
        {
          assign_from_qi_with_error(rhs, NS_GET_TEMPLATE_IID(T));
          return *this;
        }

      nsCOMPtr<T>&
      operator=( const nsGetServiceByCID rhs )
          
        {
          assign_from_gs_cid(rhs, NS_GET_TEMPLATE_IID(T));
          return *this;
        }

      nsCOMPtr<T>&
      operator=( const nsGetServiceByCIDWithError& rhs )
          
        {
          assign_from_gs_cid_with_error(rhs, NS_GET_TEMPLATE_IID(T));
          return *this;
        }

      nsCOMPtr<T>&
      operator=( const nsGetServiceByContractID rhs )
          
        {
          assign_from_gs_contractid(rhs, NS_GET_TEMPLATE_IID(T));
          return *this;
        }

      nsCOMPtr<T>&
      operator=( const nsGetServiceByContractIDWithError& rhs )
          
        {
          assign_from_gs_contractid_with_error(rhs, NS_GET_TEMPLATE_IID(T));
          return *this;
        }

      nsCOMPtr<T>&
      operator=( const nsCOMPtr_helper& rhs )
          
          
        {
          assign_from_helper(rhs, NS_GET_TEMPLATE_IID(T));
          NSCAP_ASSERT_NO_QUERY_NEEDED();
          return *this;
        }

      void
      swap( nsCOMPtr<T>& rhs )
          
        {
#ifdef NSCAP_FEATURE_USE_BASE
          nsISupports* temp = rhs.mRawPtr;
#else
          T* temp = rhs.mRawPtr;
#endif
          NSCAP_LOG_ASSIGNMENT(&rhs, mRawPtr);
          NSCAP_LOG_ASSIGNMENT(this, temp);
          NSCAP_LOG_RELEASE(this, mRawPtr);
          NSCAP_LOG_RELEASE(&rhs, temp);
          rhs.mRawPtr = mRawPtr;
          mRawPtr = temp;
          
        }

      void
      swap( T*& rhs )
          
        {
#ifdef NSCAP_FEATURE_USE_BASE
          nsISupports* temp = rhs;
#else
          T* temp = rhs;
#endif
          NSCAP_LOG_ASSIGNMENT(this, temp);
          NSCAP_LOG_RELEASE(this, mRawPtr);
          rhs = reinterpret_cast<T*>(mRawPtr);
          mRawPtr = temp;
          NSCAP_ASSERT_NO_QUERY_NEEDED();
        }


        

      already_AddRefed<T>
      forget()
          
          
        {
          T* temp = 0;
          swap(temp);
          return temp;
        }

      void
      forget( T** rhs NS_OUTPARAM )
          
          
          
        {
          NS_ASSERTION(rhs, "Null pointer passed to forget!");
          *rhs = 0;
          swap(*rhs);
        }

      T*
      get() const
          



        {
          return reinterpret_cast<T*>(mRawPtr);
        }

      operator T*() const
          







        {
          return get();
        }

      T*
      operator->() const
        {
          NS_PRECONDITION(mRawPtr != 0, "You can't dereference a NULL nsCOMPtr with operator->().");
          return get();
        }

#ifdef CANT_RESOLVE_CPP_CONST_AMBIGUITY
  

      nsCOMPtr<T>*
      get_address() const
          
          
        {
          return const_cast<nsCOMPtr<T>*>(this);
        }

#else 

      nsCOMPtr<T>*
      get_address()
          
          
        {
          return this;
        }

      const nsCOMPtr<T>*
      get_address() const
          
          
        {
          return this;
        }

#endif 

    public:
      T&
      operator*() const
        {
          NS_PRECONDITION(mRawPtr != 0, "You can't dereference a NULL nsCOMPtr with operator*().");
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



  








NS_SPECIALIZE_TEMPLATE
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

      nsCOMPtr( const nsCOMPtr<nsISupports>& aSmartPtr )
            : nsCOMPtr_base(aSmartPtr.mRawPtr)
          
        {
          if ( mRawPtr )
            NSCAP_ADDREF(this, mRawPtr);
          NSCAP_LOG_ASSIGNMENT(this, aSmartPtr.mRawPtr);
        }

      nsCOMPtr( nsISupports* aRawPtr )
            : nsCOMPtr_base(aRawPtr)
          
        {
          if ( mRawPtr )
            NSCAP_ADDREF(this, mRawPtr);
          NSCAP_LOG_ASSIGNMENT(this, aRawPtr);
        }

      nsCOMPtr( const already_AddRefed<nsISupports>& aSmartPtr )
            : nsCOMPtr_base(aSmartPtr.mRawPtr)
          
        {
          NSCAP_LOG_ASSIGNMENT(this, aSmartPtr.mRawPtr);
        }

      nsCOMPtr( const nsQueryInterface qi )
            : nsCOMPtr_base(0)
          
        {
          NSCAP_LOG_ASSIGNMENT(this, 0);
          assign_from_qi(qi, NS_GET_IID(nsISupports));
        }

      nsCOMPtr( const nsQueryInterfaceWithError& qi )
            : nsCOMPtr_base(0)
          
        {
          NSCAP_LOG_ASSIGNMENT(this, 0);
          assign_from_qi_with_error(qi, NS_GET_IID(nsISupports));
        }

      nsCOMPtr( const nsGetServiceByCID gs )
            : nsCOMPtr_base(0)
          
        {
          NSCAP_LOG_ASSIGNMENT(this, 0);
          assign_from_gs_cid(gs, NS_GET_IID(nsISupports));
        }

      nsCOMPtr( const nsGetServiceByCIDWithError& gs )
            : nsCOMPtr_base(0)
          
        {
          NSCAP_LOG_ASSIGNMENT(this, 0);
          assign_from_gs_cid_with_error(gs, NS_GET_IID(nsISupports));
        }

      nsCOMPtr( const nsGetServiceByContractID gs )
            : nsCOMPtr_base(0)
          
        {
          NSCAP_LOG_ASSIGNMENT(this, 0);
          assign_from_gs_contractid(gs, NS_GET_IID(nsISupports));
        }

      nsCOMPtr( const nsGetServiceByContractIDWithError& gs )
            : nsCOMPtr_base(0)
          
        {
          NSCAP_LOG_ASSIGNMENT(this, 0);
          assign_from_gs_contractid_with_error(gs, NS_GET_IID(nsISupports));
        }

      nsCOMPtr( const nsCOMPtr_helper& helper )
            : nsCOMPtr_base(0)
          
          
        {
          NSCAP_LOG_ASSIGNMENT(this, 0);
          assign_from_helper(helper, NS_GET_IID(nsISupports));
        }


        

      nsCOMPtr<nsISupports>&
      operator=( const nsCOMPtr<nsISupports>& rhs )
          
        {
          assign_with_AddRef(rhs.mRawPtr);
          return *this;
        }

      nsCOMPtr<nsISupports>&
      operator=( nsISupports* rhs )
          
        {
          assign_with_AddRef(rhs);
          return *this;
        }

      nsCOMPtr<nsISupports>&
      operator=( const already_AddRefed<nsISupports>& rhs )
          
        {
          assign_assuming_AddRef(rhs.mRawPtr);
          return *this;
        }

      nsCOMPtr<nsISupports>&
      operator=( const nsQueryInterface rhs )
          
        {
          assign_from_qi(rhs, NS_GET_IID(nsISupports));
          return *this;
        }

      nsCOMPtr<nsISupports>&
      operator=( const nsQueryInterfaceWithError& rhs )
          
        {
          assign_from_qi_with_error(rhs, NS_GET_IID(nsISupports));
          return *this;
        }

      nsCOMPtr<nsISupports>&
      operator=( const nsGetServiceByCID rhs )
          
        {
          assign_from_gs_cid(rhs, NS_GET_IID(nsISupports));
          return *this;
        }

      nsCOMPtr<nsISupports>&
      operator=( const nsGetServiceByCIDWithError& rhs )
          
        {
          assign_from_gs_cid_with_error(rhs, NS_GET_IID(nsISupports));
          return *this;
        }

      nsCOMPtr<nsISupports>&
      operator=( const nsGetServiceByContractID rhs )
          
        {
          assign_from_gs_contractid(rhs, NS_GET_IID(nsISupports));
          return *this;
        }

      nsCOMPtr<nsISupports>&
      operator=( const nsGetServiceByContractIDWithError& rhs )
          
        {
          assign_from_gs_contractid_with_error(rhs, NS_GET_IID(nsISupports));
          return *this;
        }

      nsCOMPtr<nsISupports>&
      operator=( const nsCOMPtr_helper& rhs )
          
          
        {
          assign_from_helper(rhs, NS_GET_IID(nsISupports));
          return *this;
        }

      void
      swap( nsCOMPtr<nsISupports>& rhs )
          
        {
          nsISupports* temp = rhs.mRawPtr;
          NSCAP_LOG_ASSIGNMENT(&rhs, mRawPtr);
          NSCAP_LOG_ASSIGNMENT(this, temp);
          NSCAP_LOG_RELEASE(this, mRawPtr);
          NSCAP_LOG_RELEASE(&rhs, temp);
          rhs.mRawPtr = mRawPtr;
          mRawPtr = temp;
        }

      void
      swap( nsISupports*& rhs )
          
        {
          nsISupports* temp = rhs;
          NSCAP_LOG_ASSIGNMENT(this, temp);
          NSCAP_LOG_RELEASE(this, mRawPtr);
          rhs = mRawPtr;
          mRawPtr = temp;
        }

      already_AddRefed<nsISupports>
      forget()
          
          
        {
          nsISupports* temp = 0;
          swap(temp);
          return temp;
        }

      void
      forget( nsISupports** rhs NS_OUTPARAM )
          
          
          
        {
          NS_ASSERTION(rhs, "Null pointer passed to forget!");
          *rhs = 0;
          swap(*rhs);
        }

        

      nsISupports*
      get() const
          




        {
          return reinterpret_cast<nsISupports*>(mRawPtr);
        }

      operator nsISupports*() const
          







        {
          return get();
        }

      nsISupports*
      operator->() const
        {
          NS_PRECONDITION(mRawPtr != 0, "You can't dereference a NULL nsCOMPtr with operator->().");
          return get();
        }

#ifdef CANT_RESOLVE_CPP_CONST_AMBIGUITY
  

      nsCOMPtr<nsISupports>*
      get_address() const
          
          
        {
          return const_cast<nsCOMPtr<nsISupports>*>(this);
        }

#else 

      nsCOMPtr<nsISupports>*
      get_address()
          
          
        {
          return this;
        }

      const nsCOMPtr<nsISupports>*
      get_address() const
          
          
        {
          return this;
        }

#endif 

    public:

      nsISupports&
      operator*() const
        {
          NS_PRECONDITION(mRawPtr != 0, "You can't dereference a NULL nsCOMPtr with operator*().");
          return *get();
        }

      nsISupports**
      StartAssignment()
        {
#ifndef NSCAP_FEATURE_INLINE_STARTASSIGNMENT
          return reinterpret_cast<nsISupports**>(begin_assignment());
#else
          assign_assuming_AddRef(0);
          return reinterpret_cast<nsISupports**>(&mRawPtr);
#endif
        }
  };

#ifndef NSCAP_FEATURE_USE_BASE
template <class T>
void
nsCOMPtr<T>::assign_with_AddRef( nsISupports* rawPtr )
  {
    if ( rawPtr )
      NSCAP_ADDREF(this, rawPtr);
    assign_assuming_AddRef(reinterpret_cast<T*>(rawPtr));
  }

template <class T>
void
nsCOMPtr<T>::assign_from_qi( const nsQueryInterface qi, const nsIID& aIID )
  {
    void* newRawPtr;
    if ( NS_FAILED( qi(aIID, &newRawPtr) ) )
      newRawPtr = 0;
    assign_assuming_AddRef(static_cast<T*>(newRawPtr));
  }

template <class T>
void
nsCOMPtr<T>::assign_from_qi_with_error( const nsQueryInterfaceWithError& qi, const nsIID& aIID )
  {
    void* newRawPtr;
    if ( NS_FAILED( qi(aIID, &newRawPtr) ) )
      newRawPtr = 0;
    assign_assuming_AddRef(static_cast<T*>(newRawPtr));
  }

template <class T>
void
nsCOMPtr<T>::assign_from_gs_cid( const nsGetServiceByCID gs, const nsIID& aIID )
  {
    void* newRawPtr;
    if ( NS_FAILED( gs(aIID, &newRawPtr) ) )
      newRawPtr = 0;
    assign_assuming_AddRef(static_cast<T*>(newRawPtr));
  }

template <class T>
void
nsCOMPtr<T>::assign_from_gs_cid_with_error( const nsGetServiceByCIDWithError& gs, const nsIID& aIID )
  {
    void* newRawPtr;
    if ( NS_FAILED( gs(aIID, &newRawPtr) ) )
      newRawPtr = 0;
    assign_assuming_AddRef(static_cast<T*>(newRawPtr));
  }

template <class T>
void
nsCOMPtr<T>::assign_from_gs_contractid( const nsGetServiceByContractID gs, const nsIID& aIID )
  {
    void* newRawPtr;
    if ( NS_FAILED( gs(aIID, &newRawPtr) ) )
      newRawPtr = 0;
    assign_assuming_AddRef(static_cast<T*>(newRawPtr));
  }

template <class T>
void
nsCOMPtr<T>::assign_from_gs_contractid_with_error( const nsGetServiceByContractIDWithError& gs, const nsIID& aIID )
  {
    void* newRawPtr;
    if ( NS_FAILED( gs(aIID, &newRawPtr) ) )
      newRawPtr = 0;
    assign_assuming_AddRef(static_cast<T*>(newRawPtr));
  }

template <class T>
void
nsCOMPtr<T>::assign_from_helper( const nsCOMPtr_helper& helper, const nsIID& aIID )
  {
    void* newRawPtr;
    if ( NS_FAILED( helper(aIID, &newRawPtr) ) )
      newRawPtr = 0;
    assign_assuming_AddRef(static_cast<T*>(newRawPtr));
  }

template <class T>
void**
nsCOMPtr<T>::begin_assignment()
  {
    assign_assuming_AddRef(0);
    union { T** mT; void** mVoid; } result;
    result.mT = &mRawPtr;
    return result.mVoid;
  }
#endif

#ifdef CANT_RESOLVE_CPP_CONST_AMBIGUITY



template <class T>
inline
nsCOMPtr<T>*
address_of( const nsCOMPtr<T>& aPtr )
  {
    return aPtr.get_address();
  }

#else 

template <class T>
inline
nsCOMPtr<T>*
address_of( nsCOMPtr<T>& aPtr )
  {
    return aPtr.get_address();
  }

template <class T>
inline
const nsCOMPtr<T>*
address_of( const nsCOMPtr<T>& aPtr )
  {
    return aPtr.get_address();
  }

#endif 

template <class T>
class nsGetterAddRefs
    
















  {
    public:
      explicit
      nsGetterAddRefs( nsCOMPtr<T>& aSmartPtr )
          : mTargetSmartPtr(aSmartPtr)
        {
          
        }

#if defined(NSCAP_FEATURE_TEST_DONTQUERY_CASES) || defined(NSCAP_LOG_EXTERNAL_ASSIGNMENT)
     ~nsGetterAddRefs()
        {
#ifdef NSCAP_LOG_EXTERNAL_ASSIGNMENT
          NSCAP_LOG_ASSIGNMENT(reinterpret_cast<void *>(address_of(mTargetSmartPtr)), mTargetSmartPtr.get());
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

      operator nsISupports**()
        {
          return reinterpret_cast<nsISupports**>(mTargetSmartPtr.StartAssignment());
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
      nsCOMPtr<T>& mTargetSmartPtr;
  };


NS_SPECIALIZE_TEMPLATE
class nsGetterAddRefs<nsISupports>
  {
    public:
      explicit
      nsGetterAddRefs( nsCOMPtr<nsISupports>& aSmartPtr )
          : mTargetSmartPtr(aSmartPtr)
        {
          
        }

#ifdef NSCAP_LOG_EXTERNAL_ASSIGNMENT
     ~nsGetterAddRefs()
        {
          NSCAP_LOG_ASSIGNMENT(reinterpret_cast<void *>(address_of(mTargetSmartPtr)), mTargetSmartPtr.get());
        }
#endif

      operator void**()
        {
          return reinterpret_cast<void**>(mTargetSmartPtr.StartAssignment());
        }

      operator nsISupports**()
        {
          return mTargetSmartPtr.StartAssignment();
        }

      nsISupports*&
      operator*()
        {
          return *(mTargetSmartPtr.StartAssignment());
        }

    private:
      nsCOMPtr<nsISupports>& mTargetSmartPtr;
  };


template <class T>
inline
nsGetterAddRefs<T>
getter_AddRefs( nsCOMPtr<T>& aSmartPtr )
    



  {
    return nsGetterAddRefs<T>(aSmartPtr);
  }

template <class T, class DestinationType>
inline
nsresult
CallQueryInterface( T* aSource, nsGetterAddRefs<DestinationType> aDestination )
{
    return CallQueryInterface(aSource,
                              static_cast<DestinationType**>(aDestination));
}


  

template <class T, class U>
inline
NSCAP_BOOL
operator==( const nsCOMPtr<T>& lhs, const nsCOMPtr<U>& rhs )
  {
    return static_cast<const T*>(lhs.get()) == static_cast<const U*>(rhs.get());
  }


template <class T, class U>
inline
NSCAP_BOOL
operator!=( const nsCOMPtr<T>& lhs, const nsCOMPtr<U>& rhs )
  {
    return static_cast<const T*>(lhs.get()) != static_cast<const U*>(rhs.get());
  }


  

template <class T, class U>
inline
NSCAP_BOOL
operator==( const nsCOMPtr<T>& lhs, const U* rhs )
  {
    return static_cast<const T*>(lhs.get()) == rhs;
  }

template <class T, class U>
inline
NSCAP_BOOL
operator==( const U* lhs, const nsCOMPtr<T>& rhs )
  {
    return lhs == static_cast<const T*>(rhs.get());
  }

template <class T, class U>
inline
NSCAP_BOOL
operator!=( const nsCOMPtr<T>& lhs, const U* rhs )
  {
    return static_cast<const T*>(lhs.get()) != rhs;
  }

template <class T, class U>
inline
NSCAP_BOOL
operator!=( const U* lhs, const nsCOMPtr<T>& rhs )
  {
    return lhs != static_cast<const T*>(rhs.get());
  }

  
  
  
  
  
  




#if defined(_MSC_VER) && (_MSC_VER < 1310)
#ifndef NSCAP_DONT_PROVIDE_NONCONST_OPEQ
#define NSCAP_DONT_PROVIDE_NONCONST_OPEQ
#endif
#endif

#ifndef NSCAP_DONT_PROVIDE_NONCONST_OPEQ
template <class T, class U>
inline
NSCAP_BOOL
operator==( const nsCOMPtr<T>& lhs, U* rhs )
  {
    return static_cast<const T*>(lhs.get()) == const_cast<const U*>(rhs);
  }

template <class T, class U>
inline
NSCAP_BOOL
operator==( U* lhs, const nsCOMPtr<T>& rhs )
  {
    return const_cast<const U*>(lhs) == static_cast<const T*>(rhs.get());
  }

template <class T, class U>
inline
NSCAP_BOOL
operator!=( const nsCOMPtr<T>& lhs, U* rhs )
  {
    return static_cast<const T*>(lhs.get()) != const_cast<const U*>(rhs);
  }

template <class T, class U>
inline
NSCAP_BOOL
operator!=( U* lhs, const nsCOMPtr<T>& rhs )
  {
    return const_cast<const U*>(lhs) != static_cast<const T*>(rhs.get());
  }
#endif



  

class NSCAP_Zero;

template <class T>
inline
NSCAP_BOOL
operator==( const nsCOMPtr<T>& lhs, NSCAP_Zero* rhs )
    
  {
    return static_cast<const void*>(lhs.get()) == reinterpret_cast<const void*>(rhs);
  }

template <class T>
inline
NSCAP_BOOL
operator==( NSCAP_Zero* lhs, const nsCOMPtr<T>& rhs )
    
  {
    return reinterpret_cast<const void*>(lhs) == static_cast<const void*>(rhs.get());
  }

template <class T>
inline
NSCAP_BOOL
operator!=( const nsCOMPtr<T>& lhs, NSCAP_Zero* rhs )
    
  {
    return static_cast<const void*>(lhs.get()) != reinterpret_cast<const void*>(rhs);
  }

template <class T>
inline
NSCAP_BOOL
operator!=( NSCAP_Zero* lhs, const nsCOMPtr<T>& rhs )
    
  {
    return reinterpret_cast<const void*>(lhs) != static_cast<const void*>(rhs.get());
  }


#ifdef HAVE_CPP_TROUBLE_COMPARING_TO_ZERO

  
  

template <class T>
inline
NSCAP_BOOL
operator==( const nsCOMPtr<T>& lhs, int rhs )
    
  {
    return static_cast<const void*>(lhs.get()) == reinterpret_cast<const void*>(rhs);
  }

template <class T>
inline
NSCAP_BOOL
operator==( int lhs, const nsCOMPtr<T>& rhs )
    
  {
    return reinterpret_cast<const void*>(lhs) == static_cast<const void*>(rhs.get());
  }

#endif 

  

inline
NSCAP_BOOL
SameCOMIdentity( nsISupports* lhs, nsISupports* rhs )
  {
    return nsCOMPtr<nsISupports>( do_QueryInterface(lhs) ) == nsCOMPtr<nsISupports>( do_QueryInterface(rhs) );
  }



template <class SourceType, class DestinationType>
inline
nsresult
CallQueryInterface( nsCOMPtr<SourceType>& aSourcePtr, DestinationType** aDestPtr )
  {
    return CallQueryInterface(aSourcePtr.get(), aDestPtr);
  }

#endif 
