



















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
#include "nsFrameSelection.h"
#include "nsContentUtils.h" 
#include "nsRefreshDriver.h"
#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/TextEvents.h"

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

  void Init(nsIDocument* aDocument, nsPresContext* aPresContext,
            nsViewManager* aViewManager, nsStyleSet* aStyleSet,
            nsCompatibility aCompatMode);
  virtual NS_HIDDEN_(void) Destroy() MOZ_OVERRIDE;
  virtual NS_HIDDEN_(void) MakeZombie() MOZ_OVERRIDE;

  virtual NS_HIDDEN_(nsresult) SetPreferenceStyleRules(bool aForceReflow) MOZ_OVERRIDE;

  NS_IMETHOD GetSelection(SelectionType aType, nsISelection** aSelection);
  virtual mozilla::Selection* GetCurrentSelection(SelectionType aType) MOZ_OVERRIDE;

  NS_IMETHOD SetDisplaySelection(int16_t aToggle) MOZ_OVERRIDE;
  NS_IMETHOD GetDisplaySelection(int16_t *aToggle) MOZ_OVERRIDE;
  NS_IMETHOD ScrollSelectionIntoView(SelectionType aType, SelectionRegion aRegion,
                                     int16_t aFlags) MOZ_OVERRIDE;
  NS_IMETHOD RepaintSelection(SelectionType aType) MOZ_OVERRIDE;

  virtual NS_HIDDEN_(void) BeginObservingDocument() MOZ_OVERRIDE;
  virtual NS_HIDDEN_(void) EndObservingDocument() MOZ_OVERRIDE;
  virtual NS_HIDDEN_(nsresult) Initialize(nscoord aWidth, nscoord aHeight) MOZ_OVERRIDE;
  virtual NS_HIDDEN_(nsresult) ResizeReflow(nscoord aWidth, nscoord aHeight) MOZ_OVERRIDE;
  virtual NS_HIDDEN_(nsresult) ResizeReflowOverride(nscoord aWidth, nscoord aHeight) MOZ_OVERRIDE;
  virtual NS_HIDDEN_(nsIPageSequenceFrame*) GetPageSequenceFrame() const MOZ_OVERRIDE;
  virtual NS_HIDDEN_(nsIFrame*) GetRealPrimaryFrameFor(nsIContent* aContent) const MOZ_OVERRIDE;

  virtual NS_HIDDEN_(nsIFrame*) GetPlaceholderFrameFor(nsIFrame* aFrame) const MOZ_OVERRIDE;
  virtual NS_HIDDEN_(void) FrameNeedsReflow(nsIFrame *aFrame, IntrinsicDirty aIntrinsicDirty,
                                            nsFrameState aBitToAdd) MOZ_OVERRIDE;
  virtual NS_HIDDEN_(void) FrameNeedsToContinueReflow(nsIFrame *aFrame) MOZ_OVERRIDE;
  virtual NS_HIDDEN_(void) CancelAllPendingReflows() MOZ_OVERRIDE;
  virtual NS_HIDDEN_(bool) IsSafeToFlush() const MOZ_OVERRIDE;
  virtual NS_HIDDEN_(void) FlushPendingNotifications(mozFlushType aType) MOZ_OVERRIDE;
  virtual NS_HIDDEN_(void) FlushPendingNotifications(mozilla::ChangesToFlush aType) MOZ_OVERRIDE;

  


  virtual NS_HIDDEN_(nsresult) RecreateFramesFor(nsIContent* aContent) MOZ_OVERRIDE;

  


  virtual NS_HIDDEN_(nsresult) PostReflowCallback(nsIReflowCallback* aCallback) MOZ_OVERRIDE;
  virtual NS_HIDDEN_(void) CancelReflowCallback(nsIReflowCallback* aCallback) MOZ_OVERRIDE;

  virtual NS_HIDDEN_(void) ClearFrameRefs(nsIFrame* aFrame) MOZ_OVERRIDE;
  virtual NS_HIDDEN_(already_AddRefed<nsRenderingContext>) GetReferenceRenderingContext();
  virtual NS_HIDDEN_(nsresult) GoToAnchor(const nsAString& aAnchorName, bool aScroll) MOZ_OVERRIDE;
  virtual NS_HIDDEN_(nsresult) ScrollToAnchor() MOZ_OVERRIDE;

  virtual NS_HIDDEN_(nsresult) ScrollContentIntoView(nsIContent* aContent,
                                                     ScrollAxis  aVertical,
                                                     ScrollAxis  aHorizontal,
                                                     uint32_t    aFlags) MOZ_OVERRIDE;
  virtual bool ScrollFrameRectIntoView(nsIFrame*     aFrame,
                                       const nsRect& aRect,
                                       ScrollAxis    aVertical,
                                       ScrollAxis    aHorizontal,
                                       uint32_t      aFlags) MOZ_OVERRIDE;
  virtual nsRectVisibility GetRectVisibility(nsIFrame *aFrame,
                                             const nsRect &aRect,
                                             nscoord aMinTwips) const MOZ_OVERRIDE;

  virtual NS_HIDDEN_(void) SetIgnoreFrameDestruction(bool aIgnore) MOZ_OVERRIDE;
  virtual NS_HIDDEN_(void) NotifyDestroyingFrame(nsIFrame* aFrame) MOZ_OVERRIDE;

  virtual NS_HIDDEN_(nsresult) CaptureHistoryState(nsILayoutHistoryState** aLayoutHistoryState) MOZ_OVERRIDE;

  virtual NS_HIDDEN_(void) UnsuppressPainting() MOZ_OVERRIDE;

  virtual nsresult GetAgentStyleSheets(nsCOMArray<nsIStyleSheet>& aSheets) MOZ_OVERRIDE;
  virtual nsresult SetAgentStyleSheets(const nsCOMArray<nsIStyleSheet>& aSheets) MOZ_OVERRIDE;

  virtual nsresult AddOverrideStyleSheet(nsIStyleSheet *aSheet) MOZ_OVERRIDE;
  virtual nsresult RemoveOverrideStyleSheet(nsIStyleSheet *aSheet) MOZ_OVERRIDE;

  virtual NS_HIDDEN_(nsresult) HandleEventWithTarget(
                                 mozilla::WidgetEvent* aEvent,
                                 nsIFrame* aFrame,
                                 nsIContent* aContent,
                                 nsEventStatus* aStatus) MOZ_OVERRIDE;
  virtual NS_HIDDEN_(nsIFrame*) GetEventTargetFrame() MOZ_OVERRIDE;
  virtual NS_HIDDEN_(already_AddRefed<nsIContent>) GetEventTargetContent(
                                                     mozilla::WidgetEvent* aEvent) MOZ_OVERRIDE;


  virtual nsresult ReconstructFrames(void) MOZ_OVERRIDE;
  virtual void Freeze() MOZ_OVERRIDE;
  virtual void Thaw() MOZ_OVERRIDE;
  virtual void FireOrClearDelayedEvents(bool aFireEvents) MOZ_OVERRIDE;

  virtual NS_HIDDEN_(nsresult) RenderDocument(const nsRect& aRect, uint32_t aFlags,
                                              nscolor aBackgroundColor,
                                              gfxContext* aThebesContext) MOZ_OVERRIDE;

  virtual already_AddRefed<gfxASurface> RenderNode(nsIDOMNode* aNode,
                                                   nsIntRegion* aRegion,
                                                   nsIntPoint& aPoint,
                                                   nsIntRect* aScreenRect) MOZ_OVERRIDE;

  virtual already_AddRefed<gfxASurface> RenderSelection(nsISelection* aSelection,
                                                        nsIntPoint& aPoint,
                                                        nsIntRect* aScreenRect) MOZ_OVERRIDE;

  virtual already_AddRefed<nsPIDOMWindow> GetRootWindow() MOZ_OVERRIDE;

  virtual LayerManager* GetLayerManager() MOZ_OVERRIDE;

  virtual void SetIgnoreViewportScrolling(bool aIgnore) MOZ_OVERRIDE;

  virtual void SetDisplayPort(const nsRect& aDisplayPort);

  virtual nsresult SetResolution(float aXResolution, float aYResolution) MOZ_OVERRIDE;

  

  virtual void Paint(nsView* aViewToPaint, const nsRegion& aDirtyRegion,
                     uint32_t aFlags) MOZ_OVERRIDE;
  virtual nsresult HandleEvent(nsIFrame* aFrame,
                               mozilla::WidgetGUIEvent* aEvent,
                               bool aDontRetargetEvents,
                               nsEventStatus* aEventStatus) MOZ_OVERRIDE;
  virtual NS_HIDDEN_(nsresult) HandleDOMEventWithTarget(
                                 nsIContent* aTargetContent,
                                 mozilla::WidgetEvent* aEvent,
                                 nsEventStatus* aStatus) MOZ_OVERRIDE;
  virtual NS_HIDDEN_(nsresult) HandleDOMEventWithTarget(nsIContent* aTargetContent,
                                                        nsIDOMEvent* aEvent,
                                                        nsEventStatus* aStatus) MOZ_OVERRIDE;
  virtual bool ShouldIgnoreInvalidation() MOZ_OVERRIDE;
  virtual void WillPaint() MOZ_OVERRIDE;
  virtual void WillPaintWindow() MOZ_OVERRIDE;
  virtual void DidPaintWindow() MOZ_OVERRIDE;
  virtual void ScheduleViewManagerFlush() MOZ_OVERRIDE;
  virtual void DispatchSynthMouseMove(mozilla::WidgetGUIEvent* aEvent,
                                      bool aFlushOnHoverChange) MOZ_OVERRIDE;
  virtual void ClearMouseCaptureOnView(nsView* aView) MOZ_OVERRIDE;
  virtual bool IsVisible() MOZ_OVERRIDE;

  
  virtual NS_HIDDEN_(already_AddRefed<nsCaret>) GetCaret() const MOZ_OVERRIDE;
  virtual NS_HIDDEN_(void) MaybeInvalidateCaretPosition() MOZ_OVERRIDE;
  NS_IMETHOD SetCaretEnabled(bool aInEnable) MOZ_OVERRIDE;
  NS_IMETHOD SetCaretReadOnly(bool aReadOnly) MOZ_OVERRIDE;
  NS_IMETHOD GetCaretEnabled(bool *aOutEnabled) MOZ_OVERRIDE;
  NS_IMETHOD SetCaretVisibilityDuringSelection(bool aVisibility) MOZ_OVERRIDE;
  NS_IMETHOD GetCaretVisible(bool *_retval) MOZ_OVERRIDE;
  virtual void SetCaret(nsCaret *aNewCaret) MOZ_OVERRIDE;
  virtual void RestoreCaret() MOZ_OVERRIDE;

  NS_IMETHOD SetSelectionFlags(int16_t aInEnable) MOZ_OVERRIDE;
  NS_IMETHOD GetSelectionFlags(int16_t *aOutEnable) MOZ_OVERRIDE;

  

  NS_IMETHOD CharacterMove(bool aForward, bool aExtend) MOZ_OVERRIDE;
  NS_IMETHOD CharacterExtendForDelete() MOZ_OVERRIDE;
  NS_IMETHOD CharacterExtendForBackspace() MOZ_OVERRIDE;
  NS_IMETHOD WordMove(bool aForward, bool aExtend) MOZ_OVERRIDE;
  NS_IMETHOD WordExtendForDelete(bool aForward) MOZ_OVERRIDE;
  NS_IMETHOD LineMove(bool aForward, bool aExtend) MOZ_OVERRIDE;
  NS_IMETHOD IntraLineMove(bool aForward, bool aExtend) MOZ_OVERRIDE;
  NS_IMETHOD PageMove(bool aForward, bool aExtend) MOZ_OVERRIDE;
  NS_IMETHOD ScrollPage(bool aForward) MOZ_OVERRIDE;
  NS_IMETHOD ScrollLine(bool aForward) MOZ_OVERRIDE;
  NS_IMETHOD ScrollCharacter(bool aRight) MOZ_OVERRIDE;
  NS_IMETHOD CompleteScroll(bool aForward) MOZ_OVERRIDE;
  NS_IMETHOD CompleteMove(bool aForward, bool aExtend) MOZ_OVERRIDE;
  NS_IMETHOD SelectAll() MOZ_OVERRIDE;
  NS_IMETHOD CheckVisibility(nsIDOMNode *node, int16_t startOffset, int16_t EndOffset, bool *_retval) MOZ_OVERRIDE;
  virtual nsresult CheckVisibilityContent(nsIContent* aNode, int16_t aStartOffset,
                                          int16_t aEndOffset, bool* aRetval) MOZ_OVERRIDE;

  
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

  
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATAWILLCHANGE
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTEWILLCHANGE
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  NS_DECL_NSIOBSERVER

