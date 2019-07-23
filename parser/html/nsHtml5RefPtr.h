







































#ifndef nsHtml5RefPtr_h___
#define nsHtml5RefPtr_h___

#include "nsIThread.h"

template <class T>
class nsHtml5RefPtrReleaser : public nsRunnable
  {
    private:
      T* mPtr;
    public:
      nsHtml5RefPtrReleaser(T* aPtr)
          : mPtr(aPtr)
        {}
      NS_IMETHODIMP Run()
        {
          mPtr->Release();
          return NS_OK;
        }
  };







template <class T>
class nsHtml5RefPtr
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
            release(oldPtr);
        }

      void
      release( T* aPtr )
        {
          nsCOMPtr<nsIRunnable> releaser = new nsHtml5RefPtrReleaser<T>(aPtr);
          if (NS_FAILED(NS_DispatchToMainThread(releaser))) 
            {
              NS_WARNING("Failed to dispatch releaser event.");
            }
        }

    private:
      T* mRawPtr;

    public:
      typedef T element_type;
      
     ~nsHtml5RefPtr()
        {
          if ( mRawPtr )
            release(mRawPtr);
        }

        

      nsHtml5RefPtr()
            : mRawPtr(0)
          
        {
        }

      nsHtml5RefPtr( const nsHtml5RefPtr<T>& aSmartPtr )
            : mRawPtr(aSmartPtr.mRawPtr)
          
        {
          if ( mRawPtr )
            mRawPtr->AddRef();
        }

      nsHtml5RefPtr( T* aRawPtr )
            : mRawPtr(aRawPtr)
          
        {
          if ( mRawPtr )
            mRawPtr->AddRef();
        }

      nsHtml5RefPtr( const already_AddRefed<T>& aSmartPtr )
            : mRawPtr(aSmartPtr.mRawPtr)
          
        {
        }

        

      nsHtml5RefPtr<T>&
      operator=( const nsHtml5RefPtr<T>& rhs )
          
        {
          assign_with_AddRef(rhs.mRawPtr);
          return *this;
        }

      nsHtml5RefPtr<T>&
      operator=( T* rhs )
          
        {
          assign_with_AddRef(rhs);
          return *this;
        }

      nsHtml5RefPtr<T>&
      operator=( const already_AddRefed<T>& rhs )
          
        {
          assign_assuming_AddRef(rhs.mRawPtr);
          return *this;
        }

        

      void
      swap( nsHtml5RefPtr<T>& rhs )
          
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

      T*
      operator->() const
        {
          NS_PRECONDITION(mRawPtr != 0, "You can't dereference a NULL nsHtml5RefPtr with operator->().");
          return get();
        }

#ifdef CANT_RESOLVE_CPP_CONST_AMBIGUITY
  

      nsHtml5RefPtr<T>*
      get_address() const
          
          
        {
          return const_cast<nsHtml5RefPtr<T>*>(this);
        }

#else 

      nsHtml5RefPtr<T>*
      get_address()
          
          
        {
          return this;
        }

      const nsHtml5RefPtr<T>*
      get_address() const
          
          
        {
          return this;
        }

