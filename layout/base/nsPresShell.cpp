



















#include "mozilla/dom/PBrowserChild.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/Likely.h"
#include "mozilla/Util.h"
#include <algorithm>

#ifdef XP_WIN
#include "winuser.h"
#endif

#include "nsPresShell.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "mozilla/dom/Element.h"
#include "nsIDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsCSSStyleSheet.h"
#include "nsIDOMCSSStyleSheet.h"  
#include "nsAnimationManager.h"
#include "nsINameSpaceManager.h"  
#include "nsIServiceManager.h"
#include "nsFrame.h"
#include "FrameLayerBuilder.h"
#include "nsViewManager.h"
#include "nsView.h"
#include "nsCRTGlue.h"
#include "prlog.h"
#include "prprf.h"
#include "prinrval.h"
#include "nsTArray.h"
#include "nsCOMArray.h"
#include "nsHashtable.h"
#include "nsContainerFrame.h"
#include "nsDOMEvent.h"
#include "nsHTMLParts.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "mozilla/Selection.h"
#include "nsLayoutCID.h"
#include "nsGkAtoms.h"
#include "nsIDOMRange.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMElement.h"
#include "nsRange.h"
#include "nsCSSPseudoElements.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsIPageSequenceFrame.h"
#include "nsCaret.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMXMLDocument.h"
#include "nsFrameManager.h"
#include "nsEventStateManager.h"
#include "nsIMEStateManager.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsILayoutHistoryState.h"
#include "nsILineIterator.h" 
#include "nsWeakPtr.h"
#include "pldhash.h"
#include "nsDOMTouchEvent.h"
#include "nsIObserverService.h"
#include "nsIDocShell.h"        
#include "nsIBaseWindow.h"
#include "nsError.h"
#include "nsLayoutUtils.h"
#include "nsCSSRendering.h"
  
#include "prenv.h"
#include "nsAlgorithm.h"
#include "nsIAttribute.h"
#include "nsIGlobalHistory2.h"
#include "nsDisplayList.h"
#include "nsRegion.h"
#include "nsRenderingContext.h"
#include "nsAutoLayoutPhase.h"
#ifdef MOZ_REFLOW_PERF
#include "nsFontMetrics.h"
#endif
#include "PositionedEventTargeting.h"

#include "nsIReflowCallback.h"

#include "nsPIDOMWindow.h"
#include "nsFocusManager.h"
#include "nsNPAPIPluginInstance.h"
#include "nsIObjectFrame.h"
#include "nsIObjectLoadingContent.h"
#include "nsNetUtil.h"
#include "nsEventDispatcher.h"
#include "nsThreadUtils.h"
#include "nsStyleSheetService.h"
#include "gfxImageSurface.h"
#include "gfxContext.h"
#ifdef MOZ_MEDIA
#include "nsHTMLMediaElement.h"
#endif
#include "nsSMILAnimationController.h"
#include "SVGContentUtils.h"
#include "nsSVGUtils.h"
#include "nsSVGEffects.h"
#include "SVGFragmentIdentifier.h"

#include "nsRefreshDriver.h"


#include "nsWidgetsCID.h"
#include "nsIClipboard.h"
#include "nsIClipboardHelper.h"
#include "nsIDocShellTreeItem.h"
#include "nsIURI.h"
#include "nsIScrollableFrame.h"
#include "prtime.h"
#include "prlong.h"
#include "nsIDragService.h"
#include "nsCopySupport.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMHTMLLinkElement.h"
#include "nsITimer.h"
#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#include "mozilla/a11y/DocAccessible.h"
#ifdef DEBUG
#include "mozilla/a11y/Logging.h"
#endif
#endif


#include "nsStyleChangeList.h"
#include "nsCSSFrameConstructor.h"
#ifdef MOZ_XUL
#include "nsMenuFrame.h"
#include "nsTreeBodyFrame.h"
#include "nsIBoxObject.h"
#include "nsITreeBoxObject.h"
#include "nsMenuPopupFrame.h"
#include "nsITreeColumns.h"
#include "nsIDOMXULMultSelectCntrlEl.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsIDOMXULMenuListElement.h"

#endif
#include "nsPlaceholderFrame.h"
#include "nsCanvasFrame.h"


#include "nsIContentViewer.h"
#include "imgIEncoder.h"
#include "gfxPlatform.h"

#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"
#include "sampler.h"
#include "mozilla/css/ImageLoader.h"

#include "Layers.h"
#include "nsTransitionManager.h"
#include "LayerTreeInvalidation.h"
#include "nsAsyncDOMEvent.h"

#define ANCHOR_SCROLL_FLAGS \
  (nsIPresShell::SCROLL_OVERFLOW_HIDDEN | nsIPresShell::SCROLL_NO_PARENT_FRAMES)

using namespace mozilla;
using namespace mozilla::css;
using namespace mozilla::dom;
using namespace mozilla::layers;

CapturingContentInfo nsIPresShell::gCaptureInfo =
  { false , false , false ,
    false , nullptr  };
nsIContent* nsIPresShell::gKeyDownTarget;
nsInterfaceHashtable<nsUint32HashKey, nsIDOMTouch> nsIPresShell::gCaptureTouchList;
bool nsIPresShell::gPreventMouseEvents = false;



static void ColorToString(nscolor aColor, nsAutoString &aString);


struct RangePaintInfo {
  nsRefPtr<nsRange> mRange;
  nsDisplayListBuilder mBuilder;
  nsDisplayList mList;

  
  nsPoint mRootOffset;

  RangePaintInfo(nsRange* aRange, nsIFrame* aFrame)
    : mRange(aRange), mBuilder(aFrame, nsDisplayListBuilder::PAINTING, false)
  {
    MOZ_COUNT_CTOR(RangePaintInfo);
  }

  ~RangePaintInfo()
  {
    mList.DeleteAll();
    MOZ_COUNT_DTOR(RangePaintInfo);
  }
};

#undef NOISY



#ifdef DEBUG



static uint32_t gVerifyReflowFlags;

struct VerifyReflowFlags {
  const char*    name;
  uint32_t bit;
};

static const VerifyReflowFlags gFlags[] = {
  { "verify",                VERIFY_REFLOW_ON },
  { "reflow",                VERIFY_REFLOW_NOISY },
  { "all",                   VERIFY_REFLOW_ALL },
  { "list-commands",         VERIFY_REFLOW_DUMP_COMMANDS },
  { "noisy-commands",        VERIFY_REFLOW_NOISY_RC },
  { "really-noisy-commands", VERIFY_REFLOW_REALLY_NOISY_RC },
  { "resize",                VERIFY_REFLOW_DURING_RESIZE_REFLOW },
};

#define NUM_VERIFY_REFLOW_FLAGS (sizeof(gFlags) / sizeof(gFlags[0]))

static void
ShowVerifyReflowFlags()
{
  printf("Here are the available GECKO_VERIFY_REFLOW_FLAGS:\n");
  const VerifyReflowFlags* flag = gFlags;
  const VerifyReflowFlags* limit = gFlags + NUM_VERIFY_REFLOW_FLAGS;
  while (flag < limit) {
    printf("  %s\n", flag->name);
    ++flag;
  }
  printf("Note: GECKO_VERIFY_REFLOW_FLAGS is a comma separated list of flag\n");
  printf("names (no whitespace)\n");
}
#endif




#ifdef MOZ_REFLOW_PERF
class ReflowCountMgr;

static const char kGrandTotalsStr[] = "Grand Totals";


class ReflowCounter {
public:
  ReflowCounter(ReflowCountMgr * aMgr = nullptr);
  ~ReflowCounter();

  void ClearTotals();
  void DisplayTotals(const char * aStr);
  void DisplayDiffTotals(const char * aStr);
  void DisplayHTMLTotals(const char * aStr);

  void Add()                { mTotal++;         }
  void Add(uint32_t aTotal) { mTotal += aTotal; }

  void CalcDiffInTotals();
  void SetTotalsCache();

  void SetMgr(ReflowCountMgr * aMgr) { mMgr = aMgr; }

  uint32_t GetTotal() { return mTotal; }
  
protected:
  void DisplayTotals(uint32_t aTotal, const char * aTitle);
  void DisplayHTMLTotals(uint32_t aTotal, const char * aTitle);

  uint32_t mTotal;
  uint32_t mCacheTotal;

  ReflowCountMgr * mMgr; 
};


class IndiReflowCounter {
public:
  IndiReflowCounter(ReflowCountMgr * aMgr = nullptr)
    : mFrame(nullptr),
      mCount(0),
      mMgr(aMgr),
      mCounter(aMgr),
      mHasBeenOutput(false)
    {}
  virtual ~IndiReflowCounter() {}

  nsAutoString mName;
  nsIFrame *   mFrame;   
  int32_t      mCount;

  ReflowCountMgr * mMgr; 

  ReflowCounter mCounter;
  bool          mHasBeenOutput;

};




class ReflowCountMgr {
public:
  ReflowCountMgr();
  virtual ~ReflowCountMgr();

  void ClearTotals();
  void ClearGrandTotals();
  void DisplayTotals(const char * aStr);
  void DisplayHTMLTotals(const char * aStr);
  void DisplayDiffsInTotals(const char * aStr);

  void Add(const char * aName, nsIFrame * aFrame);
  ReflowCounter * LookUp(const char * aName);

  void PaintCount(const char *aName, nsRenderingContext* aRenderingContext,
                  nsPresContext *aPresContext, nsIFrame *aFrame,
                  const nsPoint &aOffset, uint32_t aColor);

  FILE * GetOutFile() { return mFD; }

  PLHashTable * GetIndiFrameHT() { return mIndiFrameCounts; }

  void SetPresContext(nsPresContext * aPresContext) { mPresContext = aPresContext; } 
  void SetPresShell(nsIPresShell* aPresShell) { mPresShell= aPresShell; } 

  void SetDumpFrameCounts(bool aVal)         { mDumpFrameCounts = aVal; }
  void SetDumpFrameByFrameCounts(bool aVal)  { mDumpFrameByFrameCounts = aVal; }
  void SetPaintFrameCounts(bool aVal)        { mPaintFrameByFrameCounts = aVal; }

  bool IsPaintingFrameCounts() { return mPaintFrameByFrameCounts; }

protected:
  void DisplayTotals(uint32_t aTotal, uint32_t * aDupArray, char * aTitle);
  void DisplayHTMLTotals(uint32_t aTotal, uint32_t * aDupArray, char * aTitle);

  static int RemoveItems(PLHashEntry *he, int i, void *arg);
  static int RemoveIndiItems(PLHashEntry *he, int i, void *arg);
  void CleanUp();

  
  static int DoSingleTotal(PLHashEntry *he, int i, void *arg);
  static int DoSingleIndi(PLHashEntry *he, int i, void *arg);

  void DoGrandTotals();
  void DoIndiTotalsTree();

  
  static int DoSingleHTMLTotal(PLHashEntry *he, int i, void *arg);
  void DoGrandHTMLTotals();

  
  static int DoClearTotals(PLHashEntry *he, int i, void *arg);

  
  static int DoDisplayDiffTotals(PLHashEntry *he, int i, void *arg);

  PLHashTable * mCounts;
  PLHashTable * mIndiFrameCounts;
  FILE * mFD;
  
  bool mDumpFrameCounts;
  bool mDumpFrameByFrameCounts;
  bool mPaintFrameByFrameCounts;

  bool mCycledOnce;

  
  nsPresContext * mPresContext;
  nsIPresShell*    mPresShell;

  
};
#endif



#define SHOW_CARET







#define NS_MAX_REFLOW_TIME    1000000
static int32_t gMaxRCProcessingTime = -1;

struct nsCallbackEventRequest
{
  nsIReflowCallback* callback;
  nsCallbackEventRequest* next;
};


#define ASSERT_REFLOW_SCHEDULED_STATE()                                       \
  NS_ASSERTION(mReflowScheduled ==                                            \
                 GetPresContext()->RefreshDriver()->                          \
                   IsLayoutFlushObserver(this), "Unexpected state")

class nsAutoCauseReflowNotifier
{
public:
  nsAutoCauseReflowNotifier(PresShell* aShell)
    : mShell(aShell)
  {
    mShell->WillCauseReflow();
  }
  ~nsAutoCauseReflowNotifier()
  {
    
    
    if (!mShell->mHaveShutDown) {
      mShell->DidCauseReflow();
    }
    else {
      nsContentUtils::RemoveScriptBlocker();
    }
  }

  PresShell* mShell;
};

class NS_STACK_CLASS nsPresShellEventCB : public nsDispatchingCallback
{
public:
  nsPresShellEventCB(PresShell* aPresShell) : mPresShell(aPresShell) {}

  virtual void HandleEvent(nsEventChainPostVisitor& aVisitor)
  {
    if (aVisitor.mPresContext && aVisitor.mEvent->eventStructType != NS_EVENT) {
      if (aVisitor.mEvent->message == NS_MOUSE_BUTTON_DOWN ||
          aVisitor.mEvent->message == NS_MOUSE_BUTTON_UP) {
        
        
        
        
        
        mPresShell->FlushPendingNotifications(Flush_Layout);
      } else if (aVisitor.mEvent->message == NS_WHEEL_WHEEL &&
                 aVisitor.mEventStatus != nsEventStatus_eConsumeNoDefault) {
        nsIFrame* frame = mPresShell->GetCurrentEventFrame();
        if (frame) {
          
          
          
          
          nsRefPtr<nsEventStateManager> esm =
            aVisitor.mPresContext->EventStateManager();
          esm->DispatchLegacyMouseScrollEvents(frame,
                 static_cast<widget::WheelEvent*>(aVisitor.mEvent),
                 &aVisitor.mEventStatus);
        }
      }
      nsIFrame* frame = mPresShell->GetCurrentEventFrame();
      if (!frame &&
          (aVisitor.mEvent->message == NS_MOUSE_BUTTON_UP ||
           aVisitor.mEvent->message == NS_TOUCH_END)) {
        
        
        frame = mPresShell->GetRootFrame();
      }
      if (frame) {
        frame->HandleEvent(aVisitor.mPresContext,
                           (nsGUIEvent*) aVisitor.mEvent,
                           &aVisitor.mEventStatus);
      }
    }
  }

  nsRefPtr<PresShell> mPresShell;
};

class nsBeforeFirstPaintDispatcher : public nsRunnable
{
public:
  nsBeforeFirstPaintDispatcher(nsIDocument* aDocument)
  : mDocument(aDocument) {}

  
  
  NS_IMETHOD Run()
  {
    nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
    if (observerService) {
      observerService->NotifyObservers(mDocument, "before-first-paint", NULL);
    }
    return NS_OK;
  }

private:
  nsCOMPtr<nsIDocument> mDocument;
};

bool PresShell::sDisableNonTestMouseEvents = false;

#ifdef PR_LOGGING
PRLogModuleInfo* PresShell::gLog;
#endif

#ifdef DEBUG
static void
VerifyStyleTree(nsPresContext* aPresContext, nsFrameManager* aFrameManager)
{
  if (nsFrame::GetVerifyStyleTreeEnable()) {
    nsIFrame* rootFrame = aFrameManager->GetRootFrame();
    aFrameManager->DebugVerifyStyleTree(rootFrame);
  }
}
#define VERIFY_STYLE_TREE ::VerifyStyleTree(mPresContext, mFrameConstructor)
#else
#define VERIFY_STYLE_TREE
#endif

static bool gVerifyReflowEnabled;

bool
nsIPresShell::GetVerifyReflowEnable()
{
#ifdef DEBUG
  static bool firstTime = true;
  if (firstTime) {
    firstTime = false;
    char* flags = PR_GetEnv("GECKO_VERIFY_REFLOW_FLAGS");
    if (flags) {
      bool error = false;

      for (;;) {
        char* comma = PL_strchr(flags, ',');
        if (comma)
          *comma = '\0';

        bool found = false;
        const VerifyReflowFlags* flag = gFlags;
        const VerifyReflowFlags* limit = gFlags + NUM_VERIFY_REFLOW_FLAGS;
        while (flag < limit) {
          if (PL_strcasecmp(flag->name, flags) == 0) {
            gVerifyReflowFlags |= flag->bit;
            found = true;
            break;
          }
          ++flag;
        }

        if (! found)
          error = true;

        if (! comma)
          break;

        *comma = ',';
        flags = comma + 1;
      }

      if (error)
        ShowVerifyReflowFlags();
    }

    if (VERIFY_REFLOW_ON & gVerifyReflowFlags) {
      gVerifyReflowEnabled = true;

      printf("Note: verifyreflow is enabled");
      if (VERIFY_REFLOW_NOISY & gVerifyReflowFlags) {
        printf(" (noisy)");
      }
      if (VERIFY_REFLOW_ALL & gVerifyReflowFlags) {
        printf(" (all)");
      }
      if (VERIFY_REFLOW_DUMP_COMMANDS & gVerifyReflowFlags) {
        printf(" (show reflow commands)");
      }
      if (VERIFY_REFLOW_NOISY_RC & gVerifyReflowFlags) {
        printf(" (noisy reflow commands)");
        if (VERIFY_REFLOW_REALLY_NOISY_RC & gVerifyReflowFlags) {
          printf(" (REALLY noisy reflow commands)");
        }
      }
      printf("\n");
    }
  }
#endif
  return gVerifyReflowEnabled;
}
  
void 
PresShell::AddInvalidateHiddenPresShellObserver(nsRefreshDriver *aDriver)
{
  if (!mHiddenInvalidationObserverRefreshDriver && !mIsDestroying && !mHaveShutDown) {
    aDriver->AddPresShellToInvalidateIfHidden(this);
    mHiddenInvalidationObserverRefreshDriver = aDriver;
  }
}

void
nsIPresShell::InvalidatePresShellIfHidden()
{
  if (!IsVisible() && mPresContext) {
    mPresContext->NotifyInvalidation(0);
  }
  mHiddenInvalidationObserverRefreshDriver = nullptr;
}

void
nsIPresShell::SetVerifyReflowEnable(bool aEnabled)
{
  gVerifyReflowEnabled = aEnabled;
}

 void
nsIPresShell::AddWeakFrameExternal(nsWeakFrame* aWeakFrame)
{
  AddWeakFrameInternal(aWeakFrame);
}

void
nsIPresShell::AddWeakFrameInternal(nsWeakFrame* aWeakFrame)
{
  if (aWeakFrame->GetFrame()) {
    aWeakFrame->GetFrame()->AddStateBits(NS_FRAME_EXTERNAL_REFERENCE);
  }
  aWeakFrame->SetPreviousWeakFrame(mWeakFrames);
  mWeakFrames = aWeakFrame;
}

 void
nsIPresShell::RemoveWeakFrameExternal(nsWeakFrame* aWeakFrame)
{
  RemoveWeakFrameInternal(aWeakFrame);
}

void
nsIPresShell::RemoveWeakFrameInternal(nsWeakFrame* aWeakFrame)
{
  if (mWeakFrames == aWeakFrame) {
    mWeakFrames = aWeakFrame->GetPreviousWeakFrame();
    return;
  }
  nsWeakFrame* nextWeak = mWeakFrames;
  while (nextWeak && nextWeak->GetPreviousWeakFrame() != aWeakFrame) {
    nextWeak = nextWeak->GetPreviousWeakFrame();
  }
  if (nextWeak) {
    nextWeak->SetPreviousWeakFrame(aWeakFrame->GetPreviousWeakFrame());
  }
}

already_AddRefed<nsFrameSelection>
nsIPresShell::FrameSelection()
{
  NS_IF_ADDREF(mSelection);
  return mSelection;
}



nsresult
NS_NewPresShell(nsIPresShell** aInstancePtrResult)
{
  NS_PRECONDITION(nullptr != aInstancePtrResult, "null ptr");

  if (!aInstancePtrResult)
    return NS_ERROR_NULL_POINTER;

  *aInstancePtrResult = new PresShell();

  NS_ADDREF(*aInstancePtrResult);
  return NS_OK;
}

static bool sSynthMouseMove = true;

PresShell::PresShell()
  : mMouseLocation(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE)
{
  mSelection = nullptr;
#ifdef MOZ_REFLOW_PERF
  mReflowCountMgr = new ReflowCountMgr();
  mReflowCountMgr->SetPresContext(mPresContext);
  mReflowCountMgr->SetPresShell(this);
#endif
#ifdef PR_LOGGING
  if (! gLog)
    gLog = PR_NewLogModule("PresShell");
#endif
  mSelectionFlags = nsISelectionDisplay::DISPLAY_TEXT | nsISelectionDisplay::DISPLAY_IMAGES;
  mIsThemeSupportDisabled = false;
  mIsActive = true;
  
#ifdef MOZ_ANDROID_OMTC
  
  
  
  mIsFirstPaint = false;
#else
  mIsFirstPaint = true;
#endif
  mFrozen = false;
#ifdef DEBUG
  mPresArenaAllocCount = 0;
#endif
  mRenderFlags = 0;
  mXResolution = 1.0;
  mYResolution = 1.0;
  mViewportOverridden = false;

  mScrollPositionClampingScrollPortSizeSet = false;

  mMaxLineBoxWidth = 0;

  static bool addedSynthMouseMove = false;
  if (!addedSynthMouseMove) {
    Preferences::AddBoolVarCache(&sSynthMouseMove,
                                 "layout.reflow.synthMouseMove", true);
    addedSynthMouseMove = true;
  }
}

NS_IMPL_ISUPPORTS7(PresShell, nsIPresShell, nsIDocumentObserver,
                   nsISelectionController,
                   nsISelectionDisplay, nsIObserver, nsISupportsWeakReference,
                   nsIMutationObserver)

PresShell::~PresShell()
{
  if (!mHaveShutDown) {
    NS_NOTREACHED("Someone did not call nsIPresShell::destroy");
    Destroy();
  }

  NS_ASSERTION(mCurrentEventContentStack.Count() == 0,
               "Huh, event content left on the stack in pres shell dtor!");
  NS_ASSERTION(mFirstCallbackEventRequest == nullptr &&
               mLastCallbackEventRequest == nullptr,
               "post-reflow queues not empty.  This means we're leaking");

#ifdef DEBUG
  MOZ_ASSERT(mPresArenaAllocCount == 0,
             "Some pres arena objects were not freed");
#endif

  delete mStyleSet;
  delete mFrameConstructor;

  mCurrentEventContent = nullptr;

  NS_IF_RELEASE(mPresContext);
  NS_IF_RELEASE(mDocument);
  NS_IF_RELEASE(mSelection);
}





nsresult
PresShell::Init(nsIDocument* aDocument,
                nsPresContext* aPresContext,
                nsViewManager* aViewManager,
                nsStyleSet* aStyleSet,
                nsCompatibility aCompatMode)
{
  NS_PRECONDITION(nullptr != aDocument, "null ptr");
  NS_PRECONDITION(nullptr != aPresContext, "null ptr");
  NS_PRECONDITION(nullptr != aViewManager, "null ptr");
  nsresult result;

  if ((nullptr == aDocument) || (nullptr == aPresContext) ||
      (nullptr == aViewManager)) {
    return NS_ERROR_NULL_POINTER;
  }
  if (mDocument) {
    NS_WARNING("PresShell double init'ed");
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  mFramesToDirty.Init();

  mDocument = aDocument;
  NS_ADDREF(mDocument);
  mViewManager = aViewManager;

  
  mFrameConstructor = new nsCSSFrameConstructor(mDocument, this);

  mFrameManager = mFrameConstructor;

  
  mViewManager->SetPresShell(this);

  
  mPresContext = aPresContext;
  NS_ADDREF(mPresContext);
  aPresContext->SetShell(this);

  
  result = aStyleSet->Init(aPresContext);
  NS_ENSURE_SUCCESS(result, result);

  
  
  
  
  mStyleSet = aStyleSet;

  
  
  
  mPresContext->CompatibilityModeChanged();

  
  
  SetPreferenceStyleRules(false);

  NS_ADDREF(mSelection = new nsFrameSelection());

  
  
  
  
  result =  mFrameConstructor->Init(mStyleSet);
  if (NS_FAILED(result)) {
    NS_WARNING("Frame manager initialization failed");
    mStyleSet = nullptr;
    return result;
  }

  mSelection->Init(this, nullptr);

  
#ifdef SHOW_CARET
  
  mCaret = new nsCaret();
  mCaret->Init(this);
  mOriginalCaret = mCaret;

  
#endif  
  
  
  nsPresContext::nsPresContextType type = aPresContext->Type();
  if (type != nsPresContext::eContext_PrintPreview &&
      type != nsPresContext::eContext_Print)
    SetDisplaySelection(nsISelectionController::SELECTION_DISABLED);
  
  if (gMaxRCProcessingTime == -1) {
    gMaxRCProcessingTime =
      Preferences::GetInt("layout.reflow.timeslice", NS_MAX_REFLOW_TIME);
  }

  {
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (os) {
      os->AddObserver(this, "agent-sheet-added", false);
      os->AddObserver(this, "user-sheet-added", false);
      os->AddObserver(this, "author-sheet-added", false);
      os->AddObserver(this, "agent-sheet-removed", false);
      os->AddObserver(this, "user-sheet-removed", false);
      os->AddObserver(this, "author-sheet-removed", false);
#ifdef MOZ_XUL
      os->AddObserver(this, "chrome-flush-skin-caches", false);
#endif
    }
  }

#ifdef MOZ_REFLOW_PERF
    if (mReflowCountMgr) {
      bool paintFrameCounts =
        Preferences::GetBool("layout.reflow.showframecounts");

      bool dumpFrameCounts =
        Preferences::GetBool("layout.reflow.dumpframecounts");

      bool dumpFrameByFrameCounts =
        Preferences::GetBool("layout.reflow.dumpframebyframecounts");

      mReflowCountMgr->SetDumpFrameCounts(dumpFrameCounts);
      mReflowCountMgr->SetDumpFrameByFrameCounts(dumpFrameByFrameCounts);
      mReflowCountMgr->SetPaintFrameCounts(paintFrameCounts);
    }
#endif

  if (mDocument->HasAnimationController()) {
    nsSMILAnimationController* animCtrl = mDocument->GetAnimationController();
    animCtrl->NotifyRefreshDriverCreated(GetPresContext()->RefreshDriver());
  }

  
  QueryIsActive();

  
  SetupFontInflation();

  return NS_OK;
}

void
PresShell::Destroy()
{
  NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
    "destroy called on presshell while scripts not blocked");

#ifdef MOZ_REFLOW_PERF
  DumpReflows();
  if (mReflowCountMgr) {
    delete mReflowCountMgr;
    mReflowCountMgr = nullptr;
  }
#endif

  if (mHaveShutDown)
    return;

#ifdef ACCESSIBILITY
  if (mDocAccessible) {
#ifdef DEBUG
    if (a11y::logging::IsEnabled(a11y::logging::eDocDestroy))
      a11y::logging::DocDestroy("presshell destroyed", mDocument);
#endif

    mDocAccessible->Shutdown();
    mDocAccessible = nullptr;
  }
#endif 

  MaybeReleaseCapturingContent();

  if (gKeyDownTarget && gKeyDownTarget->OwnerDoc() == mDocument) {
    NS_RELEASE(gKeyDownTarget);
  }

  if (mContentToScrollTo) {
    mContentToScrollTo->DeleteProperty(nsGkAtoms::scrolling);
    mContentToScrollTo = nullptr;
  }

  if (mPresContext) {
    
    
    mPresContext->EventStateManager()->NotifyDestroyPresContext(mPresContext);
  }

  {
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (os) {
      os->RemoveObserver(this, "agent-sheet-added");
      os->RemoveObserver(this, "user-sheet-added");
      os->RemoveObserver(this, "author-sheet-added");
      os->RemoveObserver(this, "agent-sheet-removed");
      os->RemoveObserver(this, "user-sheet-removed");
      os->RemoveObserver(this, "author-sheet-removed");
#ifdef MOZ_XUL
      os->RemoveObserver(this, "chrome-flush-skin-caches");
#endif
    }
  }

  
  if (mPaintSuppressionTimer) {
    mPaintSuppressionTimer->Cancel();
    mPaintSuppressionTimer = nullptr;
  }

  
  if (mReflowContinueTimer) {
    mReflowContinueTimer->Cancel();
    mReflowContinueTimer = nullptr;
  }

  mSynthMouseMoveEvent.Revoke();

  if (mCaret) {
    mCaret->Terminate();
    mCaret = nullptr;
  }

  if (mSelection) {
    mSelection->DisconnectFromPresShell();
  }

  
  ClearPreferenceStyleRules();

  mIsDestroying = true;

  
  
  
  

  
  
  
  

  mCurrentEventFrame = nullptr;

  int32_t i, count = mCurrentEventFrameStack.Length();
  for (i = 0; i < count; i++) {
    mCurrentEventFrameStack[i] = nullptr;
  }

  mFramesToDirty.Clear();

  if (mViewManager) {
    
    
    mViewManager->SetPresShell(nullptr);
    mViewManager = nullptr;
  }

  mStyleSet->BeginShutdown(mPresContext);
  nsRefreshDriver* rd = GetPresContext()->RefreshDriver();

  
  
  
  if (mDocument) {
    NS_ASSERTION(mDocument->GetShell() == this, "Wrong shell?");
    mDocument->DeleteShell();

    if (mDocument->HasAnimationController()) {
      mDocument->GetAnimationController()->NotifyRefreshDriverDestroying(rd);
    }
  }

  
  
  
  rd->RemoveLayoutFlushObserver(this);
  if (mHiddenInvalidationObserverRefreshDriver) {
    mHiddenInvalidationObserverRefreshDriver->RemovePresShellToInvalidateIfHidden(this);
  }

  if (rd->PresContext() == GetPresContext()) {
    rd->RevokeViewManagerFlush();
  }

  mResizeEvent.Revoke();
  if (mAsyncResizeTimerIsActive) {
    mAsyncResizeEventTimer->Cancel();
    mAsyncResizeTimerIsActive = false;
  }

  CancelAllPendingReflows();
  CancelPostedReflowCallbacks();

  
  mFrameConstructor->WillDestroyFrameTree();

  
  
  
  if (mPresContext) {
    
    
    
    
    
    mPresContext->PropertyTable()->DeleteAll();
  }


  NS_WARN_IF_FALSE(!mWeakFrames, "Weak frames alive after destroying FrameManager");
  while (mWeakFrames) {
    mWeakFrames->Clear(this);
  }

  
  mStyleSet->Shutdown(mPresContext);

  if (mPresContext) {
    
    
    
    mPresContext->SetShell(nullptr);

    
    mPresContext->SetLinkHandler(nullptr);
  }

  mHaveShutDown = true;
}

void
PresShell::MakeZombie()
{
  mIsZombie = true;
  CancelAllPendingReflows();
}

void
nsIPresShell::SetAuthorStyleDisabled(bool aStyleDisabled)
{
  if (aStyleDisabled != mStyleSet->GetAuthorStyleDisabled()) {
    mStyleSet->SetAuthorStyleDisabled(aStyleDisabled);
    ReconstructStyleData();
  }
}

bool
nsIPresShell::GetAuthorStyleDisabled() const
{
  return mStyleSet->GetAuthorStyleDisabled();
}

nsresult
PresShell::SetPreferenceStyleRules(bool aForceReflow)
{
  if (!mDocument) {
    return NS_ERROR_NULL_POINTER;
  }

  nsPIDOMWindow *window = mDocument->GetWindow();

  
  
  
  

  if (!window) {
    return NS_ERROR_NULL_POINTER;
  } 

  NS_PRECONDITION(mPresContext, "presContext cannot be null");
  if (mPresContext) {
    
    if (nsContentUtils::IsInChromeDocshell(mDocument)) {
      return NS_OK;
    }

#ifdef DEBUG_attinasi
    printf("Setting Preference Style Rules:\n");
#endif
    
    
    
    
    
    nsresult result = ClearPreferenceStyleRules();
      
    
    
    
    if (NS_SUCCEEDED(result)) {
      result = SetPrefLinkRules();
    }
    if (NS_SUCCEEDED(result)) {
      result = SetPrefFocusRules();
    }
    if (NS_SUCCEEDED(result)) {
      result = SetPrefNoScriptRule();
    }
    if (NS_SUCCEEDED(result)) {
      result = SetPrefNoFramesRule();
    }
#ifdef DEBUG_attinasi
    printf( "Preference Style Rules set: error=%ld\n", (long)result);
#endif

    
    

    return result;
  }

  return NS_ERROR_NULL_POINTER;
}

nsresult PresShell::ClearPreferenceStyleRules(void)
{
  nsresult result = NS_OK;
  if (mPrefStyleSheet) {
    NS_ASSERTION(mStyleSet, "null styleset entirely unexpected!");
    if (mStyleSet) {
      
      
#ifdef DEBUG
      int32_t numBefore = mStyleSet->SheetCount(nsStyleSet::eUserSheet);
      NS_ASSERTION(numBefore > 0, "no user stylesheets in styleset, but we have one!");
#endif
      mStyleSet->RemoveStyleSheet(nsStyleSet::eUserSheet, mPrefStyleSheet);

#ifdef DEBUG_attinasi
      NS_ASSERTION((numBefore - 1) == mStyleSet->GetNumberOfUserStyleSheets(),
                   "Pref stylesheet was not removed");
      printf("PrefStyleSheet removed\n");
#endif
      
      mPrefStyleSheet = nullptr;
    }
  }
  return result;
}

nsresult
PresShell::CreatePreferenceStyleSheet()
{
  NS_ASSERTION(!mPrefStyleSheet, "prefStyleSheet already exists");
  mPrefStyleSheet = new nsCSSStyleSheet(CORS_NONE);
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), "about:PreferenceStyleSheet", nullptr);
  if (NS_FAILED(rv)) {
    mPrefStyleSheet = nullptr;
    return rv;
  }
  NS_ASSERTION(uri, "null but no error");
  mPrefStyleSheet->SetURIs(uri, uri, uri);
  mPrefStyleSheet->SetComplete();
  uint32_t index;
  rv =
    mPrefStyleSheet->InsertRuleInternal(NS_LITERAL_STRING("@namespace svg url(http://www.w3.org/2000/svg);"),
                                        0, &index);
  if (NS_FAILED(rv)) {
    mPrefStyleSheet = nullptr;
    return rv;
  }
  rv =
    mPrefStyleSheet->InsertRuleInternal(NS_LITERAL_STRING("@namespace url(http://www.w3.org/1999/xhtml);"),
                                        0, &index);
  if (NS_FAILED(rv)) {
    mPrefStyleSheet = nullptr;
    return rv;
  }

  mStyleSet->AppendStyleSheet(nsStyleSet::eUserSheet, mPrefStyleSheet);
  return NS_OK;
}




static uint32_t sInsertPrefSheetRulesAt = 2;

