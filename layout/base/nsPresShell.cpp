

























































#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "mozilla/dom/Element.h"
#include "nsIDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsStubDocumentObserver.h"
#include "nsStyleSet.h"
#include "nsCSSStyleSheet.h" 
#include "nsIDOMCSSStyleSheet.h"  
#ifdef MOZ_CSS_ANIMATIONS
#include "nsAnimationManager.h"
#endif
#include "nsINameSpaceManager.h"  
#include "nsIServiceManager.h"
#include "nsFrame.h"
#include "nsIViewManager.h"
#include "nsCRT.h"
#include "nsCRTGlue.h"
#include "prlog.h"
#include "prmem.h"
#include "prprf.h"
#include "prinrval.h"
#include "nsTArray.h"
#include "nsCOMArray.h"
#include "nsHashtable.h"
#include "nsIViewObserver.h"
#include "nsContainerFrame.h"
#include "nsDOMEvent.h"
#include "nsGUIEvent.h"
#include "nsHTMLParts.h"
#include "nsContentUtils.h"
#include "nsISelection.h"
#include "nsISelectionController.h"
#include "nsISelectionPrivate.h"
#include "nsLayoutCID.h"
#include "nsGkAtoms.h"
#include "nsIDOMRange.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNode.h"
#include "nsIDOM3Node.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMElement.h"
#include "nsRange.h"
#include "nsCSSPseudoElements.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsWeakReference.h"
#include "nsIPageSequenceFrame.h"
#include "nsCaret.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMXMLDocument.h"
#include "nsIParser.h"
#include "nsParserCIID.h"
#include "nsFrameSelection.h"
#include "nsIDOMNSHTMLTextAreaElement.h"
#include "nsViewsCID.h"
#include "nsPresArena.h"
#include "nsFrameManager.h"
#include "nsEventStateManager.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsILayoutHistoryState.h"
#include "nsILineIterator.h" 
#include "nsWeakPtr.h"
#include "pldhash.h"
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "nsIDocShell.h"        
#include "nsIBaseWindow.h"
#include "nsLayoutErrors.h"
#include "nsLayoutUtils.h"
#include "nsCSSRendering.h"
  
#include "prenv.h"
#include "nsIAttribute.h"
#include "nsIGlobalHistory2.h"
#include "nsDisplayList.h"
#include "nsIRegion.h"
#include "nsRegion.h"
#include "nsRenderingContext.h"

#ifdef MOZ_REFLOW_PERF
#include "nsFontMetrics.h"
#endif

#include "nsIReflowCallback.h"

#include "nsPIDOMWindow.h"
#include "nsFocusManager.h"
#include "nsIPluginInstance.h"
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
#ifdef MOZ_SMIL
#include "nsSMILAnimationController.h"
#endif

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

#include "mozilla/FunctionTimer.h"

#include "Layers.h"

#ifdef NS_FUNCTION_TIMER
#define NS_TIME_FUNCTION_DECLARE_DOCURL                \
  nsCAutoString docURL__("N/A");                       \
  nsIURI *uri__ = mDocument->GetDocumentURI();         \
  if (uri__) uri__->GetSpec(docURL__);
#define NS_TIME_FUNCTION_WITH_DOCURL                   \
  NS_TIME_FUNCTION_DECLARE_DOCURL                      \
  NS_TIME_FUNCTION_MIN_FMT(1.0,                        \
     "%s (line %d) (document: %s)", MOZ_FUNCTION_NAME, \
     __LINE__, docURL__.get())
#else
#define NS_TIME_FUNCTION_WITH_DOCURL do{} while(0)
#endif

#define ANCHOR_SCROLL_FLAGS (SCROLL_OVERFLOW_HIDDEN | SCROLL_NO_PARENT_FRAMES)

#include "nsContentCID.h"
static NS_DEFINE_IID(kRangeCID,     NS_RANGE_CID);


#include "nsIMemoryReporter.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::layers;

PRBool nsIPresShell::gIsAccessibilityActive = PR_FALSE;
CapturingContentInfo nsIPresShell::gCaptureInfo =
  { PR_FALSE ,     PR_FALSE ,
    PR_FALSE , nsnull  };
nsIContent* nsIPresShell::gKeyDownTarget;

static PRUint32
ChangeFlag(PRUint32 aFlags, PRBool aOnOff, PRUint32 aFlag)
{
  PRUint32 flags;
  if (aOnOff) {
    flags = (aFlags | aFlag);
  } else {
    flags = (aFlag & ~aFlag);
  }
  return flags;
}



static void ColorToString(nscolor aColor, nsAutoString &aString);


static NS_DEFINE_CID(kFrameSelectionCID, NS_FRAMESELECTION_CID);


struct RangePaintInfo {
  nsCOMPtr<nsIRange> mRange;
  nsDisplayListBuilder mBuilder;
  nsDisplayList mList;

  
  nsPoint mRootOffset;

  RangePaintInfo(nsIRange* aRange, nsIFrame* aFrame)
    : mRange(aRange), mBuilder(aFrame, nsDisplayListBuilder::PAINTING, PR_FALSE)
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



#ifdef NS_DEBUG



static PRUint32 gVerifyReflowFlags;

struct VerifyReflowFlags {
  const char*    name;
  PRUint32 bit;
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
  ReflowCounter(ReflowCountMgr * aMgr = nsnull);
  ~ReflowCounter();

  void ClearTotals();
  void DisplayTotals(const char * aStr);
  void DisplayDiffTotals(const char * aStr);
  void DisplayHTMLTotals(const char * aStr);

  void Add()                { mTotal++;         }
  void Add(PRUint32 aTotal) { mTotal += aTotal; }

  void CalcDiffInTotals();
  void SetTotalsCache();

  void SetMgr(ReflowCountMgr * aMgr) { mMgr = aMgr; }

  PRUint32 GetTotal() { return mTotal; }
  
protected:
  void DisplayTotals(PRUint32 aTotal, const char * aTitle);
  void DisplayHTMLTotals(PRUint32 aTotal, const char * aTitle);

  PRUint32 mTotal;
  PRUint32 mCacheTotal;

  ReflowCountMgr * mMgr; 
};


class IndiReflowCounter {
public:
  IndiReflowCounter(ReflowCountMgr * aMgr = nsnull)
    : mFrame(nsnull),
      mCount(0),
      mMgr(aMgr),
      mCounter(aMgr),
      mHasBeenOutput(PR_FALSE)
    {}
  virtual ~IndiReflowCounter() {}

  nsAutoString mName;
  nsIFrame *   mFrame;   
  PRInt32      mCount;

  ReflowCountMgr * mMgr; 

  ReflowCounter mCounter;
  PRBool        mHasBeenOutput;

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
                  const nsPoint &aOffset, PRUint32 aColor);

  FILE * GetOutFile() { return mFD; }

  PLHashTable * GetIndiFrameHT() { return mIndiFrameCounts; }

  void SetPresContext(nsPresContext * aPresContext) { mPresContext = aPresContext; } 
  void SetPresShell(nsIPresShell* aPresShell) { mPresShell= aPresShell; } 

  void SetDumpFrameCounts(PRBool aVal)         { mDumpFrameCounts = aVal; }
  void SetDumpFrameByFrameCounts(PRBool aVal)  { mDumpFrameByFrameCounts = aVal; }
  void SetPaintFrameCounts(PRBool aVal)        { mPaintFrameByFrameCounts = aVal; }

  PRBool IsPaintingFrameCounts() { return mPaintFrameByFrameCounts; }

protected:
  void DisplayTotals(PRUint32 aTotal, PRUint32 * aDupArray, char * aTitle);
  void DisplayHTMLTotals(PRUint32 aTotal, PRUint32 * aDupArray, char * aTitle);

  static PRIntn RemoveItems(PLHashEntry *he, PRIntn i, void *arg);
  static PRIntn RemoveIndiItems(PLHashEntry *he, PRIntn i, void *arg);
  void CleanUp();

  
  static PRIntn DoSingleTotal(PLHashEntry *he, PRIntn i, void *arg);
  static PRIntn DoSingleIndi(PLHashEntry *he, PRIntn i, void *arg);

  void DoGrandTotals();
  void DoIndiTotalsTree();

  
  static PRIntn DoSingleHTMLTotal(PLHashEntry *he, PRIntn i, void *arg);
  void DoGrandHTMLTotals();

  
  static PRIntn DoClearTotals(PLHashEntry *he, PRIntn i, void *arg);

  
  static PRIntn DoDisplayDiffTotals(PLHashEntry *he, PRIntn i, void *arg);

  PLHashTable * mCounts;
  PLHashTable * mIndiFrameCounts;
  FILE * mFD;
  
  PRBool mDumpFrameCounts;
  PRBool mDumpFrameByFrameCounts;
  PRBool mPaintFrameByFrameCounts;

  PRBool mCycledOnce;

  
  nsPresContext * mPresContext;
  nsIPresShell*    mPresShell;

  
};
#endif



#define SHOW_CARET







#define NS_MAX_REFLOW_TIME    1000000
static PRInt32 gMaxRCProcessingTime = -1;

#define MARK_INCREMENT 50
#define BLOCK_INCREMENT 4044 /* a bit under 4096, for malloc overhead */




struct StackBlock {
   
   
   
   char mBlock[BLOCK_INCREMENT];

   
   
   
   StackBlock* mNext;

   StackBlock() : mNext(nsnull) { }
   ~StackBlock() { }
};




struct StackMark {
   
   StackBlock* mBlock;
   
   
   size_t mPos;
};






class StackArena {
public:
  StackArena();
  ~StackArena();

  nsresult Init() { return mBlocks ? NS_OK : NS_ERROR_OUT_OF_MEMORY; }

  
  void* Allocate(size_t aSize);
  void Push();
  void Pop();

  PRUint32 Size() {
    PRUint32 result = 0;
    StackBlock *block = mBlocks;
    while (block) {
      result += sizeof(StackBlock);
      block = block->mNext;
    }
    return result;
  }

private:
  
  size_t mPos;

  
  
  StackBlock* mBlocks;

  
  StackBlock* mCurBlock;

  
  StackMark* mMarks;

  
  PRUint32 mStackTop;

  
  PRUint32 mMarkLength;
};



StackArena::StackArena()
{
  mMarkLength = 0;
  mMarks = nsnull;

  
  mBlocks = new StackBlock();
  mCurBlock = mBlocks;

  mStackTop = 0;
  mPos = 0;
}

StackArena::~StackArena()
{
  
  delete[] mMarks;
  while(mBlocks)
  {
    StackBlock* toDelete = mBlocks;
    mBlocks = mBlocks->mNext;
    delete toDelete;
  }
} 

void
StackArena::Push()
{
  
  
  
  if (mStackTop >= mMarkLength)
  {
    PRUint32 newLength = mStackTop + MARK_INCREMENT;
    StackMark* newMarks = new StackMark[newLength];
    if (newMarks) {
      if (mMarkLength)
        memcpy(newMarks, mMarks, sizeof(StackMark)*mMarkLength);
      
      
      for (; mMarkLength < mStackTop; ++mMarkLength) {
        NS_NOTREACHED("should only hit this on out-of-memory");
        newMarks[mMarkLength].mBlock = mCurBlock;
        newMarks[mMarkLength].mPos = mPos;
      }
      delete [] mMarks;
      mMarks = newMarks;
      mMarkLength = newLength;
    }
  }

  
  NS_ASSERTION(mStackTop < mMarkLength, "out of memory");
  if (mStackTop < mMarkLength) {
    mMarks[mStackTop].mBlock = mCurBlock;
    mMarks[mStackTop].mPos = mPos;
  }

  mStackTop++;
}

void*
StackArena::Allocate(size_t aSize)
{
  NS_ASSERTION(mStackTop > 0, "Allocate called without Push");

  
  
  aSize = PR_ROUNDUP(aSize, 8);

  
  if (mPos + aSize >= BLOCK_INCREMENT)
  {
    NS_ASSERTION(aSize <= BLOCK_INCREMENT,"Requested memory is greater that our block size!!");
    if (mCurBlock->mNext == nsnull)
      mCurBlock->mNext = new StackBlock();

    mCurBlock =  mCurBlock->mNext;
    mPos = 0;
  }

  
  void *result = mCurBlock->mBlock + mPos;
  mPos += aSize;

  return result;
}

void
StackArena::Pop()
{
  
  NS_ASSERTION(mStackTop > 0, "unmatched pop");
  mStackTop--;

  if (mStackTop >= mMarkLength) {
    
    
    NS_NOTREACHED("out of memory");
    if (mStackTop == 0) {
      
      mCurBlock = mBlocks;
      mPos = 0;
    }
    return;
  }

#ifdef DEBUG
  
  
  {
    StackBlock *block = mMarks[mStackTop].mBlock, *block_end = mCurBlock;
    size_t pos = mMarks[mStackTop].mPos;
    for (; block != block_end; block = block->mNext, pos = 0) {
      memset(block->mBlock + pos, 0xdd, sizeof(block->mBlock) - pos);
    }
    memset(block->mBlock + pos, 0xdd, mPos - pos);
  }
#endif

  mCurBlock = mMarks[mStackTop].mBlock;
  mPos      = mMarks[mStackTop].mPos;
}

struct nsCallbackEventRequest
{
  nsIReflowCallback* callback;
  nsCallbackEventRequest* next;
};


#define ASSERT_REFLOW_SCHEDULED_STATE()                                       \
  NS_ASSERTION(mReflowScheduled ==                                            \
                 GetPresContext()->RefreshDriver()->                          \
                   IsLayoutFlushObserver(this), "Unexpected state")

class nsPresShellEventCB;
class nsAutoCauseReflowNotifier;

