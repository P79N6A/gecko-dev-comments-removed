




#include "nsPrintData.h"

#include "nsIStringBundle.h"
#include "nsIServiceManager.h"
#include "nsPrintObject.h"
#include "nsPrintPreviewListener.h"
#include "nsIWebProgressListener.h"
#include "mozilla/Services.h"

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif



#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif

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
  mShrinkRatio(1.0), mOrigDCScale(1.0), mPPEventListeners(NULL), 
  mBrandName(nullptr)
{
  MOZ_COUNT_CTOR(nsPrintData);
  nsCOMPtr<nsIStringBundle> brandBundle;
  nsCOMPtr<nsIStringBundleService> svc =
    mozilla::services::GetStringBundleService();
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

#ifdef MOZ_CRASHREPORTER
#define ASSERT_AND_NOTE(message) \
  { NS_ASSERTION(false, message); \
    CrashReporter::AppendAppNotesToCrashReport(NS_LITERAL_CSTRING(message "\n")); }
#else
#define ASSERT_AND_NOTE(message) \
  NS_ASSERTION(false, message);
#endif

static void
AssertPresShellsAndContextsSane(nsPrintObject* aPO,
                                nsTArray<nsIPresShell*>& aPresShells,
                                nsTArray<nsPresContext*>& aPresContexts)
{
  if (aPO->mPresShell) {
    if (aPresShells.Contains(aPO->mPresShell)) {
      ASSERT_AND_NOTE("duplicate pres shells in print object tree");
    } else {
      aPresShells.AppendElement(aPO->mPresShell);
    }
  }
  if (aPO->mPresContext) {
    if (aPresContexts.Contains(aPO->mPresContext)) {
      ASSERT_AND_NOTE("duplicate pres contexts in print object tree");
    } else {
      aPresContexts.AppendElement(aPO->mPresContext);
    }
  }
  if (aPO->mPresShell && !aPO->mPresContext) {
    ASSERT_AND_NOTE("print object has pres shell but no pres context");
  }
  if (!aPO->mPresShell && aPO->mPresContext) {
    ASSERT_AND_NOTE("print object has pres context but no pres shell");
  }
  if (aPO->mPresContext &&
      aPO->mPresShell &&
      aPO->mPresContext->GetPresShell() &&
      aPO->mPresContext->GetPresShell() != aPO->mPresShell) {
    ASSERT_AND_NOTE("print object has mismatching pres shell and pres context");
  }

  for (uint32_t i = 0; i < aPO->mKids.Length(); i++) {
    AssertPresShellsAndContextsSane(aPO->mKids[i], aPresShells, aPresContexts);
  }
}

#ifdef MOZ_CRASHREPORTER
static void
AppendBoolean(nsCString& aString, bool aValue)
{
  if (aValue) {
    aString.AppendLiteral("true");
  } else {
    aString.AppendLiteral("false");
  }
}

static void
NotePrintObjectTree(nsPrintObject* aPO, int32_t aDepth)
{
  nsCString note;
  for (int32_t i = 0; i < aDepth; i++) {
    note.AppendLiteral("  ");
  }
  note.AppendInt(static_cast<uint64_t>(reinterpret_cast<uintptr_t>(aPO)), 16);
  note.AppendLiteral(" = { mFrameType = ");
  note.AppendInt(aPO->mFrameType);
  note.AppendLiteral(", mHasBeenPrinted = ");
  AppendBoolean(note, aPO->mHasBeenPrinted);
  note.AppendLiteral(", mDontPrint = ");
  AppendBoolean(note, aPO->mDontPrint);
  note.AppendLiteral(", mPrintAsIs = ");
  AppendBoolean(note, aPO->mPrintAsIs);
  note.AppendLiteral(", mInvisible = ");
  AppendBoolean(note, aPO->mInvisible);
  note.AppendLiteral(", mPrintPreview = ");
  AppendBoolean(note, aPO->mPrintPreview);
  note.AppendLiteral(", mDidCreateDocShell = ");
  AppendBoolean(note, aPO->mDidCreateDocShell);
  note.AppendLiteral(", mShrinkRatio = ");
  note.AppendFloat(aPO->mShrinkRatio);
  note.AppendLiteral(", mZoomRatio = ");
  note.AppendFloat(aPO->mZoomRatio);
  note.AppendLiteral(", mContent = ");
  if (aPO->mContent) {
    nsString tag;
    aPO->mContent->Tag()->ToString(tag);
    LossyAppendUTF16toASCII(tag, note);
  } else {
    note.AppendLiteral("null");
  }
  note.AppendLiteral(" }\n");
  CrashReporter::AppendAppNotesToCrashReport(note);
  for (uint32_t i = 0; i < aPO->mKids.Length(); i++) {
    NotePrintObjectTree(aPO->mKids[i], aDepth + 1);
  }
}

static void
NotePrintObjectTree(nsPrintObject* aPO)
{
  CrashReporter::AppendAppNotesToCrashReport(NS_LITERAL_CSTRING("Print object tree:\n"));
  NotePrintObjectTree(aPO, 1);
}
#endif

#undef ASSERT_AND_NOTE

static void
AssertPresShellsAndContextsSane(nsPrintObject* aPO)
{
  nsTArray<nsIPresShell*> presShells;
  nsTArray<nsPresContext*> presContexts;
  AssertPresShellsAndContextsSane(aPO, presShells, presContexts);
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

  AssertPresShellsAndContextsSane(mPrintObject);
#ifdef MOZ_CRASHREPORTER
  NotePrintObjectTree(mPrintObject);
#endif
  delete mPrintObject;

  if (mBrandName) {
    NS_Free(mBrandName);
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

