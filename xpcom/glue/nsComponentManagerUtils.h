





#ifndef nsComponentManagerUtils_h__
#define nsComponentManagerUtils_h__

#include "nscore.h"
#include "nsCOMPtr.h"

#include "nsIFactory.h"


nsresult CallCreateInstance(const nsCID& aClass, nsISupports* aDelegate,
                            const nsIID& aIID, void** aResult);

nsresult CallCreateInstance(const char* aContractID, nsISupports* aDelegate,
                            const nsIID& aIID, void** aResult);

nsresult CallGetClassObject(const nsCID& aClass, const nsIID& aIID,
                            void** aResult);

nsresult CallGetClassObject(const char* aContractID, const nsIID& aIID,
                            void** aResult);


class MOZ_STACK_CLASS nsCreateInstanceByCID final : public nsCOMPtr_helper
{
public:
  nsCreateInstanceByCID(const nsCID& aCID, nsISupports* aOuter,
                        nsresult* aErrorPtr)
    : mCID(aCID)
    , mOuter(aOuter)
    , mErrorPtr(aErrorPtr)
  {
  }

  virtual nsresult NS_FASTCALL operator()(const nsIID&, void**) const
    override;

private:
  const nsCID&    mCID;
  nsISupports* MOZ_NON_OWNING_REF mOuter;
  nsresult*       mErrorPtr;
};

class MOZ_STACK_CLASS nsCreateInstanceByContractID final : public nsCOMPtr_helper
{
public:
  nsCreateInstanceByContractID(const char* aContractID, nsISupports* aOuter,
                               nsresult* aErrorPtr)
    : mContractID(aContractID)
    , mOuter(aOuter)
    , mErrorPtr(aErrorPtr)
  {
  }

  virtual nsresult NS_FASTCALL operator()(const nsIID&, void**) const override;

private:
  const char*   mContractID;
  nsISupports* MOZ_NON_OWNING_REF mOuter;
  nsresult*     mErrorPtr;
};

class MOZ_STACK_CLASS nsCreateInstanceFromFactory final : public nsCOMPtr_helper
{
public:
  nsCreateInstanceFromFactory(nsIFactory* aFactory, nsISupports* aOuter,
                              nsresult* aErrorPtr)
    : mFactory(aFactory)
    , mOuter(aOuter)
    , mErrorPtr(aErrorPtr)
  {
  }

  virtual nsresult NS_FASTCALL operator()(const nsIID&, void**) const override;

private:
  nsIFactory* MOZ_NON_OWNING_REF mFactory;
  nsISupports* MOZ_NON_OWNING_REF mOuter;
  nsresult*     mErrorPtr;
};


inline const nsCreateInstanceByCID
do_CreateInstance(const nsCID& aCID, nsresult* aError = 0)
{
  return nsCreateInstanceByCID(aCID, 0, aError);
}

inline const nsCreateInstanceByCID
do_CreateInstance(const nsCID& aCID, nsISupports* aOuter, nsresult* aError = 0)
{
  return nsCreateInstanceByCID(aCID, aOuter, aError);
}

inline const nsCreateInstanceByContractID
do_CreateInstance(const char* aContractID, nsresult* aError = 0)
{
  return nsCreateInstanceByContractID(aContractID, 0, aError);
}

inline const nsCreateInstanceByContractID
do_CreateInstance(const char* aContractID, nsISupports* aOuter,
                  nsresult* aError = 0)
{
  return nsCreateInstanceByContractID(aContractID, aOuter, aError);
}

inline const nsCreateInstanceFromFactory
do_CreateInstance(nsIFactory* aFactory, nsresult* aError = 0)
{
  return nsCreateInstanceFromFactory(aFactory, 0, aError);
}

inline const nsCreateInstanceFromFactory
do_CreateInstance(nsIFactory* aFactory, nsISupports* aOuter,
                  nsresult* aError = 0)
{
  return nsCreateInstanceFromFactory(aFactory, aOuter, aError);
}


class MOZ_STACK_CLASS nsGetClassObjectByCID final : public nsCOMPtr_helper
{
public:
  nsGetClassObjectByCID(const nsCID& aCID, nsresult* aErrorPtr)
    : mCID(aCID)
    , mErrorPtr(aErrorPtr)
  {
  }