class PresShell : public nsIPresShell, public nsIViewObserver,
                  public nsStubDocumentObserver,
                  public nsISelectionController, public nsIObserver,
                  public nsSupportsWeakReference
{
public:
  PresShell();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  
  NS_DECL_ISUPPORTS

  
  virtual NS_HIDDEN_(nsresult) Init(nsIDocument* aDocument,
                                   nsPresContext* aPresContext,
                                   nsIViewManager* aViewManager,
                                   nsStyleSet* aStyleSet,
                                   nsCompatibility aCompatMode);
  virtual NS_HIDDEN_(void) Destroy();

  virtual NS_HIDDEN_(void*) AllocateFrame(nsQueryFrame::FrameIID aCode,
                                          size_t aSize);
  virtual NS_HIDDEN_(void)  FreeFrame(nsQueryFrame::FrameIID aCode,
                                      void* aChunk);

  virtual NS_HIDDEN_(void*) AllocateMisc(size_t aSize);
  virtual NS_HIDDEN_(void)  FreeMisc(size_t aSize, void* aChunk);

  
  virtual NS_HIDDEN_(void) PushStackMemory();
  virtual NS_HIDDEN_(void) PopStackMemory();
  virtual NS_HIDDEN_(void*) AllocateStackMemory(size_t aSize);

  virtual NS_HIDDEN_(nsresult) SetPreferenceStyleRules(PRBool aForceReflow);

  NS_IMETHOD GetSelection(SelectionType aType, nsISelection** aSelection);
  virtual nsISelection* GetCurrentSelection(SelectionType aType);

  NS_IMETHOD SetDisplaySelection(PRInt16 aToggle);
  NS_IMETHOD GetDisplaySelection(PRInt16 *aToggle);
  NS_IMETHOD ScrollSelectionIntoView(SelectionType aType, SelectionRegion aRegion,
                                     PRInt16 aFlags);
  NS_IMETHOD RepaintSelection(SelectionType aType);

  virtual NS_HIDDEN_(void) BeginObservingDocument();
  virtual NS_HIDDEN_(void) EndObservingDocument();
  virtual NS_HIDDEN_(nsresult) InitialReflow(nscoord aWidth, nscoord aHeight);
  virtual NS_HIDDEN_(nsresult) ResizeReflow(nscoord aWidth, nscoord aHeight);
  virtual NS_HIDDEN_(nsresult) ResizeReflowOverride(nscoord aWidth, nscoord aHeight);
  virtual NS_HIDDEN_(void) StyleChangeReflow();
  virtual NS_HIDDEN_(nsIPageSequenceFrame*) GetPageSequenceFrame() const;
  virtual NS_HIDDEN_(nsIFrame*) GetRealPrimaryFrameFor(nsIContent* aContent) const;

  virtual NS_HIDDEN_(nsIFrame*) GetPlaceholderFrameFor(nsIFrame* aFrame) const;
  virtual NS_HIDDEN_(void) FrameNeedsReflow(nsIFrame *aFrame, IntrinsicDirty aIntrinsicDirty,
                                            nsFrameState aBitToAdd);
  virtual NS_HIDDEN_(void) FrameNeedsToContinueReflow(nsIFrame *aFrame);
  virtual NS_HIDDEN_(void) CancelAllPendingReflows();
  virtual NS_HIDDEN_(PRBool) IsSafeToFlush() const;
  virtual NS_HIDDEN_(void) FlushPendingNotifications(mozFlushType aType);

  


  virtual NS_HIDDEN_(nsresult) RecreateFramesFor(nsIContent* aContent);

  


  virtual NS_HIDDEN_(nsresult) PostReflowCallback(nsIReflowCallback* aCallback);
  virtual NS_HIDDEN_(void) CancelReflowCallback(nsIReflowCallback* aCallback);

  virtual NS_HIDDEN_(void) ClearFrameRefs(nsIFrame* aFrame);
  virtual NS_HIDDEN_(already_AddRefed<nsRenderingContext>) GetReferenceRenderingContext();
  virtual NS_HIDDEN_(nsresult) GoToAnchor(const nsAString& aAnchorName, PRBool aScroll);
  virtual NS_HIDDEN_(nsresult) ScrollToAnchor();

  virtual NS_HIDDEN_(nsresult) ScrollContentIntoView(nsIContent* aContent,
                                                     PRIntn      aVPercent,
                                                     PRIntn      aHPercent,
                                                     PRUint32    aFlags);
  virtual PRBool ScrollFrameRectIntoView(nsIFrame*     aFrame,
                                         const nsRect& aRect,
                                         PRIntn        aVPercent,
                                         PRIntn        aHPercent,
                                         PRUint32      aFlags);
  virtual nsRectVisibility GetRectVisibility(nsIFrame *aFrame,
                                             const nsRect &aRect,
                                             nscoord aMinTwips) const;

  virtual NS_HIDDEN_(void) SetIgnoreFrameDestruction(PRBool aIgnore);
  virtual NS_HIDDEN_(void) NotifyDestroyingFrame(nsIFrame* aFrame);

  virtual NS_HIDDEN_(nsresult) GetLinkLocation(nsIDOMNode* aNode, nsAString& aLocationString) const;

  virtual NS_HIDDEN_(nsresult) CaptureHistoryState(nsILayoutHistoryState** aLayoutHistoryState, PRBool aLeavingPage);

  virtual NS_HIDDEN_(void) UnsuppressPainting();

  virtual nsresult GetAgentStyleSheets(nsCOMArray<nsIStyleSheet>& aSheets);
  virtual nsresult SetAgentStyleSheets(const nsCOMArray<nsIStyleSheet>& aSheets);

  virtual nsresult AddOverrideStyleSheet(nsIStyleSheet *aSheet);
  virtual nsresult RemoveOverrideStyleSheet(nsIStyleSheet *aSheet);

  virtual NS_HIDDEN_(nsresult) HandleEventWithTarget(nsEvent* aEvent, nsIFrame* aFrame,
                                                     nsIContent* aContent,
                                                     nsEventStatus* aStatus);
  virtual NS_HIDDEN_(nsIFrame*) GetEventTargetFrame();
  virtual NS_HIDDEN_(already_AddRefed<nsIContent>) GetEventTargetContent(nsEvent* aEvent);


  virtual nsresult ReconstructFrames(void);
  virtual void Freeze();
  virtual void Thaw();
  virtual void FireOrClearDelayedEvents(PRBool aFireEvents);

  virtual nsIFrame* GetFrameForPoint(nsIFrame* aFrame, nsPoint aPt);

  virtual NS_HIDDEN_(nsresult) RenderDocument(const nsRect& aRect, PRUint32 aFlags,
                                              nscolor aBackgroundColor,
                                              gfxContext* aThebesContext);

  virtual already_AddRefed<gfxASurface> RenderNode(nsIDOMNode* aNode,
                                                   nsIntRegion* aRegion,
                                                   nsIntPoint& aPoint,
                                                   nsIntRect* aScreenRect);

  virtual already_AddRefed<gfxASurface> RenderSelection(nsISelection* aSelection,
                                                        nsIntPoint& aPoint,
                                                        nsIntRect* aScreenRect);

  virtual already_AddRefed<nsPIDOMWindow> GetRootWindow();

  virtual LayerManager* GetLayerManager();

  virtual void SetIgnoreViewportScrolling(PRBool aIgnore);

  virtual void SetDisplayPort(const nsRect& aDisplayPort);

  virtual nsresult SetResolution(float aXResolution, float aYResolution);

  

  NS_IMETHOD Paint(nsIView* aViewToPaint,
                   nsIWidget* aWidget,
                   const nsRegion& aDirtyRegion,
                   const nsIntRegion& aIntDirtyRegion,
                   PRBool aPaintDefaultBackground,
                   PRBool aWillSendDidPaint);
  NS_IMETHOD HandleEvent(nsIView*        aView,
                         nsGUIEvent*     aEvent,
                         PRBool          aDontRetargetEvents,
                         nsEventStatus*  aEventStatus);
  virtual NS_HIDDEN_(nsresult) HandleDOMEventWithTarget(nsIContent* aTargetContent,
                                                        nsEvent* aEvent,
                                                        nsEventStatus* aStatus);
  virtual NS_HIDDEN_(nsresult) HandleDOMEventWithTarget(nsIContent* aTargetContent,
                                                        nsIDOMEvent* aEvent,
                                                        nsEventStatus* aStatus);
  NS_IMETHOD ResizeReflow(nsIView *aView, nscoord aWidth, nscoord aHeight);
  NS_IMETHOD_(PRBool) ShouldIgnoreInvalidation();
  NS_IMETHOD_(void) WillPaint(PRBool aWillSendDidPaint);
  NS_IMETHOD_(void) DidPaint();
  NS_IMETHOD_(void) DispatchSynthMouseMove(nsGUIEvent *aEvent,
                                           PRBool aFlushOnHoverChange);
  NS_IMETHOD_(void) ClearMouseCapture(nsIView* aView);

  
  virtual NS_HIDDEN_(already_AddRefed<nsCaret>) GetCaret() const;
  virtual NS_HIDDEN_(void) MaybeInvalidateCaretPosition();
  NS_IMETHOD SetCaretEnabled(PRBool aInEnable);
  NS_IMETHOD SetCaretReadOnly(PRBool aReadOnly);
  NS_IMETHOD GetCaretEnabled(PRBool *aOutEnabled);
  NS_IMETHOD SetCaretVisibilityDuringSelection(PRBool aVisibility);
  NS_IMETHOD GetCaretVisible(PRBool *_retval);
  virtual void SetCaret(nsCaret *aNewCaret);
  virtual void RestoreCaret();

  NS_IMETHOD SetSelectionFlags(PRInt16 aInEnable);
  NS_IMETHOD GetSelectionFlags(PRInt16 *aOutEnable);

  

  NS_IMETHOD CharacterMove(PRBool aForward, PRBool aExtend);
  NS_IMETHOD CharacterExtendForDelete();
  NS_IMETHOD CharacterExtendForBackspace();
  NS_IMETHOD WordMove(PRBool aForward, PRBool aExtend);
  NS_IMETHOD WordExtendForDelete(PRBool aForward);
  NS_IMETHOD LineMove(PRBool aForward, PRBool aExtend);
  NS_IMETHOD IntraLineMove(PRBool aForward, PRBool aExtend);
  NS_IMETHOD PageMove(PRBool aForward, PRBool aExtend);
  NS_IMETHOD ScrollPage(PRBool aForward);
  NS_IMETHOD ScrollLine(PRBool aForward);
  NS_IMETHOD ScrollHorizontal(PRBool aLeft);
  NS_IMETHOD CompleteScroll(PRBool aForward);
  NS_IMETHOD CompleteMove(PRBool aForward, PRBool aExtend);
  NS_IMETHOD SelectAll();
  NS_IMETHOD CheckVisibility(nsIDOMNode *node, PRInt16 startOffset, PRInt16 EndOffset, PRBool *_retval);

  
  NS_DECL_NSIDOCUMENTOBSERVER_BEGINUPDATE
  NS_DECL_NSIDOCUMENTOBSERVER_ENDUPDATE
  NS_DECL_NSIDOCUMENTOBSERVER_BEGINLOAD
  NS_DECL_NSIDOCUMENTOBSERVER_ENDLOAD
  NS_DECL_NSIDOCUMENTOBSERVER_CONTENTSTATECHANGED
  NS_DECL_NSIDOCUMENTOBSERVER_DOCUMENTSTATESCHANGED
  NS_DECL_NSIDOCUMENTOBSERVER_STYLESHEETADDED
  NS_DECL_NSIDOCUMENTOBSERVER_STYLESHEETREMOVED
  NS_DECL_NSIDOCUMENTOBSERVER_STYLESHEETAPPLICABLESTATECHANGED
  NS_DECL_NSIDOCUMENTOBSERVER_STYLERULECHANGED
  NS_DECL_NSIDOCUMENTOBSERVER_STYLERULEADDED
  NS_DECL_NSIDOCUMENTOBSERVER_STYLERULEREMOVED

  
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTEWILLCHANGE
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  NS_DECL_NSIOBSERVER

#ifdef MOZ_REFLOW_PERF
  virtual NS_HIDDEN_(void) DumpReflows();
  virtual NS_HIDDEN_(void) CountReflows(const char * aName, nsIFrame * aFrame);
  virtual NS_HIDDEN_(void) PaintCount(const char * aName,
                                      nsRenderingContext* aRenderingContext,
                                      nsPresContext* aPresContext,
                                      nsIFrame * aFrame,
                                      const nsPoint& aOffset,
                                      PRUint32 aColor);
  virtual NS_HIDDEN_(void) SetPaintFrameCount(PRBool aOn);
  virtual PRBool IsPaintingFrameCounts();
#endif

#ifdef DEBUG
  virtual void ListStyleContexts(nsIFrame *aRootFrame, FILE *out,
                                 PRInt32 aIndent = 0);

  virtual void ListStyleSheets(FILE *out, PRInt32 aIndent = 0);
  virtual void VerifyStyleTree();
#endif

#ifdef PR_LOGGING
  static PRLogModuleInfo* gLog;
#endif

  virtual NS_HIDDEN_(void) DisableNonTestMouseEvents(PRBool aDisable);

  virtual void UpdateCanvasBackground();

  virtual nsresult AddCanvasBackgroundColorItem(nsDisplayListBuilder& aBuilder,
                                                nsDisplayList& aList,
                                                nsIFrame* aFrame,
                                                const nsRect& aBounds,
                                                nscolor aBackstopColor,
                                                PRUint32 aFlags);

  virtual nsresult AddPrintPreviewBackgroundItem(nsDisplayListBuilder& aBuilder,
                                                 nsDisplayList& aList,
                                                 nsIFrame* aFrame,
                                                 const nsRect& aBounds);

  virtual nscolor ComputeBackstopColor(nsIView* aDisplayRoot);

  virtual NS_HIDDEN_(nsresult) SetIsActive(PRBool aIsActive);

  virtual PRBool GetIsViewportOverridden() { return mViewportOverridden; }

  virtual PRBool IsLayoutFlushObserver()
  {
    return GetPresContext()->RefreshDriver()->
      IsLayoutFlushObserver(this);
  }

protected:
  virtual ~PresShell();

  void HandlePostedReflowCallbacks(PRBool aInterruptible);
  void CancelPostedReflowCallbacks();

  void UnsuppressAndInvalidate();

  void WillCauseReflow() {
    nsContentUtils::AddScriptBlocker();
    ++mChangeNestCount;
  }
  nsresult DidCauseReflow();
  friend class nsAutoCauseReflowNotifier;

  void     WillDoReflow();
  void     DidDoReflow(PRBool aInterruptible);
  
  
  PRBool   ProcessReflowCommands(PRBool aInterruptible);
  
  
  
  void     MaybeScheduleReflow();
  
  
  
  void     ScheduleReflow();

  
  nsresult ResizeReflowIgnoreOverride(nscoord aWidth, nscoord aHeight);

  
  PRBool DoReflow(nsIFrame* aFrame, PRBool aInterruptible);
#ifdef DEBUG
  void DoVerifyReflow();
  void VerifyHasDirtyRootAncestor(nsIFrame* aFrame);
#endif

  
  void DoScrollContentIntoView(nsIContent* aContent,
                               PRIntn      aVPercent,
                               PRIntn      aHPercent,
                               PRUint32    aFlags);

  friend struct AutoRenderingStateSaveRestore;
  friend struct RenderingState;

  struct RenderingState {
    RenderingState(PresShell* aPresShell) 
      : mRenderFlags(aPresShell->mRenderFlags)
      , mXResolution(aPresShell->mXResolution)
      , mYResolution(aPresShell->mYResolution)
    { }
    PRUint32 mRenderFlags;
    float mXResolution;
    float mYResolution;
  };

  struct AutoSaveRestoreRenderingState {
    AutoSaveRestoreRenderingState(PresShell* aPresShell)
      : mPresShell(aPresShell)
      , mOldState(aPresShell)
    {}

    ~AutoSaveRestoreRenderingState()
    {
      mPresShell->mRenderFlags = mOldState.mRenderFlags;
      mPresShell->mXResolution = mOldState.mXResolution;
      mPresShell->mYResolution = mOldState.mYResolution;
    }

    PresShell* mPresShell;
    RenderingState mOldState;
  };

  void SetRenderingState(const RenderingState& aState);

  friend class nsPresShellEventCB;

  PRBool mCaretEnabled;
#ifdef NS_DEBUG
  nsresult CloneStyleSet(nsStyleSet* aSet, nsStyleSet** aResult);
  PRBool VerifyIncrementalReflow();
  PRBool mInVerifyReflow;
  void ShowEventTargetDebug();
#endif

    



  nsresult ClearPreferenceStyleRules(void);
  nsresult CreatePreferenceStyleSheet(void);
  nsresult SetPrefLinkRules(void);
  nsresult SetPrefFocusRules(void);
  nsresult SetPrefNoScriptRule();
  nsresult SetPrefNoFramesRule(void);

  

  
  
  nsRect ClipListToRange(nsDisplayListBuilder *aBuilder,
                         nsDisplayList* aList,
                         nsIRange* aRange);

  
  
  RangePaintInfo* CreateRangePaintInfo(nsIDOMRange* aRange,
                                       nsRect& aSurfaceRect,
                                       PRBool aForPrimarySelection);

  









  already_AddRefed<gfxASurface>
  PaintRangePaintInfo(nsTArray<nsAutoPtr<RangePaintInfo> >* aItems,
                      nsISelection* aSelection,
                      nsIntRegion* aRegion,
                      nsRect aArea,
                      nsIntPoint& aPoint,
                      nsIntRect* aScreenRect);

  



  void AddUserSheet(nsISupports* aSheet);
  void AddAgentSheet(nsISupports* aSheet);
  void RemoveSheet(nsStyleSet::sheetType aType, nsISupports* aSheet);

  
  void HideViewIfPopup(nsIView* aView);

  
  void RestoreRootScrollPosition();

  void MaybeReleaseCapturingContent()
  {
    nsCOMPtr<nsFrameSelection> frameSelection = FrameSelection();
    if (frameSelection) {
      frameSelection->SetMouseDownState(PR_FALSE);
    }
    if (gCaptureInfo.mContent &&
        gCaptureInfo.mContent->GetOwnerDoc() == mDocument) {
      SetCapturingContent(nsnull, 0);
    }
  }

  nsresult HandleRetargetedEvent(nsEvent* aEvent, nsIView* aView,
                                 nsEventStatus* aStatus, nsIContent* aTarget)
  {
    PushCurrentEventInfo(nsnull, nsnull);
    mCurrentEventContent = aTarget;
    nsresult rv = NS_OK;
    if (GetCurrentEventFrame()) {
      rv = HandleEventInternal(aEvent, aView, aStatus);
    }
    PopCurrentEventInfo();
    return rv;
  }

  nsRefPtr<nsCSSStyleSheet> mPrefStyleSheet; 
                                             
#ifdef DEBUG
  PRUint32                  mUpdateCount;
#endif
  
  nsTArray<nsIFrame*> mDirtyRoots;

  PRPackedBool mDocumentLoading;

  PRPackedBool mIgnoreFrameDestruction;
  PRPackedBool mHaveShutDown;

  PRPackedBool mViewportOverridden;

  PRPackedBool mLastRootReflowHadUnconstrainedHeight;

  
  
  
  PRUint32  mChangeNestCount;
  
  nsIFrame*   mCurrentEventFrame;
  nsCOMPtr<nsIContent> mCurrentEventContent;
  nsTArray<nsIFrame*> mCurrentEventFrameStack;
  nsCOMArray<nsIContent> mCurrentEventContentStack;

  nsCOMPtr<nsIContent>          mLastAnchorScrolledTo;
  nscoord                       mLastAnchorScrollPositionY;
  nsRefPtr<nsCaret>             mCaret;
  nsRefPtr<nsCaret>             mOriginalCaret;
  nsPresArena                   mFrameArena;
  StackArena                    mStackArena;
  nsCOMPtr<nsIDragService>      mDragService;
  
#ifdef DEBUG
  
  
  nsIFrame* mCurrentReflowRoot;
#endif

  
  
  nsTHashtable< nsPtrHashKey<nsIFrame> > mFramesToDirty;

  
  
  
  
  
  nsCOMPtr<nsIContent> mContentToScrollTo;
  PRIntn mContentScrollVPosition;
  PRIntn mContentScrollHPosition;
  PRUint32 mContentToScrollToFlags;

  class nsDelayedEvent
  {
  public:
    virtual ~nsDelayedEvent() {};
    virtual void Dispatch(PresShell* aShell) {}
  };

  class nsDelayedInputEvent : public nsDelayedEvent
  {
  public:
    virtual void Dispatch(PresShell* aShell)
    {
      if (mEvent && mEvent->widget) {
        nsCOMPtr<nsIWidget> w = mEvent->widget;
        nsEventStatus status;
        w->DispatchEvent(mEvent, status);
      }
    }

  protected:
    void Init(nsInputEvent* aEvent)
    {
      mEvent->time = aEvent->time;
      mEvent->refPoint = aEvent->refPoint;
      mEvent->isShift = aEvent->isShift;
      mEvent->isControl = aEvent->isControl;
      mEvent->isAlt = aEvent->isAlt;
      mEvent->isMeta = aEvent->isMeta;
    }

    nsDelayedInputEvent()
    : nsDelayedEvent(), mEvent(nsnull) {}

    nsInputEvent* mEvent;
  };

  class nsDelayedMouseEvent : public nsDelayedInputEvent
  {
  public:
    nsDelayedMouseEvent(nsMouseEvent* aEvent) : nsDelayedInputEvent()
    {
      mEvent = new nsMouseEvent(NS_IS_TRUSTED_EVENT(aEvent),
                                aEvent->message,
                                aEvent->widget,
                                aEvent->reason,
                                aEvent->context);
      if (mEvent) {
        Init(aEvent);
        static_cast<nsMouseEvent*>(mEvent)->clickCount = aEvent->clickCount;
      }
    }

    virtual ~nsDelayedMouseEvent()
    {
      delete static_cast<nsMouseEvent*>(mEvent);
    }
  };

  class nsDelayedKeyEvent : public nsDelayedInputEvent
  {
  public:
    nsDelayedKeyEvent(nsKeyEvent* aEvent) : nsDelayedInputEvent()
    {
      mEvent = new nsKeyEvent(NS_IS_TRUSTED_EVENT(aEvent),
                              aEvent->message,
                              aEvent->widget);
      if (mEvent) {
        Init(aEvent);
        static_cast<nsKeyEvent*>(mEvent)->keyCode = aEvent->keyCode;
        static_cast<nsKeyEvent*>(mEvent)->charCode = aEvent->charCode;
        static_cast<nsKeyEvent*>(mEvent)->alternativeCharCodes =
          aEvent->alternativeCharCodes;
        static_cast<nsKeyEvent*>(mEvent)->isChar = aEvent->isChar;
      }
    }

    virtual ~nsDelayedKeyEvent()
    {
      delete static_cast<nsKeyEvent*>(mEvent);
    }
  };

  PRPackedBool                         mNoDelayedMouseEvents;
  PRPackedBool                         mNoDelayedKeyEvents;
  nsTArray<nsAutoPtr<nsDelayedEvent> > mDelayedEvents;

  nsCallbackEventRequest* mFirstCallbackEventRequest;
  nsCallbackEventRequest* mLastCallbackEventRequest;

  PRPackedBool      mIsDocumentGone;      
                                          
                                          
  PRPackedBool      mShouldUnsuppressPainting;  
                                                
  nsCOMPtr<nsITimer> mPaintSuppressionTimer; 
                                             
                                             
#define PAINTLOCK_EVENT_DELAY 250 // 250ms.  This is actually
                                  
                                  
                                  

  static void sPaintSuppressionCallback(nsITimer* aTimer, void* aPresShell); 

  
  
  
  
  nsCOMPtr<nsITimer> mReflowContinueTimer;
  static void sReflowContinueCallback(nsITimer* aTimer, void* aPresShell);
  PRBool ScheduleReflowOffTimer();
  
#ifdef MOZ_REFLOW_PERF
  ReflowCountMgr * mReflowCountMgr;
#endif

  static PRBool sDisableNonTestMouseEvents;

private:

  PRBool InZombieDocument(nsIContent *aContent);
  already_AddRefed<nsIPresShell> GetParentPresShell();
  nsresult RetargetEventToParent(nsGUIEvent* aEvent,
                                 nsEventStatus*  aEventStatus);

  
protected:
  
  nsIFrame* GetCurrentEventFrame();
private:
  void PushCurrentEventInfo(nsIFrame* aFrame, nsIContent* aContent);
  void PopCurrentEventInfo();
  nsresult HandleEventInternal(nsEvent* aEvent, nsIView* aView,
                               nsEventStatus *aStatus);
  nsresult HandlePositionedEvent(nsIView*       aView,
                                 nsIFrame*      aTargetFrame,
                                 nsGUIEvent*    aEvent,
                                 nsEventStatus* aEventStatus);
  
  
  already_AddRefed<nsPIDOMWindow> GetFocusedDOMWindowInOurWindow();

  














  PRBool AdjustContextMenuKeyEvent(nsMouseEvent* aEvent);

  
  PRBool PrepareToUseCaretPosition(nsIWidget* aEventWidget, nsIntPoint& aTargetPt);

  
  
  void GetCurrentItemAndPositionForElement(nsIDOMElement *aCurrentEl,
                                           nsIContent **aTargetToUse,
                                           nsIntPoint& aTargetPt,
                                           nsIWidget *aRootWidget);

  void FireResizeEvent();
  void FireBeforeResizeEvent();
  static void AsyncResizeEventCallback(nsITimer* aTimer, void* aPresShell);
  nsRevocableEventPtr<nsRunnableMethod<PresShell> > mResizeEvent;
  nsCOMPtr<nsITimer> mAsyncResizeEventTimer;
  PRPackedBool mAsyncResizeTimerIsActive;
  PRPackedBool mInResize;

  virtual void SynthesizeMouseMove(PRBool aFromScroll);

  
  
  void RecordMouseLocation(nsGUIEvent* aEvent);
  
  
  
  
  
  
  
  
  nsPoint mMouseLocation;
  class nsSynthMouseMoveEvent : public nsRunnable {
  public:
    nsSynthMouseMoveEvent(PresShell* aPresShell, PRBool aFromScroll)
      : mPresShell(aPresShell), mFromScroll(aFromScroll) {
      NS_ASSERTION(mPresShell, "null parameter");
    }
    void Revoke() { mPresShell = nsnull; }
    NS_IMETHOD Run() {
      if (mPresShell)
        mPresShell->ProcessSynthMouseMoveEvent(mFromScroll);
      return NS_OK;
    }
  private:
    PresShell* mPresShell;
    PRBool mFromScroll;
  };
  nsRevocableEventPtr<nsSynthMouseMoveEvent> mSynthMouseMoveEvent;
  void ProcessSynthMouseMoveEvent(PRBool aFromScroll);

  PresShell* GetRootPresShell();

private:
#ifdef DEBUG
  
  PRUint32 mPresArenaAllocCount;
#endif

public:

  PRUint32 EstimateMemoryUsed() {
    PRUint32 result = 0;

    result += sizeof(PresShell);
    result += mStackArena.Size();
    result += mFrameArena.Size();

    return result;
  }

  static PLDHashOperator LiveShellSizeEnumerator(PresShellPtrKey *aEntry,
                                                 void *userArg)
  {
    PresShell *aShell = static_cast<PresShell*>(aEntry->GetKey());
    PRUint32 *val = (PRUint32*)userArg;
    *val += aShell->EstimateMemoryUsed();
    *val += aShell->mPresContext->EstimateMemoryUsed();
    return PL_DHASH_NEXT;
  }

  static PLDHashOperator LiveShellBidiSizeEnumerator(PresShellPtrKey *aEntry,
                                                     void *userArg)
  {
    PresShell *aShell = static_cast<PresShell*>(aEntry->GetKey());
    PRUint32 *val = (PRUint32*)userArg;
    *val += aShell->mPresContext->GetBidiMemoryUsed();
    return PL_DHASH_NEXT;
  }

  static PRUint32
  EstimateShellsMemory(nsTHashtable<PresShellPtrKey>::Enumerator aEnumerator)
  {
    PRUint32 result = 0;
    sLiveShells->EnumerateEntries(aEnumerator, &result);
    return result;
  }
                  
                                  
  static PRInt64 SizeOfLayoutMemoryReporter(void *) {
    return EstimateShellsMemory(LiveShellSizeEnumerator);
  }

  static PRInt64 SizeOfBidiMemoryReporter(void *) {
    return EstimateShellsMemory(LiveShellBidiSizeEnumerator);
  }

protected:
  void QueryIsActive();
  nsresult UpdateImageLockingState();
};

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
      nsIFrame* frame = mPresShell->GetCurrentEventFrame();
      if (frame) {
        frame->HandleEvent(aVisitor.mPresContext,
                           (nsGUIEvent*) aVisitor.mEvent,
                           &aVisitor.mEventStatus);
      }
    }
  }

  nsRefPtr<PresShell> mPresShell;
};

PRBool PresShell::sDisableNonTestMouseEvents = PR_FALSE;

#ifdef PR_LOGGING
PRLogModuleInfo* PresShell::gLog;
#endif

#ifdef NS_DEBUG
static void
VerifyStyleTree(nsPresContext* aPresContext, nsFrameManager* aFrameManager)
{
  if (nsFrame::GetVerifyStyleTreeEnable()) {
    nsIFrame* rootFrame = aFrameManager->GetRootFrame();
    aFrameManager->DebugVerifyStyleTree(rootFrame);
  }
}
#define VERIFY_STYLE_TREE ::VerifyStyleTree(mPresContext, FrameManager())
#else
#define VERIFY_STYLE_TREE
#endif

static PRBool gVerifyReflowEnabled;

PRBool
nsIPresShell::GetVerifyReflowEnable()
{
#ifdef NS_DEBUG
  static PRBool firstTime = PR_TRUE;
  if (firstTime) {
    firstTime = PR_FALSE;
    char* flags = PR_GetEnv("GECKO_VERIFY_REFLOW_FLAGS");
    if (flags) {
      PRBool error = PR_FALSE;

      for (;;) {
        char* comma = PL_strchr(flags, ',');
        if (comma)
          *comma = '\0';

        PRBool found = PR_FALSE;
        const VerifyReflowFlags* flag = gFlags;
        const VerifyReflowFlags* limit = gFlags + NUM_VERIFY_REFLOW_FLAGS;
        while (flag < limit) {
          if (PL_strcasecmp(flag->name, flags) == 0) {
            gVerifyReflowFlags |= flag->bit;
            found = PR_TRUE;
            break;
          }
          ++flag;
        }

        if (! found)
          error = PR_TRUE;

        if (! comma)
          break;

        *comma = ',';
        flags = comma + 1;
      }

      if (error)
        ShowVerifyReflowFlags();
    }

    if (VERIFY_REFLOW_ON & gVerifyReflowFlags) {
      gVerifyReflowEnabled = PR_TRUE;

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
nsIPresShell::SetVerifyReflowEnable(PRBool aEnabled)
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
  NS_PRECONDITION(nsnull != aInstancePtrResult, "null ptr");

  if (!aInstancePtrResult)
    return NS_ERROR_NULL_POINTER;

  *aInstancePtrResult = new PresShell();
  if (!*aInstancePtrResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aInstancePtrResult);
  return NS_OK;
}

nsTHashtable<PresShell::PresShellPtrKey> *nsIPresShell::sLiveShells = 0;

NS_MEMORY_REPORTER_IMPLEMENT(LayoutPresShell,
                             "heap-used/layout/all",
                             "Memory used by layout PresShell, PresContext, "
                             "and other related areas.",
                             PresShell::SizeOfLayoutMemoryReporter,
                             nsnull)

NS_MEMORY_REPORTER_IMPLEMENT(LayoutBidi,
                             "heap-used/layout/bidi",
                             "Memory used by layout Bidi processor.",
                             PresShell::SizeOfBidiMemoryReporter,
                             nsnull)

PresShell::PresShell()
  : mMouseLocation(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE)
{
  mSelection = nsnull;
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
  mIsThemeSupportDisabled = PR_FALSE;
  mIsActive = PR_TRUE;
  mFrozen = PR_FALSE;
#ifdef DEBUG
  mPresArenaAllocCount = 0;
#endif
  mRenderFlags = 0;
  mXResolution = 1.0;
  mYResolution = 1.0;
  mViewportOverridden = PR_FALSE;

  static bool registeredReporter = false;
  if (!registeredReporter) {
    NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(LayoutPresShell));
    NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(LayoutBidi));
    registeredReporter = true;
  }

  new (this) nsFrameManager();

  sLiveShells->PutEntry(this);
}

NS_IMPL_ISUPPORTS8(PresShell, nsIPresShell, nsIDocumentObserver,
                   nsIViewObserver, nsISelectionController,
                   nsISelectionDisplay, nsIObserver, nsISupportsWeakReference,
                   nsIMutationObserver)

PresShell::~PresShell()
{
  sLiveShells->RemoveEntry(this);

  if (!mHaveShutDown) {
    NS_NOTREACHED("Someone did not call nsIPresShell::destroy");
    Destroy();
  }

  NS_ASSERTION(mCurrentEventContentStack.Count() == 0,
               "Huh, event content left on the stack in pres shell dtor!");
  NS_ASSERTION(mFirstCallbackEventRequest == nsnull &&
               mLastCallbackEventRequest == nsnull,
               "post-reflow queues not empty.  This means we're leaking");

#ifdef DEBUG
  NS_ASSERTION(mPresArenaAllocCount == 0,
               "Some pres arena objects were not freed");
#endif

  delete mStyleSet;
  delete mFrameConstructor;

  mCurrentEventContent = nsnull;

  NS_IF_RELEASE(mPresContext);
  NS_IF_RELEASE(mDocument);
  NS_IF_RELEASE(mSelection);
}





nsresult
PresShell::Init(nsIDocument* aDocument,
                nsPresContext* aPresContext,
                nsIViewManager* aViewManager,
                nsStyleSet* aStyleSet,
                nsCompatibility aCompatMode)
{
  NS_TIME_FUNCTION_MIN(1.0);

  NS_PRECONDITION(nsnull != aDocument, "null ptr");
  NS_PRECONDITION(nsnull != aPresContext, "null ptr");
  NS_PRECONDITION(nsnull != aViewManager, "null ptr");
  nsresult result;

  if ((nsnull == aDocument) || (nsnull == aPresContext) ||
      (nsnull == aViewManager)) {
    return NS_ERROR_NULL_POINTER;
  }
  if (mDocument) {
    NS_WARNING("PresShell double init'ed");
    return NS_ERROR_ALREADY_INITIALIZED;
  }
  result = mStackArena.Init();
  NS_ENSURE_SUCCESS(result, result);

  if (!mFramesToDirty.Init()) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  mDocument = aDocument;
  NS_ADDREF(mDocument);
  mViewManager = aViewManager;

  
  mFrameConstructor = new nsCSSFrameConstructor(mDocument, this);
  NS_ENSURE_TRUE(mFrameConstructor, NS_ERROR_OUT_OF_MEMORY);

  
  mViewManager->SetViewObserver(this);

  
  mPresContext = aPresContext;
  NS_ADDREF(mPresContext);
  aPresContext->SetShell(this);

  
  result = aStyleSet->Init(aPresContext);
  NS_ENSURE_SUCCESS(result, result);

  
  
  
  
  mStyleSet = aStyleSet;

  
  
  
  mPresContext->CompatibilityModeChanged();

  
  
  SetPreferenceStyleRules(PR_FALSE);

  result = CallCreateInstance(kFrameSelectionCID, &mSelection);
  if (NS_FAILED(result)) {
    mStyleSet = nsnull;
    return result;
  }

  
  result = FrameManager()->Init(this, mStyleSet);
  if (NS_FAILED(result)) {
    NS_WARNING("Frame manager initialization failed");
    mStyleSet = nsnull;
    return result;
  }

  mSelection->Init(this, nsnull);

  
#ifdef SHOW_CARET
  
  nsresult  err = NS_NewCaret(getter_AddRefs(mCaret));
  if (NS_SUCCEEDED(err))
  {
    mCaret->Init(this);
    mOriginalCaret = mCaret;
  }

  
#endif  
  
  
  nsPresContext::nsPresContextType type = aPresContext->Type();
  if (type != nsPresContext::eContext_PrintPreview &&
      type != nsPresContext::eContext_Print)
    SetDisplaySelection(nsISelectionController::SELECTION_DISABLED);
  
  if (gMaxRCProcessingTime == -1) {
    gMaxRCProcessingTime =
      nsContentUtils::GetIntPref("layout.reflow.timeslice",
                                 NS_MAX_REFLOW_TIME);
  }

  {
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (os) {
      os->AddObserver(this, "agent-sheet-added", PR_FALSE);
      os->AddObserver(this, "user-sheet-added", PR_FALSE);
      os->AddObserver(this, "agent-sheet-removed", PR_FALSE);
      os->AddObserver(this, "user-sheet-removed", PR_FALSE);
#ifdef MOZ_XUL
      os->AddObserver(this, "chrome-flush-skin-caches", PR_FALSE);
#endif
#ifdef ACCESSIBILITY
      os->AddObserver(this, "a11y-init-or-shutdown", PR_FALSE);
#endif
    }
  }

  
  mDragService = do_GetService("@mozilla.org/widget/dragservice;1");

#ifdef MOZ_REFLOW_PERF
    if (mReflowCountMgr) {
      PRBool paintFrameCounts =
        nsContentUtils::GetBoolPref("layout.reflow.showframecounts");

      PRBool dumpFrameCounts =
        nsContentUtils::GetBoolPref("layout.reflow.dumpframecounts");

      PRBool dumpFrameByFrameCounts =
        nsContentUtils::GetBoolPref("layout.reflow.dumpframebyframecounts");

      mReflowCountMgr->SetDumpFrameCounts(dumpFrameCounts);
      mReflowCountMgr->SetDumpFrameByFrameCounts(dumpFrameByFrameCounts);
      mReflowCountMgr->SetPaintFrameCounts(paintFrameCounts);
    }
#endif

#ifdef MOZ_SMIL
  if (mDocument->HasAnimationController()) {
    nsSMILAnimationController* animCtrl = mDocument->GetAnimationController();
    animCtrl->NotifyRefreshDriverCreated(GetPresContext()->RefreshDriver());
  }
#endif 

  
  QueryIsActive();

  return NS_OK;
}

