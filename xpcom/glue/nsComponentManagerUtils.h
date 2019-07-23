




































#ifndef nsComponentManagerUtils_h__
#define nsComponentManagerUtils_h__

#ifndef nscore_h__
#include "nscore.h"
#endif

#ifndef nsCOMPtr_h__
#include "nsCOMPtr.h"
#endif

#include "nsIFactory.h"


NS_COM_GLUE nsresult
CallCreateInstance
  (const nsCID &aClass, nsISupports *aDelegate, const nsIID &aIID,
   void **aResult);

NS_COM_GLUE nsresult
CallCreateInstance
  (const char *aContractID, nsISupports *aDelegate, const nsIID &aIID,
   void **aResult);

NS_COM_GLUE nsresult
CallGetClassObject
  (const nsCID &aClass, const nsIID &aIID, void **aResult);

NS_COM_GLUE nsresult
CallGetClassObject
  (const char *aContractID, const nsIID &aIID, void **aResult);


class NS_COM_GLUE nsCreateInstanceByCID : public nsCOMPtr_helper
{
public:
    nsCreateInstanceByCID( const nsCID& aCID, nsISupports* aOuter, nsresult* aErrorPtr )
        : mCID(aCID),
          mOuter(aOuter),
          mErrorPtr(aErrorPtr)
    {
        
    }
    
    virtual nsresult NS_FASTCALL operator()( const nsIID&, void** ) const;
    
private:
    const nsCID&    mCID;
    nsISupports*    mOuter;
    nsresult*       mErrorPtr;
};

class NS_COM_GLUE nsCreateInstanceByContractID : public nsCOMPtr_helper
{
public:
    nsCreateInstanceByContractID( const char* aContractID, nsISupports* aOuter, nsresult* aErrorPtr )
        : mContractID(aContractID),
          mOuter(aOuter),
          mErrorPtr(aErrorPtr)
    {
        
    }
    
    virtual nsresult NS_FASTCALL operator()( const nsIID&, void** ) const;
    
private:
    const char*   mContractID;
    nsISupports*  mOuter;
    nsresult*     mErrorPtr;
};

class NS_COM_GLUE nsCreateInstanceFromFactory : public nsCOMPtr_helper
{
public:
    nsCreateInstanceFromFactory( nsIFactory* aFactory, nsISupports* aOuter, nsresult* aErrorPtr )
        : mFactory(aFactory),
          mOuter(aOuter),
          mErrorPtr(aErrorPtr)
    {
        
    }
    
    virtual nsresult NS_FASTCALL operator()( const nsIID&, void** ) const;
    
private:
    nsIFactory*   mFactory;
    nsISupports*  mOuter;
    nsresult*     mErrorPtr;
};


inline
const nsCreateInstanceByCID
do_CreateInstance( const nsCID& aCID, nsresult* error = 0 )
{
    return nsCreateInstanceByCID(aCID, 0, error);
}

inline
const nsCreateInstanceByCID
do_CreateInstance( const nsCID& aCID, nsISupports* aOuter, nsresult* error = 0 )
{
    return nsCreateInstanceByCID(aCID, aOuter, error);
}

inline
const nsCreateInstanceByContractID
do_CreateInstance( const char* aContractID, nsresult* error = 0 )
{
    return nsCreateInstanceByContractID(aContractID, 0, error);
}

inline
const nsCreateInstanceByContractID
do_CreateInstance( const char* aContractID, nsISupports* aOuter, nsresult* error = 0 )
{
    return nsCreateInstanceByContractID(aContractID, aOuter, error);
}

inline
const nsCreateInstanceFromFactory
do_CreateInstance( nsIFactory* aFactory, nsresult* error = 0 )
{
    return nsCreateInstanceFromFactory(aFactory, 0, error);
}

inline
const nsCreateInstanceFromFactory
do_CreateInstance( nsIFactory* aFactory, nsISupports* aOuter, nsresult* error = 0 )
{
    return nsCreateInstanceFromFactory(aFactory, aOuter, error);
}


class NS_COM_GLUE nsGetClassObjectByCID : public nsCOMPtr_helper
{
public:
    nsGetClassObjectByCID( const nsCID& aCID, nsresult* aErrorPtr )
        : mCID(aCID),
          mErrorPtr(aErrorPtr)
    {
        
    }
    
    virtual nsresult NS_FASTCALL operator()( const nsIID&, void** ) const;
    
private:
    const nsCID&    mCID;
    nsresult*       mErrorPtr;
};

