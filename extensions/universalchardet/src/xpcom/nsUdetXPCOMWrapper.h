




#ifndef _nsUdetXPCOMWrapper_h__
#define _nsUdetXPCOMWrapper_h__
#include "nsISupports.h"
#include "nsICharsetDetector.h"
#include "nsIStringCharsetDetector.h"
#include "nsICharsetDetectionObserver.h"
#include "nsCOMPtr.h"

#include "nsIFactory.h"


#define NS_JA_PSMDETECTOR_CID \
{ 0x12bb8f1b, 0x2389, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


#define NS_JA_STRING_PSMDETECTOR_CID \
{ 0x12bb8f1c, 0x2389, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


class nsXPCOMDetector :  
      public nsUniversalDetector,
      public nsICharsetDetector
{
  NS_DECL_ISUPPORTS
  public:
    nsXPCOMDetector();
    NS_IMETHOD Init(nsICharsetDetectionObserver* aObserver) override;
    NS_IMETHOD DoIt(const char* aBuf, uint32_t aLen, bool *oDontFeedMe) override;
    NS_IMETHOD Done() override;
  protected:
    virtual ~nsXPCOMDetector();
    virtual void Report(const char* aCharset) override;
  private:
    nsCOMPtr<nsICharsetDetectionObserver> mObserver;
};



class nsXPCOMStringDetector :  
      public nsUniversalDetector,
      public nsIStringCharsetDetector
{
  NS_DECL_ISUPPORTS
  public:
    nsXPCOMStringDetector();
    NS_IMETHOD DoIt(const char* aBuf, uint32_t aLen, 
                    const char** oCharset, nsDetectionConfident &oConf) override;
  protected:
    virtual ~nsXPCOMStringDetector();
    virtual void Report(const char* aCharset) override;
  private:
    nsCOMPtr<nsICharsetDetectionObserver> mObserver;
    const char* mResult;
};



class nsJAPSMDetector : public nsXPCOMDetector
{
public:
  nsJAPSMDetector() 
    : nsXPCOMDetector() {}
};

class nsJAStringPSMDetector : public nsXPCOMStringDetector
{
public:
  nsJAStringPSMDetector() 
    : nsXPCOMStringDetector() {}
};

#endif 