nsresult
PresShell::SetPrefNoScriptRule()
{
  nsresult rv = NS_OK;

  
  
  nsIDocument* doc = mDocument;
  if (doc->IsStaticDocument()) {
    doc = doc->GetOriginalDocument();
  }

  bool scriptEnabled = doc->IsScriptEnabled();
  if (scriptEnabled) {
    if (!mPrefStyleSheet) {
      rv = CreatePreferenceStyleSheet();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    uint32_t index = 0;
    mPrefStyleSheet->
      InsertRuleInternal(NS_LITERAL_STRING("noscript{display:none!important}"),
                         sInsertPrefSheetRulesAt, &index);
  }

  return rv;
}

nsresult PresShell::SetPrefNoFramesRule(void)
{
  NS_ASSERTION(mPresContext,"null prescontext not allowed");
  if (!mPresContext) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv = NS_OK;
  
  if (!mPrefStyleSheet) {
    rv = CreatePreferenceStyleSheet();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  NS_ASSERTION(mPrefStyleSheet, "prefstylesheet should not be null");
  
  bool allowSubframes = true;
  nsCOMPtr<nsISupports> container = mPresContext->GetContainer();     
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(container));
  if (docShell) {
    docShell->GetAllowSubframes(&allowSubframes);
  }
  if (!allowSubframes) {
    uint32_t index = 0;
    rv = mPrefStyleSheet->
      InsertRuleInternal(NS_LITERAL_STRING("noframes{display:block}"),
                         sInsertPrefSheetRulesAt, &index);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mPrefStyleSheet->
      InsertRuleInternal(NS_LITERAL_STRING("frame, frameset, iframe {display:none!important}"),
                         sInsertPrefSheetRulesAt, &index);
  }
  return rv;
}
  
nsresult PresShell::SetPrefLinkRules(void)
{
  NS_ASSERTION(mPresContext,"null prescontext not allowed");
  if (!mPresContext) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv = NS_OK;
  
  if (!mPrefStyleSheet) {
    rv = CreatePreferenceStyleSheet();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  NS_ASSERTION(mPrefStyleSheet, "prefstylesheet should not be null");
  
  
  
  
  
  
  
  nscolor linkColor(mPresContext->DefaultLinkColor());
  nscolor activeColor(mPresContext->DefaultActiveLinkColor());
  nscolor visitedColor(mPresContext->DefaultVisitedLinkColor());
  
  NS_NAMED_LITERAL_STRING(ruleClose, "}");
  uint32_t index = 0;
  nsAutoString strColor;

  
  ColorToString(linkColor, strColor);
  rv = mPrefStyleSheet->
    InsertRuleInternal(NS_LITERAL_STRING("*|*:link{color:") +
                       strColor + ruleClose,
                       sInsertPrefSheetRulesAt, &index);
  NS_ENSURE_SUCCESS(rv, rv);

  
  ColorToString(visitedColor, strColor);
  rv = mPrefStyleSheet->
    InsertRuleInternal(NS_LITERAL_STRING("*|*:visited{color:") +
                       strColor + ruleClose,
                       sInsertPrefSheetRulesAt, &index);
  NS_ENSURE_SUCCESS(rv, rv);

  
  ColorToString(activeColor, strColor);
  rv = mPrefStyleSheet->
    InsertRuleInternal(NS_LITERAL_STRING("*|*:-moz-any-link:active{color:") +
                       strColor + ruleClose,
                       sInsertPrefSheetRulesAt, &index);
  NS_ENSURE_SUCCESS(rv, rv);

  bool underlineLinks =
    mPresContext->GetCachedBoolPref(kPresContext_UnderlineLinks);

  if (underlineLinks) {
    
    
    
    
    
    rv = mPrefStyleSheet->
      InsertRuleInternal(NS_LITERAL_STRING("*|*:-moz-any-link:not(svg|a){text-decoration:underline}"),
                         sInsertPrefSheetRulesAt, &index);
  } else {
    rv = mPrefStyleSheet->
      InsertRuleInternal(NS_LITERAL_STRING("*|*:-moz-any-link{text-decoration:none}"),
                         sInsertPrefSheetRulesAt, &index);
  }

  return rv;          
}

nsresult PresShell::SetPrefFocusRules(void)
{
  NS_ASSERTION(mPresContext,"null prescontext not allowed");
  nsresult result = NS_OK;

  if (!mPresContext)
    result = NS_ERROR_FAILURE;

  if (NS_SUCCEEDED(result) && !mPrefStyleSheet)
    result = CreatePreferenceStyleSheet();

  if (NS_SUCCEEDED(result)) {
    NS_ASSERTION(mPrefStyleSheet, "prefstylesheet should not be null");

    if (mPresContext->GetUseFocusColors()) {
      nscolor focusBackground(mPresContext->FocusBackgroundColor());
      nscolor focusText(mPresContext->FocusTextColor());

      
      uint32_t index = 0;
      nsAutoString strRule, strColor;

      
      
      ColorToString(focusText,strColor);
      strRule.AppendLiteral("*:focus,*:focus>font {color: ");
      strRule.Append(strColor);
      strRule.AppendLiteral(" !important; background-color: ");
      ColorToString(focusBackground,strColor);
      strRule.Append(strColor);
      strRule.AppendLiteral(" !important; } ");
      
      result = mPrefStyleSheet->
        InsertRuleInternal(strRule, sInsertPrefSheetRulesAt, &index);
    }
    uint8_t focusRingWidth = mPresContext->FocusRingWidth();
    bool focusRingOnAnything = mPresContext->GetFocusRingOnAnything();
    uint8_t focusRingStyle = mPresContext->GetFocusRingStyle();

    if ((NS_SUCCEEDED(result) && focusRingWidth != 1 && focusRingWidth <= 4 ) || focusRingOnAnything) {
      uint32_t index = 0;
      nsAutoString strRule;
      if (!focusRingOnAnything)
        strRule.AppendLiteral("*|*:link:focus, *|*:visited");    
      strRule.AppendLiteral(":focus {outline: ");     
      strRule.AppendInt(focusRingWidth);
      if (focusRingStyle == 0) 
        strRule.AppendLiteral("px solid -moz-mac-focusring !important; -moz-outline-radius: 3px; outline-offset: 1px; } ");
      else 
        strRule.AppendLiteral("px dotted WindowText !important; } ");
      
      result = mPrefStyleSheet->
        InsertRuleInternal(strRule, sInsertPrefSheetRulesAt, &index);
      NS_ENSURE_SUCCESS(result, result);
      if (focusRingWidth != 1) {
        
        strRule.AssignLiteral("button::-moz-focus-inner, input[type=\"reset\"]::-moz-focus-inner,");
        strRule.AppendLiteral("input[type=\"button\"]::-moz-focus-inner, ");
        strRule.AppendLiteral("input[type=\"submit\"]::-moz-focus-inner { padding: 1px 2px 1px 2px; border: ");
        strRule.AppendInt(focusRingWidth);
        if (focusRingStyle == 0) 
          strRule.AppendLiteral("px solid transparent !important; } ");
        else
          strRule.AppendLiteral("px dotted transparent !important; } ");
        result = mPrefStyleSheet->
          InsertRuleInternal(strRule, sInsertPrefSheetRulesAt, &index);
        NS_ENSURE_SUCCESS(result, result);
          
        strRule.AssignLiteral("button:focus::-moz-focus-inner, input[type=\"reset\"]:focus::-moz-focus-inner,");
        strRule.AppendLiteral("input[type=\"button\"]:focus::-moz-focus-inner, input[type=\"submit\"]:focus::-moz-focus-inner {");
        strRule.AppendLiteral("border-color: ButtonText !important; }");
        result = mPrefStyleSheet->
          InsertRuleInternal(strRule, sInsertPrefSheetRulesAt, &index);
      }
    }
  }
  return result;
}

void
PresShell::AddUserSheet(nsISupports* aSheet)
{
  
  
  
  
  
  nsCOMPtr<nsIStyleSheetService> dummy =
    do_GetService(NS_STYLESHEETSERVICE_CONTRACTID);

  mStyleSet->BeginUpdate();
  
  nsStyleSheetService *sheetService = nsStyleSheetService::gInstance;
  nsCOMArray<nsIStyleSheet> & userSheets = *sheetService->UserStyleSheets();
  int32_t i;
  
  
  for (i = 0; i < userSheets.Count(); ++i) {
    mStyleSet->RemoveStyleSheet(nsStyleSet::eUserSheet, userSheets[i]);
  }

  
  
  for (i = userSheets.Count() - 1; i >= 0; --i) {
    mStyleSet->PrependStyleSheet(nsStyleSet::eUserSheet, userSheets[i]);
  }

  mStyleSet->EndUpdate();

  ReconstructStyleData();
}

void
PresShell::AddAgentSheet(nsISupports* aSheet)
{
  
  
  nsCOMPtr<nsIStyleSheet> sheet = do_QueryInterface(aSheet);
  if (!sheet) {
    return;
  }

  mStyleSet->AppendStyleSheet(nsStyleSet::eAgentSheet, sheet);
  ReconstructStyleData();
}

void
PresShell::AddAuthorSheet(nsISupports* aSheet)
{
  nsCOMPtr<nsIStyleSheet> sheet = do_QueryInterface(aSheet);
  if (!sheet) {
    return;
  }

  
  
  nsIStyleSheet* firstAuthorSheet = mDocument->FirstAdditionalAuthorSheet();
  if (firstAuthorSheet) {
    mStyleSet->InsertStyleSheetBefore(nsStyleSet::eDocSheet, sheet, firstAuthorSheet);
  } else {
    mStyleSet->AppendStyleSheet(nsStyleSet::eDocSheet, sheet);  
  }

  ReconstructStyleData();
}

void
PresShell::RemoveSheet(nsStyleSet::sheetType aType, nsISupports* aSheet)
{
  nsCOMPtr<nsIStyleSheet> sheet = do_QueryInterface(aSheet);
  if (!sheet) {
    return;
  }

  mStyleSet->RemoveStyleSheet(aType, sheet);
  ReconstructStyleData();
}

NS_IMETHODIMP
PresShell::SetDisplaySelection(int16_t aToggle)
{
  mSelection->SetDisplaySelection(aToggle);
  return NS_OK;
}

NS_IMETHODIMP
PresShell::GetDisplaySelection(int16_t *aToggle)
{
  *aToggle = mSelection->GetDisplaySelection();
  return NS_OK;
}

NS_IMETHODIMP
PresShell::GetSelection(SelectionType aType, nsISelection **aSelection)
{
  if (!aSelection || !mSelection)
    return NS_ERROR_NULL_POINTER;

  *aSelection = mSelection->GetSelection(aType);

  if (!(*aSelection))
    return NS_ERROR_INVALID_ARG;

  NS_ADDREF(*aSelection);

  return NS_OK;
}

Selection*
PresShell::GetCurrentSelection(SelectionType aType)
{
  if (!mSelection)
    return nullptr;

  return mSelection->GetSelection(aType);
}

NS_IMETHODIMP
PresShell::ScrollSelectionIntoView(SelectionType aType, SelectionRegion aRegion,
                                   int16_t aFlags)
{
  if (!mSelection)
    return NS_ERROR_NULL_POINTER;

  return mSelection->ScrollSelectionIntoView(aType, aRegion, aFlags);
}

NS_IMETHODIMP
PresShell::RepaintSelection(SelectionType aType)
{
  if (!mSelection)
    return NS_ERROR_NULL_POINTER;

  return mSelection->RepaintSelection(aType);
}


void
PresShell::BeginObservingDocument()
{
  if (mDocument && !mIsDestroying) {
    mDocument->AddObserver(this);
    if (mIsDocumentGone) {
      NS_WARNING("Adding a presshell that was disconnected from the document "
                 "as a document observer?  Sounds wrong...");
      mIsDocumentGone = false;
    }
  }
}


void
PresShell::EndObservingDocument()
{
  
  
  mIsDocumentGone = true;
  if (mDocument) {
    mDocument->RemoveObserver(this);
  }
}

#ifdef DEBUG_kipp
char* nsPresShell_ReflowStackPointerTop;
#endif

nsresult
PresShell::Initialize(nscoord aWidth, nscoord aHeight)
{
  if (mIsDestroying) {
    return NS_OK;
  }

  if (!mDocument) {
    
    return NS_OK;
  }

  mozilla::TimeStamp timerStart = mozilla::TimeStamp::Now();

  NS_ASSERTION(!mDidInitialize, "Why are we being called?");

  nsCOMPtr<nsIPresShell> kungFuDeathGrip(this);
  mDidInitialize = true;

#ifdef DEBUG
  if (VERIFY_REFLOW_NOISY_RC & gVerifyReflowFlags) {
    if (mDocument) {
      nsIURI *uri = mDocument->GetDocumentURI();
      if (uri) {
        nsAutoCString url;
        uri->GetSpec(url);
        printf("*** PresShell::Initialize (this=%p, url='%s')\n", (void*)this, url.get());
      }
    }
  }
#endif

  if (mCaret)
    mCaret->EraseCaret();

  
  

  mPresContext->SetVisibleArea(nsRect(0, 0, aWidth, aHeight));

  
  
  
  
  nsIFrame* rootFrame = mFrameConstructor->GetRootFrame();
  NS_ASSERTION(!rootFrame, "How did that happen, exactly?");
  if (!rootFrame) {
    nsAutoScriptBlocker scriptBlocker;
    mFrameConstructor->BeginUpdate();
    mFrameConstructor->ConstructRootFrame(&rootFrame);
    mFrameConstructor->SetRootFrame(rootFrame);
    mFrameConstructor->EndUpdate();
  }

  NS_ENSURE_STATE(!mHaveShutDown);

  if (!rootFrame) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  for (nsIFrame* f = rootFrame; f; f = nsLayoutUtils::GetCrossDocParentFrame(f)) {
    if (f->GetStateBits() & NS_FRAME_NO_COMPONENT_ALPHA) {
      f->InvalidateFrameSubtree();
      f->RemoveStateBits(NS_FRAME_NO_COMPONENT_ALPHA);
    }
  }

  Element *root = mDocument->GetRootElement();

  if (root) {
    {
      nsAutoCauseReflowNotifier reflowNotifier(this);
      mFrameConstructor->BeginUpdate();

      
      
      mFrameConstructor->ContentInserted(nullptr, root, nullptr, false);
      VERIFY_STYLE_TREE;

      
      
      NS_ENSURE_STATE(!mHaveShutDown);

      mFrameConstructor->EndUpdate();
    }

    
    NS_ENSURE_STATE(!mHaveShutDown);

    
    mDocument->BindingManager()->ProcessAttachedQueue();

    
    NS_ENSURE_STATE(!mHaveShutDown);

    
    
    {
      nsAutoScriptBlocker scriptBlocker;
      mFrameConstructor->CreateNeededFrames();
      mFrameConstructor->ProcessPendingRestyles();
    }

    
    NS_ENSURE_STATE(!mHaveShutDown);
  }

  NS_ASSERTION(rootFrame, "How did that happen?");

  
  
  if (MOZ_LIKELY(rootFrame->GetStateBits() & NS_FRAME_IS_DIRTY)) {
    
    rootFrame->RemoveStateBits(NS_FRAME_IS_DIRTY |
                               NS_FRAME_HAS_DIRTY_CHILDREN);
    NS_ASSERTION(!mDirtyRoots.Contains(rootFrame),
                 "Why is the root in mDirtyRoots already?");
    FrameNeedsReflow(rootFrame, eResize, NS_FRAME_IS_DIRTY);
    NS_ASSERTION(mDirtyRoots.Contains(rootFrame),
                 "Should be in mDirtyRoots now");
    NS_ASSERTION(mReflowScheduled, "Why no reflow scheduled?");
  }

  
  
  
  
  if (!mDocumentLoading) {
    RestoreRootScrollPosition();
  }

  
  if (!mPresContext->IsPaginated()) {
    
    
    
    mPaintingSuppressed = true;
    
    nsIDocument::ReadyState readyState = mDocument->GetReadyStateEnum();
    if (readyState != nsIDocument::READYSTATE_COMPLETE) {
      mPaintSuppressionTimer = do_CreateInstance("@mozilla.org/timer;1");
    }
    if (!mPaintSuppressionTimer) {
      mPaintingSuppressed = false;
    } else {
      

      
      int32_t delay =
        Preferences::GetInt("nglayout.initialpaint.delay",
                            PAINTLOCK_EVENT_DELAY);

      mPaintSuppressionTimer->InitWithFuncCallback(sPaintSuppressionCallback,
                                                   this, delay, 
                                                   nsITimer::TYPE_ONE_SHOT);
    }
  }

  if (root && root->IsXUL()) {
    mozilla::Telemetry::AccumulateTimeDelta(Telemetry::XUL_INITIAL_FRAME_CONSTRUCTION,
                                            timerStart);
  }

  return NS_OK; 
}

void
PresShell::sPaintSuppressionCallback(nsITimer *aTimer, void* aPresShell)
{
  nsRefPtr<PresShell> self = static_cast<PresShell*>(aPresShell);
  if (self)
    self->UnsuppressPainting();
}

void
PresShell::AsyncResizeEventCallback(nsITimer* aTimer, void* aPresShell)
{
  static_cast<PresShell*>(aPresShell)->FireResizeEvent();
}

nsresult
PresShell::ResizeReflowOverride(nscoord aWidth, nscoord aHeight)
{
  mViewportOverridden = true;
  return ResizeReflowIgnoreOverride(aWidth, aHeight);
}

nsresult
PresShell::ResizeReflow(nscoord aWidth, nscoord aHeight)
{
  if (mViewportOverridden) {
    
    
    return NS_OK;
  }
  return ResizeReflowIgnoreOverride(aWidth, aHeight);
}

nsresult
PresShell::ResizeReflowIgnoreOverride(nscoord aWidth, nscoord aHeight)
{
  NS_PRECONDITION(!mIsReflowing, "Shouldn't be in reflow here!");
  NS_PRECONDITION(aWidth != NS_UNCONSTRAINEDSIZE,
                  "shouldn't use unconstrained widths anymore");
  
  
  
  
  nsIFrame* rootFrame = mFrameConstructor->GetRootFrame();

  if (!rootFrame && aHeight == NS_UNCONSTRAINEDSIZE) {
    
    
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsRefPtr<nsViewManager> viewManagerDeathGrip = mViewManager;
  
  nsCOMPtr<nsIPresShell> kungFuDeathGrip(this);

  if (!mIsDestroying && !mResizeEvent.IsPending() &&
      !mAsyncResizeTimerIsActive) {
    FireBeforeResizeEvent();
  }

  mPresContext->SetVisibleArea(nsRect(0, 0, aWidth, aHeight));

  
  rootFrame = mFrameConstructor->GetRootFrame();
  if (!rootFrame)
    return NS_OK;

  if (!GetPresContext()->SupressingResizeReflow())
  {
    
    
    mDocument->FlushPendingNotifications(Flush_ContentAndNotify);

    
    {
      nsAutoScriptBlocker scriptBlocker;
      mFrameConstructor->CreateNeededFrames();
      mFrameConstructor->ProcessPendingRestyles();
    }

    rootFrame = mFrameConstructor->GetRootFrame();
    if (!mIsDestroying && rootFrame) {
      
      

      {
        nsAutoCauseReflowNotifier crNotifier(this);
        WillDoReflow();

        
        AUTO_LAYOUT_PHASE_ENTRY_POINT(GetPresContext(), Reflow);
        nsViewManager::AutoDisableRefresh refreshBlocker(mViewManager);

        mDirtyRoots.RemoveElement(rootFrame);
        DoReflow(rootFrame, true);
      }

      DidDoReflow(true);
    }
  }

  rootFrame = mFrameConstructor->GetRootFrame();
  if (aHeight == NS_UNCONSTRAINEDSIZE && rootFrame) {
    mPresContext->SetVisibleArea(
      nsRect(0, 0, aWidth, rootFrame->GetRect().height));
  }

  if (!mIsDestroying && !mResizeEvent.IsPending() &&
      !mAsyncResizeTimerIsActive) {
    if (mInResize) {
      if (!mAsyncResizeEventTimer) {
        mAsyncResizeEventTimer = do_CreateInstance("@mozilla.org/timer;1");
      }
      if (mAsyncResizeEventTimer) {
        mAsyncResizeTimerIsActive = true;
        mAsyncResizeEventTimer->InitWithFuncCallback(AsyncResizeEventCallback,
                                                     this, 15,
                                                     nsITimer::TYPE_ONE_SHOT);
      }
    } else {
      nsRefPtr<nsRunnableMethod<PresShell> > resizeEvent =
        NS_NewRunnableMethod(this, &PresShell::FireResizeEvent);
      if (NS_SUCCEEDED(NS_DispatchToCurrentThread(resizeEvent))) {
        mResizeEvent = resizeEvent;
        mDocument->SetNeedStyleFlush();
      }
    }
  }

  return NS_OK; 
}

void
PresShell::FireBeforeResizeEvent()
{
  if (mIsDocumentGone)
    return;

  
  nsEvent event(true, NS_BEFORERESIZE_EVENT);

  nsPIDOMWindow *window = mDocument->GetWindow();
  if (window) {
    nsCOMPtr<nsIPresShell> kungFuDeathGrip(this);
    nsEventDispatcher::Dispatch(window, mPresContext, &event);
  }
}

void
PresShell::FireResizeEvent()
{
  if (mAsyncResizeTimerIsActive) {
    mAsyncResizeTimerIsActive = false;
    mAsyncResizeEventTimer->Cancel();
  }
  mResizeEvent.Revoke();

  if (mIsDocumentGone)
    return;

  
  nsEvent event(true, NS_RESIZE_EVENT);
  nsEventStatus status = nsEventStatus_eIgnore;

  nsPIDOMWindow *window = mDocument->GetWindow();
  if (window) {
    nsCOMPtr<nsIPresShell> kungFuDeathGrip(this);
    mInResize = true;
    nsEventDispatcher::Dispatch(window, mPresContext, &event, nullptr, &status);
    mInResize = false;
  }
}

void
PresShell::SetIgnoreFrameDestruction(bool aIgnore)
{
  if (mDocument) {
    
    
    mDocument->StyleImageLoader()->ClearAll();
  }
  mIgnoreFrameDestruction = aIgnore;
}

void
PresShell::NotifyDestroyingFrame(nsIFrame* aFrame)
{
  if (!mIgnoreFrameDestruction) {
    mDocument->StyleImageLoader()->DropRequestsForFrame(aFrame);

    mFrameConstructor->NotifyDestroyingFrame(aFrame);

    for (int32_t idx = mDirtyRoots.Length(); idx; ) {
      --idx;
      if (mDirtyRoots[idx] == aFrame) {
        mDirtyRoots.RemoveElementAt(idx);
      }
    }

    
    mPresContext->NotifyDestroyingFrame(aFrame);

    if (aFrame == mCurrentEventFrame) {
      mCurrentEventContent = aFrame->GetContent();
      mCurrentEventFrame = nullptr;
    }

  #ifdef DEBUG
    if (aFrame == mDrawEventTargetFrame) {
      mDrawEventTargetFrame = nullptr;
    }
  #endif

    for (unsigned int i=0; i < mCurrentEventFrameStack.Length(); i++) {
      if (aFrame == mCurrentEventFrameStack.ElementAt(i)) {
        
        
        nsIContent *currentEventContent = aFrame->GetContent();
        mCurrentEventContentStack.ReplaceObjectAt(currentEventContent, i);
        mCurrentEventFrameStack[i] = nullptr;
      }
    }
  
    mFramesToDirty.RemoveEntry(aFrame);
  } else {
    
    
    
    
    mPresContext->PropertyTable()->
      Delete(aFrame, FrameLayerBuilder::LayerManagerDataProperty());
  }
}

already_AddRefed<nsCaret> PresShell::GetCaret() const
{
  nsCaret* caret = mCaret;
  NS_IF_ADDREF(caret);
  return caret;
}

void PresShell::MaybeInvalidateCaretPosition()
{
  if (mCaret) {
    mCaret->InvalidateOutsideCaret();
  }
}

void PresShell::SetCaret(nsCaret *aNewCaret)
{
  mCaret = aNewCaret;
}

void PresShell::RestoreCaret()
{
  mCaret = mOriginalCaret;
}

NS_IMETHODIMP PresShell::SetCaretEnabled(bool aInEnable)
{
  bool oldEnabled = mCaretEnabled;

  mCaretEnabled = aInEnable;

  if (mCaret && (mCaretEnabled != oldEnabled))
  {





    mCaret->SetCaretVisible(mCaretEnabled);
  }

  return NS_OK;
}

NS_IMETHODIMP PresShell::SetCaretReadOnly(bool aReadOnly)
{
  if (mCaret)
    mCaret->SetCaretReadOnly(aReadOnly);
  return NS_OK;
}

NS_IMETHODIMP PresShell::GetCaretEnabled(bool *aOutEnabled)
{
  NS_ENSURE_ARG_POINTER(aOutEnabled);
  *aOutEnabled = mCaretEnabled;
  return NS_OK;
}

NS_IMETHODIMP PresShell::SetCaretVisibilityDuringSelection(bool aVisibility)
{
  if (mCaret)
    mCaret->SetVisibilityDuringSelection(aVisibility);
  return NS_OK;
}

NS_IMETHODIMP PresShell::GetCaretVisible(bool *aOutIsVisible)
{
  *aOutIsVisible = false;
  if (mCaret) {
    nsresult rv = mCaret->GetCaretVisible(aOutIsVisible);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  return NS_OK;
}

NS_IMETHODIMP PresShell::SetSelectionFlags(int16_t aInEnable)
{
  mSelectionFlags = aInEnable;
  return NS_OK;
}

NS_IMETHODIMP PresShell::GetSelectionFlags(int16_t *aOutEnable)
{
  if (!aOutEnable)
    return NS_ERROR_INVALID_ARG;
  *aOutEnable = mSelectionFlags;
  return NS_OK;
}



NS_IMETHODIMP 
PresShell::CharacterMove(bool aForward, bool aExtend)
{
  return mSelection->CharacterMove(aForward, aExtend);  
}

NS_IMETHODIMP
PresShell::CharacterExtendForDelete()
{
  return mSelection->CharacterExtendForDelete();
}

NS_IMETHODIMP
PresShell::CharacterExtendForBackspace()
{
  return mSelection->CharacterExtendForBackspace();
}

NS_IMETHODIMP 
PresShell::WordMove(bool aForward, bool aExtend)
{
  nsresult result = mSelection->WordMove(aForward, aExtend);


  if (NS_FAILED(result))
    result = CompleteMove(aForward, aExtend);
  return result;
}

NS_IMETHODIMP 
PresShell::WordExtendForDelete(bool aForward)
{
  return mSelection->WordExtendForDelete(aForward);  
}

NS_IMETHODIMP 
PresShell::LineMove(bool aForward, bool aExtend)
{
  nsresult result = mSelection->LineMove(aForward, aExtend);  


  if (NS_FAILED(result)) 
    result = CompleteMove(aForward,aExtend);
  return result;
}

NS_IMETHODIMP 
PresShell::IntraLineMove(bool aForward, bool aExtend)
{
  return mSelection->IntraLineMove(aForward, aExtend);  
}



NS_IMETHODIMP 
PresShell::PageMove(bool aForward, bool aExtend)
{
  nsIScrollableFrame *scrollableFrame =
    GetFrameToScrollAsScrollable(nsIPresShell::eVertical);
  if (!scrollableFrame)
    return NS_OK;

  mSelection->CommonPageMove(aForward, aExtend, scrollableFrame);
  
  
  return ScrollSelectionIntoView(nsISelectionController::SELECTION_NORMAL,
                                 nsISelectionController::SELECTION_FOCUS_REGION,
                                 nsISelectionController::SCROLL_SYNCHRONOUS);
}



NS_IMETHODIMP 
PresShell::ScrollPage(bool aForward)
{
  nsIScrollableFrame* scrollFrame =
    GetFrameToScrollAsScrollable(nsIPresShell::eVertical);
  if (scrollFrame) {
    scrollFrame->ScrollBy(nsIntPoint(0, aForward ? 1 : -1),
                          nsIScrollableFrame::PAGES,
                          nsIScrollableFrame::SMOOTH);
  }
  return NS_OK;
}

NS_IMETHODIMP
PresShell::ScrollLine(bool aForward)
{
  nsIScrollableFrame* scrollFrame =
    GetFrameToScrollAsScrollable(nsIPresShell::eVertical);
  if (scrollFrame) {
    int32_t lineCount = Preferences::GetInt("toolkit.scrollbox.verticalScrollDistance",
                                            NS_DEFAULT_VERTICAL_SCROLL_DISTANCE);
    scrollFrame->ScrollBy(nsIntPoint(0, aForward ? lineCount : -lineCount),
                          nsIScrollableFrame::LINES,
                          nsIScrollableFrame::SMOOTH);
  }
  return NS_OK;
}

NS_IMETHODIMP
PresShell::ScrollCharacter(bool aRight)
{
  nsIScrollableFrame* scrollFrame =
    GetFrameToScrollAsScrollable(nsIPresShell::eHorizontal);
  if (scrollFrame) {
    int32_t h = Preferences::GetInt("toolkit.scrollbox.horizontalScrollDistance",
                                    NS_DEFAULT_HORIZONTAL_SCROLL_DISTANCE);
    scrollFrame->ScrollBy(nsIntPoint(aRight ? h : -h, 0),
                          nsIScrollableFrame::LINES,
                          nsIScrollableFrame::SMOOTH);
  }
  return NS_OK;
}

NS_IMETHODIMP
PresShell::CompleteScroll(bool aForward)
{
  nsIScrollableFrame* scrollFrame =
    GetFrameToScrollAsScrollable(nsIPresShell::eVertical);
  if (scrollFrame) {
    scrollFrame->ScrollBy(nsIntPoint(0, aForward ? 1 : -1),
                          nsIScrollableFrame::WHOLE,
                          nsIScrollableFrame::INSTANT);
  }
  return NS_OK;
}

NS_IMETHODIMP
PresShell::CompleteMove(bool aForward, bool aExtend)
{
  
  
  nsIContent* limiter = mSelection->GetAncestorLimiter();
  nsIFrame* frame = limiter ? limiter->GetPrimaryFrame()
                            : FrameConstructor()->GetRootElementFrame();
  if (!frame)
    return NS_ERROR_FAILURE;
  nsIFrame::CaretPosition pos =
    frame->GetExtremeCaretPosition(!aForward);
  mSelection->HandleClick(pos.mResultContent, pos.mContentOffset,
                          pos.mContentOffset, aExtend, false, aForward);
  if (limiter) {
    
    mSelection->SetAncestorLimiter(limiter);
  }
    
  
  
  return ScrollSelectionIntoView(nsISelectionController::SELECTION_NORMAL, 
                                 nsISelectionController::SELECTION_FOCUS_REGION,
                                 nsISelectionController::SCROLL_SYNCHRONOUS);
}

NS_IMETHODIMP 
PresShell::SelectAll()
{
  return mSelection->SelectAll();
}

static void
DoCheckVisibility(nsPresContext* aPresContext,
                  nsIContent* aNode,
                  int16_t aStartOffset,
                  int16_t aEndOffset,
                  bool* aRetval)
{
  nsIFrame* frame = aNode->GetPrimaryFrame();
  if (!frame) {
    
    return;
  }

  
  
  bool finished = false;
  frame->CheckVisibility(aPresContext, aStartOffset, aEndOffset, true,
                         &finished, aRetval);
  
}

NS_IMETHODIMP
PresShell::CheckVisibility(nsIDOMNode *node, int16_t startOffset, int16_t EndOffset, bool *_retval)
{
  if (!node || startOffset>EndOffset || !_retval || startOffset<0 || EndOffset<0)
    return NS_ERROR_INVALID_ARG;
  *_retval = false; 
  nsCOMPtr<nsIContent> content(do_QueryInterface(node));
  if (!content)
    return NS_ERROR_FAILURE;

  DoCheckVisibility(mPresContext, content, startOffset, EndOffset, _retval);
  return NS_OK;
}

nsresult
PresShell::CheckVisibilityContent(nsIContent* aNode, int16_t aStartOffset,
                                  int16_t aEndOffset, bool* aRetval)
{
  if (!aNode || aStartOffset > aEndOffset || !aRetval ||
      aStartOffset < 0 || aEndOffset < 0) {
    return NS_ERROR_INVALID_ARG;
  }

  *aRetval = false;
  DoCheckVisibility(mPresContext, aNode, aStartOffset, aEndOffset, aRetval);
  return NS_OK;
}




void
PresShell::StyleChangeReflow()
{
  nsIFrame* rootFrame = mFrameConstructor->GetRootFrame();
  
  
  if (!rootFrame)
    return;

  FrameNeedsReflow(rootFrame, eStyleChange, NS_FRAME_IS_DIRTY);
}

nsIFrame*
nsIPresShell::GetRootFrameExternal() const
{
  return mFrameConstructor->GetRootFrame();
}

nsIFrame*
nsIPresShell::GetRootScrollFrame() const
{
  nsIFrame* rootFrame = mFrameConstructor->GetRootFrame();
  
  if (!rootFrame || nsGkAtoms::viewportFrame != rootFrame->GetType())
    return nullptr;
  nsIFrame* theFrame = rootFrame->GetFirstPrincipalChild();
  if (!theFrame || nsGkAtoms::scrollFrame != theFrame->GetType())
    return nullptr;
  return theFrame;
}

nsIScrollableFrame*
nsIPresShell::GetRootScrollFrameAsScrollable() const
{
  nsIFrame* frame = GetRootScrollFrame();
  if (!frame)
    return nullptr;
  nsIScrollableFrame* scrollableFrame = do_QueryFrame(frame);
  NS_ASSERTION(scrollableFrame,
               "All scroll frames must implement nsIScrollableFrame");
  return scrollableFrame;
}

nsIScrollableFrame*
nsIPresShell::GetRootScrollFrameAsScrollableExternal() const
{
  return GetRootScrollFrameAsScrollable();
}

nsIPageSequenceFrame*
PresShell::GetPageSequenceFrame() const
{
  nsIFrame* frame = mFrameConstructor->GetPageSequenceFrame();
  return do_QueryFrame(frame);
}

void
PresShell::BeginUpdate(nsIDocument *aDocument, nsUpdateType aUpdateType)
{
#ifdef DEBUG
  mUpdateCount++;
#endif
  mFrameConstructor->BeginUpdate();

  if (aUpdateType & UPDATE_STYLE)
    mStyleSet->BeginUpdate();
}

void
PresShell::EndUpdate(nsIDocument *aDocument, nsUpdateType aUpdateType)
{
#ifdef DEBUG
  NS_PRECONDITION(0 != mUpdateCount, "too many EndUpdate's");
  --mUpdateCount;
#endif

  if (aUpdateType & UPDATE_STYLE) {
    mStyleSet->EndUpdate();
    if (mStylesHaveChanged || !mChangedScopeStyleRoots.IsEmpty())
      ReconstructStyleData();
  }

  mFrameConstructor->EndUpdate();
}

void
PresShell::RestoreRootScrollPosition()
{
  nsIScrollableFrame* scrollableFrame = GetRootScrollFrameAsScrollable();
  if (scrollableFrame) {
    scrollableFrame->ScrollToRestoredPosition();
  }
}

void
PresShell::BeginLoad(nsIDocument *aDocument)
{  
  mDocumentLoading = true;
}

void
PresShell::EndLoad(nsIDocument *aDocument)
{
  NS_PRECONDITION(aDocument == mDocument, "Wrong document");
  
  RestoreRootScrollPosition();
  
  mDocumentLoading = false;
}

#ifdef DEBUG
void
PresShell::VerifyHasDirtyRootAncestor(nsIFrame* aFrame)
{
  
  return;
  
  
  
  if (!aFrame->GetParent()) {
    return;
  }
        
  
  
  while (aFrame && (aFrame->GetStateBits() & NS_FRAME_HAS_DIRTY_CHILDREN)) {
    if (((aFrame->GetStateBits() & NS_FRAME_REFLOW_ROOT) ||
         !aFrame->GetParent()) &&
        mDirtyRoots.Contains(aFrame)) {
      return;
    }

    aFrame = aFrame->GetParent();
  }
  NS_NOTREACHED("Frame has dirty bits set but isn't scheduled to be "
                "reflowed?");
}
#endif

void
PresShell::FrameNeedsReflow(nsIFrame *aFrame, IntrinsicDirty aIntrinsicDirty,
                            nsFrameState aBitToAdd)
{
  NS_PRECONDITION(aBitToAdd == NS_FRAME_IS_DIRTY ||
                  aBitToAdd == NS_FRAME_HAS_DIRTY_CHILDREN,
                  "Unexpected bits being added");
  NS_PRECONDITION(aIntrinsicDirty != eStyleChange ||
                  aBitToAdd == NS_FRAME_IS_DIRTY,
                  "bits don't correspond to style change reason");

  NS_ASSERTION(!mIsReflowing, "can't mark frame dirty during reflow");

  
  
  if (! mDidInitialize)
    return;

  
  if (mIsDestroying)
    return;

#ifdef DEBUG
  
  if (mInVerifyReflow)
    return;

  if (VERIFY_REFLOW_NOISY_RC & gVerifyReflowFlags) {
    printf("\nPresShell@%p: frame %p needs reflow\n", (void*)this, (void*)aFrame);
    if (VERIFY_REFLOW_REALLY_NOISY_RC & gVerifyReflowFlags) {
      printf("Current content model:\n");
      Element *rootElement = mDocument->GetRootElement();
      if (rootElement) {
        rootElement->List(stdout, 0);
      }
    }
  }  
#endif

  nsAutoTArray<nsIFrame*, 4> subtrees;
  subtrees.AppendElement(aFrame);

  do {
    nsIFrame *subtreeRoot = subtrees.ElementAt(subtrees.Length() - 1);
    subtrees.RemoveElementAt(subtrees.Length() - 1);

    
    
    bool wasDirty = NS_SUBTREE_DIRTY(subtreeRoot);
    subtreeRoot->AddStateBits(aBitToAdd);

    
    
    bool targetFrameDirty = (aBitToAdd == NS_FRAME_IS_DIRTY);

#define FRAME_IS_REFLOW_ROOT(_f)                   \
  ((_f->GetStateBits() & NS_FRAME_REFLOW_ROOT) &&  \
   (_f != subtreeRoot || !targetFrameDirty))


    
    

    if (aIntrinsicDirty != eResize) {
      
      
      
      
      for (nsIFrame *a = subtreeRoot;
           a && !FRAME_IS_REFLOW_ROOT(a);
           a = a->GetParent())
        a->MarkIntrinsicWidthsDirty();
    }

    if (aIntrinsicDirty == eStyleChange) {
      
      
      
      
      nsAutoTArray<nsIFrame*, 32> stack;
      stack.AppendElement(subtreeRoot);

      do {
        nsIFrame *f = stack.ElementAt(stack.Length() - 1);
        stack.RemoveElementAt(stack.Length() - 1);

        if (f->GetType() == nsGkAtoms::placeholderFrame) {
          nsIFrame *oof = nsPlaceholderFrame::GetRealFrameForPlaceholder(f);
          if (!nsLayoutUtils::IsProperAncestorFrame(subtreeRoot, oof)) {
            
            subtrees.AppendElement(oof);
          }
        }

        nsIFrame::ChildListIterator lists(f);
        for (; !lists.IsDone(); lists.Next()) {
          nsFrameList::Enumerator childFrames(lists.CurrentList());
          for (; !childFrames.AtEnd(); childFrames.Next()) {
            nsIFrame* kid = childFrames.get();
            kid->MarkIntrinsicWidthsDirty();
            stack.AppendElement(kid);
          }
        }
      } while (stack.Length() != 0);
    }

    
    
    
    nsIFrame *f = subtreeRoot;
    for (;;) {
      if (FRAME_IS_REFLOW_ROOT(f) || !f->GetParent()) {
        
        if (!wasDirty) {
          mDirtyRoots.AppendElement(f);
          mDocument->SetNeedLayoutFlush();
        }
#ifdef DEBUG
        else {
          VerifyHasDirtyRootAncestor(f);
        }
#endif
        
        break;
      }

      nsIFrame *child = f;
      f = f->GetParent();
      wasDirty = NS_SUBTREE_DIRTY(f);
      f->ChildIsDirty(child);
      NS_ASSERTION(f->GetStateBits() & NS_FRAME_HAS_DIRTY_CHILDREN,
                   "ChildIsDirty didn't do its job");
      if (wasDirty) {
        
#ifdef DEBUG
        VerifyHasDirtyRootAncestor(f);
#endif
        break;
      }
    }
  } while (subtrees.Length() != 0);

  MaybeScheduleReflow();
}

void
PresShell::FrameNeedsToContinueReflow(nsIFrame *aFrame)
{
  NS_ASSERTION(mIsReflowing, "Must be in reflow when marking path dirty.");  
  NS_PRECONDITION(mCurrentReflowRoot, "Must have a current reflow root here");
  NS_ASSERTION(aFrame == mCurrentReflowRoot ||
               nsLayoutUtils::IsProperAncestorFrame(mCurrentReflowRoot, aFrame),
               "Frame passed in is not the descendant of mCurrentReflowRoot");
  NS_ASSERTION(aFrame->GetStateBits() & NS_FRAME_IN_REFLOW,
               "Frame passed in not in reflow?");

  mFramesToDirty.PutEntry(aFrame);
}

nsIScrollableFrame*
nsIPresShell::GetFrameToScrollAsScrollable(
                nsIPresShell::ScrollDirection aDirection)
{
  nsIScrollableFrame* scrollFrame = nullptr;

  nsCOMPtr<nsIContent> focusedContent;
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (fm && mDocument) {
    nsCOMPtr<nsIDOMWindow> window = do_QueryInterface(mDocument->GetWindow());

    nsCOMPtr<nsIDOMElement> focusedElement;
    fm->GetFocusedElementForWindow(window, false, nullptr, getter_AddRefs(focusedElement));
    focusedContent = do_QueryInterface(focusedElement);
  }
  if (!focusedContent && mSelection) {
    nsISelection* domSelection = mSelection->
      GetSelection(nsISelectionController::SELECTION_NORMAL);
    if (domSelection) {
      nsCOMPtr<nsIDOMNode> focusedNode;
      domSelection->GetFocusNode(getter_AddRefs(focusedNode));
      focusedContent = do_QueryInterface(focusedNode);
    }
  }
  if (focusedContent) {
    nsIFrame* startFrame = focusedContent->GetPrimaryFrame();
    if (startFrame) {
      scrollFrame = startFrame->GetScrollTargetFrame();
      if (scrollFrame) {
        startFrame = scrollFrame->GetScrolledFrame();
      }
      if (aDirection == nsIPresShell::eEither) {
        scrollFrame =
          nsLayoutUtils::GetNearestScrollableFrame(startFrame);
      } else {
        scrollFrame =
          nsLayoutUtils::GetNearestScrollableFrameForDirection(startFrame,
            aDirection == eVertical ? nsLayoutUtils::eVertical :
                                      nsLayoutUtils::eHorizontal);
      }
    }
  }
  if (!scrollFrame) {
    scrollFrame = GetRootScrollFrameAsScrollable();
  }
  return scrollFrame;
}

void
PresShell::CancelAllPendingReflows()
{
  mDirtyRoots.Clear();

  if (mReflowScheduled) {
    GetPresContext()->RefreshDriver()->RemoveLayoutFlushObserver(this);
    mReflowScheduled = false;
  }

  ASSERT_REFLOW_SCHEDULED_STATE();
}

nsresult
PresShell::RecreateFramesFor(nsIContent* aContent)
{
  NS_ENSURE_TRUE(mPresContext, NS_ERROR_FAILURE);
  if (!mDidInitialize) {
    
    
    return NS_OK;
  }

  
  

  NS_ASSERTION(mViewManager, "Should have view manager");

  
  
  mDocument->FlushPendingNotifications(Flush_ContentAndNotify);

  nsAutoScriptBlocker scriptBlocker;

  nsStyleChangeList changeList;
  changeList.AppendChange(nullptr, aContent, nsChangeHint_ReconstructFrame);

  
  ++mChangeNestCount;
  css::OverflowChangedTracker tracker;
  nsresult rv = mFrameConstructor->ProcessRestyledFrames(changeList, tracker);
  tracker.Flush();
  --mChangeNestCount;
  
  return rv;
}

void
nsIPresShell::PostRecreateFramesFor(Element* aElement)
{
  FrameConstructor()->PostRestyleEvent(aElement, nsRestyleHint(0),
                                       nsChangeHint_ReconstructFrame);
}

void
nsIPresShell::RestyleForAnimation(Element* aElement, nsRestyleHint aHint)
{
  FrameConstructor()->PostAnimationRestyleEvent(aElement, aHint,
                                                NS_STYLE_HINT_NONE);
}

void
PresShell::ClearFrameRefs(nsIFrame* aFrame)
{
  mPresContext->EventStateManager()->ClearFrameRefs(aFrame);

  nsWeakFrame* weakFrame = mWeakFrames;
  while (weakFrame) {
    nsWeakFrame* prev = weakFrame->GetPreviousWeakFrame();
    if (weakFrame->GetFrame() == aFrame) {
      
      weakFrame->Clear(this);
    }
    weakFrame = prev;
  }
}

already_AddRefed<nsRenderingContext>
PresShell::GetReferenceRenderingContext()
{
  nsDeviceContext* devCtx = mPresContext->DeviceContext();
  nsRefPtr<nsRenderingContext> rc;
  if (mPresContext->IsScreen()) {
    rc = new nsRenderingContext();
    rc->Init(devCtx, gfxPlatform::GetPlatform()->ScreenReferenceSurface());
  } else {
    devCtx->CreateRenderingContext(*getter_AddRefs(rc));
  }
  return rc.forget();
}

nsresult
PresShell::GoToAnchor(const nsAString& aAnchorName, bool aScroll)
{
  if (!mDocument) {
    return NS_ERROR_FAILURE;
  }
  
  const Element *root = mDocument->GetRootElement();
  if (root && root->IsSVG(nsGkAtoms::svg)) {
    
    
    if (SVGFragmentIdentifier::ProcessFragmentIdentifier(mDocument, aAnchorName)) {
      return NS_OK;
    }
  }

  
  nsRefPtr<nsEventStateManager> esm = mPresContext->EventStateManager();

  if (aAnchorName.IsEmpty()) {
    NS_ASSERTION(!aScroll, "can't scroll to empty anchor name");
    esm->SetContentState(nullptr, NS_EVENT_STATE_URLTARGET);
    return NS_OK;
  }

  nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(mDocument);
  nsresult rv = NS_OK;
  nsCOMPtr<nsIContent> content;

  
  if (mDocument) {    
    content = mDocument->GetElementById(aAnchorName);
  }

  
  if (!content && htmlDoc) {
    nsCOMPtr<nsIDOMNodeList> list;
    
    rv = htmlDoc->GetElementsByName(aAnchorName, getter_AddRefs(list));
    if (NS_SUCCEEDED(rv) && list) {
      uint32_t i;
      
      for (i = 0; true; i++) {
        nsCOMPtr<nsIDOMNode> node;
        rv = list->Item(i, getter_AddRefs(node));
        if (!node) {  
          break;
        }
        
        content = do_QueryInterface(node);
        if (content) {
          if (content->Tag() == nsGkAtoms::a && content->IsHTML()) {
            break;
          }
          content = nullptr;
        }
      }
    }
  }

  
  if (!content && !htmlDoc)
  {
    nsCOMPtr<nsIDOMDocument> doc = do_QueryInterface(mDocument);
    nsCOMPtr<nsIDOMNodeList> list;
    NS_NAMED_LITERAL_STRING(nameSpace, "http://www.w3.org/1999/xhtml");
    
    rv = doc->GetElementsByTagNameNS(nameSpace, NS_LITERAL_STRING("a"), getter_AddRefs(list));
    if (NS_SUCCEEDED(rv) && list) {
      uint32_t i;
      
      for (i = 0; true; i++) {
        nsCOMPtr<nsIDOMNode> node;
        rv = list->Item(i, getter_AddRefs(node));
        if (!node) { 
          break;
        }
        
        nsCOMPtr<nsIDOMElement> element = do_QueryInterface(node);
        nsAutoString value;
        if (element && NS_SUCCEEDED(element->GetAttribute(NS_LITERAL_STRING("name"), value))) {
          if (value.Equals(aAnchorName)) {
            content = do_QueryInterface(element);
            break;
          }
        }
      }
    }
  }

  esm->SetContentState(content, NS_EVENT_STATE_URLTARGET);

#ifdef ACCESSIBILITY
  nsIContent *anchorTarget = content;
#endif

  if (content) {
    if (aScroll) {
      rv = ScrollContentIntoView(content,
                                 ScrollAxis(SCROLL_TOP, SCROLL_ALWAYS),
                                 ScrollAxis(),
                                 ANCHOR_SCROLL_FLAGS);
      NS_ENSURE_SUCCESS(rv, rv);

      nsIScrollableFrame* rootScroll = GetRootScrollFrameAsScrollable();
      if (rootScroll) {
        mLastAnchorScrolledTo = content;
        mLastAnchorScrollPositionY = rootScroll->GetScrollPosition().y;
      }
    }

    
    
    bool selectAnchor = Preferences::GetBool("layout.selectanchor");

    
    
    
    nsRefPtr<nsIDOMRange> jumpToRange = new nsRange();
    while (content && content->GetFirstChild()) {
      content = content->GetFirstChild();
    }
    nsCOMPtr<nsIDOMNode> node(do_QueryInterface(content));
    NS_ASSERTION(node, "No nsIDOMNode for descendant of anchor");
    jumpToRange->SelectNodeContents(node);
    
    nsISelection* sel = mSelection->
      GetSelection(nsISelectionController::SELECTION_NORMAL);
    if (sel) {
      sel->RemoveAllRanges();
      sel->AddRange(jumpToRange);
      if (!selectAnchor) {
        
        sel->CollapseToStart();
      }
    }
    
    
    nsPIDOMWindow *win = mDocument->GetWindow();

    nsIFocusManager* fm = nsFocusManager::GetFocusManager();
    if (fm && win) {
      nsCOMPtr<nsIDOMWindow> focusedWindow;
      fm->GetFocusedWindow(getter_AddRefs(focusedWindow));
      if (SameCOMIdentity(win, focusedWindow)) {
        fm->ClearFocus(focusedWindow);
      }
    }

    
    if (content->IsNodeOfType(nsINode::eANIMATION)) {
      SVGContentUtils::ActivateByHyperlink(content.get());
    }
  } else {
    rv = NS_ERROR_FAILURE;
    NS_NAMED_LITERAL_STRING(top, "top");
    if (nsContentUtils::EqualsIgnoreASCIICase(aAnchorName, top)) {
      
      
      rv = NS_OK;
      nsIScrollableFrame* sf = GetRootScrollFrameAsScrollable();
      
      
      if (aScroll && sf) {
        
        sf->ScrollTo(nsPoint(0, 0), nsIScrollableFrame::INSTANT);
      }
    }
  }

#ifdef ACCESSIBILITY
  if (anchorTarget) {
    nsAccessibilityService* accService = AccService();
    if (accService)
      accService->NotifyOfAnchorJumpTo(anchorTarget);
  }
#endif

  return rv;
}

nsresult
PresShell::ScrollToAnchor()
{
  if (!mLastAnchorScrolledTo)
    return NS_OK;

  NS_ASSERTION(mDidInitialize, "should have done initial reflow by now");

  nsIScrollableFrame* rootScroll = GetRootScrollFrameAsScrollable();
  if (!rootScroll ||
      mLastAnchorScrollPositionY != rootScroll->GetScrollPosition().y)
    return NS_OK;

  nsresult rv = ScrollContentIntoView(mLastAnchorScrolledTo,
                                      ScrollAxis(SCROLL_TOP, SCROLL_ALWAYS),
                                      ScrollAxis(),
                                      ANCHOR_SCROLL_FLAGS);
  mLastAnchorScrolledTo = nullptr;
  return rv;
}














static void
AccumulateFrameBounds(nsIFrame* aContainerFrame,
                      nsIFrame* aFrame,
                      bool aUseWholeLineHeightForInlines,
                      nsRect& aRect,
                      bool& aHaveRect,
                      nsIFrame*& aPrevBlock,
                      nsAutoLineIterator& aLines,
                      int32_t& aCurLine)
{
  nsIFrame* frame = aFrame;
  nsRect frameBounds = nsRect(nsPoint(0, 0), aFrame->GetSize());

  
  
  
  if (frameBounds.height == 0 || aUseWholeLineHeightForInlines) {
    nsIFrame *prevFrame = aFrame;
    nsIFrame *f = aFrame;

    while (f && f->IsFrameOfType(nsIFrame::eLineParticipant) &&
           !f->IsTransformed() && !f->IsPositioned()) {
      prevFrame = f;
      f = prevFrame->GetParent();
    }

    if (f != aFrame &&
        f &&
        f->GetType() == nsGkAtoms::blockFrame) {
      
      if (f != aPrevBlock) {
        aLines = f->GetLineIterator();
        aPrevBlock = f;
        aCurLine = 0;
      }
      if (aLines) {
        int32_t index = aLines->FindLineContaining(prevFrame, aCurLine);
        if (index >= 0) {
          aCurLine = index;
          nsIFrame *trash1;
          int32_t trash2;
          nsRect lineBounds;
          uint32_t trash3;

          if (NS_SUCCEEDED(aLines->GetLine(index, &trash1, &trash2,
                                           lineBounds, &trash3))) {
            frameBounds += frame->GetOffsetTo(f);
            frame = f;
            if (lineBounds.y < frameBounds.y) {
              frameBounds.height = frameBounds.YMost() - lineBounds.y;
              frameBounds.y = lineBounds.y;
            }
          }
        }
      }
    }
  }

  nsRect transformedBounds = nsLayoutUtils::TransformFrameRectToAncestor(frame,
    frameBounds, aContainerFrame);

  if (aHaveRect) {
    
    
    
    aRect.UnionRectEdges(aRect, transformedBounds);
  } else {
    aHaveRect = true;
    aRect = transformedBounds;
  }
}

static bool
ComputeNeedToScroll(nsIPresShell::WhenToScroll aWhenToScroll,
                    nscoord                    aLineSize,
                    nscoord                    aRectMin,
                    nscoord                    aRectMax,
                    nscoord                    aViewMin,
                    nscoord                    aViewMax) {
  
  if (nsIPresShell::SCROLL_ALWAYS == aWhenToScroll) {
    
    return true;
  } else if (nsIPresShell::SCROLL_IF_NOT_VISIBLE == aWhenToScroll) {
    
    return aRectMax - aLineSize <= aViewMin ||
           aRectMin + aLineSize >= aViewMax;
  } else if (nsIPresShell::SCROLL_IF_NOT_FULLY_VISIBLE == aWhenToScroll) {
    
    return !(aRectMin >= aViewMin && aRectMax <= aViewMax) &&
      std::min(aViewMax, aRectMax) - std::max(aRectMin, aViewMin) < aViewMax - aViewMin;
  }
  return false;
}

static nscoord
ComputeWhereToScroll(int16_t aWhereToScroll,
                     nscoord aOriginalCoord,
                     nscoord aRectMin,
                     nscoord aRectMax,
                     nscoord aViewMin,
                     nscoord aViewMax,
                     nscoord* aRangeMin,
                     nscoord* aRangeMax) {
  nscoord resultCoord = aOriginalCoord;
  
  
  if (nsIPresShell::SCROLL_MINIMUM == aWhereToScroll) {
    if (aRectMin < aViewMin) {
      
      resultCoord = aRectMin;
    } else if (aRectMax > aViewMax) {
      
      
      resultCoord = aOriginalCoord + aRectMax - aViewMax;
      if (resultCoord > aRectMin) {
        resultCoord = aRectMin;
      }
    }
  } else {
    nscoord frameAlignCoord =
      NSToCoordRound(aRectMin + (aRectMax - aRectMin) * (aWhereToScroll / 100.0f));
    resultCoord =  NSToCoordRound(frameAlignCoord - (aViewMax - aViewMin) * (
                                  aWhereToScroll / 100.0f));
  }
  nscoord scrollPortLength = aViewMax - aViewMin;
  
  *aRangeMin = std::min(resultCoord, aRectMax - scrollPortLength);
  *aRangeMax = std::max(resultCoord, aRectMin);
  return resultCoord;
}









static void ScrollToShowRect(nsIScrollableFrame*      aScrollFrame,
                             const nsRect&            aRect,
                             nsIPresShell::ScrollAxis aVertical,
                             nsIPresShell::ScrollAxis aHorizontal,
                             uint32_t                 aFlags)
{
  nsPoint scrollPt = aScrollFrame->GetScrollPosition();
  nsRect visibleRect(scrollPt,
                     aScrollFrame->GetScrollPositionClampingScrollPortSize());

  nsSize lineSize;
  
  
  
  
  
  if (aVertical.mWhenToScroll == nsIPresShell::SCROLL_IF_NOT_VISIBLE ||
      aHorizontal.mWhenToScroll == nsIPresShell::SCROLL_IF_NOT_VISIBLE) {
    lineSize = aScrollFrame->GetLineScrollAmount();
  }
  nsPresContext::ScrollbarStyles ss = aScrollFrame->GetScrollbarStyles();
  nsRect allowedRange(scrollPt, nsSize(0, 0));
  bool needToScroll = false;
  uint32_t directions = aScrollFrame->GetPerceivedScrollingDirections();

  if (((aFlags & nsIPresShell::SCROLL_OVERFLOW_HIDDEN) ||
       ss.mVertical != NS_STYLE_OVERFLOW_HIDDEN) &&
      (!aVertical.mOnlyIfPerceivedScrollableDirection ||
       (directions & nsIScrollableFrame::VERTICAL))) {

    if (ComputeNeedToScroll(aVertical.mWhenToScroll,
                            lineSize.height,
                            aRect.y,
                            aRect.YMost(),
                            visibleRect.y,
                            visibleRect.YMost())) {
      nscoord maxHeight;
      scrollPt.y = ComputeWhereToScroll(aVertical.mWhereToScroll,
                                        scrollPt.y,
                                        aRect.y,
                                        aRect.YMost(),
                                        visibleRect.y,
                                        visibleRect.YMost(),
                                        &allowedRange.y, &maxHeight);
      allowedRange.height = maxHeight - allowedRange.y;
      needToScroll = true;
    }
  }

  if (((aFlags & nsIPresShell::SCROLL_OVERFLOW_HIDDEN) ||
       ss.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN) &&
      (!aHorizontal.mOnlyIfPerceivedScrollableDirection ||
       (directions & nsIScrollableFrame::HORIZONTAL))) {

    if (ComputeNeedToScroll(aHorizontal.mWhenToScroll,
                            lineSize.width,
                            aRect.x,
                            aRect.XMost(),
                            visibleRect.x,
                            visibleRect.XMost())) {
      nscoord maxWidth;
      scrollPt.x = ComputeWhereToScroll(aHorizontal.mWhereToScroll,
                                        scrollPt.x,
                                        aRect.x,
                                        aRect.XMost(),
                                        visibleRect.x,
                                        visibleRect.XMost(),
                                        &allowedRange.x, &maxWidth);
      allowedRange.width = maxWidth - allowedRange.x;
      needToScroll = true;
    }
  }

  
  
  if (needToScroll) {
    aScrollFrame->ScrollTo(scrollPt, nsIScrollableFrame::INSTANT, &allowedRange);
  }
}

static void
DeleteScrollIntoViewData(void* aObject, nsIAtom* aPropertyName,
                         void* aPropertyValue, void* aData)
{
  PresShell::ScrollIntoViewData* data =
    static_cast<PresShell::ScrollIntoViewData*>(aPropertyValue);
  delete data;
}

nsresult
PresShell::ScrollContentIntoView(nsIContent*              aContent,
                                 nsIPresShell::ScrollAxis aVertical,
                                 nsIPresShell::ScrollAxis aHorizontal,
                                 uint32_t                 aFlags)
{
  NS_ENSURE_TRUE(aContent, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDocument> currentDoc = aContent->GetCurrentDoc();
  NS_ENSURE_STATE(currentDoc);

  NS_ASSERTION(mDidInitialize, "should have done initial reflow by now");

  if (mContentToScrollTo) {
    mContentToScrollTo->DeleteProperty(nsGkAtoms::scrolling);
  }
  mContentToScrollTo = aContent;
  ScrollIntoViewData* data = new ScrollIntoViewData();
  data->mContentScrollVAxis = aVertical;
  data->mContentScrollHAxis = aHorizontal;
  data->mContentToScrollToFlags = aFlags;
  if (NS_FAILED(mContentToScrollTo->SetProperty(nsGkAtoms::scrolling, data,
                                                ::DeleteScrollIntoViewData))) {
    mContentToScrollTo = nullptr;
  }

  
  currentDoc->SetNeedLayoutFlush();
  currentDoc->FlushPendingNotifications(Flush_InterruptibleLayout);

  
  
  
  
  
  
  
  
  if (mContentToScrollTo) {
    DoScrollContentIntoView();
  }
  return NS_OK;
}

void
PresShell::DoScrollContentIntoView()
{
  NS_ASSERTION(mDidInitialize, "should have done initial reflow by now");

  nsIFrame* frame = mContentToScrollTo->GetPrimaryFrame();
  if (!frame) {
    mContentToScrollTo->DeleteProperty(nsGkAtoms::scrolling);
    mContentToScrollTo = nullptr;
    return;
  }

  if (frame->GetStateBits() & NS_FRAME_FIRST_REFLOW) {
    
    
    
    return;
  }

  
  
  nsIFrame* container =
    nsLayoutUtils::GetClosestFrameOfType(frame->GetParent(), nsGkAtoms::scrollFrame);
  if (!container) {
    
    return;
  }

  ScrollIntoViewData* data = static_cast<ScrollIntoViewData*>(
    mContentToScrollTo->GetProperty(nsGkAtoms::scrolling));
  if (MOZ_UNLIKELY(!data)) {
    mContentToScrollTo = nullptr;
    return;
  }

  
  
  
  
  
  
  
  
  
  
  nsRect frameBounds;
  bool haveRect = false;
  bool useWholeLineHeightForInlines =
    data->mContentScrollVAxis.mWhenToScroll != nsIPresShell::SCROLL_IF_NOT_FULLY_VISIBLE;
  
  
  nsIFrame* prevBlock = nullptr;
  nsAutoLineIterator lines;
  
  
  int32_t curLine = 0;
  do {
    AccumulateFrameBounds(container, frame, useWholeLineHeightForInlines,
                          frameBounds, haveRect, prevBlock, lines, curLine);
  } while ((frame = frame->GetNextContinuation()));

  ScrollFrameRectIntoView(container, frameBounds, data->mContentScrollVAxis,
                          data->mContentScrollHAxis,
                          data->mContentToScrollToFlags);
}

bool
PresShell::ScrollFrameRectIntoView(nsIFrame*                aFrame,
                                   const nsRect&            aRect,
                                   nsIPresShell::ScrollAxis aVertical,
                                   nsIPresShell::ScrollAxis aHorizontal,
                                   uint32_t                 aFlags)
{
  bool didScroll = false;
  
  nsRect rect = aRect;
  nsIFrame* container = aFrame;
  
  
  do {
    nsIScrollableFrame* sf = do_QueryFrame(container);
    if (sf) {
      nsPoint oldPosition = sf->GetScrollPosition();
      ScrollToShowRect(sf, rect - sf->GetScrolledFrame()->GetPosition(),
                       aVertical, aHorizontal, aFlags);
      nsPoint newPosition = sf->GetScrollPosition();
      
      
      rect += oldPosition - newPosition;

      if (oldPosition != newPosition) {
        didScroll = true;
      }

      
      if (aFlags & nsIPresShell::SCROLL_FIRST_ANCESTOR_ONLY) {
        break;
      }
    }
    nsIFrame* parent;
    if (container->IsTransformed()) {
      container->GetTransformMatrix(nullptr, &parent);
      rect = nsLayoutUtils::TransformFrameRectToAncestor(container, rect, parent);
    } else {
      rect += container->GetPosition();
      parent = container->GetParent();
    }
    if (!parent && !(aFlags & nsIPresShell::SCROLL_NO_PARENT_FRAMES)) {
      nsPoint extraOffset(0,0);
      parent = nsLayoutUtils::GetCrossDocParentFrame(container, &extraOffset);
      if (parent) {
        int32_t APD = container->PresContext()->AppUnitsPerDevPixel();        
        int32_t parentAPD = parent->PresContext()->AppUnitsPerDevPixel();
        rect = rect.ConvertAppUnitsRoundOut(APD, parentAPD);
        rect += extraOffset;
      }
    }
    container = parent;
  } while (container);

  return didScroll;
}

nsRectVisibility
PresShell::GetRectVisibility(nsIFrame* aFrame,
                             const nsRect &aRect,
                             nscoord aMinTwips) const
{
  NS_ASSERTION(aFrame->PresContext() == GetPresContext(),
               "prescontext mismatch?");
  nsIFrame* rootFrame = mFrameConstructor->GetRootFrame();
  NS_ASSERTION(rootFrame,
               "How can someone have a frame for this presshell when there's no root?");
  nsIScrollableFrame* sf = GetRootScrollFrameAsScrollable();
  nsRect scrollPortRect;
  if (sf) {
    scrollPortRect = sf->GetScrollPortRect();
    nsIFrame* f = do_QueryFrame(sf);
    scrollPortRect += f->GetOffsetTo(rootFrame);
  } else {
    scrollPortRect = nsRect(nsPoint(0,0), rootFrame->GetSize());
  }

  nsRect r = aRect + aFrame->GetOffsetTo(rootFrame);
  
  
  if (scrollPortRect.Contains(r))
    return nsRectVisibility_kVisible;

  nsRect insetRect = scrollPortRect;
  insetRect.Deflate(aMinTwips, aMinTwips);
  if (r.YMost() <= insetRect.y)
    return nsRectVisibility_kAboveViewport;
  if (r.y >= insetRect.YMost())
    return nsRectVisibility_kBelowViewport;
  if (r.XMost() <= insetRect.x)
    return nsRectVisibility_kLeftOfViewport;
  if (r.x >= insetRect.XMost())
    return nsRectVisibility_kRightOfViewport;

  return nsRectVisibility_kVisible;
}

void
PresShell::ScheduleViewManagerFlush()
{
  nsPresContext* presContext = GetPresContext();
  if (presContext) {
    presContext->RefreshDriver()->ScheduleViewManagerFlush();
  }
  if (mDocument) {
    mDocument->SetNeedLayoutFlush();
  }
}

void
PresShell::DispatchSynthMouseMove(nsGUIEvent *aEvent,
                                  bool aFlushOnHoverChange)
{
  uint32_t hoverGenerationBefore = mFrameConstructor->GetHoverGeneration();
  nsEventStatus status;
  nsView* targetView = nsView::GetViewFor(aEvent->widget);
  if (!targetView)
    return;
  targetView->GetViewManager()->DispatchEvent(aEvent, targetView, &status);
  if (aFlushOnHoverChange &&
      hoverGenerationBefore != mFrameConstructor->GetHoverGeneration()) {
    
    
    FlushPendingNotifications(Flush_Layout);
  }
}

void
PresShell::ClearMouseCaptureOnView(nsView* aView)
{
  if (gCaptureInfo.mContent) {
    if (aView) {
      
      
      nsIFrame* frame = gCaptureInfo.mContent->GetPrimaryFrame();
      if (frame) {
        nsView* view = frame->GetClosestView();
        
        
        if (view) {
          do {
            if (view == aView) {
              NS_RELEASE(gCaptureInfo.mContent);
              
              
              gCaptureInfo.mAllowed = false;
              break;
            }

            view = view->GetParent();
          } while (view);
          
          return;
        }
      }
    }

    NS_RELEASE(gCaptureInfo.mContent);
  }

  
  
  
  gCaptureInfo.mAllowed = false;
}

void
nsIPresShell::ClearMouseCapture(nsIFrame* aFrame)
{
  if (!gCaptureInfo.mContent) {
    gCaptureInfo.mAllowed = false;
    return;
  }

  
  if (!aFrame) {
    NS_RELEASE(gCaptureInfo.mContent);
    gCaptureInfo.mAllowed = false;
    return;
  }

  nsIFrame* capturingFrame = gCaptureInfo.mContent->GetPrimaryFrame();
  if (!capturingFrame) {
    NS_RELEASE(gCaptureInfo.mContent);
    gCaptureInfo.mAllowed = false;
    return;
  }

  if (nsLayoutUtils::IsAncestorFrameCrossDoc(aFrame, capturingFrame)) {
    NS_RELEASE(gCaptureInfo.mContent);
    gCaptureInfo.mAllowed = false;
  }
}

nsresult
PresShell::CaptureHistoryState(nsILayoutHistoryState** aState)
{
  nsresult rv = NS_OK;

  NS_PRECONDITION(nullptr != aState, "null state pointer");

  
  
  
  
  
  
  nsCOMPtr<nsISupports> container = mPresContext->GetContainer();
  if (!container)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(container));
  if (!docShell)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsILayoutHistoryState> historyState;
  docShell->GetLayoutHistoryState(getter_AddRefs(historyState));
  if (!historyState) {
    
    rv = NS_NewLayoutHistoryState(getter_AddRefs(historyState));
  
    if (NS_FAILED(rv)) { 
      *aState = nullptr;
      return rv;
    }    

    docShell->SetLayoutHistoryState(historyState);
  }

  *aState = historyState;
  NS_IF_ADDREF(*aState);
  
  
  nsIFrame* rootFrame = mFrameConstructor->GetRootFrame();
  if (!rootFrame) return NS_OK;

  mFrameConstructor->CaptureFrameState(rootFrame, historyState);  
 
  return NS_OK;
}

void
PresShell::UnsuppressAndInvalidate()
{
  
  
  if ((!mDocument->IsResourceDoc() && !mPresContext->EnsureVisible()) ||
      mHaveShutDown) {
    
    return;
  }

  if (!mDocument->IsResourceDoc()) {
    
    
    
    nsContentUtils::AddScriptRunner(new nsBeforeFirstPaintDispatcher(mDocument));
  }

  mPaintingSuppressed = false;
  nsIFrame* rootFrame = mFrameConstructor->GetRootFrame();
  if (rootFrame) {
    
    rootFrame->InvalidateFrame();

    if (mCaretEnabled && mCaret) {
      mCaret->CheckCaretDrawingState();
    }
  }

  
  nsPIDOMWindow *win = mDocument->GetWindow();
  if (win)
    win->SetReadyForFocus();

  if (!mHaveShutDown)
    SynthesizeMouseMove(false);
}

void
PresShell::UnsuppressPainting()
{
  if (mPaintSuppressionTimer) {
    mPaintSuppressionTimer->Cancel();
    mPaintSuppressionTimer = nullptr;
  }

  if (mIsDocumentGone || !mPaintingSuppressed)
    return;

  
  
  
  
  if (!mDirtyRoots.IsEmpty())
    mShouldUnsuppressPainting = true;
  else
    UnsuppressAndInvalidate();
}


nsresult
PresShell::PostReflowCallback(nsIReflowCallback* aCallback)
{
  void* result = AllocateMisc(sizeof(nsCallbackEventRequest));
  nsCallbackEventRequest* request = (nsCallbackEventRequest*)result;

  request->callback = aCallback;
  request->next = nullptr;

  if (mLastCallbackEventRequest) {
    mLastCallbackEventRequest = mLastCallbackEventRequest->next = request;
  } else {
    mFirstCallbackEventRequest = request;
    mLastCallbackEventRequest = request;
  }
 
  return NS_OK;
}

void
PresShell::CancelReflowCallback(nsIReflowCallback* aCallback)
{
   nsCallbackEventRequest* before = nullptr;
   nsCallbackEventRequest* node = mFirstCallbackEventRequest;
   while(node)
   {
      nsIReflowCallback* callback = node->callback;

      if (callback == aCallback) 
      {
        nsCallbackEventRequest* toFree = node;
        if (node == mFirstCallbackEventRequest) {
          node = node->next;
          mFirstCallbackEventRequest = node;
          NS_ASSERTION(before == nullptr, "impossible");
        } else {
          node = node->next;
          before->next = node;
        }

        if (toFree == mLastCallbackEventRequest) {
          mLastCallbackEventRequest = before;
        }

        FreeMisc(sizeof(nsCallbackEventRequest), toFree);
      } else {
        before = node;
        node = node->next;
      }
   }
}

void
PresShell::CancelPostedReflowCallbacks()
{
  while (mFirstCallbackEventRequest) {
    nsCallbackEventRequest* node = mFirstCallbackEventRequest;
    mFirstCallbackEventRequest = node->next;
    if (!mFirstCallbackEventRequest) {
      mLastCallbackEventRequest = nullptr;
    }
    nsIReflowCallback* callback = node->callback;
    FreeMisc(sizeof(nsCallbackEventRequest), node);
    if (callback) {
      callback->ReflowCallbackCanceled();
    }
  }
}

void
PresShell::HandlePostedReflowCallbacks(bool aInterruptible)
{
   bool shouldFlush = false;

   while (mFirstCallbackEventRequest) {
     nsCallbackEventRequest* node = mFirstCallbackEventRequest;
     mFirstCallbackEventRequest = node->next;
     if (!mFirstCallbackEventRequest) {
       mLastCallbackEventRequest = nullptr;
     }
     nsIReflowCallback* callback = node->callback;
     FreeMisc(sizeof(nsCallbackEventRequest), node);
     if (callback) {
       if (callback->ReflowFinished()) {
         shouldFlush = true;
       }
     }
   }

   mozFlushType flushType =
     aInterruptible ? Flush_InterruptibleLayout : Flush_Layout;
   if (shouldFlush)
     FlushPendingNotifications(flushType);
}

bool
PresShell::IsSafeToFlush() const
{
  
  bool isSafeToFlush = !mIsReflowing &&
                         !mChangeNestCount;

  if (isSafeToFlush) {
    
    nsViewManager* viewManager = GetViewManager();
    if (viewManager) {
      bool isPainting = false;
      viewManager->IsPainting(isPainting);
      if (isPainting) {
        isSafeToFlush = false;
      }
    }
  }

  return isSafeToFlush;
}


void
PresShell::FlushPendingNotifications(mozFlushType aType)
{
  
  mozilla::ChangesToFlush flush(aType, aType >= Flush_Style);
  FlushPendingNotifications(flush);
}

void
PresShell::FlushPendingNotifications(mozilla::ChangesToFlush aFlush)
{
  if (mIsZombie) {
    return;
  }

  




  mozFlushType flushType = aFlush.mFlushType;

#ifdef MOZ_ENABLE_PROFILER_SPS
  static const char flushTypeNames[][20] = {
    "Content",
    "ContentAndNotify",
    "Style",
    "InterruptibleLayout",
    "Layout",
    "Display"
  };

  
  MOZ_ASSERT(static_cast<uint32_t>(flushType) <= ArrayLength(flushTypeNames));

  SAMPLE_LABEL_PRINTF("layout", "Flush", "(Flush_%s)",
                      flushTypeNames[flushType - 1]);
#endif

#ifdef ACCESSIBILITY
#ifdef DEBUG
  nsAccessibilityService* accService = GetAccService();
  if (accService) {
    NS_ASSERTION(!accService->IsProcessingRefreshDriverNotification(),
                 "Flush during accessible tree update!");
  }
#endif
#endif

  NS_ASSERTION(flushType >= Flush_Frames, "Why did we get called?");

  bool isSafeToFlush = IsSafeToFlush();

  
  
  bool hasHadScriptObject;
  if (mDocument->GetScriptHandlingObject(hasHadScriptObject) ||
      hasHadScriptObject) {
    isSafeToFlush = isSafeToFlush && nsContentUtils::IsSafeToRunScript();
  }

  NS_ASSERTION(!isSafeToFlush || mViewManager, "Must have view manager");
  
  nsRefPtr<nsViewManager> viewManagerDeathGrip = mViewManager;
  if (isSafeToFlush && mViewManager) {
    
    
    nsCOMPtr<nsIPresShell> kungFuDeathGrip(this);

    if (mResizeEvent.IsPending()) {
      FireResizeEvent();
      if (mIsDestroying) {
        return;
      }
    }

    
    
    
    
    
    
    mDocument->FlushExternalResources(flushType);

    
    
    
    
    mDocument->FlushPendingNotifications(Flush_ContentAndNotify);

    
    
    if (!mIsDestroying) {
      mViewManager->FlushDelayedResize(false);
      mPresContext->FlushPendingMediaFeatureValuesChanged();

      
      
      
      mPresContext->FlushUserFontSet();

      
      if (mDocument->HasAnimationController()) {
        mDocument->GetAnimationController()->FlushResampleRequests();
      }

      if (aFlush.mFlushAnimations &&
          CommonAnimationManager::ThrottlingEnabled() &&
          !mPresContext->StyleUpdateForAllAnimationsIsUpToDate()) {
        mPresContext->AnimationManager()->
          FlushAnimations(CommonAnimationManager::Cannot_Throttle);
        mPresContext->TransitionManager()->
          FlushTransitions(CommonAnimationManager::Cannot_Throttle);
        mPresContext->TickLastStyleUpdateForAllAnimations();
      }

      
      if (!mIsDestroying) {
        nsAutoScriptBlocker scriptBlocker;
        mFrameConstructor->CreateNeededFrames();
        mFrameConstructor->ProcessPendingRestyles();
      }
    }

    
    
    if (!mIsDestroying) {
      mPresContext->AnimationManager()->DispatchEvents();
    }

    
    
    
    if (!mIsDestroying) {
      mDocument->BindingManager()->ProcessAttachedQueue();
    }

    
    
    
    
    
    
    
    if (!mIsDestroying) {
      nsAutoScriptBlocker scriptBlocker;
      mFrameConstructor->CreateNeededFrames();
      mFrameConstructor->ProcessPendingRestyles();
    }


    
    
    

    if (flushType >= (mSuppressInterruptibleReflows ? Flush_Layout : Flush_InterruptibleLayout) &&
        !mIsDestroying) {
      mFrameConstructor->RecalcQuotesAndCounters();
      mViewManager->FlushDelayedResize(true);
      if (ProcessReflowCommands(flushType < Flush_Layout) && mContentToScrollTo) {
        
        DoScrollContentIntoView();
        if (mContentToScrollTo) {
          mContentToScrollTo->DeleteProperty(nsGkAtoms::scrolling);
          mContentToScrollTo = nullptr;
        }
      }
    } else if (!mIsDestroying && mSuppressInterruptibleReflows &&
               flushType == Flush_InterruptibleLayout) {
      
      
      mDocument->SetNeedLayoutFlush();
    }

    if (flushType >= Flush_Layout) {
      if (!mIsDestroying) {
        mViewManager->UpdateWidgetGeometry();
      }
    }
  }
}

void
PresShell::CharacterDataChanged(nsIDocument *aDocument,
                                nsIContent*  aContent,
                                CharacterDataChangeInfo* aInfo)
{
  NS_PRECONDITION(!mIsDocumentGone, "Unexpected CharacterDataChanged");
  NS_PRECONDITION(aDocument == mDocument, "Unexpected aDocument");

  nsAutoCauseReflowNotifier crNotifier(this);

  if (mCaret) {
    
    
    
    
    
    
    mCaret->InvalidateOutsideCaret();
  }

  
  
  
  nsIContent *container = aContent->GetParent();
  uint32_t selectorFlags =
    container ? (container->GetFlags() & NODE_ALL_SELECTOR_FLAGS) : 0;
  if (selectorFlags != 0 && !aContent->IsRootOfAnonymousSubtree()) {
    Element* element = container->AsElement();
    if (aInfo->mAppend && !aContent->GetNextSibling())
      mFrameConstructor->RestyleForAppend(element, aContent);
    else
      mFrameConstructor->RestyleForInsertOrChange(element, aContent);
  }

  mFrameConstructor->CharacterDataChanged(aContent, aInfo);
  VERIFY_STYLE_TREE;
}

void
PresShell::ContentStateChanged(nsIDocument* aDocument,
                               nsIContent* aContent,
                               nsEventStates aStateMask)
{
  NS_PRECONDITION(!mIsDocumentGone, "Unexpected ContentStateChanged");
  NS_PRECONDITION(aDocument == mDocument, "Unexpected aDocument");

  if (mDidInitialize) {
    nsAutoCauseReflowNotifier crNotifier(this);
    mFrameConstructor->ContentStateChanged(aContent, aStateMask);
    VERIFY_STYLE_TREE;
  }
}

void
PresShell::DocumentStatesChanged(nsIDocument* aDocument,
                                 nsEventStates aStateMask)
{
  NS_PRECONDITION(!mIsDocumentGone, "Unexpected DocumentStatesChanged");
  NS_PRECONDITION(aDocument == mDocument, "Unexpected aDocument");

  if (mDidInitialize &&
      mStyleSet->HasDocumentStateDependentStyle(mPresContext,
                                                mDocument->GetRootElement(),
                                                aStateMask)) {
    mFrameConstructor->PostRestyleEvent(mDocument->GetRootElement(),
                                        eRestyle_Subtree, NS_STYLE_HINT_NONE);
    VERIFY_STYLE_TREE;
  }

  if (aStateMask.HasState(NS_DOCUMENT_STATE_WINDOW_INACTIVE)) {
    nsIFrame* root = mFrameConstructor->GetRootFrame();
    if (root) {
      FrameLayerBuilder::InvalidateAllLayersForFrame(root);
      if (root->HasView()) {
        root->GetView()->SetForcedRepaint(true);
      }
      root->SchedulePaint();
    }
  }
}

void
PresShell::AttributeWillChange(nsIDocument* aDocument,
                               Element*     aElement,
                               int32_t      aNameSpaceID,
                               nsIAtom*     aAttribute,
                               int32_t      aModType)
{
  NS_PRECONDITION(!mIsDocumentGone, "Unexpected AttributeWillChange");
  NS_PRECONDITION(aDocument == mDocument, "Unexpected aDocument");

  
  
  
  if (mDidInitialize) {
    nsAutoCauseReflowNotifier crNotifier(this);
    mFrameConstructor->AttributeWillChange(aElement, aNameSpaceID,
                                           aAttribute, aModType);
    VERIFY_STYLE_TREE;
  }
}

void
PresShell::AttributeChanged(nsIDocument* aDocument,
                            Element*     aElement,
                            int32_t      aNameSpaceID,
                            nsIAtom*     aAttribute,
                            int32_t      aModType)
{
  NS_PRECONDITION(!mIsDocumentGone, "Unexpected AttributeChanged");
  NS_PRECONDITION(aDocument == mDocument, "Unexpected aDocument");

  
  
  
  if (mDidInitialize) {
    nsAutoCauseReflowNotifier crNotifier(this);
    mFrameConstructor->AttributeChanged(aElement, aNameSpaceID,
                                        aAttribute, aModType);
    VERIFY_STYLE_TREE;
  }
}

void
PresShell::ContentAppended(nsIDocument *aDocument,
                           nsIContent* aContainer,
                           nsIContent* aFirstNewContent,
                           int32_t     aNewIndexInContainer)
{
  NS_PRECONDITION(!mIsDocumentGone, "Unexpected ContentAppended");
  NS_PRECONDITION(aDocument == mDocument, "Unexpected aDocument");
  NS_PRECONDITION(aContainer, "must have container");
  
  if (!mDidInitialize) {
    return;
  }
  
  nsAutoCauseReflowNotifier crNotifier(this);

  
  
  
  mFrameConstructor->RestyleForAppend(aContainer->AsElement(), aFirstNewContent);

  mFrameConstructor->ContentAppended(aContainer, aFirstNewContent, true);
  VERIFY_STYLE_TREE;
}

void
PresShell::ContentInserted(nsIDocument* aDocument,
                           nsIContent*  aContainer,
                           nsIContent*  aChild,
                           int32_t      aIndexInContainer)
{
  NS_PRECONDITION(!mIsDocumentGone, "Unexpected ContentInserted");
  NS_PRECONDITION(aDocument == mDocument, "Unexpected aDocument");

  if (!mDidInitialize) {
    return;
  }
  
  nsAutoCauseReflowNotifier crNotifier(this);

  
  
  
  if (aContainer)
    mFrameConstructor->RestyleForInsertOrChange(aContainer->AsElement(), aChild);

  mFrameConstructor->ContentInserted(aContainer, aChild, nullptr, true);
  VERIFY_STYLE_TREE;
}

void
PresShell::ContentRemoved(nsIDocument *aDocument,
                          nsIContent* aContainer,
                          nsIContent* aChild,
                          int32_t     aIndexInContainer,
                          nsIContent* aPreviousSibling)
{
  NS_PRECONDITION(!mIsDocumentGone, "Unexpected ContentRemoved");
  NS_PRECONDITION(aDocument == mDocument, "Unexpected aDocument");

  
  if (mCaret) {
    mCaret->InvalidateOutsideCaret();
  }

  
  
  mPresContext->EventStateManager()->ContentRemoved(aDocument, aChild);

  nsAutoCauseReflowNotifier crNotifier(this);

  
  
  
  nsIContent* oldNextSibling;
  if (aContainer) {
    oldNextSibling = aContainer->GetChildAt(aIndexInContainer);
  } else {
    oldNextSibling = nullptr;
  }
  
  if (aContainer && aContainer->IsElement()) {
    mFrameConstructor->RestyleForRemove(aContainer->AsElement(), aChild,
                                        oldNextSibling);
  }

  bool didReconstruct;
  mFrameConstructor->ContentRemoved(aContainer, aChild, oldNextSibling,
                                    nsCSSFrameConstructor::REMOVE_CONTENT,
                                    &didReconstruct);

  VERIFY_STYLE_TREE;
}

nsresult
PresShell::ReconstructFrames(void)
{
  NS_PRECONDITION(!mFrameConstructor->GetRootFrame() || mDidInitialize,
                  "Must not have root frame before initial reflow");
  if (!mDidInitialize) {
    
    return NS_OK;
  }

  nsCOMPtr<nsIPresShell> kungFuDeathGrip(this);

  
  
  mDocument->FlushPendingNotifications(Flush_ContentAndNotify);

  nsAutoCauseReflowNotifier crNotifier(this);
  mFrameConstructor->BeginUpdate();
  nsresult rv = mFrameConstructor->ReconstructDocElementHierarchy();
  VERIFY_STYLE_TREE;
  mFrameConstructor->EndUpdate();

  return rv;
}

void
nsIPresShell::ReconstructStyleDataInternal()
{
  nsAutoTArray<nsRefPtr<mozilla::dom::Element>,1> scopeRoots;
  mChangedScopeStyleRoots.SwapElements(scopeRoots);

  if (mStylesHaveChanged) {
    
    
    scopeRoots.Clear();
  }

  mStylesHaveChanged = false;

  if (mIsDestroying) {
    
    return;
  }

  if (mPresContext) {
    mPresContext->RebuildUserFontSet();
    mPresContext->AnimationManager()->KeyframesListIsDirty();
  }

  Element* root = mDocument->GetRootElement();
  if (!mDidInitialize) {
    
    return;
  }

  if (!root) {
    
    return;
  }
  
  if (scopeRoots.IsEmpty()) {
    
    
    
    mFrameConstructor->PostRestyleEvent(root, eRestyle_Subtree,
                                        NS_STYLE_HINT_NONE);
  } else {
    for (uint32_t i = 0; i < scopeRoots.Length(); i++) {
      Element* scopeRoot = scopeRoots[i];
      mFrameConstructor->PostRestyleEvent(scopeRoot, eRestyle_Subtree,
                                          NS_STYLE_HINT_NONE);
    }
  }
}

void
nsIPresShell::ReconstructStyleDataExternal()
{
  ReconstructStyleDataInternal();
}

void
PresShell::RecordStyleSheetChange(nsIStyleSheet* aStyleSheet)
{
  if (mStylesHaveChanged)
    return;

  nsRefPtr<nsCSSStyleSheet> cssStyleSheet = do_QueryObject(aStyleSheet);
  if (cssStyleSheet) {
    Element* scopeElement = cssStyleSheet->GetScopeElement();
    if (scopeElement) {
      mChangedScopeStyleRoots.AppendElement(scopeElement);
      return;
    }
  }

  mStylesHaveChanged = true;
}

void
PresShell::StyleSheetAdded(nsIDocument *aDocument,
                           nsIStyleSheet* aStyleSheet,
                           bool aDocumentSheet)
{
  
  NS_PRECONDITION(aStyleSheet, "Must have a style sheet!");

  if (aStyleSheet->IsApplicable() && aStyleSheet->HasRules()) {
    RecordStyleSheetChange(aStyleSheet);
  }
}

void 
PresShell::StyleSheetRemoved(nsIDocument *aDocument,
                             nsIStyleSheet* aStyleSheet,
                             bool aDocumentSheet)
{
  
  NS_PRECONDITION(aStyleSheet, "Must have a style sheet!");

  if (aStyleSheet->IsApplicable() && aStyleSheet->HasRules()) {
    RecordStyleSheetChange(aStyleSheet);
  }
}

void
PresShell::StyleSheetApplicableStateChanged(nsIDocument *aDocument,
                                            nsIStyleSheet* aStyleSheet,
                                            bool aApplicable)
{
  if (aStyleSheet->HasRules()) {
    RecordStyleSheetChange(aStyleSheet);
  }
}

void
PresShell::StyleRuleChanged(nsIDocument *aDocument,
                            nsIStyleSheet* aStyleSheet,
                            nsIStyleRule* aOldStyleRule,
                            nsIStyleRule* aNewStyleRule)
{
  RecordStyleSheetChange(aStyleSheet);
}

void
PresShell::StyleRuleAdded(nsIDocument *aDocument,
                          nsIStyleSheet* aStyleSheet,
                          nsIStyleRule* aStyleRule) 
{
  RecordStyleSheetChange(aStyleSheet);
}

void
PresShell::StyleRuleRemoved(nsIDocument *aDocument,
                            nsIStyleSheet* aStyleSheet,
                            nsIStyleRule* aStyleRule) 
{
  RecordStyleSheetChange(aStyleSheet);
}

nsIFrame*
PresShell::GetRealPrimaryFrameFor(nsIContent* aContent) const
{
  if (aContent->GetDocument() != GetDocument()) {
    return nullptr;
  }
  nsIFrame *primaryFrame = aContent->GetPrimaryFrame();
  if (!primaryFrame)
    return nullptr;
  return nsPlaceholderFrame::GetRealFrameFor(primaryFrame);
}

nsIFrame*
PresShell::GetPlaceholderFrameFor(nsIFrame* aFrame) const
{
  return mFrameConstructor->GetPlaceholderFrameFor(aFrame);
}

nsresult
PresShell::RenderDocument(const nsRect& aRect, uint32_t aFlags,
                          nscolor aBackgroundColor,
                          gfxContext* aThebesContext)
{
  NS_ENSURE_TRUE(!(aFlags & RENDER_IS_UNTRUSTED), NS_ERROR_NOT_IMPLEMENTED);

  nsRootPresContext* rootPresContext = mPresContext->GetRootPresContext();
  if (rootPresContext) {
    rootPresContext->FlushWillPaintObservers();
    if (mIsDestroying)
      return NS_OK;
  }

  nsAutoScriptBlocker blockScripts;

  
  gfxRect r(0, 0,
            nsPresContext::AppUnitsToFloatCSSPixels(aRect.width),
            nsPresContext::AppUnitsToFloatCSSPixels(aRect.height));
  aThebesContext->NewPath();
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
  aThebesContext->Rectangle(r, true);
#else
  aThebesContext->Rectangle(r);
#endif

  nsIFrame* rootFrame = mFrameConstructor->GetRootFrame();
  if (!rootFrame) {
    
    aThebesContext->SetColor(gfxRGBA(aBackgroundColor));
    aThebesContext->Fill();
    return NS_OK;
  }

  gfxContextAutoSaveRestore save(aThebesContext);

  gfxContext::GraphicsOperator oldOperator = aThebesContext->CurrentOperator();
  if (oldOperator == gfxContext::OPERATOR_OVER) {
    
    
    aThebesContext->Clip();
  }

  
  
  
  
  
  
  
  bool needsGroup = NS_GET_A(aBackgroundColor) < 0xff ||
    oldOperator != gfxContext::OPERATOR_OVER;

  if (needsGroup) {
    aThebesContext->PushGroup(NS_GET_A(aBackgroundColor) == 0xff ?
                              gfxASurface::CONTENT_COLOR :
                              gfxASurface::CONTENT_COLOR_ALPHA);
    aThebesContext->Save();

    if (oldOperator != gfxContext::OPERATOR_OVER) {
      
      
      
      
      
      aThebesContext->Clip();
      aThebesContext->SetOperator(gfxContext::OPERATOR_OVER);
    }
  }

  aThebesContext->Translate(gfxPoint(-nsPresContext::AppUnitsToFloatCSSPixels(aRect.x),
                                     -nsPresContext::AppUnitsToFloatCSSPixels(aRect.y)));

  nsDeviceContext* devCtx = mPresContext->DeviceContext();
  gfxFloat scale = gfxFloat(devCtx->AppUnitsPerDevPixel())/nsPresContext::AppUnitsPerCSSPixel();
  aThebesContext->Scale(scale, scale);

  
  
  
  aThebesContext->NudgeCurrentMatrixToIntegers();

  AutoSaveRestoreRenderingState _(this);

  nsRefPtr<nsRenderingContext> rc = new nsRenderingContext();
  rc->Init(devCtx, aThebesContext);

  bool wouldFlushRetainedLayers = false;
  uint32_t flags = nsLayoutUtils::PAINT_IGNORE_SUPPRESSION;
  if (aThebesContext->CurrentMatrix().HasNonIntegerTranslation()) {
    flags |= nsLayoutUtils::PAINT_IN_TRANSFORM;
  }
  if (!(aFlags & RENDER_ASYNC_DECODE_IMAGES)) {
    flags |= nsLayoutUtils::PAINT_SYNC_DECODE_IMAGES;
  }
  if (aFlags & RENDER_USE_WIDGET_LAYERS) {
    
    nsView* view = rootFrame->GetView();
    if (view && view->GetWidget() &&
        nsLayoutUtils::GetDisplayRootFrame(rootFrame) == rootFrame) {
      flags |= nsLayoutUtils::PAINT_WIDGET_LAYERS;
    }
  }
  if (!(aFlags & RENDER_CARET)) {
    wouldFlushRetainedLayers = true;
    flags |= nsLayoutUtils::PAINT_HIDE_CARET;
  }
  if (aFlags & RENDER_IGNORE_VIEWPORT_SCROLLING) {
    wouldFlushRetainedLayers = !IgnoringViewportScrolling();
    mRenderFlags = ChangeFlag(mRenderFlags, true, STATE_IGNORING_VIEWPORT_SCROLLING);
  }
  if (aFlags & RENDER_DOCUMENT_RELATIVE) {
    
    
    
    
    wouldFlushRetainedLayers = true;
    flags |= nsLayoutUtils::PAINT_DOCUMENT_RELATIVE;
  }

  
  if ((flags & nsLayoutUtils::PAINT_WIDGET_LAYERS) && wouldFlushRetainedLayers) {
    flags &= ~nsLayoutUtils::PAINT_WIDGET_LAYERS;
  }

  nsLayoutUtils::PaintFrame(rc, rootFrame, nsRegion(aRect),
                            aBackgroundColor, flags);

  
  if (needsGroup) {
    aThebesContext->Restore();
    aThebesContext->PopGroupToSource();
    aThebesContext->Paint();
  }

  return NS_OK;
}





nsRect
PresShell::ClipListToRange(nsDisplayListBuilder *aBuilder,
                           nsDisplayList* aList,
                           nsRange* aRange)
{
  
  
  
  
  
  
  
  
  
  nsRect surfaceRect;
  nsDisplayList tmpList;

  nsDisplayItem* i;
  while ((i = aList->RemoveBottom())) {
    
    
    nsDisplayItem* itemToInsert = nullptr;
    nsIFrame* frame = i->GetUnderlyingFrame();
    if (frame) {
      nsIContent* content = frame->GetContent();
      if (content) {
        bool atStart = (content == aRange->GetStartParent());
        bool atEnd = (content == aRange->GetEndParent());
        if ((atStart || atEnd) && frame->GetType() == nsGkAtoms::textFrame) {
          int32_t frameStartOffset, frameEndOffset;
          frame->GetOffsets(frameStartOffset, frameEndOffset);

          int32_t hilightStart =
            atStart ? std::max(aRange->StartOffset(), frameStartOffset) : frameStartOffset;
          int32_t hilightEnd =
            atEnd ? std::min(aRange->EndOffset(), frameEndOffset) : frameEndOffset;
          if (hilightStart < hilightEnd) {
            
            nsPoint startPoint, endPoint;
            frame->GetPointFromOffset(hilightStart, &startPoint);
            frame->GetPointFromOffset(hilightEnd, &endPoint);

            
            
            
            
            nsRect textRect(aBuilder->ToReferenceFrame(frame), frame->GetSize());
            nscoord x = std::min(startPoint.x, endPoint.x);
            textRect.x += x;
            textRect.width = std::max(startPoint.x, endPoint.x) - x;
            surfaceRect.UnionRect(surfaceRect, textRect);

            
            
            
            itemToInsert = new (aBuilder)
                nsDisplayClip(aBuilder, frame, i, textRect);
          }
        }
        
        
        
        else if (content->GetCurrentDoc() ==
                   aRange->GetStartParent()->GetCurrentDoc()) {
          
          bool before, after;
          nsresult rv =
            nsRange::CompareNodeToRange(content, aRange, &before, &after);
          if (NS_SUCCEEDED(rv) && !before && !after) {
            itemToInsert = i;
            bool snap;
            surfaceRect.UnionRect(surfaceRect, i->GetBounds(aBuilder, &snap));
          }
        }
      }
    }

    
    
    nsDisplayList* sublist = i->GetSameCoordinateSystemChildren();
    if (itemToInsert || sublist) {
      tmpList.AppendToTop(itemToInsert ? itemToInsert : i);
      
      if (sublist)
        surfaceRect.UnionRect(surfaceRect,
          ClipListToRange(aBuilder, sublist, aRange));
    }
    else {
      
      i->~nsDisplayItem();
    }
  }

  
  aList->AppendToTop(&tmpList);

  return surfaceRect;
}

#ifdef DEBUG
#include <stdio.h>

static bool gDumpRangePaintList = false;
#endif

RangePaintInfo*
PresShell::CreateRangePaintInfo(nsIDOMRange* aRange,
                                nsRect& aSurfaceRect,
                                bool aForPrimarySelection)
{
  RangePaintInfo* info = nullptr;

  nsRange* range = static_cast<nsRange*>(aRange);

  nsIFrame* ancestorFrame;
  nsIFrame* rootFrame = GetRootFrame();

  
  
  
  nsINode* startParent = range->GetStartParent();
  nsINode* endParent = range->GetEndParent();
  nsIDocument* doc = startParent->GetCurrentDoc();
  if (startParent == doc || endParent == doc) {
    ancestorFrame = rootFrame;
  }
  else {
    nsINode* ancestor = nsContentUtils::GetCommonAncestor(startParent, endParent);
    NS_ASSERTION(!ancestor || ancestor->IsNodeOfType(nsINode::eCONTENT),
                 "common ancestor is not content");
    if (!ancestor || !ancestor->IsNodeOfType(nsINode::eCONTENT))
      return nullptr;

    nsIContent* ancestorContent = static_cast<nsIContent*>(ancestor);
    ancestorFrame = ancestorContent->GetPrimaryFrame();

    
    
    while (ancestorFrame &&
           nsLayoutUtils::GetNextContinuationOrSpecialSibling(ancestorFrame))
      ancestorFrame = ancestorFrame->GetParent();
  }

  if (!ancestorFrame)
    return nullptr;

  info = new RangePaintInfo(range, ancestorFrame);

  nsRect ancestorRect = ancestorFrame->GetVisualOverflowRect();

  
  info->mBuilder.SetIncludeAllOutOfFlows();
  if (aForPrimarySelection) {
    info->mBuilder.SetSelectedFramesOnly();
  }
  info->mBuilder.EnterPresShell(ancestorFrame, ancestorRect);
  ancestorFrame->BuildDisplayListForStackingContext(&info->mBuilder,
                                                    ancestorRect, &info->mList);

#ifdef DEBUG
  if (gDumpRangePaintList) {
    fprintf(stderr, "CreateRangePaintInfo --- before ClipListToRange:\n");
    nsFrame::PrintDisplayList(&(info->mBuilder), info->mList);
  }
#endif

  nsRect rangeRect = ClipListToRange(&info->mBuilder, &info->mList, range);

  info->mBuilder.LeavePresShell(ancestorFrame, ancestorRect);

#ifdef DEBUG
  if (gDumpRangePaintList) {
    fprintf(stderr, "CreateRangePaintInfo --- after ClipListToRange:\n");
    nsFrame::PrintDisplayList(&(info->mBuilder), info->mList);
  }
#endif

  
  
  
  info->mRootOffset = ancestorFrame->GetOffsetTo(rootFrame);
  rangeRect.MoveBy(info->mRootOffset);
  aSurfaceRect.UnionRect(aSurfaceRect, rangeRect);

  return info;
}

already_AddRefed<gfxASurface>
PresShell::PaintRangePaintInfo(nsTArray<nsAutoPtr<RangePaintInfo> >* aItems,
                               nsISelection* aSelection,
                               nsIntRegion* aRegion,
                               nsRect aArea,
                               nsIntPoint& aPoint,
                               nsIntRect* aScreenRect)
{
  nsPresContext* pc = GetPresContext();
  if (!pc || aArea.width == 0 || aArea.height == 0)
    return nullptr;

  nsDeviceContext* deviceContext = pc->DeviceContext();

  
  nsIntRect pixelArea = aArea.ToOutsidePixels(pc->AppUnitsPerDevPixel());

  
  float scale = 0.0;
  nsIntRect rootScreenRect =
    GetRootFrame()->GetScreenRectInAppUnits().ToNearestPixels(
      pc->AppUnitsPerDevPixel());

  
  
  nsRect maxSize;
  deviceContext->GetClientRect(maxSize);
  nscoord maxWidth = pc->AppUnitsToDevPixels(maxSize.width >> 1);
  nscoord maxHeight = pc->AppUnitsToDevPixels(maxSize.height >> 1);
  bool resize = (pixelArea.width > maxWidth || pixelArea.height > maxHeight);
  if (resize) {
    scale = 1.0;
    
    
    
    if (pixelArea.width > maxWidth)
      scale = std::min(scale, float(maxWidth) / pixelArea.width);
    if (pixelArea.height > maxHeight)
      scale = std::min(scale, float(maxHeight) / pixelArea.height);

    pixelArea.width = NSToIntFloor(float(pixelArea.width) * scale);
    pixelArea.height = NSToIntFloor(float(pixelArea.height) * scale);

    
    nscoord left = rootScreenRect.x + pixelArea.x;
    nscoord top = rootScreenRect.y + pixelArea.y;
    aScreenRect->x = NSToIntFloor(aPoint.x - float(aPoint.x - left) * scale);
    aScreenRect->y = NSToIntFloor(aPoint.y - float(aPoint.y - top) * scale);
  }
  else {
    
    aScreenRect->MoveTo(rootScreenRect.x + pixelArea.x, rootScreenRect.y + pixelArea.y);
  }
  aScreenRect->width = pixelArea.width;
  aScreenRect->height = pixelArea.height;

  gfxImageSurface* surface =
    new gfxImageSurface(gfxIntSize(pixelArea.width, pixelArea.height),
                        gfxImageSurface::ImageFormatARGB32);
  if (surface->CairoStatus()) {
    delete surface;
    return nullptr;
  }

  
  gfxContext context(surface);
  context.SetOperator(gfxContext::OPERATOR_CLEAR);
  context.Rectangle(gfxRect(0, 0, pixelArea.width, pixelArea.height));
  context.Fill();

  nsRefPtr<nsRenderingContext> rc = new nsRenderingContext();
  rc->Init(deviceContext, surface);

  if (aRegion) {
    
    nsIntRegion region =
      aRegion->ToAppUnits(nsPresContext::AppUnitsPerCSSPixel())
        .ToOutsidePixels(pc->AppUnitsPerDevPixel());
    rc->SetClip(region);
  }

  if (resize)
    rc->Scale(scale, scale);

  
  rc->Translate(-aArea.TopLeft());

  
  
  
  nsRefPtr<nsFrameSelection> frameSelection;
  if (aSelection) {
    nsCOMPtr<nsISelectionPrivate> selpriv = do_QueryInterface(aSelection);
    selpriv->GetFrameSelection(getter_AddRefs(frameSelection));
  }
  else {
    frameSelection = FrameSelection();
  }
  int16_t oldDisplaySelection = frameSelection->GetDisplaySelection();
  frameSelection->SetDisplaySelection(nsISelectionController::SELECTION_HIDDEN);

  
  int32_t count = aItems->Length();
  for (int32_t i = 0; i < count; i++) {
    RangePaintInfo* rangeInfo = (*aItems)[i];
    
    
    nsRenderingContext::AutoPushTranslation
      translate(rc, rangeInfo->mRootOffset);

    aArea.MoveBy(-rangeInfo->mRootOffset.x, -rangeInfo->mRootOffset.y);
    nsRegion visible(aArea);
    rangeInfo->mList.ComputeVisibilityForRoot(&rangeInfo->mBuilder, &visible);
    rangeInfo->mList.PaintRoot(&rangeInfo->mBuilder, rc, nsDisplayList::PAINT_DEFAULT);
    aArea.MoveBy(rangeInfo->mRootOffset.x, rangeInfo->mRootOffset.y);
  }

  
  frameSelection->SetDisplaySelection(oldDisplaySelection);

  NS_ADDREF(surface);
  return surface;
}

already_AddRefed<gfxASurface>
PresShell::RenderNode(nsIDOMNode* aNode,
                      nsIntRegion* aRegion,
                      nsIntPoint& aPoint,
                      nsIntRect* aScreenRect)
{
  
  
  nsRect area;
  nsTArray<nsAutoPtr<RangePaintInfo> > rangeItems;

  
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  if (!node->IsInDoc())
    return nullptr;
  
  nsRefPtr<nsRange> range = new nsRange();
  if (NS_FAILED(range->SelectNode(aNode)))
    return nullptr;

  RangePaintInfo* info = CreateRangePaintInfo(range, area, false);
  if (info && !rangeItems.AppendElement(info)) {
    delete info;
    return nullptr;
  }

  if (aRegion) {
    
    nsIntRect rrectPixels = aRegion->GetBounds();

    nsRect rrect = rrectPixels.ToAppUnits(nsPresContext::AppUnitsPerCSSPixel());
    area.IntersectRect(area, rrect);
    
    nsPresContext* pc = GetPresContext();
    if (!pc)
      return nullptr;

    
    aRegion->MoveBy(-pc->AppUnitsToDevPixels(area.x),
                    -pc->AppUnitsToDevPixels(area.y));
  }

  return PaintRangePaintInfo(&rangeItems, nullptr, aRegion, area, aPoint,
                             aScreenRect);
}

already_AddRefed<gfxASurface>
PresShell::RenderSelection(nsISelection* aSelection,
                           nsIntPoint& aPoint,
                           nsIntRect* aScreenRect)
{
  
  
  nsRect area;
  nsTArray<nsAutoPtr<RangePaintInfo> > rangeItems;

  
  
  
  int32_t numRanges;
  aSelection->GetRangeCount(&numRanges);
  NS_ASSERTION(numRanges > 0, "RenderSelection called with no selection");

  for (int32_t r = 0; r < numRanges; r++)
  {
    nsCOMPtr<nsIDOMRange> range;
    aSelection->GetRangeAt(r, getter_AddRefs(range));

    RangePaintInfo* info = CreateRangePaintInfo(range, area, true);
    if (info && !rangeItems.AppendElement(info)) {
      delete info;
      return nullptr;
    }
  }

  return PaintRangePaintInfo(&rangeItems, aSelection, nullptr, area, aPoint,
                             aScreenRect);
}

void
PresShell::AddPrintPreviewBackgroundItem(nsDisplayListBuilder& aBuilder,
                                         nsDisplayList&        aList,
                                         nsIFrame*             aFrame,
                                         const nsRect&         aBounds)
{
  aList.AppendNewToBottom(new (&aBuilder)
    nsDisplaySolidColor(&aBuilder, aFrame, aBounds, NS_RGB(115, 115, 115)));
}

static bool
AddCanvasBackgroundColor(const nsDisplayList& aList, nsIFrame* aCanvasFrame,
                         nscolor aColor)
{
  for (nsDisplayItem* i = aList.GetBottom(); i; i = i->GetAbove()) {
    if (i->GetUnderlyingFrame() == aCanvasFrame &&
        i->GetType() == nsDisplayItem::TYPE_CANVAS_BACKGROUND_COLOR) {
      nsDisplayCanvasBackgroundColor* bg = static_cast<nsDisplayCanvasBackgroundColor*>(i);
      bg->SetExtraBackgroundColor(aColor);
      return true;
    }
    nsDisplayList* sublist = i->GetSameCoordinateSystemChildren();
    if (sublist && AddCanvasBackgroundColor(*sublist, aCanvasFrame, aColor))
      return true;
  }
  return false;
}

void
PresShell::AddCanvasBackgroundColorItem(nsDisplayListBuilder& aBuilder,
                                        nsDisplayList&        aList,
                                        nsIFrame*             aFrame,
                                        const nsRect&         aBounds,
                                        nscolor               aBackstopColor,
                                        uint32_t              aFlags)
{
  if (aBounds.IsEmpty()) {
    return;
  }
  
  
  
  
  
  
  if (!(aFlags & nsIPresShell::FORCE_DRAW) &&
      !nsCSSRendering::IsCanvasFrame(aFrame)) {
    return;
  }

  nscolor bgcolor = NS_ComposeColors(aBackstopColor, mCanvasBackgroundColor);
  if (NS_GET_A(bgcolor) == 0)
    return;

  
  
  
  
  if (!aFrame->GetParent()) {
    nsIScrollableFrame* sf =
      aFrame->PresContext()->PresShell()->GetRootScrollFrameAsScrollable();
    if (sf) {
      nsCanvasFrame* canvasFrame = do_QueryFrame(sf->GetScrolledFrame());
      if (canvasFrame && canvasFrame->IsVisibleForPainting(&aBuilder)) {
        if (AddCanvasBackgroundColor(aList, canvasFrame, bgcolor))
          return;
      }
    }
  }

  aList.AppendNewToBottom(
    new (&aBuilder) nsDisplaySolidColor(&aBuilder, aFrame, aBounds, bgcolor));
}

static bool IsTransparentContainerElement(nsPresContext* aPresContext)
{
  nsCOMPtr<nsISupports> container = aPresContext->GetContainerInternal();
  nsCOMPtr<nsIDocShellTreeItem> docShellItem = do_QueryInterface(container);
  nsCOMPtr<nsPIDOMWindow> pwin(do_GetInterface(docShellItem));
  if (!pwin)
    return false;
  nsCOMPtr<nsIContent> containerElement =
    do_QueryInterface(pwin->GetFrameElementInternal());
  return containerElement &&
         containerElement->HasAttr(kNameSpaceID_None, nsGkAtoms::transparent);
}

nscolor PresShell::GetDefaultBackgroundColorToDraw()
{
  if (!mPresContext || !mPresContext->GetBackgroundColorDraw()) {
    return NS_RGB(255,255,255);
  }
  return mPresContext->DefaultBackgroundColor();
}

void PresShell::UpdateCanvasBackground()
{
  
  
  
  nsIFrame* rootStyleFrame = FrameConstructor()->GetRootElementStyleFrame();
  if (rootStyleFrame) {
    nsStyleContext* bgStyle =
      nsCSSRendering::FindRootFrameBackground(rootStyleFrame);
    
    
    
    
    bool drawBackgroundImage;
    bool drawBackgroundColor;

    mCanvasBackgroundColor =
      nsCSSRendering::DetermineBackgroundColor(mPresContext, bgStyle,
                                               rootStyleFrame,
                                               drawBackgroundImage,
                                               drawBackgroundColor);
    if (GetPresContext()->IsCrossProcessRootContentDocument() &&
        !IsTransparentContainerElement(mPresContext)) {
      mCanvasBackgroundColor =
        NS_ComposeColors(GetDefaultBackgroundColorToDraw(), mCanvasBackgroundColor);
    }
  }

  
  
  
  if (!FrameConstructor()->GetRootElementFrame()) {
    mCanvasBackgroundColor = GetDefaultBackgroundColorToDraw();
  }
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    if (TabChild* tabChild = GetTabChildFrom(this)) {
      tabChild->SetBackgroundColor(mCanvasBackgroundColor);
    }
  }
}

nscolor PresShell::ComputeBackstopColor(nsView* aDisplayRoot)
{
  nsIWidget* widget = aDisplayRoot->GetWidget();
  if (widget && (widget->GetTransparencyMode() != eTransparencyOpaque ||
                 widget->WidgetPaintsBackground())) {
    
    
    return NS_RGBA(0,0,0,0);
  }
  
  
  
  return GetDefaultBackgroundColorToDraw();
}

struct PaintParams {
  nscolor mBackgroundColor;
};

LayerManager* PresShell::GetLayerManager()
{
  NS_ASSERTION(mViewManager, "Should have view manager");

  nsView* rootView = mViewManager->GetRootView();
  if (rootView) {
    if (nsIWidget* widget = rootView->GetWidget()) {
      return widget->GetLayerManager();
    }
  }
  return nullptr;
}

void PresShell::SetIgnoreViewportScrolling(bool aIgnore)
{
  if (IgnoringViewportScrolling() == aIgnore) {
    return;
  }
  RenderingState state(this);
  state.mRenderFlags = ChangeFlag(state.mRenderFlags, aIgnore,
                                  STATE_IGNORING_VIEWPORT_SCROLLING);
  SetRenderingState(state);
}

void PresShell::SetDisplayPort(const nsRect& aDisplayPort)
{
  NS_ABORT_IF_FALSE(false, "SetDisplayPort is deprecated");
}

nsresult PresShell::SetResolution(float aXResolution, float aYResolution)
{
  if (!(aXResolution > 0.0 && aYResolution > 0.0)) {
    return NS_ERROR_ILLEGAL_VALUE;
  }
  if (aXResolution == mXResolution && aYResolution == mYResolution) {
    return NS_OK;
  }
  RenderingState state(this);
  state.mXResolution = aXResolution;
  state.mYResolution = aYResolution;
  SetRenderingState(state);
  return NS_OK;
 }

void PresShell::SetRenderingState(const RenderingState& aState)
{
  if (mRenderFlags != aState.mRenderFlags) {
    
    
    LayerManager* manager = GetLayerManager();
    if (manager) {
      FrameLayerBuilder::InvalidateAllLayers(manager);
    }
  }

  mRenderFlags = aState.mRenderFlags;
  mXResolution = aState.mXResolution;
  mYResolution = aState.mYResolution;
}

void PresShell::SynthesizeMouseMove(bool aFromScroll)
{
  if (!sSynthMouseMove)
    return;

  if (mPaintingSuppressed || !mIsActive || !mPresContext) {
    return;
  }

  if (!mPresContext->IsRoot()) {
    nsIPresShell* rootPresShell = GetRootPresShell();
    if (rootPresShell) {
      rootPresShell->SynthesizeMouseMove(aFromScroll);
    }
    return;
  }

  if (mMouseLocation == nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE))
    return;

  if (!mSynthMouseMoveEvent.IsPending()) {
    nsRefPtr<nsSynthMouseMoveEvent> ev =
        new nsSynthMouseMoveEvent(this, aFromScroll);

    if (!GetPresContext()->RefreshDriver()->AddRefreshObserver(ev,
                                                               Flush_Display)) {
      NS_WARNING("failed to dispatch nsSynthMouseMoveEvent");
      return;
    }

    mSynthMouseMoveEvent = ev;
  }
}













static nsView* FindFloatingViewContaining(nsView* aView, nsPoint aPt)
{
  if (aView->GetVisibility() == nsViewVisibility_kHide)
    
    return nullptr;

  nsIFrame* frame = aView->GetFrame();
  if (frame) {
    if (!frame->IsVisibleConsideringAncestors(nsIFrame::VISIBILITY_CROSS_CHROME_CONTENT_BOUNDARY) ||
        !frame->PresContext()->PresShell()->IsActive()) {
      return nullptr;
    }
  }

  for (nsView* v = aView->GetFirstChild(); v; v = v->GetNextSibling()) {
    nsView* r = FindFloatingViewContaining(v, v->ConvertFromParentCoords(aPt));
    if (r)
      return r;
  }

  if (aView->GetFloating() && aView->HasWidget() &&
      aView->GetDimensions().Contains(aPt))
    return aView;
    
  return nullptr;
}










static nsView* FindViewContaining(nsView* aView, nsPoint aPt)
{
  if (!aView->GetDimensions().Contains(aPt) ||
      aView->GetVisibility() == nsViewVisibility_kHide) {
    return nullptr;
  }

  nsIFrame* frame = aView->GetFrame();
  if (frame) {
    if (!frame->IsVisibleConsideringAncestors(nsIFrame::VISIBILITY_CROSS_CHROME_CONTENT_BOUNDARY) ||
        !frame->PresContext()->PresShell()->IsActive()) {
      return nullptr;
    }
  }

  for (nsView* v = aView->GetFirstChild(); v; v = v->GetNextSibling()) {
    nsView* r = FindViewContaining(v, v->ConvertFromParentCoords(aPt));
    if (r)
      return r;
  }

  return aView;
}

void
PresShell::ProcessSynthMouseMoveEvent(bool aFromScroll)
{
  
  nsCOMPtr<nsIDragSession> dragSession = nsContentUtils::GetDragSession();
  if (dragSession) {
    mSynthMouseMoveEvent.Forget();
    return;
  }

  
  
  if (aFromScroll) {
    mSynthMouseMoveEvent.Forget();
  }

  nsView* rootView = mViewManager ? mViewManager->GetRootView() : nullptr;
  if (mMouseLocation == nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE) ||
      !rootView || !rootView->HasWidget() || !mPresContext) {
    mSynthMouseMoveEvent.Forget();
    return;
  }

  NS_ASSERTION(mPresContext->IsRoot(), "Only a root pres shell should be here");

  
  
  nsCOMPtr<nsIPresShell> kungFuDeathGrip(this);
  
#ifdef DEBUG_MOUSE_LOCATION
  printf("[ps=%p]synthesizing mouse move to (%d,%d)\n",
         this, mMouseLocation.x, mMouseLocation.y);
#endif

  int32_t APD = mPresContext->AppUnitsPerDevPixel();

  
  
  
  
  nsView* view = nullptr;

  
  int32_t viewAPD;

  
  
  nsPoint refpoint(0, 0);

  
  
  nsViewManager *pointVM = nullptr;

  
  
  view = FindFloatingViewContaining(rootView, mMouseLocation);
  if (!view) {
    view = rootView;
    nsView *pointView = FindViewContaining(rootView, mMouseLocation);
    
    pointVM = (pointView ? pointView : view)->GetViewManager();
    refpoint = mMouseLocation + rootView->ViewToWidgetOffset();
    viewAPD = APD;
  } else {
    pointVM = view->GetViewManager();
    nsIFrame* frame = view->GetFrame();
    NS_ASSERTION(frame, "floating views can't be anonymous");
    viewAPD = frame->PresContext()->AppUnitsPerDevPixel();
    refpoint = mMouseLocation.ConvertAppUnits(APD, viewAPD);
    refpoint -= view->GetOffsetTo(rootView);
    refpoint += view->ViewToWidgetOffset();
  }
  NS_ASSERTION(view->GetWidget(), "view should have a widget here");
  nsMouseEvent event(true, NS_MOUSE_MOVE, view->GetWidget(),
                     nsMouseEvent::eSynthesized);
  event.refPoint = refpoint.ToNearestPixels(viewAPD);
  event.time = PR_IntervalNow();
  
  

  nsCOMPtr<nsIPresShell> shell = pointVM->GetPresShell();
  if (shell) {
    shell->DispatchSynthMouseMove(&event, !aFromScroll);
  }

  if (!aFromScroll) {
    mSynthMouseMoveEvent.Forget();
  }
}

class nsAutoNotifyDidPaint
{
public:
  nsAutoNotifyDidPaint(PresShell* aShell, uint32_t aFlags)
    : mShell(aShell), mFlags(aFlags)
  {
  }
  ~nsAutoNotifyDidPaint()
  {
    mShell->GetPresContext()->NotifyDidPaintForSubtree(mFlags);
  }

private:
  PresShell* mShell;
  uint32_t mFlags;
};

void
PresShell::Paint(nsView*        aViewToPaint,
                 const nsRegion& aDirtyRegion,
                 uint32_t        aFlags)
{
  SAMPLE_LABEL("Paint", "PresShell::Paint");
  NS_ASSERTION(!mIsDestroying, "painting a destroyed PresShell");
  NS_ASSERTION(aViewToPaint, "null view");

  if (!mIsActive || mIsZombie) {
    return;
  }

  nsAutoNotifyDidPaint notifyDidPaint(this, aFlags);

  nsPresContext* presContext = GetPresContext();
  AUTO_LAYOUT_PHASE_ENTRY_POINT(presContext, Paint);

  nsIFrame* frame = aViewToPaint->GetFrame();

  bool isRetainingManager;
  LayerManager* layerManager =
    aViewToPaint->GetWidget()->GetLayerManager(&isRetainingManager);
  NS_ASSERTION(layerManager, "Must be in paint event");

  
  
  
  
  
  if (mIsFirstPaint && !mPaintingSuppressed) {
    layerManager->SetIsFirstPaint();
    mIsFirstPaint = false;
  }

  if (frame && isRetainingManager) {
    
    
    
    
    
    
    if (!(aFlags & PAINT_LAYERS)) {
      if (layerManager->HasShadowManager()) {
        return;
      }
      layerManager->BeginTransaction();
      if (layerManager->EndEmptyTransaction()) {
        return;
      }
      NS_WARNING("Must complete empty transaction when compositing!");
    } else {
      layerManager->BeginTransaction();
    }

    if (!(frame->GetStateBits() & NS_FRAME_UPDATE_LAYER_TREE)) {
      NotifySubDocInvalidationFunc computeInvalidFunc =
        presContext->MayHavePaintEventListenerInSubDocument() ? nsPresContext::NotifySubDocInvalidation : 0;
      bool computeInvalidRect = computeInvalidFunc ||
                                (layerManager->GetBackendType() == LAYERS_BASIC);

      nsAutoPtr<LayerProperties> props(computeInvalidRect ? 
                                         LayerProperties::CloneFrom(layerManager->GetRoot()) : 
                                         nullptr);

      if (layerManager->EndEmptyTransaction((aFlags & PAINT_COMPOSITE) ?
            LayerManager::END_DEFAULT : LayerManager::END_NO_COMPOSITE)) {
        nsIntRegion invalid;
        if (props) {
          invalid = props->ComputeDifferences(layerManager->GetRoot(), computeInvalidFunc);
        } else {
          LayerProperties::ClearInvalidations(layerManager->GetRoot());
        }
        if (props) {
          if (!invalid.IsEmpty()) {
            nsIntRect bounds = invalid.GetBounds();
            nsRect rect(presContext->DevPixelsToAppUnits(bounds.x),
                        presContext->DevPixelsToAppUnits(bounds.y),
                        presContext->DevPixelsToAppUnits(bounds.width),
                        presContext->DevPixelsToAppUnits(bounds.height));
            aViewToPaint->GetViewManager()->InvalidateViewNoSuppression(aViewToPaint, rect);
            presContext->NotifyInvalidation(bounds, 0);
          }
        } else {
          aViewToPaint->GetViewManager()->InvalidateView(aViewToPaint);
        }

        frame->UpdatePaintCountForPaintedPresShells();
        return;
      }
    }
    frame->RemoveStateBits(NS_FRAME_UPDATE_LAYER_TREE);
  } else {
    layerManager->BeginTransaction();
  }
  if (frame) {
    frame->ClearPresShellsFromLastPaint();
  }

  nscolor bgcolor = ComputeBackstopColor(aViewToPaint);
  uint32_t flags = nsLayoutUtils::PAINT_WIDGET_LAYERS | nsLayoutUtils::PAINT_EXISTING_TRANSACTION;
  if (!(aFlags & PAINT_COMPOSITE)) {
    flags |= nsLayoutUtils::PAINT_NO_COMPOSITE;
  }
  if (aViewToPaint->InAlternatePaint()) {
    flags |= nsLayoutUtils::PAINT_NO_CLEAR_INVALIDATIONS;
  }

  if (frame) {
    
    
    
    
    frame->BeginDeferringInvalidatesForDisplayRoot(aDirtyRegion);

    
    nsLayoutUtils::PaintFrame(nullptr, frame, aDirtyRegion, bgcolor, flags);

    frame->EndDeferringInvalidatesForDisplayRoot();
    return;
  }

  nsRefPtr<ColorLayer> root = layerManager->CreateColorLayer();
  if (root) {
    nsPresContext* pc = GetPresContext();
    nsIntRect bounds =
      pc->GetVisibleArea().ToOutsidePixels(pc->AppUnitsPerDevPixel());
    bgcolor = NS_ComposeColors(bgcolor, mCanvasBackgroundColor);
    root->SetColor(bgcolor);
    root->SetVisibleRegion(bounds);
    layerManager->SetRoot(root);
  }
  layerManager->EndTransaction(NULL, NULL, (aFlags & PAINT_COMPOSITE) ?
    LayerManager::END_DEFAULT : LayerManager::END_NO_COMPOSITE);
}


void
nsIPresShell::SetCapturingContent(nsIContent* aContent, uint8_t aFlags)
{
  
  
  if (!aContent && gCaptureInfo.mPointerLock &&
      !(aFlags & CAPTURE_POINTERLOCK)) {
    return;
  }

  NS_IF_RELEASE(gCaptureInfo.mContent);

  
  
  if ((aFlags & CAPTURE_IGNOREALLOWED) || gCaptureInfo.mAllowed ||
      (aFlags & CAPTURE_POINTERLOCK)) {
    if (aContent) {
      NS_ADDREF(gCaptureInfo.mContent = aContent);
    }
    
    gCaptureInfo.mRetargetToElement = ((aFlags & CAPTURE_RETARGETTOELEMENT) != 0) ||
                                      ((aFlags & CAPTURE_POINTERLOCK) != 0);
    gCaptureInfo.mPreventDrag = (aFlags & CAPTURE_PREVENTDRAG) != 0;
    gCaptureInfo.mPointerLock = (aFlags & CAPTURE_POINTERLOCK) != 0;
  }
}

nsIFrame*
PresShell::GetCurrentEventFrame()
{
  if (MOZ_UNLIKELY(mIsDestroying)) {
    return nullptr;
  }
    
  if (!mCurrentEventFrame && mCurrentEventContent) {
    
    
    
    
    if (mCurrentEventContent->GetDocument()) {
      mCurrentEventFrame = mCurrentEventContent->GetPrimaryFrame();
    }
  }

  return mCurrentEventFrame;
}

nsIFrame*
PresShell::GetEventTargetFrame()
{
  return GetCurrentEventFrame();
}

already_AddRefed<nsIContent>
PresShell::GetEventTargetContent(nsEvent* aEvent)
{
  nsIContent* content = nullptr;

  if (mCurrentEventContent) {
    content = mCurrentEventContent;
    NS_IF_ADDREF(content);
  } else {
    nsIFrame* currentEventFrame = GetCurrentEventFrame();
    if (currentEventFrame) {
      currentEventFrame->GetContentForEvent(aEvent, &content);
    } else {
      content = nullptr;
    }
  }
  return content;
}

void
PresShell::PushCurrentEventInfo(nsIFrame* aFrame, nsIContent* aContent)
{
  if (mCurrentEventFrame || mCurrentEventContent) {
    mCurrentEventFrameStack.InsertElementAt(0, mCurrentEventFrame);
    mCurrentEventContentStack.InsertObjectAt(mCurrentEventContent, 0);
  }
  mCurrentEventFrame = aFrame;
  mCurrentEventContent = aContent;
}

void
PresShell::PopCurrentEventInfo()
{
  mCurrentEventFrame = nullptr;
  mCurrentEventContent = nullptr;

  if (0 != mCurrentEventFrameStack.Length()) {
    mCurrentEventFrame = mCurrentEventFrameStack.ElementAt(0);
    mCurrentEventFrameStack.RemoveElementAt(0);
    mCurrentEventContent = mCurrentEventContentStack.ObjectAt(0);
    mCurrentEventContentStack.RemoveObjectAt(0);
  }
}

bool PresShell::InZombieDocument(nsIContent *aContent)
{
  
  
  
  
  
  
  nsIDocument *doc = aContent->GetDocument();
  return !doc || !doc->GetWindow();
}

already_AddRefed<nsPIDOMWindow>
PresShell::GetRootWindow()
{
  nsCOMPtr<nsPIDOMWindow> window =
    do_QueryInterface(mDocument->GetWindow());
  if (window) {
    nsCOMPtr<nsPIDOMWindow> rootWindow = window->GetPrivateRoot();
    NS_ASSERTION(rootWindow, "nsPIDOMWindow::GetPrivateRoot() returns NULL");
    return rootWindow.forget();
  }

  
  
  nsCOMPtr<nsIPresShell> parent = GetParentPresShell();
  NS_ENSURE_TRUE(parent, nullptr);
  return parent->GetRootWindow();
}

already_AddRefed<nsIPresShell>
PresShell::GetParentPresShell()
{
  NS_ENSURE_TRUE(mPresContext, nullptr);
  nsCOMPtr<nsISupports> container = mPresContext->GetContainer();
  if (!container) {
    container = do_QueryReferent(mForwardingContainer);
  }

  
  nsCOMPtr<nsIDocShellTreeItem> treeItem = do_QueryInterface(container);
  
  NS_ENSURE_TRUE(treeItem, nullptr);

  nsCOMPtr<nsIDocShellTreeItem> parentTreeItem;
  treeItem->GetParent(getter_AddRefs(parentTreeItem));
  nsCOMPtr<nsIDocShell> parentDocShell = do_QueryInterface(parentTreeItem);
  NS_ENSURE_TRUE(parentDocShell && treeItem != parentTreeItem, nullptr);

  nsCOMPtr<nsIPresShell> parentPresShell = parentDocShell->GetPresShell();
  return parentPresShell.forget();
}

nsresult
PresShell::RetargetEventToParent(nsGUIEvent*     aEvent,
                                 nsEventStatus*  aEventStatus)
{
  
  
  
  

  nsCOMPtr<nsIPresShell> kungFuDeathGrip(this);
  nsCOMPtr<nsIPresShell> parentPresShell = GetParentPresShell();
  NS_ENSURE_TRUE(parentPresShell, NS_ERROR_FAILURE);

  
  return parentPresShell->HandleEvent(parentPresShell->GetRootFrame(), aEvent, true, aEventStatus);
}

void
PresShell::DisableNonTestMouseEvents(bool aDisable)
{
  sDisableNonTestMouseEvents = aDisable;
}

already_AddRefed<nsPIDOMWindow>
PresShell::GetFocusedDOMWindowInOurWindow()
{
  nsCOMPtr<nsPIDOMWindow> rootWindow = GetRootWindow();
  NS_ENSURE_TRUE(rootWindow, nullptr);
  nsPIDOMWindow* focusedWindow;
  nsFocusManager::GetFocusedDescendant(rootWindow, true, &focusedWindow);
  return focusedWindow;
}

void
PresShell::RecordMouseLocation(nsGUIEvent* aEvent)
{
  if (!mPresContext)
    return;

  if (!mPresContext->IsRoot()) {
    PresShell* rootPresShell = GetRootPresShell();
    if (rootPresShell) {
      rootPresShell->RecordMouseLocation(aEvent);
    }
    return;
  }

  if ((aEvent->message == NS_MOUSE_MOVE &&
       static_cast<nsMouseEvent*>(aEvent)->reason == nsMouseEvent::eReal) ||
      aEvent->message == NS_MOUSE_ENTER ||
      aEvent->message == NS_MOUSE_BUTTON_DOWN ||
      aEvent->message == NS_MOUSE_BUTTON_UP) {
    nsIFrame* rootFrame = GetRootFrame();
    if (!rootFrame) {
      nsView* rootView = mViewManager->GetRootView();
      mMouseLocation = nsLayoutUtils::TranslateWidgetToView(mPresContext,
        aEvent->widget, aEvent->refPoint, rootView);
    } else {
      mMouseLocation =
        nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, rootFrame);
    }
#ifdef DEBUG_MOUSE_LOCATION
    if (aEvent->message == NS_MOUSE_ENTER)
      printf("[ps=%p]got mouse enter for %p\n",
             this, aEvent->widget);
    printf("[ps=%p]setting mouse location to (%d,%d)\n",
           this, mMouseLocation.x, mMouseLocation.y);
#endif
    if (aEvent->message == NS_MOUSE_ENTER)
      SynthesizeMouseMove(false);
  } else if (aEvent->message == NS_MOUSE_EXIT) {
    
    
    
    
    
    mMouseLocation = nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
#ifdef DEBUG_MOUSE_LOCATION
    printf("[ps=%p]got mouse exit for %p\n",
           this, aEvent->widget);
    printf("[ps=%p]clearing mouse location\n",
           this);
#endif
  }
}

