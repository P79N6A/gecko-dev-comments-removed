




#ifndef nsStreamLoader_h__
#define nsStreamLoader_h__

#include "nsIStreamLoader.h"
#include "nsCOMPtr.h"
#include "mozilla/Attributes.h"
#include "mozilla/Vector.h"

class nsIRequest;

class nsStreamLoader MOZ_FINAL : public nsIStreamLoader
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLOADER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

  nsStreamLoader();

  static nsresult
  Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

protected:
  ~nsStreamLoader();

  static NS_METHOD WriteSegmentFun(nsIInputStream *, void *, const char *,
                                   uint32_t, uint32_t, uint32_t *);

  
  
  void ReleaseData();

  nsCOMPtr<nsIStreamLoaderObserver> mObserver;
  nsCOMPtr<nsISupports>             mContext;  
  nsCOMPtr<nsIRequest>              mRequest;

  
  
  mozilla::Vector<uint8_t, 0> mData;
};

#endif 
