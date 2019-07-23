




































#ifndef _nsUdetXPCOMWrapper_h__
#define _nsUdetXPCOMWrapper_h__
#include "nsISupports.h"
#include "nsICharsetDetector.h"
#include "nsIStringCharsetDetector.h"
#include "nsICharsetDetectionObserver.h"
#include "nsCOMPtr.h"

#include "nsIFactory.h"


#define NS_UNIVERSAL_DETECTOR_CID \
{ 0x374e0cde, 0xf605, 0x4259, { 0x8c, 0x92, 0xe6, 0x39, 0xc6, 0xc2, 0xee, 0xef } }


#define NS_UNIVERSAL_STRING_DETECTOR_CID \
{ 0x6ee5301a, 0x3981, 0x49bd, { 0x85, 0xf8, 0x1a, 0x2c, 0xc2, 0x28, 0xcf, 0x3e } }



class nsUniversalXPCOMDetector :  
      public nsUniversalDetector,
      public nsICharsetDetector
{
  NS_DECL_ISUPPORTS
  public:
    nsUniversalXPCOMDetector();
    virtual ~nsUniversalXPCOMDetector();
    NS_IMETHOD Init(nsICharsetDetectionObserver* aObserver);
    NS_IMETHOD DoIt(const char* aBuf, PRUint32 aLen, PRBool *oDontFeedMe);
    NS_IMETHOD Done();
  protected:
    virtual void Report(const char* aCharset);
  private:
    nsCOMPtr<nsICharsetDetectionObserver> mObserver;
};



class nsUniversalXPCOMStringDetector :  
      public nsUniversalDetector,
      public nsIStringCharsetDetector
{
  NS_DECL_ISUPPORTS
  public:
    nsUniversalXPCOMStringDetector();
    virtual ~nsUniversalXPCOMStringDetector();
    NS_IMETHOD DoIt(const char* aBuf, PRUint32 aLen, 
                    const char** oCharset, nsDetectionConfident &oConf);
  protected:
    virtual void Report(const char* aCharset);
  private:
    nsCOMPtr<nsICharsetDetectionObserver> mObserver;
    const char* mResult;
};

#endif 
