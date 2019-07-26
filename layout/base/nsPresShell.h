



















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
#include "mozilla/Attributes.h"

class nsRange;
class nsIDragService;
class nsCSSStyleSheet;

struct RangePaintInfo;
struct nsCallbackEventRequest;
#ifdef MOZ_REFLOW_PERF
class ReflowCountMgr;
#endif

class nsPresShellEventCB;
class nsAutoCauseReflowNotifier;



#define PAINTLOCK_EVENT_DELAY 250

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
                                                     ScrollAxis  aVertical,
                                                     ScrollAxis  aHorizontal,
                                                     PRUint32    aFlags);
  virtual bool ScrollFrameRectIntoView(nsIFrame*     aFrame,
                                       const nsRect& aRect,
                                       ScrollAxis    aVertical,
                                       ScrollAxis    aHorizontal,
                                       PRUint32      aFlags);
  virtual nsRectVisibility GetRectVisibility(nsIFrame *aFrame,
                                             const nsRect &aRect,
                                             nscoord aMinTwips) const;

  virtual NS_HIDDEN_(void) SetIgnoreFrameDestruction(bool aIgnore);
  virtual NS_HIDDEN_(void) NotifyDestroyingFrame(nsIFrame* aFrame);

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
                     bool aWillSendDidPaint);
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
  virtual void ScheduleViewManagerFlush();
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
  NS_IMETHOD ScrollCharacter(bool aRight);
  NS_IMETHOD CompleteScroll(bool aForward);
  NS_IMETHOD CompleteMove(bool aForward, bool aExtend);
  NS_IMETHOD SelectAll();
  NS_IMETHOD CheckVisibility(nsIDOMNode *node, PRInt16 startOffset, PRInt16 EndOffset, bool *_retval);
  virtual nsresult CheckVisibilityContent(nsIContent* aNode, PRInt16 aStartOffset,
                                          PRInt16 aEndOffset, bool* aRetval);

  
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

  void SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                           nsArenaMemoryStats *aArenaObjectsSize,
                           size_t *aPresShellSize,
                           size_t *aStyleSetsSize,
                           size_t *aTextRunsSize,
                           size_t *aPresContextSize);
  size_t SizeOfTextRuns(nsMallocSizeOfFun aMallocSizeOf) const;

  
  
  struct ScrollIntoViewData {
    ScrollAxis mContentScrollVAxis;
    ScrollAxis mContentScrollHAxis;
    PRUint32   mContentToScrollToFlags;
  };

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

  bool TouchesAreEqual(nsIDOMTouch *aTouch1, nsIDOMTouch *aTouch2);
  void DispatchTouchEvent(nsEvent *aEvent,
                          nsEventStatus* aStatus,
                          nsPresShellEventCB* aEventCB,
                          bool aTouchIsNew);

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

  
  void DoScrollContentIntoView();

  






  void SetupFontInflation();

  friend struct AutoRenderingStateSaveRestore;
  friend struct RenderingState;

  struct RenderingState {
    RenderingState(PresShell* aPresShell) 
      : mXResolution(aPresShell->mXResolution)
      , mYResolution(aPresShell->mYResolution)
      , mRenderFlags(aPresShell->mRenderFlags)
    { }
    float mXResolution;
    float mYResolution;
    RenderFlags mRenderFlags;
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
  static RenderFlags ChangeFlag(RenderFlags aFlags, bool aOnOff,
                                eRenderFlag aFlag)
  {
    return aOnOff ? (aFlags | aFlag) : (aFlag & ~aFlag);
  }


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
                         nsRange* aRange);

  
  
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
      mEvent->modifiers = aEvent->modifiers;
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

  
  
  void RecordMouseLocation(nsGUIEvent* aEvent);
  class nsSynthMouseMoveEvent MOZ_FINAL : public nsARefreshObserver {
  public:
    nsSynthMouseMoveEvent(PresShell* aPresShell, bool aFromScroll)
      : mPresShell(aPresShell), mFromScroll(aFromScroll) {
      NS_ASSERTION(mPresShell, "null parameter");
    }
    ~nsSynthMouseMoveEvent() {
      Revoke();
    }

    NS_INLINE_DECL_REFCOUNTING(nsSynthMouseMoveEvent)
    
    void Revoke() {
      if (mPresShell) {
        mPresShell->GetPresContext()->RefreshDriver()->
          RemoveRefreshObserver(this, Flush_Display);
        mPresShell = nsnull;
      }
    }
    virtual void WillRefresh(mozilla::TimeStamp aTime) {
      if (mPresShell)
        mPresShell->ProcessSynthMouseMoveEvent(mFromScroll);
    }
  private:
    PresShell* mPresShell;
    bool mFromScroll;
  };
  void ProcessSynthMouseMoveEvent(bool aFromScroll);

  void QueryIsActive();
  nsresult UpdateImageLockingState();

