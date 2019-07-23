





































#ifndef nsIWeakReferenceUtils_h__
#define nsIWeakReferenceUtils_h__

#ifndef nsCOMPtr_h__
#include "nsCOMPtr.h"
#endif

typedef nsCOMPtr<nsIWeakReference> nsWeakPtr;







template <class T, class DestinationType>
inline
nsresult
CallQueryReferent( T* aSource, DestinationType** aDestination )
  {
    NS_PRECONDITION(aSource, "null parameter");
    NS_PRECONDITION(aDestination, "null parameter");

    return aSource->QueryReferent(NS_GET_TEMPLATE_IID(DestinationType),
                                  reinterpret_cast<void**>(aDestination));
  }


class NS_COM_GLUE nsQueryReferent : public nsCOMPtr_helper
  {
    public:
      nsQueryReferent( nsIWeakReference* aWeakPtr, nsresult* error )
          : mWeakPtr(aWeakPtr),
            mErrorPtr(error)
        {
          
        }

      virtual nsresult NS_FASTCALL operator()( const nsIID& aIID, void** ) const;

    private:
      nsIWeakReference*  mWeakPtr;
      nsresult*          mErrorPtr;
  };

inline
const nsQueryReferent
do_QueryReferent( nsIWeakReference* aRawPtr, nsresult* error = 0 )
  {
    return nsQueryReferent(aRawPtr, error);
  }


  


extern NS_COM_GLUE
nsIWeakReference*
NS_GetWeakReference( nsISupports* , nsresult* aResult=0 );

  






inline
already_AddRefed<nsIWeakReference>
do_GetWeakReference( nsISupports* aRawPtr, nsresult* error = 0 )
  {
    return NS_GetWeakReference(aRawPtr, error);
  }

inline
void
do_GetWeakReference( nsIWeakReference* aRawPtr, nsresult* error = 0 )
  {
    
    
    
  }

template <class T>
inline
void
do_GetWeakReference( already_AddRefed<T>& )
  {
    
    
    
  }

template <class T>
inline
void
do_GetWeakReference( already_AddRefed<T>&, nsresult* )
  {
    
    
    
  }

#endif
