




































#ifndef nsClassicDetectors_h__
#define nsClassicDetectors_h__

#include "nsCOMPtr.h"
#include "nsIFactory.h"


#define NS_JA_CLASSIC_DETECTOR_CID \
{ 0x1d2394a0, 0x542a, 0x11d3, { 0x91, 0x4d, 0x0, 0x60, 0x8, 0xa6, 0xed, 0xf6 } }


#define NS_JA_CLASSIC_STRING_DETECTOR_CID \
{ 0x1d2394a1, 0x542a, 0x11d3, { 0x91, 0x4d, 0x0, 0x60, 0x8, 0xa6, 0xed, 0xf6 } }


#define NS_KO_CLASSIC_DETECTOR_CID \
{ 0x1d2394a2, 0x542a, 0x11d3, { 0x91, 0x4d, 0x0, 0x60, 0x8, 0xa6, 0xed, 0xf6 } }


#define NS_KO_CLASSIC_STRING_DETECTOR_CID \
{ 0x1d2394a3, 0x542a, 0x11d3, { 0x91, 0x4d, 0x0, 0x60, 0x8, 0xa6, 0xed, 0xf6 } }

class nsClassicDetector : 
      public nsICharsetDetector 
{
public:
  NS_DECL_ISUPPORTS

  nsClassicDetector(const char* language);
  virtual ~nsClassicDetector();
  NS_IMETHOD Init(nsICharsetDetectionObserver* aObserver);
  NS_IMETHOD DoIt(const char* aBuf, PRUint32 aLen, PRBool* oDontFeedMe);
  NS_IMETHOD Done();
 
private:
  nsCOMPtr<nsICharsetDetectionObserver> mObserver;
  char mCharset[65];
  char mLanguage[32];
};


class nsClassicStringDetector : 
      public nsIStringCharsetDetector 
{
public:
  NS_DECL_ISUPPORTS

  nsClassicStringDetector(const char* language);
  virtual ~nsClassicStringDetector();
  NS_IMETHOD DoIt(const char* aBuf, PRUint32 aLen, 
                  const char** oCharset, 
                  nsDetectionConfident &oConfident);
protected:
  char mCharset[65];
  char mLanguage[32];
};

class nsJACharsetClassicDetector : public nsClassicDetector
{
public:
  nsJACharsetClassicDetector() 
    : nsClassicDetector("ja") {};
};

class nsJAStringCharsetClassicDetector : public nsClassicStringDetector
{
public:
  nsJAStringCharsetClassicDetector() 
    : nsClassicStringDetector("ja") {};
};

class nsKOCharsetClassicDetector : public nsClassicDetector
{
public:
  nsKOCharsetClassicDetector() 
    : nsClassicDetector("ko") {};
};

class nsKOStringCharsetClassicDetector : public nsClassicStringDetector
{
public:
  nsKOStringCharsetClassicDetector() 
    : nsClassicStringDetector("ko") {};
};

#endif 