static void
EvictTouchPoint(nsCOMPtr<nsIDOMTouch>& aTouch)
{
  nsIWidget *widget = nullptr;
  
  nsCOMPtr<nsINode> node(do_QueryInterface(aTouch->GetTarget()));
  if (!node) {
    return;
  }
  nsIDocument* doc = node->GetCurrentDoc();
  if (!doc) {
    return;
  }
  nsIPresShell *presShell = doc->GetShell();
  if (!presShell) {
    return;
  }
  nsIFrame* frame = presShell->GetRootFrame();
  if (!frame) {
    return;
  }
  nsPoint pt(aTouch->mRefPoint.x, aTouch->mRefPoint.y);
  widget = frame->GetView()->GetNearestWidget(&pt);

  if (!widget) {
    return;
  }
  
  nsTouchEvent event(true, NS_TOUCH_END, widget);
  event.widget = widget;
  event.time = PR_IntervalNow();
  event.touches.AppendElement(aTouch);

  nsEventStatus status;
  widget->DispatchEvent(&event, status);
}

static PLDHashOperator
AppendToTouchList(const uint32_t& aKey, nsCOMPtr<nsIDOMTouch>& aData, void *aTouchList)
{
  nsTArray<nsCOMPtr<nsIDOMTouch> > *touches = static_cast<nsTArray<nsCOMPtr<nsIDOMTouch> > *>(aTouchList);
  aData->mChanged = false;
  touches->AppendElement(aData);
  return PL_DHASH_NEXT;
}

