

























































#ifndef nsPresShell_h_
#define nsPresShell_h_

#include "nsIPresShell.h"
#include "nsStubDocumentObserver.h"
#include "nsISelectionController.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsCRT.h"
#include "nsAutoPtr.h"
#include "nsIWidget.h"
#include "nsStyleSet.h"
#include "nsPresArena.h"
#include "nsFrameSelection.h"
#include "nsGUIEvent.h"
#include "nsContentUtils.h"
#include "nsRefreshDriver.h"

class nsIRange;
class nsIDragService;
class nsCSSStyleSheet;

struct RangePaintInfo;
struct nsCallbackEventRequest;
#ifdef MOZ_REFLOW_PERF
class ReflowCountMgr;
#endif

#define STACK_ARENA_MARK_INCREMENT 50

#define STACK_ARENA_BLOCK_INCREMENT 4044




struct StackBlock {
   
   
   
   char mBlock[STACK_ARENA_BLOCK_INCREMENT];

   
   
   
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

class nsPresShellEventCB;
class nsAutoCauseReflowNotifier;

class PresShell : public nsIPresShell,
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

  virtual NS_HIDDEN_(nsresult) SetPreferenceStyleRules(bool aForceReflow);

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
  virtual NS_HIDDEN_(bool) IsSafeToFlush() const;
  virtual NS_HIDDEN_(void) FlushPendingNotifications(mozFlushType aType);

  


  virtual NS_HIDDEN_(nsresult) RecreateFramesFor(nsIContent* aContent);

  


  virtual NS_HIDDEN_(nsresult) PostReflowCallback(nsIReflowCallback* aCallback);
  virtual NS_HIDDEN_(void) CancelReflowCallback(nsIReflowCallback* aCallback);

  virtual NS_HIDDEN_(void) ClearFrameRefs(nsIFrame* aFrame);
  virtual NS_HIDDEN_(already_AddRefed<nsRenderingContext>) GetReferenceRenderingContext();
  virtual NS_HIDDEN_(nsresult) GoToAnchor(const nsAString& aAnchorName, bool aScroll);
  virtual NS_HIDDEN_(nsresult) ScrollToAnchor();

  virtual NS_HIDDEN_(nsresult) ScrollContentIntoView(nsIContent* aContent,
                                                     PRIntn      aVPercent,
                                                     PRIntn      aHPercent,
                                                     PRUint32    aFlags);
  virtual bool ScrollFrameRectIntoView(nsIFrame*     aFrame,
                                         const nsRect& aRect,
                                         PRIntn        aVPercent,
                                         PRIntn        aHPercent,
                                         PRUint32      aFlags);
  virtual nsRectVisibility GetRectVisibility(nsIFrame *aFrame,
                                             const nsRect &aRect,
                                             nscoord aMinTwips) const;

  virtual NS_HIDDEN_(void) SetIgnoreFrameDestruction(bool aIgnore);
  virtual NS_HIDDEN_(void) NotifyDestroyingFrame(nsIFrame* aFrame);

  virtual NS_HIDDEN_(nsresult) GetLinkLocation(nsIDOMNode* aNode, nsAString& aLocationString) const;

  virtual NS_HIDDEN_(nsresult) CaptureHistoryState(nsILayoutHistoryState** aLayoutHistoryState, bool aLeavingPage);

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
  virtual void FireOrClearDelayedEvents(bool aFireEvents);

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

  virtual void SetIgnoreViewportScrolling(bool aIgnore);

  virtual void SetDisplayPort(const nsRect& aDisplayPort);

  virtual nsresult SetResolution(float aXResolution, float aYResolution);

  

  virtual void Paint(nsIView* aViewToPaint, nsIWidget* aWidget,
                     const nsRegion& aDirtyRegion, const nsIntRegion& aIntDirtyRegion,
                     bool aPaintDefaultBackground, bool aWillSendDidPaint);
  virtual nsresult HandleEvent(nsIFrame*       aFrame,
                               nsGUIEvent*     aEvent,
                               bool            aDontRetargetEvents,
                               nsEventStatus*  aEventStatus);
  virtual NS_HIDDEN_(nsresult) HandleDOMEventWithTarget(nsIContent* aTargetContent,
                                                        nsEvent* aEvent,
                                                        nsEventStatus* aStatus);
  virtual NS_HIDDEN_(nsresult) HandleDOMEventWithTarget(nsIContent* aTargetContent,
                                                        nsIDOMEvent* aEvent,
                                                        nsEventStatus* aStatus);
  virtual bool ShouldIgnoreInvalidation();
  virtual void WillPaint(bool aWillSendDidPaint);
  virtual void DidPaint();
  virtual void DispatchSynthMouseMove(nsGUIEvent *aEvent, bool aFlushOnHoverChange);
  virtual void ClearMouseCaptureOnView(nsIView* aView);
  virtual bool IsVisible();

  
  virtual NS_HIDDEN_(already_AddRefed<nsCaret>) GetCaret() const;
  virtual NS_HIDDEN_(void) MaybeInvalidateCaretPosition();
  NS_IMETHOD SetCaretEnabled(bool aInEnable);
  NS_IMETHOD SetCaretReadOnly(bool aReadOnly);
  NS_IMETHOD GetCaretEnabled(bool *aOutEnabled);
  NS_IMETHOD SetCaretVisibilityDuringSelection(bool aVisibility);
  NS_IMETHOD GetCaretVisible(bool *_retval);
  virtual void SetCaret(nsCaret *aNewCaret);
  virtual void RestoreCaret();