#endif 

    public:
      T&
      operator*() const
        {
          NS_PRECONDITION(mRawPtr != 0, "You can't dereference a NULL nsHtml5RefPtr with operator*().");
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

#ifdef CANT_RESOLVE_CPP_CONST_AMBIGUITY



template <class T>
inline
nsHtml5RefPtr<T>*
address_of( const nsHtml5RefPtr<T>& aPtr )
  {
    return aPtr.get_address();
  }

#else 

template <class T>
inline
nsHtml5RefPtr<T>*
address_of( nsHtml5RefPtr<T>& aPtr )
  {
    return aPtr.get_address();
  }

template <class T>
inline
const nsHtml5RefPtr<T>*
address_of( const nsHtml5RefPtr<T>& aPtr )
  {
    return aPtr.get_address();
  }

#endif 

template <class T>
class nsHtml5RefPtrGetterAddRefs
    
















  {
    public:
      explicit
      nsHtml5RefPtrGetterAddRefs( nsHtml5RefPtr<T>& aSmartPtr )
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
      nsHtml5RefPtr<T>& mTargetSmartPtr;
  };

template <class T>
inline
nsHtml5RefPtrGetterAddRefs<T>
getter_AddRefs( nsHtml5RefPtr<T>& aSmartPtr )
    



  {
    return nsHtml5RefPtrGetterAddRefs<T>(aSmartPtr);
  }



  

template <class T, class U>
inline
NSCAP_BOOL
operator==( const nsHtml5RefPtr<T>& lhs, const nsHtml5RefPtr<U>& rhs )
  {
    return static_cast<const T*>(lhs.get()) == static_cast<const U*>(rhs.get());
  }


template <class T, class U>
inline
NSCAP_BOOL
operator!=( const nsHtml5RefPtr<T>& lhs, const nsHtml5RefPtr<U>& rhs )
  {
    return static_cast<const T*>(lhs.get()) != static_cast<const U*>(rhs.get());
  }


  

template <class T, class U>
inline
NSCAP_BOOL
operator==( const nsHtml5RefPtr<T>& lhs, const U* rhs )
  {
    return static_cast<const T*>(lhs.get()) == static_cast<const U*>(rhs);
  }

template <class T, class U>
inline
NSCAP_BOOL
operator==( const U* lhs, const nsHtml5RefPtr<T>& rhs )
  {
    return static_cast<const U*>(lhs) == static_cast<const T*>(rhs.get());
  }

template <class T, class U>
inline
NSCAP_BOOL
operator!=( const nsHtml5RefPtr<T>& lhs, const U* rhs )
  {
    return static_cast<const T*>(lhs.get()) != static_cast<const U*>(rhs);
  }

template <class T, class U>
inline
NSCAP_BOOL
operator!=( const U* lhs, const nsHtml5RefPtr<T>& rhs )
  {
    return static_cast<const U*>(lhs) != static_cast<const T*>(rhs.get());
  }

  
  
  
  
  
  

#ifndef NSCAP_DONT_PROVIDE_NONCONST_OPEQ
template <class T, class U>
inline
NSCAP_BOOL
operator==( const nsHtml5RefPtr<T>& lhs, U* rhs )
  {
    return static_cast<const T*>(lhs.get()) == const_cast<const U*>(rhs);
  }

template <class T, class U>
inline
NSCAP_BOOL
operator==( U* lhs, const nsHtml5RefPtr<T>& rhs )
  {
    return const_cast<const U*>(lhs) == static_cast<const T*>(rhs.get());
  }

template <class T, class U>
inline
NSCAP_BOOL
operator!=( const nsHtml5RefPtr<T>& lhs, U* rhs )
  {
    return static_cast<const T*>(lhs.get()) != const_cast<const U*>(rhs);
  }

template <class T, class U>
inline
NSCAP_BOOL
operator!=( U* lhs, const nsHtml5RefPtr<T>& rhs )
  {
    return const_cast<const U*>(lhs) != static_cast<const T*>(rhs.get());
  }
#endif



  

template <class T>
inline
NSCAP_BOOL
operator==( const nsHtml5RefPtr<T>& lhs, NSCAP_Zero* rhs )
    
  {
    return static_cast<const void*>(lhs.get()) == reinterpret_cast<const void*>(rhs);
  }

template <class T>
inline
NSCAP_BOOL
operator==( NSCAP_Zero* lhs, const nsHtml5RefPtr<T>& rhs )
    
  {
    return reinterpret_cast<const void*>(lhs) == static_cast<const void*>(rhs.get());
  }

template <class T>
inline
NSCAP_BOOL
operator!=( const nsHtml5RefPtr<T>& lhs, NSCAP_Zero* rhs )
    
  {
    return static_cast<const void*>(lhs.get()) != reinterpret_cast<const void*>(rhs);
  }

template <class T>
inline
NSCAP_BOOL
operator!=( NSCAP_Zero* lhs, const nsHtml5RefPtr<T>& rhs )
    
  {
    return reinterpret_cast<const void*>(lhs) != static_cast<const void*>(rhs.get());
  }


#ifdef HAVE_CPP_TROUBLE_COMPARING_TO_ZERO

  
  

template <class T>
inline
NSCAP_BOOL
operator==( const nsHtml5RefPtr<T>& lhs, int rhs )
    
  {
    return static_cast<const void*>(lhs.get()) == reinterpret_cast<const void*>(rhs);
  }

template <class T>
inline
NSCAP_BOOL
operator==( int lhs, const nsHtml5RefPtr<T>& rhs )
    
  {
    return reinterpret_cast<const void*>(lhs) == static_cast<const void*>(rhs.get());
  }

#endif 

#endif 
