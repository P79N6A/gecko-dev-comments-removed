




































#ifndef nsStreamLoader_h__
#define nsStreamLoader_h__

#include "nsIRequest.h"
#include "nsIStreamLoader.h"
#include "nsCOMPtr.h"
#include "nsString.h"

class nsStreamLoader : public nsIStreamLoader
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLOADER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

  nsStreamLoader();
  ~nsStreamLoader();

  static NS_METHOD
  Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

protected:
  static NS_METHOD WriteSegmentFun(nsIInputStream *, void *, const char *,
                                   PRUint32, PRUint32, PRUint32 *);

  nsCOMPtr<nsIStreamLoaderObserver> mObserver;
  nsCOMPtr<nsISupports>             mContext;  
  nsCOMPtr<nsIRequest>              mRequest;

  PRUint8  *mData;
  PRUint32  mAllocated;
  PRUint32  mLength;
};

#endif 
