




#include "nsPrintData.h"

#include "nsIStringBundle.h"
#include "nsIServiceManager.h"
#include "nsPrintObject.h"
#include "nsPrintPreviewListener.h"
#include "nsIWebProgressListener.h"
#include "mozilla/Services.h"



#include "prlog.h"

#ifdef PR_LOGGING
#define DUMP_LAYOUT_LEVEL 9 // this turns on the dumping of each doucment's layout info
static PRLogModuleInfo *
GetPrintingLog()
{
  static PRLogModuleInfo *sLog;
  if (!sLog)
    sLog = PR_NewLogModule("printing");
  return sLog;
}
#define PR_PL(_p1)  PR_LOG(GetPrintingLog(), PR_LOG_DEBUG, _p1);
#else
#define PRT_YESNO(_p)
#define PR_PL(_p1)
#endif




nsPrintData::nsPrintData(ePrintDataType aType) :
  mType(aType), mDebugFilePtr(nullptr), mPrintObject(nullptr), mSelectedPO(nullptr),
  mPrintDocList(0), mIsIFrameSelected(false),
  mIsParentAFrameSet(false), mOnStartSent(false),
  mIsAborted(false), mPreparingForPrint(false), mDocWasToBeDestroyed(false),
  mShrinkToFit(false), mPrintFrameType(nsIPrintSettings::kFramesAsIs), 
  mNumPrintablePages(0), mNumPagesPrinted(0),
  mShrinkRatio(1.0), mOrigDCScale(1.0), mPPEventListeners(nullptr), 
  mBrandName(nullptr)
{
  MOZ_COUNT_CTOR(nsPrintData);
  nsCOMPtr<nsIStringBundle> brandBundle;
  nsCOMPtr<nsIStringBundleService> svc =
    mozilla::services::GetStringBundleService();
  if (svc) {
    svc->CreateBundle( "chrome://branding/locale/brand.properties", getter_AddRefs( brandBundle ) );
    if (brandBundle) {
      brandBundle->GetStringFromName(MOZ_UTF16("brandShortName"), &mBrandName );
    }
  }

  if (!mBrandName) {
    mBrandName = ToNewUnicode(NS_LITERAL_STRING("Mozilla Document"));
  }

}

nsPrintData::~nsPrintData()
{
  MOZ_COUNT_DTOR(nsPrintData);
  
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
    bool isCancelled = false;
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

  if (mBrandName) {
    free(mBrandName);
  }
}

void nsPrintData::OnStartPrinting()
{
  if (!mOnStartSent) {
    DoOnProgressChange(0, 0, true, nsIWebProgressListener::STATE_START|nsIWebProgressListener::STATE_IS_DOCUMENT|nsIWebProgressListener::STATE_IS_NETWORK);
    mOnStartSent = true;
  }
}

void nsPrintData::OnEndPrinting()
{
  DoOnProgressChange(100, 100, true, nsIWebProgressListener::STATE_STOP|nsIWebProgressListener::STATE_IS_DOCUMENT);
  DoOnProgressChange(100, 100, true, nsIWebProgressListener::STATE_STOP|nsIWebProgressListener::STATE_IS_NETWORK);
}

void
nsPrintData::DoOnProgressChange(int32_t      aProgress,
                                int32_t      aMaxProgress,
                                bool         aDoStartStop,
                                int32_t      aFlag)
{
  for (int32_t i=0;i<mPrintProgressListeners.Count();i++) {
    nsIWebProgressListener* wpl = mPrintProgressListeners.ObjectAt(i);
    wpl->OnProgressChange(nullptr, nullptr, aProgress, aMaxProgress, aProgress, aMaxProgress);
    if (aDoStartStop) {
      wpl->OnStateChange(nullptr, nullptr, aFlag, NS_OK);
    }
  }
}

