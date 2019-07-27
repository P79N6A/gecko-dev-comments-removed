




#ifndef nsStreamLoader_h__
#define nsStreamLoader_h__

#include "nsIThreadRetargetableStreamListener.h"
#include "nsIStreamLoader.h"
#include "nsCOMPtr.h"
#include "mozilla/Attributes.h"
#include "mozilla/Vector.h"

class nsIRequest;

class nsStreamLoader final : public nsIStreamLoader
                               , public nsIThreadRetargetableStreamListener
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSISTREAMLOADER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSITHREADRETARGETABLESTREAMLISTENER

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