void
PresShell::Destroy()
{
  NS_TIME_FUNCTION_MIN(1.0);

  NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
    "destroy called on presshell while scripts not blocked");

#ifdef MOZ_REFLOW_PERF
  DumpReflows();
  if (mReflowCountMgr) {
    delete mReflowCountMgr;
    mReflowCountMgr = nsnull;
  }
#endif

  if (mHaveShutDown)
    return;

#ifdef ACCESSIBILITY
  nsAccessibilityService* accService = AccService();
  if (accService) {
    accService->PresShellDestroyed(this);
  }
#endif 

  MaybeReleaseCapturingContent();

  if (gKeyDownTarget && gKeyDownTarget->GetOwnerDoc() == mDocument) {
    NS_RELEASE(gKeyDownTarget);
  }

  mContentToScrollTo = nsnull;

  if (mPresContext) {
    
    
    mPresContext->EventStateManager()->NotifyDestroyPresContext(mPresContext);
  }

  {
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (os) {
      os->RemoveObserver(this, "agent-sheet-added");
      os->RemoveObserver(this, "user-sheet-added");
      os->RemoveObserver(this, "agent-sheet-removed");
      os->RemoveObserver(this, "user-sheet-removed");
#ifdef MOZ_XUL
      os->RemoveObserver(this, "chrome-flush-skin-caches");
#endif
#ifdef ACCESSIBILITY
      os->RemoveObserver(this, "a11y-init-or-shutdown");
#endif
    }
  }

  
  if (mPaintSuppressionTimer) {
    mPaintSuppressionTimer->Cancel();
    mPaintSuppressionTimer = nsnull;
  }

  
  if (mReflowContinueTimer) {
    mReflowContinueTimer->Cancel();
    mReflowContinueTimer = nsnull;
  }

  mSynthMouseMoveEvent.Revoke();

  if (mCaret) {
    mCaret->Terminate();
    mCaret = nsnull;
  }

  if (mSelection) {
    mSelection->DisconnectFromPresShell();
  }

  
  ClearPreferenceStyleRules();

  mIsDestroying = PR_TRUE;

  
  
  
  

  
  
  
  

  mCurrentEventFrame = nsnull;

  PRInt32 i, count = mCurrentEventFrameStack.Length();
  for (i = 0; i < count; i++) {
    mCurrentEventFrameStack[i] = nsnull;
  }

  mFramesToDirty.Clear();

  if (mViewManager) {
    
    
    mViewManager->SetViewObserver(nsnull);
    mViewManager = nsnull;
  }

  mStyleSet->BeginShutdown(mPresContext);
  nsRefreshDriver* rd = GetPresContext()->RefreshDriver();

  
  
  
  if (mDocument) {
    NS_ASSERTION(mDocument->GetShell() == this, "Wrong shell?");
    mDocument->DeleteShell();

#ifdef MOZ_SMIL
    if (mDocument->HasAnimationController()) {
      mDocument->GetAnimationController()->NotifyRefreshDriverDestroying(rd);
    }
#endif 
  }

  
  
  
  rd->RemoveLayoutFlushObserver(this);
  mResizeEvent.Revoke();
  if (mAsyncResizeTimerIsActive) {
    mAsyncResizeEventTimer->Cancel();
    mAsyncResizeTimerIsActive = PR_FALSE;
  }

  CancelAllPendingReflows();
  CancelPostedReflowCallbacks();

  
  mFrameConstructor->WillDestroyFrameTree();
  FrameManager()->Destroy();

  
  
  
  if (mPresContext) {
    
    
    
    
    
    mPresContext->PropertyTable()->DeleteAll();
  }


  NS_WARN_IF_FALSE(!mWeakFrames, "Weak frames alive after destroying FrameManager");
  while (mWeakFrames) {
    mWeakFrames->Clear(this);
  }

  
  mStyleSet->Shutdown(mPresContext);

  if (mPresContext) {
    
    
    
    mPresContext->SetShell(nsnull);

    
    mPresContext->SetLinkHandler(nsnull);
  }

  mHaveShutDown = PR_TRUE;
}

                  
 void
PresShell::PushStackMemory()
{
  mStackArena.Push();
}

 void
PresShell::PopStackMemory()
{
  mStackArena.Pop();
}

 void*
PresShell::AllocateStackMemory(size_t aSize)
{
  return mStackArena.Allocate(aSize);
}

void
PresShell::FreeFrame(nsQueryFrame::FrameIID aCode, void* aPtr)
{
#ifdef DEBUG
  mPresArenaAllocCount--;
#endif
  if (PRESARENA_MUST_FREE_DURING_DESTROY || !mIsDestroying)
    mFrameArena.FreeByCode(aCode, aPtr);
}

void*
PresShell::AllocateFrame(nsQueryFrame::FrameIID aCode, size_t aSize)
{
#ifdef DEBUG
  mPresArenaAllocCount++;
#endif
  void* result = mFrameArena.AllocateByCode(aCode, aSize);

  if (result) {
    memset(result, 0, aSize);
  }
  return result;
}

void
PresShell::FreeMisc(size_t aSize, void* aPtr)
{
#ifdef DEBUG
  mPresArenaAllocCount--;
#endif
  if (PRESARENA_MUST_FREE_DURING_DESTROY || !mIsDestroying)
    mFrameArena.FreeBySize(aSize, aPtr);
}

void*
PresShell::AllocateMisc(size_t aSize)
{
#ifdef DEBUG
  mPresArenaAllocCount++;
#endif
  return mFrameArena.AllocateBySize(aSize);
}

void
nsIPresShell::SetAuthorStyleDisabled(PRBool aStyleDisabled)
{
  if (aStyleDisabled != mStyleSet->GetAuthorStyleDisabled()) {
    mStyleSet->SetAuthorStyleDisabled(aStyleDisabled);
    ReconstructStyleData();
  }
}

PRBool
nsIPresShell::GetAuthorStyleDisabled() const
{
  return mStyleSet->GetAuthorStyleDisabled();
}

nsresult
PresShell::SetPreferenceStyleRules(PRBool aForceReflow)
{
  NS_TIME_FUNCTION_MIN(1.0);

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
      
      
#ifdef NS_DEBUG
      PRInt32 numBefore = mStyleSet->SheetCount(nsStyleSet::eUserSheet);
      NS_ASSERTION(numBefore > 0, "no user stylesheets in styleset, but we have one!");
#endif
      mStyleSet->RemoveStyleSheet(nsStyleSet::eUserSheet, mPrefStyleSheet);

#ifdef DEBUG_attinasi
      NS_ASSERTION((numBefore - 1) == mStyleSet->GetNumberOfUserStyleSheets(),
                   "Pref stylesheet was not removed");
      printf("PrefStyleSheet removed\n");
#endif
      
      mPrefStyleSheet = nsnull;
    }
  }
  return result;
}

nsresult PresShell::CreatePreferenceStyleSheet(void)
{
  NS_TIME_FUNCTION_MIN(1.0);

  NS_ASSERTION(!mPrefStyleSheet, "prefStyleSheet already exists");
  nsresult result = NS_NewCSSStyleSheet(getter_AddRefs(mPrefStyleSheet));
  if (NS_SUCCEEDED(result)) {
    NS_ASSERTION(mPrefStyleSheet, "null but no error");
    nsCOMPtr<nsIURI> uri;
    result = NS_NewURI(getter_AddRefs(uri), "about:PreferenceStyleSheet", nsnull);
    if (NS_SUCCEEDED(result)) {
      NS_ASSERTION(uri, "null but no error");
      mPrefStyleSheet->SetURIs(uri, uri, uri);
      mPrefStyleSheet->SetComplete();
      PRUint32 index;
      result =
        mPrefStyleSheet->InsertRuleInternal(NS_LITERAL_STRING("@namespace url(http://www.w3.org/1999/xhtml);"),
                                            0, &index);
      if (NS_SUCCEEDED(result)) {
        mStyleSet->AppendStyleSheet(nsStyleSet::eUserSheet, mPrefStyleSheet);
      }
    }
  }

#ifdef DEBUG_attinasi
  printf("CreatePrefStyleSheet completed: error=%ld\n",(long)result);
#endif

  if (NS_FAILED(result)) {
    mPrefStyleSheet = nsnull;
  }

  return result;
}




static PRUint32 sInsertPrefSheetRulesAt = 1;

