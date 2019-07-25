






































#ifndef nsAutoPtr_h___
#define nsAutoPtr_h___


#ifndef nsCOMPtr_h___
  
  
  
#include "nsCOMPtr.h"
#endif





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
      assign( T* newPtr )
        {
          T* oldPtr = mRawPtr;
          mRawPtr = newPtr;
          delete oldPtr;
        }

      
      
      
      
      
      class Ptr
        {
          public:
            Ptr( T* aPtr )
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

      nsAutoPtr( Ptr aRawPtr )
            : mRawPtr(aRawPtr)
          
        {
        }

      nsAutoPtr( nsAutoPtr<T>& aSmartPtr )
            : mRawPtr( aSmartPtr.forget() )
          
        {
        }


        

      nsAutoPtr<T>&
      operator=( T* rhs )
          
        {
          assign(rhs);
          return *this;
        }

      nsAutoPtr<T>& operator=( nsAutoPtr<T>& rhs )
          
        {
          assign(rhs.forget());
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
          NS_PRECONDITION(mRawPtr != 0, "You can't dereference a NULL nsAutoPtr with operator->().");
          return get();
        }

      
      
      
#ifndef _MSC_VER
      template <class U, class V>
      U&
      operator->*(U V::* aMember)
        {
          NS_PRECONDITION(mRawPtr != 0, "You can't dereference a NULL nsAutoPtr with operator->*().");
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
          NS_PRECONDITION(mRawPtr != 0, "You can't dereference a NULL nsAutoPtr with operator*().");
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
inline
nsAutoPtr<T>*
address_of( nsAutoPtr<T>& aPtr )
  {
    return aPtr.get_address();
  }

template <class T>
inline
const nsAutoPtr<T>*
address_of( const nsAutoPtr<T>& aPtr )
  {
    return aPtr.get_address();
  }

template <class T>
class nsAutoPtrGetterTransfers
    
















  {
    public:
      explicit
      nsAutoPtrGetterTransfers( nsAutoPtr<T>& aSmartPtr )
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
inline
nsAutoPtrGetterTransfers<T>
getter_Transfers( nsAutoPtr<T>& aSmartPtr )
    



  {
    return nsAutoPtrGetterTransfers<T>(aSmartPtr);
  }



  

template <class T, class U>
inline
bool
operator==( const nsAutoPtr<T>& lhs, const nsAutoPtr<U>& rhs )
  {
    return static_cast<const T*>(lhs.get()) == static_cast<const U*>(rhs.get());
  }


template <class T, class U>
inline
bool
operator!=( const nsAutoPtr<T>& lhs, const nsAutoPtr<U>& rhs )
  {
    return static_cast<const T*>(lhs.get()) != static_cast<const U*>(rhs.get());
  }


  

template <class T, class U>
inline
bool
operator==( const nsAutoPtr<T>& lhs, const U* rhs )
  {
    return static_cast<const T*>(lhs.get()) == static_cast<const U*>(rhs);
  }

template <class T, class U>
inline
bool
operator==( const U* lhs, const nsAutoPtr<T>& rhs )
  {
    return static_cast<const U*>(lhs) == static_cast<const T*>(rhs.get());
  }

template <class T, class U>
inline
bool
operator!=( const nsAutoPtr<T>& lhs, const U* rhs )
  {
    return static_cast<const T*>(lhs.get()) != static_cast<const U*>(rhs);
  }

template <class T, class U>
inline
bool
operator!=( const U* lhs, const nsAutoPtr<T>& rhs )
  {
    return static_cast<const U*>(lhs) != static_cast<const T*>(rhs.get());
  }

  
  
  
  
  
  

#ifndef NSCAP_DONT_PROVIDE_NONCONST_OPEQ
template <class T, class U>
inline
bool
operator==( const nsAutoPtr<T>& lhs, U* rhs )
  {
    return static_cast<const T*>(lhs.get()) == const_cast<const U*>(rhs);
  }

template <class T, class U>
inline
bool
operator==( U* lhs, const nsAutoPtr<T>& rhs )
  {
    return const_cast<const U*>(lhs) == static_cast<const T*>(rhs.get());
  }

template <class T, class U>
inline
bool
operator!=( const nsAutoPtr<T>& lhs, U* rhs )
  {
    return static_cast<const T*>(lhs.get()) != const_cast<const U*>(rhs);
  }

template <class T, class U>
inline
bool
operator!=( U* lhs, const nsAutoPtr<T>& rhs )
  {
    return const_cast<const U*>(lhs) != static_cast<const T*>(rhs.get());
  }
#endif



  

template <class T>
inline
bool
operator==( const nsAutoPtr<T>& lhs, NSCAP_Zero* rhs )
    
  {
    return static_cast<const void*>(lhs.get()) == reinterpret_cast<const void*>(rhs);
  }

template <class T>
inline
bool
operator==( NSCAP_Zero* lhs, const nsAutoPtr<T>& rhs )
    
  {
    return reinterpret_cast<const void*>(lhs) == static_cast<const void*>(rhs.get());
  }

template <class T>
inline
bool
operator!=( const nsAutoPtr<T>& lhs, NSCAP_Zero* rhs )
    
  {
    return static_cast<const void*>(lhs.get()) != reinterpret_cast<const void*>(rhs);
  }

template <class T>
inline
bool
operator!=( NSCAP_Zero* lhs, const nsAutoPtr<T>& rhs )
    
  {
    return reinterpret_cast<const void*>(lhs) != static_cast<const void*>(rhs.get());
  }


#ifdef HAVE_CPP_TROUBLE_COMPARING_TO_ZERO

  
  

template <class T>
inline
bool
operator==( const nsAutoPtr<T>& lhs, int rhs )
    
  {
    return static_cast<const void*>(lhs.get()) == reinterpret_cast<const void*>(rhs);
  }

template <class T>
inline
bool
operator==( int lhs, const nsAutoPtr<T>& rhs )
    
  {
    return reinterpret_cast<const void*>(lhs) == static_cast<const void*>(rhs.get());
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
      assign( T* newPtr )
        {
          T* oldPtr = mRawPtr;
          mRawPtr = newPtr;
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

      nsAutoArrayPtr( T* aRawPtr )
            : mRawPtr(aRawPtr)
          
        {
        }

      nsAutoArrayPtr( nsAutoArrayPtr<T>& aSmartPtr )
            : mRawPtr( aSmartPtr.forget() )
          
        {
        }


        

      nsAutoArrayPtr<T>&
      operator=( T* rhs )
          
        {
          assign(rhs);
          return *this;
        }

      nsAutoArrayPtr<T>& operator=( nsAutoArrayPtr<T>& rhs )
          
        {
          assign(rhs.forget());
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
          NS_PRECONDITION(mRawPtr != 0, "You can't dereference a NULL nsAutoArrayPtr with operator->().");
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
          NS_PRECONDITION(mRawPtr != 0, "You can't dereference a NULL nsAutoArrayPtr with operator*().");
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
inline
nsAutoArrayPtr<T>*
address_of( nsAutoArrayPtr<T>& aPtr )
  {
    return aPtr.get_address();
  }

template <class T>
inline
const nsAutoArrayPtr<T>*
address_of( const nsAutoArrayPtr<T>& aPtr )
  {
    return aPtr.get_address();
  }

template <class T>
class nsAutoArrayPtrGetterTransfers
    
















  {
    public:
      explicit
      nsAutoArrayPtrGetterTransfers( nsAutoArrayPtr<T>& aSmartPtr )
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
inline
nsAutoArrayPtrGetterTransfers<T>
getter_Transfers( nsAutoArrayPtr<T>& aSmartPtr )
    



  {
    return nsAutoArrayPtrGetterTransfers<T>(aSmartPtr);
  }



  

template <class T, class U>
inline
bool
operator==( const nsAutoArrayPtr<T>& lhs, const nsAutoArrayPtr<U>& rhs )
  {
    return static_cast<const T*>(lhs.get()) == static_cast<const U*>(rhs.get());
  }


template <class T, class U>
inline
bool
operator!=( const nsAutoArrayPtr<T>& lhs, const nsAutoArrayPtr<U>& rhs )
  {
    return static_cast<const T*>(lhs.get()) != static_cast<const U*>(rhs.get());
  }


  

template <class T, class U>
inline
bool
operator==( const nsAutoArrayPtr<T>& lhs, const U* rhs )
  {
    return static_cast<const T*>(lhs.get()) == static_cast<const U*>(rhs);
  }

template <class T, class U>
inline
bool
operator==( const U* lhs, const nsAutoArrayPtr<T>& rhs )
  {
    return static_cast<const U*>(lhs) == static_cast<const T*>(rhs.get());
  }

template <class T, class U>
inline
bool
operator!=( const nsAutoArrayPtr<T>& lhs, const U* rhs )
  {
    return static_cast<const T*>(lhs.get()) != static_cast<const U*>(rhs);
  }

template <class T, class U>
inline
bool
operator!=( const U* lhs, const nsAutoArrayPtr<T>& rhs )
  {
    return static_cast<const U*>(lhs) != static_cast<const T*>(rhs.get());
  }

  
  
  
  
  
  

#ifndef NSCAP_DONT_PROVIDE_NONCONST_OPEQ
template <class T, class U>
inline
bool
operator==( const nsAutoArrayPtr<T>& lhs, U* rhs )
  {
    return static_cast<const T*>(lhs.get()) == const_cast<const U*>(rhs);
  }

template <class T, class U>
inline
bool
operator==( U* lhs, const nsAutoArrayPtr<T>& rhs )
  {
    return const_cast<const U*>(lhs) == static_cast<const T*>(rhs.get());
  }

template <class T, class U>
inline
bool
operator!=( const nsAutoArrayPtr<T>& lhs, U* rhs )
  {
    return static_cast<const T*>(lhs.get()) != const_cast<const U*>(rhs);
  }

template <class T, class U>
inline
bool
operator!=( U* lhs, const nsAutoArrayPtr<T>& rhs )
  {
    return const_cast<const U*>(lhs) != static_cast<const T*>(rhs.get());
  }
#endif



  

template <class T>
inline
bool
operator==( const nsAutoArrayPtr<T>& lhs, NSCAP_Zero* rhs )
    
  {
    return static_cast<const void*>(lhs.get()) == reinterpret_cast<const void*>(rhs);
  }

template <class T>
inline
bool
operator==( NSCAP_Zero* lhs, const nsAutoArrayPtr<T>& rhs )
    
  {
    return reinterpret_cast<const void*>(lhs) == static_cast<const void*>(rhs.get());
  }

template <class T>
inline
bool
operator!=( const nsAutoArrayPtr<T>& lhs, NSCAP_Zero* rhs )
    
  {
    return static_cast<const void*>(lhs.get()) != reinterpret_cast<const void*>(rhs);
  }

template <class T>
inline
bool
operator!=( NSCAP_Zero* lhs, const nsAutoArrayPtr<T>& rhs )
    
  {
    return reinterpret_cast<const void*>(lhs) != static_cast<const void*>(rhs.get());
  }


#ifdef HAVE_CPP_TROUBLE_COMPARING_TO_ZERO

  
  

template <class T>
inline
bool
operator==( const nsAutoArrayPtr<T>& lhs, int rhs )
    
  {
    return static_cast<const void*>(lhs.get()) == reinterpret_cast<const void*>(rhs);
  }

template <class T>
inline
bool
operator==( int lhs, const nsAutoArrayPtr<T>& rhs )
    
  {
    return reinterpret_cast<const void*>(lhs) == static_cast<const void*>(rhs.get());
  }

#endif 






template <class T>
class nsRefPtr
  {
    private:

      void
      assign_with_AddRef( T* rawPtr )
        {
          if ( rawPtr )
            rawPtr->AddRef();
          assign_assuming_AddRef(rawPtr);
        }

      void**
      begin_assignment()
        {
          assign_assuming_AddRef(0);
          return reinterpret_cast<void**>(&mRawPtr);
        }

      void
      assign_assuming_AddRef( T* newPtr )
        {
          T* oldPtr = mRawPtr;
          mRawPtr = newPtr;
          if ( oldPtr )
            oldPtr->Release();
        }

    private:
      T* mRawPtr;

    public:
      typedef T element_type;
      
     ~nsRefPtr()
        {
          if ( mRawPtr )
            mRawPtr->Release();
        }

        

      nsRefPtr()
            : mRawPtr(0)
          
        {
        }

      nsRefPtr( const nsRefPtr<T>& aSmartPtr )
            : mRawPtr(aSmartPtr.mRawPtr)
          
        {
          if ( mRawPtr )
            mRawPtr->AddRef();
        }

      nsRefPtr( T* aRawPtr )
            : mRawPtr(aRawPtr)
          
        {
          if ( mRawPtr )
            mRawPtr->AddRef();
        }

      template <typename I>
      nsRefPtr( const already_AddRefed<I>& aSmartPtr )
            : mRawPtr(aSmartPtr.mRawPtr)
          
        {
        }

      nsRefPtr( const nsCOMPtr_helper& helper )
        {
          void* newRawPtr;
          if (NS_FAILED(helper(NS_GET_TEMPLATE_IID(T), &newRawPtr)))
            newRawPtr = 0;
          mRawPtr = static_cast<T*>(newRawPtr);
        }

        

      nsRefPtr<T>&
      operator=( const nsRefPtr<T>& rhs )
          
        {
          assign_with_AddRef(rhs.mRawPtr);
          return *this;
        }

      nsRefPtr<T>&
      operator=( T* rhs )
          
        {
          assign_with_AddRef(rhs);
          return *this;
        }

      template <typename I>
      nsRefPtr<T>&
      operator=( const already_AddRefed<I>& rhs )
          
        {
          assign_assuming_AddRef(rhs.mRawPtr);
          return *this;
        }

      nsRefPtr<T>&
      operator=( const nsCOMPtr_helper& helper )
        {
          void* newRawPtr;
          if (NS_FAILED(helper(NS_GET_TEMPLATE_IID(T), &newRawPtr)))
            newRawPtr = 0;
          assign_assuming_AddRef(static_cast<T*>(newRawPtr));
          return *this;
        }

        

      void
      swap( nsRefPtr<T>& rhs )
          
        {
          T* temp = rhs.mRawPtr;
          rhs.mRawPtr = mRawPtr;
          mRawPtr = temp;
        }

      void
      swap( T*& rhs )
          
        {
          T* temp = rhs;
          rhs = mRawPtr;
          mRawPtr = temp;
        }

      already_AddRefed<T>
      forget()
          
          
        {
          T* temp = 0;
          swap(temp);
          return temp;
        }

      template <typename I>
      void
      forget( I** rhs)
          
          
          
          
        {
          NS_ASSERTION(rhs, "Null pointer passed to forget!");
          *rhs = mRawPtr;
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

      nsCOMPtr_base::nsDerivedSafe<T>*
      operator->() const
        {
          NS_PRECONDITION(mRawPtr != 0, "You can't dereference a NULL nsRefPtr with operator->().");
          return reinterpret_cast<nsCOMPtr_base::nsDerivedSafe<T>*> (get());
        }

      
      
      
#ifndef _MSC_VER
      template <class U, class V>
      U&
      operator->*(U V::* aMember)
        {
          NS_PRECONDITION(mRawPtr != 0, "You can't dereference a NULL nsRefPtr with operator->*().");
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
          NS_PRECONDITION(mRawPtr != 0, "You can't dereference a NULL nsRefPtr with operator*().");
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

template <class T>
inline
nsRefPtr<T>*
address_of( nsRefPtr<T>& aPtr )
  {
    return aPtr.get_address();
  }

template <class T>
inline
const nsRefPtr<T>*
address_of( const nsRefPtr<T>& aPtr )
  {
    return aPtr.get_address();
  }

template <class T>
class nsRefPtrGetterAddRefs
    
















  {
    public:
      explicit
      nsRefPtrGetterAddRefs( nsRefPtr<T>& aSmartPtr )
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
inline
nsRefPtrGetterAddRefs<T>
getter_AddRefs( nsRefPtr<T>& aSmartPtr )
    



  {
    return nsRefPtrGetterAddRefs<T>(aSmartPtr);
  }



  

template <class T, class U>
inline
bool
operator==( const nsRefPtr<T>& lhs, const nsRefPtr<U>& rhs )
  {
    return static_cast<const T*>(lhs.get()) == static_cast<const U*>(rhs.get());
  }


template <class T, class U>
inline
bool
operator!=( const nsRefPtr<T>& lhs, const nsRefPtr<U>& rhs )
  {
    return static_cast<const T*>(lhs.get()) != static_cast<const U*>(rhs.get());
  }


  

template <class T, class U>
inline
bool
operator==( const nsRefPtr<T>& lhs, const U* rhs )
  {
    return static_cast<const T*>(lhs.get()) == static_cast<const U*>(rhs);
  }

template <class T, class U>
inline
bool
operator==( const U* lhs, const nsRefPtr<T>& rhs )
  {
    return static_cast<const U*>(lhs) == static_cast<const T*>(rhs.get());
  }

template <class T, class U>
inline
bool
operator!=( const nsRefPtr<T>& lhs, const U* rhs )
  {
    return static_cast<const T*>(lhs.get()) != static_cast<const U*>(rhs);
  }

template <class T, class U>
inline
bool
operator!=( const U* lhs, const nsRefPtr<T>& rhs )
  {
    return static_cast<const U*>(lhs) != static_cast<const T*>(rhs.get());
  }

  
  
  
  
  
  

#ifndef NSCAP_DONT_PROVIDE_NONCONST_OPEQ
template <class T, class U>
inline
bool
operator==( const nsRefPtr<T>& lhs, U* rhs )
  {
    return static_cast<const T*>(lhs.get()) == const_cast<const U*>(rhs);
  }

template <class T, class U>
inline
bool
operator==( U* lhs, const nsRefPtr<T>& rhs )
  {
    return const_cast<const U*>(lhs) == static_cast<const T*>(rhs.get());
  }

template <class T, class U>
inline
bool
operator!=( const nsRefPtr<T>& lhs, U* rhs )
  {
    return static_cast<const T*>(lhs.get()) != const_cast<const U*>(rhs);
  }

template <class T, class U>
inline
bool
operator!=( U* lhs, const nsRefPtr<T>& rhs )
  {
    return const_cast<const U*>(lhs) != static_cast<const T*>(rhs.get());
  }
#endif



  

template <class T>
inline
bool
operator==( const nsRefPtr<T>& lhs, NSCAP_Zero* rhs )
    
  {
    return static_cast<const void*>(lhs.get()) == reinterpret_cast<const void*>(rhs);
  }

template <class T>
inline
bool
operator==( NSCAP_Zero* lhs, const nsRefPtr<T>& rhs )
    
  {
    return reinterpret_cast<const void*>(lhs) == static_cast<const void*>(rhs.get());
  }

template <class T>
inline
bool
operator!=( const nsRefPtr<T>& lhs, NSCAP_Zero* rhs )
    
  {
    return static_cast<const void*>(lhs.get()) != reinterpret_cast<const void*>(rhs);
  }

template <class T>
inline
bool
operator!=( NSCAP_Zero* lhs, const nsRefPtr<T>& rhs )
    
  {
    return reinterpret_cast<const void*>(lhs) != static_cast<const void*>(rhs.get());
  }


#ifdef HAVE_CPP_TROUBLE_COMPARING_TO_ZERO

  
  

template <class T>
inline
bool
operator==( const nsRefPtr<T>& lhs, int rhs )
    
  {
    return static_cast<const void*>(lhs.get()) == reinterpret_cast<const void*>(rhs);
  }

template <class T>
inline
bool
operator==( int lhs, const nsRefPtr<T>& rhs )
    
  {
    return reinterpret_cast<const void*>(lhs) == static_cast<const void*>(rhs.get());
  }

#endif 

template <class SourceType, class DestinationType>
inline
nsresult
CallQueryInterface( nsRefPtr<SourceType>& aSourcePtr, DestinationType** aDestPtr )
  {
    return CallQueryInterface(aSourcePtr.get(), aDestPtr);
  }



template<class T>
class nsQueryObject : public nsCOMPtr_helper
{
public:
  nsQueryObject(T* aRawPtr)
    : mRawPtr(aRawPtr) {}

  virtual nsresult NS_FASTCALL operator()( const nsIID& aIID, void** aResult ) const {
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
    : mRawPtr(aRawPtr), mErrorPtr(aErrorPtr) {}

  virtual nsresult NS_FASTCALL operator()( const nsIID& aIID, void** aResult ) const {
    nsresult status = mRawPtr ? mRawPtr->QueryInterface(aIID, aResult)
                              : NS_ERROR_NULL_POINTER;
    if (mErrorPtr)
      *mErrorPtr = status;
    return status;
  }
private:
  T* mRawPtr;
  nsresult* mErrorPtr;
};

template<class T>
inline
nsQueryObject<T>
do_QueryObject(T* aRawPtr)
{
  return nsQueryObject<T>(aRawPtr);
}

template<class T>
inline
nsQueryObject<T>
do_QueryObject(nsCOMPtr<T>& aRawPtr)
{
  return nsQueryObject<T>(aRawPtr);
}

template<class T>
inline
nsQueryObject<T>
do_QueryObject(nsRefPtr<T>& aRawPtr)
{
  return nsQueryObject<T>(aRawPtr);
}

template<class T>
inline
nsQueryObjectWithError<T>
do_QueryObject(T* aRawPtr, nsresult* aErrorPtr)
{
  return nsQueryObjectWithError<T>(aRawPtr, aErrorPtr);
}

template<class T>
inline
nsQueryObjectWithError<T>
do_QueryObject(nsCOMPtr<T>& aRawPtr, nsresult* aErrorPtr)
{
  return nsQueryObjectWithError<T>(aRawPtr, aErrorPtr);
}

template<class T>
inline
nsQueryObjectWithError<T>
do_QueryObject(nsRefPtr<T>& aRawPtr, nsresult* aErrorPtr)
{
  return nsQueryObjectWithError<T>(aRawPtr, aErrorPtr);
}



#endif 