  virtual nsresult NS_FASTCALL operator()(const nsIID&, void**) const override;

private:
  const nsCID&    mCID;
  nsresult*       mErrorPtr;
};

class MOZ_STACK_CLASS nsGetClassObjectByContractID final : public nsCOMPtr_helper
{
public:
  nsGetClassObjectByContractID(const char* aContractID, nsresult* aErrorPtr)
    : mContractID(aContractID)
    , mErrorPtr(aErrorPtr)
  {
  }

  virtual nsresult NS_FASTCALL operator()(const nsIID&, void**) const override;

private:
  const char*   mContractID;
  nsresult*     mErrorPtr;
};








inline const nsGetClassObjectByCID
do_GetClassObject(const nsCID& aCID, nsresult* aError = 0)
{
  return nsGetClassObjectByCID(aCID, aError);
}

inline const nsGetClassObjectByContractID
do_GetClassObject(const char* aContractID, nsresult* aError = 0)
{
  return nsGetClassObjectByContractID(aContractID, aError);
}


template<class DestinationType>
inline nsresult
CallCreateInstance(const nsCID& aClass,
                   nsISupports* aDelegate,
                   DestinationType** aDestination)
{
  NS_PRECONDITION(aDestination, "null parameter");

  return CallCreateInstance(aClass, aDelegate,
                            NS_GET_TEMPLATE_IID(DestinationType),
                            reinterpret_cast<void**>(aDestination));
}

template<class DestinationType>
inline nsresult
CallCreateInstance(const nsCID& aClass, DestinationType** aDestination)
{
  NS_PRECONDITION(aDestination, "null parameter");

  return CallCreateInstance(aClass, nullptr,
                            NS_GET_TEMPLATE_IID(DestinationType),
                            reinterpret_cast<void**>(aDestination));
}

template<class DestinationType>
inline nsresult
CallCreateInstance(const char* aContractID,
                   nsISupports* aDelegate,
                   DestinationType** aDestination)
{
  NS_PRECONDITION(aContractID, "null parameter");
  NS_PRECONDITION(aDestination, "null parameter");

  return CallCreateInstance(aContractID,
                            aDelegate,
                            NS_GET_TEMPLATE_IID(DestinationType),
                            reinterpret_cast<void**>(aDestination));
}

template<class DestinationType>
inline nsresult
CallCreateInstance(const char* aContractID, DestinationType** aDestination)
{
  NS_PRECONDITION(aContractID, "null parameter");
  NS_PRECONDITION(aDestination, "null parameter");

  return CallCreateInstance(aContractID, nullptr,
                            NS_GET_TEMPLATE_IID(DestinationType),
                            reinterpret_cast<void**>(aDestination));
}

template<class DestinationType>
inline nsresult
CallCreateInstance(nsIFactory* aFactory,
                   nsISupports* aDelegate,
                   DestinationType** aDestination)
{
  NS_PRECONDITION(aFactory, "null parameter");
  NS_PRECONDITION(aDestination, "null parameter");

  return aFactory->CreateInstance(aDelegate,
                                  NS_GET_TEMPLATE_IID(DestinationType),
                                  reinterpret_cast<void**>(aDestination));
}

template<class DestinationType>
inline nsresult
CallCreateInstance(nsIFactory* aFactory, DestinationType** aDestination)
{
  NS_PRECONDITION(aFactory, "null parameter");
  NS_PRECONDITION(aDestination, "null parameter");

  return aFactory->CreateInstance(nullptr,
                                  NS_GET_TEMPLATE_IID(DestinationType),
                                  reinterpret_cast<void**>(aDestination));
}

template<class DestinationType>
inline nsresult
CallGetClassObject(const nsCID& aClass, DestinationType** aDestination)
{
  NS_PRECONDITION(aDestination, "null parameter");

  return CallGetClassObject(aClass, NS_GET_TEMPLATE_IID(DestinationType),
                            reinterpret_cast<void**>(aDestination));
}

template<class DestinationType>
inline nsresult
CallGetClassObject(const char* aContractID, DestinationType** aDestination)
{
  NS_PRECONDITION(aDestination, "null parameter");

  return CallGetClassObject(aContractID, NS_GET_TEMPLATE_IID(DestinationType),
                            reinterpret_cast<void**>(aDestination));
}

#endif 