static PLDHashOperator
FindAnyTarget(const uint32_t& aKey, nsCOMPtr<nsIDOMTouch>& aData,
              void* aAnyTarget)
{
  if (aData) {
    nsCOMPtr<nsIDOMEventTarget> target;
    aData->GetTarget(getter_AddRefs(target));
    if (target) {
      nsCOMPtr<nsIContent>* content =
        static_cast<nsCOMPtr<nsIContent>*>(aAnyTarget);
      *content = do_QueryInterface(target);
      return PL_DHASH_STOP;
    }
  }
  return PL_DHASH_NEXT;
}

nsIFrame* GetNearestFrameContainingPresShell(nsIPresShell* aPresShell)
{
  nsView* view = aPresShell->GetViewManager()->GetRootView();
  while (view && !view->GetFrame()) {
    view = view->GetParent();
  }

  nsIFrame* frame = nullptr;
  if (view) {
    frame = view->GetFrame();
  }

  return frame;
}

nsresult
PresShell::HandleEvent(nsIFrame        *aFrame,
                       nsGUIEvent*     aEvent,
                       bool            aDontRetargetEvents,
                       nsEventStatus*  aEventStatus)
{
  NS_ASSERTION(aFrame, "null frame");

  if (mIsDestroying ||
      (sDisableNonTestMouseEvents && !aEvent->mFlags.mIsSynthesizedForTests &&
       NS_IS_MOUSE_EVENT(aEvent))) {
    return NS_OK;
  }

  RecordMouseLocation(aEvent);

  if (!nsContentUtils::IsSafeToRunScript())
    return NS_OK;

  nsIContent* capturingContent =
    NS_IS_MOUSE_EVENT(aEvent) || aEvent->eventStructType == NS_WHEEL_EVENT ?
      GetCapturingContent() : nullptr;

  nsCOMPtr<nsIDocument> retargetEventDoc;
  if (!aDontRetargetEvents) {
    
    
    
    
    
    
    
    
    
    if (NS_IsEventTargetedAtFocusedWindow(aEvent)) {
      nsCOMPtr<nsPIDOMWindow> window = GetFocusedDOMWindowInOurWindow();
      
      
      if (!window) {
        return NS_OK;
      }

      retargetEventDoc = do_QueryInterface(window->GetExtantDocument());
      if (!retargetEventDoc)
        return NS_OK;
    } else if (capturingContent) {
      
      
      retargetEventDoc = capturingContent->GetCurrentDoc();
#ifdef ANDROID
    } else if (aEvent->eventStructType == NS_TOUCH_EVENT) {
      retargetEventDoc = GetTouchEventTargetDocument();
#endif
    }

    if (retargetEventDoc) {
      nsCOMPtr<nsIPresShell> presShell = retargetEventDoc->GetShell();
      if (!presShell)
        return NS_OK;

      if (presShell != this) {
        nsIFrame* frame = presShell->GetRootFrame();
        if (!frame) {
          if (aEvent->message == NS_QUERY_TEXT_CONTENT ||
              NS_IS_CONTENT_COMMAND_EVENT(aEvent)) {
            return NS_OK;
          }

          frame = GetNearestFrameContainingPresShell(presShell);
        }

        if (!frame)
          return NS_OK;

        nsCOMPtr<nsIPresShell> shell = frame->PresContext()->GetPresShell();
        return shell->HandleEvent(frame, aEvent, true, aEventStatus);
      }
    }
  }

  if (aEvent->eventStructType == NS_KEY_EVENT &&
      mDocument && mDocument->EventHandlingSuppressed()) {
    if (aEvent->message == NS_KEY_DOWN) {
      mNoDelayedKeyEvents = true;
    } else if (!mNoDelayedKeyEvents) {
      nsDelayedEvent* event =
        new nsDelayedKeyEvent(static_cast<nsKeyEvent*>(aEvent));
      if (!mDelayedEvents.AppendElement(event)) {
        delete event;
      }
    }
    return NS_OK;
  }

  nsIFrame* frame = aFrame;

  if (aEvent->eventStructType == NS_TOUCH_EVENT) {
    FlushPendingNotifications(Flush_Layout);
    frame = GetNearestFrameContainingPresShell(this);
  }

  bool dispatchUsingCoordinates = NS_IsEventUsingCoordinates(aEvent);
  if (dispatchUsingCoordinates) {
    NS_WARN_IF_FALSE(frame, "Nothing to handle this event!");
    if (!frame)
      return NS_OK;

    nsPresContext* framePresContext = frame->PresContext();
    nsPresContext* rootPresContext = framePresContext->GetRootPresContext();
    NS_ASSERTION(rootPresContext == mPresContext->GetRootPresContext(),
                 "How did we end up outside the connected prescontext/viewmanager hierarchy?"); 
    
    
    
    if (framePresContext == rootPresContext &&
        frame == mFrameConstructor->GetRootFrame()) {
      nsIFrame* popupFrame =
        nsLayoutUtils::GetPopupFrameForEventCoordinates(rootPresContext, aEvent);
      
      
      if (popupFrame &&
          !nsContentUtils::ContentIsCrossDocDescendantOf(
             framePresContext->GetPresShell()->GetDocument(),
             popupFrame->GetContent())) {
        frame = popupFrame;
      }
    }

    bool captureRetarget = false;
    if (capturingContent) {
      
      
      
      
      bool vis;
      nsCOMPtr<nsISupports> supports = mPresContext->GetContainer();
      nsCOMPtr<nsIBaseWindow> baseWin(do_QueryInterface(supports));
      if (baseWin && NS_SUCCEEDED(baseWin->GetVisibility(&vis)) && vis) {
        captureRetarget = gCaptureInfo.mRetargetToElement;
        if (!captureRetarget) {
          
          
          NS_ASSERTION(capturingContent->GetCurrentDoc() == GetDocument(),
                       "Unexpected document");
          nsIFrame* captureFrame = capturingContent->GetPrimaryFrame();
          if (captureFrame) {
            if (capturingContent->Tag() == nsGkAtoms::select &&
                capturingContent->IsHTML()) {
              
              
              nsIFrame* childFrame = captureFrame->GetChildList(nsIFrame::kSelectPopupList).FirstChild();
              if (childFrame) {
                captureFrame = childFrame;
              }
            }

            
            
            nsIScrollableFrame* scrollFrame = do_QueryFrame(captureFrame);
            if (scrollFrame) {
              frame = scrollFrame->GetScrolledFrame();
            }
          }
        }
      }
      else {
        ClearMouseCapture(nullptr);
        capturingContent = nullptr;
      }
    }

    
    if (aEvent->eventStructType == NS_TOUCH_EVENT &&
        aEvent->message != NS_TOUCH_START) {
      captureRetarget = true;
    }

    bool isWindowLevelMouseExit = (aEvent->message == NS_MOUSE_EXIT) &&
      (static_cast<nsMouseEvent*>(aEvent)->exit == nsMouseEvent::eTopLevel);

    
    
    
    
    
    if (!captureRetarget && !isWindowLevelMouseExit) {
      nsPoint eventPoint;
      if (aEvent->message == NS_TOUCH_START) {
        nsTouchEvent* touchEvent = static_cast<nsTouchEvent*>(aEvent);
        
        
        
        if (touchEvent->touches.Length() == 1) {
          nsTArray<nsCOMPtr<nsIDOMTouch> > touches;
          gCaptureTouchList.Enumerate(&AppendToTouchList, (void *)&touches);
          for (uint32_t i = 0; i < touches.Length(); ++i) {
            EvictTouchPoint(touches[i]);
          }
        }
        
        
        
        nsCOMPtr<nsIContent> anyTarget;
        if (gCaptureTouchList.Count() > 0) {
          gCaptureTouchList.Enumerate(&FindAnyTarget, &anyTarget);
        } else {
          gPreventMouseEvents = false;
        }

        
        for (int32_t i = touchEvent->touches.Length(); i; ) {
          --i;
          nsIDOMTouch *touch = touchEvent->touches[i];
          nsDOMTouch *domtouch = static_cast<nsDOMTouch*>(touch);
          touch->mMessage = aEvent->message;

          int32_t id = 0;
          touch->GetIdentifier(&id);
          if (!gCaptureTouchList.Get(id, nullptr)) {
            
            
            touch->mChanged = true;

            
            uint32_t flags = 0;
            eventPoint = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent,
                                                              touch->mRefPoint,
                                                              frame);
            nsIFrame* target = FindFrameTargetedByInputEvent(aEvent,
                                                             frame,
                                                             eventPoint,
                                                             flags);
            if (target && !anyTarget) {
              target->GetContentForEvent(aEvent, getter_AddRefs(anyTarget));
              while (anyTarget && !anyTarget->IsElement()) {
                anyTarget = anyTarget->GetParent();
              }
              domtouch->SetTarget(anyTarget);
              gCaptureTouchList.Put(id, touch);
            } else {
              nsIFrame* newTargetFrame = nullptr;
              for (nsIFrame* f = target; f;
                   f = nsLayoutUtils::GetParentOrPlaceholderForCrossDoc(f)) {
                if (f->PresContext()->Document() == anyTarget->OwnerDoc()) {
                  newTargetFrame = f;
                  break;
                }
                
                
                
                f = f->PresContext()->GetPresShell()->GetRootFrame();
              }
    
              
              
              
              
              if (!newTargetFrame) {
                touchEvent->touches.RemoveElementAt(i);
              } else {
                target = newTargetFrame;
                nsCOMPtr<nsIContent> targetContent;
                target->GetContentForEvent(aEvent, getter_AddRefs(targetContent));
                while (targetContent && !targetContent->IsElement()) {
                  targetContent = targetContent->GetParent();
                }
                touch->SetTarget(targetContent);
                gCaptureTouchList.Put(id, touch);
              }
            }
            if (target) {
              frame = target;
            }
          } else {
            
            
            touch->mChanged = false;
            int32_t id;
            touch->GetIdentifier(&id);

            nsCOMPtr<nsIDOMTouch> oldTouch;
            gCaptureTouchList.Get(id, getter_AddRefs(oldTouch));
            if (oldTouch) {
              nsCOMPtr<nsIDOMEventTarget> targetPtr;
              oldTouch->GetTarget(getter_AddRefs(targetPtr));
              domtouch->SetTarget(targetPtr);
              gCaptureTouchList.Put(id, touch);
            }
          }
        }
      } else {
        eventPoint = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, frame);
      }
      uint32_t flags = 0;
      if (aEvent->eventStructType == NS_MOUSE_EVENT &&
          static_cast<nsMouseEvent*>(aEvent)->ignoreRootScrollFrame) {
        flags |= INPUT_IGNORE_ROOT_SCROLL_FRAME;
      }
      nsIFrame* target =
        FindFrameTargetedByInputEvent(aEvent, frame, eventPoint, flags);
      if (target) {
        frame = target;
      }
    }

    
    
    
    
    if (capturingContent &&
        (gCaptureInfo.mRetargetToElement || !frame->GetContent() ||
         !nsContentUtils::ContentIsCrossDocDescendantOf(frame->GetContent(),
                                                        capturingContent))) {
      
      
      NS_ASSERTION(capturingContent->GetCurrentDoc() == GetDocument(),
                   "Unexpected document");
      nsIFrame* capturingFrame = capturingContent->GetPrimaryFrame();
      if (capturingFrame) {
        frame = capturingFrame;
      }
    }

    
    
    if (aEvent->eventStructType == NS_MOUSE_EVENT &&
        frame->PresContext()->Document()->EventHandlingSuppressed()) {
      if (aEvent->message == NS_MOUSE_BUTTON_DOWN) {
        mNoDelayedMouseEvents = true;
      } else if (!mNoDelayedMouseEvents && aEvent->message == NS_MOUSE_BUTTON_UP) {
        nsDelayedEvent* event =
          new nsDelayedMouseEvent(static_cast<nsMouseEvent*>(aEvent));
        if (!mDelayedEvents.AppendElement(event)) {
          delete event;
        }
      }

      return NS_OK;
    }

    PresShell* shell =
        static_cast<PresShell*>(frame->PresContext()->PresShell());
    switch (aEvent->message) {
      case NS_TOUCH_MOVE:
      case NS_TOUCH_CANCEL:
      case NS_TOUCH_END: {
        
        nsTouchEvent* touchEvent = static_cast<nsTouchEvent*>(aEvent);
        nsTArray<nsCOMPtr<nsIDOMTouch> >  &touches = touchEvent->touches;
        for (uint32_t i = 0; i < touches.Length(); ++i) {
          nsIDOMTouch *touch = touches[i];
          if (!touch) {
            break;
          }
  
          int32_t id;
          touch->GetIdentifier(&id);
          nsCOMPtr<nsIDOMTouch> oldTouch;
          gCaptureTouchList.Get(id, getter_AddRefs(oldTouch));
          if (!oldTouch) {
            break;
          }
  
          nsCOMPtr<nsIDOMEventTarget> targetPtr;
          oldTouch->GetTarget(getter_AddRefs(targetPtr));
          nsCOMPtr<nsIContent> content = do_QueryInterface(targetPtr);
          if (!content) {
            break;
          }

          nsIFrame* contentFrame = content->GetPrimaryFrame();
          if (!contentFrame) {
            break;
          }

          shell = static_cast<PresShell*>(
                      contentFrame->PresContext()->PresShell());
          if (shell) {
            break;
          }
        }
        break;
      }
    }

    
    
    
    
    
    
    
    
    
    nsEventStateManager* activeESM =
      nsEventStateManager::GetActiveEventStateManager();
    if (activeESM && NS_IS_MOUSE_EVENT(aEvent) &&
        activeESM != shell->GetPresContext()->EventStateManager() &&
        static_cast<nsEventStateManager*>(activeESM)->GetPresContext()) {
      nsIPresShell* activeShell =
        static_cast<nsEventStateManager*>(activeESM)->GetPresContext()->GetPresShell();
      if (activeShell &&
          nsContentUtils::ContentIsCrossDocDescendantOf(activeShell->GetDocument(),
                                                        shell->GetDocument())) {
        shell = static_cast<PresShell*>(activeShell);
        frame = shell->GetRootFrame();
      }
    }

    if (shell != this) {
      
      
      nsCOMPtr<nsIPresShell> kungFuDeathGrip(shell);
      
      
      
      
      return shell->HandlePositionedEvent(frame, aEvent, aEventStatus);
    }

    return HandlePositionedEvent(frame, aEvent, aEventStatus);
  }
  
  nsresult rv = NS_OK;
  
  if (frame) {
    PushCurrentEventInfo(nullptr, nullptr);

    
    if (NS_IsEventTargetedAtFocusedContent(aEvent)) {
      mCurrentEventContent = nullptr;

      nsCOMPtr<nsPIDOMWindow> window =
        do_QueryInterface(mDocument->GetWindow());
      nsCOMPtr<nsPIDOMWindow> focusedWindow;
      nsCOMPtr<nsIContent> eventTarget =
        nsFocusManager::GetFocusedDescendant(window, false,
                                             getter_AddRefs(focusedWindow));

      
      
      
      
      if (!eventTarget || !eventTarget->GetPrimaryFrame()) {
        nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(mDocument);
        if (htmlDoc) {
          nsCOMPtr<nsIDOMHTMLElement> body;
          htmlDoc->GetBody(getter_AddRefs(body));
          eventTarget = do_QueryInterface(body);
          if (!eventTarget) {
            eventTarget = mDocument->GetRootElement();
          }
        } else {
          eventTarget = mDocument->GetRootElement();
        }
      }

      if (aEvent->message == NS_KEY_DOWN) {
        NS_IF_RELEASE(gKeyDownTarget);
        NS_IF_ADDREF(gKeyDownTarget = eventTarget);
      }
      else if ((aEvent->message == NS_KEY_PRESS || aEvent->message == NS_KEY_UP) &&
               gKeyDownTarget) {
        
        
        
        
        
        
        if (eventTarget &&
            nsContentUtils::IsChromeDoc(gKeyDownTarget->GetCurrentDoc()) !=
            nsContentUtils::IsChromeDoc(eventTarget->GetCurrentDoc())) {
          eventTarget = gKeyDownTarget;
        }

        if (aEvent->message == NS_KEY_UP) {
          NS_RELEASE(gKeyDownTarget);
        }
      }

      mCurrentEventFrame = nullptr;
      nsIDocument* targetDoc = eventTarget ? eventTarget->OwnerDoc() : nullptr;
      if (targetDoc && targetDoc != mDocument) {
        PopCurrentEventInfo();
        nsCOMPtr<nsIPresShell> shell = targetDoc->GetShell();
        if (shell) {
          rv = static_cast<PresShell*>(shell.get())->
            HandleRetargetedEvent(aEvent, aEventStatus, eventTarget);
        }
        return rv;
      } else {
        mCurrentEventContent = eventTarget;
      }
        
      if (!mCurrentEventContent || !GetCurrentEventFrame() ||
          InZombieDocument(mCurrentEventContent)) {
        rv = RetargetEventToParent(aEvent, aEventStatus);
        PopCurrentEventInfo();
        return rv;
      }
    } else {
      mCurrentEventFrame = frame;
    }
    if (GetCurrentEventFrame()) {
      rv = HandleEventInternal(aEvent, aEventStatus);
    }
  
#ifdef DEBUG
    ShowEventTargetDebug();
#endif
    PopCurrentEventInfo();
  } else {
    
    

    if (!NS_EVENT_NEEDS_FRAME(aEvent)) {
      mCurrentEventFrame = nullptr;
      return HandleEventInternal(aEvent, aEventStatus);
    }
    else if (NS_IS_KEY_EVENT(aEvent)) {
      
      
      return RetargetEventToParent(aEvent, aEventStatus);
    }
  }

  return rv;
}

