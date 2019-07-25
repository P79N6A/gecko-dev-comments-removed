




#ifndef nsStreamLoader_h__
#define nsStreamLoader_h__

#include "nsIRequest.h"
#include "nsIStreamLoader.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "mozilla/Attributes.h"

class nsStreamLoader MOZ_FINAL : public nsIStreamLoader
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLOADER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

  nsStreamLoader();
  ~nsStreamLoader();

  static nsresult
  Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

protected:
  static NS_METHOD WriteSegmentFun(nsIInputStream *, void *, const char *,
                                   uint32_t, uint32_t, uint32_t *);

  nsCOMPtr<nsIStreamLoaderObserver> mObserver;
  nsCOMPtr<nsISupports>             mContext;  
  nsCOMPtr<nsIRequest>              mRequest;

  uint8_t  *mData;      
  uint32_t  mAllocated; 
                        
  uint32_t  mLength;    
                        
};

#endif 
