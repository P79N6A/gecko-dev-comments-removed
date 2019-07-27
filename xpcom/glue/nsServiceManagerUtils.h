





#ifndef nsServiceManagerUtils_h__
#define nsServiceManagerUtils_h__

#include "nsIServiceManager.h"
#include "nsCOMPtr.h"

inline const nsGetServiceByCID
do_GetService(const nsCID& aCID)
{
  return nsGetServiceByCID(aCID);
}

inline const nsGetServiceByCIDWithError
do_GetService(const nsCID& aCID, nsresult* aError)
{
  return nsGetServiceByCIDWithError(aCID, aError);
}

inline const nsGetServiceByContractID
do_GetService(const char* aContractID)
{
  return nsGetServiceByContractID(aContractID);
}

inline const nsGetServiceByContractIDWithError
do_GetService(const char* aContractID, nsresult* aError)
{
  return nsGetServiceByContractIDWithError(aContractID, aError);
}

class MOZ_STACK_CLASS nsGetServiceFromCategory : public nsCOMPtr_helper
{
public:
  nsGetServiceFromCategory(const char* aCategory, const char* aEntry,
                           nsresult* aErrorPtr)
    : mCategory(aCategory)
    , mEntry(aEntry)
    , mErrorPtr(aErrorPtr)
  {
  }

  virtual nsresult NS_FASTCALL operator()(const nsIID&, void**) const;
protected:
  const char*                 mCategory;
  const char*                 mEntry;
  nsresult*                   mErrorPtr;
};

inline const nsGetServiceFromCategory
do_GetServiceFromCategory(const char* aCategory, const char* aEntry,
                          nsresult* aError = 0)
{
  return nsGetServiceFromCategory(aCategory, aEntry, aError);
}

nsresult CallGetService(const nsCID& aClass, const nsIID& aIID, void** aResult);

nsresult CallGetService(const char* aContractID, const nsIID& aIID,
                        void** aResult);


template<class DestinationType>
inline nsresult
CallGetService(const nsCID& aClass,
               DestinationType** aDestination)
{
  NS_PRECONDITION(aDestination, "null parameter");

  return CallGetService(aClass,
                        NS_GET_TEMPLATE_IID(DestinationType),
                        reinterpret_cast<void**>(aDestination));
}

template<class DestinationType>
inline nsresult
CallGetService(const char* aContractID,
               DestinationType** aDestination)
{
  NS_PRECONDITION(aContractID, "null parameter");
  NS_PRECONDITION(aDestination, "null parameter");

  return CallGetService(aContractID,
                        NS_GET_TEMPLATE_IID(DestinationType),
                        reinterpret_cast<void**>(aDestination));
}

#endif