#ifdef ANDROID
nsIDocument*
PresShell::GetTouchEventTargetDocument()
{
  nsPresContext* context = GetPresContext();
  if (!context || !context->IsRoot()) {
    return nullptr;
  }

  nsCOMPtr<nsISupports> container = context->GetContainer();
  nsCOMPtr<nsIDocShellTreeItem> shellAsTreeItem = do_QueryInterface(container);
  if (!shellAsTreeItem) {
    return nullptr;
  }

  nsCOMPtr<nsIDocShellTreeOwner> owner;
  shellAsTreeItem->GetTreeOwner(getter_AddRefs(owner));
  if (!owner) {
    return nullptr;
  }

  
  nsCOMPtr<nsIDocShellTreeItem> item;
  owner->GetPrimaryContentShell(getter_AddRefs(item));
  nsCOMPtr<nsIDocShell> childDocShell = do_QueryInterface(item);
  nsCOMPtr<nsIDocument> result = do_GetInterface(childDocShell);
  return result;
}
#endif

#ifdef DEBUG
void
PresShell::ShowEventTargetDebug()
{
  if (nsFrame::GetShowEventTargetFrameBorder() &&
      GetCurrentEventFrame()) {
    if (mDrawEventTargetFrame) {
      mDrawEventTargetFrame->InvalidateFrame();
    }

    mDrawEventTargetFrame = mCurrentEventFrame;
    mDrawEventTargetFrame->InvalidateFrame();
  }
}
#endif