  NS_IMETHOD SetSelectionFlags(PRInt16 aInEnable);
  NS_IMETHOD GetSelectionFlags(PRInt16 *aOutEnable);

  

  NS_IMETHOD CharacterMove(bool aForward, bool aExtend);
  NS_IMETHOD CharacterExtendForDelete();
  NS_IMETHOD CharacterExtendForBackspace();
  NS_IMETHOD WordMove(bool aForward, bool aExtend);
  NS_IMETHOD WordExtendForDelete(bool aForward);
  NS_IMETHOD LineMove(bool aForward, bool aExtend);
  NS_IMETHOD IntraLineMove(bool aForward, bool aExtend);
  NS_IMETHOD PageMove(bool aForward, bool aExtend);
  NS_IMETHOD ScrollPage(bool aForward);
  NS_IMETHOD ScrollLine(bool aForward);
  NS_IMETHOD ScrollHorizontal(bool aLeft);
  NS_IMETHOD CompleteScroll(bool aForward);
  NS_IMETHOD CompleteMove(bool aForward, bool aExtend);
  NS_IMETHOD SelectAll();
  NS_IMETHOD CheckVisibility(nsIDOMNode *node, PRInt16 startOffset, PRInt16 EndOffset, bool *_retval);

  
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
  virtual NS_HIDDEN_(void) SetPaintFrameCount(bool aOn);
  virtual bool IsPaintingFrameCounts();
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

  virtual NS_HIDDEN_(void) DisableNonTestMouseEvents(bool aDisable);

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

  virtual NS_HIDDEN_(nsresult) SetIsActive(bool aIsActive);

  virtual bool GetIsViewportOverridden() { return mViewportOverridden; }

  virtual bool IsLayoutFlushObserver()
  {
    return GetPresContext()->RefreshDriver()->
      IsLayoutFlushObserver(this);
  }

protected:
  virtual ~PresShell();

  void HandlePostedReflowCallbacks(bool aInterruptible);
  void CancelPostedReflowCallbacks();

  void UnsuppressAndInvalidate();

  void WillCauseReflow() {
    nsContentUtils::AddScriptBlocker();
    ++mChangeNestCount;
  }
  nsresult DidCauseReflow();
  friend class nsAutoCauseReflowNotifier;

  void     WillDoReflow();
  void     DidDoReflow(bool aInterruptible);
  
  
  bool     ProcessReflowCommands(bool aInterruptible);
  
  
  
  void     MaybeScheduleReflow();
  
  
  
  void     ScheduleReflow();

  
  nsresult ResizeReflowIgnoreOverride(nscoord aWidth, nscoord aHeight);

  
  bool DoReflow(nsIFrame* aFrame, bool aInterruptible);
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

  bool mCaretEnabled;
#ifdef NS_DEBUG
  nsStyleSet* CloneStyleSet(nsStyleSet* aSet);
  bool VerifyIncrementalReflow();
  bool mInVerifyReflow;
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
                                       bool aForPrimarySelection);

  









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
    nsRefPtr<nsFrameSelection> frameSelection = FrameSelection();
    if (frameSelection) {
      frameSelection->SetMouseDownState(false);
    }
    if (gCaptureInfo.mContent &&
        gCaptureInfo.mContent->OwnerDoc() == mDocument) {
      SetCapturingContent(nsnull, 0);
    }
  }

  nsresult HandleRetargetedEvent(nsEvent* aEvent, nsEventStatus* aStatus, nsIContent* aTarget)
  {
    PushCurrentEventInfo(nsnull, nsnull);
    mCurrentEventContent = aTarget;
    nsresult rv = NS_OK;
    if (GetCurrentEventFrame()) {
      rv = HandleEventInternal(aEvent, aStatus);
    }
    PopCurrentEventInfo();
    return rv;
  }

  nsRefPtr<nsCSSStyleSheet> mPrefStyleSheet; 
                                             
#ifdef DEBUG
  PRUint32                  mUpdateCount;
