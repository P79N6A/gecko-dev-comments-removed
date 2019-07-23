





































#ifndef __nsInterfaceRequestorUtils_h
#define __nsInterfaceRequestorUtils_h

#include "nsCOMPtr.h"



template <class T, class DestinationType>
inline
nsresult
CallGetInterface( T* aSource, DestinationType** aDestination )
  {
    NS_PRECONDITION(aSource, "null parameter");
    NS_PRECONDITION(aDestination, "null parameter");

    return aSource->GetInterface(NS_GET_TEMPLATE_IID(DestinationType),
                                 NS_REINTERPRET_CAST(void**, aDestination));
  }

class NS_COM_GLUE nsGetInterface : public nsCOMPtr_helper
  {
    public:
      nsGetInterface( nsISupports* aSource, nsresult* error )
          : mSource(aSource),
            mErrorPtr(error)
        {
          
        }

      virtual nsresult NS_FASTCALL operator()( const nsIID&, void** ) const;

    private:
      nsISupports*          mSource;
      nsresult*             mErrorPtr;
  };

inline
const nsGetInterface
do_GetInterface( nsISupports* aSource, nsresult* error = 0 )
  {
    return nsGetInterface(aSource, error);
  }

#endif 

