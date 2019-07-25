





































#ifndef nsUnicharStreamLoader_h__
#define nsUnicharStreamLoader_h__

#include "nsIChannel.h"
#include "nsIUnicharStreamLoader.h"
#include "nsIUnicodeDecoder.h"
#include "nsCOMPtr.h"
#include "nsString.h"

class nsIInputStream;

class nsUnicharStreamLoader : public nsIUnicharStreamLoader
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIUNICHARSTREAMLOADER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

  nsUnicharStreamLoader() {}
  virtual ~nsUnicharStreamLoader() {}

  static nsresult Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

protected:
  nsresult DetermineCharset();

  


  static NS_METHOD WriteSegmentFun(nsIInputStream *, void *, const char *,
                                   PRUint32, PRUint32, PRUint32 *);

  nsCOMPtr<nsIUnicharStreamLoaderObserver> mObserver;
  nsCOMPtr<nsIUnicodeDecoder>              mDecoder;
  nsCOMPtr<nsISupports>                    mContext;
  nsCOMPtr<nsIChannel>                     mChannel;
  nsCString                                mCharset;

  
  
  nsCString                                mRawData;

  
  
  
  nsString                                 mBuffer;
};

#endif 