nsresult
PresShell::SetPrefNoScriptRule()
{
  NS_TIME_FUNCTION_MIN(1.0);

  nsresult rv = NS_OK;

  
  
  nsIDocument* doc = mDocument;
  if (mPresContext->Type() == nsPresContext::eContext_PrintPreview ||
      mPresContext->Type() == nsPresContext::eContext_Print) {
    while (doc->GetOriginalDocument()) {
      doc = doc->GetOriginalDocument();
    }
  }

  PRBool scriptEnabled = doc->IsScriptEnabled();
  if (scriptEnabled) {
    if (!mPrefStyleSheet) {
      rv = CreatePreferenceStyleSheet();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    PRUint32 index = 0;
    mPrefStyleSheet->
      InsertRuleInternal(NS_LITERAL_STRING("noscript{display:none!important}"),
                         sInsertPrefSheetRulesAt, &index);
  }

  return rv;
}

nsresult PresShell::SetPrefNoFramesRule(void)
{
  NS_TIME_FUNCTION_MIN(1.0);

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
  
  PRBool allowSubframes = PR_TRUE;
  nsCOMPtr<nsISupports> container = mPresContext->GetContainer();     
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(container));
  if (docShell) {
    docShell->GetAllowSubframes(&allowSubframes);
  }
  if (!allowSubframes) {
    PRUint32 index = 0;
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
  NS_TIME_FUNCTION_MIN(1.0);

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
  PRUint32 index = 0;
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

  PRBool underlineLinks =
    mPresContext->GetCachedBoolPref(kPresContext_UnderlineLinks);

  if (underlineLinks) {
    
    
    
    
    
    rv = mPrefStyleSheet->
      InsertRuleInternal(NS_LITERAL_STRING("*|*:-moz-any-link{text-decoration:underline}"),
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
  NS_TIME_FUNCTION_MIN(1.0);

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

      
      PRUint32 index = 0;
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
    PRUint8 focusRingWidth = mPresContext->FocusRingWidth();
    PRBool focusRingOnAnything = mPresContext->GetFocusRingOnAnything();
    PRUint8 focusRingStyle = mPresContext->GetFocusRingStyle();

    if ((NS_SUCCEEDED(result) && focusRingWidth != 1 && focusRingWidth <= 4 ) || focusRingOnAnything) {
      PRUint32 index = 0;
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
  PRInt32 i;
  
  
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
PresShell::SetDisplaySelection(PRInt16 aToggle)
{
  mSelection->SetDisplaySelection(aToggle);
  return NS_OK;
}

NS_IMETHODIMP
PresShell::GetDisplaySelection(PRInt16 *aToggle)
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

nsISelection*
PresShell::GetCurrentSelection(SelectionType aType)
{
  if (!mSelection)
    return nsnull;

  return mSelection->GetSelection(aType);
}

NS_IMETHODIMP
PresShell::ScrollSelectionIntoView(SelectionType aType, SelectionRegion aRegion,
                                   PRInt16 aFlags)
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
      mIsDocumentGone = PR_FALSE;
    }
  }
}


void
PresShell::EndObservingDocument()
{
  
  
  mIsDocumentGone = PR_TRUE;
  if (mDocument) {
    mDocument->RemoveObserver(this);
  }
}

#ifdef DEBUG_kipp
char* nsPresShell_ReflowStackPointerTop;
#endif

nsresult
PresShell::InitialReflow(nscoord aWidth, nscoord aHeight)
{
  if (mIsDestroying) {
    return NS_OK;
  }

  if (!mDocument) {
    
    return NS_OK;
  }

  NS_TIME_FUNCTION_WITH_DOCURL;

  NS_ASSERTION(!mDidInitialReflow, "Why are we being called?");

  nsCOMPtr<nsIPresShell> kungFuDeathGrip(this);
  mDidInitialReflow = PR_TRUE;

#ifdef NS_DEBUG
  if (VERIFY_REFLOW_NOISY_RC & gVerifyReflowFlags) {
    if (mDocument) {
      nsIURI *uri = mDocument->GetDocumentURI();
      if (uri) {
        nsCAutoString url;
        uri->GetSpec(url);
        printf("*** PresShell::InitialReflow (this=%p, url='%s')\n", (void*)this, url.get());
      }
    }
  }
#endif

  if (mCaret)
    mCaret->EraseCaret();

  
  

  mPresContext->SetVisibleArea(nsRect(0, 0, aWidth, aHeight));

  
  
  
  
  nsIFrame* rootFrame = FrameManager()->GetRootFrame();
  NS_ASSERTION(!rootFrame, "How did that happen, exactly?");
  if (!rootFrame) {
    nsAutoScriptBlocker scriptBlocker;
    mFrameConstructor->BeginUpdate();
    mFrameConstructor->ConstructRootFrame(&rootFrame);
    FrameManager()->SetRootFrame(rootFrame);
    mFrameConstructor->EndUpdate();
  }

  NS_ENSURE_STATE(!mHaveShutDown);

  if (!rootFrame) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  Element *root = mDocument->GetRootElement();

  if (root) {
    {
      nsAutoCauseReflowNotifier reflowNotifier(this);
      mFrameConstructor->BeginUpdate();

      
      
      mFrameConstructor->ContentInserted(nsnull, root, nsnull, PR_FALSE);
      VERIFY_STYLE_TREE;

      
      
      NS_ENSURE_STATE(!mHaveShutDown);

      mFrameConstructor->EndUpdate();
    }

    
    NS_ENSURE_STATE(!mHaveShutDown);

    
    mDocument->BindingManager()->ProcessAttachedQueue();

    NS_TIME_FUNCTION_MARK("XBL binding constructors fired");

    
    NS_ENSURE_STATE(!mHaveShutDown);

    
    
    {
      nsAutoScriptBlocker scriptBlocker;
      mFrameConstructor->CreateNeededFrames();
      mFrameConstructor->ProcessPendingRestyles();
    }

    
    NS_ENSURE_STATE(!mHaveShutDown);
  }

  NS_ASSERTION(rootFrame, "How did that happen?");

  
  
  NS_ASSERTION(!mDirtyRoots.Contains(rootFrame),
               "Why is the root in mDirtyRoots already?");

  rootFrame->RemoveStateBits(NS_FRAME_IS_DIRTY |
                             NS_FRAME_HAS_DIRTY_CHILDREN);
  FrameNeedsReflow(rootFrame, eResize, NS_FRAME_IS_DIRTY);

  NS_ASSERTION(mDirtyRoots.Contains(rootFrame),
               "Should be in mDirtyRoots now");
  NS_ASSERTION(mReflowScheduled, "Why no reflow scheduled?");

  
  
  
  
  if (!mDocumentLoading) {
    RestoreRootScrollPosition();
  }

  
  if (!mPresContext->IsPaginated()) {
    
    
    
    mPaintingSuppressed = PR_TRUE;
    
    nsIDocument::ReadyState readyState = mDocument->GetReadyStateEnum();
    if (readyState != nsIDocument::READYSTATE_COMPLETE) {
      mPaintSuppressionTimer = do_CreateInstance("@mozilla.org/timer;1");
    }
    if (!mPaintSuppressionTimer) {
      mPaintingSuppressed = PR_FALSE;
    } else {
      

      
      PRInt32 delay =
        nsContentUtils::GetIntPref("nglayout.initialpaint.delay",
                                   PAINTLOCK_EVENT_DELAY);

      mPaintSuppressionTimer->InitWithFuncCallback(sPaintSuppressionCallback,
                                                   this, delay, 
                                                   nsITimer::TYPE_ONE_SHOT);
    }
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
  mViewportOverridden = PR_TRUE;
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
  
  
  
  
  nsIFrame* rootFrame = FrameManager()->GetRootFrame();

  if (!rootFrame && aHeight == NS_UNCONSTRAINEDSIZE) {
    
    
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (!mIsDestroying && !mResizeEvent.IsPending() &&
      !mAsyncResizeTimerIsActive) {
    FireBeforeResizeEvent();
  }

  mPresContext->SetVisibleArea(nsRect(0, 0, aWidth, aHeight));

  
  if (!rootFrame)
    return NS_OK;

  NS_ASSERTION(mViewManager, "Must have view manager");
  nsCOMPtr<nsIViewManager> viewManagerDeathGrip = mViewManager;
  
  nsCOMPtr<nsIPresShell> kungFuDeathGrip(this);
  if (!GetPresContext()->SupressingResizeReflow())
  {
    nsIViewManager::UpdateViewBatch batch(mViewManager);

    
    
    mDocument->FlushPendingNotifications(Flush_ContentAndNotify);

    
    {
      nsAutoScriptBlocker scriptBlocker;
      mFrameConstructor->CreateNeededFrames();
      mFrameConstructor->ProcessPendingRestyles();
    }

    if (!mIsDestroying) {
      
      

      {
        nsAutoCauseReflowNotifier crNotifier(this);
        WillDoReflow();

        
        AUTO_LAYOUT_PHASE_ENTRY_POINT(GetPresContext(), Reflow);

        mDirtyRoots.RemoveElement(rootFrame);
        DoReflow(rootFrame, PR_TRUE);
      }

      DidDoReflow(PR_TRUE);
    }

    batch.EndUpdateViewBatch(NS_VMREFRESH_NO_SYNC);
  }

  if (aHeight == NS_UNCONSTRAINEDSIZE) {
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
        mAsyncResizeTimerIsActive = PR_TRUE;
        mAsyncResizeEventTimer->InitWithFuncCallback(AsyncResizeEventCallback,
                                                     this, 15,
                                                     nsITimer::TYPE_ONE_SHOT);
      }
    } else {
      nsRefPtr<nsRunnableMethod<PresShell> > resizeEvent =
        NS_NewRunnableMethod(this, &PresShell::FireResizeEvent);
      if (NS_SUCCEEDED(NS_DispatchToCurrentThread(resizeEvent))) {
        mResizeEvent = resizeEvent;
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

  
  nsEvent event(PR_TRUE, NS_BEFORERESIZE_EVENT);

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
    mAsyncResizeTimerIsActive = PR_FALSE;
    mAsyncResizeEventTimer->Cancel();
  }
  mResizeEvent.Revoke();

  if (mIsDocumentGone)
    return;

  
  nsEvent event(PR_TRUE, NS_RESIZE_EVENT);
  nsEventStatus status = nsEventStatus_eIgnore;

  nsPIDOMWindow *window = mDocument->GetWindow();
  if (window) {
    nsCOMPtr<nsIPresShell> kungFuDeathGrip(this);
    mInResize = PR_TRUE;
    nsEventDispatcher::Dispatch(window, mPresContext, &event, nsnull, &status);
    mInResize = PR_FALSE;
  }
}

void
PresShell::SetIgnoreFrameDestruction(PRBool aIgnore)
{
  mIgnoreFrameDestruction = aIgnore;
}

void
PresShell::NotifyDestroyingFrame(nsIFrame* aFrame)
{
  NS_TIME_FUNCTION_MIN(1.0);

  mPresContext->ForgetUpdatePluginGeometryFrame(aFrame);

  if (!mIgnoreFrameDestruction) {
    mPresContext->StopImagesFor(aFrame);

    mFrameConstructor->NotifyDestroyingFrame(aFrame);

    for (PRInt32 idx = mDirtyRoots.Length(); idx; ) {
      --idx;
      if (mDirtyRoots[idx] == aFrame) {
        mDirtyRoots.RemoveElementAt(idx);
      }
    }

    
    FrameManager()->NotifyDestroyingFrame(aFrame);

    
    mPresContext->NotifyDestroyingFrame(aFrame);

    if (aFrame == mCurrentEventFrame) {
      mCurrentEventContent = aFrame->GetContent();
      mCurrentEventFrame = nsnull;
    }

  #ifdef NS_DEBUG
    if (aFrame == mDrawEventTargetFrame) {
      mDrawEventTargetFrame = nsnull;
    }
  #endif

    for (unsigned int i=0; i < mCurrentEventFrameStack.Length(); i++) {
      if (aFrame == mCurrentEventFrameStack.ElementAt(i)) {
        
        
        nsIContent *currentEventContent = aFrame->GetContent();
        mCurrentEventContentStack.ReplaceObjectAt(currentEventContent, i);
        mCurrentEventFrameStack[i] = nsnull;
      }
    }
  
    mFramesToDirty.RemoveEntry(aFrame);
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

NS_IMETHODIMP PresShell::SetCaretEnabled(PRBool aInEnable)
{
  PRBool oldEnabled = mCaretEnabled;

  mCaretEnabled = aInEnable;

  if (mCaret && (mCaretEnabled != oldEnabled))
  {





    mCaret->SetCaretVisible(mCaretEnabled);
  }

  return NS_OK;
}

NS_IMETHODIMP PresShell::SetCaretReadOnly(PRBool aReadOnly)
{
  if (mCaret)
    mCaret->SetCaretReadOnly(aReadOnly);
  return NS_OK;
}

NS_IMETHODIMP PresShell::GetCaretEnabled(PRBool *aOutEnabled)
{
  NS_ENSURE_ARG_POINTER(aOutEnabled);
  *aOutEnabled = mCaretEnabled;
  return NS_OK;
}

NS_IMETHODIMP PresShell::SetCaretVisibilityDuringSelection(PRBool aVisibility)
{
  if (mCaret)
    mCaret->SetVisibilityDuringSelection(aVisibility);
  return NS_OK;
}

NS_IMETHODIMP PresShell::GetCaretVisible(PRBool *aOutIsVisible)
{
  *aOutIsVisible = PR_FALSE;
  if (mCaret) {
    nsresult rv = mCaret->GetCaretVisible(aOutIsVisible);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  return NS_OK;
}

NS_IMETHODIMP PresShell::SetSelectionFlags(PRInt16 aInEnable)
{
  mSelectionFlags = aInEnable;
  return NS_OK;
}

NS_IMETHODIMP PresShell::GetSelectionFlags(PRInt16 *aOutEnable)
{
  if (!aOutEnable)
    return NS_ERROR_INVALID_ARG;
  *aOutEnable = mSelectionFlags;
  return NS_OK;
}



NS_IMETHODIMP 
PresShell::CharacterMove(PRBool aForward, PRBool aExtend)
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
PresShell::WordMove(PRBool aForward, PRBool aExtend)
{
  return mSelection->WordMove(aForward, aExtend);  
}

NS_IMETHODIMP 
PresShell::WordExtendForDelete(PRBool aForward)
{
  return mSelection->WordExtendForDelete(aForward);  
}

NS_IMETHODIMP 
PresShell::LineMove(PRBool aForward, PRBool aExtend)
{
  nsresult result = mSelection->LineMove(aForward, aExtend);  


  if (NS_FAILED(result)) 
    result = CompleteMove(aForward,aExtend);
  return result;
}

NS_IMETHODIMP 
PresShell::IntraLineMove(PRBool aForward, PRBool aExtend)
{
  return mSelection->IntraLineMove(aForward, aExtend);  
}



NS_IMETHODIMP 
PresShell::PageMove(PRBool aForward, PRBool aExtend)
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
PresShell::ScrollPage(PRBool aForward)
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
PresShell::ScrollLine(PRBool aForward)
{
  nsIScrollableFrame* scrollFrame =
    GetFrameToScrollAsScrollable(nsIPresShell::eVertical);
  if (scrollFrame) {
    PRInt32 lineCount = 1;
#ifdef MOZ_WIDGET_COCOA
    
    
    lineCount = 2;
#endif
    scrollFrame->ScrollBy(nsIntPoint(0, aForward ? lineCount : -lineCount),
                          nsIScrollableFrame::LINES,
                          nsIScrollableFrame::SMOOTH);
      

    
    

  
    
    nsIViewManager* viewManager = GetViewManager();
    if (viewManager) {
      viewManager->ForceUpdate();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
PresShell::ScrollHorizontal(PRBool aLeft)
{
  nsIScrollableFrame* scrollFrame =
    GetFrameToScrollAsScrollable(nsIPresShell::eHorizontal);
  if (scrollFrame) {
    scrollFrame->ScrollBy(nsIntPoint(aLeft ? -1 : 1, 0),
                          nsIScrollableFrame::LINES,
                          nsIScrollableFrame::SMOOTH);

    
    

  
    
    nsIViewManager* viewManager = GetViewManager();
    if (viewManager) {
      viewManager->ForceUpdate();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
PresShell::CompleteScroll(PRBool aForward)
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
PresShell::CompleteMove(PRBool aForward, PRBool aExtend)
{
  
  
  nsIContent* limiter = mSelection->GetAncestorLimiter();
  nsIFrame* frame = limiter ? limiter->GetPrimaryFrame()
                            : FrameConstructor()->GetRootElementFrame();
  if (!frame)
    return NS_ERROR_FAILURE;
  nsPeekOffsetStruct pos = frame->GetExtremeCaretPosition(!aForward);
  mSelection->HandleClick(pos.mResultContent, pos.mContentOffset,
                          pos.mContentOffset, aExtend, PR_FALSE, aForward);
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

NS_IMETHODIMP
PresShell::CheckVisibility(nsIDOMNode *node, PRInt16 startOffset, PRInt16 EndOffset, PRBool *_retval)
{
  if (!node || startOffset>EndOffset || !_retval || startOffset<0 || EndOffset<0)
    return NS_ERROR_INVALID_ARG;
  *_retval = PR_FALSE; 
  nsCOMPtr<nsIContent> content(do_QueryInterface(node));
  if (!content)
    return NS_ERROR_FAILURE;
  nsIFrame *frame = content->GetPrimaryFrame();
  if (!frame) 
    return NS_OK;  
  
  
  PRBool finished = PR_FALSE;
  frame->CheckVisibility(mPresContext,startOffset,EndOffset,PR_TRUE,&finished, _retval);
  return NS_OK;
}




void
PresShell::StyleChangeReflow()
{
  nsIFrame* rootFrame = FrameManager()->GetRootFrame();
  
  
  if (!rootFrame)
    return;

  FrameNeedsReflow(rootFrame, eStyleChange, NS_FRAME_IS_DIRTY);
}

nsIFrame*
nsIPresShell::GetRootFrameExternal() const
{
  return FrameManager()->GetRootFrame();
}

nsIFrame*
nsIPresShell::GetRootScrollFrame() const
{
  nsIFrame* rootFrame = FrameManager()->GetRootFrame();
  
  if (!rootFrame || nsGkAtoms::viewportFrame != rootFrame->GetType())
    return nsnull;
  nsIFrame* theFrame = rootFrame->GetFirstChild(nsnull);
  if (!theFrame || nsGkAtoms::scrollFrame != theFrame->GetType())
    return nsnull;
  return theFrame;
}

nsIScrollableFrame*
nsIPresShell::GetRootScrollFrameAsScrollable() const
{
  nsIFrame* frame = GetRootScrollFrame();
  if (!frame)
    return nsnull;
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

nsIFrame*
PresShell::GetFrameForPoint(nsIFrame* aFrame, nsPoint aPt)
{
  return nsLayoutUtils::GetFrameForPoint(aFrame, aPt);
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
    if (mStylesHaveChanged)
      ReconstructStyleData();
  }

  mFrameConstructor->EndUpdate();
}

void
PresShell::RestoreRootScrollPosition()
{
  
  nsCOMPtr<nsILayoutHistoryState> historyState =
    mDocument->GetLayoutHistoryState();
  
  
  
  
  nsAutoScriptBlocker scriptBlocker;
  ++mChangeNestCount;

  if (historyState) {
    nsIFrame* scrollFrame = GetRootScrollFrame();
    if (scrollFrame) {
      nsIScrollableFrame* scrollableFrame = do_QueryFrame(scrollFrame);
      if (scrollableFrame) {
        FrameManager()->RestoreFrameStateFor(scrollFrame, historyState,
                                             nsIStatefulFrame::eDocumentScrollState);
        scrollableFrame->ScrollToRestoredPosition();
      }
    }
  }

  --mChangeNestCount;
}

void
PresShell::BeginLoad(nsIDocument *aDocument)
{  
  mDocumentLoading = PR_TRUE;
}

void
PresShell::EndLoad(nsIDocument *aDocument)
{
  NS_PRECONDITION(aDocument == mDocument, "Wrong document");
  
  RestoreRootScrollPosition();
  
  mDocumentLoading = PR_FALSE;
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
#ifdef NS_FUNCTION_TIMER
  NS_TIME_FUNCTION_DECLARE_DOCURL;
  nsCAutoString frameType__("N/A");
  nsIAtom *atomType__ = aFrame ? aFrame->GetType() : nsnull;
  if (atomType__) atomType__->ToUTF8String(frameType__);
  NS_TIME_FUNCTION_MIN_FMT(1.0, "%s (line %d) (document: %s, frame type: %s)", MOZ_FUNCTION_NAME,
                           __LINE__, docURL__.get(), frameType__.get());
#endif

  NS_PRECONDITION(aBitToAdd == NS_FRAME_IS_DIRTY ||
                  aBitToAdd == NS_FRAME_HAS_DIRTY_CHILDREN,
                  "Unexpected bits being added");
  NS_PRECONDITION(aIntrinsicDirty != eStyleChange ||
                  aBitToAdd == NS_FRAME_IS_DIRTY,
                  "bits don't correspond to style change reason");

  NS_ASSERTION(!mIsReflowing, "can't mark frame dirty during reflow");

  
  
  if (! mDidInitialReflow)
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

    
    
    PRBool wasDirty = NS_SUBTREE_DIRTY(subtreeRoot);
    subtreeRoot->AddStateBits(aBitToAdd);

    
    
    PRBool targetFrameDirty = (aBitToAdd == NS_FRAME_IS_DIRTY);

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

        PRInt32 childListIndex = 0;
        nsIAtom *childListName;
        do {
          childListName = f->GetAdditionalChildListName(childListIndex++);
          for (nsIFrame *kid = f->GetFirstChild(childListName); kid;
               kid = kid->GetNextSibling()) {
            kid->MarkIntrinsicWidthsDirty();
            stack.AppendElement(kid);
          }
        } while (childListName);
      } while (stack.Length() != 0);
    }

    
    
    
    nsIFrame *f = subtreeRoot;
    for (;;) {
      if (FRAME_IS_REFLOW_ROOT(f) || !f->GetParent()) {
        
        if (!wasDirty) {
          mDirtyRoots.AppendElement(f);
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
  nsIScrollableFrame* scrollFrame = nsnull;

  nsCOMPtr<nsIContent> focusedContent;
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (fm && mDocument) {
    nsCOMPtr<nsIDOMWindow> window = do_QueryInterface(mDocument->GetWindow());

    nsCOMPtr<nsIDOMElement> focusedElement;
    fm->GetFocusedElementForWindow(window, PR_FALSE, nsnull, getter_AddRefs(focusedElement));
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
    mReflowScheduled = PR_FALSE;
  }

  ASSERT_REFLOW_SCHEDULED_STATE();
}

nsresult
PresShell::RecreateFramesFor(nsIContent* aContent)
{
  NS_TIME_FUNCTION_MIN(1.0);

  NS_ENSURE_TRUE(mPresContext, NS_ERROR_FAILURE);
  if (!mDidInitialReflow) {
    
    
    return NS_OK;
  }

  
  

  NS_ASSERTION(mViewManager, "Should have view manager");
  nsIViewManager::UpdateViewBatch batch(mViewManager);

  
  
  mDocument->FlushPendingNotifications(Flush_ContentAndNotify);

  nsAutoScriptBlocker scriptBlocker;

  nsStyleChangeList changeList;
  changeList.AppendChange(nsnull, aContent, nsChangeHint_ReconstructFrame);

  
  ++mChangeNestCount;
  nsresult rv = mFrameConstructor->ProcessRestyledFrames(changeList);
  --mChangeNestCount;
  
  batch.EndUpdateViewBatch(NS_VMREFRESH_NO_SYNC);
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
  NS_TIME_FUNCTION_MIN(1.0);

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
PresShell::GoToAnchor(const nsAString& aAnchorName, PRBool aScroll)
{
  if (!mDocument) {
    return NS_ERROR_FAILURE;
  }
  
  
  nsRefPtr<nsEventStateManager> esm = mPresContext->EventStateManager();

  if (aAnchorName.IsEmpty()) {
    NS_ASSERTION(!aScroll, "can't scroll to empty anchor name");
    esm->SetContentState(nsnull, NS_EVENT_STATE_URLTARGET);
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
      PRUint32 i;
      
      for (i = 0; PR_TRUE; i++) {
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
          content = nsnull;
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
      PRUint32 i;
      
      for (i = 0; PR_TRUE; i++) {
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
      rv = ScrollContentIntoView(content, NS_PRESSHELL_SCROLL_TOP,
                                 NS_PRESSHELL_SCROLL_ANYWHERE,
                                 ANCHOR_SCROLL_FLAGS);
      NS_ENSURE_SUCCESS(rv, rv);

      nsIScrollableFrame* rootScroll = GetRootScrollFrameAsScrollable();
      if (rootScroll) {
        mLastAnchorScrolledTo = content;
        mLastAnchorScrollPositionY = rootScroll->GetScrollPosition().y;
      }
    }

    
    
    PRBool selectAnchor = nsContentUtils::GetBoolPref("layout.selectanchor");

    
    
    
    nsCOMPtr<nsIDOMRange> jumpToRange = do_CreateInstance(kRangeCID);
    if (jumpToRange) {
      while (content && content->GetChildCount() > 0) {
        content = content->GetChildAt(0);
      }
      nsCOMPtr<nsIDOMNode> node(do_QueryInterface(content));
      NS_ASSERTION(node, "No nsIDOMNode for descendant of anchor");
      jumpToRange->SelectNodeContents(node);
    }
    if (jumpToRange) {
      
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
        if (SameCOMIdentity(win, focusedWindow))
          fm->ClearFocus(focusedWindow);
      }
    }
  } else {
    rv = NS_ERROR_FAILURE; 

    
    
    if ((NS_LossyConvertUTF16toASCII(aAnchorName).LowerCaseEqualsLiteral("top")) &&
        (mPresContext->CompatibilityMode() == eCompatibility_NavQuirks)) {
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

  NS_ASSERTION(mDidInitialReflow, "should have done initial reflow by now");

  nsIScrollableFrame* rootScroll = GetRootScrollFrameAsScrollable();
  if (!rootScroll ||
      mLastAnchorScrollPositionY != rootScroll->GetScrollPosition().y)
    return NS_OK;

  nsresult rv = ScrollContentIntoView(mLastAnchorScrolledTo, NS_PRESSHELL_SCROLL_TOP,
                                      NS_PRESSHELL_SCROLL_ANYWHERE,
                                      ANCHOR_SCROLL_FLAGS);
  mLastAnchorScrolledTo = nsnull;
  return rv;
}











static void
AccumulateFrameBounds(nsIFrame* aContainerFrame,
                      nsIFrame* aFrame,
                      PRBool aUseWholeLineHeightForInlines,
                      nsRect& aRect,
                      PRBool& aHaveRect)
{
  nsRect frameBounds = aFrame->GetRect() +
    aFrame->GetParent()->GetOffsetTo(aContainerFrame);

  
  
  
  if (frameBounds.height == 0 || aUseWholeLineHeightForInlines) {
    nsIAtom* frameType = NULL;
    nsIFrame *prevFrame = aFrame;
    nsIFrame *f = aFrame;

    while (f &&
           (frameType = f->GetType()) == nsGkAtoms::inlineFrame) {
      prevFrame = f;
      f = prevFrame->GetParent();
    }

    if (f != aFrame &&
        f &&
        frameType == nsGkAtoms::blockFrame) {
      
      nsAutoLineIterator lines = f->GetLineIterator();
      if (lines) {
        PRInt32 index = lines->FindLineContaining(prevFrame);
        if (index >= 0) {
          nsIFrame *trash1;
          PRInt32 trash2;
          nsRect lineBounds;
          PRUint32 trash3;

          if (NS_SUCCEEDED(lines->GetLine(index, &trash1, &trash2,
                                          lineBounds, &trash3))) {
            lineBounds += f->GetOffsetTo(aContainerFrame);
            if (lineBounds.y < frameBounds.y) {
              frameBounds.height = frameBounds.YMost() - lineBounds.y;
              frameBounds.y = lineBounds.y;
            }
          }
        }
      }
    }
  }

  if (aHaveRect) {
    
    
    
    aRect.UnionRectEdges(aRect, frameBounds);
  } else {
    aHaveRect = PR_TRUE;
    aRect = frameBounds;
  }
}









static void ScrollToShowRect(nsIScrollableFrame* aScrollFrame,
                             const nsRect&       aRect,
                             PRIntn              aVPercent,
                             PRIntn              aHPercent,
                             PRUint32            aFlags)
{
  nsPoint scrollPt = aScrollFrame->GetScrollPosition();
  nsRect visibleRect(scrollPt, aScrollFrame->GetScrollPortRect().Size());
  nsSize lineSize = aScrollFrame->GetLineScrollAmount();
  nsPresContext::ScrollbarStyles ss = aScrollFrame->GetScrollbarStyles();

  if ((aFlags & nsIPresShell::SCROLL_OVERFLOW_HIDDEN) ||
      ss.mVertical != NS_STYLE_OVERFLOW_HIDDEN) {
    
    if (NS_PRESSHELL_SCROLL_ANYWHERE == aVPercent ||
        (NS_PRESSHELL_SCROLL_IF_NOT_VISIBLE == aVPercent &&
         aRect.height < lineSize.height)) {
      
      
      if (aRect.y < visibleRect.y) {
        
        scrollPt.y = aRect.y;
      } else if (aRect.YMost() > visibleRect.YMost()) {
        
        
        scrollPt.y += aRect.YMost() - visibleRect.YMost();
        if (scrollPt.y > aRect.y) {
          scrollPt.y = aRect.y;
        }
      }
    } else if (NS_PRESSHELL_SCROLL_IF_NOT_VISIBLE == aVPercent) {
      
      if (aRect.YMost() - lineSize.height < visibleRect.y) {
        
        scrollPt.y = aRect.y;
      }  else if (aRect.y + lineSize.height > visibleRect.YMost()) {
        
        
        scrollPt.y += aRect.YMost() - visibleRect.YMost();
        if (scrollPt.y > aRect.y) {
          scrollPt.y = aRect.y;
        }
      }
    } else {
      
      nscoord frameAlignY =
        NSToCoordRound(aRect.y + aRect.height * (aVPercent / 100.0f));
      scrollPt.y =
        NSToCoordRound(frameAlignY - visibleRect.height * (aVPercent / 100.0f));
    }
  }

  if ((aFlags & nsIPresShell::SCROLL_OVERFLOW_HIDDEN) ||
      ss.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN) {
    
    if (NS_PRESSHELL_SCROLL_ANYWHERE == aHPercent ||
        (NS_PRESSHELL_SCROLL_IF_NOT_VISIBLE == aHPercent &&
         aRect.width < lineSize.width)) {
      
      
      if (aRect.x < visibleRect.x) {
        
        scrollPt.x = aRect.x;
      } else if (aRect.XMost() > visibleRect.XMost()) {
        
        
        scrollPt.x += aRect.XMost() - visibleRect.XMost();
        if (scrollPt.x > aRect.x) {
          scrollPt.x = aRect.x;
        }
      }
    } else if (NS_PRESSHELL_SCROLL_IF_NOT_VISIBLE == aHPercent) {
      
      if (aRect.XMost() - lineSize.width < visibleRect.x) {
        
        scrollPt.x = aRect.x;
      }  else if (aRect.x + lineSize.width > visibleRect.XMost()) {
        
        
        scrollPt.x += aRect.XMost() - visibleRect.XMost();
        if (scrollPt.x > aRect.x) {
          scrollPt.x = aRect.x;
        }
      }
    } else {
      
      nscoord frameAlignX =
        NSToCoordRound(aRect.x + (aRect.width) * (aHPercent / 100.0f));
      scrollPt.x =
        NSToCoordRound(frameAlignX - visibleRect.width * (aHPercent / 100.0f));
    }
  }

  aScrollFrame->ScrollTo(scrollPt, nsIScrollableFrame::INSTANT);
}

nsresult
PresShell::ScrollContentIntoView(nsIContent* aContent,
                                 PRIntn      aVPercent,
                                 PRIntn      aHPercent,
                                 PRUint32    aFlags)
{
  nsCOMPtr<nsIContent> content = aContent; 
  NS_ENSURE_TRUE(content, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDocument> currentDoc = content->GetCurrentDoc();
  NS_ENSURE_STATE(currentDoc);

  NS_ASSERTION(mDidInitialReflow, "should have done initial reflow by now");

  mContentToScrollTo = aContent;
  mContentScrollVPosition = aVPercent;
  mContentScrollHPosition = aHPercent;
  mContentToScrollToFlags = aFlags;

  
  currentDoc->FlushPendingNotifications(Flush_InterruptibleLayout);

  
  
  
  
  
  
  
  
  if (mContentToScrollTo) {
    DoScrollContentIntoView(content, aVPercent, aHPercent, aFlags);
  }
  return NS_OK;
}

void
PresShell::DoScrollContentIntoView(nsIContent* aContent,
                                   PRIntn      aVPercent,
                                   PRIntn      aHPercent,
                                   PRUint32    aFlags)
{
  NS_ASSERTION(mDidInitialReflow, "should have done initial reflow by now");

  nsIFrame* frame = aContent->GetPrimaryFrame();
  if (!frame) {
    mContentToScrollTo = nsnull;
    return;
  }

  if (frame->GetStateBits() & NS_FRAME_FIRST_REFLOW) {
    
    
    
    return;
  }

  nsIFrame* container =
    nsLayoutUtils::GetClosestFrameOfType(frame, nsGkAtoms::scrollFrame);
  if (!container) {
    
    return;
  }

  
  
  
  
  
  
  
  
  
  
  nsRect frameBounds;
  PRBool haveRect = PR_FALSE;
  PRBool useWholeLineHeightForInlines = aVPercent != NS_PRESSHELL_SCROLL_ANYWHERE;
  do {
    AccumulateFrameBounds(container, frame, useWholeLineHeightForInlines,
                          frameBounds, haveRect);
  } while ((frame = frame->GetNextContinuation()));

  ScrollFrameRectIntoView(container, frameBounds, aVPercent, aHPercent,
                          aFlags);
}

PRBool
PresShell::ScrollFrameRectIntoView(nsIFrame*     aFrame,
                                   const nsRect& aRect,
                                   PRIntn        aVPercent,
                                   PRIntn        aHPercent,
                                   PRUint32      aFlags)
{
  PRBool didScroll = PR_FALSE;
  
  nsRect rect = aRect;
  nsIFrame* container = aFrame;
  
  
  do {
    nsIScrollableFrame* sf = do_QueryFrame(container);
    if (sf) {
      nsPoint oldPosition = sf->GetScrollPosition();
      ScrollToShowRect(sf, rect - sf->GetScrolledFrame()->GetPosition(),
                       aVPercent, aHPercent, aFlags);
      nsPoint newPosition = sf->GetScrollPosition();
      
      
      rect += oldPosition - newPosition;

      if (oldPosition != newPosition) {
        didScroll = PR_TRUE;
      }

      
      if (aFlags & nsIPresShell::SCROLL_FIRST_ANCESTOR_ONLY) {
        break;
      }
    }
    rect += container->GetPosition();
    nsIFrame* parent = container->GetParent();
    if (!parent && !(aFlags & nsIPresShell::SCROLL_NO_PARENT_FRAMES)) {
      nsPoint extraOffset(0,0);
      parent = nsLayoutUtils::GetCrossDocParentFrame(container, &extraOffset);
      if (parent) {
        PRInt32 APD = container->PresContext()->AppUnitsPerDevPixel();        
        PRInt32 parentAPD = parent->PresContext()->AppUnitsPerDevPixel();
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
  nsIFrame* rootFrame = FrameManager()->GetRootFrame();
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


nsresult PresShell::GetLinkLocation(nsIDOMNode* aNode, nsAString& aLocationString) const
{
#ifdef DEBUG_dr
  printf("dr :: PresShell::GetLinkLocation\n");
#endif

  NS_ENSURE_ARG_POINTER(aNode);
  nsresult rv;
  nsAutoString anchorText;
  static const char strippedChars[] = "\t\r\n";

  
  nsCOMPtr<nsIDOMHTMLAnchorElement> anchor(do_QueryInterface(aNode));
  nsCOMPtr<nsIDOMHTMLAreaElement> area;
  nsCOMPtr<nsIDOMHTMLLinkElement> link;
  nsAutoString xlinkType;
  if (anchor) {
    rv = anchor->GetHref(anchorText);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    
    area = do_QueryInterface(aNode);
    if (area) {
      rv = area->GetHref(anchorText);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      
      link = do_QueryInterface(aNode);
      if (link) {
        rv = link->GetHref(anchorText);
        NS_ENSURE_SUCCESS(rv, rv);
      } else {
        
        nsCOMPtr<nsIDOMElement> element(do_QueryInterface(aNode));
        if (element) {
          NS_NAMED_LITERAL_STRING(xlinkNS,"http://www.w3.org/1999/xlink");
          element->GetAttributeNS(xlinkNS,NS_LITERAL_STRING("type"),xlinkType);
          if (xlinkType.EqualsLiteral("simple")) {
            element->GetAttributeNS(xlinkNS,NS_LITERAL_STRING("href"),anchorText);
            if (!anchorText.IsEmpty()) {
              

              nsAutoString base;
              nsCOMPtr<nsIDOM3Node> node(do_QueryInterface(aNode,&rv));
              NS_ENSURE_SUCCESS(rv, rv);
              node->GetBaseURI(base);

              nsCOMPtr<nsIIOService>
                ios(do_GetService("@mozilla.org/network/io-service;1", &rv));
              NS_ENSURE_SUCCESS(rv, rv);

              nsCOMPtr<nsIURI> baseURI;
              rv = ios->NewURI(NS_ConvertUTF16toUTF8(base),nsnull,nsnull,getter_AddRefs(baseURI));
              NS_ENSURE_SUCCESS(rv, rv);

              nsCAutoString spec;
              rv = baseURI->Resolve(NS_ConvertUTF16toUTF8(anchorText),spec);
              NS_ENSURE_SUCCESS(rv, rv);

              CopyUTF8toUTF16(spec, anchorText);
            }
          }
        }
      }
    }
  }

  if (anchor || area || link || xlinkType.EqualsLiteral("simple")) {
    
    anchorText.StripChars(strippedChars);

    aLocationString = anchorText;

    return NS_OK;
  }

  
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP_(void)
PresShell::DispatchSynthMouseMove(nsGUIEvent *aEvent,
                                  PRBool aFlushOnHoverChange)
{
  PRUint32 hoverGenerationBefore = mFrameConstructor->GetHoverGeneration();
  nsEventStatus status;
  nsIView* targetView = nsIView::GetViewFor(aEvent->widget);
  targetView->GetViewManager()->DispatchEvent(aEvent, targetView, &status);
  if (aFlushOnHoverChange &&
      hoverGenerationBefore != mFrameConstructor->GetHoverGeneration()) {
    
    
    FlushPendingNotifications(Flush_Layout);
  }
}

NS_IMETHODIMP_(void)
PresShell::ClearMouseCapture(nsIView* aView)
{
  if (gCaptureInfo.mContent) {
    if (aView) {
      
      
      nsIFrame* frame = gCaptureInfo.mContent->GetPrimaryFrame();
      if (frame) {
        nsIView* view = frame->GetClosestView();
        
        
        if (view) {
          do {
            if (view == aView) {
              NS_RELEASE(gCaptureInfo.mContent);
              
              
              gCaptureInfo.mAllowed = PR_FALSE;
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

  
  
  
  gCaptureInfo.mAllowed = PR_FALSE;
}

nsresult
PresShell::CaptureHistoryState(nsILayoutHistoryState** aState, PRBool aLeavingPage)
{
  NS_TIME_FUNCTION_MIN(1.0);

  nsresult rv = NS_OK;

  NS_PRECONDITION(nsnull != aState, "null state pointer");

  
  
  
  
  
  
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
      *aState = nsnull;
      return rv;
    }    

    docShell->SetLayoutHistoryState(historyState);
  }

  *aState = historyState;
  NS_IF_ADDREF(*aState);
  
  
  nsIFrame* rootFrame = FrameManager()->GetRootFrame();
  if (!rootFrame) return NS_OK;
  
  
  
  
  if (aLeavingPage) {
    nsIFrame* scrollFrame = GetRootScrollFrame();
    if (scrollFrame) {
      FrameManager()->CaptureFrameStateFor(scrollFrame, historyState,
                                           nsIStatefulFrame::eDocumentScrollState);
    }
  }

  FrameManager()->CaptureFrameState(rootFrame, historyState);  
 
  return NS_OK;
}

void
PresShell::UnsuppressAndInvalidate()
{
  
  
  if ((!mDocument->IsResourceDoc() && !mPresContext->EnsureVisible()) ||
      mHaveShutDown) {
    
    return;
  }
  
  mPaintingSuppressed = PR_FALSE;
  nsIFrame* rootFrame = FrameManager()->GetRootFrame();
  if (rootFrame) {
    
    nsRect rect(nsPoint(0, 0), rootFrame->GetSize());
    rootFrame->Invalidate(rect);

    if (mCaretEnabled && mCaret) {
      mCaret->CheckCaretDrawingState();
    }

    nsRootPresContext* rootPC = mPresContext->GetRootPresContext();
    if (rootPC) {
      rootPC->RequestUpdatePluginGeometry(rootFrame);
    }
  }

  
  nsPIDOMWindow *win = mDocument->GetWindow();
  if (win)
    win->SetReadyForFocus();

  if (!mHaveShutDown)
    SynthesizeMouseMove(PR_FALSE);
}

void
PresShell::UnsuppressPainting()
{
  if (mPaintSuppressionTimer) {
    mPaintSuppressionTimer->Cancel();
    mPaintSuppressionTimer = nsnull;
  }

  if (mIsDocumentGone || !mPaintingSuppressed)
    return;

  
  
  
  
  if (mDirtyRoots.Length() > 0)
    mShouldUnsuppressPainting = PR_TRUE;
  else
    UnsuppressAndInvalidate();
}


nsresult
PresShell::PostReflowCallback(nsIReflowCallback* aCallback)
{
  void* result = AllocateMisc(sizeof(nsCallbackEventRequest));
  if (NS_UNLIKELY(!result)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsCallbackEventRequest* request = (nsCallbackEventRequest*)result;

  request->callback = aCallback;
  request->next = nsnull;

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
   nsCallbackEventRequest* before = nsnull;
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
          NS_ASSERTION(before == nsnull, "impossible");
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
      mLastCallbackEventRequest = nsnull;
    }
    nsIReflowCallback* callback = node->callback;
    FreeMisc(sizeof(nsCallbackEventRequest), node);
    if (callback) {
      callback->ReflowCallbackCanceled();
    }
  }
}

void
PresShell::HandlePostedReflowCallbacks(PRBool aInterruptible)
{
   PRBool shouldFlush = PR_FALSE;

   while (mFirstCallbackEventRequest) {
     nsCallbackEventRequest* node = mFirstCallbackEventRequest;
     mFirstCallbackEventRequest = node->next;
     if (!mFirstCallbackEventRequest) {
       mLastCallbackEventRequest = nsnull;
     }
     nsIReflowCallback* callback = node->callback;
     FreeMisc(sizeof(nsCallbackEventRequest), node);
     if (callback) {
       if (callback->ReflowFinished()) {
         shouldFlush = PR_TRUE;
       }
     }
   }

   mozFlushType flushType =
     aInterruptible ? Flush_InterruptibleLayout : Flush_Layout;
   if (shouldFlush)
     FlushPendingNotifications(flushType);
}

PRBool
PresShell::IsSafeToFlush() const
{
  
  PRBool isSafeToFlush = !mIsReflowing &&
                         !mChangeNestCount;

  if (isSafeToFlush) {
    
    nsIViewManager* viewManager = GetViewManager();
    if (viewManager) {
      PRBool isPainting = PR_FALSE;
      viewManager->IsPainting(isPainting);
      if (isPainting) {
        isSafeToFlush = PR_FALSE;
      }
    }
  }

  return isSafeToFlush;
}


void
PresShell::FlushPendingNotifications(mozFlushType aType)
{
#ifdef NS_FUNCTION_TIMER
  NS_TIME_FUNCTION_DECLARE_DOCURL;
  static const char *flushTypeNames[] = {
    "Flush_Content",
    "Flush_ContentAndNotify",
    "Flush_Styles",
    "Flush_InterruptibleLayout",
    "Flush_Layout",
    "Flush_Display"
  };
  NS_TIME_FUNCTION_MIN_FMT(1.0, "%s (line %d) (document: %s, type: %s)", MOZ_FUNCTION_NAME,
                           __LINE__, docURL__.get(), flushTypeNames[aType - 1]);
#endif

  NS_ASSERTION(aType >= Flush_Frames, "Why did we get called?");

  PRBool isSafeToFlush = IsSafeToFlush();

  
  
  PRBool hasHadScriptObject;
  if (mDocument->GetScriptHandlingObject(hasHadScriptObject) ||
      hasHadScriptObject) {
    isSafeToFlush = isSafeToFlush && nsContentUtils::IsSafeToRunScript();
  }

  NS_ASSERTION(!isSafeToFlush || mViewManager, "Must have view manager");
  
  nsCOMPtr<nsIViewManager> viewManagerDeathGrip = mViewManager;
  if (isSafeToFlush && mViewManager) {
    
    
    nsCOMPtr<nsIPresShell> kungFuDeathGrip(this);

    if (mResizeEvent.IsPending()) {
      FireResizeEvent();
      if (mIsDestroying) {
        return;
      }
    }

    
    
    
    nsIViewManager::UpdateViewBatch batch(mViewManager);

    
    
    
    
    
    
    mDocument->FlushExternalResources(aType);

    
    
    
    
    mDocument->FlushPendingNotifications(Flush_ContentAndNotify);

    
    
    if (!mIsDestroying) {
      mViewManager->FlushDelayedResize(PR_FALSE);
      mPresContext->FlushPendingMediaFeatureValuesChanged();

      
      
      
      mPresContext->FlushUserFontSet();

#ifdef MOZ_SMIL
      
      if (mDocument->HasAnimationController()) {
        mDocument->GetAnimationController()->FlushResampleRequests();
      }
#endif 

      nsAutoScriptBlocker scriptBlocker;
      mFrameConstructor->CreateNeededFrames();
      mFrameConstructor->ProcessPendingRestyles();
    }

#ifdef MOZ_CSS_ANIMATIONS
    
    
    if (!mIsDestroying) {
      mPresContext->AnimationManager()->DispatchEvents();
    }
#endif

    
    
    
    if (!mIsDestroying) {
      mDocument->BindingManager()->ProcessAttachedQueue();
    }

    
    
    
    
    
    
    
    if (!mIsDestroying) {
      nsAutoScriptBlocker scriptBlocker;
      mFrameConstructor->CreateNeededFrames();
      mFrameConstructor->ProcessPendingRestyles();
    }


    
    
    

    if (aType >= (mSuppressInterruptibleReflows ? Flush_Layout : Flush_InterruptibleLayout) &&
        !mIsDestroying) {
      mFrameConstructor->RecalcQuotesAndCounters();
      mViewManager->FlushDelayedResize(PR_TRUE);
      if (ProcessReflowCommands(aType < Flush_Layout) && mContentToScrollTo) {
        
        DoScrollContentIntoView(mContentToScrollTo, mContentScrollVPosition,
                                mContentScrollHPosition,
                                mContentToScrollToFlags);
        mContentToScrollTo = nsnull;
      }
    }

    if (aType >= Flush_Layout) {
      
      
      
      nsRootPresContext* rootPresContext = mPresContext->GetRootPresContext();
      if (rootPresContext) {
        rootPresContext->UpdatePluginGeometry();
      }
#ifdef DEBUG
      if (!mIsDestroying) {
        nsIView* rootView = mViewManager->GetRootView();
        if (rootView) {
          nsRect bounds = rootView->GetBounds();
          NS_ASSERTION(bounds.Size() == mPresContext->GetVisibleArea().Size(),
                       "root view / pres context visible size mismatch");
        }
      }
#endif
    }

    PRUint32 updateFlags = NS_VMREFRESH_NO_SYNC;
    if (aType >= Flush_Display) {
      
      
      updateFlags = NS_VMREFRESH_IMMEDIATE;
    }
    batch.EndUpdateViewBatch(updateFlags);
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
  PRUint32 selectorFlags =
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

  if (mDidInitialReflow) {
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

  if (mDidInitialReflow &&
      mStyleSet->HasDocumentStateDependentStyle(mPresContext,
                                                mDocument->GetRootElement(),
                                                aStateMask)) {
    mFrameConstructor->PostRestyleEvent(mDocument->GetRootElement(),
                                        eRestyle_Subtree, NS_STYLE_HINT_NONE);
    VERIFY_STYLE_TREE;
  }

  if (aStateMask.HasState(NS_DOCUMENT_STATE_WINDOW_INACTIVE)) {
    nsIFrame* root = FrameManager()->GetRootFrame();
    if (root) {
      root->InvalidateFrameSubtree();
    }
  }
}

void
PresShell::AttributeWillChange(nsIDocument* aDocument,
                               Element*     aElement,
                               PRInt32      aNameSpaceID,
                               nsIAtom*     aAttribute,
                               PRInt32      aModType)
{
  NS_PRECONDITION(!mIsDocumentGone, "Unexpected AttributeWillChange");
  NS_PRECONDITION(aDocument == mDocument, "Unexpected aDocument");

  
  
  
  if (mDidInitialReflow) {
    nsAutoCauseReflowNotifier crNotifier(this);
    mFrameConstructor->AttributeWillChange(aElement, aNameSpaceID,
                                           aAttribute, aModType);
    VERIFY_STYLE_TREE;
  }
}

void
PresShell::AttributeChanged(nsIDocument* aDocument,
                            Element*     aElement,
                            PRInt32      aNameSpaceID,
                            nsIAtom*     aAttribute,
                            PRInt32      aModType)
{
  NS_PRECONDITION(!mIsDocumentGone, "Unexpected AttributeChanged");
  NS_PRECONDITION(aDocument == mDocument, "Unexpected aDocument");

  
  
  
  if (mDidInitialReflow) {
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
                           PRInt32     aNewIndexInContainer)
{
  NS_PRECONDITION(!mIsDocumentGone, "Unexpected ContentAppended");
  NS_PRECONDITION(aDocument == mDocument, "Unexpected aDocument");
  NS_PRECONDITION(aContainer, "must have container");
  
  if (!mDidInitialReflow) {
    return;
  }
  
  nsAutoCauseReflowNotifier crNotifier(this);

  
  
  
  mFrameConstructor->RestyleForAppend(aContainer->AsElement(), aFirstNewContent);

  mFrameConstructor->ContentAppended(aContainer, aFirstNewContent, PR_TRUE);
  VERIFY_STYLE_TREE;
}

void
PresShell::ContentInserted(nsIDocument* aDocument,
                           nsIContent*  aContainer,
                           nsIContent*  aChild,
                           PRInt32      aIndexInContainer)
{
  NS_PRECONDITION(!mIsDocumentGone, "Unexpected ContentInserted");
  NS_PRECONDITION(aDocument == mDocument, "Unexpected aDocument");

  if (!mDidInitialReflow) {
    return;
  }
  
  nsAutoCauseReflowNotifier crNotifier(this);

  
  
  
  if (aContainer)
    mFrameConstructor->RestyleForInsertOrChange(aContainer->AsElement(), aChild);

  mFrameConstructor->ContentInserted(aContainer, aChild, nsnull, PR_TRUE);
  VERIFY_STYLE_TREE;
}

void
PresShell::ContentRemoved(nsIDocument *aDocument,
                          nsIContent* aContainer,
                          nsIContent* aChild,
                          PRInt32     aIndexInContainer,
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
    oldNextSibling = nsnull;
  }
  
  if (aContainer)
    mFrameConstructor->RestyleForRemove(aContainer->AsElement(), aChild,
                                        oldNextSibling);

  PRBool didReconstruct;
  mFrameConstructor->ContentRemoved(aContainer, aChild, oldNextSibling,
                                    nsCSSFrameConstructor::REMOVE_CONTENT,
                                    &didReconstruct);

  VERIFY_STYLE_TREE;
}

nsresult
PresShell::ReconstructFrames(void)
{
  NS_PRECONDITION(!FrameManager()->GetRootFrame() || mDidInitialReflow,
                  "Must not have root frame before initial reflow");
  if (!mDidInitialReflow) {
    
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
  mStylesHaveChanged = PR_FALSE;

  if (mIsDestroying) {
    
    return;
  }

  if (mPresContext) {
    mPresContext->RebuildUserFontSet();
#ifdef MOZ_CSS_ANIMATIONS
    mPresContext->AnimationManager()->KeyframesListIsDirty();
#endif
  }

  Element* root = mDocument->GetRootElement();
  if (!mDidInitialReflow) {
    
    return;
  }

  if (!root) {
    
    return;
  }
  
  mFrameConstructor->PostRestyleEvent(root, eRestyle_Subtree, NS_STYLE_HINT_NONE);
}

void
nsIPresShell::ReconstructStyleDataExternal()
{
  ReconstructStyleDataInternal();
}

void
PresShell::StyleSheetAdded(nsIDocument *aDocument,
                           nsIStyleSheet* aStyleSheet,
                           PRBool aDocumentSheet)
{
  
  NS_PRECONDITION(aStyleSheet, "Must have a style sheet!");

  if (aStyleSheet->IsApplicable() && aStyleSheet->HasRules()) {
    mStylesHaveChanged = PR_TRUE;
  }
}

void 
PresShell::StyleSheetRemoved(nsIDocument *aDocument,
                             nsIStyleSheet* aStyleSheet,
                             PRBool aDocumentSheet)
{
  
  NS_PRECONDITION(aStyleSheet, "Must have a style sheet!");

  if (aStyleSheet->IsApplicable() && aStyleSheet->HasRules()) {
    mStylesHaveChanged = PR_TRUE;
  }
}

void
PresShell::StyleSheetApplicableStateChanged(nsIDocument *aDocument,
                                            nsIStyleSheet* aStyleSheet,
                                            PRBool aApplicable)
{
  if (aStyleSheet->HasRules()) {
    mStylesHaveChanged = PR_TRUE;
  }
}

void
PresShell::StyleRuleChanged(nsIDocument *aDocument,
                            nsIStyleSheet* aStyleSheet,
                            nsIStyleRule* aOldStyleRule,
                            nsIStyleRule* aNewStyleRule)
{
  mStylesHaveChanged = PR_TRUE;
}

void
PresShell::StyleRuleAdded(nsIDocument *aDocument,
                          nsIStyleSheet* aStyleSheet,
                          nsIStyleRule* aStyleRule) 
{
  mStylesHaveChanged = PR_TRUE;
}

void
PresShell::StyleRuleRemoved(nsIDocument *aDocument,
                            nsIStyleSheet* aStyleSheet,
                            nsIStyleRule* aStyleRule) 
{
  mStylesHaveChanged = PR_TRUE;
}

nsIFrame*
PresShell::GetRealPrimaryFrameFor(nsIContent* aContent) const
{
  if (aContent->GetDocument() != GetDocument()) {
    return nsnull;
  }
  nsIFrame *primaryFrame = aContent->GetPrimaryFrame();
  if (!primaryFrame)
    return nsnull;
  return nsPlaceholderFrame::GetRealFrameFor(primaryFrame);
}

nsIFrame*
PresShell::GetPlaceholderFrameFor(nsIFrame* aFrame) const
{
  return FrameManager()->GetPlaceholderFrameFor(aFrame);
}

nsresult
PresShell::RenderDocument(const nsRect& aRect, PRUint32 aFlags,
                          nscolor aBackgroundColor,
                          gfxContext* aThebesContext)
{
  NS_TIME_FUNCTION_WITH_DOCURL;

  NS_ENSURE_TRUE(!(aFlags & RENDER_IS_UNTRUSTED), NS_ERROR_NOT_IMPLEMENTED);

  nsAutoScriptBlocker blockScripts;

  
  gfxRect r(0, 0,
            nsPresContext::AppUnitsToFloatCSSPixels(aRect.width),
            nsPresContext::AppUnitsToFloatCSSPixels(aRect.height));
  aThebesContext->NewPath();
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
  aThebesContext->Rectangle(r, PR_TRUE);
#else
  aThebesContext->Rectangle(r);
#endif

  nsIFrame* rootFrame = FrameManager()->GetRootFrame();
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

  
  
  
  
  
  
  
  PRBool needsGroup = NS_GET_A(aBackgroundColor) < 0xff ||
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

  PRBool wouldFlushRetainedLayers = PR_FALSE;
  PRUint32 flags = nsLayoutUtils::PAINT_IGNORE_SUPPRESSION;
  if (aThebesContext->CurrentMatrix().HasNonIntegerTranslation()) {
    flags |= nsLayoutUtils::PAINT_IN_TRANSFORM;
  }
  if (!(aFlags & RENDER_ASYNC_DECODE_IMAGES)) {
    flags |= nsLayoutUtils::PAINT_SYNC_DECODE_IMAGES;
  }
  if (aFlags & RENDER_USE_WIDGET_LAYERS) {
    
    nsIView* view = rootFrame->GetView();
    if (view && view->GetWidget() &&
        nsLayoutUtils::GetDisplayRootFrame(rootFrame) == rootFrame) {
      flags |= nsLayoutUtils::PAINT_WIDGET_LAYERS;
    }
  }
  if (!(aFlags & RENDER_CARET)) {
    wouldFlushRetainedLayers = PR_TRUE;
    flags |= nsLayoutUtils::PAINT_HIDE_CARET;
  }
  if (aFlags & RENDER_IGNORE_VIEWPORT_SCROLLING) {
    wouldFlushRetainedLayers = !IgnoringViewportScrolling();
    mRenderFlags = ChangeFlag(mRenderFlags, PR_TRUE, STATE_IGNORING_VIEWPORT_SCROLLING);
  }
  if (aFlags & RENDER_DOCUMENT_RELATIVE) {
    
    
    
    
    wouldFlushRetainedLayers = PR_TRUE;
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
                           nsIRange* aRange)
{
  NS_TIME_FUNCTION_WITH_DOCURL;

  
  
  
  
  
  
  
  
  
  nsRect surfaceRect;
  nsDisplayList tmpList;

  nsDisplayItem* i;
  while ((i = aList->RemoveBottom())) {
    
    
    nsDisplayItem* itemToInsert = nsnull;
    nsIFrame* frame = i->GetUnderlyingFrame();
    if (frame) {
      nsIContent* content = frame->GetContent();
      if (content) {
        PRBool atStart = (content == aRange->GetStartParent());
        PRBool atEnd = (content == aRange->GetEndParent());
        if ((atStart || atEnd) && frame->GetType() == nsGkAtoms::textFrame) {
          PRInt32 frameStartOffset, frameEndOffset;
          frame->GetOffsets(frameStartOffset, frameEndOffset);

          PRInt32 hilightStart =
            atStart ? NS_MAX(aRange->StartOffset(), frameStartOffset) : frameStartOffset;
          PRInt32 hilightEnd =
            atEnd ? NS_MIN(aRange->EndOffset(), frameEndOffset) : frameEndOffset;
          if (hilightStart < hilightEnd) {
            
            nsPoint startPoint, endPoint;
            frame->GetPointFromOffset(hilightStart, &startPoint);
            frame->GetPointFromOffset(hilightEnd, &endPoint);

            
            
            
            
            nsRect textRect(aBuilder->ToReferenceFrame(frame), frame->GetSize());
            nscoord x = NS_MIN(startPoint.x, endPoint.x);
            textRect.x += x;
            textRect.width = NS_MAX(startPoint.x, endPoint.x) - x;
            surfaceRect.UnionRect(surfaceRect, textRect);

            
            
            
            itemToInsert = new (aBuilder)
                nsDisplayClip(aBuilder, frame, i, textRect);
          }
        }
        
        
        
        else if (content->GetCurrentDoc() ==
                   aRange->GetStartParent()->GetCurrentDoc()) {
          
          PRBool before, after;
          nsresult rv =
            nsRange::CompareNodeToRange(content, aRange, &before, &after);
          if (NS_SUCCEEDED(rv) && !before && !after) {
            itemToInsert = i;
            surfaceRect.UnionRect(surfaceRect, i->GetBounds(aBuilder));
          }
        }
      }
    }

    
    
    nsDisplayList* sublist = i->GetList();
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

static PRBool gDumpRangePaintList = PR_FALSE;
#endif

RangePaintInfo*
PresShell::CreateRangePaintInfo(nsIDOMRange* aRange,
                                nsRect& aSurfaceRect,
                                PRBool aForPrimarySelection)
{
  NS_TIME_FUNCTION_WITH_DOCURL;

  RangePaintInfo* info = nsnull;

  nsCOMPtr<nsIRange> range = do_QueryInterface(aRange);
  if (!range)
    return nsnull;

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
      return nsnull;

    nsIContent* ancestorContent = static_cast<nsIContent*>(ancestor);
    ancestorFrame = ancestorContent->GetPrimaryFrame();

    
    
    while (ancestorFrame &&
           nsLayoutUtils::GetNextContinuationOrSpecialSibling(ancestorFrame))
      ancestorFrame = ancestorFrame->GetParent();
  }

  if (!ancestorFrame)
    return nsnull;

  info = new RangePaintInfo(range, ancestorFrame);
  if (!info)
    return nsnull;

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
  NS_TIME_FUNCTION_WITH_DOCURL;

  nsPresContext* pc = GetPresContext();
  if (!pc || aArea.width == 0 || aArea.height == 0)
    return nsnull;

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
  PRBool resize = (pixelArea.width > maxWidth || pixelArea.height > maxHeight);
  if (resize) {
    scale = 1.0;
    
    
    
    if (pixelArea.width > maxWidth)
      scale = NS_MIN(scale, float(maxWidth) / pixelArea.width);
    if (pixelArea.height > maxHeight)
      scale = NS_MIN(scale, float(maxHeight) / pixelArea.height);

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
  if (!surface || surface->CairoStatus()) {
    delete surface;
    return nsnull;
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

  
  
  
  nsCOMPtr<nsFrameSelection> frameSelection;
  if (aSelection) {
    nsCOMPtr<nsISelectionPrivate> selpriv = do_QueryInterface(aSelection);
    selpriv->GetFrameSelection(getter_AddRefs(frameSelection));
  }
  else {
    frameSelection = FrameSelection();
  }
  PRInt16 oldDisplaySelection = frameSelection->GetDisplaySelection();
  frameSelection->SetDisplaySelection(nsISelectionController::SELECTION_HIDDEN);

  
  PRInt32 count = aItems->Length();
  for (PRInt32 i = 0; i < count; i++) {
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
    return nsnull;
  
  nsCOMPtr<nsIDOMRange> range;
  NS_NewRange(getter_AddRefs(range));
  if (NS_FAILED(range->SelectNode(aNode)))
    return nsnull;

  RangePaintInfo* info = CreateRangePaintInfo(range, area, PR_FALSE);
  if (info && !rangeItems.AppendElement(info)) {
    delete info;
    return nsnull;
  }

  if (aRegion) {
    
    nsIntRect rrectPixels = aRegion->GetBounds();

    nsRect rrect = rrectPixels.ToAppUnits(nsPresContext::AppUnitsPerCSSPixel());
    area.IntersectRect(area, rrect);
    
    nsPresContext* pc = GetPresContext();
    if (!pc)
      return nsnull;

    
    aRegion->MoveBy(-pc->AppUnitsToDevPixels(area.x),
                    -pc->AppUnitsToDevPixels(area.y));
  }

  return PaintRangePaintInfo(&rangeItems, nsnull, aRegion, area, aPoint,
                             aScreenRect);
}

already_AddRefed<gfxASurface>
PresShell::RenderSelection(nsISelection* aSelection,
                           nsIntPoint& aPoint,
                           nsIntRect* aScreenRect)
{
  
  
  nsRect area;
  nsTArray<nsAutoPtr<RangePaintInfo> > rangeItems;

  
  
  
  PRInt32 numRanges;
  aSelection->GetRangeCount(&numRanges);
  NS_ASSERTION(numRanges > 0, "RenderSelection called with no selection");

  for (PRInt32 r = 0; r < numRanges; r++)
  {
    nsCOMPtr<nsIDOMRange> range;
    aSelection->GetRangeAt(r, getter_AddRefs(range));

    RangePaintInfo* info = CreateRangePaintInfo(range, area, PR_TRUE);
    if (info && !rangeItems.AppendElement(info)) {
      delete info;
      return nsnull;
    }
  }

  return PaintRangePaintInfo(&rangeItems, aSelection, nsnull, area, aPoint,
                             aScreenRect);
}

nsresult
PresShell::AddPrintPreviewBackgroundItem(nsDisplayListBuilder& aBuilder,
                                         nsDisplayList&        aList,
                                         nsIFrame*             aFrame,
                                         const nsRect&         aBounds)
{
  return aList.AppendNewToBottom(new (&aBuilder)
    nsDisplaySolidColor(&aBuilder, aFrame, aBounds, NS_RGB(115, 115, 115)));
}

static PRBool
AddCanvasBackgroundColor(const nsDisplayList& aList, nsIFrame* aCanvasFrame,
                         nscolor aColor)
{
  for (nsDisplayItem* i = aList.GetBottom(); i; i = i->GetAbove()) {
    if (i->GetUnderlyingFrame() == aCanvasFrame &&
        i->GetType() == nsDisplayItem::TYPE_CANVAS_BACKGROUND) {
      nsDisplayCanvasBackground* bg = static_cast<nsDisplayCanvasBackground*>(i);
      bg->SetExtraBackgroundColor(aColor);
      return PR_TRUE;
    }
    nsDisplayList* sublist = i->GetList();
    if (sublist && AddCanvasBackgroundColor(*sublist, aCanvasFrame, aColor))
      return PR_TRUE;
  }
  return PR_FALSE;
}

nsresult PresShell::AddCanvasBackgroundColorItem(nsDisplayListBuilder& aBuilder,
                                                 nsDisplayList&        aList,
                                                 nsIFrame*             aFrame,
                                                 const nsRect&         aBounds,
                                                 nscolor               aBackstopColor,
                                                 PRUint32              aFlags)
{
  
  
  
  
  
  
  if (!(aFlags & nsIPresShell::FORCE_DRAW) &&
      !nsCSSRendering::IsCanvasFrame(aFrame)) {
    return NS_OK;
  }

  nscolor bgcolor = NS_ComposeColors(aBackstopColor, mCanvasBackgroundColor);
  if (NS_GET_A(bgcolor) == 0)
    return NS_OK;

  
  
  
  
  if (!aFrame->GetParent()) {
    nsIScrollableFrame* sf =
      aFrame->PresContext()->PresShell()->GetRootScrollFrameAsScrollable();
    if (sf) {
      nsCanvasFrame* canvasFrame = do_QueryFrame(sf->GetScrolledFrame());
      if (canvasFrame && canvasFrame->IsVisibleForPainting(&aBuilder)) {
        if (AddCanvasBackgroundColor(aList, canvasFrame, bgcolor))
          return NS_OK;
      }
    }
  }

  return aList.AppendNewToBottom(
      new (&aBuilder) nsDisplaySolidColor(&aBuilder, aFrame, aBounds, bgcolor,
        !!(aFlags & nsIPresShell::ROOT_CONTENT_DOC_BG)));
}

static PRBool IsTransparentContainerElement(nsPresContext* aPresContext)
{
  nsCOMPtr<nsISupports> container = aPresContext->GetContainerInternal();
  nsCOMPtr<nsIDocShellTreeItem> docShellItem = do_QueryInterface(container);
  nsCOMPtr<nsPIDOMWindow> pwin(do_GetInterface(docShellItem));
  if (!pwin)
    return PR_FALSE;
  nsCOMPtr<nsIContent> containerElement =
    do_QueryInterface(pwin->GetFrameElementInternal());
  return containerElement &&
         containerElement->HasAttr(kNameSpaceID_None, nsGkAtoms::transparent);
}

void PresShell::UpdateCanvasBackground()
{
  
  
  
  nsIFrame* rootStyleFrame = FrameConstructor()->GetRootElementStyleFrame();
  if (rootStyleFrame) {
    nsStyleContext* bgStyle =
      nsCSSRendering::FindRootFrameBackground(rootStyleFrame);
    
    
    
    
    mCanvasBackgroundColor =
      nsCSSRendering::DetermineBackgroundColor(mPresContext, bgStyle,
                                               rootStyleFrame);
    if (GetPresContext()->IsRootContentDocument() &&
        !IsTransparentContainerElement(mPresContext)) {
      mCanvasBackgroundColor =
        NS_ComposeColors(mPresContext->DefaultBackgroundColor(), mCanvasBackgroundColor);
    }
  }

  
  
  
  if (!FrameConstructor()->GetRootElementFrame()) {
    mCanvasBackgroundColor = mPresContext->DefaultBackgroundColor();
  }
}

nscolor PresShell::ComputeBackstopColor(nsIView* aDisplayRoot)
{
  nsIWidget* widget = aDisplayRoot->GetWidget();
  if (widget && widget->GetTransparencyMode() != eTransparencyOpaque) {
    
    
    return NS_RGBA(0,0,0,0);
  }
  
  
  
  return GetPresContext()->DefaultBackgroundColor();
}

struct PaintParams {
  nscolor mBackgroundColor;
};

LayerManager* PresShell::GetLayerManager()
{
  NS_ASSERTION(mViewManager, "Should have view manager");

  nsIView* rootView = mViewManager->GetRootView();
  if (rootView) {
    if (nsIWidget* widget = rootView->GetWidget()) {
      return widget->GetLayerManager();
    }
  }
  return nsnull;
}

void PresShell::SetIgnoreViewportScrolling(PRBool aIgnore)
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
  if (!(aXResolution > 0.0 && aXResolution > 0.0)) {
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

void PresShell::SynthesizeMouseMove(PRBool aFromScroll)
{
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

    if (NS_FAILED(NS_DispatchToCurrentThread(ev))) {
      NS_WARNING("failed to dispatch nsSynthMouseMoveEvent");
      return;
    }

    mSynthMouseMoveEvent = ev;
  }
}













static nsIView* FindFloatingViewContaining(nsIView* aView, nsPoint aPt)
{
  if (aView->GetVisibility() == nsViewVisibility_kHide)
    
    return nsnull;

  nsIFrame* frame = static_cast<nsIFrame*>(aView->GetClientData());
  if (frame && !frame->PresContext()->PresShell()->IsActive()) {
    return nsnull;
  }

  for (nsIView* v = aView->GetFirstChild(); v; v = v->GetNextSibling()) {
    nsIView* r = FindFloatingViewContaining(v, v->ConvertFromParentCoords(aPt));
    if (r)
      return r;
  }

  if (aView->GetFloating() && aView->HasWidget() &&
      aView->GetDimensions().Contains(aPt))
    return aView;
    
  return nsnull;
}










static nsIView* FindViewContaining(nsIView* aView, nsPoint aPt)
{
  if (!aView->GetDimensions().Contains(aPt) ||
      aView->GetVisibility() == nsViewVisibility_kHide) {
    return nsnull;
  }

  nsIFrame* frame = static_cast<nsIFrame*>(aView->GetClientData());
  if (frame && !frame->PresContext()->PresShell()->IsActive()) {
    return nsnull;
  }

  for (nsIView* v = aView->GetFirstChild(); v; v = v->GetNextSibling()) {
    nsIView* r = FindViewContaining(v, v->ConvertFromParentCoords(aPt));
    if (r)
      return r;
  }

  return aView;
}

void
PresShell::ProcessSynthMouseMoveEvent(PRBool aFromScroll)
{
  
  
  if (aFromScroll) {
    mSynthMouseMoveEvent.Forget();
  }

  nsIView* rootView = mViewManager ? mViewManager->GetRootView() : nsnull;
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

  PRInt32 APD = mPresContext->AppUnitsPerDevPixel();

  
  
  
  
  nsIView* view = nsnull;

  
  PRInt32 viewAPD;

  
  
  nsPoint refpoint(0, 0);

  
  
  nsIViewManager *pointVM = nsnull;

  
  
  view = FindFloatingViewContaining(rootView, mMouseLocation);
  if (!view) {
    view = rootView;
    nsIView *pointView = FindViewContaining(rootView, mMouseLocation);
    
    pointVM = (pointView ? pointView : view)->GetViewManager();
    refpoint = mMouseLocation + rootView->ViewToWidgetOffset();
    viewAPD = APD;
  } else {
    pointVM = view->GetViewManager();
    nsIFrame* frame = static_cast<nsIFrame*>(view->GetClientData());
    NS_ASSERTION(frame, "floating views can't be anonymous");
    viewAPD = frame->PresContext()->AppUnitsPerDevPixel();
    refpoint = mMouseLocation.ConvertAppUnits(APD, viewAPD);
    refpoint -= view->GetOffsetTo(rootView);
    refpoint += view->ViewToWidgetOffset();
  }
  NS_ASSERTION(view->GetWidget(), "view should have a widget here");
  nsMouseEvent event(PR_TRUE, NS_MOUSE_MOVE, view->GetWidget(),
                     nsMouseEvent::eSynthesized);
  event.refPoint = refpoint.ToNearestPixels(viewAPD);
  event.time = PR_IntervalNow();
  

  nsCOMPtr<nsIViewObserver> observer = pointVM->GetViewObserver();
  if (observer) {
    observer->DispatchSynthMouseMove(&event, !aFromScroll);
  }

  if (!aFromScroll) {
    mSynthMouseMoveEvent.Forget();
  }
}

static void DrawThebesLayer(ThebesLayer* aLayer,
                            gfxContext* aContext,
                            const nsIntRegion& aRegionToDraw,
                            const nsIntRegion& aRegionToInvalidate,
                            void* aCallbackData)
{
  PaintParams* params = static_cast<PaintParams*>(aCallbackData);
  aContext->NewPath();
  aContext->SetColor(gfxRGBA(params->mBackgroundColor));
  nsIntRect dirtyRect = aRegionToDraw.GetBounds();
  aContext->Rectangle(
    gfxRect(dirtyRect.x, dirtyRect.y, dirtyRect.width, dirtyRect.height));
  aContext->Fill();
}

NS_IMETHODIMP
PresShell::Paint(nsIView*           aViewToPaint,
                 nsIWidget*         aWidgetToPaint,
                 const nsRegion&    aDirtyRegion,
                 const nsIntRegion& aIntDirtyRegion,
                 PRBool             aPaintDefaultBackground,
                 PRBool             aWillSendDidPaint)
{
#ifdef NS_FUNCTION_TIMER
  NS_TIME_FUNCTION_DECLARE_DOCURL;
  const nsRect& bounds__ = aDirtyRegion.GetBounds();
  NS_TIME_FUNCTION_MIN_FMT(1.0, "%s (line %d) (document: %s, dirty rect: (<%f, %f>, <%f, %f>)",
                           MOZ_FUNCTION_NAME, __LINE__, docURL__.get(),
                           NSCoordToFloat(bounds__.x),
                           NSCoordToFloat(bounds__.y),
                           NSCoordToFloat(bounds__.XMost()),
                           NSCoordToFloat(bounds__.YMost()));
#endif

  NS_ASSERTION(!mIsDestroying, "painting a destroyed PresShell");
  NS_ASSERTION(aViewToPaint, "null view");
  NS_ASSERTION(aWidgetToPaint, "Can't paint without a widget");

  nsPresContext* presContext = GetPresContext();
  AUTO_LAYOUT_PHASE_ENTRY_POINT(presContext, Paint);

  nsIFrame* frame = aPaintDefaultBackground
      ? nsnull : static_cast<nsIFrame*>(aViewToPaint->GetClientData());

  bool isRetainingManager;
  LayerManager* layerManager =
    aWidgetToPaint->GetLayerManager(&isRetainingManager);
  NS_ASSERTION(layerManager, "Must be in paint event");
  layerManager->BeginTransaction();

  if (frame && isRetainingManager) {
    
    
    
    
    
    
    if (!(frame->GetStateBits() & NS_FRAME_UPDATE_LAYER_TREE)) {
      if (layerManager->EndEmptyTransaction()) {
        frame->UpdatePaintCountForPaintedPresShells();
        presContext->NotifyDidPaintForSubtree();
        
        return NS_OK;
      }
    }
    

    frame->RemoveStateBits(NS_FRAME_UPDATE_LAYER_TREE);
  }
  if (frame) {
    frame->ClearPresShellsFromLastPaint();
  }

  nscolor bgcolor = ComputeBackstopColor(aViewToPaint);

  if (frame) {
    
    
    
    
    frame->BeginDeferringInvalidatesForDisplayRoot(aDirtyRegion);

    
    
    
    
    nsLayoutUtils::PaintFrame(nsnull, frame, aDirtyRegion, bgcolor,
                              nsLayoutUtils::PAINT_WIDGET_LAYERS |
                              nsLayoutUtils::PAINT_EXISTING_TRANSACTION);

    frame->EndDeferringInvalidatesForDisplayRoot();
    presContext->NotifyDidPaintForSubtree();
    return NS_OK;
  }

  nsRefPtr<ThebesLayer> root = layerManager->CreateThebesLayer();
  if (root) {
    root->SetVisibleRegion(aIntDirtyRegion);
    layerManager->SetRoot(root);
  }
  bgcolor = NS_ComposeColors(bgcolor, mCanvasBackgroundColor);
  PaintParams params = { bgcolor };
  layerManager->EndTransaction(DrawThebesLayer, &params);

  presContext->NotifyDidPaintForSubtree();
  return NS_OK;
}


void
nsIPresShell::SetCapturingContent(nsIContent* aContent, PRUint8 aFlags)
{
  NS_IF_RELEASE(gCaptureInfo.mContent);

  
  
  if ((aFlags & CAPTURE_IGNOREALLOWED) || gCaptureInfo.mAllowed) {
    if (aContent) {
      NS_ADDREF(gCaptureInfo.mContent = aContent);
    }
    gCaptureInfo.mRetargetToElement = (aFlags & CAPTURE_RETARGETTOELEMENT) != 0;
    gCaptureInfo.mPreventDrag = (aFlags & CAPTURE_PREVENTDRAG) != 0;
  }
}

nsIFrame*
PresShell::GetCurrentEventFrame()
{
  if (NS_UNLIKELY(mIsDestroying)) {
    return nsnull;
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
  nsIContent* content = nsnull;

  if (mCurrentEventContent) {
    content = mCurrentEventContent;
    NS_IF_ADDREF(content);
  } else {
    nsIFrame* currentEventFrame = GetCurrentEventFrame();
    if (currentEventFrame) {
      currentEventFrame->GetContentForEvent(mPresContext, aEvent, &content);
    } else {
      content = nsnull;
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
  mCurrentEventFrame = nsnull;
  mCurrentEventContent = nsnull;

  if (0 != mCurrentEventFrameStack.Length()) {
    mCurrentEventFrame = mCurrentEventFrameStack.ElementAt(0);
    mCurrentEventFrameStack.RemoveElementAt(0);
    mCurrentEventContent = mCurrentEventContentStack.ObjectAt(0);
    mCurrentEventContentStack.RemoveObjectAt(0);
  }
}

PRBool PresShell::InZombieDocument(nsIContent *aContent)
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
  NS_ENSURE_TRUE(parent, nsnull);
  return parent->GetRootWindow();
}

already_AddRefed<nsIPresShell>
PresShell::GetParentPresShell()
{
  NS_ENSURE_TRUE(mPresContext, nsnull);
  nsCOMPtr<nsISupports> container = mPresContext->GetContainer();
  if (!container) {
    container = do_QueryReferent(mForwardingContainer);
  }

  
  nsCOMPtr<nsIDocShellTreeItem> treeItem = do_QueryInterface(container);
  
  NS_ENSURE_TRUE(treeItem, nsnull);

  nsCOMPtr<nsIDocShellTreeItem> parentTreeItem;
  treeItem->GetParent(getter_AddRefs(parentTreeItem));
  nsCOMPtr<nsIDocShell> parentDocShell = do_QueryInterface(parentTreeItem);
  NS_ENSURE_TRUE(parentDocShell && treeItem != parentTreeItem, nsnull);

  nsIPresShell* parentPresShell = nsnull;
  parentDocShell->GetPresShell(&parentPresShell);
  return parentPresShell;
}

nsresult
PresShell::RetargetEventToParent(nsGUIEvent*     aEvent,
                                 nsEventStatus*  aEventStatus)
{
  
  
  
  

  nsCOMPtr<nsIPresShell> kungFuDeathGrip(this);
  nsCOMPtr<nsIPresShell> parentPresShell = GetParentPresShell();
  NS_ENSURE_TRUE(parentPresShell, NS_ERROR_FAILURE);
  nsCOMPtr<nsIViewObserver> parentViewObserver = 
    do_QueryInterface(parentPresShell);
  if (!parentViewObserver) {
    return NS_ERROR_FAILURE;
  }

  
  nsIView *parentRootView = parentPresShell->GetViewManager()->GetRootView();
  
  return parentViewObserver->HandleEvent(parentRootView, aEvent, PR_TRUE, aEventStatus);
}

void
PresShell::DisableNonTestMouseEvents(PRBool aDisable)
{
  sDisableNonTestMouseEvents = aDisable;
}

already_AddRefed<nsPIDOMWindow>
PresShell::GetFocusedDOMWindowInOurWindow()
{
  nsCOMPtr<nsPIDOMWindow> rootWindow = GetRootWindow();
  NS_ENSURE_TRUE(rootWindow, nsnull);
  nsPIDOMWindow* focusedWindow;
  nsFocusManager::GetFocusedDescendant(rootWindow, PR_TRUE, &focusedWindow);
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
      nsIView* rootView = mViewManager->GetRootView();
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
      SynthesizeMouseMove(PR_FALSE);
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

NS_IMETHODIMP
PresShell::HandleEvent(nsIView         *aView,
                       nsGUIEvent*     aEvent,
                       PRBool          aDontRetargetEvents,
                       nsEventStatus*  aEventStatus)
{
  NS_ASSERTION(aView, "null view");

  if (mIsDestroying ||
      (sDisableNonTestMouseEvents && NS_IS_MOUSE_EVENT(aEvent) &&
       !(aEvent->flags & NS_EVENT_FLAG_SYNTHETIC_TEST_EVENT))) {
    return NS_OK;
  }

  RecordMouseLocation(aEvent);

#ifdef ACCESSIBILITY
  if (aEvent->eventStructType == NS_ACCESSIBLE_EVENT) {
    NS_TIME_FUNCTION_MIN(1.0);

    
    
    return HandleEventInternal(aEvent, aView, aEventStatus);
  }
#endif

  if (!nsContentUtils::IsSafeToRunScript())
    return NS_OK;

  NS_TIME_FUNCTION_MIN(1.0);

  nsIContent* capturingContent =
    NS_IS_MOUSE_EVENT(aEvent) ? GetCapturingContent() : nsnull;

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
    }

    if (retargetEventDoc) {
      nsIPresShell* presShell = retargetEventDoc->GetShell();
      if (!presShell)
        return NS_OK;

      if (presShell != this) {
        nsCOMPtr<nsIViewObserver> viewObserver = do_QueryInterface(presShell);
        if (!viewObserver)
          return NS_ERROR_FAILURE;

        nsIView* view = presShell->GetViewManager()->GetRootView();
        return viewObserver->HandleEvent(view, aEvent, PR_TRUE, aEventStatus);
      }
    }
  }

  
  if (aEvent->message == NS_THEMECHANGED && mPresContext) {
    mPresContext->ThemeChanged();
    return NS_OK;
  }

  if (aEvent->message == NS_UISTATECHANGED && mDocument) {
    nsPIDOMWindow* win = mDocument->GetWindow();
    if (win) {
      nsUIStateChangeEvent* event = (nsUIStateChangeEvent*)aEvent;
      win->SetKeyboardIndicators(event->showAccelerators, event->showFocusRings);
    }
    return NS_OK;
  }

  
  
  if ((aEvent->message == NS_SYSCOLORCHANGED) && mPresContext) {
    nsIViewManager* vm = GetViewManager();
    if (vm) {
      
      
      
      
      nsIView* view = vm->GetRootView();
      if (view == aView) {
        *aEventStatus = nsEventStatus_eConsumeDoDefault;
        mPresContext->SysColorChanged();
        return NS_OK;
      }
    }
    return NS_OK;
  }

  if (aEvent->eventStructType == NS_KEY_EVENT &&
      mDocument && mDocument->EventHandlingSuppressed()) {
    if (aEvent->message == NS_KEY_DOWN) {
      mNoDelayedKeyEvents = PR_TRUE;
    } else if (!mNoDelayedKeyEvents) {
      nsDelayedEvent* event =
        new nsDelayedKeyEvent(static_cast<nsKeyEvent*>(aEvent));
      if (event && !mDelayedEvents.AppendElement(event)) {
        delete event;
      }
    }
    return NS_OK;
  }

  nsIFrame* frame = static_cast<nsIFrame*>(aView->GetClientData());
  PRBool dispatchUsingCoordinates = NS_IsEventUsingCoordinates(aEvent);

  
  
  if (!frame &&
      (dispatchUsingCoordinates || NS_IS_KEY_EVENT(aEvent) ||
       NS_IS_IME_RELATED_EVENT(aEvent) ||
       NS_IS_NON_RETARGETED_PLUGIN_EVENT(aEvent) ||
       aEvent->message == NS_PLUGIN_ACTIVATE ||
       aEvent->message == NS_PLUGIN_FOCUS)) {
    nsIView* targetView = aView;
    while (targetView && !targetView->GetClientData()) {
      targetView = targetView->GetParent();
    }
    
    if (targetView) {
      aView = targetView;
      frame = static_cast<nsIFrame*>(aView->GetClientData());
    }
  }

  if (dispatchUsingCoordinates) {
    NS_WARN_IF_FALSE(frame, "Nothing to handle this event!");
    if (!frame)
      return NS_OK;

    nsPresContext* framePresContext = frame->PresContext();
    nsPresContext* rootPresContext = framePresContext->GetRootPresContext();
    NS_ASSERTION(rootPresContext == mPresContext->GetRootPresContext(),
                 "How did we end up outside the connected prescontext/viewmanager hierarchy?"); 
    
    
    
    if (framePresContext == rootPresContext &&
        frame == FrameManager()->GetRootFrame()) {
      nsIFrame* popupFrame =
        nsLayoutUtils::GetPopupFrameForEventCoordinates(rootPresContext, aEvent);
      
      
      if (popupFrame &&
          !nsContentUtils::ContentIsCrossDocDescendantOf(
             framePresContext->GetPresShell()->GetDocument(),
             popupFrame->GetContent())) {
        frame = popupFrame;
      }
    }

    PRBool captureRetarget = PR_FALSE;
    if (capturingContent) {
      
      
      
      
      PRBool vis;
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
              
              
              nsIFrame* childFrame = captureFrame->GetChildList(nsGkAtoms::selectPopupList).FirstChild();
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
        ClearMouseCapture(nsnull);
        capturingContent = nsnull;
      }
    }

    
    
    
    if (!captureRetarget) {
      nsPoint eventPoint
          = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, frame);
      {
        PRBool ignoreRootScrollFrame = PR_FALSE;
        if (aEvent->eventStructType == NS_MOUSE_EVENT) {
          ignoreRootScrollFrame = static_cast<nsMouseEvent*>(aEvent)->ignoreRootScrollFrame;
        }
        nsIFrame* target = nsLayoutUtils::GetFrameForPoint(frame, eventPoint,
                                                           PR_FALSE, ignoreRootScrollFrame);
        if (target) {
          frame = target;
        }
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
        aView = frame->GetClosestView();
      }
    }

    
    
    if (aEvent->eventStructType == NS_MOUSE_EVENT &&
        frame->PresContext()->Document()->EventHandlingSuppressed()) {
      if (aEvent->message == NS_MOUSE_BUTTON_DOWN) {
        mNoDelayedMouseEvents = PR_TRUE;
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
        nsIView* activeShellRootView = shell->GetViewManager()->GetRootView();
        frame = static_cast<nsIFrame*>(activeShellRootView->GetClientData());
      }
    }

    if (shell != this) {
      
      
      nsCOMPtr<nsIPresShell> kungFuDeathGrip(shell);
      nsIView* subshellRootView = shell->GetViewManager()->GetRootView();
      
      
      
      
      return shell->HandlePositionedEvent(subshellRootView, frame,
                                          aEvent, aEventStatus);
    }

    return HandlePositionedEvent(aView, frame, aEvent, aEventStatus);
  }
  
  nsresult rv = NS_OK;
  
  if (frame) {
    PushCurrentEventInfo(nsnull, nsnull);

    
    if (NS_IsEventTargetedAtFocusedContent(aEvent)) {
      mCurrentEventContent = nsnull;

      nsCOMPtr<nsPIDOMWindow> window =
        do_QueryInterface(mDocument->GetWindow());
      nsCOMPtr<nsPIDOMWindow> focusedWindow;
      nsCOMPtr<nsIContent> eventTarget =
        nsFocusManager::GetFocusedDescendant(window, PR_FALSE,
                                             getter_AddRefs(focusedWindow));

      
      
      
      
      if (!eventTarget || !eventTarget->GetPrimaryFrame()) {
        eventTarget = mDocument->GetRootElement();
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

      mCurrentEventFrame = nsnull;
      nsIDocument* targetDoc = eventTarget ? eventTarget->GetOwnerDoc() : nsnull;
      if (targetDoc && targetDoc != mDocument) {
        PopCurrentEventInfo();
        nsIPresShell* shell = targetDoc->GetShell();
        nsCOMPtr<nsIViewObserver> vo = do_QueryInterface(shell);
        if (vo) {
          nsIView* root = shell->GetViewManager()->GetRootView();
          rv = static_cast<PresShell*>(shell)->HandleRetargetedEvent(aEvent,
                                                                     root,
                                                                     aEventStatus,
                                                                     eventTarget);
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
      rv = HandleEventInternal(aEvent, aView, aEventStatus);
    }
  
#ifdef NS_DEBUG
    ShowEventTargetDebug();
#endif
    PopCurrentEventInfo();
  } else {
    
    

    if (!NS_EVENT_NEEDS_FRAME(aEvent)) {
      mCurrentEventFrame = nsnull;
      return HandleEventInternal(aEvent, aView, aEventStatus);
    }
    else if (NS_IS_KEY_EVENT(aEvent)) {
      
      
      return RetargetEventToParent(aEvent, aEventStatus);
    }
  }

  return rv;
}

#ifdef NS_DEBUG
void
PresShell::ShowEventTargetDebug()
{
  if (nsFrame::GetShowEventTargetFrameBorder() &&
      GetCurrentEventFrame()) {
    if (mDrawEventTargetFrame) {
      mDrawEventTargetFrame->Invalidate(
          nsRect(nsPoint(0, 0), mDrawEventTargetFrame->GetSize()));
    }

    mDrawEventTargetFrame = mCurrentEventFrame;
    mDrawEventTargetFrame->Invalidate(
        nsRect(nsPoint(0, 0), mDrawEventTargetFrame->GetSize()));
  }
}
#endif

nsresult
PresShell::HandlePositionedEvent(nsIView*       aView,
                                 nsIFrame*      aTargetFrame,
                                 nsGUIEvent*    aEvent,
                                 nsEventStatus* aEventStatus)
{
  nsresult rv = NS_OK;
  
  PushCurrentEventInfo(nsnull, nsnull);
  
  mCurrentEventFrame = aTargetFrame;

  if (mCurrentEventFrame) {
    nsCOMPtr<nsIContent> targetElement;
    mCurrentEventFrame->GetContentForEvent(mPresContext, aEvent,
                                           getter_AddRefs(targetElement));

    
    
    
    if (targetElement) {
      
      
      
      
      
      
      
      
      while (targetElement && !targetElement->IsElement()) {
        targetElement = targetElement->GetParent();
      }

      
      if (!targetElement) {
        mCurrentEventContent = nsnull;
        mCurrentEventFrame = nsnull;
      } else if (targetElement != mCurrentEventContent) {
        mCurrentEventContent = targetElement;
      }
    }
  }

  if (GetCurrentEventFrame()) {
    rv = HandleEventInternal(aEvent, aView, aEventStatus);
  }

#ifdef NS_DEBUG
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
  nsresult rv = HandleEventInternal(aEvent, nsnull, aStatus);
  PopCurrentEventInfo();
  return rv;
}

static inline PRBool
IsSynthesizedMouseEvent(nsEvent* aEvent)
{
  return aEvent->eventStructType == NS_MOUSE_EVENT &&
         static_cast<nsMouseEvent*>(aEvent)->reason != nsMouseEvent::eReal;
}

static PRBool CanHandleContextMenuEvent(nsMouseEvent* aMouseEvent,
                                        nsIFrame* aFrame)
{
#if defined(XP_MACOSX) && defined(MOZ_XUL)
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm) {
    nsIFrame* popupFrame = pm->GetTopPopup(ePopupTypeMenu);
    if (popupFrame) {
      
      
      if (aMouseEvent->context == nsMouseEvent::eContextMenuKey) {
        return PR_FALSE;
      } else if (aMouseEvent->widget) {
         nsWindowType windowType;
         aMouseEvent->widget->GetWindowType(windowType);
         if (windowType == eWindowType_popup) {
           for (nsIFrame* current = aFrame; current;
                current = nsLayoutUtils::GetCrossDocParentFrame(current)) {
             if (current->GetType() == nsGkAtoms::menuPopupFrame) {
               return PR_FALSE;
             }
           }
         }
      }
    }
  }
#endif
  return PR_TRUE;
}

nsresult
PresShell::HandleEventInternal(nsEvent* aEvent, nsIView *aView,
                               nsEventStatus* aStatus)
{
  NS_TIME_FUNCTION_MIN(1.0);

#ifdef ACCESSIBILITY
  if (aEvent->eventStructType == NS_ACCESSIBLE_EVENT)
  {
    nsAccessibleEvent *accEvent = static_cast<nsAccessibleEvent*>(aEvent);
    accEvent->mAccessible = nsnull;

    nsCOMPtr<nsIAccessibilityService> accService =
      do_GetService("@mozilla.org/accessibilityService;1");
    if (accService) {
      nsCOMPtr<nsISupports> container = mPresContext->GetContainer();
      if (!container) {
        
        
        return NS_OK;
      }

      
      
      accEvent->mAccessible =
        accService->GetRootDocumentAccessible(this, nsContentUtils::IsSafeToRunScript());

      
      
      gIsAccessibilityActive = PR_TRUE;
      return NS_OK;
    }
  }
#endif

  nsRefPtr<nsEventStateManager> manager = mPresContext->EventStateManager();
  nsresult rv = NS_OK;

  if (!NS_EVENT_NEEDS_FRAME(aEvent) || GetCurrentEventFrame()) {
    PRBool isHandlingUserInput = PR_FALSE;

    
    if (NS_IS_TRUSTED_EVENT(aEvent)) {
      switch (aEvent->message) {
      case NS_MOUSE_BUTTON_DOWN:
      case NS_MOUSE_BUTTON_UP:
      case NS_KEY_PRESS:
      case NS_KEY_DOWN:
      case NS_KEY_UP:
        isHandlingUserInput = PR_TRUE;
        break;
      case NS_DRAGDROP_DROP:
        nsCOMPtr<nsIDragSession> session = nsContentUtils::GetDragSession();
        if (session) {
          PRBool onlyChromeDrop = PR_FALSE;
          session->GetOnlyChromeDrop(&onlyChromeDrop);
          if (onlyChromeDrop) {
            aEvent->flags |= NS_EVENT_FLAG_ONLY_CHROME_DISPATCH;
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
    }                                

    nsAutoHandlingUserInputStatePusher userInpStatePusher(isHandlingUserInput,
                                                          aEvent, mDocument);

    if (NS_IS_TRUSTED_EVENT(aEvent) && aEvent->message == NS_MOUSE_MOVE) {
      nsIPresShell::AllowMouseCapture(
        nsEventStateManager::GetActiveEventStateManager() == manager);
    }

    nsAutoPopupStatePusher popupStatePusher(nsDOMEvent::GetEventPopupControlState(aEvent));

    
    
    aEvent->target = nsnull;

    nsWeakView weakView(aView);
    
    
    rv = manager->PreHandleEvent(mPresContext, aEvent, mCurrentEventFrame,
                                 aStatus, aView);

    
    if (GetCurrentEventFrame() && NS_SUCCEEDED(rv)) {
      PRBool wasHandlingKeyBoardEvent =
        nsContentUtils::IsHandlingKeyBoardEvent();
      if (aEvent->eventStructType == NS_KEY_EVENT) {
        nsContentUtils::SetIsHandlingKeyBoardEvent(PR_TRUE);
      }
      
      
      
      
      if (!IsSynthesizedMouseEvent(aEvent)) {
        nsPresShellEventCB eventCB(this);
        if (mCurrentEventContent) {
          nsEventDispatcher::Dispatch(mCurrentEventContent, mPresContext,
                                      aEvent, nsnull, aStatus, &eventCB);
        }
        else {
          nsCOMPtr<nsIContent> targetContent;
          rv = mCurrentEventFrame->GetContentForEvent(mPresContext, aEvent,
                                                      getter_AddRefs(targetContent));
          if (NS_SUCCEEDED(rv) && targetContent) {
            nsEventDispatcher::Dispatch(targetContent, mPresContext, aEvent,
                                        nsnull, aStatus, &eventCB);
          } else if (mDocument) {
            nsEventDispatcher::Dispatch(mDocument, mPresContext, aEvent,
                                        nsnull, aStatus, nsnull);
          }
        }
      }

      nsContentUtils::SetIsHandlingKeyBoardEvent(wasHandlingKeyBoardEvent);

      
      
      if (!mIsDestroying && NS_SUCCEEDED(rv)) {
        rv = manager->PostHandleEvent(mPresContext, aEvent,
                                      GetCurrentEventFrame(), aStatus,
                                      weakView.GetView());
      }
    }

    if (aEvent->message == NS_MOUSE_BUTTON_UP) {
      
      SetCapturingContent(nsnull, 0);
    } else if (aEvent->message == NS_MOUSE_MOVE) {
      nsIPresShell::AllowMouseCapture(PR_FALSE);
    }
  }
  return rv;
}



nsresult
PresShell::HandleDOMEventWithTarget(nsIContent* aTargetContent, nsEvent* aEvent,
                                    nsEventStatus* aStatus)
{
  nsresult rv = NS_OK;

  PushCurrentEventInfo(nsnull, aTargetContent);

  
  
  
  
  
  nsCOMPtr<nsISupports> container = mPresContext->GetContainer();
  if (container) {

    
    rv = nsEventDispatcher::Dispatch(aTargetContent, mPresContext, aEvent, nsnull,
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

  PushCurrentEventInfo(nsnull, aTargetContent);
  nsCOMPtr<nsISupports> container = mPresContext->GetContainer();
  if (container) {
    rv = nsEventDispatcher::DispatchDOMEvent(aTargetContent, nsnull, aEvent,
                                             mPresContext, aStatus);
  }

  PopCurrentEventInfo();
  return rv;
}

PRBool
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

      return PR_TRUE;
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
      nsIFrame* rootFrame = FrameManager()->GetRootFrame();
      if (rootFrame) {
        nsIView* view = rootFrame->GetClosestView(&offset);
        offset += view->GetOffsetToWidget(aEvent->widget);
        aEvent->refPoint =
          offset.ToNearestPixels(mPresContext->AppUnitsPerDevPixel());
      }
    }
  } else {
    aEvent->widget = nsnull;
  }

  
  nsIntPoint caretPoint;
  
  
  if (PrepareToUseCaretPosition(aEvent->widget, caretPoint)) {
    
    aEvent->refPoint = caretPoint;
    return PR_TRUE;
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
      mCurrentEventFrame = nsnull;
      GetCurrentEventFrame();
    }
  }

  return PR_TRUE;
}












PRBool
PresShell::PrepareToUseCaretPosition(nsIWidget* aEventWidget, nsIntPoint& aTargetPt)
{
  nsresult rv;

  
  nsRefPtr<nsCaret> caret = GetCaret();
  NS_ENSURE_TRUE(caret, PR_FALSE);

  PRBool caretVisible = PR_FALSE;
  rv = caret->GetCaretVisible(&caretVisible);
  if (NS_FAILED(rv) || ! caretVisible)
    return PR_FALSE;

  
  
  nsISelection* domSelection = caret->GetCaretDOMSelection();
  NS_ENSURE_TRUE(domSelection, PR_FALSE);

  
  
  
  nsIFrame* frame = nsnull; 
  nsCOMPtr<nsIDOMNode> node;
  rv = domSelection->GetFocusNode(getter_AddRefs(node));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  NS_ENSURE_TRUE(node, PR_FALSE);
  nsCOMPtr<nsIContent> content(do_QueryInterface(node));
  if (content) {
    nsIContent* nonNative = content->FindFirstNonNativeAnonymous();
    content = nonNative;
  }

  if (content) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    rv = ScrollContentIntoView(content, NS_PRESSHELL_SCROLL_IF_NOT_VISIBLE,
                                        NS_PRESSHELL_SCROLL_IF_NOT_VISIBLE,
                                        SCROLL_OVERFLOW_HIDDEN);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);
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
    NS_ENSURE_SUCCESS(rv, PR_FALSE);
  }

  nsPresContext* presContext = GetPresContext();

  
  nsRect caretCoords;
  nsIFrame* caretFrame = caret->GetGeometry(domSelection, &caretCoords);
  if (!caretFrame)
    return PR_FALSE;
  nsPoint viewOffset;
  nsIView* view = caretFrame->GetClosestView(&viewOffset);
  if (!view)
    return PR_FALSE;
  
  if (aEventWidget) {
    viewOffset += view->GetOffsetToWidget(aEventWidget);
  }
  caretCoords.MoveBy(viewOffset);

  
  aTargetPt.x =
    presContext->AppUnitsToDevPixels(caretCoords.x + caretCoords.width);
  aTargetPt.y =
    presContext->AppUnitsToDevPixels(caretCoords.y + caretCoords.height);

  
  
  aTargetPt.y -= 1;

  return PR_TRUE;
}

void
PresShell::GetCurrentItemAndPositionForElement(nsIDOMElement *aCurrentEl,
                                               nsIContent** aTargetToUse,
                                               nsIntPoint& aTargetPt,
                                               nsIWidget *aRootWidget)
{
  nsCOMPtr<nsIContent> focusedContent(do_QueryInterface(aCurrentEl));
  ScrollContentIntoView(focusedContent, NS_PRESSHELL_SCROLL_ANYWHERE,
                                        NS_PRESSHELL_SCROLL_ANYWHERE,
                                        SCROLL_OVERFLOW_HIDDEN);

  nsPresContext* presContext = GetPresContext();

  PRBool istree = PR_FALSE, checkLineHeight = PR_TRUE;
  nscoord extraTreeY = 0;

#ifdef MOZ_XUL
  
  
  
  
  nsCOMPtr<nsIDOMXULSelectControlItemElement> item;
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelect =
    do_QueryInterface(aCurrentEl);
  if (multiSelect) {
    checkLineHeight = PR_FALSE;
    
    PRInt32 currentIndex;
    multiSelect->GetCurrentIndex(&currentIndex);
    if (currentIndex >= 0) {
      nsCOMPtr<nsIDOMXULElement> xulElement(do_QueryInterface(aCurrentEl));
      if (xulElement) {
        nsCOMPtr<nsIBoxObject> box;
        xulElement->GetBoxObject(getter_AddRefs(box));
        nsCOMPtr<nsITreeBoxObject> treeBox(do_QueryInterface(box));
        
        
        
        
        
        if (treeBox) {
          treeBox->EnsureRowIsVisible(currentIndex);
          PRInt32 firstVisibleRow, rowHeight;
          treeBox->GetFirstVisibleRow(&firstVisibleRow);
          treeBox->GetRowHeight(&rowHeight);

          extraTreeY += presContext->CSSPixelsToAppUnits(
                          (currentIndex - firstVisibleRow + 1) * rowHeight);
          istree = PR_TRUE;

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
        checkLineHeight = PR_FALSE;
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

    
    nsIView *view = frame->GetClosestView(&frameOrigin);
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
          PRInt32 APD = presContext->AppUnitsPerDevPixel();
          PRInt32 scrollAPD = f->PresContext()->AppUnitsPerDevPixel();
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

NS_IMETHODIMP
PresShell::ResizeReflow(nsIView *aView, nscoord aWidth, nscoord aHeight)
{
  return ResizeReflow(aWidth, aHeight);
}

NS_IMETHODIMP_(PRBool)
PresShell::ShouldIgnoreInvalidation()
{
  return mPaintingSuppressed || !mIsActive;
}

NS_IMETHODIMP_(void)
PresShell::WillPaint(PRBool aWillSendDidPaint)
{
  
  
  if (mPaintingSuppressed || !mIsActive) {
    return;
  }

  if (!aWillSendDidPaint) {
    nsRootPresContext* rootPresContext = mPresContext->GetRootPresContext();
    if (!rootPresContext) {
      return;
    }
    if (rootPresContext == mPresContext) {
      rootPresContext->UpdatePluginGeometry();
    }
  }

  
  
  
  
  FlushPendingNotifications(Flush_InterruptibleLayout);
}

NS_IMETHODIMP_(void)
PresShell::DidPaint()
{
  if (mPaintingSuppressed || !mIsActive) {
    return;
  }

  nsRootPresContext* rootPresContext = mPresContext->GetRootPresContext();
  if (!rootPresContext) {
    return;
  }
  if (rootPresContext == mPresContext) {
    rootPresContext->UpdatePluginGeometry();
  }
}

nsresult
PresShell::GetAgentStyleSheets(nsCOMArray<nsIStyleSheet>& aSheets)
{
  aSheets.Clear();
  PRInt32 sheetCount = mStyleSet->SheetCount(nsStyleSet::eAgentSheet);

  for (PRInt32 i = 0; i < sheetCount; ++i) {
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
  nsIFrame *frame = aContent->GetPrimaryFrame();
  nsIObjectFrame *objectFrame = do_QueryFrame(frame);
  if (objectFrame) {
    objectFrame->StopPlugin();
  }
}

static PRBool
FreezeSubDocument(nsIDocument *aDocument, void *aData)
{
  nsIPresShell *shell = aDocument->GetShell();
  if (shell)
    shell->Freeze();

  return PR_TRUE;
}

void
PresShell::Freeze()
{
  MaybeReleaseCapturingContent();

  mDocument->EnumerateFreezableElements(FreezeElement, nsnull);

  if (mCaret) {
    mCaret->SetCaretVisible(PR_FALSE);
  }

  mPaintingSuppressed = PR_TRUE;

  if (mDocument) {
    mDocument->EnumerateSubDocuments(FreezeSubDocument, nsnull);
  }

  nsPresContext* presContext = GetPresContext();
  if (presContext &&
      presContext->RefreshDriver()->PresContext() == presContext) {
    presContext->RefreshDriver()->Freeze();
  }

  if (presContext) {
    nsRootPresContext* rootPresContext = presContext->GetRootPresContext();
    if (rootPresContext) {
      rootPresContext->
        RootForgetUpdatePluginGeometryFrameForPresContext(mPresContext);
    }
  }

  mFrozen = PR_TRUE;
  if (mDocument) {
    UpdateImageLockingState();
  }
}

void
PresShell::FireOrClearDelayedEvents(PRBool aFireEvents)
{
  mNoDelayedMouseEvents = PR_FALSE;
  mNoDelayedKeyEvents = PR_FALSE;
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
  nsCOMPtr<nsIObjectLoadingContent> objlc(do_QueryInterface(aContent));
  if (objlc) {
    nsCOMPtr<nsIPluginInstance> inst;
    objlc->EnsureInstantiation(getter_AddRefs(inst));
  }
}

static PRBool
ThawSubDocument(nsIDocument *aDocument, void *aData)
{
  nsIPresShell *shell = aDocument->GetShell();
  if (shell)
    shell->Thaw();

  return PR_TRUE;
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
    mDocument->EnumerateSubDocuments(ThawSubDocument, nsnull);

  
  
  QueryIsActive();

  
  mFrozen = PR_FALSE;
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
    mReflowScheduled = PR_TRUE;
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
PresShell::DidDoReflow(PRBool aInterruptible)
{
  mFrameConstructor->EndUpdate();
  
  HandlePostedReflowCallbacks(aInterruptible);
  SynthesizeMouseMove(PR_FALSE);
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
  self->mReflowContinueTimer = nsnull;
  self->ScheduleReflow();
}

PRBool
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
      return PR_FALSE;
    }
  }
  return PR_TRUE;
}

PRBool
PresShell::DoReflow(nsIFrame* target, PRBool aInterruptible)
{
  NS_TIME_FUNCTION_WITH_DOCURL;

  if (mReflowContinueTimer) {
    mReflowContinueTimer->Cancel();
    mReflowContinueTimer = nsnull;
  }

  nsIFrame* rootFrame = FrameManager()->GetRootFrame();

  nsRefPtr<nsRenderingContext> rcx = GetReferenceRenderingContext();
  if (!rcx) {
    NS_NOTREACHED("CreateRenderingContext failure");
    return PR_FALSE;
  }

#ifdef DEBUG
  mCurrentReflowRoot = target;
#endif

  target->WillReflow(mPresContext);

  
  
  
  nsSize size;
  if (target == rootFrame) {
     size = mPresContext->GetVisibleArea().Size();

     
     
     target->InvalidateRectDifference(mPresContext->GetVisibleArea(),
                                      target->GetRect());
  } else {
     size = target->GetSize();
  }

  NS_ASSERTION(!target->GetNextInFlow() && !target->GetPrevInFlow(),
               "reflow roots should never split");

  
  
  nsSize reflowSize(size.width, NS_UNCONSTRAINEDSIZE);
  nsHTMLReflowState reflowState(mPresContext, target, rcx, reflowSize);

  if (rootFrame == target) {
    
    
    
    
    
    
    
    PRBool hasUnconstrainedHeight = size.height == NS_UNCONSTRAINEDSIZE;

    if (hasUnconstrainedHeight || mLastRootReflowHadUnconstrainedHeight) {
      reflowState.mFlags.mVResize = PR_TRUE;
    }

    mLastRootReflowHadUnconstrainedHeight = hasUnconstrainedHeight;
  }

  
  NS_ASSERTION(reflowState.mComputedMargin == nsMargin(0, 0, 0, 0),
               "reflow state should not set margin for reflow roots");
  if (size.height != NS_UNCONSTRAINEDSIZE) {
    nscoord computedHeight =
      size.height - reflowState.mComputedBorderPadding.TopBottom();
    computedHeight = NS_MAX(computedHeight, 0);
    reflowState.SetComputedHeight(computedHeight);
  }
  NS_ASSERTION(reflowState.ComputedWidth() ==
                 size.width -
                   reflowState.mComputedBorderPadding.LeftRight(),
               "reflow state computed incorrect width");

  mPresContext->ReflowStarted(aInterruptible);
  mIsReflowing = PR_TRUE;

  nsReflowStatus status;
  nsHTMLReflowMetrics desiredSize;
  target->Reflow(mPresContext, desiredSize, reflowState, status);

  
  
  
  
  NS_ASSERTION((target == rootFrame && size.height == NS_UNCONSTRAINEDSIZE) ||
               (desiredSize.width == size.width &&
                desiredSize.height == size.height),
               "non-root frame's desired size changed during an "
               "incremental reflow");
  NS_ASSERTION(desiredSize.VisualOverflow().IsEqualInterior(
                 nsRect(nsPoint(0, 0),
                        nsSize(desiredSize.width, desiredSize.height))),
               "reflow roots must not have visible overflow");
  NS_ASSERTION(desiredSize.ScrollableOverflow().IsEqualEdges(
                 nsRect(nsPoint(0, 0),
                        nsSize(desiredSize.width, desiredSize.height))),
               "reflow roots must not have scrollable overflow");
  NS_ASSERTION(status == NS_FRAME_COMPLETE,
               "reflow roots should never split");

  target->SetSize(nsSize(desiredSize.width, desiredSize.height));

  nsContainerFrame::SyncFrameViewAfterReflow(mPresContext, target,
                                             target->GetView(),
                                             desiredSize.VisualOverflow());
  nsContainerFrame::SyncWindowProperties(mPresContext, target,
                                         target->GetView());

  target->DidReflow(mPresContext, nsnull, NS_FRAME_REFLOW_FINISHED);
  if (target == rootFrame && size.height == NS_UNCONSTRAINEDSIZE) {
    mPresContext->SetVisibleArea(nsRect(0, 0, desiredSize.width,
                                        desiredSize.height));
  }

#ifdef DEBUG
  mCurrentReflowRoot = nsnull;
#endif

  NS_ASSERTION(mPresContext->HasPendingInterrupt() ||
               mFramesToDirty.Count() == 0,
               "Why do we need to dirty anything if not interrupted?");

  mIsReflowing = PR_FALSE;
  PRBool interrupted = mPresContext->HasPendingInterrupt();
  if (interrupted) {
    
    mFramesToDirty.EnumerateEntries(&MarkFramesDirtyToRoot, target);
    NS_ASSERTION(NS_SUBTREE_DIRTY(target), "Why is the target not dirty?");
    mDirtyRoots.AppendElement(target);

    
    
#ifdef NOISY_INTERRUPTIBLE_REFLOW
    printf("mFramesToDirty.Count() == %u\n", mFramesToDirty.Count());
#endif 
    mFramesToDirty.Clear();

    
    
    
    mSuppressInterruptibleReflows = PR_TRUE;
    MaybeScheduleReflow();
  }

  nsRootPresContext* rootPC = mPresContext->GetRootPresContext();
  if (rootPC) {
    rootPC->RequestUpdatePluginGeometry(target);
  }

  return !interrupted;
}

#ifdef DEBUG
void
PresShell::DoVerifyReflow()
{
  if (GetVerifyReflowEnable()) {
    
    
    nsIView* rootView = mViewManager->GetRootView();
    mViewManager->UpdateView(rootView, NS_VMREFRESH_IMMEDIATE);

    FlushPendingNotifications(Flush_Layout);
    mInVerifyReflow = PR_TRUE;
    PRBool ok = VerifyIncrementalReflow();
    mInVerifyReflow = PR_FALSE;
    if (VERIFY_REFLOW_ALL & gVerifyReflowFlags) {
      printf("ProcessReflowCommands: finished (%s)\n",
             ok ? "ok" : "failed");
    }

    if (0 != mDirtyRoots.Length()) {
      printf("XXX yikes! reflow commands queued during verify-reflow\n");
    }
  }
}
#endif

PRBool
PresShell::ProcessReflowCommands(PRBool aInterruptible)
{
  NS_TIME_FUNCTION_WITH_DOCURL;

  PRBool interrupted = PR_FALSE;
  if (0 != mDirtyRoots.Length()) {

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

      do {
        
        PRInt32 idx = mDirtyRoots.Length() - 1;
        nsIFrame *target = mDirtyRoots[idx];
        mDirtyRoots.RemoveElementAt(idx);

        if (!NS_SUBTREE_DIRTY(target)) {
          
          
          
          continue;
        }

        interrupted = !DoReflow(target, aInterruptible);

        
        
      } while (!interrupted && mDirtyRoots.Length() &&
               (!aInterruptible || PR_IntervalNow() < deadline));

      interrupted = mDirtyRoots.Length() != 0;
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

      
      
      
      
      
      if (mDirtyRoots.Length())
        MaybeScheduleReflow();
    }
  }

  if (!mIsDestroying && mShouldUnsuppressPainting &&
      mDirtyRoots.Length() == 0) {
    
    
    
    
    mShouldUnsuppressPainting = PR_FALSE;
    UnsuppressAndInvalidate();
  }

  return !interrupted;
}

#ifdef MOZ_XUL







typedef PRBool (* frameWalkerFn)(nsIFrame *aFrame, void *aClosure);
   
static PRBool
ReResolveMenusAndTrees(nsIFrame *aFrame, void *aClosure)
{
  
  
  nsTreeBodyFrame *treeBody = do_QueryFrame(aFrame);
  if (treeBody)
    treeBody->ClearStyleAndImageCaches();

  
  
  
  
  if (aFrame && aFrame->GetType() == nsGkAtoms::menuFrame)
    (static_cast<nsMenuFrame *>(aFrame))->CloseMenu(PR_TRUE);
  return PR_TRUE;
}

static PRBool
ReframeImageBoxes(nsIFrame *aFrame, void *aClosure)
{
  nsStyleChangeList *list = static_cast<nsStyleChangeList*>(aClosure);
  if (aFrame->GetType() == nsGkAtoms::imageBoxFrame) {
    list->AppendChange(aFrame, aFrame->GetContent(),
                       NS_STYLE_HINT_FRAMECHANGE);
    return PR_FALSE; 
  }
  return PR_TRUE; 
}

static void
WalkFramesThroughPlaceholders(nsPresContext *aPresContext, nsIFrame *aFrame,
                              frameWalkerFn aFunc, void *aClosure)
{
  PRBool walkChildren = (*aFunc)(aFrame, aClosure);
  if (!walkChildren)
    return;

  PRInt32 listIndex = 0;
  nsIAtom* childList = nsnull;

  do {
    nsIFrame *child = aFrame->GetFirstChild(childList);
    while (child) {
      if (!(child->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
        
        
        WalkFramesThroughPlaceholders(aPresContext,
                                      nsPlaceholderFrame::GetRealFrameFor(child),
                                      aFunc, aClosure);
      }
      child = child->GetNextSibling();
    }

    childList = aFrame->GetAdditionalChildListName(listIndex++);
  } while (childList);
}
#endif

NS_IMETHODIMP
PresShell::Observe(nsISupports* aSubject, 
                   const char* aTopic,
                   const PRUnichar* aData)
{
#ifdef MOZ_XUL
  if (!nsCRT::strcmp(aTopic, "chrome-flush-skin-caches")) {
    nsIFrame *rootFrame = FrameManager()->GetRootFrame();
    
    
    if (rootFrame) {
      NS_ASSERTION(mViewManager, "View manager must exist");
      nsIViewManager::UpdateViewBatch batch(mViewManager);

      nsWeakFrame weakRoot(rootFrame);
      
      
      mDocument->FlushPendingNotifications(Flush_ContentAndNotify);

      if (weakRoot.IsAlive()) {
        WalkFramesThroughPlaceholders(mPresContext, rootFrame,
                                      &ReResolveMenusAndTrees, nsnull);

        
        
        nsStyleChangeList changeList;
        WalkFramesThroughPlaceholders(mPresContext, rootFrame,
                                      ReframeImageBoxes, &changeList);
        
        
        {
          nsAutoScriptBlocker scriptBlocker;
          ++mChangeNestCount;
          mFrameConstructor->ProcessRestyledFrames(changeList);
          --mChangeNestCount;
        }
      }
      batch.EndUpdateViewBatch(NS_VMREFRESH_NO_SYNC);
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

  if (!nsCRT::strcmp(aTopic, "agent-sheet-removed") && mStyleSet) {
    RemoveSheet(nsStyleSet::eAgentSheet, aSubject);
    return NS_OK;
  }

  if (!nsCRT::strcmp(aTopic, "user-sheet-removed") && mStyleSet) {
    RemoveSheet(nsStyleSet::eUserSheet, aSubject);
    return NS_OK;
  }

#ifdef ACCESSIBILITY
  if (!nsCRT::strcmp(aTopic, "a11y-init-or-shutdown")) {
    gIsAccessibilityActive = aData && *aData == '1';
    return NS_OK;
  }
#endif
  NS_WARNING("unrecognized topic in PresShell::Observe");
  return NS_ERROR_FAILURE;
}

PRBool
nsIPresShell::AddRefreshObserverInternal(nsARefreshObserver* aObserver,
                                         mozFlushType aFlushType)
{
  nsPresContext* presContext = GetPresContext();
  return presContext ? presContext->RefreshDriver()->
    AddRefreshObserver(aObserver, aFlushType) : PR_FALSE;
}

 PRBool
nsIPresShell::AddRefreshObserverExternal(nsARefreshObserver* aObserver,
                                         mozFlushType aFlushType)
{
  return AddRefreshObserverInternal(aObserver, aFlushType);
}

PRBool
nsIPresShell::RemoveRefreshObserverInternal(nsARefreshObserver* aObserver,
                                            mozFlushType aFlushType)
{
  nsPresContext* presContext = GetPresContext();
  return presContext ? presContext->RefreshDriver()->
    RemoveRefreshObserver(aObserver, aFlushType) : PR_FALSE;
}

 PRBool
nsIPresShell::RemoveRefreshObserverExternal(nsARefreshObserver* aObserver,
                                            mozFlushType aFlushType)
{
  return RemoveRefreshObserverInternal(aObserver, aFlushType);
}







#ifdef NS_DEBUG
#include "nsViewsCID.h"
#include "nsWidgetsCID.h"
#include "nsIURL.h"
#include "nsILinkHandler.h"

static NS_DEFINE_CID(kViewManagerCID, NS_VIEW_MANAGER_CID);

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

static PRBool
CompareTrees(nsPresContext* aFirstPresContext, nsIFrame* aFirstFrame, 
             nsPresContext* aSecondPresContext, nsIFrame* aSecondFrame)
{
  if (!aFirstPresContext || !aFirstFrame || !aSecondPresContext || !aSecondFrame)
    return PR_TRUE;
  
  
  
  
  PRBool ok = PR_TRUE;
  nsIAtom* listName = nsnull;
  PRInt32 listIndex = 0;
  do {
    const nsFrameList& kids1 = aFirstFrame->GetChildList(listName);
    const nsFrameList& kids2 = aSecondFrame->GetChildList(listName);
    PRInt32 l1 = kids1.GetLength();
    PRInt32 l2 = kids2.GetLength();;
    if (l1 != l2) {
      ok = PR_FALSE;
      LogVerifyMessage(kids1.FirstChild(), kids2.FirstChild(),
                       "child counts don't match: ");
      printf("%d != %d\n", l1, l2);
      if (0 == (VERIFY_REFLOW_ALL & gVerifyReflowFlags)) {
        break;
      }
    }

    nsIntRect r1, r2;
    nsIView* v1, *v2;
    for (nsFrameList::Enumerator e1(kids1), e2(kids2);
         ;
         e1.Next(), e2.Next()) {
      nsIFrame* k1 = e1.get();
      nsIFrame* k2 = e2.get();
      if (((nsnull == k1) && (nsnull != k2)) ||
          ((nsnull != k1) && (nsnull == k2))) {
        ok = PR_FALSE;
        LogVerifyMessage(k1, k2, "child lists are different\n");
        break;
      }
      else if (nsnull != k1) {
        
        if (!k1->GetRect().IsEqualInterior(k2->GetRect())) {
          ok = PR_FALSE;
          LogVerifyMessage(k1, k2, "(frame rects)", k1->GetRect(), k2->GetRect());
        }

        
        
        
        
        v1 = k1->GetView();
        v2 = k2->GetView();
        if (((nsnull == v1) && (nsnull != v2)) ||
            ((nsnull != v1) && (nsnull == v2))) {
          ok = PR_FALSE;
          LogVerifyMessage(k1, k2, "child views are not matched\n");
        }
        else if (nsnull != v1) {
          if (!v1->GetBounds().IsEqualInterior(v2->GetBounds())) {
            LogVerifyMessage(k1, k2, "(view rects)", v1->GetBounds(), v2->GetBounds());
          }

          nsIWidget* w1 = v1->GetWidget();
          nsIWidget* w2 = v2->GetWidget();
          if (((nsnull == w1) && (nsnull != w2)) ||
              ((nsnull != w1) && (nsnull == w2))) {
            ok = PR_FALSE;
            LogVerifyMessage(k1, k2, "child widgets are not matched\n");
          }
          else if (nsnull != w1) {
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
          ok = PR_FALSE;
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

    nsIAtom* listName1 = aFirstFrame->GetAdditionalChildListName(listIndex);
    nsIAtom* listName2 = aSecondFrame->GetAdditionalChildListName(listIndex);
    listIndex++;
    if (listName1 != listName2) {
      if (0 == (VERIFY_REFLOW_ALL & gVerifyReflowFlags)) {
        ok = PR_FALSE;
      }
      LogVerifyMessage(kids1.FirstChild(), kids2.FirstChild(),
                       "child list names are not matched: ");
      nsAutoString tmp;
      if (nsnull != listName1) {
        listName1->ToString(tmp);
        fputs(NS_LossyConvertUTF16toASCII(tmp).get(), stdout);
      }
      else
        fputs("(null)", stdout);
      printf(" != ");
      if (nsnull != listName2) {
        listName2->ToString(tmp);
        fputs(NS_LossyConvertUTF16toASCII(tmp).get(), stdout);
      }
      else
        fputs("(null)", stdout);
      printf("\n");
      break;
    }
    listName = listName1;
  } while (ok && (listName != nsnull));

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
      if (nsnull != tag) {
        NS_RELEASE(tag);
        return aRoot;
      }
    }

    
    nsIFrame* kid = aRoot->GetFirstChild(nsnull);
    while (nsnull != kid) {
      nsIFrame* result = FindTopFrame(kid);
      if (nsnull != result) {
        return result;
      }
      kid = kid->GetNextSibling();
    }
  }
  return nsnull;
}
#endif


#ifdef DEBUG

nsresult
PresShell::CloneStyleSet(nsStyleSet* aSet, nsStyleSet** aResult)
{
  nsStyleSet *clone = new nsStyleSet();
  if (!clone) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  PRInt32 i, n = aSet->SheetCount(nsStyleSet::eOverrideSheet);
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
  *aResult = clone;
  return NS_OK;
}

#ifdef DEBUG_Eli
static nsresult
DumpToPNG(nsIPresShell* shell, nsAString& name) {
  PRInt32 width=1000, height=1000;
  nsRect r(0, 0, shell->GetPresContext()->DevPixelsToAppUnits(width),
                 shell->GetPresContext()->DevPixelsToAppUnits(height));

  nsRefPtr<gfxImageSurface> imgSurface =
     new gfxImageSurface(gfxIntSize(width, height),
                         gfxImageSurface::ImageFormatARGB32);
  NS_ENSURE_TRUE(imgSurface, NS_ERROR_OUT_OF_MEMORY);

  nsRefPtr<gfxContext> imgContext = new gfxContext(imgSurface);

  nsRefPtr<gfxASurface> surface = 
    gfxPlatform::GetPlatform()->
    CreateOffscreenSurface(gfxIntSize(width, height),
      gfxASurface::ContentFromFormat(gfxASurface::ImageFormatARGB32));
  NS_ENSURE_TRUE(surface, NS_ERROR_OUT_OF_MEMORY);

  nsRefPtr<gfxContext> context = new gfxContext(surface);
  NS_ENSURE_TRUE(context, NS_ERROR_OUT_OF_MEMORY);

  shell->RenderDocument(r, 0, NS_RGB(255, 255, 0), context);

  imgContext->DrawSurface(surface, gfxSize(width, height));

  nsCOMPtr<imgIEncoder> encoder = do_CreateInstance("@mozilla.org/image/encoder;2?type=image/png");
  NS_ENSURE_TRUE(encoder, NS_ERROR_FAILURE);
  encoder->InitFromData(imgSurface->Data(), imgSurface->Stride() * height,
                        width, height, imgSurface->Stride(),
                        imgIEncoder::INPUT_FORMAT_HOSTARGB, EmptyString());

  
  nsCOMPtr<nsILocalFile> file = do_CreateInstance("@mozilla.org/file/local;1");
  NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);
  rv = file->InitWithPath(name);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length;
  encoder->Available(&length);

  nsCOMPtr<nsIOutputStream> outputStream;
  rv = NS_NewLocalFileOutputStream(getter_AddRefs(outputStream), file);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIOutputStream> bufferedOutputStream;
  rv = NS_NewBufferedOutputStream(getter_AddRefs(bufferedOutputStream),
                                  outputStream, length);

  PRUint32 numWritten;
  rv = bufferedOutputStream->WriteFrom(encoder, length, &numWritten);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
#endif



PRBool
PresShell::VerifyIncrementalReflow()
{
   if (VERIFY_REFLOW_NOISY & gVerifyReflowFlags) {
     printf("Building Verification Tree...\n");
   }

  
  nsRefPtr<nsPresContext> cx =
       new nsRootPresContext(mDocument, mPresContext->IsPaginated() ?
                                        nsPresContext::eContext_PrintPreview :
                                        nsPresContext::eContext_Galley);
  NS_ENSURE_TRUE(cx, PR_FALSE);

  nsDeviceContext *dc = mPresContext->DeviceContext();
  nsresult rv = cx->Init(dc);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  
  nsIView* rootView = mViewManager->GetRootView();
  NS_ENSURE_TRUE(rootView->HasWidget(), PR_FALSE);
  nsIWidget* parentWidget = rootView->GetWidget();

  
  nsCOMPtr<nsIViewManager> vm = do_CreateInstance(kViewManagerCID);
  NS_ENSURE_TRUE(vm, PR_FALSE);
  rv = vm->Init(dc);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  
  
  nsRect tbounds = mPresContext->GetVisibleArea();
  nsIView* view = vm->CreateView(tbounds, nsnull);
  NS_ENSURE_TRUE(view, PR_FALSE);

  
  rv = view->CreateWidgetForParent(parentWidget, nsnull, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  
  vm->SetRootView(view);

  
  
  nsRect r = mPresContext->GetVisibleArea();
  cx->SetVisibleArea(r);

  
  
  nsAutoPtr<nsStyleSet> newSet;
  rv = CloneStyleSet(mStyleSet, getter_Transfers(newSet));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  nsCOMPtr<nsIPresShell> sh;
  rv = mDocument->CreateShell(cx, vm, newSet, getter_AddRefs(sh));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  newSet.forget();
  
  sh->SetVerifyReflowEnable(PR_FALSE); 
  vm->SetViewObserver((nsIViewObserver *)((PresShell*)sh.get()));
  {
    nsAutoCauseReflowNotifier crNotifier(this);
    sh->InitialReflow(r.width, r.height);
  }
  mDocument->BindingManager()->ProcessAttachedQueue();
  sh->FlushPendingNotifications(Flush_Layout);
  sh->SetVerifyReflowEnable(PR_TRUE);  
  
  
  ((PresShell*)sh.get())->mPaintingSuppressed = PR_FALSE;
  if (VERIFY_REFLOW_NOISY & gVerifyReflowFlags) {
     printf("Verification Tree built, comparing...\n");
  }

  
  
  nsIFrame* root1 = FrameManager()->GetRootFrame();
  nsIFrame* root2 = sh->FrameManager()->GetRootFrame();
  PRBool ok = CompareTrees(mPresContext, root1, cx, root2);
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
PresShell::ListStyleContexts(nsIFrame *aRootFrame, FILE *out, PRInt32 aIndent)
{
  nsStyleContext *sc = aRootFrame->GetStyleContext();
  if (sc)
    sc->List(out, aIndent);
}

void
PresShell::ListStyleSheets(FILE *out, PRInt32 aIndent)
{
  PRInt32 sheetCount = mStyleSet->SheetCount(nsStyleSet::eDocSheet);
  for (PRInt32 i = 0; i < sheetCount; ++i) {
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
    nsCAutoString uriStr;
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
                      PRUint32 aColor)
{
  if (mReflowCountMgr) {
    mReflowCountMgr->PaintCount(aName, aRenderingContext, aPresContext,
                                aFrame, aOffset, aColor);
  }
}


void
PresShell::SetPaintFrameCount(PRBool aPaintFrameCounts)
{
  if (mReflowCountMgr) {
    mReflowCountMgr->SetPaintFrameCounts(aPaintFrameCounts);
  }
}

PRBool
PresShell::IsPaintingFrameCounts()
{
  if (mReflowCountMgr)
    return mReflowCountMgr->IsPaintingFrameCounts();
  return PR_FALSE;
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


void ReflowCounter::DisplayTotals(PRUint32 aTotal, const char * aTitle)
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


void ReflowCounter::DisplayHTMLTotals(PRUint32 aTotal, const char * aTitle)
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




ReflowCountMgr::ReflowCountMgr()
{
  mCounts = PL_NewHashTable(10, PL_HashString, PL_CompareStrings, 
                                PL_CompareValues, nsnull, nsnull);
  mIndiFrameCounts = PL_NewHashTable(10, PL_HashString, PL_CompareStrings, 
                                     PL_CompareValues, nsnull, nsnull);
  mCycledOnce              = PR_FALSE;
  mDumpFrameCounts         = PR_FALSE;
  mDumpFrameByFrameCounts  = PR_FALSE;
  mPaintFrameByFrameCounts = PR_FALSE;
}


ReflowCountMgr::~ReflowCountMgr()
{
  CleanUp();
}


ReflowCounter * ReflowCountMgr::LookUp(const char * aName)
{
  if (nsnull != mCounts) {
    ReflowCounter * counter = (ReflowCounter *)PL_HashTableLookup(mCounts, aName);
    return counter;
  }
  return nsnull;

}


void ReflowCountMgr::Add(const char * aName, nsIFrame * aFrame)
{
  NS_ASSERTION(aName != nsnull, "Name shouldn't be null!");

  if (mDumpFrameCounts && nsnull != mCounts) {
    ReflowCounter * counter = (ReflowCounter *)PL_HashTableLookup(mCounts, aName);
    if (counter == nsnull) {
      counter = new ReflowCounter(this);
      NS_ASSERTION(counter != nsnull, "null ptr");
      char * name = NS_strdup(aName);
      NS_ASSERTION(name != nsnull, "null ptr");
      PL_HashTableAdd(mCounts, name, counter);
    }
    counter->Add();
  }

  if ((mDumpFrameByFrameCounts || mPaintFrameByFrameCounts) && 
      nsnull != mIndiFrameCounts && 
      aFrame != nsnull) {
    char * key = new char[16];
    sprintf(key, "%p", (void*)aFrame);
    IndiReflowCounter * counter = (IndiReflowCounter *)PL_HashTableLookup(mIndiFrameCounts, key);
    if (counter == nsnull) {
      counter = new IndiReflowCounter(this);
      NS_ASSERTION(counter != nsnull, "null ptr");
      counter->mFrame = aFrame;
      counter->mName.AssignASCII(aName);
      PL_HashTableAdd(mIndiFrameCounts, key, counter);
    }
    
    if (counter != nsnull && counter->mName.EqualsASCII(aName)) {
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
                                PRUint32        aColor)
{
  if (mPaintFrameByFrameCounts &&
      nsnull != mIndiFrameCounts &&
      aFrame != nsnull) {
    char * key = new char[16];
    sprintf(key, "%p", (void*)aFrame);
    IndiReflowCounter * counter =
      (IndiReflowCounter *)PL_HashTableLookup(mIndiFrameCounts, key);
    if (counter != nsnull && counter->mName.EqualsASCII(aName)) {
      aRenderingContext->PushState();
      aRenderingContext->Translate(aOffset);
      nsFont font("Times", NS_FONT_STYLE_NORMAL, NS_FONT_VARIANT_NORMAL,
                  NS_FONT_WEIGHT_NORMAL, NS_FONT_STRETCH_NORMAL, 0,
                  nsPresContext::CSSPixelsToAppUnits(11));

      nsRefPtr<nsFontMetrics> fm = aPresContext->GetMetricsFor(font);
      aRenderingContext->SetFont(fm);
      char buf[16];
      sprintf(buf, "%d", counter->mCount);
      nscoord x = 0, y = fm->MaxAscent();
      nscoord width, height = fm->MaxHeight();
      aRenderingContext->SetTextRunRTL(PR_FALSE);
      width = aRenderingContext->GetWidth(buf);

      PRUint32 color;
      PRUint32 color2;
      if (aColor != 0) {
        color  = aColor;
        color2 = NS_RGB(0,0,0);
      } else {
        PRUint8 rc = 0, gc = 0, bc = 0;
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


PRIntn ReflowCountMgr::RemoveItems(PLHashEntry *he, PRIntn i, void *arg)
{
  char *str = (char *)he->key;
  ReflowCounter * counter = (ReflowCounter *)he->value;
  delete counter;
  NS_Free(str);

  return HT_ENUMERATE_REMOVE;
}


PRIntn ReflowCountMgr::RemoveIndiItems(PLHashEntry *he, PRIntn i, void *arg)
{
  char *str = (char *)he->key;
  IndiReflowCounter * counter = (IndiReflowCounter *)he->value;
  delete counter;
  NS_Free(str);

  return HT_ENUMERATE_REMOVE;
}


void ReflowCountMgr::CleanUp()
{
  if (nsnull != mCounts) {
    PL_HashTableEnumerateEntries(mCounts, RemoveItems, nsnull);
    PL_HashTableDestroy(mCounts);
    mCounts = nsnull;
  }

  if (nsnull != mIndiFrameCounts) {
    PL_HashTableEnumerateEntries(mIndiFrameCounts, RemoveIndiItems, nsnull);
    PL_HashTableDestroy(mIndiFrameCounts);
    mIndiFrameCounts = nsnull;
  }
}


PRIntn ReflowCountMgr::DoSingleTotal(PLHashEntry *he, PRIntn i, void *arg)
{
  char *str = (char *)he->key;
  ReflowCounter * counter = (ReflowCounter *)he->value;

  counter->DisplayTotals(str);

  return HT_ENUMERATE_NEXT;
}


void ReflowCountMgr::DoGrandTotals()
{
  if (nsnull != mCounts) {
    ReflowCounter * gTots = (ReflowCounter *)PL_HashTableLookup(mCounts, kGrandTotalsStr);
    if (gTots == nsnull) {
      gTots = new ReflowCounter(this);
      PL_HashTableAdd(mCounts, NS_strdup(kGrandTotalsStr), gTots);
    } else {
      gTots->ClearTotals();
    }

    printf("\t\t\t\tTotal\n");
    for (PRUint32 i=0;i<78;i++) {
      printf("-");
    }
    printf("\n");
    PL_HashTableEnumerateEntries(mCounts, DoSingleTotal, this);
  }
}

static void RecurseIndiTotals(nsPresContext* aPresContext, 
                              PLHashTable *   aHT, 
                              nsIFrame *      aParentFrame,
                              PRInt32         aLevel)
{
  if (aParentFrame == nsnull) {
    return;
  }

  char key[16];
  sprintf(key, "%p", (void*)aParentFrame);
  IndiReflowCounter * counter = (IndiReflowCounter *)PL_HashTableLookup(aHT, key);
  if (counter) {
    counter->mHasBeenOutput = PR_TRUE;
    char * name = ToNewCString(counter->mName);
    for (PRInt32 i=0;i<aLevel;i++) printf(" ");
    printf("%s - %p   [%d][", name, (void*)aParentFrame, counter->mCount);
    printf("%d", counter->mCounter.GetTotal());
    printf("]\n");
    nsMemory::Free(name);
  }

  nsIFrame* child = aParentFrame->GetFirstChild(nsnull);
  while (child) {
    RecurseIndiTotals(aPresContext, aHT, child, aLevel+1);
    child = child->GetNextSibling();
  }

}


PRIntn ReflowCountMgr::DoSingleIndi(PLHashEntry *he, PRIntn i, void *arg)
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
  if (nsnull != mCounts) {
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


PRIntn ReflowCountMgr::DoSingleHTMLTotal(PLHashEntry *he, PRIntn i, void *arg)
{
  char *str = (char *)he->key;
  ReflowCounter * counter = (ReflowCounter *)he->value;

  counter->DisplayHTMLTotals(str);

  return HT_ENUMERATE_NEXT;
}


void ReflowCountMgr::DoGrandHTMLTotals()
{
  if (nsnull != mCounts) {
    ReflowCounter * gTots = (ReflowCounter *)PL_HashTableLookup(mCounts, kGrandTotalsStr);
    if (gTots == nsnull) {
      gTots = new ReflowCounter(this);
      PL_HashTableAdd(mCounts, NS_strdup(kGrandTotalsStr), gTots);
    } else {
      gTots->ClearTotals();
    }

    static const char * title[] = {"Class", "Reflows"};
    fprintf(mFD, "<tr>");
    for (PRUint32 i=0; i < NS_ARRAY_LENGTH(title); i++) {
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
    mFD = nsnull;
  }
#endif 
}


PRIntn ReflowCountMgr::DoClearTotals(PLHashEntry *he, PRIntn i, void *arg)
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
  if (nsnull != mCounts) {
    ReflowCounter * gTots = (ReflowCounter *)PL_HashTableLookup(mCounts, kGrandTotalsStr);
    if (gTots == nsnull) {
      gTots = new ReflowCounter(this);
      PL_HashTableAdd(mCounts, NS_strdup(kGrandTotalsStr), gTots);
    } else {
      gTots->ClearTotals();
      gTots->SetTotalsCache();
    }
  }
}


PRIntn ReflowCountMgr::DoDisplayDiffTotals(PLHashEntry *he, PRIntn i, void *arg)
{
  PRBool cycledOnce = (arg != 0);

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
    for (PRInt32 i=0;i<78;i++) {
      printf("-");
    }
    printf("\n");
    ClearGrandTotals();
  }
  PL_HashTableEnumerateEntries(mCounts, DoDisplayDiffTotals, (void *)mCycledOnce);

  mCycledOnce = PR_TRUE;
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
  return FrameConstructor()->GetAbsoluteContainingBlock(aFrame);
}

#ifdef ACCESSIBILITY
nsAccessibilityService*
nsIPresShell::AccService()
{
#ifdef MOZ_ENABLE_LIBXUL
  return GetAccService();
#else
  if (gIsAccessibilityActive) {
    nsCOMPtr<nsIAccessibilityService> srv =
      do_GetService("@mozilla.org/accessibilityService;1");
    return static_cast<nsAccessibilityService*>(srv.get());
  }
  return nsnull;
#endif
}
#endif

void nsIPresShell::InitializeStatics()
{
  NS_ASSERTION(sLiveShells == nsnull, "InitializeStatics called multiple times!");
  sLiveShells = new nsTHashtable<PresShellPtrKey>();
  sLiveShells->Init();
}

void nsIPresShell::ReleaseStatics()
{
  NS_ASSERTION(sLiveShells, "ReleaseStatics called without Initialize!");
  delete sLiveShells;
  sLiveShells = nsnull;
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
    PRBool isActive;
    nsresult rv = docshell->GetIsActive(&isActive);
    if (NS_SUCCEEDED(rv))
      SetIsActive(isActive);
  }
}


static PRBool
SetExternalResourceIsActive(nsIDocument* aDocument, void* aClosure)
{
  nsIPresShell* shell = aDocument->GetShell();
  if (shell) {
    shell->SetIsActive(*static_cast<PRBool*>(aClosure));
  }
  return PR_TRUE;
}

nsresult
PresShell::SetIsActive(PRBool aIsActive)
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
  nsresult rv = UpdateImageLockingState();
#ifdef ACCESSIBILITY
  nsAccessibilityService* accService = AccService();
  if (accService) {
    accService->PresShellActivated(this);
  }
#endif
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
  return nsnull;
}
