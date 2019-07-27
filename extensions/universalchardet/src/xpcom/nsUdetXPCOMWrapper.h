




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


#define NS_JA_PSMDETECTOR_CID \
{ 0x12bb8f1b, 0x2389, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


#define NS_JA_STRING_PSMDETECTOR_CID \
{ 0x12bb8f1c, 0x2389, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


#define NS_KO_PSMDETECTOR_CID \
{ 0xea06d4e1, 0x2b3d, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


#define NS_ZHCN_PSMDETECTOR_CID \
{ 0xea06d4e2, 0x2b3d, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


#define NS_ZHTW_PSMDETECTOR_CID \
{ 0xea06d4e3, 0x2b3d, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }



#define NS_KO_STRING_PSMDETECTOR_CID \
{ 0xea06d4e4, 0x2b3d, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


#define NS_ZHCN_STRING_PSMDETECTOR_CID \
{ 0xea06d4e5, 0x2b3d, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


#define NS_ZHTW_STRING_PSMDETECTOR_CID \
{ 0xea06d4e6, 0x2b3d, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }



#define NS_ZH_STRING_PSMDETECTOR_CID \
{ 0xfcacef21, 0x2b40, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


#define NS_CJK_STRING_PSMDETECTOR_CID \
{ 0xfcacef22, 0x2b40, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }



#define NS_ZH_PSMDETECTOR_CID \
{ 0xfcacef23, 0x2b40, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


#define NS_CJK_PSMDETECTOR_CID \
{ 0xfcacef24, 0x2b40, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


class nsXPCOMDetector :  
      public nsUniversalDetector,
      public nsICharsetDetector
{
  NS_DECL_ISUPPORTS
  public:
    explicit nsXPCOMDetector(uint32_t aLanguageFilter);
    NS_IMETHOD Init(nsICharsetDetectionObserver* aObserver);
    NS_IMETHOD DoIt(const char* aBuf, uint32_t aLen, bool *oDontFeedMe);
    NS_IMETHOD Done();
  protected:
    virtual ~nsXPCOMDetector();
    virtual void Report(const char* aCharset);
  private:
    nsCOMPtr<nsICharsetDetectionObserver> mObserver;
};



class nsXPCOMStringDetector :  
      public nsUniversalDetector,
      public nsIStringCharsetDetector
{
  NS_DECL_ISUPPORTS
  public:
    explicit nsXPCOMStringDetector(uint32_t aLanguageFilter);
    NS_IMETHOD DoIt(const char* aBuf, uint32_t aLen, 
                    const char** oCharset, nsDetectionConfident &oConf);
  protected:
    virtual ~nsXPCOMStringDetector();
    virtual void Report(const char* aCharset);
  private:
    nsCOMPtr<nsICharsetDetectionObserver> mObserver;
    const char* mResult;
};


class nsUniversalXPCOMDetector : public nsXPCOMDetector
{
public:
  nsUniversalXPCOMDetector() 
    : nsXPCOMDetector(NS_FILTER_ALL) {}
};

class nsUniversalXPCOMStringDetector : public nsXPCOMStringDetector
{
public:
  nsUniversalXPCOMStringDetector() 
    : nsXPCOMStringDetector(NS_FILTER_ALL) {}
};

class nsJAPSMDetector : public nsXPCOMDetector
{
public:
  nsJAPSMDetector() 
    : nsXPCOMDetector(NS_FILTER_JAPANESE) {}
};

class nsJAStringPSMDetector : public nsXPCOMStringDetector
{
public:
  nsJAStringPSMDetector() 
    : nsXPCOMStringDetector(NS_FILTER_JAPANESE) {}
};

class nsKOPSMDetector : public nsXPCOMDetector
{
public:
  nsKOPSMDetector() 
    : nsXPCOMDetector(NS_FILTER_KOREAN) {}
};

class nsKOStringPSMDetector : public nsXPCOMStringDetector
{
public:
  nsKOStringPSMDetector() 
    : nsXPCOMStringDetector(NS_FILTER_KOREAN) {}
};

class nsZHTWPSMDetector : public nsXPCOMDetector
{
public:
  nsZHTWPSMDetector() 
    : nsXPCOMDetector(NS_FILTER_CHINESE_TRADITIONAL) {}
};

class nsZHTWStringPSMDetector : public nsXPCOMStringDetector
{
public:
  nsZHTWStringPSMDetector() 
    : nsXPCOMStringDetector(NS_FILTER_CHINESE_TRADITIONAL) {}
};

class nsZHCNPSMDetector : public nsXPCOMDetector
{
public:
  nsZHCNPSMDetector() 
    : nsXPCOMDetector(NS_FILTER_CHINESE_SIMPLIFIED) {}
};

class nsZHCNStringPSMDetector : public nsXPCOMStringDetector
{
public:
  nsZHCNStringPSMDetector() 
    : nsXPCOMStringDetector(NS_FILTER_CHINESE_SIMPLIFIED) {}
};

class nsZHPSMDetector : public nsXPCOMDetector
{
public:
  nsZHPSMDetector() 
    : nsXPCOMDetector(NS_FILTER_CHINESE) {}
};

class nsZHStringPSMDetector : public nsXPCOMStringDetector
{
public:
  nsZHStringPSMDetector() 
    : nsXPCOMStringDetector(NS_FILTER_CHINESE) {}
};

class nsCJKPSMDetector : public nsXPCOMDetector
{
public:
  nsCJKPSMDetector() 
    : nsXPCOMDetector(NS_FILTER_CJK) {}
};

class nsCJKStringPSMDetector : public nsXPCOMStringDetector
{
public:
  nsCJKStringPSMDetector() 
    : nsXPCOMStringDetector(NS_FILTER_CJK) {}
};

#endif 
