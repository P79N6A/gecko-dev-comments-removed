



















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
#include "nsContentUtils.h" 
#include "nsRefreshDriver.h"
#include "TouchManager.h"
#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "mozilla/MemoryReporting.h"

class nsRange;
class nsIDragService;

struct RangePaintInfo;
struct nsCallbackEventRequest;
#ifdef MOZ_REFLOW_PERF
class ReflowCountMgr;
#endif

class nsPresShellEventCB;
class nsAutoCauseReflowNotifier;

namespace mozilla {
class CSSStyleSheet;
class EventDispatchingCallback;
} 



#define PAINTLOCK_EVENT_DELAY 250

class PresShell final : public nsIPresShell,
                        public nsStubDocumentObserver,
                        public nsISelectionController,
                        public nsIObserver,
                        public nsSupportsWeakReference
{
public:
  PresShell();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  
  NS_DECL_ISUPPORTS

  
  static bool TouchCaretPrefEnabled();

  
  static bool SelectionCaretPrefEnabled();

  
  static bool BeforeAfterKeyboardEventEnabled();

  void Init(nsIDocument* aDocument, nsPresContext* aPresContext,
            nsViewManager* aViewManager, nsStyleSet* aStyleSet,
            nsCompatibility aCompatMode);
  virtual void Destroy() override;
  virtual void MakeZombie() override;

  virtual nsresult SetPreferenceStyleRules(bool aForceReflow) override;

  NS_IMETHOD GetSelection(SelectionType aType, nsISelection** aSelection) override;
  virtual mozilla::dom::Selection* GetCurrentSelection(SelectionType aType) override;

  NS_IMETHOD SetDisplaySelection(int16_t aToggle) override;
  NS_IMETHOD GetDisplaySelection(int16_t *aToggle) override;
  NS_IMETHOD ScrollSelectionIntoView(SelectionType aType, SelectionRegion aRegion,
                                     int16_t aFlags) override;
  NS_IMETHOD RepaintSelection(SelectionType aType) override;

  virtual void BeginObservingDocument() override;
  virtual void EndObservingDocument() override;
  virtual nsresult Initialize(nscoord aWidth, nscoord aHeight) override;
  virtual nsresult ResizeReflow(nscoord aWidth, nscoord aHeight) override;
  virtual nsresult ResizeReflowOverride(nscoord aWidth, nscoord aHeight) override;
  virtual nsIPageSequenceFrame* GetPageSequenceFrame() const override;
  virtual nsCanvasFrame* GetCanvasFrame() const override;
  virtual nsIFrame* GetRealPrimaryFrameFor(nsIContent* aContent) const override;

  virtual nsIFrame* GetPlaceholderFrameFor(nsIFrame* aFrame) const override;
  virtual void FrameNeedsReflow(nsIFrame *aFrame, IntrinsicDirty aIntrinsicDirty,
                                            nsFrameState aBitToAdd) override;
  virtual void FrameNeedsToContinueReflow(nsIFrame *aFrame) override;
  virtual void CancelAllPendingReflows() override;
  virtual bool IsSafeToFlush() const override;
  virtual void FlushPendingNotifications(mozFlushType aType) override;
  virtual void FlushPendingNotifications(mozilla::ChangesToFlush aType) override;
  virtual void DestroyFramesFor(nsIContent*  aContent,
                                nsIContent** aDestroyedFramesFor) override;
  virtual void CreateFramesFor(nsIContent* aContent) override;

  


  virtual nsresult RecreateFramesFor(nsIContent* aContent) override;

  


  virtual nsresult PostReflowCallback(nsIReflowCallback* aCallback) override;
  virtual void CancelReflowCallback(nsIReflowCallback* aCallback) override;

  virtual void ClearFrameRefs(nsIFrame* aFrame) override;
  virtual already_AddRefed<gfxContext> CreateReferenceRenderingContext() override;
  virtual nsresult GoToAnchor(const nsAString& aAnchorName, bool aScroll,
                              uint32_t aAdditionalScrollFlags = 0) override;
  virtual nsresult ScrollToAnchor() override;

  virtual nsresult ScrollContentIntoView(nsIContent* aContent,
                                                     ScrollAxis  aVertical,
                                                     ScrollAxis  aHorizontal,
                                                     uint32_t    aFlags) override;
  virtual bool ScrollFrameRectIntoView(nsIFrame*     aFrame,
                                       const nsRect& aRect,
                                       ScrollAxis    aVertical,
                                       ScrollAxis    aHorizontal,
                                       uint32_t      aFlags) override;
  virtual nsRectVisibility GetRectVisibility(nsIFrame *aFrame,
                                             const nsRect &aRect,
                                             nscoord aMinTwips) const override;

  virtual void SetIgnoreFrameDestruction(bool aIgnore) override;
  virtual void NotifyDestroyingFrame(nsIFrame* aFrame) override;

  virtual nsresult CaptureHistoryState(nsILayoutHistoryState** aLayoutHistoryState) override;

  virtual void UnsuppressPainting() override;

  virtual nsresult GetAgentStyleSheets(nsCOMArray<nsIStyleSheet>& aSheets) override;
  virtual nsresult SetAgentStyleSheets(const nsCOMArray<nsIStyleSheet>& aSheets) override;

  virtual nsresult AddOverrideStyleSheet(nsIStyleSheet *aSheet) override;
  virtual nsresult RemoveOverrideStyleSheet(nsIStyleSheet *aSheet) override;

  virtual nsresult HandleEventWithTarget(
                                 mozilla::WidgetEvent* aEvent,
                                 nsIFrame* aFrame,
                                 nsIContent* aContent,
                                 nsEventStatus* aStatus) override;
  virtual nsIFrame* GetEventTargetFrame() override;
  virtual already_AddRefed<nsIContent> GetEventTargetContent(
                                                     mozilla::WidgetEvent* aEvent) override;

  virtual void NotifyCounterStylesAreDirty() override;

  virtual nsresult ReconstructFrames(void) override;
  virtual void Freeze() override;
  virtual void Thaw() override;
  virtual void FireOrClearDelayedEvents(bool aFireEvents) override;

  virtual nsresult RenderDocument(const nsRect& aRect, uint32_t aFlags,
                                              nscolor aBackgroundColor,
                                              gfxContext* aThebesContext) override;

  virtual mozilla::TemporaryRef<SourceSurface>
  RenderNode(nsIDOMNode* aNode,
             nsIntRegion* aRegion,
             nsIntPoint& aPoint,
             nsIntRect* aScreenRect) override;

  virtual mozilla::TemporaryRef<SourceSurface>
  RenderSelection(nsISelection* aSelection,
                  nsIntPoint& aPoint,
                  nsIntRect* aScreenRect) override;

  virtual already_AddRefed<nsPIDOMWindow> GetRootWindow() override;

  virtual LayerManager* GetLayerManager() override;

  virtual void SetIgnoreViewportScrolling(bool aIgnore) override;

  virtual nsresult SetResolution(float aResolution) override {
    return SetResolutionImpl(aResolution,  false);
  }
  virtual nsresult SetResolutionAndScaleTo(float aResolution) override {
    return SetResolutionImpl(aResolution,  true);
  }
  virtual bool ScaleToResolution() const override;
  virtual float GetCumulativeResolution() override;

  

  virtual void Paint(nsView* aViewToPaint, const nsRegion& aDirtyRegion,
                     uint32_t aFlags) override;
  virtual nsresult HandleEvent(nsIFrame* aFrame,
                               mozilla::WidgetGUIEvent* aEvent,
                               bool aDontRetargetEvents,
                               nsEventStatus* aEventStatus,
                               nsIContent** aTargetContent) override;
  virtual nsresult HandleDOMEventWithTarget(
                                 nsIContent* aTargetContent,
                                 mozilla::WidgetEvent* aEvent,
                                 nsEventStatus* aStatus) override;
  virtual nsresult HandleDOMEventWithTarget(nsIContent* aTargetContent,
                                                        nsIDOMEvent* aEvent,
                                                        nsEventStatus* aStatus) override;
  virtual bool ShouldIgnoreInvalidation() override;
  virtual void WillPaint() override;
  virtual void WillPaintWindow() override;
  virtual void DidPaintWindow() override;
  virtual void ScheduleViewManagerFlush(PaintType aType = PAINT_DEFAULT) override;
  virtual void DispatchSynthMouseMove(mozilla::WidgetGUIEvent* aEvent,
                                      bool aFlushOnHoverChange) override;
  virtual void ClearMouseCaptureOnView(nsView* aView) override;
  virtual bool IsVisible() override;

  
  virtual already_AddRefed<mozilla::TouchCaret> GetTouchCaret() const override;
  virtual mozilla::dom::Element* GetTouchCaretElement() const override;
  
  virtual already_AddRefed<mozilla::SelectionCarets> GetSelectionCarets() const override;
  virtual mozilla::dom::Element* GetSelectionCaretsStartElement() const override;
  virtual mozilla::dom::Element* GetSelectionCaretsEndElement() const override;
  
  virtual already_AddRefed<nsCaret> GetCaret() const override;
  NS_IMETHOD SetCaretEnabled(bool aInEnable) override;
  NS_IMETHOD SetCaretReadOnly(bool aReadOnly) override;
  NS_IMETHOD GetCaretEnabled(bool *aOutEnabled) override;
  NS_IMETHOD SetCaretVisibilityDuringSelection(bool aVisibility) override;
  NS_IMETHOD GetCaretVisible(bool *_retval) override;
  virtual void SetCaret(nsCaret *aNewCaret) override;
  virtual void RestoreCaret() override;

  NS_IMETHOD SetSelectionFlags(int16_t aInEnable) override;
  NS_IMETHOD GetSelectionFlags(int16_t *aOutEnable) override;

  

  NS_IMETHOD PhysicalMove(int16_t aDirection, int16_t aAmount, bool aExtend) override;
  NS_IMETHOD CharacterMove(bool aForward, bool aExtend) override;
  NS_IMETHOD CharacterExtendForDelete() override;
  NS_IMETHOD CharacterExtendForBackspace() override;
  NS_IMETHOD WordMove(bool aForward, bool aExtend) override;
  NS_IMETHOD WordExtendForDelete(bool aForward) override;
  NS_IMETHOD LineMove(bool aForward, bool aExtend) override;
  NS_IMETHOD IntraLineMove(bool aForward, bool aExtend) override;
  NS_IMETHOD PageMove(bool aForward, bool aExtend) override;
  NS_IMETHOD ScrollPage(bool aForward) override;
  NS_IMETHOD ScrollLine(bool aForward) override;
  NS_IMETHOD ScrollCharacter(bool aRight) override;
  NS_IMETHOD CompleteScroll(bool aForward) override;
  NS_IMETHOD CompleteMove(bool aForward, bool aExtend) override;
  NS_IMETHOD SelectAll() override;
  NS_IMETHOD CheckVisibility(nsIDOMNode *node, int16_t startOffset, int16_t EndOffset, bool *_retval) override;
  virtual nsresult CheckVisibilityContent(nsIContent* aNode, int16_t aStartOffset,
                                          int16_t aEndOffset, bool* aRetval) override;

  
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
  virtual void DumpReflows() override;
  virtual void CountReflows(const char * aName, nsIFrame * aFrame) override;
  virtual void PaintCount(const char * aName,
                                      nsRenderingContext* aRenderingContext,
                                      nsPresContext* aPresContext,
                                      nsIFrame * aFrame,
                                      const nsPoint& aOffset,
                                      uint32_t aColor) override;
  virtual void SetPaintFrameCount(bool aOn) override;
  virtual bool IsPaintingFrameCounts() override;
#endif

#ifdef DEBUG
  virtual void ListStyleContexts(nsIFrame *aRootFrame, FILE *out,
                                 int32_t aIndent = 0) override;

  virtual void ListStyleSheets(FILE *out, int32_t aIndent = 0) override;
  virtual void VerifyStyleTree() override;
#endif

#ifdef PR_LOGGING
  static PRLogModuleInfo* gLog;
#endif

  virtual void DisableNonTestMouseEvents(bool aDisable) override;

  virtual void UpdateCanvasBackground() override;

  virtual void AddCanvasBackgroundColorItem(nsDisplayListBuilder& aBuilder,
                                            nsDisplayList& aList,
                                            nsIFrame* aFrame,
                                            const nsRect& aBounds,
                                            nscolor aBackstopColor,
                                            uint32_t aFlags) override;

  virtual void AddPrintPreviewBackgroundItem(nsDisplayListBuilder& aBuilder,
                                             nsDisplayList& aList,
                                             nsIFrame* aFrame,
                                             const nsRect& aBounds) override;

  virtual nscolor ComputeBackstopColor(nsView* aDisplayRoot) override;

  virtual nsresult SetIsActive(bool aIsActive) override;

  virtual bool GetIsViewportOverridden() override { return mViewportOverridden; }

  virtual bool IsLayoutFlushObserver() override
  {
    return GetPresContext()->RefreshDriver()->
      IsLayoutFlushObserver(this);
  }

  virtual void LoadComplete() override;

  void AddSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                              nsArenaMemoryStats *aArenaObjectsSize,
                              size_t *aPresShellSize,
                              size_t *aStyleSetsSize,
                              size_t *aTextRunsSize,
                              size_t *aPresContextSize) override;
  size_t SizeOfTextRuns(mozilla::MallocSizeOf aMallocSizeOf) const;

  virtual void AddInvalidateHiddenPresShellObserver(nsRefreshDriver *aDriver) override;

  
  
  struct ScrollIntoViewData {
    ScrollAxis mContentScrollVAxis;
    ScrollAxis mContentScrollHAxis;
    uint32_t   mContentToScrollToFlags;
  };

  virtual void ScheduleImageVisibilityUpdate() override;

  virtual void RebuildImageVisibilityDisplayList(const nsDisplayList& aList) override;
  virtual void RebuildImageVisibility(nsRect* aRect = nullptr) override;

  virtual void EnsureImageInVisibleList(nsIImageLoadingContent* aImage) override;

  virtual void RemoveImageFromVisibleList(nsIImageLoadingContent* aImage) override;

  virtual bool AssumeAllImagesVisible() override;

  virtual void RecordShadowStyleChange(mozilla::dom::ShadowRoot* aShadowRoot) override;

  virtual void DispatchAfterKeyboardEvent(nsINode* aTarget,
                                          const mozilla::WidgetKeyboardEvent& aEvent,
                                          bool aEmbeddedCancelled) override;

  void SetNextPaintCompressed() { mNextPaintCompressed = true; }

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

  nsresult DispatchEventToDOM(mozilla::WidgetEvent* aEvent,
                              nsEventStatus* aStatus,
                              nsPresShellEventCB* aEventCB);
  void DispatchTouchEventToDOM(mozilla::WidgetEvent* aEvent,
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
    explicit RenderingState(PresShell* aPresShell)
      : mResolution(aPresShell->mResolution)
      , mRenderFlags(aPresShell->mRenderFlags)
    { }
    float mResolution;
    RenderFlags mRenderFlags;
  };

  struct AutoSaveRestoreRenderingState {
    explicit AutoSaveRestoreRenderingState(PresShell* aPresShell)
      : mPresShell(aPresShell)
      , mOldState(aPresShell)
    {}

    ~AutoSaveRestoreRenderingState()
    {
      mPresShell->mRenderFlags = mOldState.mRenderFlags;
      mPresShell->mResolution = mOldState.mResolution;
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

  









  mozilla::TemporaryRef<SourceSurface>
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

  void MaybeReleaseCapturingContent();

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

  class DelayedEvent
  {
  public:
    virtual ~DelayedEvent() { }
    virtual void Dispatch() { }
  };

  class DelayedInputEvent : public DelayedEvent
  {
  public:
    virtual void Dispatch() override;

  protected:
    DelayedInputEvent();
    virtual ~DelayedInputEvent();

    mozilla::WidgetInputEvent* mEvent;
  };

  class DelayedMouseEvent : public DelayedInputEvent
  {
  public:
    explicit DelayedMouseEvent(mozilla::WidgetMouseEvent* aEvent);
  };

  class DelayedKeyEvent : public DelayedInputEvent
  {
  public:
    explicit DelayedKeyEvent(mozilla::WidgetKeyboardEvent* aEvent);
  };

  
  
  void RecordMouseLocation(mozilla::WidgetGUIEvent* aEvent);
  class nsSynthMouseMoveEvent final : public nsARefreshObserver {
  public:
    nsSynthMouseMoveEvent(PresShell* aPresShell, bool aFromScroll)
      : mPresShell(aPresShell), mFromScroll(aFromScroll) {
      NS_ASSERTION(mPresShell, "null parameter");
    }

  private:
  
    ~nsSynthMouseMoveEvent() {
      Revoke();
    }

  public:
    NS_INLINE_DECL_REFCOUNTING(nsSynthMouseMoveEvent, override)

    void Revoke() {
      if (mPresShell) {
        mPresShell->GetPresContext()->RefreshDriver()->
          RemoveRefreshObserver(this, Flush_Display);
        mPresShell = nullptr;
      }
    }
    virtual void WillRefresh(mozilla::TimeStamp aTime) override {
      if (mPresShell) {
        nsRefPtr<PresShell> shell = mPresShell;
        shell->ProcessSynthMouseMoveEvent(mFromScroll);
      }
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
  already_AddRefed<nsIPresShell> GetParentPresShellForEventHandling();
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

  
  bool PrepareToUseCaretPosition(nsIWidget* aEventWidget,
                                 mozilla::LayoutDeviceIntPoint& aTargetPt);

  
  
  void GetCurrentItemAndPositionForElement(nsIDOMElement *aCurrentEl,
                                           nsIContent **aTargetToUse,
                                           mozilla::LayoutDeviceIntPoint& aTargetPt,
                                           nsIWidget *aRootWidget);

  void FireResizeEvent();
  static void AsyncResizeEventCallback(nsITimer* aTimer, void* aPresShell);

  virtual void SynthesizeMouseMove(bool aFromScroll) override;

  PresShell* GetRootPresShell();

  nscolor GetDefaultBackgroundColorToDraw();

  DOMHighResTimeStamp GetPerformanceNow();

  
  static void sPaintSuppressionCallback(nsITimer* aTimer, void* aPresShell);

  
  static void sReflowContinueCallback(nsITimer* aTimer, void* aPresShell);
  bool ScheduleReflowOffTimer();

  
  virtual void WindowSizeMoveDone() override;
  virtual void SysColorChanged() override { mPresContext->SysColorChanged(); }
  virtual void ThemeChanged() override { mPresContext->ThemeChanged(); }
  virtual void BackingScaleFactorChanged() override { mPresContext->UIResolutionChanged(); }

  virtual void PausePainting() override;
  virtual void ResumePainting() override;

  void UpdateImageVisibility();
  void UpdateActivePointerState(mozilla::WidgetGUIEvent* aEvent);

  nsRevocableEventPtr<nsRunnableMethod<PresShell> > mUpdateImageVisibilityEvent;

  void ClearVisibleImagesList(uint32_t aNonvisibleAction);
  static void ClearImageVisibilityVisited(nsView* aView, bool aClear);
  static void MarkImagesInListVisible(const nsDisplayList& aList);
  void MarkImagesInSubtreeVisible(nsIFrame* aFrame, const nsRect& aRect);

  
  void HandleKeyboardEvent(nsINode* aTarget,
                           mozilla::WidgetKeyboardEvent& aEvent,
                           bool aEmbeddedCancelled,
                           nsEventStatus* aStatus,
                           mozilla::EventDispatchingCallback* aEventCB);
  void DispatchBeforeKeyboardEventInternal(
         const nsTArray<nsCOMPtr<mozilla::dom::Element> >& aChain,
         const mozilla::WidgetKeyboardEvent& aEvent,
         size_t& aChainIndex,
         bool& aDefaultPrevented);
  void DispatchAfterKeyboardEventInternal(
         const nsTArray<nsCOMPtr<mozilla::dom::Element> >& aChain,
         const mozilla::WidgetKeyboardEvent& aEvent,
         bool aEmbeddedCancelled,
         size_t aChainIndex = 0);
  bool CanDispatchEvent(const mozilla::WidgetGUIEvent* aEvent = nullptr) const;

  
  nsTHashtable< nsRefPtrHashKey<nsIImageLoadingContent> > mVisibleImages;

  nsresult SetResolutionImpl(float aResolution, bool aScaleToResolution);

#ifdef DEBUG
  
  
  nsIFrame*                 mCurrentReflowRoot;
  uint32_t                  mUpdateCount;
#endif

#ifdef MOZ_REFLOW_PERF
  ReflowCountMgr*           mReflowCountMgr;
#endif

  
  
  
  
  
  
  
  
  nsPoint                   mMouseLocation;

  
  nsRefPtr<mozilla::CSSStyleSheet> mPrefStyleSheet;

  
  
  nsTHashtable<nsPtrHashKey<nsIFrame> > mFramesToDirty;

  
  nsTArray<nsIFrame*>       mDirtyRoots;

  nsTArray<nsAutoPtr<DelayedEvent> > mDelayedEvents;
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

  
  TouchManager              mTouchManager;

  
  nsRefPtr<mozilla::TouchCaret> mTouchCaret;
  nsRefPtr<mozilla::SelectionCarets> mSelectionCarets;

  
  
  
  nsCOMPtr<nsITimer>        mPaintSuppressionTimer;

  nsCOMPtr<nsITimer>        mDelayedPaintTimer;

  
  DOMHighResTimeStamp       mLastReflowStart;

  mozilla::TimeStamp        mLoadBegin;  

  
  
  
  
  
  nsCOMPtr<nsIContent>      mContentToScrollTo;

  nscoord                   mLastAnchorScrollPositionY;

  
  
  
  nsCOMPtr<nsIContent>      mPointerEventTarget;

  
  
  
  uint16_t                  mChangeNestCount;

  bool                      mDocumentLoading : 1;
  bool                      mIgnoreFrameDestruction : 1;
  bool                      mHaveShutDown : 1;
  bool                      mViewportOverridden : 1;
  bool                      mLastRootReflowHadUnconstrainedBSize : 1;
  bool                      mNoDelayedMouseEvents : 1;
  bool                      mNoDelayedKeyEvents : 1;

  
  
  bool                      mIsDocumentGone : 1;

  
  
  bool                      mShouldUnsuppressPainting : 1;

  bool                      mAsyncResizeTimerIsActive : 1;
  bool                      mInResize : 1;

  bool                      mImageVisibilityVisited : 1;

  bool                      mNextPaintCompressed : 1;

  bool                      mHasCSSBackgroundColor : 1;

  
  
  
  bool                      mScaleToResolution : 1;

  
  bool                      mIsLastChromeOnlyEscapeKeyConsumed : 1;

  static bool               sDisableNonTestMouseEvents;
};

#endif 
