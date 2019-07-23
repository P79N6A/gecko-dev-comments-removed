





































#include "nsDebugDetector.h"
#include "pratom.h"
#include "nsCharDetDll.h"


#define REPORT_CHARSET "ISO-8859-7"
#define REPORT_CONFIDENT  eSureAnswer


nsDebugDetector::nsDebugDetector( nsDebugDetectorSel aSel)
{
  mSel = aSel;
  mBlks = 0;
  mObserver = nsnull;
  mStop = PR_FALSE;
}

nsDebugDetector::~nsDebugDetector()
{
}

NS_IMETHODIMP nsDebugDetector::Init(nsICharsetDetectionObserver* aObserver)
{
  NS_ASSERTION(mObserver == nsnull , "Init twice");
  if(nsnull == aObserver)
     return NS_ERROR_ILLEGAL_VALUE;

  mObserver = aObserver;
  return NS_OK;
}


NS_IMETHODIMP nsDebugDetector::DoIt(const char* aBytesArray, PRUint32 aLen, PRBool* oDontFeedMe)
{
  NS_ASSERTION(mObserver != nsnull , "have not init yet");
  NS_ASSERTION(mStop == PR_FALSE , "don't call DoIt if we return PR_TRUE in oDontFeedMe");

  if((nsnull == aBytesArray) || (nsnull == oDontFeedMe))
     return NS_ERROR_ILLEGAL_VALUE;

  mBlks++;
  if((k1stBlk == mSel) && (1 == mBlks)) {
     *oDontFeedMe = mStop = PR_TRUE;
     Report();
  } else if((k2ndBlk == mSel) && (2 == mBlks)) {
     *oDontFeedMe = mStop = PR_TRUE;
     Report();
  } else {
     *oDontFeedMe = mStop = PR_FALSE;
  }
   
  return NS_OK;
}


NS_IMETHODIMP nsDebugDetector::Done()
{
  NS_ASSERTION(mObserver != nsnull , "have not init yet");
  if(klastBlk == mSel)
     Report();
  return NS_OK;
}

void nsDebugDetector::Report()
{
  mObserver->Notify( REPORT_CHARSET, REPORT_CONFIDENT);
}


NS_IMPL_ISUPPORTS1(nsDebugDetector, nsICharsetDetector)