nsresult
PresShell::HandlePositionedEvent(nsIFrame*      aTargetFrame,
                                 nsGUIEvent*    aEvent,
                                 nsEventStatus* aEventStatus)
{
  nsresult rv = NS_OK;
  
  PushCurrentEventInfo(nullptr, nullptr);
  
  mCurrentEventFrame = aTargetFrame;

  if (mCurrentEventFrame) {
    nsCOMPtr<nsIContent> targetElement;
    mCurrentEventFrame->GetContentForEvent(aEvent,
                                           getter_AddRefs(targetElement));

    
    
    
    if (targetElement) {
      
      
      
      
      
      
      
      
      while (targetElement && !targetElement->IsElement()) {
        targetElement = targetElement->GetParent();
      }

      
      if (!targetElement) {
        mCurrentEventContent = nullptr;
        mCurrentEventFrame = nullptr;
      } else if (targetElement != mCurrentEventContent) {
        mCurrentEventContent = targetElement;
      }
    }
  }

  if (GetCurrentEventFrame()) {
    rv = HandleEventInternal(aEvent, aEventStatus);
  }

#ifdef DEBUG
  ShowEventTargetDebug();
#endif
  PopCurrentEventInfo();
  return rv;
}

nsresult
PresShell::HandleEventWithTarget(nsEvent* aEvent, nsIFrame* aFrame,
                                 nsIContent* aContent, nsEventStatus* aStatus)
{
  PushCurrentEventInfo(aFrame, aContent);
  nsresult rv = HandleEventInternal(aEvent, aStatus);
  PopCurrentEventInfo();
  return rv;
}

static bool CanHandleContextMenuEvent(nsMouseEvent* aMouseEvent,
                                        nsIFrame* aFrame)
{
#if defined(XP_MACOSX) && defined(MOZ_XUL)
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm) {
    nsIFrame* popupFrame = pm->GetTopPopup(ePopupTypeMenu);
    if (popupFrame) {
      
      
      if (aMouseEvent->context == nsMouseEvent::eContextMenuKey) {
        return false;
      } else if (aMouseEvent->widget) {
         nsWindowType windowType;
         aMouseEvent->widget->GetWindowType(windowType);
         if (windowType == eWindowType_popup) {
           for (nsIFrame* current = aFrame; current;
                current = nsLayoutUtils::GetCrossDocParentFrame(current)) {
             if (current->GetType() == nsGkAtoms::menuPopupFrame) {
               return false;
             }
           }
         }
      }
    }
  }
#endif
  return true;
}

nsresult
PresShell::HandleEventInternal(nsEvent* aEvent, nsEventStatus* aStatus)
{
  nsRefPtr<nsEventStateManager> manager = mPresContext->EventStateManager();
  nsresult rv = NS_OK;

  if (!NS_EVENT_NEEDS_FRAME(aEvent) || GetCurrentEventFrame()) {
    bool touchIsNew = false;
    bool isHandlingUserInput = false;

    
    if (aEvent->mFlags.mIsTrusted) {
      switch (aEvent->message) {
      case NS_KEY_PRESS:
      case NS_KEY_DOWN:
      case NS_KEY_UP: {
        nsIDocument *doc = mCurrentEventContent ?
                           mCurrentEventContent->OwnerDoc() : nullptr;
        nsIDocument* root = nullptr;
        if (static_cast<const nsKeyEvent*>(aEvent)->keyCode == NS_VK_ESCAPE &&
            (root = nsContentUtils::GetRootDocument(doc)) &&
            root->IsFullScreenDoc()) {
          
          
          
          
          aEvent->mFlags.mDefaultPrevented = true;
          aEvent->mFlags.mOnlyChromeDispatch = true;

          if (aEvent->message == NS_KEY_UP) {
             
             
            nsIDocument::ExitFullScreen(true);
          }
        }
        
        
      }
      case NS_MOUSE_BUTTON_DOWN:
      case NS_MOUSE_BUTTON_UP:
        isHandlingUserInput = true;
        break;
      case NS_TOUCH_CANCEL:
      case NS_TOUCH_END: {
        
        
        nsTouchEvent* touchEvent = static_cast<nsTouchEvent*>(aEvent);
        nsTArray<nsCOMPtr<nsIDOMTouch> >  &touches = touchEvent->touches;
        for (uint32_t i = 0; i < touches.Length(); ++i) {
          nsIDOMTouch *touch = touches[i];
          nsDOMTouch *domtouch = static_cast<nsDOMTouch*>(touch);
          if (!touch) {
            continue;
          }
          touch->mMessage = aEvent->message;
          touch->mChanged = true;
          nsCOMPtr<nsIDOMTouch> oldTouch;

          int32_t id;
          touch->GetIdentifier(&id);

          gCaptureTouchList.Get(id, getter_AddRefs(oldTouch));
          if (!oldTouch) {
            continue;
          }
          nsCOMPtr<nsIDOMEventTarget> targetPtr;
          oldTouch->GetTarget(getter_AddRefs(targetPtr));

          mCurrentEventContent = do_QueryInterface(targetPtr);
          domtouch->SetTarget(targetPtr);
          gCaptureTouchList.Remove(id);
        }
        
        gCaptureTouchList.Enumerate(&AppendToTouchList, (void *)&touches);
        break;
      }
      case NS_TOUCH_MOVE: {
        
        nsTouchEvent* touchEvent = static_cast<nsTouchEvent*>(aEvent);
        nsTArray<nsCOMPtr<nsIDOMTouch> >& touches = touchEvent->touches;
        bool haveChanged = false;
        for (int32_t i = touches.Length(); i; ) {
          --i;
          nsIDOMTouch *touch = touches[i];
          nsDOMTouch *domtouch = static_cast<nsDOMTouch*>(touch);
          if (!touch) {
            continue;
          }
          int32_t id;
          touch->GetIdentifier(&id);
          touch->mMessage = aEvent->message;

          nsCOMPtr<nsIDOMTouch> oldTouch;
          gCaptureTouchList.Get(id, getter_AddRefs(oldTouch));
          if (!oldTouch) {
            touches.RemoveElementAt(i);
            continue;
          }
          if(domtouch->Equals(oldTouch)) {
            touch->mChanged = true;
            haveChanged = true;
          }

          nsCOMPtr<nsIDOMEventTarget> targetPtr;
          oldTouch->GetTarget(getter_AddRefs(targetPtr));
          if (!targetPtr) {
            touches.RemoveElementAt(i);
            continue;
          }
          domtouch->SetTarget(targetPtr);

          gCaptureTouchList.Put(id, touch);
          
          
          if (oldTouch->mMessage != touch->mMessage) {
            touchIsNew = true;
          }
        }
        
        if (!haveChanged) {
          if (gPreventMouseEvents) {
              *aStatus = nsEventStatus_eConsumeNoDefault;
          }
          return NS_OK;
        }
        break;
      }
      case NS_DRAGDROP_DROP:
        nsCOMPtr<nsIDragSession> session = nsContentUtils::GetDragSession();
        if (session) {
          bool onlyChromeDrop = false;
          session->GetOnlyChromeDrop(&onlyChromeDrop);
          if (onlyChromeDrop) {
            aEvent->mFlags.mOnlyChromeDispatch = true;
          }
        }
        break;
      }
    }

    if (aEvent->message == NS_CONTEXTMENU) {
      nsMouseEvent* me = static_cast<nsMouseEvent*>(aEvent);
      if (!CanHandleContextMenuEvent(me, GetCurrentEventFrame())) {
        return NS_OK;
      }
      if (me->context == nsMouseEvent::eContextMenuKey &&
          !AdjustContextMenuKeyEvent(me)) {
        return NS_OK;
      }
      if (me->IsShift()) {
        aEvent->mFlags.mOnlyChromeDispatch = true;
        aEvent->mFlags.mRetargetToNonNativeAnonymous = true;
      }
    }

    nsAutoHandlingUserInputStatePusher userInpStatePusher(isHandlingUserInput,
                                                          aEvent, mDocument);

    if (aEvent->mFlags.mIsTrusted && aEvent->message == NS_MOUSE_MOVE) {
      nsIPresShell::AllowMouseCapture(
        nsEventStateManager::GetActiveEventStateManager() == manager);
    }

    nsAutoPopupStatePusher popupStatePusher(nsDOMEvent::GetEventPopupControlState(aEvent));

    
    
    aEvent->target = nullptr;

    
    
    rv = manager->PreHandleEvent(mPresContext, aEvent, mCurrentEventFrame, aStatus);

    
    if (NS_SUCCEEDED(rv)) {
      bool wasHandlingKeyBoardEvent =
        nsContentUtils::IsHandlingKeyBoardEvent();
      if (aEvent->eventStructType == NS_KEY_EVENT) {
        nsContentUtils::SetIsHandlingKeyBoardEvent(true);
      }
      if (NS_IsAllowedToDispatchDOMEvent(aEvent)) {
        nsPresShellEventCB eventCB(this);
        if (aEvent->eventStructType == NS_TOUCH_EVENT) {
          DispatchTouchEvent(aEvent, aStatus, &eventCB, touchIsNew);
        } else {
          nsCOMPtr<nsINode> eventTarget = mCurrentEventContent.get();
          nsPresShellEventCB* eventCBPtr = &eventCB;
          if (!eventTarget) {
            nsCOMPtr<nsIContent> targetContent;
            if (mCurrentEventFrame) {
              rv = mCurrentEventFrame->
                     GetContentForEvent(aEvent, getter_AddRefs(targetContent));
            }
            if (NS_SUCCEEDED(rv) && targetContent) {
              eventTarget = do_QueryInterface(targetContent);
            } else if (mDocument) {
              eventTarget = do_QueryInterface(mDocument);
              
              
              eventCBPtr = nullptr;
            }
          }
          if (eventTarget) {
            if (aEvent->eventStructType == NS_COMPOSITION_EVENT ||
                aEvent->eventStructType == NS_TEXT_EVENT) {
              nsIMEStateManager::DispatchCompositionEvent(eventTarget,
                mPresContext, aEvent, aStatus, eventCBPtr);
            } else {
              nsEventDispatcher::Dispatch(eventTarget, mPresContext,
                                          aEvent, nullptr, aStatus, eventCBPtr);
            }
          }
        }
      }

      nsContentUtils::SetIsHandlingKeyBoardEvent(wasHandlingKeyBoardEvent);

      
      
      if (!mIsDestroying && NS_SUCCEEDED(rv)) {
        rv = manager->PostHandleEvent(mPresContext, aEvent,
                                      GetCurrentEventFrame(), aStatus);
      }
    }

    if (aEvent->message == NS_MOUSE_BUTTON_UP) {
      
      SetCapturingContent(nullptr, 0);
    } else if (aEvent->message == NS_MOUSE_MOVE) {
      nsIPresShell::AllowMouseCapture(false);
    }
  }
  return rv;
}

void
PresShell::DispatchTouchEvent(nsEvent *aEvent,
                              nsEventStatus* aStatus,
                              nsPresShellEventCB* aEventCB,
                              bool aTouchIsNew)
{
  
  
  bool canPrevent = aEvent->message == NS_TOUCH_START ||
              (aEvent->message == NS_TOUCH_MOVE && aTouchIsNew);
  bool preventDefault = false;
  nsEventStatus tmpStatus = nsEventStatus_eIgnore;
  nsTouchEvent* touchEvent = static_cast<nsTouchEvent*>(aEvent);

  
  for (uint32_t i = 0; i < touchEvent->touches.Length(); ++i) {
    nsIDOMTouch *touch = touchEvent->touches[i];
    if (!touch || !touch->mChanged) {
      continue;
    }

    nsCOMPtr<nsIDOMEventTarget> targetPtr;
    touch->GetTarget(getter_AddRefs(targetPtr));
    nsCOMPtr<nsIContent> content = do_QueryInterface(targetPtr);
    if (!content) {
      continue;
    }

    nsIDocument* doc = content->OwnerDoc();
    nsIContent* capturingContent = GetCapturingContent();
    if (capturingContent) {
      if (capturingContent->OwnerDoc() != doc) {
        
        continue;
      }
      content = capturingContent;
    }
    
    nsTouchEvent newEvent(touchEvent->mFlags.mIsTrusted, touchEvent);
    newEvent.target = targetPtr;

    nsRefPtr<PresShell> contentPresShell;
    if (doc == mDocument) {
      contentPresShell = static_cast<PresShell*>(doc->GetShell());
      if (contentPresShell) {
        
        
        contentPresShell->PushCurrentEventInfo(
            content->GetPrimaryFrame(), content);
      }
    }

    nsIPresShell *presShell = doc->GetShell();
    if (!presShell) {
      continue;
    }

    nsPresContext *context = presShell->GetPresContext();

    tmpStatus = nsEventStatus_eIgnore;
    nsEventDispatcher::Dispatch(targetPtr, context,
                                &newEvent, nullptr, &tmpStatus, aEventCB);
    if (nsEventStatus_eConsumeNoDefault == tmpStatus) {
      preventDefault = true;
    }

    if (contentPresShell) {
      contentPresShell->PopCurrentEventInfo();
    }
  }

  
  
  
  if (preventDefault && canPrevent) {
    gPreventMouseEvents = true;
  }

  if (gPreventMouseEvents) {
    *aStatus = nsEventStatus_eConsumeNoDefault;
  } else {
    *aStatus = nsEventStatus_eIgnore;
  }
}



nsresult
PresShell::HandleDOMEventWithTarget(nsIContent* aTargetContent, nsEvent* aEvent,
                                    nsEventStatus* aStatus)
{
  nsresult rv = NS_OK;

  PushCurrentEventInfo(nullptr, aTargetContent);

  
  
  
  
  
  nsCOMPtr<nsISupports> container = mPresContext->GetContainer();
  if (container) {

    
    rv = nsEventDispatcher::Dispatch(aTargetContent, mPresContext, aEvent, nullptr,
                                     aStatus);
  }

  PopCurrentEventInfo();
  return rv;
}


nsresult
PresShell::HandleDOMEventWithTarget(nsIContent* aTargetContent,
                                    nsIDOMEvent* aEvent,
                                    nsEventStatus* aStatus)
{
  nsresult rv = NS_OK;

  PushCurrentEventInfo(nullptr, aTargetContent);
  nsCOMPtr<nsISupports> container = mPresContext->GetContainer();
  if (container) {
    rv = nsEventDispatcher::DispatchDOMEvent(aTargetContent, nullptr, aEvent,
                                             mPresContext, aStatus);
  }

  PopCurrentEventInfo();
  return rv;
}

bool
PresShell::AdjustContextMenuKeyEvent(nsMouseEvent* aEvent)
{
#ifdef MOZ_XUL
  
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm) {
    nsIFrame* popupFrame = pm->GetTopPopup(ePopupTypeMenu);
    if (popupFrame) {
      nsIFrame* itemFrame = 
        (static_cast<nsMenuPopupFrame *>(popupFrame))->GetCurrentMenuItem();
      if (!itemFrame)
        itemFrame = popupFrame;

      nsCOMPtr<nsIWidget> widget = popupFrame->GetNearestWidget();
      aEvent->widget = widget;
      nsIntPoint widgetPoint = widget->WidgetToScreenOffset();
      aEvent->refPoint = itemFrame->GetScreenRect().BottomLeft() - widgetPoint;

      mCurrentEventContent = itemFrame->GetContent();
      mCurrentEventFrame = itemFrame;

      return true;
    }
  }
#endif

  
  
  
  
  
  
  
  
  nsRootPresContext* rootPC = mPresContext->GetRootPresContext();
  aEvent->refPoint.x = 0;
  aEvent->refPoint.y = 0;
  if (rootPC) {
    rootPC->PresShell()->GetViewManager()->
      GetRootWidget(getter_AddRefs(aEvent->widget));

    if (aEvent->widget) {
      
      nsPoint offset(0, 0);
      nsIFrame* rootFrame = mFrameConstructor->GetRootFrame();
      if (rootFrame) {
        nsView* view = rootFrame->GetClosestView(&offset);
        offset += view->GetOffsetToWidget(aEvent->widget);
        aEvent->refPoint =
          offset.ToNearestPixels(mPresContext->AppUnitsPerDevPixel());
      }
    }
  } else {
    aEvent->widget = nullptr;
  }

  
  nsIntPoint caretPoint;
  
  
  if (PrepareToUseCaretPosition(aEvent->widget, caretPoint)) {
    
    aEvent->refPoint = caretPoint;
    return true;
  }

  
  
  
  nsCOMPtr<nsIDOMElement> currentFocus;
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (fm)
    fm->GetFocusedElement(getter_AddRefs(currentFocus));

  
  if (currentFocus) {
    nsCOMPtr<nsIContent> currentPointElement;
    GetCurrentItemAndPositionForElement(currentFocus,
                                        getter_AddRefs(currentPointElement),
                                        aEvent->refPoint,
                                        aEvent->widget);
    if (currentPointElement) {
      mCurrentEventContent = currentPointElement;
      mCurrentEventFrame = nullptr;
      GetCurrentEventFrame();
    }
  }

  return true;
}












bool
PresShell::PrepareToUseCaretPosition(nsIWidget* aEventWidget, nsIntPoint& aTargetPt)
{
  nsresult rv;

  
  nsRefPtr<nsCaret> caret = GetCaret();
  NS_ENSURE_TRUE(caret, false);

  bool caretVisible = false;
  rv = caret->GetCaretVisible(&caretVisible);
  if (NS_FAILED(rv) || ! caretVisible)
    return false;

  
  
  nsISelection* domSelection = caret->GetCaretDOMSelection();
  NS_ENSURE_TRUE(domSelection, false);

  
  
  
  nsIFrame* frame = nullptr; 
  nsCOMPtr<nsIDOMNode> node;
  rv = domSelection->GetFocusNode(getter_AddRefs(node));
  NS_ENSURE_SUCCESS(rv, false);
  NS_ENSURE_TRUE(node, false);
  nsCOMPtr<nsIContent> content(do_QueryInterface(node));
  if (content) {
    nsIContent* nonNative = content->FindFirstNonChromeOnlyAccessContent();
    content = nonNative;
  }

  if (content) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    rv = ScrollContentIntoView(content,
                               nsIPresShell::ScrollAxis(
                                 nsIPresShell::SCROLL_MINIMUM,
                                 nsIPresShell::SCROLL_IF_NOT_VISIBLE),
                               nsIPresShell::ScrollAxis(
                                 nsIPresShell::SCROLL_MINIMUM,
                                 nsIPresShell::SCROLL_IF_NOT_VISIBLE),
                               nsIPresShell::SCROLL_OVERFLOW_HIDDEN);
    NS_ENSURE_SUCCESS(rv, false);
    frame = content->GetPrimaryFrame();
    NS_WARN_IF_FALSE(frame, "No frame for focused content?");
  }

  
  
  
  
  
  
  
  nsCOMPtr<nsISelectionController> selCon;
  if (frame)
    frame->GetSelectionController(GetPresContext(), getter_AddRefs(selCon));
  else
    selCon = static_cast<nsISelectionController *>(this);
  if (selCon) {
    rv = selCon->ScrollSelectionIntoView(nsISelectionController::SELECTION_NORMAL,
                                         nsISelectionController::SELECTION_FOCUS_REGION,
                                         nsISelectionController::SCROLL_SYNCHRONOUS);
    NS_ENSURE_SUCCESS(rv, false);
  }

  nsPresContext* presContext = GetPresContext();

  
  nsRect caretCoords;
  nsIFrame* caretFrame = caret->GetGeometry(domSelection, &caretCoords);
  if (!caretFrame)
    return false;
  nsPoint viewOffset;
  nsView* view = caretFrame->GetClosestView(&viewOffset);
  if (!view)
    return false;
  
  if (aEventWidget) {
    viewOffset += view->GetOffsetToWidget(aEventWidget);
  }
  caretCoords.MoveBy(viewOffset);

  
  aTargetPt.x =
    presContext->AppUnitsToDevPixels(caretCoords.x + caretCoords.width);
  aTargetPt.y =
    presContext->AppUnitsToDevPixels(caretCoords.y + caretCoords.height);

  
  
  aTargetPt.y -= 1;

  return true;
}

