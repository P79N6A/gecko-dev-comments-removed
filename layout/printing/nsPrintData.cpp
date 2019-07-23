




































#include "nsPrintData.h"

#include "nsIStringBundle.h"
#include "nsIServiceManager.h"
#include "nsPrintObject.h"
#include "nsPrintPreviewListener.h"
#include "nsIWebProgressListener.h"



#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif

#include "prlog.h"

#ifdef PR_LOGGING
#define DUMP_LAYOUT_LEVEL 9 // this turns on the dumping of each doucment's layout info
static PRLogModuleInfo * kPrintingLogMod = PR_NewLogModule("printing");
#define PR_PL(_p1)  PR_LOG(kPrintingLogMod, PR_LOG_DEBUG, _p1);
#else
#define PRT_YESNO(_p)
#define PR_PL(_p1)
#endif




nsPrintData::nsPrintData(ePrintDataType aType) :
  mType(aType), mDebugFilePtr(nsnull), mPrintObject(nsnull), mSelectedPO(nsnull),
  mPrintDocList(nsnull), mIsIFrameSelected(PR_FALSE),
  mIsParentAFrameSet(PR_FALSE), mOnStartSent(PR_FALSE),
  mIsAborted(PR_FALSE), mPreparingForPrint(PR_FALSE), mDocWasToBeDestroyed(PR_FALSE),
  mShrinkToFit(PR_FALSE), mPrintFrameType(nsIPrintSettings::kFramesAsIs), 
  mNumPrintablePages(0), mNumPagesPrinted(0),
  mShrinkRatio(1.0), mOrigDCScale(1.0), mPPEventListeners(NULL), 
  mBrandName(nsnull)
{

  nsCOMPtr<nsIStringBundle> brandBundle;
  nsCOMPtr<nsIStringBundleService> svc( do_GetService( NS_STRINGBUNDLE_CONTRACTID ) );
  if (svc) {
    svc->CreateBundle( "chrome://branding/locale/brand.properties", getter_AddRefs( brandBundle ) );
    if (brandBundle) {
      brandBundle->GetStringFromName(NS_LITERAL_STRING("brandShortName").get(), &mBrandName );
    }
  }

  if (!mBrandName) {
    mBrandName = ToNewUnicode(NS_LITERAL_STRING("Mozilla Document"));
  }

}

nsPrintData::~nsPrintData()
{
  
  if (mPPEventListeners) {
    mPPEventListeners->RemoveListeners();
    NS_RELEASE(mPPEventListeners);
  }

  
  if (mOnStartSent && mType != eIsPrintPreview) {
    OnEndPrinting();
  }

  if (mPrintDC && !mDebugFilePtr) {
    PR_PL(("****************** End Document ************************\n"));
    PR_PL(("\n"));
    PRBool isCancelled = PR_FALSE;
    mPrintSettings->GetIsCancelled(&isCancelled);

    nsresult rv = NS_OK;
    if (mType == eIsPrinting) {
      if (!isCancelled && !mIsAborted) {
        rv = mPrintDC->EndDocument();
      } else {
        rv = mPrintDC->AbortDocument();  
      }
      if (NS_FAILED(rv)) {
        
      }
    }
  }

  delete mPrintObject;

  if (mPrintDocList != nsnull) {
    mPrintDocList->Clear();
    delete mPrintDocList;
  }

  if (mBrandName) {
    NS_Free(mBrandName);
  }
}

void nsPrintData::OnStartPrinting()
{
  if (!mOnStartSent) {
    DoOnProgressChange(0, 0, PR_TRUE, nsIWebProgressListener::STATE_START|nsIWebProgressListener::STATE_IS_DOCUMENT);
    mOnStartSent = PR_TRUE;
  }
}

void nsPrintData::OnEndPrinting()
{
  DoOnProgressChange(100, 100, PR_TRUE, nsIWebProgressListener::STATE_STOP|nsIWebProgressListener::STATE_IS_DOCUMENT);
}

void
nsPrintData::DoOnProgressChange(PRInt32      aProgess,
                                PRInt32      aMaxProgress,
                                PRBool       aDoStartStop,
                                PRInt32      aFlag)
{
  if (aProgess == 0) return;

  for (PRInt32 i=0;i<mPrintProgressListeners.Count();i++) {
    nsIWebProgressListener* wpl = mPrintProgressListeners.ObjectAt(i);
    wpl->OnProgressChange(nsnull, nsnull, aProgess, aMaxProgress, aProgess, aMaxProgress);
    if (aDoStartStop) {
      wpl->OnStateChange(nsnull, nsnull, aFlag, 0);
    }
  }
}