#endif
  
  nsTArray<nsIFrame*> mDirtyRoots;

  bool mDocumentLoading;

  bool mIgnoreFrameDestruction;
  bool mHaveShutDown;

  bool mViewportOverridden;

  bool mLastRootReflowHadUnconstrainedHeight;

  
  
  
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
      Init(aEvent);
      static_cast<nsMouseEvent*>(mEvent)->clickCount = aEvent->clickCount;
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
      Init(aEvent);
      static_cast<nsKeyEvent*>(mEvent)->keyCode = aEvent->keyCode;
      static_cast<nsKeyEvent*>(mEvent)->charCode = aEvent->charCode;
      static_cast<nsKeyEvent*>(mEvent)->alternativeCharCodes =
        aEvent->alternativeCharCodes;
      static_cast<nsKeyEvent*>(mEvent)->isChar = aEvent->isChar;
    }

    virtual ~nsDelayedKeyEvent()
    {
      delete static_cast<nsKeyEvent*>(mEvent);
    }
  };

  bool                                 mNoDelayedMouseEvents;
  bool                                 mNoDelayedKeyEvents;
  nsTArray<nsAutoPtr<nsDelayedEvent> > mDelayedEvents;

  nsCallbackEventRequest* mFirstCallbackEventRequest;
  nsCallbackEventRequest* mLastCallbackEventRequest;

  bool              mIsDocumentGone;      
                                          
                                          
  bool              mShouldUnsuppressPainting;  
                                                
  nsCOMPtr<nsITimer> mPaintSuppressionTimer; 
                                             
                                             
#define PAINTLOCK_EVENT_DELAY 250 // 250ms.  This is actually
                                  
                                  
                                  

  static void sPaintSuppressionCallback(nsITimer* aTimer, void* aPresShell); 

  
  
  
  
  nsCOMPtr<nsITimer> mReflowContinueTimer;
  static void sReflowContinueCallback(nsITimer* aTimer, void* aPresShell);
  bool ScheduleReflowOffTimer();
  
#ifdef MOZ_REFLOW_PERF
  ReflowCountMgr * mReflowCountMgr;
#endif

  static bool sDisableNonTestMouseEvents;

private:

  bool InZombieDocument(nsIContent *aContent);
  already_AddRefed<nsIPresShell> GetParentPresShell();
  nsresult RetargetEventToParent(nsGUIEvent* aEvent,
                                 nsEventStatus*  aEventStatus);

  
protected:
  
  nsIFrame* GetCurrentEventFrame();
private:
  void PushCurrentEventInfo(nsIFrame* aFrame, nsIContent* aContent);
  void PopCurrentEventInfo();
  nsresult HandleEventInternal(nsEvent* aEvent, nsEventStatus *aStatus);
  nsresult HandlePositionedEvent(nsIFrame*      aTargetFrame,
                                 nsGUIEvent*    aEvent,
                                 nsEventStatus* aEventStatus);
  
  
  already_AddRefed<nsPIDOMWindow> GetFocusedDOMWindowInOurWindow();

  














  bool AdjustContextMenuKeyEvent(nsMouseEvent* aEvent);

  
  bool PrepareToUseCaretPosition(nsIWidget* aEventWidget, nsIntPoint& aTargetPt);

  
  
  void GetCurrentItemAndPositionForElement(nsIDOMElement *aCurrentEl,
                                           nsIContent **aTargetToUse,
                                           nsIntPoint& aTargetPt,
                                           nsIWidget *aRootWidget);

  void FireResizeEvent();
  void FireBeforeResizeEvent();
  static void AsyncResizeEventCallback(nsITimer* aTimer, void* aPresShell);
  nsRevocableEventPtr<nsRunnableMethod<PresShell> > mResizeEvent;
  nsCOMPtr<nsITimer> mAsyncResizeEventTimer;
  bool mAsyncResizeTimerIsActive;
  bool mInResize;

  virtual void SynthesizeMouseMove(bool aFromScroll);

  
  
  void RecordMouseLocation(nsGUIEvent* aEvent);
  
  
  
  
  
  
  
  
  nsPoint mMouseLocation;
  class nsSynthMouseMoveEvent : public nsRunnable {
  public:
    nsSynthMouseMoveEvent(PresShell* aPresShell, bool aFromScroll)
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
    bool mFromScroll;
  };
  nsRevocableEventPtr<nsSynthMouseMoveEvent> mSynthMouseMoveEvent;
  void ProcessSynthMouseMoveEvent(bool aFromScroll);

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

  PRUint64 ComputeTextRunMemoryUsed();

  class MemoryReporter : public nsIMemoryMultiReporter
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIMEMORYMULTIREPORTER
  protected:
    static PLDHashOperator SizeEnumerator(PresShellPtrKey *aEntry, void *userArg);
  };

protected:
  void QueryIsActive();
  nsresult UpdateImageLockingState();

private:
  nscolor GetDefaultBackgroundColorToDraw();
};

#endif 
