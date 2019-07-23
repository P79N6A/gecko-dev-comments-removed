





































#ifndef nsUnicharStreamLoader_h__
#define nsUnicharStreamLoader_h__

#include "nsIUnicharStreamLoader.h"
#include "nsCOMPtr.h"
#include "nsIChannel.h"
#include "nsString.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"

class nsUnicharStreamLoader : public nsIUnicharStreamLoader
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIUNICHARSTREAMLOADER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

  nsUnicharStreamLoader() { }
  virtual ~nsUnicharStreamLoader() {}

  static NS_METHOD
  Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

protected:
  


  static NS_METHOD WriteSegmentFun(nsIInputStream *, void *, const char *,
                                   PRUint32, PRUint32, PRUint32 *);

  nsCOMPtr<nsIUnicharStreamLoaderObserver> mObserver;
  nsCOMPtr<nsISupports>                    mContext;  
  nsCString                                mCharset;
  nsCOMPtr<nsIChannel>                     mChannel;
  nsCOMPtr<nsIInputStream>                 mInputStream;
  nsCOMPtr<nsIOutputStream>                mOutputStream;
  PRUint32                                 mSegmentSize;
  
};

#endif 