#ifdef MOZ_REFLOW_PERF
  virtual NS_HIDDEN_(void) DumpReflows() MOZ_OVERRIDE;
  virtual NS_HIDDEN_(void) CountReflows(const char * aName, nsIFrame * aFrame) MOZ_OVERRIDE;
  virtual NS_HIDDEN_(void) PaintCount(const char * aName,
                                      nsRenderingContext* aRenderingContext,
                                      nsPresContext* aPresContext,
                                      nsIFrame * aFrame,
                                      const nsPoint& aOffset,
                                      uint32_t aColor) MOZ_OVERRIDE;
  virtual NS_HIDDEN_(void) SetPaintFrameCount(bool aOn) MOZ_OVERRIDE;
  virtual bool IsPaintingFrameCounts() MOZ_OVERRIDE;
#endif

#ifdef DEBUG
  virtual void ListStyleContexts(nsIFrame *aRootFrame, FILE *out,
                                 int32_t aIndent = 0) MOZ_OVERRIDE;

  virtual void ListStyleSheets(FILE *out, int32_t aIndent = 0) MOZ_OVERRIDE;
  virtual void VerifyStyleTree() MOZ_OVERRIDE;
#endif

#ifdef PR_LOGGING
  static PRLogModuleInfo* gLog;
#endif

  virtual NS_HIDDEN_(void) DisableNonTestMouseEvents(bool aDisable) MOZ_OVERRIDE;

  virtual void UpdateCanvasBackground() MOZ_OVERRIDE;

  virtual void AddCanvasBackgroundColorItem(nsDisplayListBuilder& aBuilder,
                                            nsDisplayList& aList,
                                            nsIFrame* aFrame,
                                            const nsRect& aBounds,
                                            nscolor aBackstopColor,
                                            uint32_t aFlags) MOZ_OVERRIDE;

  virtual void AddPrintPreviewBackgroundItem(nsDisplayListBuilder& aBuilder,
                                             nsDisplayList& aList,
                                             nsIFrame* aFrame,
                                             const nsRect& aBounds) MOZ_OVERRIDE;

  virtual nscolor ComputeBackstopColor(nsView* aDisplayRoot) MOZ_OVERRIDE;

  virtual NS_HIDDEN_(nsresult) SetIsActive(bool aIsActive) MOZ_OVERRIDE;

  virtual bool GetIsViewportOverridden() MOZ_OVERRIDE { return mViewportOverridden; }

  virtual bool IsLayoutFlushObserver() MOZ_OVERRIDE
  {
    return GetPresContext()->RefreshDriver()->
      IsLayoutFlushObserver(this);
  }

  void AddSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                              nsArenaMemoryStats *aArenaObjectsSize,
                              size_t *aPresShellSize,
                              size_t *aStyleSetsSize,
                              size_t *aTextRunsSize,
                              size_t *aPresContextSize) MOZ_OVERRIDE;
  size_t SizeOfTextRuns(mozilla::MallocSizeOf aMallocSizeOf) const;

  virtual void AddInvalidateHiddenPresShellObserver(nsRefreshDriver *aDriver) MOZ_OVERRIDE;

  
  
  struct ScrollIntoViewData {
    ScrollAxis mContentScrollVAxis;
    ScrollAxis mContentScrollHAxis;
    uint32_t   mContentToScrollToFlags;
  };

  virtual void ScheduleImageVisibilityUpdate() MOZ_OVERRIDE;

  virtual void RebuildImageVisibility(const nsDisplayList& aList) MOZ_OVERRIDE;

  virtual void EnsureImageInVisibleList(nsIImageLoadingContent* aImage) MOZ_OVERRIDE;

  virtual void RemoveImageFromVisibleList(nsIImageLoadingContent* aImage) MOZ_OVERRIDE;

  virtual bool AssumeAllImagesVisible() MOZ_OVERRIDE;

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

  void DispatchTouchEvent(mozilla::WidgetEvent* aEvent,
                          nsEventStatus* aStatus,
                          nsPresShellEventCB* aEventCB,
                          bool aTouchIsNew);

  void     WillDoReflow();

  






  void     DidDoReflow(bool aInterruptible, bool aWasInterrupted);
  
  
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
#ifdef DEBUG
  nsStyleSet* CloneStyleSet(nsStyleSet* aSet);
  bool VerifyIncrementalReflow();
  bool mInVerifyReflow;
  void ShowEventTargetDebug();