class NS_COM_GLUE nsGetClassObjectByContractID : public nsCOMPtr_helper
{
public:
    nsGetClassObjectByContractID( const char* aContractID, nsresult* aErrorPtr )
        : mContractID(aContractID),
          mErrorPtr(aErrorPtr)
    {
        
    }
    
    virtual nsresult NS_FASTCALL operator()( const nsIID&, void** ) const;
    
private:
    const char*   mContractID;
    nsresult*     mErrorPtr;
};








inline const nsGetClassObjectByCID
do_GetClassObject( const nsCID& aCID, nsresult* error = 0 )
{
    return nsGetClassObjectByCID(aCID, error);
}

inline const nsGetClassObjectByContractID
do_GetClassObject( const char* aContractID, nsresult* error = 0 )
{
    return nsGetClassObjectByContractID(aContractID, error);
}


template <class DestinationType>
inline
nsresult
CallCreateInstance( const nsCID &aClass,
                    nsISupports *aDelegate,
                    DestinationType** aDestination )
{
    NS_PRECONDITION(aDestination, "null parameter");
    
    return CallCreateInstance(aClass, aDelegate,
                              NS_GET_TEMPLATE_IID(DestinationType),
                              NS_REINTERPRET_CAST(void**, aDestination));
}

template <class DestinationType>
inline
nsresult
CallCreateInstance( const nsCID &aClass,
                    DestinationType** aDestination )
{
    NS_PRECONDITION(aDestination, "null parameter");
    
    return CallCreateInstance(aClass, nsnull,
                              NS_GET_TEMPLATE_IID(DestinationType),
                              NS_REINTERPRET_CAST(void**, aDestination));
}

template <class DestinationType>
inline
nsresult
CallCreateInstance( const char *aContractID,
                    nsISupports *aDelegate,
                    DestinationType** aDestination )
{
    NS_PRECONDITION(aContractID, "null parameter");
    NS_PRECONDITION(aDestination, "null parameter");
    
    return CallCreateInstance(aContractID, 
                              aDelegate,
                              NS_GET_TEMPLATE_IID(DestinationType),
                              NS_REINTERPRET_CAST(void**, aDestination));
}

template <class DestinationType>
inline
nsresult
CallCreateInstance( const char *aContractID,
                    DestinationType** aDestination )
{
    NS_PRECONDITION(aContractID, "null parameter");
    NS_PRECONDITION(aDestination, "null parameter");
    
    return CallCreateInstance(aContractID, nsnull,
                              NS_GET_TEMPLATE_IID(DestinationType),
                              NS_REINTERPRET_CAST(void**, aDestination));
}

template <class DestinationType>
inline
nsresult
CallCreateInstance( nsIFactory *aFactory,
                    nsISupports *aDelegate,
                    DestinationType** aDestination )
{
    NS_PRECONDITION(aFactory, "null parameter");
    NS_PRECONDITION(aDestination, "null parameter");
    
    return aFactory->CreateInstance(aDelegate,
                                    NS_GET_TEMPLATE_IID(DestinationType),
                                    NS_REINTERPRET_CAST(void**, aDestination));
}

template <class DestinationType>
inline
nsresult
CallCreateInstance( nsIFactory *aFactory,
                    DestinationType** aDestination )
{
    NS_PRECONDITION(aFactory, "null parameter");
    NS_PRECONDITION(aDestination, "null parameter");
    
    return aFactory->CreateInstance(nsnull,
                                    NS_GET_TEMPLATE_IID(DestinationType),
                                    NS_REINTERPRET_CAST(void**, aDestination));
}

template <class DestinationType>
inline
nsresult
CallGetClassObject( const nsCID &aClass,
                    DestinationType** aDestination )
{
    NS_PRECONDITION(aDestination, "null parameter");
    
    return CallGetClassObject(aClass,
        NS_GET_TEMPLATE_IID(DestinationType), NS_REINTERPRET_CAST(void**, aDestination));
}

template <class DestinationType>
inline
nsresult
CallGetClassObject( const char* aContractID,
                    DestinationType** aDestination )
{
    NS_PRECONDITION(aDestination, "null parameter");
    
    return CallGetClassObject(aContractID,
        NS_GET_TEMPLATE_IID(DestinationType), NS_REINTERPRET_CAST(void**, aDestination));
}

#endif 
