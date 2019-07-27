







#include "mozilla/Attributes.h"

#include "nsWeakReference.h"
#include "nsCOMPtr.h"

class nsWeakReference MOZ_FINAL : public nsIWeakReference
{
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIWEAKREFERENCE
  virtual size_t SizeOfOnlyThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
  friend class nsSupportsWeakReference;

  explicit nsWeakReference(nsSupportsWeakReference* aReferent)
    : mReferent(aReferent)
    
  {
  }

  ~nsWeakReference()
  
  {
    if (mReferent) {
      mReferent->NoticeProxyDestruction();
    }
  }

  void
  NoticeReferentDestruction()
  
  {
    mReferent = 0;
  }

  nsSupportsWeakReference*  mReferent;
};

nsresult
nsQueryReferent::operator()(const nsIID& aIID, void** aAnswer) const
{
  nsresult status;
  if (mWeakPtr) {
    if (NS_FAILED(status = mWeakPtr->QueryReferent(aIID, aAnswer))) {
      *aAnswer = 0;
    }
  } else {
    status = NS_ERROR_NULL_POINTER;
  }

  if (mErrorPtr) {
    *mErrorPtr = status;
  }
  return status;
}

nsIWeakReference*  
NS_GetWeakReference(nsISupports* aInstancePtr, nsresult* aErrorPtr)
{
  nsresult status;

  nsIWeakReference* result = nullptr;

  if (aInstancePtr) {
    nsCOMPtr<nsISupportsWeakReference> factoryPtr =
      do_QueryInterface(aInstancePtr, &status);
    if (factoryPtr) {
      status = factoryPtr->GetWeakReference(&result);
    }
    
  } else {
    status = NS_ERROR_NULL_POINTER;
  }

  if (aErrorPtr) {
    *aErrorPtr = status;
  }
  return result;
}

nsresult
nsSupportsWeakReference::GetWeakReference(nsIWeakReference** aInstancePtr)
{
  if (!aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  if (!mProxy) {
    mProxy = new nsWeakReference(this);
  }
  *aInstancePtr = mProxy;

  nsresult status;
  if (!*aInstancePtr) {
    status = NS_ERROR_OUT_OF_MEMORY;
  } else {
    NS_ADDREF(*aInstancePtr);
    status = NS_OK;
  }

  return status;
}

NS_IMPL_ISUPPORTS(nsWeakReference, nsIWeakReference)

NS_IMETHODIMP
nsWeakReference::QueryReferent(const nsIID& aIID, void** aInstancePtr)
{
  return mReferent ? mReferent->QueryInterface(aIID, aInstancePtr) :
                     NS_ERROR_NULL_POINTER;
}

size_t
nsWeakReference::SizeOfOnlyThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this);
}

void
nsSupportsWeakReference::ClearWeakReferences()
{
  if (mProxy) {
    mProxy->NoticeReferentDestruction();
    mProxy = 0;
  }
}