void
PresShell::GetCurrentItemAndPositionForElement(nsIDOMElement *aCurrentEl,
                                               nsIContent** aTargetToUse,
                                               nsIntPoint& aTargetPt,
                                               nsIWidget *aRootWidget)
{
  nsCOMPtr<nsIContent> focusedContent(do_QueryInterface(aCurrentEl));
  ScrollContentIntoView(focusedContent,
                        ScrollAxis(),
                        ScrollAxis(),
                        nsIPresShell::SCROLL_OVERFLOW_HIDDEN);

  nsPresContext* presContext = GetPresContext();

  bool istree = false, checkLineHeight = true;
  nscoord extraTreeY = 0;

#ifdef MOZ_XUL
  
  
  
  
  nsCOMPtr<nsIDOMXULSelectControlItemElement> item;
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelect =
    do_QueryInterface(aCurrentEl);
  if (multiSelect) {
    checkLineHeight = false;
    
    int32_t currentIndex;
    multiSelect->GetCurrentIndex(&currentIndex);
    if (currentIndex >= 0) {
      nsCOMPtr<nsIDOMXULElement> xulElement(do_QueryInterface(aCurrentEl));
      if (xulElement) {
        nsCOMPtr<nsIBoxObject> box;
        xulElement->GetBoxObject(getter_AddRefs(box));
        nsCOMPtr<nsITreeBoxObject> treeBox(do_QueryInterface(box));
        
        
        
        
        
        if (treeBox) {
          treeBox->EnsureRowIsVisible(currentIndex);
          int32_t firstVisibleRow, rowHeight;
          treeBox->GetFirstVisibleRow(&firstVisibleRow);
          treeBox->GetRowHeight(&rowHeight);

          extraTreeY += presContext->CSSPixelsToAppUnits(
                          (currentIndex - firstVisibleRow + 1) * rowHeight);
          istree = true;

          nsCOMPtr<nsITreeColumns> cols;
          treeBox->GetColumns(getter_AddRefs(cols));
          if (cols) {
            nsCOMPtr<nsITreeColumn> col;
            cols->GetFirstColumn(getter_AddRefs(col));
            if (col) {
              nsCOMPtr<nsIDOMElement> colElement;
              col->GetElement(getter_AddRefs(colElement));
              nsCOMPtr<nsIContent> colContent(do_QueryInterface(colElement));
              if (colContent) {
                nsIFrame* frame = colContent->GetPrimaryFrame();
                if (frame) {
                  extraTreeY += frame->GetSize().height;
                }
              }
            }
          }
        }
        else {
          multiSelect->GetCurrentItem(getter_AddRefs(item));
        }
      }
    }
  }
  else {
    
    nsCOMPtr<nsIDOMXULMenuListElement> menulist = do_QueryInterface(aCurrentEl);
    if (!menulist) {
      nsCOMPtr<nsIDOMXULSelectControlElement> select =
        do_QueryInterface(aCurrentEl);
      if (select) {
        checkLineHeight = false;
        select->GetSelectedItem(getter_AddRefs(item));
      }
    }
  }

  if (item)
    focusedContent = do_QueryInterface(item);
#endif

  nsIFrame *frame = focusedContent->GetPrimaryFrame();
  if (frame) {
    NS_ASSERTION(frame->PresContext() == GetPresContext(),
      "handling event for focused content that is not in our document?");

    nsPoint frameOrigin(0, 0);

    
    nsView *view = frame->GetClosestView(&frameOrigin);
    NS_ASSERTION(view, "No view for frame");

    
    if (aRootWidget) {
      frameOrigin += view->GetOffsetToWidget(aRootWidget);
    }

    
    
    
    
    
    
    
    
    nscoord extra = 0;
    if (!istree) {
      extra = frame->GetSize().height;
      if (checkLineHeight) {
        nsIScrollableFrame *scrollFrame =
          nsLayoutUtils::GetNearestScrollableFrame(frame);
        if (scrollFrame) {
          nsSize scrollAmount = scrollFrame->GetLineScrollAmount();
          nsIFrame* f = do_QueryFrame(scrollFrame);
          int32_t APD = presContext->AppUnitsPerDevPixel();
          int32_t scrollAPD = f->PresContext()->AppUnitsPerDevPixel();
          scrollAmount = scrollAmount.ConvertAppUnits(scrollAPD, APD);
          if (extra > scrollAmount.height) {
            extra = scrollAmount.height;
          }
        }
      }
    }

    aTargetPt.x = presContext->AppUnitsToDevPixels(frameOrigin.x);
    aTargetPt.y = presContext->AppUnitsToDevPixels(
                    frameOrigin.y + extra + extraTreeY);
  }

  NS_IF_ADDREF(*aTargetToUse = focusedContent);
}

bool
PresShell::ShouldIgnoreInvalidation()
{
  return mPaintingSuppressed || !mIsActive;
}

void
PresShell::WillPaint()
{
  nsRootPresContext* rootPresContext = mPresContext->GetRootPresContext();
  if (!rootPresContext) {
    
    
    
    return;
  }

  
  
  if (mPaintingSuppressed || !mIsActive || !IsVisible()) {
    return;
  }

  rootPresContext->FlushWillPaintObservers();
  if (mIsDestroying)
    return;

  
  
  
  
  FlushPendingNotifications(ChangesToFlush(Flush_InterruptibleLayout, false));
}

void
PresShell::WillPaintWindow()
{
  nsRootPresContext* rootPresContext = mPresContext->GetRootPresContext();
  if (rootPresContext != mPresContext) {
    
    
    return;
  }

#ifndef XP_MACOSX
  rootPresContext->ApplyPluginGeometryUpdates();
#endif
}

void
PresShell::DidPaintWindow()
{
  nsRootPresContext* rootPresContext = mPresContext->GetRootPresContext();
  if (rootPresContext != mPresContext) {
    
    
    return;
  }

  if (nsContentUtils::XPConnect()) {
    nsContentUtils::XPConnect()->NotifyDidPaint();
  }
}

bool
PresShell::IsVisible()
{
  if (!mViewManager)
    return false;

  nsView* view = mViewManager->GetRootView();
  if (!view)
    return true;

  
  view = view->GetParent();
  if (!view)
    return true;
  
  
  view = view->GetParent();
  if (!view)
    return true;

  nsIFrame* frame = view->GetFrame();
  if (!frame)
    return true;

  return frame->IsVisibleConsideringAncestors(nsIFrame::VISIBILITY_CROSS_CHROME_CONTENT_BOUNDARY);
}

nsresult
PresShell::GetAgentStyleSheets(nsCOMArray<nsIStyleSheet>& aSheets)
{
  aSheets.Clear();
  int32_t sheetCount = mStyleSet->SheetCount(nsStyleSet::eAgentSheet);

  for (int32_t i = 0; i < sheetCount; ++i) {
    nsIStyleSheet *sheet = mStyleSet->StyleSheetAt(nsStyleSet::eAgentSheet, i);
    if (!aSheets.AppendObject(sheet))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

nsresult
PresShell::SetAgentStyleSheets(const nsCOMArray<nsIStyleSheet>& aSheets)
{
  return mStyleSet->ReplaceSheets(nsStyleSet::eAgentSheet, aSheets);
}

nsresult
PresShell::AddOverrideStyleSheet(nsIStyleSheet *aSheet)
{
  return mStyleSet->PrependStyleSheet(nsStyleSet::eOverrideSheet, aSheet);
}

nsresult
PresShell::RemoveOverrideStyleSheet(nsIStyleSheet *aSheet)
{
  return mStyleSet->RemoveStyleSheet(nsStyleSet::eOverrideSheet, aSheet);
}

static void
FreezeElement(nsIContent *aContent, void * )
{
  nsCOMPtr<nsIObjectLoadingContent> olc(do_QueryInterface(aContent));
  if (olc) {
    olc->StopPluginInstance();
  }
}

static bool
FreezeSubDocument(nsIDocument *aDocument, void *aData)
{
  nsIPresShell *shell = aDocument->GetShell();
  if (shell)
    shell->Freeze();

  return true;
}

void
PresShell::Freeze()
{
  MaybeReleaseCapturingContent();

  mDocument->EnumerateFreezableElements(FreezeElement, nullptr);

  if (mCaret) {
    mCaret->SetCaretVisible(false);
  }

  mPaintingSuppressed = true;

  if (mDocument) {
    mDocument->EnumerateSubDocuments(FreezeSubDocument, nullptr);
  }

  nsPresContext* presContext = GetPresContext();
  if (presContext &&
      presContext->RefreshDriver()->PresContext() == presContext) {
    presContext->RefreshDriver()->Freeze();
  }

  mFrozen = true;
  if (mDocument) {
    UpdateImageLockingState();
  }
}

void
PresShell::FireOrClearDelayedEvents(bool aFireEvents)
{
  mNoDelayedMouseEvents = false;
  mNoDelayedKeyEvents = false;
  if (!aFireEvents) {
    mDelayedEvents.Clear();
    return;
  }

  if (mDocument) {
    nsCOMPtr<nsIDocument> doc = mDocument;
    while (!mIsDestroying && mDelayedEvents.Length() &&
           !doc->EventHandlingSuppressed()) {
      nsAutoPtr<nsDelayedEvent> ev(mDelayedEvents[0].forget());
      mDelayedEvents.RemoveElementAt(0);
      ev->Dispatch(this);
    }
    if (!doc->EventHandlingSuppressed()) {
      mDelayedEvents.Clear();
    }
  }
}

static void
ThawElement(nsIContent *aContent, void *aShell)
{
  nsCOMPtr<nsIObjectLoadingContent> olc(do_QueryInterface(aContent));
  if (olc) {
    olc->AsyncStartPluginInstance();
  }
}

static bool
ThawSubDocument(nsIDocument *aDocument, void *aData)
{
  nsIPresShell *shell = aDocument->GetShell();
  if (shell)
    shell->Thaw();

  return true;
}

void
PresShell::Thaw()
{
  nsPresContext* presContext = GetPresContext();
  if (presContext &&
      presContext->RefreshDriver()->PresContext() == presContext) {
    presContext->RefreshDriver()->Thaw();
  }

  mDocument->EnumerateFreezableElements(ThawElement, this);

  if (mDocument)
    mDocument->EnumerateSubDocuments(ThawSubDocument, nullptr);

  
  
  QueryIsActive();

  
  mFrozen = false;
  UpdateImageLockingState();

  UnsuppressPainting();
}





void
PresShell::MaybeScheduleReflow()
{
  ASSERT_REFLOW_SCHEDULED_STATE();
  if (mReflowScheduled || mIsDestroying || mIsReflowing ||
      mDirtyRoots.Length() == 0)
    return;

  if (!mPresContext->HasPendingInterrupt() || !ScheduleReflowOffTimer()) {
    ScheduleReflow();
  }

  ASSERT_REFLOW_SCHEDULED_STATE();
}

void
PresShell::ScheduleReflow()
{
  NS_PRECONDITION(!mReflowScheduled, "Why are we trying to schedule a reflow?");
  ASSERT_REFLOW_SCHEDULED_STATE();

  if (GetPresContext()->RefreshDriver()->AddLayoutFlushObserver(this)) {
    mReflowScheduled = true;
  }

  ASSERT_REFLOW_SCHEDULED_STATE();
}

nsresult
PresShell::DidCauseReflow()
{
  NS_ASSERTION(mChangeNestCount != 0, "Unexpected call to DidCauseReflow()");
  --mChangeNestCount;
  nsContentUtils::RemoveScriptBlocker();

  return NS_OK;
}

void
PresShell::WillDoReflow()
{
  
  
  if (mCaret) {
    mCaret->InvalidateOutsideCaret();
  }

  mPresContext->FlushUserFontSet();

  mFrameConstructor->BeginUpdate();
}

void
PresShell::DidDoReflow(bool aInterruptible)
{
  mFrameConstructor->EndUpdate();
  
  HandlePostedReflowCallbacks(aInterruptible);
  if (sSynthMouseMove) {
    SynthesizeMouseMove(false);
  }
  if (mCaret) {
    
    
    mCaret->InvalidateOutsideCaret();
    mCaret->UpdateCaretPosition();
  }
}

static PLDHashOperator
MarkFramesDirtyToRoot(nsPtrHashKey<nsIFrame>* p, void* closure)
{
  nsIFrame* target = static_cast<nsIFrame*>(closure);
  for (nsIFrame* f = p->GetKey(); f && !NS_SUBTREE_DIRTY(f);
       f = f->GetParent()) {
    f->AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);

    if (f == target) {
      break;
    }
  }

  return PL_DHASH_NEXT;
}

void
PresShell::sReflowContinueCallback(nsITimer* aTimer, void* aPresShell)
{
  nsRefPtr<PresShell> self = static_cast<PresShell*>(aPresShell);

  NS_PRECONDITION(aTimer == self->mReflowContinueTimer, "Unexpected timer");
  self->mReflowContinueTimer = nullptr;
  self->ScheduleReflow();
}

bool
PresShell::ScheduleReflowOffTimer()
{
  NS_PRECONDITION(!mReflowScheduled, "Shouldn't get here");
  ASSERT_REFLOW_SCHEDULED_STATE();

  if (!mReflowContinueTimer) {
    mReflowContinueTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (!mReflowContinueTimer ||
        NS_FAILED(mReflowContinueTimer->
                    InitWithFuncCallback(sReflowContinueCallback, this, 30,
                                         nsITimer::TYPE_ONE_SHOT))) {
      return false;
    }
  }
  return true;
}

bool
PresShell::DoReflow(nsIFrame* target, bool aInterruptible)
{
  if (mIsZombie) {
    return true;
  }

  target->SchedulePaint();
  nsIFrame *parent = nsLayoutUtils::GetCrossDocParentFrame(target);
  while (parent) {
    nsSVGEffects::InvalidateDirectRenderingObservers(parent);
    parent = nsLayoutUtils::GetCrossDocParentFrame(parent);
  }

  nsAutoCString docURL("N/A");
  nsIURI *uri = mDocument->GetDocumentURI();
  if (uri)
    uri->GetSpec(docURL);
  SAMPLE_LABEL_PRINTF("layout", "DoReflow", "(%s)", docURL.get());

  if (mReflowContinueTimer) {
    mReflowContinueTimer->Cancel();
    mReflowContinueTimer = nullptr;
  }

  nsIFrame* rootFrame = mFrameConstructor->GetRootFrame();

  nsRefPtr<nsRenderingContext> rcx = GetReferenceRenderingContext();
  if (!rcx) {
    NS_NOTREACHED("CreateRenderingContext failure");
    return false;
  }

#ifdef DEBUG
  mCurrentReflowRoot = target;
#endif

  target->WillReflow(mPresContext);

  
  
  
  nsSize size;
  if (target == rootFrame) {
     size = mPresContext->GetVisibleArea().Size();
  } else {
     size = target->GetSize();
  }

  NS_ASSERTION(!target->GetNextInFlow() && !target->GetPrevInFlow(),
               "reflow roots should never split");

  
  
  nsSize reflowSize(size.width, NS_UNCONSTRAINEDSIZE);
  nsHTMLReflowState reflowState(mPresContext, target, rcx, reflowSize);

  if (rootFrame == target) {
    
    
    
    
    
    
    
    bool hasUnconstrainedHeight = size.height == NS_UNCONSTRAINEDSIZE;

    if (hasUnconstrainedHeight || mLastRootReflowHadUnconstrainedHeight) {
      reflowState.mFlags.mVResize = true;
    }

    mLastRootReflowHadUnconstrainedHeight = hasUnconstrainedHeight;
  }

  
  NS_ASSERTION(reflowState.mComputedMargin == nsMargin(0, 0, 0, 0),
               "reflow state should not set margin for reflow roots");
  if (size.height != NS_UNCONSTRAINEDSIZE) {
    nscoord computedHeight =
      size.height - reflowState.mComputedBorderPadding.TopBottom();
    computedHeight = std::max(computedHeight, 0);
    reflowState.SetComputedHeight(computedHeight);
  }
  NS_ASSERTION(reflowState.ComputedWidth() ==
                 size.width -
                   reflowState.mComputedBorderPadding.LeftRight(),
               "reflow state computed incorrect width");

  mPresContext->ReflowStarted(aInterruptible);
  mIsReflowing = true;

  nsReflowStatus status;
  nsHTMLReflowMetrics desiredSize;
  target->Reflow(mPresContext, desiredSize, reflowState, status);

  
  
  
  
  nsRect boundsRelativeToTarget = nsRect(0, 0, desiredSize.width, desiredSize.height);
  NS_ASSERTION((target == rootFrame && size.height == NS_UNCONSTRAINEDSIZE) ||
               (desiredSize.width == size.width &&
                desiredSize.height == size.height),
               "non-root frame's desired size changed during an "
               "incremental reflow");
  NS_ASSERTION(target == rootFrame ||
               desiredSize.VisualOverflow().IsEqualInterior(boundsRelativeToTarget),
               "non-root reflow roots must not have visible overflow");
  NS_ASSERTION(target == rootFrame ||
               desiredSize.ScrollableOverflow().IsEqualEdges(boundsRelativeToTarget),
               "non-root reflow roots must not have scrollable overflow");
  NS_ASSERTION(status == NS_FRAME_COMPLETE,
               "reflow roots should never split");

  target->SetSize(boundsRelativeToTarget.Size());

  
  
  
  
  nsContainerFrame::SyncFrameViewAfterReflow(mPresContext, target,
                                             target->GetView(),
                                             boundsRelativeToTarget);
  nsContainerFrame::SyncWindowProperties(mPresContext, target,
                                         target->GetView(), rcx);

  target->DidReflow(mPresContext, nullptr, nsDidReflowStatus::FINISHED);
  if (target == rootFrame && size.height == NS_UNCONSTRAINEDSIZE) {
    mPresContext->SetVisibleArea(boundsRelativeToTarget);
  }

#ifdef DEBUG
  mCurrentReflowRoot = nullptr;
#endif

  NS_ASSERTION(mPresContext->HasPendingInterrupt() ||
               mFramesToDirty.Count() == 0,
               "Why do we need to dirty anything if not interrupted?");

  mIsReflowing = false;
  bool interrupted = mPresContext->HasPendingInterrupt();
  if (interrupted) {
    
    mFramesToDirty.EnumerateEntries(&MarkFramesDirtyToRoot, target);
    NS_ASSERTION(NS_SUBTREE_DIRTY(target), "Why is the target not dirty?");
    mDirtyRoots.AppendElement(target);
    mDocument->SetNeedLayoutFlush();

    
    
#ifdef NOISY_INTERRUPTIBLE_REFLOW
    printf("mFramesToDirty.Count() == %u\n", mFramesToDirty.Count());
#endif 
    mFramesToDirty.Clear();

    
    
    
    mSuppressInterruptibleReflows = true;
    MaybeScheduleReflow();
  }

  return !interrupted;
}

#ifdef DEBUG
void
PresShell::DoVerifyReflow()
{
  if (GetVerifyReflowEnable()) {
    
    
    nsView* rootView = mViewManager->GetRootView();
    mViewManager->InvalidateView(rootView);

    FlushPendingNotifications(Flush_Layout);
    mInVerifyReflow = true;
    bool ok = VerifyIncrementalReflow();
    mInVerifyReflow = false;
    if (VERIFY_REFLOW_ALL & gVerifyReflowFlags) {
      printf("ProcessReflowCommands: finished (%s)\n",
             ok ? "ok" : "failed");
    }

    if (!mDirtyRoots.IsEmpty()) {
      printf("XXX yikes! reflow commands queued during verify-reflow\n");
    }
  }
}
#endif

bool
PresShell::ProcessReflowCommands(bool aInterruptible)
{
  if (mDirtyRoots.IsEmpty() && !mShouldUnsuppressPainting) {
    
    return true;
  }

  mozilla::TimeStamp timerStart = mozilla::TimeStamp::Now();
  bool interrupted = false;
  if (!mDirtyRoots.IsEmpty()) {

#ifdef DEBUG
    if (VERIFY_REFLOW_DUMP_COMMANDS & gVerifyReflowFlags) {
      printf("ProcessReflowCommands: begin incremental reflow\n");
    }
#endif

    
    const PRIntervalTime deadline = aInterruptible
        ? PR_IntervalNow() + PR_MicrosecondsToInterval(gMaxRCProcessingTime)
        : (PRIntervalTime)0;

    
    {
      nsAutoScriptBlocker scriptBlocker;
      WillDoReflow();
      AUTO_LAYOUT_PHASE_ENTRY_POINT(GetPresContext(), Reflow);
      nsViewManager::AutoDisableRefresh refreshBlocker(mViewManager);

      do {
        
        int32_t idx = mDirtyRoots.Length() - 1;
        nsIFrame *target = mDirtyRoots[idx];
        mDirtyRoots.RemoveElementAt(idx);

        if (!NS_SUBTREE_DIRTY(target)) {
          
          
          
          continue;
        }

        interrupted = !DoReflow(target, aInterruptible);

        
        
      } while (!interrupted && !mDirtyRoots.IsEmpty() &&
               (!aInterruptible || PR_IntervalNow() < deadline));

      interrupted = !mDirtyRoots.IsEmpty();
    }

    
    if (!mIsDestroying) {
      DidDoReflow(aInterruptible);
    }

    
    if (!mIsDestroying) {
#ifdef DEBUG
      if (VERIFY_REFLOW_DUMP_COMMANDS & gVerifyReflowFlags) {
        printf("\nPresShell::ProcessReflowCommands() finished: this=%p\n",
               (void*)this);
      }
      DoVerifyReflow();
#endif

      
      
      
      
      
      if (!mDirtyRoots.IsEmpty()) {
        MaybeScheduleReflow();
        
        mDocument->SetNeedLayoutFlush();
      }
    }
  }

  if (!mIsDestroying && mShouldUnsuppressPainting &&
      mDirtyRoots.IsEmpty()) {
    
    
    
    
    mShouldUnsuppressPainting = false;
    UnsuppressAndInvalidate();
  }

  if (mDocument->GetRootElement()) {
    Telemetry::ID id;
    if (mDocument->GetRootElement()->IsXUL()) {
      id = mIsActive
        ? Telemetry::XUL_FOREGROUND_REFLOW_MS
        : Telemetry::XUL_BACKGROUND_REFLOW_MS;
    } else {
      id = mIsActive
        ? Telemetry::HTML_FOREGROUND_REFLOW_MS
        : Telemetry::HTML_BACKGROUND_REFLOW_MS;
    }
    Telemetry::AccumulateTimeDelta(id, timerStart);
  }

  return !interrupted;
}

void
PresShell::WindowSizeMoveDone()
{
  if (mPresContext) {
    nsEventStateManager::ClearGlobalActiveContent(nullptr);
    ClearMouseCapture(nullptr);
  }
}

#ifdef MOZ_XUL







typedef bool (* frameWalkerFn)(nsIFrame *aFrame, void *aClosure);
   
static bool
ReResolveMenusAndTrees(nsIFrame *aFrame, void *aClosure)
{
  
  
  nsTreeBodyFrame *treeBody = do_QueryFrame(aFrame);
  if (treeBody)
    treeBody->ClearStyleAndImageCaches();

  
  
  
  
  nsMenuFrame* menu = do_QueryFrame(aFrame);
  if (menu)
    menu->CloseMenu(true);
  return true;
}

static bool
ReframeImageBoxes(nsIFrame *aFrame, void *aClosure)
{
  nsStyleChangeList *list = static_cast<nsStyleChangeList*>(aClosure);
  if (aFrame->GetType() == nsGkAtoms::imageBoxFrame) {
    list->AppendChange(aFrame, aFrame->GetContent(),
                       NS_STYLE_HINT_FRAMECHANGE);
    return false; 
  }
  return true; 
}

static void
WalkFramesThroughPlaceholders(nsPresContext *aPresContext, nsIFrame *aFrame,
                              frameWalkerFn aFunc, void *aClosure)
{
  bool walkChildren = (*aFunc)(aFrame, aClosure);
  if (!walkChildren)
    return;

  nsIFrame::ChildListIterator lists(aFrame);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsIFrame* child = childFrames.get();
      if (!(child->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
        
        
        WalkFramesThroughPlaceholders(aPresContext,
                                      nsPlaceholderFrame::GetRealFrameFor(child),
                                      aFunc, aClosure);
      }
    }
  }
}
#endif

NS_IMETHODIMP
PresShell::Observe(nsISupports* aSubject, 
                   const char* aTopic,
                   const PRUnichar* aData)
{
#ifdef MOZ_XUL
  if (!nsCRT::strcmp(aTopic, "chrome-flush-skin-caches")) {
    nsIFrame *rootFrame = mFrameConstructor->GetRootFrame();
    
    
    if (rootFrame) {
      NS_ASSERTION(mViewManager, "View manager must exist");

      nsWeakFrame weakRoot(rootFrame);
      
      
      mDocument->FlushPendingNotifications(Flush_ContentAndNotify);

      if (weakRoot.IsAlive()) {
        WalkFramesThroughPlaceholders(mPresContext, rootFrame,
                                      &ReResolveMenusAndTrees, nullptr);

        
        
        nsStyleChangeList changeList;
        WalkFramesThroughPlaceholders(mPresContext, rootFrame,
                                      ReframeImageBoxes, &changeList);
        
        
        {
          nsAutoScriptBlocker scriptBlocker;
          ++mChangeNestCount;
          css::OverflowChangedTracker tracker;
          mFrameConstructor->ProcessRestyledFrames(changeList, tracker);
          tracker.Flush();
          --mChangeNestCount;
        }
      }
    }
    return NS_OK;
  }
#endif

  if (!nsCRT::strcmp(aTopic, "agent-sheet-added") && mStyleSet) {
    AddAgentSheet(aSubject);
    return NS_OK;
  }

  if (!nsCRT::strcmp(aTopic, "user-sheet-added") && mStyleSet) {
    AddUserSheet(aSubject);
    return NS_OK;
  }

  if (!nsCRT::strcmp(aTopic, "author-sheet-added") && mStyleSet) {
    AddAuthorSheet(aSubject);
    return NS_OK;
  }

  if (!nsCRT::strcmp(aTopic, "agent-sheet-removed") && mStyleSet) {
    RemoveSheet(nsStyleSet::eAgentSheet, aSubject);
    return NS_OK;
  }

  if (!nsCRT::strcmp(aTopic, "user-sheet-removed") && mStyleSet) {
    RemoveSheet(nsStyleSet::eUserSheet, aSubject);
    return NS_OK;
  }

  if (!nsCRT::strcmp(aTopic, "author-sheet-removed") && mStyleSet) {
    RemoveSheet(nsStyleSet::eDocSheet, aSubject);
    return NS_OK;
  }

  NS_WARNING("unrecognized topic in PresShell::Observe");
  return NS_ERROR_FAILURE;
}

bool
nsIPresShell::AddRefreshObserverInternal(nsARefreshObserver* aObserver,
                                         mozFlushType aFlushType)
{
  nsPresContext* presContext = GetPresContext();
  return presContext ? presContext->RefreshDriver()->
    AddRefreshObserver(aObserver, aFlushType) : false;
}

 bool
nsIPresShell::AddRefreshObserverExternal(nsARefreshObserver* aObserver,
                                         mozFlushType aFlushType)
{
  return AddRefreshObserverInternal(aObserver, aFlushType);
}

bool
nsIPresShell::RemoveRefreshObserverInternal(nsARefreshObserver* aObserver,
                                            mozFlushType aFlushType)
{
  nsPresContext* presContext = GetPresContext();
  return presContext ? presContext->RefreshDriver()->
    RemoveRefreshObserver(aObserver, aFlushType) : false;
}

 bool
nsIPresShell::RemoveRefreshObserverExternal(nsARefreshObserver* aObserver,
                                            mozFlushType aFlushType)
{
  return RemoveRefreshObserverInternal(aObserver, aFlushType);
}







#ifdef DEBUG
#include "nsIURL.h"
#include "nsILinkHandler.h"

static void
LogVerifyMessage(nsIFrame* k1, nsIFrame* k2, const char* aMsg)
{
  nsAutoString n1, n2;
  if (k1) {
    k1->GetFrameName(n1);
  } else {
    n1.Assign(NS_LITERAL_STRING("(null)"));
  }

  if (k2) {
    k2->GetFrameName(n2);
  } else {
    n2.Assign(NS_LITERAL_STRING("(null)"));
  }

  printf("verifyreflow: %s %p != %s %p  %s\n",
         NS_LossyConvertUTF16toASCII(n1).get(), (void*)k1,
         NS_LossyConvertUTF16toASCII(n2).get(), (void*)k2, aMsg);
}

static void
LogVerifyMessage(nsIFrame* k1, nsIFrame* k2, const char* aMsg,
                 const nsRect& r1, const nsRect& r2)
{
  printf("VerifyReflow Error:\n");
  nsAutoString name;

  if (k1) {
    k1->GetFrameName(name);
    printf("  %s %p ", NS_LossyConvertUTF16toASCII(name).get(), (void*)k1);
  }
  printf("{%d, %d, %d, %d} != \n", r1.x, r1.y, r1.width, r1.height);

  if (k2) {
    k2->GetFrameName(name);
    printf("  %s %p ", NS_LossyConvertUTF16toASCII(name).get(), (void*)k2);
  }
  printf("{%d, %d, %d, %d}\n  %s\n",
         r2.x, r2.y, r2.width, r2.height, aMsg);
}

static void
LogVerifyMessage(nsIFrame* k1, nsIFrame* k2, const char* aMsg,
                 const nsIntRect& r1, const nsIntRect& r2)
{
  printf("VerifyReflow Error:\n");
  nsAutoString name;

  if (k1) {
    k1->GetFrameName(name);
    printf("  %s %p ", NS_LossyConvertUTF16toASCII(name).get(), (void*)k1);
  }
  printf("{%d, %d, %d, %d} != \n", r1.x, r1.y, r1.width, r1.height);

  if (k2) {
    k2->GetFrameName(name);
    printf("  %s %p ", NS_LossyConvertUTF16toASCII(name).get(), (void*)k2);
  }
  printf("{%d, %d, %d, %d}\n  %s\n",
         r2.x, r2.y, r2.width, r2.height, aMsg);
}

static bool
CompareTrees(nsPresContext* aFirstPresContext, nsIFrame* aFirstFrame, 
             nsPresContext* aSecondPresContext, nsIFrame* aSecondFrame)
{
  if (!aFirstPresContext || !aFirstFrame || !aSecondPresContext || !aSecondFrame)
    return true;
  
  
  
  
  bool ok = true;
  nsIFrame::ChildListIterator lists1(aFirstFrame);
  nsIFrame::ChildListIterator lists2(aSecondFrame);
  do {
    const nsFrameList& kids1 = !lists1.IsDone() ? lists1.CurrentList() : nsFrameList();
    const nsFrameList& kids2 = !lists2.IsDone() ? lists2.CurrentList() : nsFrameList();
    int32_t l1 = kids1.GetLength();
    int32_t l2 = kids2.GetLength();;
    if (l1 != l2) {
      ok = false;
      LogVerifyMessage(kids1.FirstChild(), kids2.FirstChild(),
                       "child counts don't match: ");
      printf("%d != %d\n", l1, l2);
      if (0 == (VERIFY_REFLOW_ALL & gVerifyReflowFlags)) {
        break;
      }
    }

    nsIntRect r1, r2;
    nsView* v1, *v2;
    for (nsFrameList::Enumerator e1(kids1), e2(kids2);
         ;
         e1.Next(), e2.Next()) {
      nsIFrame* k1 = e1.get();
      nsIFrame* k2 = e2.get();
      if (((nullptr == k1) && (nullptr != k2)) ||
          ((nullptr != k1) && (nullptr == k2))) {
        ok = false;
        LogVerifyMessage(k1, k2, "child lists are different\n");
        break;
      }
      else if (nullptr != k1) {
        
        if (!k1->GetRect().IsEqualInterior(k2->GetRect())) {
          ok = false;
          LogVerifyMessage(k1, k2, "(frame rects)", k1->GetRect(), k2->GetRect());
        }

        
        
        
        
        v1 = k1->GetView();
        v2 = k2->GetView();
        if (((nullptr == v1) && (nullptr != v2)) ||
            ((nullptr != v1) && (nullptr == v2))) {
          ok = false;
          LogVerifyMessage(k1, k2, "child views are not matched\n");
        }
        else if (nullptr != v1) {
          if (!v1->GetBounds().IsEqualInterior(v2->GetBounds())) {
            LogVerifyMessage(k1, k2, "(view rects)", v1->GetBounds(), v2->GetBounds());
          }

          nsIWidget* w1 = v1->GetWidget();
          nsIWidget* w2 = v2->GetWidget();
          if (((nullptr == w1) && (nullptr != w2)) ||
              ((nullptr != w1) && (nullptr == w2))) {
            ok = false;
            LogVerifyMessage(k1, k2, "child widgets are not matched\n");
          }
          else if (nullptr != w1) {
            w1->GetBounds(r1);
            w2->GetBounds(r2);
            if (!r1.IsEqualEdges(r2)) {
              LogVerifyMessage(k1, k2, "(widget rects)", r1, r2);
            }
          }
        }
        if (!ok && (0 == (VERIFY_REFLOW_ALL & gVerifyReflowFlags))) {
          break;
        }

        

        
        if (!CompareTrees(aFirstPresContext, k1, aSecondPresContext, k2)) {
          ok = false;
          if (0 == (VERIFY_REFLOW_ALL & gVerifyReflowFlags)) {
            break;
          }
        }
      }
      else {
        break;
      }
    }
    if (!ok && (0 == (VERIFY_REFLOW_ALL & gVerifyReflowFlags))) {
      break;
    }

    lists1.Next();
    lists2.Next();
    if (lists1.IsDone() != lists2.IsDone() ||
        (!lists1.IsDone() && lists1.CurrentID() != lists2.CurrentID())) {
      if (0 == (VERIFY_REFLOW_ALL & gVerifyReflowFlags)) {
        ok = false;
      }
      LogVerifyMessage(kids1.FirstChild(), kids2.FirstChild(),
                       "child list names are not matched: ");
      fprintf(stdout, "%s != %s\n",
              !lists1.IsDone() ? mozilla::layout::ChildListName(lists1.CurrentID()) : "(null)",
              !lists2.IsDone() ? mozilla::layout::ChildListName(lists2.CurrentID()) : "(null)");
      break;
    }
  } while (ok && !lists1.IsDone());

  return ok;
}
#endif

