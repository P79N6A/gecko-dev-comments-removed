





































#include "nscore.h"

#include "nsUniversalDetector.h"
#include "nsUdetXPCOMWrapper.h"
#include "nsCharSetProber.h" 

#include "nsUniversalCharDetDll.h"

#include "nsIFactory.h"
#include "nsISupports.h"
#include "pratom.h"
#include "prmem.h"
#include "nsCOMPtr.h"

static NS_DEFINE_CID(kUniversalDetectorCID, NS_UNIVERSAL_DETECTOR_CID);
static NS_DEFINE_CID(kUniversalStringDetectorCID, NS_UNIVERSAL_STRING_DETECTOR_CID);


nsUniversalXPCOMDetector:: nsUniversalXPCOMDetector() : nsUniversalDetector()
{
}

nsUniversalXPCOMDetector::~nsUniversalXPCOMDetector() 
{
}


NS_IMPL_ISUPPORTS1(nsUniversalXPCOMDetector, nsICharsetDetector)


NS_IMETHODIMP nsUniversalXPCOMDetector::Init(
              nsICharsetDetectionObserver* aObserver)
{
  NS_ASSERTION(mObserver == nsnull , "Init twice");
  if(nsnull == aObserver)
    return NS_ERROR_ILLEGAL_VALUE;

  mObserver = aObserver;
  return NS_OK;
}

NS_IMETHODIMP nsUniversalXPCOMDetector::DoIt(const char* aBuf,
              PRUint32 aLen, PRBool* oDontFeedMe)
{
  NS_ASSERTION(mObserver != nsnull , "have not init yet");

  if((nsnull == aBuf) || (nsnull == oDontFeedMe))
    return NS_ERROR_ILLEGAL_VALUE;

  nsresult rv = this->HandleData(aBuf, aLen);
  if (NS_FAILED(rv))
    return rv;

  if (mDone)
  {
    if (mDetectedCharset)
      Report(mDetectedCharset);

    *oDontFeedMe = PR_TRUE;
  }
  *oDontFeedMe = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP nsUniversalXPCOMDetector::Done()
{
  NS_ASSERTION(mObserver != nsnull , "have not init yet");
#ifdef DEBUG_chardet
  for (PRInt32 i = 0; i < NUM_OF_CHARSET_PROBERS; i++)
  {
    
    
    if (mCharSetProbers[i])
      mCharSetProbers[i]->DumpStatus();
  }
#endif

  this->DataEnd();
  return NS_OK;
}

void nsUniversalXPCOMDetector::Report(const char* aCharset)
{
  NS_ASSERTION(mObserver != nsnull , "have not init yet");
#ifdef DEBUG_chardet
  printf("Universal Charset Detector report charset %s . \r\n", aCharset);
#endif
  mObserver->Notify(aCharset, eBestAnswer);
}



nsUniversalXPCOMStringDetector:: nsUniversalXPCOMStringDetector()
  : nsUniversalDetector()
{
}

nsUniversalXPCOMStringDetector::~nsUniversalXPCOMStringDetector() 
{
}

NS_IMPL_ISUPPORTS1(nsUniversalXPCOMStringDetector, nsIStringCharsetDetector)

void nsUniversalXPCOMStringDetector::Report(const char *aCharset) 
{
  mResult = aCharset;
#ifdef DEBUG_chardet
  printf("New Charset Prober report charset %s . \r\n", aCharset);
#endif
}

NS_IMETHODIMP nsUniversalXPCOMStringDetector::DoIt(const char* aBuf,
                     PRUint32 aLen, const char** oCharset,
                     nsDetectionConfident &oConf)
{
  mResult = nsnull;
  this->Reset();
  nsresult rv = this->HandleData(aBuf, aLen); 
  if (NS_FAILED(rv))
    return rv;
  this->DataEnd();
  if (mResult)
  {
    *oCharset=mResult;
    oConf = eBestAnswer;
  }
  return NS_OK;
}