#ifdef ANDROID
  nsIDocument* GetTouchEventTargetDocument();
#endif
  bool InZombieDocument(nsIContent *aContent);
  already_AddRefed<nsIPresShell> GetParentPresShell();
  nsIFrame* GetCurrentEventFrame();
  nsresult RetargetEventToParent(nsGUIEvent* aEvent,
                                 nsEventStatus*  aEventStatus);
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

  virtual void SynthesizeMouseMove(bool aFromScroll);

  PresShell* GetRootPresShell();

  nscolor GetDefaultBackgroundColorToDraw();

  
  static void sPaintSuppressionCallback(nsITimer* aTimer, void* aPresShell);

  
  static void sReflowContinueCallback(nsITimer* aTimer, void* aPresShell);
  bool ScheduleReflowOffTimer();
  
#ifdef DEBUG
  
  
  nsIFrame*                 mCurrentReflowRoot;
  PRUint32                  mUpdateCount;
#endif

#ifdef MOZ_REFLOW_PERF
  ReflowCountMgr*           mReflowCountMgr;
#endif

  
  
  
  
  
  
  
  
  nsPoint                   mMouseLocation;

  
  nsRefPtr<nsCSSStyleSheet> mPrefStyleSheet; 

  
  
  nsTHashtable<nsPtrHashKey<nsIFrame> > mFramesToDirty;

  
  nsTArray<nsIFrame*>       mDirtyRoots;

  nsTArray<nsAutoPtr<nsDelayedEvent> > mDelayedEvents;
  nsRevocableEventPtr<nsRunnableMethod<PresShell> > mResizeEvent;
  nsCOMPtr<nsITimer>        mAsyncResizeEventTimer;
  nsIFrame*                 mCurrentEventFrame;
  nsCOMPtr<nsIContent>      mCurrentEventContent;
  nsTArray<nsIFrame*>       mCurrentEventFrameStack;
  nsCOMArray<nsIContent>    mCurrentEventContentStack;
  nsRevocableEventPtr<nsSynthMouseMoveEvent> mSynthMouseMoveEvent;
  nsCOMPtr<nsIContent>      mLastAnchorScrolledTo;
  nsRefPtr<nsCaret>         mCaret;
  nsRefPtr<nsCaret>         mOriginalCaret;
  nsCallbackEventRequest*   mFirstCallbackEventRequest;
  nsCallbackEventRequest*   mLastCallbackEventRequest;

  
  
  
  nsCOMPtr<nsITimer>        mPaintSuppressionTimer;

  
  
  
  
  nsCOMPtr<nsITimer>        mReflowContinueTimer;

  
  
  
  
  
  nsCOMPtr<nsIContent>      mContentToScrollTo;

  nscoord                   mLastAnchorScrollPositionY;

  
  
  
  PRUint16                  mChangeNestCount;
  
  bool                      mDocumentLoading : 1;
  bool                      mIgnoreFrameDestruction : 1;
  bool                      mHaveShutDown : 1;
  bool                      mViewportOverridden : 1;
  bool                      mLastRootReflowHadUnconstrainedHeight : 1;
  bool                      mNoDelayedMouseEvents : 1;
  bool                      mNoDelayedKeyEvents : 1;

  
  
  bool                      mIsDocumentGone : 1;

  
  
  bool                      mShouldUnsuppressPainting : 1;

  bool                      mAsyncResizeTimerIsActive : 1;
  bool                      mInResize : 1;

  static bool               sDisableNonTestMouseEvents;
};

#endif 