#endif

  void RecordStyleSheetChange(nsIStyleSheet* aStyleSheet);

    



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
  void AddAuthorSheet(nsISupports* aSheet);
  void RemoveSheet(nsStyleSet::sheetType aType, nsISupports* aSheet);

  
  void HideViewIfPopup(nsView* aView);

  
  void RestoreRootScrollPosition();

  void MaybeReleaseCapturingContent()
  {
    nsRefPtr<nsFrameSelection> frameSelection = FrameSelection();
    if (frameSelection) {
      frameSelection->SetMouseDownState(false);
    }
    if (gCaptureInfo.mContent &&
        gCaptureInfo.mContent->OwnerDoc() == mDocument) {
      SetCapturingContent(nullptr, 0);
    }
  }

  nsresult HandleRetargetedEvent(mozilla::WidgetEvent* aEvent,
                                 nsEventStatus* aStatus,
                                 nsIContent* aTarget)
  {
    PushCurrentEventInfo(nullptr, nullptr);
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
    nsDelayedInputEvent()
    : nsDelayedEvent(), mEvent(nullptr) {}

    virtual ~nsDelayedInputEvent()
    {
      delete mEvent;
    }

    mozilla::WidgetInputEvent* mEvent;
  };

  class nsDelayedMouseEvent : public nsDelayedInputEvent
  {
  public:
    nsDelayedMouseEvent(mozilla::WidgetMouseEvent* aEvent) :
      nsDelayedInputEvent()
    {
      mEvent = new mozilla::WidgetMouseEvent(aEvent->mFlags.mIsTrusted,
                                             aEvent->message,
                                             aEvent->widget,
                                             aEvent->reason,
                                             aEvent->context);
      static_cast<mozilla::WidgetMouseEvent*>(mEvent)->
        AssignMouseEventData(*aEvent, false);
    }
  };

  class nsDelayedKeyEvent : public nsDelayedInputEvent
  {
  public:
    nsDelayedKeyEvent(mozilla::WidgetKeyboardEvent* aEvent) :
      nsDelayedInputEvent()
    {
      mozilla::WidgetKeyboardEvent* keyEvent =
        new mozilla::WidgetKeyboardEvent(aEvent->mFlags.mIsTrusted,
                                         aEvent->message,
                                         aEvent->widget);
      keyEvent->AssignKeyEventData(*aEvent, false);
      mEvent = keyEvent;
    }
  };

  
  
  void RecordMouseLocation(mozilla::WidgetGUIEvent* aEvent);
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
        mPresShell = nullptr;
      }
    }
    virtual void WillRefresh(mozilla::TimeStamp aTime) MOZ_OVERRIDE {
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
  nsIContent* GetCurrentEventContent();
  nsIFrame* GetCurrentEventFrame();
  nsresult RetargetEventToParent(mozilla::WidgetGUIEvent* aEvent,
                                 nsEventStatus* aEventStatus);
  void PushCurrentEventInfo(nsIFrame* aFrame, nsIContent* aContent);
  void PopCurrentEventInfo();
  nsresult HandleEventInternal(mozilla::WidgetEvent* aEvent,
                               nsEventStatus* aStatus);
  nsresult HandlePositionedEvent(nsIFrame* aTargetFrame,
                                 mozilla::WidgetGUIEvent* aEvent,
                                 nsEventStatus* aEventStatus);
  
  
  already_AddRefed<nsPIDOMWindow> GetFocusedDOMWindowInOurWindow();

  














  bool AdjustContextMenuKeyEvent(mozilla::WidgetMouseEvent* aEvent);

  
  bool PrepareToUseCaretPosition(nsIWidget* aEventWidget, nsIntPoint& aTargetPt);

  
  
  void GetCurrentItemAndPositionForElement(nsIDOMElement *aCurrentEl,
                                           nsIContent **aTargetToUse,
                                           mozilla::LayoutDeviceIntPoint& aTargetPt,
                                           nsIWidget *aRootWidget);

  void FireResizeEvent();
  void FireBeforeResizeEvent();
  static void AsyncResizeEventCallback(nsITimer* aTimer, void* aPresShell);

  virtual void SynthesizeMouseMove(bool aFromScroll) MOZ_OVERRIDE;

  PresShell* GetRootPresShell();

  nscolor GetDefaultBackgroundColorToDraw();

  DOMHighResTimeStamp GetPerformanceNow();

  
  static void sPaintSuppressionCallback(nsITimer* aTimer, void* aPresShell);

  
  static void sReflowContinueCallback(nsITimer* aTimer, void* aPresShell);
  bool ScheduleReflowOffTimer();

  
  virtual void WindowSizeMoveDone() MOZ_OVERRIDE;
  virtual void SysColorChanged() MOZ_OVERRIDE { mPresContext->SysColorChanged(); }
  virtual void ThemeChanged() MOZ_OVERRIDE { mPresContext->ThemeChanged(); }
  virtual void BackingScaleFactorChanged() MOZ_OVERRIDE { mPresContext->UIResolutionChanged(); }

  void UpdateImageVisibility();

  nsRevocableEventPtr<nsRunnableMethod<PresShell> > mUpdateImageVisibilityEvent;

  void ClearVisibleImagesList();
  static void ClearImageVisibilityVisited(nsView* aView, bool aClear);
  static void MarkImagesInListVisible(const nsDisplayList& aList);

  
  nsTHashtable< nsRefPtrHashKey<nsIImageLoadingContent> > mVisibleImages;

#ifdef DEBUG
  
  
  nsIFrame*                 mCurrentReflowRoot;
  uint32_t                  mUpdateCount;
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
private:
  nsIFrame*                 mCurrentEventFrame;
  nsCOMPtr<nsIContent>      mCurrentEventContent;
  nsTArray<nsIFrame*>       mCurrentEventFrameStack;
  nsCOMArray<nsIContent>    mCurrentEventContentStack;
protected:
  nsRevocableEventPtr<nsSynthMouseMoveEvent> mSynthMouseMoveEvent;
  nsCOMPtr<nsIContent>      mLastAnchorScrolledTo;
  nsRefPtr<nsCaret>         mCaret;
  nsRefPtr<nsCaret>         mOriginalCaret;
  nsCallbackEventRequest*   mFirstCallbackEventRequest;
  nsCallbackEventRequest*   mLastCallbackEventRequest;

  
  
  
  nsCOMPtr<nsITimer>        mPaintSuppressionTimer;

  
  
  
  
  nsCOMPtr<nsITimer>        mReflowContinueTimer;

  
  DOMHighResTimeStamp       mLastReflowStart;

  
  
  
  
  
  nsCOMPtr<nsIContent>      mContentToScrollTo;

  nscoord                   mLastAnchorScrollPositionY;

  
  
  
  uint16_t                  mChangeNestCount;
  
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

  bool                      mImageVisibilityVisited : 1;

  static bool               sDisableNonTestMouseEvents;
};

#endif 
