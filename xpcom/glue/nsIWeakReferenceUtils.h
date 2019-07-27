





#ifndef nsIWeakReferenceUtils_h__
#define nsIWeakReferenceUtils_h__

#include "nsCOMPtr.h"
#include "nsIWeakReference.h"

typedef nsCOMPtr<nsIWeakReference> nsWeakPtr;







template<class T, class DestinationType>
inline nsresult
CallQueryReferent(T* aSource, DestinationType** aDestination)
{
  NS_PRECONDITION(aSource, "null parameter");
  NS_PRECONDITION(aDestination, "null parameter");

  return aSource->QueryReferent(NS_GET_TEMPLATE_IID(DestinationType),
                                reinterpret_cast<void**>(aDestination));
}


class MOZ_STACK_CLASS nsQueryReferent final : public nsCOMPtr_helper
{
public:
  nsQueryReferent(nsIWeakReference* aWeakPtr, nsresult* aError)
    : mWeakPtr(aWeakPtr)
    , mErrorPtr(aError)
  {
  }

  virtual nsresult NS_FASTCALL operator()(const nsIID& aIID, void**) const
    override;

private:
  nsIWeakReference* MOZ_NON_OWNING_REF mWeakPtr;
  nsresult*          mErrorPtr;
};

inline const nsQueryReferent
do_QueryReferent(nsIWeakReference* aRawPtr, nsresult* aError = 0)
{
  return nsQueryReferent(aRawPtr, aError);
}





extern nsIWeakReference* NS_GetWeakReference(nsISupports*,
                                             nsresult* aResult = 0);








inline already_AddRefed<nsIWeakReference>
do_GetWeakReference(nsISupports* aRawPtr, nsresult* aError = 0)
{
  return dont_AddRef(NS_GetWeakReference(aRawPtr, aError));
}

inline void
do_GetWeakReference(nsIWeakReference* aRawPtr, nsresult* aError = 0)
{
  
  
  
}

template<class T>
inline void
do_GetWeakReference(already_AddRefed<T>&)
{
  
  
  
}

template<class T>
inline void
do_GetWeakReference(already_AddRefed<T>&, nsresult*)
{
  
  
  
}

#endif