#if 0
static nsIFrame*
FindTopFrame(nsIFrame* aRoot)
{
  if (aRoot) {
    nsIContent* content = aRoot->GetContent();
    if (content) {
      nsIAtom* tag;
      content->GetTag(tag);
      if (nullptr != tag) {
        NS_RELEASE(tag);
        return aRoot;
      }
    }

    
    nsIFrame* kid = aRoot->GetFirstPrincipalChild();
    while (nullptr != kid) {
      nsIFrame* result = FindTopFrame(kid);
      if (nullptr != result) {
        return result;
      }
      kid = kid->GetNextSibling();
    }
  }
  return nullptr;
}
#endif


#ifdef DEBUG

nsStyleSet*
PresShell::CloneStyleSet(nsStyleSet* aSet)
{
  nsStyleSet *clone = new nsStyleSet();

  int32_t i, n = aSet->SheetCount(nsStyleSet::eOverrideSheet);
  for (i = 0; i < n; i++) {
    nsIStyleSheet* ss = aSet->StyleSheetAt(nsStyleSet::eOverrideSheet, i);
    if (ss)
      clone->AppendStyleSheet(nsStyleSet::eOverrideSheet, ss);
  }

  
#if 0
  n = aSet->SheetCount(nsStyleSet::eDocSheet);
  for (i = 0; i < n; i++) {
    nsIStyleSheet* ss = aSet->StyleSheetAt(nsStyleSet::eDocSheet, i);
    if (ss)
      clone->AddDocStyleSheet(ss, mDocument);
  }
#endif

  n = aSet->SheetCount(nsStyleSet::eUserSheet);
  for (i = 0; i < n; i++) {
    nsIStyleSheet* ss = aSet->StyleSheetAt(nsStyleSet::eUserSheet, i);
    if (ss)
      clone->AppendStyleSheet(nsStyleSet::eUserSheet, ss);
  }

  n = aSet->SheetCount(nsStyleSet::eAgentSheet);
  for (i = 0; i < n; i++) {
    nsIStyleSheet* ss = aSet->StyleSheetAt(nsStyleSet::eAgentSheet, i);
    if (ss)
      clone->AppendStyleSheet(nsStyleSet::eAgentSheet, ss);
  }
  return clone;
}

#ifdef DEBUG_Eli
static nsresult
DumpToPNG(nsIPresShell* shell, nsAString& name) {
  int32_t width=1000, height=1000;
  nsRect r(0, 0, shell->GetPresContext()->DevPixelsToAppUnits(width),
                 shell->GetPresContext()->DevPixelsToAppUnits(height));

  nsRefPtr<gfxImageSurface> imgSurface =
     new gfxImageSurface(gfxIntSize(width, height),
                         gfxImageSurface::ImageFormatARGB32);

  nsRefPtr<gfxContext> imgContext = new gfxContext(imgSurface);

  nsRefPtr<gfxASurface> surface = 
    gfxPlatform::GetPlatform()->
    CreateOffscreenSurface(gfxIntSize(width, height),
      gfxASurface::ContentFromFormat(gfxASurface::ImageFormatARGB32));
  NS_ENSURE_TRUE(surface, NS_ERROR_OUT_OF_MEMORY);

  nsRefPtr<gfxContext> context = new gfxContext(surface);

  shell->RenderDocument(r, 0, NS_RGB(255, 255, 0), context);

  imgContext->DrawSurface(surface, gfxSize(width, height));

  nsCOMPtr<imgIEncoder> encoder = do_CreateInstance("@mozilla.org/image/encoder;2?type=image/png");
  NS_ENSURE_TRUE(encoder, NS_ERROR_FAILURE);
  encoder->InitFromData(imgSurface->Data(), imgSurface->Stride() * height,
                        width, height, imgSurface->Stride(),
                        imgIEncoder::INPUT_FORMAT_HOSTARGB, EmptyString());

  
  nsCOMPtr<nsIFile> file = do_CreateInstance("@mozilla.org/file/local;1");
  NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);
  rv = file->InitWithPath(name);
  NS_ENSURE_SUCCESS(rv, rv);

  uint64_t length64;
  rv = encoder->Available(&length64);
  NS_ENSURE_SUCCESS(rv, rv);
  if (length64 > UINT32_MAX)
    return NS_ERROR_FILE_TOO_BIG;

  uint32_t length = (uint32_t)length64;

  nsCOMPtr<nsIOutputStream> outputStream;
  rv = NS_NewLocalFileOutputStream(getter_AddRefs(outputStream), file);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIOutputStream> bufferedOutputStream;
  rv = NS_NewBufferedOutputStream(getter_AddRefs(bufferedOutputStream),
                                  outputStream, length);
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t numWritten;
  rv = bufferedOutputStream->WriteFrom(encoder, length, &numWritten);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
#endif



bool
PresShell::VerifyIncrementalReflow()
{
   if (VERIFY_REFLOW_NOISY & gVerifyReflowFlags) {
     printf("Building Verification Tree...\n");
   }

  
  nsRefPtr<nsPresContext> cx =
       new nsRootPresContext(mDocument, mPresContext->IsPaginated() ?
                                        nsPresContext::eContext_PrintPreview :
                                        nsPresContext::eContext_Galley);
  NS_ENSURE_TRUE(cx, false);

  nsDeviceContext *dc = mPresContext->DeviceContext();
  nsresult rv = cx->Init(dc);
  NS_ENSURE_SUCCESS(rv, false);

  
  nsView* rootView = mViewManager->GetRootView();
  NS_ENSURE_TRUE(rootView->HasWidget(), false);
  nsIWidget* parentWidget = rootView->GetWidget();

  
  nsRefPtr<nsViewManager> vm = new nsViewManager();
  NS_ENSURE_TRUE(vm, false);
  rv = vm->Init(dc);
  NS_ENSURE_SUCCESS(rv, false);

  
  
  nsRect tbounds = mPresContext->GetVisibleArea();
  nsView* view = vm->CreateView(tbounds, nullptr);
  NS_ENSURE_TRUE(view, false);

  
  rv = view->CreateWidgetForParent(parentWidget, nullptr, true);
  NS_ENSURE_SUCCESS(rv, false);

  
  vm->SetRootView(view);

  
  
  nsRect r = mPresContext->GetVisibleArea();
  cx->SetVisibleArea(r);

  
  
  nsAutoPtr<nsStyleSet> newSet(CloneStyleSet(mStyleSet));
  nsCOMPtr<nsIPresShell> sh;
  rv = mDocument->CreateShell(cx, vm, newSet, getter_AddRefs(sh));
  NS_ENSURE_SUCCESS(rv, false);
  newSet.forget();
  
  sh->SetVerifyReflowEnable(false); 
  vm->SetPresShell(sh);
  {
    nsAutoCauseReflowNotifier crNotifier(this);
    sh->Initialize(r.width, r.height);
  }
  mDocument->BindingManager()->ProcessAttachedQueue();
  sh->FlushPendingNotifications(Flush_Layout);
  sh->SetVerifyReflowEnable(true);  
  
  
  ((PresShell*)sh.get())->mPaintingSuppressed = false;
  if (VERIFY_REFLOW_NOISY & gVerifyReflowFlags) {
     printf("Verification Tree built, comparing...\n");
  }

  
  
  nsIFrame* root1 = mFrameConstructor->GetRootFrame();
  nsIFrame* root2 = sh->GetRootFrame();
  bool ok = CompareTrees(mPresContext, root1, cx, root2);
  if (!ok && (VERIFY_REFLOW_NOISY & gVerifyReflowFlags)) {
    printf("Verify reflow failed, primary tree:\n");
    root1->List(stdout, 0);
    printf("Verification tree:\n");
    root2->List(stdout, 0);
  }

#ifdef DEBUG_Eli
  
  
  if (!ok) {
    nsString stra;
    static int num = 0;
    stra.AppendLiteral("C:\\mozilla\\mozilla\\debug\\filea");
    stra.AppendInt(num);
    stra.AppendLiteral(".png");
    DumpToPNG(sh, stra);
    nsString strb;
    strb.AppendLiteral("C:\\mozilla\\mozilla\\debug\\fileb");
    strb.AppendInt(num);
    strb.AppendLiteral(".png");
    DumpToPNG(this, strb);
    ++num;
  }
#endif

  sh->EndObservingDocument();
  sh->Destroy();
  if (VERIFY_REFLOW_NOISY & gVerifyReflowFlags) {
    printf("Finished Verifying Reflow...\n");
  }

  return ok;
}


void
PresShell::ListStyleContexts(nsIFrame *aRootFrame, FILE *out, int32_t aIndent)
{
  nsStyleContext *sc = aRootFrame->StyleContext();
  if (sc)
    sc->List(out, aIndent);
}

void
PresShell::ListStyleSheets(FILE *out, int32_t aIndent)
{
  int32_t sheetCount = mStyleSet->SheetCount(nsStyleSet::eDocSheet);
  for (int32_t i = 0; i < sheetCount; ++i) {
    mStyleSet->StyleSheetAt(nsStyleSet::eDocSheet, i)->List(out, aIndent);
    fputs("\n", out);
  }
}

void
PresShell::VerifyStyleTree()
{
  VERIFY_STYLE_TREE;
}
#endif






#ifdef MOZ_REFLOW_PERF

void
PresShell::DumpReflows()
{
  if (mReflowCountMgr) {
    nsAutoCString uriStr;
    if (mDocument) {
      nsIURI *uri = mDocument->GetDocumentURI();
      if (uri) {
        uri->GetPath(uriStr);
      }
    }
    mReflowCountMgr->DisplayTotals(uriStr.get());
    mReflowCountMgr->DisplayHTMLTotals(uriStr.get());
    mReflowCountMgr->DisplayDiffsInTotals("Differences");
  }
}


void
PresShell::CountReflows(const char * aName, nsIFrame * aFrame)
{
  if (mReflowCountMgr) {
    mReflowCountMgr->Add(aName, aFrame);
  }
}


void
PresShell::PaintCount(const char * aName,
                      nsRenderingContext* aRenderingContext,
                      nsPresContext* aPresContext,
                      nsIFrame * aFrame,
                      const nsPoint& aOffset,
                      uint32_t aColor)
{
  if (mReflowCountMgr) {
    mReflowCountMgr->PaintCount(aName, aRenderingContext, aPresContext,
                                aFrame, aOffset, aColor);
  }
}


void
PresShell::SetPaintFrameCount(bool aPaintFrameCounts)
{
  if (mReflowCountMgr) {
    mReflowCountMgr->SetPaintFrameCounts(aPaintFrameCounts);
  }
}

bool
PresShell::IsPaintingFrameCounts()
{
  if (mReflowCountMgr)
    return mReflowCountMgr->IsPaintingFrameCounts();
  return false;
}






ReflowCounter::ReflowCounter(ReflowCountMgr * aMgr) :
  mMgr(aMgr)
{
  ClearTotals();
  SetTotalsCache();
}


ReflowCounter::~ReflowCounter()
{
  
}


void ReflowCounter::ClearTotals()
{
  mTotal = 0;
}


void ReflowCounter::SetTotalsCache()
{
  mCacheTotal = mTotal;
}


void ReflowCounter::CalcDiffInTotals()
{
  mCacheTotal = mTotal - mCacheTotal;
}


void ReflowCounter::DisplayTotals(const char * aStr)
{
  DisplayTotals(mTotal, aStr?aStr:"Totals");
}


void ReflowCounter::DisplayDiffTotals(const char * aStr)
{
  DisplayTotals(mCacheTotal, aStr?aStr:"Diff Totals");
}


void ReflowCounter::DisplayHTMLTotals(const char * aStr)
{
  DisplayHTMLTotals(mTotal, aStr?aStr:"Totals");
}


void ReflowCounter::DisplayTotals(uint32_t aTotal, const char * aTitle)
{
  
  if (aTotal == 0) {
    return;
  }
  ReflowCounter * gTots = (ReflowCounter *)mMgr->LookUp(kGrandTotalsStr);

  printf("%25s\t", aTitle);
  printf("%d\t", aTotal);
  if (gTots != this && aTotal > 0) {
    gTots->Add(aTotal);
  }
}


void ReflowCounter::DisplayHTMLTotals(uint32_t aTotal, const char * aTitle)
{
  if (aTotal == 0) {
    return;
  }

  ReflowCounter * gTots = (ReflowCounter *)mMgr->LookUp(kGrandTotalsStr);
  FILE * fd = mMgr->GetOutFile();
  if (!fd) {
    return;
  }

  fprintf(fd, "<tr><td><center>%s</center></td>", aTitle);
  fprintf(fd, "<td><center>%d</center></td></tr>\n", aTotal);

  if (gTots != this && aTotal > 0) {
    gTots->Add(aTotal);
  }
}





#define KEY_BUF_SIZE_FOR_PTR  24 // adequate char[] buffer to sprintf a pointer

ReflowCountMgr::ReflowCountMgr()
{
  mCounts = PL_NewHashTable(10, PL_HashString, PL_CompareStrings, 
                                PL_CompareValues, nullptr, nullptr);
  mIndiFrameCounts = PL_NewHashTable(10, PL_HashString, PL_CompareStrings, 
                                     PL_CompareValues, nullptr, nullptr);
  mCycledOnce              = false;
  mDumpFrameCounts         = false;
  mDumpFrameByFrameCounts  = false;
  mPaintFrameByFrameCounts = false;
}


ReflowCountMgr::~ReflowCountMgr()
{
  CleanUp();
}


ReflowCounter * ReflowCountMgr::LookUp(const char * aName)
{
  if (nullptr != mCounts) {
    ReflowCounter * counter = (ReflowCounter *)PL_HashTableLookup(mCounts, aName);
    return counter;
  }
  return nullptr;

}


void ReflowCountMgr::Add(const char * aName, nsIFrame * aFrame)
{
  NS_ASSERTION(aName != nullptr, "Name shouldn't be null!");

  if (mDumpFrameCounts && nullptr != mCounts) {
    ReflowCounter * counter = (ReflowCounter *)PL_HashTableLookup(mCounts, aName);
    if (counter == nullptr) {
      counter = new ReflowCounter(this);
      char * name = NS_strdup(aName);
      NS_ASSERTION(name != nullptr, "null ptr");
      PL_HashTableAdd(mCounts, name, counter);
    }
    counter->Add();
  }

  if ((mDumpFrameByFrameCounts || mPaintFrameByFrameCounts) && 
      nullptr != mIndiFrameCounts && 
      aFrame != nullptr) {
    char key[KEY_BUF_SIZE_FOR_PTR];
    sprintf(key, "%p", (void*)aFrame);
    IndiReflowCounter * counter = (IndiReflowCounter *)PL_HashTableLookup(mIndiFrameCounts, key);
    if (counter == nullptr) {
      counter = new IndiReflowCounter(this);
      counter->mFrame = aFrame;
      counter->mName.AssignASCII(aName);
      PL_HashTableAdd(mIndiFrameCounts, NS_strdup(key), counter);
    }
    
    if (counter != nullptr && counter->mName.EqualsASCII(aName)) {
      counter->mCount++;
      counter->mCounter.Add(1);
    }
  }
}


void ReflowCountMgr::PaintCount(const char*     aName,
                                nsRenderingContext* aRenderingContext,
                                nsPresContext*  aPresContext,
                                nsIFrame*       aFrame,
                                const nsPoint&  aOffset,
                                uint32_t        aColor)
{
  if (mPaintFrameByFrameCounts &&
      nullptr != mIndiFrameCounts &&
      aFrame != nullptr) {
    char key[KEY_BUF_SIZE_FOR_PTR];
    sprintf(key, "%p", (void*)aFrame);
    IndiReflowCounter * counter =
      (IndiReflowCounter *)PL_HashTableLookup(mIndiFrameCounts, key);
    if (counter != nullptr && counter->mName.EqualsASCII(aName)) {
      aRenderingContext->PushState();
      aRenderingContext->Translate(aOffset);
      nsFont font("Times", NS_FONT_STYLE_NORMAL, NS_FONT_VARIANT_NORMAL,
                  NS_FONT_WEIGHT_NORMAL, NS_FONT_STRETCH_NORMAL, 0,
                  nsPresContext::CSSPixelsToAppUnits(11));

      nsRefPtr<nsFontMetrics> fm;
      aPresContext->DeviceContext()->GetMetricsFor(font,
        
        aPresContext->GetPresShell()->GetRootFrame()->
          GetStyleFont()->mLanguage,
        aPresContext->GetUserFontSet(), *getter_AddRefs(fm));

      aRenderingContext->SetFont(fm);
      char buf[16];
      sprintf(buf, "%d", counter->mCount);
      nscoord x = 0, y = fm->MaxAscent();
      nscoord width, height = fm->MaxHeight();
      aRenderingContext->SetTextRunRTL(false);
      width = aRenderingContext->GetWidth(buf);

      uint32_t color;
      uint32_t color2;
      if (aColor != 0) {
        color  = aColor;
        color2 = NS_RGB(0,0,0);
      } else {
        uint8_t rc = 0, gc = 0, bc = 0;
        if (counter->mCount < 5) {
          rc = 255;
          gc = 255;
        } else if ( counter->mCount < 11) {
          gc = 255;
        } else {
          rc = 255;
        }
        color  = NS_RGB(rc,gc,bc);
        color2 = NS_RGB(rc/2,gc/2,bc/2);
      }

      nsRect rect(0,0, width+15, height+15);
      aRenderingContext->SetColor(NS_RGB(0,0,0));
      aRenderingContext->FillRect(rect);
      aRenderingContext->SetColor(color2);
      aRenderingContext->DrawString(buf, strlen(buf), x+15,y+15);
      aRenderingContext->SetColor(color);
      aRenderingContext->DrawString(buf, strlen(buf), x,y);

      aRenderingContext->PopState();
    }
  }
}


int ReflowCountMgr::RemoveItems(PLHashEntry *he, int i, void *arg)
{
  char *str = (char *)he->key;
  ReflowCounter * counter = (ReflowCounter *)he->value;
  delete counter;
  NS_Free(str);

  return HT_ENUMERATE_REMOVE;
}


int ReflowCountMgr::RemoveIndiItems(PLHashEntry *he, int i, void *arg)
{
  char *str = (char *)he->key;
  IndiReflowCounter * counter = (IndiReflowCounter *)he->value;
  delete counter;
  NS_Free(str);

  return HT_ENUMERATE_REMOVE;
}


void ReflowCountMgr::CleanUp()
{
  if (nullptr != mCounts) {
    PL_HashTableEnumerateEntries(mCounts, RemoveItems, nullptr);
    PL_HashTableDestroy(mCounts);
    mCounts = nullptr;
  }

  if (nullptr != mIndiFrameCounts) {
    PL_HashTableEnumerateEntries(mIndiFrameCounts, RemoveIndiItems, nullptr);
    PL_HashTableDestroy(mIndiFrameCounts);
    mIndiFrameCounts = nullptr;
  }
}


int ReflowCountMgr::DoSingleTotal(PLHashEntry *he, int i, void *arg)
{
  char *str = (char *)he->key;
  ReflowCounter * counter = (ReflowCounter *)he->value;

  counter->DisplayTotals(str);

  return HT_ENUMERATE_NEXT;
}


void ReflowCountMgr::DoGrandTotals()
{
  if (nullptr != mCounts) {
    ReflowCounter * gTots = (ReflowCounter *)PL_HashTableLookup(mCounts, kGrandTotalsStr);
    if (gTots == nullptr) {
      gTots = new ReflowCounter(this);
      PL_HashTableAdd(mCounts, NS_strdup(kGrandTotalsStr), gTots);
    } else {
      gTots->ClearTotals();
    }

    printf("\t\t\t\tTotal\n");
    for (uint32_t i=0;i<78;i++) {
      printf("-");
    }
    printf("\n");
    PL_HashTableEnumerateEntries(mCounts, DoSingleTotal, this);
  }
}

static void RecurseIndiTotals(nsPresContext* aPresContext, 
                              PLHashTable *   aHT, 
                              nsIFrame *      aParentFrame,
                              int32_t         aLevel)
{
  if (aParentFrame == nullptr) {
    return;
  }

  char key[KEY_BUF_SIZE_FOR_PTR];
  sprintf(key, "%p", (void*)aParentFrame);
  IndiReflowCounter * counter = (IndiReflowCounter *)PL_HashTableLookup(aHT, key);
  if (counter) {
    counter->mHasBeenOutput = true;
    char * name = ToNewCString(counter->mName);
    for (int32_t i=0;i<aLevel;i++) printf(" ");
    printf("%s - %p   [%d][", name, (void*)aParentFrame, counter->mCount);
    printf("%d", counter->mCounter.GetTotal());
    printf("]\n");
    nsMemory::Free(name);
  }

  nsIFrame* child = aParentFrame->GetFirstPrincipalChild();
  while (child) {
    RecurseIndiTotals(aPresContext, aHT, child, aLevel+1);
    child = child->GetNextSibling();
  }

}


int ReflowCountMgr::DoSingleIndi(PLHashEntry *he, int i, void *arg)
{
  IndiReflowCounter * counter = (IndiReflowCounter *)he->value;
  if (counter && !counter->mHasBeenOutput) {
    char * name = ToNewCString(counter->mName);
    printf("%s - %p   [%d][", name, (void*)counter->mFrame, counter->mCount);
    printf("%d", counter->mCounter.GetTotal());
    printf("]\n");
    nsMemory::Free(name);
  }
  return HT_ENUMERATE_NEXT;
}


void ReflowCountMgr::DoIndiTotalsTree()
{
  if (nullptr != mCounts) {
    printf("\n------------------------------------------------\n");
    printf("-- Individual Frame Counts\n");
    printf("------------------------------------------------\n");

    if (mPresShell) {
      nsIFrame * rootFrame = mPresShell->FrameManager()->GetRootFrame();
      RecurseIndiTotals(mPresContext, mIndiFrameCounts, rootFrame, 0);
      printf("------------------------------------------------\n");
      printf("-- Individual Counts of Frames not in Root Tree\n");
      printf("------------------------------------------------\n");
      PL_HashTableEnumerateEntries(mIndiFrameCounts, DoSingleIndi, this);
    }
  }
}


int ReflowCountMgr::DoSingleHTMLTotal(PLHashEntry *he, int i, void *arg)
{
  char *str = (char *)he->key;
  ReflowCounter * counter = (ReflowCounter *)he->value;

  counter->DisplayHTMLTotals(str);

  return HT_ENUMERATE_NEXT;
}


void ReflowCountMgr::DoGrandHTMLTotals()
{
  if (nullptr != mCounts) {
    ReflowCounter * gTots = (ReflowCounter *)PL_HashTableLookup(mCounts, kGrandTotalsStr);
    if (gTots == nullptr) {
      gTots = new ReflowCounter(this);
      PL_HashTableAdd(mCounts, NS_strdup(kGrandTotalsStr), gTots);
    } else {
      gTots->ClearTotals();
    }

    static const char * title[] = {"Class", "Reflows"};
    fprintf(mFD, "<tr>");
    for (uint32_t i=0; i < ArrayLength(title); i++) {
      fprintf(mFD, "<td><center><b>%s<b></center></td>", title[i]);
    }
    fprintf(mFD, "</tr>\n");
    PL_HashTableEnumerateEntries(mCounts, DoSingleHTMLTotal, this);
  }
}


void ReflowCountMgr::DisplayTotals(const char * aStr)
{
#ifdef DEBUG_rods
  printf("%s\n", aStr?aStr:"No name");
#endif
  if (mDumpFrameCounts) {
    DoGrandTotals();
  }
  if (mDumpFrameByFrameCounts) {
    DoIndiTotalsTree();
  }

}

void ReflowCountMgr::DisplayHTMLTotals(const char * aStr)
{
#ifdef WIN32x 
  char name[1024];
  
  char * sptr = strrchr(aStr, '/');
  if (sptr) {
    sptr++;
    strcpy(name, sptr);
    char * eptr = strrchr(name, '.');
    if (eptr) {
      *eptr = 0;
    }
    strcat(name, "_stats.html");
  }
  mFD = fopen(name, "w");
  if (mFD) {
    fprintf(mFD, "<html><head><title>Reflow Stats</title></head><body>\n");
    const char * title = aStr?aStr:"No name";
    fprintf(mFD, "<center><b>%s</b><br><table border=1 style=\"background-color:#e0e0e0\">", title);
    DoGrandHTMLTotals();
    fprintf(mFD, "</center></table>\n");
    fprintf(mFD, "</body></html>\n");
    fclose(mFD);
    mFD = nullptr;
  }
#endif 
}


int ReflowCountMgr::DoClearTotals(PLHashEntry *he, int i, void *arg)
{
  ReflowCounter * counter = (ReflowCounter *)he->value;
  counter->ClearTotals();

  return HT_ENUMERATE_NEXT;
}


void ReflowCountMgr::ClearTotals()
{
  PL_HashTableEnumerateEntries(mCounts, DoClearTotals, this);
}


void ReflowCountMgr::ClearGrandTotals()
{
  if (nullptr != mCounts) {
    ReflowCounter * gTots = (ReflowCounter *)PL_HashTableLookup(mCounts, kGrandTotalsStr);
    if (gTots == nullptr) {
      gTots = new ReflowCounter(this);
      PL_HashTableAdd(mCounts, NS_strdup(kGrandTotalsStr), gTots);
    } else {
      gTots->ClearTotals();
      gTots->SetTotalsCache();
    }
  }
}


int ReflowCountMgr::DoDisplayDiffTotals(PLHashEntry *he, int i, void *arg)
{
  bool cycledOnce = (arg != 0);

  char *str = (char *)he->key;
  ReflowCounter * counter = (ReflowCounter *)he->value;

  if (cycledOnce) {
    counter->CalcDiffInTotals();
    counter->DisplayDiffTotals(str);
  }
  counter->SetTotalsCache();

  return HT_ENUMERATE_NEXT;
}


void ReflowCountMgr::DisplayDiffsInTotals(const char * aStr)
{
  if (mCycledOnce) {
    printf("Differences\n");
    for (int32_t i=0;i<78;i++) {
      printf("-");
    }
    printf("\n");
    ClearGrandTotals();
  }
  PL_HashTableEnumerateEntries(mCounts, DoDisplayDiffTotals, (void *)mCycledOnce);

  mCycledOnce = true;
}

#endif 


void ColorToString(nscolor aColor, nsAutoString &aString)
{
  char buf[8];

  PR_snprintf(buf, sizeof(buf), "#%02x%02x%02x",
              NS_GET_R(aColor), NS_GET_G(aColor), NS_GET_B(aColor));
  CopyASCIItoUTF16(buf, aString);
}

nsIFrame* nsIPresShell::GetAbsoluteContainingBlock(nsIFrame *aFrame)
{
  return FrameConstructor()->GetAbsoluteContainingBlock(aFrame,
      nsCSSFrameConstructor::ABS_POS);
}

#ifdef ACCESSIBILITY
bool
nsIPresShell::IsAccessibilityActive()
{
  return GetAccService() != nullptr;
}

nsAccessibilityService*
nsIPresShell::AccService()
{
  return GetAccService();
}
#endif

static bool inited = false;

void nsIPresShell::InitializeStatics()
{
  NS_ASSERTION(!inited, "InitializeStatics called multiple times!");
  gCaptureTouchList.Init();
  inited = true;
}

void nsIPresShell::ReleaseStatics()
{
  NS_ASSERTION(inited, "ReleaseStatics called without Initialize!");
}


void PresShell::QueryIsActive()
{
  nsCOMPtr<nsISupports> container = mPresContext->GetContainer();
  if (mDocument) {
    nsIDocument* displayDoc = mDocument->GetDisplayDocument();
    if (displayDoc) {
      
      
      
      NS_ABORT_IF_FALSE(!container,
                        "external resource doc shouldn't have "
                        "its own container");

      nsIPresShell* displayPresShell = displayDoc->GetShell();
      if (displayPresShell) {
        container = displayPresShell->GetPresContext()->GetContainer();
      }
    }
  }

  nsCOMPtr<nsIDocShell> docshell(do_QueryInterface(container));
  if (docshell) {
    bool isActive;
    nsresult rv = docshell->GetIsActive(&isActive);
    if (NS_SUCCEEDED(rv))
      SetIsActive(isActive);
  }
}


static bool
SetExternalResourceIsActive(nsIDocument* aDocument, void* aClosure)
{
  nsIPresShell* shell = aDocument->GetShell();
  if (shell) {
    shell->SetIsActive(*static_cast<bool*>(aClosure));
  }
  return true;
}

static void
SetPluginIsActive(nsIContent* aContent, void* aClosure)
{
  nsIFrame *frame = aContent->GetPrimaryFrame();
  nsIObjectFrame *objectFrame = do_QueryFrame(frame);
  if (objectFrame) {
    objectFrame->SetIsDocumentActive(*static_cast<bool*>(aClosure));
  }
}

nsresult
PresShell::SetIsActive(bool aIsActive)
{
  NS_PRECONDITION(mDocument, "should only be called with a document");

  mIsActive = aIsActive;
  nsPresContext* presContext = GetPresContext();
  if (presContext &&
      presContext->RefreshDriver()->PresContext() == presContext) {
    presContext->RefreshDriver()->SetThrottled(!mIsActive);
  }

  
  mDocument->EnumerateExternalResources(SetExternalResourceIsActive,
                                        &aIsActive);
  mDocument->EnumerateFreezableElements(SetPluginIsActive,
                                        &aIsActive);
  nsresult rv = UpdateImageLockingState();
#ifdef ACCESSIBILITY
  if (aIsActive) {
    nsAccessibilityService* accService = AccService();
    if (accService) {
      accService->PresShellActivated(this);
    }
  }
#endif

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (TabChild* tab = GetTabChildFrom(this)) {
    if (aIsActive) {
      tab->MakeVisible();
      if (!mIsZombie) {
        if (nsIFrame* root = mFrameConstructor->GetRootFrame()) {
          FrameLayerBuilder::InvalidateAllLayersForFrame(
            nsLayoutUtils::GetDisplayRootFrame(root));
          root->SchedulePaint();
        }
      }
    } else {
      tab->MakeHidden();
    }
  }

  return rv;
}





nsresult
PresShell::UpdateImageLockingState()
{
  
  return mDocument->SetImageLockingState(!mFrozen && mIsActive);
}

PresShell*
PresShell::GetRootPresShell()
{
  if (mPresContext) {
    nsPresContext* rootPresContext = mPresContext->GetRootPresContext();
    if (rootPresContext) {
      return static_cast<PresShell*>(rootPresContext->PresShell());
    }
  }
  return nullptr;
}

void
PresShell::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                               nsArenaMemoryStats *aArenaObjectsSize,
                               size_t *aPresShellSize,
                               size_t *aStyleSetsSize,
                               size_t *aTextRunsSize,
                               size_t *aPresContextSize)
{
  mFrameArena.SizeOfExcludingThis(aMallocSizeOf, aArenaObjectsSize);
  *aPresShellSize = aMallocSizeOf(this);
  *aPresShellSize += aArenaObjectsSize->mOther;

  *aStyleSetsSize = StyleSet()->SizeOfIncludingThis(aMallocSizeOf);

  *aTextRunsSize = SizeOfTextRuns(aMallocSizeOf);

  *aPresContextSize = mPresContext->SizeOfIncludingThis(aMallocSizeOf);
}

size_t
PresShell::SizeOfTextRuns(nsMallocSizeOfFun aMallocSizeOf) const
{
  nsIFrame* rootFrame = mFrameConstructor->GetRootFrame();
  if (!rootFrame) {
    return 0;
  }

  
  nsLayoutUtils::SizeOfTextRunsForFrames(rootFrame, nullptr,
                                         true);

  
  return nsLayoutUtils::SizeOfTextRunsForFrames(rootFrame, aMallocSizeOf,
                                                false);
}

void
nsIPresShell::SetScrollPositionClampingScrollPortSize(nscoord aWidth, nscoord aHeight)
{
  if (!mScrollPositionClampingScrollPortSizeSet ||
      mScrollPositionClampingScrollPortSize.width != aWidth ||
      mScrollPositionClampingScrollPortSize.height != aHeight) {
    mScrollPositionClampingScrollPortSizeSet = true;
    mScrollPositionClampingScrollPortSize.width = aWidth;
    mScrollPositionClampingScrollPortSize.height = aHeight;

    
    nsIFrame* rootFrame = mFrameConstructor->GetRootFrame();
    if (rootFrame) {
      const nsFrameList& childList = rootFrame->GetChildList(nsIFrame::kFixedList);
      for (nsIFrame* child = childList.FirstChild(); child;
           child = child->GetNextSibling()) {
        FrameNeedsReflow(child, eResize, NS_FRAME_IS_DIRTY);
      }
    }
  }
}

void
PresShell::SetupFontInflation()
{
  mFontSizeInflationEmPerLine = nsLayoutUtils::FontSizeInflationEmPerLine();
  mFontSizeInflationMinTwips = nsLayoutUtils::FontSizeInflationMinTwips();
  mFontSizeInflationLineThreshold = nsLayoutUtils::FontSizeInflationLineThreshold();
  mFontSizeInflationForceEnabled = nsLayoutUtils::FontSizeInflationForceEnabled();
  mFontSizeInflationDisabledInMasterProcess = nsLayoutUtils::FontSizeInflationDisabledInMasterProcess();
}

void
nsIPresShell::SetMaxLineBoxWidth(nscoord aMaxLineBoxWidth)
{
  NS_ASSERTION(aMaxLineBoxWidth >= 0, "attempting to set max line box width to a negative value");

  mMaxLineBoxWidth = aMaxLineBoxWidth;

  FrameNeedsReflow(GetRootFrame(), eResize, NS_FRAME_IS_DIRTY);
}
