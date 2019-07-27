





#ifndef __nsInterfaceRequestorUtils_h
#define __nsInterfaceRequestorUtils_h

#include "nsCOMPtr.h"



template<class T, class DestinationType>
inline nsresult
CallGetInterface(T* aSource, DestinationType** aDestination)
{
  NS_PRECONDITION(aSource, "null parameter");
  NS_PRECONDITION(aDestination, "null parameter");

  return aSource->GetInterface(NS_GET_TEMPLATE_IID(DestinationType),
                               reinterpret_cast<void**>(aDestination));
}

class MOZ_STACK_CLASS nsGetInterface final : public nsCOMPtr_helper
{
public:
  nsGetInterface(nsISupports* aSource, nsresult* aError)
    : mSource(aSource)
    , mErrorPtr(aError)
  {
  }

  virtual nsresult NS_FASTCALL operator()(const nsIID&, void**) const
    override;

private:
  nsISupports* MOZ_NON_OWNING_REF mSource;
  nsresult* mErrorPtr;
};

inline const nsGetInterface
do_GetInterface(nsISupports* aSource, nsresult* aError = 0)
{
  return nsGetInterface(aSource, aError);
}

#endif 

