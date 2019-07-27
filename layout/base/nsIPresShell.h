


















#ifndef nsIPresShell_h___
#define nsIPresShell_h___

#include "mozilla/EventForwards.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/WeakPtr.h"
#include "gfxPoint.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsISupports.h"
#include "nsIContent.h"
#include "nsQueryFrame.h"
#include "nsCoord.h"
#include "nsColor.h"
#include "nsCompatibility.h"
#include "nsFrameManagerBase.h"
#include "mozFlushType.h"
#include "nsWeakReference.h"
#include <stdio.h> 
#include "nsChangeHint.h"
#include "nsRefPtrHashtable.h"
#include "nsClassHashtable.h"
#include "nsPresArena.h"
#include "nsIImageLoadingContent.h"
#include "nsMargin.h"
#include "nsFrameState.h"

class nsDocShell;
class nsIDocument;
class nsIFrame;
class nsPresContext;
class nsStyleSet;
class nsViewManager;
class nsView;
class nsRenderingContext;
class nsIPageSequenceFrame;
class nsCanvasFrame;
class nsAString;
class nsCaret;
namespace mozilla {
class TouchCaret;
class SelectionCarets;
} 
class nsFrameSelection;
class nsFrameManager;
class nsILayoutHistoryState;
class nsIReflowCallback;
class nsIDOMNode;
class nsIntRegion;
class nsIStyleSheet;
class nsCSSFrameConstructor;
class nsISelection;
template<class E> class nsCOMArray;
class nsWeakFrame;
class nsIScrollableFrame;
class gfxContext;
class nsIDOMEvent;
class nsDisplayList;
class nsDisplayListBuilder;
class nsPIDOMWindow;
struct nsPoint;
class nsINode;
struct nsIntPoint;
struct nsIntRect;
struct nsRect;
class nsRegion;
class nsRefreshDriver;
class nsARefreshObserver;
class nsAPostRefreshObserver;
#ifdef ACCESSIBILITY
class nsAccessibilityService;
namespace mozilla {
namespace a11y {
class DocAccessible;
} 
} 
#endif
class nsIWidget;
struct nsArenaMemoryStats;
class nsITimer;

typedef short SelectionType;

namespace mozilla {
class EventStates;

namespace dom {
class Element;
class Touch;
class Selection;
class ShadowRoot;
} 

namespace layers {
class LayerManager;
} 

namespace gfx {
class SourceSurface;
} 
} 




#define CAPTURE_IGNOREALLOWED 1

#define CAPTURE_RETARGETTOELEMENT 2

#define CAPTURE_PREVENTDRAG 4

#define CAPTURE_POINTERLOCK 8

typedef struct CapturingContentInfo {
  
  bool mAllowed;
  bool mPointerLock;
  bool mRetargetToElement;
  bool mPreventDrag;
  nsIContent* mContent;
} CapturingContentInfo;


#define NS_IPRESSHELL_IID \
  { 0x9d010f90, 0x2d90, 0x471c, \
    { 0xb6, 0x40, 0x03, 0x8c, 0xc3, 0x50, 0xc1, 0x87 } }


#define VERIFY_REFLOW_ON                    0x01
#define VERIFY_REFLOW_NOISY                 0x02
#define VERIFY_REFLOW_ALL                   0x04
#define VERIFY_REFLOW_DUMP_COMMANDS         0x08
#define VERIFY_REFLOW_NOISY_RC              0x10
#define VERIFY_REFLOW_REALLY_NOISY_RC       0x20
#define VERIFY_REFLOW_DURING_RESIZE_REFLOW  0x40

#undef NOISY_INTERRUPTIBLE_REFLOW

enum nsRectVisibility {
  nsRectVisibility_kVisible,
  nsRectVisibility_kAboveViewport,
  nsRectVisibility_kBelowViewport,
  nsRectVisibility_kLeftOfViewport,
  nsRectVisibility_kRightOfViewport
};













class nsIPresShell : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPRESSHELL_IID)

protected:
  typedef mozilla::layers::LayerManager LayerManager;
  typedef mozilla::gfx::SourceSurface SourceSurface;

  enum eRenderFlag {
    STATE_IGNORING_VIEWPORT_SCROLLING = 0x1,
    STATE_DRAWWINDOW_NOT_FLUSHING = 0x2
  };
  typedef uint8_t RenderFlags; 

public:
  






  virtual void Destroy() = 0;

  bool IsDestroying() { return mIsDestroying; }

  









  virtual void MakeZombie() = 0;

  






  void* AllocateFrame(nsQueryFrame::FrameIID aID, size_t aSize)
  {
#ifdef DEBUG
    mPresArenaAllocCount++;
#endif
    void* result = mFrameArena.AllocateByFrameID(aID, aSize);
    memset(result, 0, aSize);
    return result;
  }

  void FreeFrame(nsQueryFrame::FrameIID aID, void* aPtr)
  {
#ifdef DEBUG
    mPresArenaAllocCount--;
#endif
    if (!mIsDestroying)
      mFrameArena.FreeByFrameID(aID, aPtr);
  }

  





  void* AllocateByObjectID(nsPresArena::ObjectID aID, size_t aSize)
  {
#ifdef DEBUG
    mPresArenaAllocCount++;
#endif
    void* result = mFrameArena.AllocateByObjectID(aID, aSize);
    memset(result, 0, aSize);
    return result;
  }

  void FreeByObjectID(nsPresArena::ObjectID aID, void* aPtr)
  {
#ifdef DEBUG
    mPresArenaAllocCount--;
#endif
    if (!mIsDestroying)
      mFrameArena.FreeByObjectID(aID, aPtr);
  }

  








  void* AllocateMisc(size_t aSize)
  {
#ifdef DEBUG
    mPresArenaAllocCount++;
#endif
    return mFrameArena.AllocateBySize(aSize);
  }

  void FreeMisc(size_t aSize, void* aPtr)
  {
#ifdef DEBUG
    mPresArenaAllocCount--;
#endif
    if (!mIsDestroying)
      mFrameArena.FreeBySize(aSize, aPtr);
  }

  nsIDocument* GetDocument() const { return mDocument; }

  nsPresContext* GetPresContext() const { return mPresContext; }

  nsViewManager* GetViewManager() const { return mViewManager; }

#ifdef ACCESSIBILITY
  


  mozilla::a11y::DocAccessible* GetDocAccessible() const
  {
    return mDocAccessible;
  }

  


  void SetDocAccessible(mozilla::a11y::DocAccessible* aDocAccessible)
  {
    mDocAccessible = aDocAccessible;
  }
#endif

#ifdef MOZILLA_INTERNAL_API
  nsStyleSet* StyleSet() const { return mStyleSet; }

  nsCSSFrameConstructor* FrameConstructor() const { return mFrameConstructor; }

  nsFrameManager* FrameManager() const {
    
    
    return reinterpret_cast<nsFrameManager*>
                           (const_cast<nsIPresShell*>(this)->mFrameManager);
  }

#endif

  


  
  
  void SetAuthorStyleDisabled(bool aDisabled);
  bool GetAuthorStyleDisabled() const;

  









  virtual void ReconstructStyleDataExternal();
  void ReconstructStyleDataInternal();
#ifdef MOZILLA_INTERNAL_API
  void ReconstructStyleData() { ReconstructStyleDataInternal(); }
#else
  void ReconstructStyleData() { ReconstructStyleDataExternal(); }
#endif

  







  virtual nsresult SetPreferenceStyleRules(bool aForceReflow) = 0;

  




  already_AddRefed<nsFrameSelection> FrameSelection();

  



  const nsFrameSelection* ConstFrameSelection() const { return mSelection; }

  
  
  virtual void BeginObservingDocument() = 0;

  
  virtual void EndObservingDocument() = 0;

  


  bool DidInitialize() const { return mDidInitialize; }

  










  virtual nsresult Initialize(nscoord aWidth, nscoord aHeight) = 0;

  



  virtual nsresult ResizeReflow(nscoord aWidth, nscoord aHeight) = 0;
  




  virtual nsresult ResizeReflowOverride(nscoord aWidth, nscoord aHeight) = 0;

  


  virtual bool GetIsViewportOverridden() = 0;

  


  virtual bool IsLayoutFlushObserver() = 0;

  


  virtual void LoadComplete() = 0;

  


  virtual nsIFrame* GetRootFrameExternal() const;
  nsIFrame* GetRootFrame() const {
#ifdef MOZILLA_INTERNAL_API
    return mFrameManager->GetRootFrame();
#else
    return GetRootFrameExternal();
#endif
  }

  


  nsIFrame* GetRootScrollFrame() const;

  


  nsIScrollableFrame* GetRootScrollFrameAsScrollable() const;

  



  virtual nsIScrollableFrame* GetRootScrollFrameAsScrollableExternal() const;

  






  enum ScrollDirection { eHorizontal, eVertical, eEither };
  nsIScrollableFrame* GetFrameToScrollAsScrollable(ScrollDirection aDirection);

  



  virtual nsIPageSequenceFrame* GetPageSequenceFrame() const = 0;

  



  virtual nsCanvasFrame* GetCanvasFrame() const = 0;

  






  virtual nsIFrame* GetRealPrimaryFrameFor(nsIContent* aContent) const = 0;

  



  virtual nsIFrame* GetPlaceholderFrameFor(nsIFrame* aFrame) const = 0;

  










  enum IntrinsicDirty {
    
    eResize,     
    eTreeChange, 
    eStyleChange 
  };
  virtual void FrameNeedsReflow(nsIFrame *aFrame,
                                IntrinsicDirty aIntrinsicDirty,
                                nsFrameState aBitToAdd) = 0;

  


  virtual void MarkFixedFramesForReflow(IntrinsicDirty aIntrinsicDirty);

  










  virtual void FrameNeedsToContinueReflow(nsIFrame *aFrame) = 0;

  virtual void CancelAllPendingReflows() = 0;

  virtual void NotifyCounterStylesAreDirty() = 0;

  






  virtual void DestroyFramesFor(nsIContent*  aContent,
                                nsIContent** aDestroyedFramesFor) = 0;
  




  virtual void CreateFramesFor(nsIContent* aContent) = 0;

  


  virtual nsresult RecreateFramesFor(nsIContent* aContent) = 0;

  void PostRecreateFramesFor(mozilla::dom::Element* aElement);
  void RestyleForAnimation(mozilla::dom::Element* aElement,
                           nsRestyleHint aHint);

  
  
  
  virtual void RecordShadowStyleChange(mozilla::dom::ShadowRoot* aShadowRoot) = 0;

  




  virtual bool IsSafeToFlush() const = 0;

  








  virtual void FlushPendingNotifications(mozFlushType aType) = 0;
  virtual void FlushPendingNotifications(mozilla::ChangesToFlush aType) = 0;

  



  virtual nsresult PostReflowCallback(nsIReflowCallback* aCallback) = 0;
  virtual void CancelReflowCallback(nsIReflowCallback* aCallback) = 0;

  virtual void ClearFrameRefs(nsIFrame* aFrame) = 0;

  




  virtual already_AddRefed<gfxContext> CreateReferenceRenderingContext() = 0;

  








  virtual nsresult GoToAnchor(const nsAString& aAnchorName, bool aScroll,
                              uint32_t aAdditionalScrollFlags = 0) = 0;

  







  virtual nsresult ScrollToAnchor() = 0;

  enum {
    SCROLL_TOP     = 0,
    SCROLL_BOTTOM  = 100,
    SCROLL_LEFT    = 0,
    SCROLL_RIGHT   = 100,
    SCROLL_CENTER  = 50,
    SCROLL_MINIMUM = -1
  };

  enum WhenToScroll {
    SCROLL_ALWAYS,
    SCROLL_IF_NOT_VISIBLE,
    SCROLL_IF_NOT_FULLY_VISIBLE
  };
  typedef struct ScrollAxis {
    int16_t mWhereToScroll;
    WhenToScroll mWhenToScroll : 8;
    bool mOnlyIfPerceivedScrollableDirection : 1;
  


































    explicit ScrollAxis(int16_t aWhere = SCROLL_MINIMUM,
                        WhenToScroll aWhen = SCROLL_IF_NOT_FULLY_VISIBLE,
                        bool aOnlyIfPerceivedScrollableDirection = false) :
      mWhereToScroll(aWhere), mWhenToScroll(aWhen),
      mOnlyIfPerceivedScrollableDirection(aOnlyIfPerceivedScrollableDirection)
    {}
  } ScrollAxis;
  






























  virtual nsresult ScrollContentIntoView(nsIContent* aContent,
                                                     ScrollAxis  aVertical,
                                                     ScrollAxis  aHorizontal,
                                                     uint32_t    aFlags) = 0;

  enum {
    SCROLL_FIRST_ANCESTOR_ONLY = 0x01,
    SCROLL_OVERFLOW_HIDDEN = 0x02,
    SCROLL_NO_PARENT_FRAMES = 0x04,
    SCROLL_SMOOTH = 0x08,
    SCROLL_SMOOTH_AUTO = 0x10
  };
  


















  virtual bool ScrollFrameRectIntoView(nsIFrame*     aFrame,
                                       const nsRect& aRect,
                                       ScrollAxis    aVertical,
                                       ScrollAxis    aHorizontal,
                                       uint32_t      aFlags) = 0;

  













  virtual nsRectVisibility GetRectVisibility(nsIFrame *aFrame,
                                             const nsRect &aRect,
                                             nscoord aMinTwips) const = 0;

  



  virtual void SetIgnoreFrameDestruction(bool aIgnore) = 0;

  




  virtual void NotifyDestroyingFrame(nsIFrame* aFrame) = 0;

  


  virtual already_AddRefed<mozilla::TouchCaret> GetTouchCaret() const = 0;

  


  virtual mozilla::dom::Element* GetTouchCaretElement() const = 0;

  


  virtual already_AddRefed<mozilla::SelectionCarets> GetSelectionCarets() const = 0;

  


  virtual mozilla::dom::Element* GetSelectionCaretsStartElement() const = 0;

  


  virtual mozilla::dom::Element* GetSelectionCaretsEndElement() const = 0;

  


  virtual already_AddRefed<nsCaret> GetCaret() const = 0;

  


  virtual void SetCaret(nsCaret *aNewCaret) = 0;

  



  virtual void RestoreCaret() = 0;

  







  NS_IMETHOD SetSelectionFlags(int16_t aInEnable) = 0;

  




  int16_t GetSelectionFlags() const { return mSelectionFlags; }

  virtual mozilla::dom::Selection* GetCurrentSelection(SelectionType aType) = 0;

  



  virtual nsresult HandleEventWithTarget(
                                 mozilla::WidgetEvent* aEvent,
                                 nsIFrame* aFrame,
                                 nsIContent* aContent,
                                 nsEventStatus* aStatus) = 0;

  



  virtual nsresult HandleDOMEventWithTarget(
                                 nsIContent* aTargetContent,
                                 mozilla::WidgetEvent* aEvent,
                                 nsEventStatus* aStatus) = 0;

  



  virtual nsresult HandleDOMEventWithTarget(nsIContent* aTargetContent,
                                                        nsIDOMEvent* aEvent,
                                                        nsEventStatus* aStatus) = 0;

  


  virtual void DispatchAfterKeyboardEvent(nsINode* aTarget,
                                          const mozilla::WidgetKeyboardEvent& aEvent,
                                          bool aEmbeddedCancelled) = 0;

  


  virtual nsIFrame* GetEventTargetFrame() = 0;

  


  virtual already_AddRefed<nsIContent> GetEventTargetContent(
                                                     mozilla::WidgetEvent* aEvent) = 0;

  



  virtual nsresult CaptureHistoryState(nsILayoutHistoryState** aLayoutHistoryState) = 0;

  



  bool IsReflowLocked() const { return mIsReflowing; }

  




  bool IsPaintingSuppressed() const { return mPaintingSuppressed; }

  




  virtual void PausePainting() = 0;

  




  virtual void ResumePainting() = 0;

  


  virtual void UnsuppressPainting() = 0;

  


  void DisableThemeSupport()
  {
    
    mIsThemeSupportDisabled = true;
  }

  


  bool IsThemeSupportEnabled() const { return !mIsThemeSupportDisabled; }

  


  virtual nsresult GetAgentStyleSheets(nsCOMArray<nsIStyleSheet>& aSheets) = 0;

  


  virtual nsresult SetAgentStyleSheets(const nsCOMArray<nsIStyleSheet>& aSheets) = 0;

  


  virtual nsresult AddOverrideStyleSheet(nsIStyleSheet *aSheet) = 0;

  


  virtual nsresult RemoveOverrideStyleSheet(nsIStyleSheet *aSheet) = 0;

  


  virtual nsresult ReconstructFrames() = 0;

  


  virtual void ContentStateChanged(nsIDocument* aDocument,
                                   nsIContent* aContent,
                                   mozilla::EventStates aStateMask) = 0;

  





  static bool GetVerifyReflowEnable();

  


  static void SetVerifyReflowEnable(bool aEnabled);

  virtual nsIFrame* GetAbsoluteContainingBlock(nsIFrame* aFrame);

#ifdef MOZ_REFLOW_PERF
  virtual void DumpReflows() = 0;
  virtual void CountReflows(const char * aName, nsIFrame * aFrame) = 0;
  virtual void PaintCount(const char * aName,
                                      nsRenderingContext* aRenderingContext,
                                      nsPresContext * aPresContext,
                                      nsIFrame * aFrame,
                                      const nsPoint& aOffset,
                                      uint32_t aColor) = 0;
  virtual void SetPaintFrameCount(bool aOn) = 0;
  virtual bool IsPaintingFrameCounts() = 0;
#endif

#ifdef DEBUG
  
  virtual void ListStyleContexts(nsIFrame *aRootFrame, FILE *out,
                                 int32_t aIndent = 0) = 0;

  virtual void ListStyleSheets(FILE *out, int32_t aIndent = 0) = 0;
  virtual void VerifyStyleTree() = 0;
#endif

#ifdef ACCESSIBILITY
  


  static bool IsAccessibilityActive();

  


  static nsAccessibilityService* AccService();
#endif

  




  virtual void Freeze() = 0;
  bool IsFrozen() { return mFrozen; }

  



  virtual void Thaw() = 0;

  virtual void FireOrClearDelayedEvents(bool aFireEvents) = 0;

  





  void SetForwardingContainer(const mozilla::WeakPtr<nsDocShell> &aContainer);

  


































  enum {
    RENDER_IS_UNTRUSTED = 0x01,
    RENDER_IGNORE_VIEWPORT_SCROLLING = 0x02,
    RENDER_CARET = 0x04,
    RENDER_USE_WIDGET_LAYERS = 0x08,
    RENDER_ASYNC_DECODE_IMAGES = 0x10,
    RENDER_DOCUMENT_RELATIVE = 0x20,
    RENDER_DRAWWINDOW_NOT_FLUSHING = 0x40
  };
  virtual nsresult RenderDocument(const nsRect& aRect, uint32_t aFlags,
                                  nscolor aBackgroundColor,
                                  gfxContext* aRenderedContext) = 0;

  





  virtual mozilla::TemporaryRef<SourceSurface>
  RenderNode(nsIDOMNode* aNode,
             nsIntRegion* aRegion,
             nsIntPoint& aPoint,
             nsIntRect* aScreenRect) = 0;

  














  virtual mozilla::TemporaryRef<SourceSurface>
  RenderSelection(nsISelection* aSelection,
                  nsIntPoint& aPoint,
                  nsIntRect* aScreenRect) = 0;

  void AddWeakFrameInternal(nsWeakFrame* aWeakFrame);
  virtual void AddWeakFrameExternal(nsWeakFrame* aWeakFrame);

  void AddWeakFrame(nsWeakFrame* aWeakFrame)
  {
#ifdef MOZILLA_INTERNAL_API
    AddWeakFrameInternal(aWeakFrame);
#else
    AddWeakFrameExternal(aWeakFrame);
#endif
  }

  void RemoveWeakFrameInternal(nsWeakFrame* aWeakFrame);
  virtual void RemoveWeakFrameExternal(nsWeakFrame* aWeakFrame);

  void RemoveWeakFrame(nsWeakFrame* aWeakFrame)
  {
#ifdef MOZILLA_INTERNAL_API
    RemoveWeakFrameInternal(aWeakFrame);
#else
    RemoveWeakFrameExternal(aWeakFrame);
#endif
  }

#ifdef DEBUG
  nsIFrame* GetDrawEventTargetFrame() { return mDrawEventTargetFrame; }
#endif

  






  virtual void DisableNonTestMouseEvents(bool aDisable) = 0;

  






  void SetCanvasBackground(nscolor aColor) { mCanvasBackgroundColor = aColor; }
  nscolor GetCanvasBackground() { return mCanvasBackgroundColor; }

  



  virtual void UpdateCanvasBackground() = 0;

  






  enum {
    FORCE_DRAW = 0x01
  };
  virtual void AddCanvasBackgroundColorItem(nsDisplayListBuilder& aBuilder,
                                            nsDisplayList& aList,
                                            nsIFrame* aFrame,
                                            const nsRect& aBounds,
                                            nscolor aBackstopColor = NS_RGBA(0,0,0,0),
                                            uint32_t aFlags = 0) = 0;


  




  virtual void AddPrintPreviewBackgroundItem(nsDisplayListBuilder& aBuilder,
                                             nsDisplayList& aList,
                                             nsIFrame* aFrame,
                                             const nsRect& aBounds) = 0;

  




  virtual nscolor ComputeBackstopColor(nsView* aDisplayRoot) = 0;

  void ObserveNativeAnonMutationsForPrint(bool aObserve)
  {
    mObservesMutationsForPrint = aObserve;
  }
  bool ObservesNativeAnonMutationsForPrint()
  {
    return mObservesMutationsForPrint;
  }

  virtual nsresult SetIsActive(bool aIsActive) = 0;

  bool IsActive()
  {
    return mIsActive;
  }

  
  static CapturingContentInfo gCaptureInfo;

  struct PointerCaptureInfo
  {
    nsCOMPtr<nsIContent> mPendingContent;
    nsCOMPtr<nsIContent> mOverrideContent;
    bool                 mReleaseContent;
    bool                 mPrimaryState;
    
    explicit PointerCaptureInfo(nsIContent* aPendingContent, bool aPrimaryState) :
      mPendingContent(aPendingContent), mReleaseContent(false), mPrimaryState(aPrimaryState)
    {
      MOZ_COUNT_CTOR(PointerCaptureInfo);
    }
    ~PointerCaptureInfo()
    {
      MOZ_COUNT_DTOR(PointerCaptureInfo);
    }

    bool Empty()
    {
      return !(mPendingContent || mOverrideContent);
    }
  };

  
  
  
  
  static nsClassHashtable<nsUint32HashKey, PointerCaptureInfo>* gPointerCaptureList;

  struct PointerInfo
  {
    bool      mActiveState;
    uint16_t  mPointerType;
    bool      mPrimaryState;
    PointerInfo(bool aActiveState, uint16_t aPointerType, bool aPrimaryState) :
      mActiveState(aActiveState), mPointerType(aPointerType), mPrimaryState(aPrimaryState) {}
  };
  
  static nsClassHashtable<nsUint32HashKey, PointerInfo>* gActivePointersIds;

  static void DispatchGotOrLostPointerCaptureEvent(bool aIsGotCapture,
                                                   uint32_t aPointerId,
                                                   uint16_t aPointerType,
                                                   bool aIsPrimary,
                                                   nsIContent* aCaptureTarget);
  static void SetPointerCapturingContent(uint32_t aPointerId, nsIContent* aContent);
  static void ReleasePointerCapturingContent(uint32_t aPointerId, nsIContent* aContent);
  static nsIContent* GetPointerCapturingContent(uint32_t aPointerId);

  
  
  static bool CheckPointerCaptureState(uint32_t aPointerId);

  
  
  static bool GetPointerInfo(uint32_t aPointerId, bool& aActiveState);

  
  static uint16_t GetPointerType(uint32_t aPointerId);

  
  static bool GetPointerPrimaryState(uint32_t aPointerId);

  





















  static void SetCapturingContent(nsIContent* aContent, uint8_t aFlags);

  


  static nsIContent* GetCapturingContent()
  {
    return gCaptureInfo.mContent;
  }

  


  static void AllowMouseCapture(bool aAllowed)
  {
    gCaptureInfo.mAllowed = aAllowed;
  }

  



  static bool IsMouseCapturePreventingDrag()
  {
    return gCaptureInfo.mPreventDrag && gCaptureInfo.mContent;
  }

  



  uint64_t GetPaintCount() { return mPaintCount; }
  void IncrementPaintCount() { ++mPaintCount; }

  


  virtual already_AddRefed<nsPIDOMWindow> GetRootWindow() = 0;

  



  virtual LayerManager* GetLayerManager() = 0;

  




  virtual void SetIgnoreViewportScrolling(bool aIgnore) = 0;
  bool IgnoringViewportScrolling() const
  { return mRenderFlags & STATE_IGNORING_VIEWPORT_SCROLLING; }

   








  virtual nsresult SetResolution(float aResolution) = 0;
  float GetResolution() { return mResolution; }
  virtual float GetCumulativeResolution() = 0;

  



  virtual nsresult SetResolutionAndScaleTo(float aResolution) = 0;

  




  virtual bool ScaleToResolution() const = 0;

  



  bool InDrawWindowNotFlushing() const
  { return mRenderFlags & STATE_DRAWWINDOW_NOT_FLUSHING; }

  


  void SetIsFirstPaint(bool aIsFirstPaint) { mIsFirstPaint = aIsFirstPaint; }

  


  bool GetIsFirstPaint() const { return mIsFirstPaint; }

  uint32_t GetPresShellId() { return mPresShellId; }

  




  virtual void SynthesizeMouseMove(bool aFromScroll,
                                   bool aIsSynthesizedForTests) = 0;

  enum PaintFlags {
    


    PAINT_LAYERS = 0x01,
    
    PAINT_COMPOSITE = 0x02,
    
    PAINT_SYNC_DECODE_IMAGES = 0x04
  };
  virtual void Paint(nsView* aViewToPaint, const nsRegion& aDirtyRegion,
                     uint32_t aFlags) = 0;
  virtual nsresult HandleEvent(nsIFrame* aFrame,
                               mozilla::WidgetGUIEvent* aEvent,
                               bool aDontRetargetEvents,
                               nsEventStatus* aEventStatus) = 0;
  virtual bool ShouldIgnoreInvalidation() = 0;
  






  virtual void WillPaint() = 0;
  




  virtual void WillPaintWindow() = 0;
  




  virtual void DidPaintWindow() = 0;

  






  enum PaintType {
    PAINT_DEFAULT,
    PAINT_DELAYED_COMPRESS
  };
  virtual void ScheduleViewManagerFlush(PaintType aType = PAINT_DEFAULT) = 0;
  virtual void ClearMouseCaptureOnView(nsView* aView) = 0;
  virtual bool IsVisible() = 0;
  virtual void DispatchSynthMouseMove(mozilla::WidgetGUIEvent* aEvent,
                                      bool aFlushOnHoverChange) = 0;

  virtual void AddSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                      nsArenaMemoryStats *aArenaObjectsSize,
                                      size_t *aPresShellSize,
                                      size_t *aStyleSetsSize,
                                      size_t *aTextRunsSize,
                                      size_t *aPresContextSize) = 0;

  


  uint32_t FontSizeInflationEmPerLine() const {
    return mFontSizeInflationEmPerLine;
  }

  uint32_t FontSizeInflationMinTwips() const {
    return mFontSizeInflationMinTwips;
  }

  uint32_t FontSizeInflationLineThreshold() const {
    return mFontSizeInflationLineThreshold;
  }

  bool FontSizeInflationForceEnabled() const {
    return mFontSizeInflationForceEnabled;
  }

  bool FontSizeInflationDisabledInMasterProcess() const {
    return mFontSizeInflationDisabledInMasterProcess;
  }

  





  bool FontSizeInflationEnabled();

  





  void NotifyFontSizeInflationEnabledIsDirty()
  {
    mFontSizeInflationEnabledIsDirty = true;
  }

  virtual void AddInvalidateHiddenPresShellObserver(nsRefreshDriver *aDriver) = 0;

  void InvalidatePresShellIfHidden();
  void CancelInvalidatePresShellIfHidden();

  
  virtual void ScheduleImageVisibilityUpdate() = 0;

  
  
  virtual void RebuildImageVisibilityDisplayList(const nsDisplayList& aList) = 0;
  virtual void RebuildImageVisibility(nsRect* aRect = nullptr) = 0;

  
  virtual void EnsureImageInVisibleList(nsIImageLoadingContent* aImage) = 0;

  
  virtual void RemoveImageFromVisibleList(nsIImageLoadingContent* aImage) = 0;

  
  virtual bool AssumeAllImagesVisible() = 0;

  


protected:
  virtual bool AddRefreshObserverExternal(nsARefreshObserver* aObserver,
                                          mozFlushType aFlushType);
  bool AddRefreshObserverInternal(nsARefreshObserver* aObserver,
                                  mozFlushType aFlushType);
  virtual bool RemoveRefreshObserverExternal(nsARefreshObserver* aObserver,
                                             mozFlushType aFlushType);
  bool RemoveRefreshObserverInternal(nsARefreshObserver* aObserver,
                                     mozFlushType aFlushType);

  




  void RecomputeFontSizeInflationEnabled();

public:
  bool AddRefreshObserver(nsARefreshObserver* aObserver,
                          mozFlushType aFlushType) {
#ifdef MOZILLA_INTERNAL_API
    return AddRefreshObserverInternal(aObserver, aFlushType);
#else
    return AddRefreshObserverExternal(aObserver, aFlushType);
#endif
  }

  bool RemoveRefreshObserver(nsARefreshObserver* aObserver,
                             mozFlushType aFlushType) {
#ifdef MOZILLA_INTERNAL_API
    return RemoveRefreshObserverInternal(aObserver, aFlushType);
#else
    return RemoveRefreshObserverExternal(aObserver, aFlushType);
#endif
  }

  virtual bool AddPostRefreshObserver(nsAPostRefreshObserver* aObserver);
  virtual bool RemovePostRefreshObserver(nsAPostRefreshObserver* aObserver);

  


  static void InitializeStatics();
  static void ReleaseStatics();

  
  
  static void ClearMouseCapture(nsIFrame* aFrame);

  void SetScrollPositionClampingScrollPortSize(nscoord aWidth, nscoord aHeight);
  bool IsScrollPositionClampingScrollPortSizeSet() {
    return mScrollPositionClampingScrollPortSizeSet;
  }
  nsSize GetScrollPositionClampingScrollPortSize() {
    NS_ASSERTION(mScrollPositionClampingScrollPortSizeSet, "asking for scroll port when its not set?");
    return mScrollPositionClampingScrollPortSize;
  }

  void SetContentDocumentFixedPositionMargins(const nsMargin& aMargins);
  const nsMargin& GetContentDocumentFixedPositionMargins() {
    return mContentDocumentFixedPositionMargins;
  }

  virtual void WindowSizeMoveDone() = 0;
  virtual void SysColorChanged() = 0;
  virtual void ThemeChanged() = 0;
  virtual void BackingScaleFactorChanged() = 0;

  nscoord MaxLineBoxWidth() {
    return mMaxLineBoxWidth;
  }

  void SetMaxLineBoxWidth(nscoord aMaxLineBoxWidth);

  







  bool IsReflowOnZoomPending() {
    return mReflowOnZoomPending;
  }

  



  void ClearReflowOnZoomPending() {
    mReflowOnZoomPending = false;
  }

  


  bool IsNeverPainting() {
    return mIsNeverPainting;
  }

  void SetNeverPainting(bool aNeverPainting) {
    mIsNeverPainting = aNeverPainting;
  }

  bool HasPendingReflow() const
    { return mReflowScheduled || mReflowContinueTimer; }

protected:
  friend class nsRefreshDriver;

  
  
  

  
  
  nsIDocument*              mDocument;      
  nsPresContext*            mPresContext;   
  nsStyleSet*               mStyleSet;      
  nsCSSFrameConstructor*    mFrameConstructor; 
  nsViewManager*           mViewManager;   
  nsPresArena               mFrameArena;
  nsFrameSelection*         mSelection;
  
  
  nsFrameManagerBase*       mFrameManager;
  mozilla::WeakPtr<nsDocShell>                 mForwardingContainer;
  nsRefreshDriver*          mHiddenInvalidationObserverRefreshDriver;
#ifdef ACCESSIBILITY
  mozilla::a11y::DocAccessible* mDocAccessible;
#endif

  
  
  
  
  nsCOMPtr<nsITimer>        mReflowContinueTimer;

#ifdef DEBUG
  nsIFrame*                 mDrawEventTargetFrame;
  
  uint32_t                  mPresArenaAllocCount;
#endif

  
  uint64_t                  mPaintCount;

  nsSize                    mScrollPositionClampingScrollPortSize;

  
  
  
  
  nsMargin                  mContentDocumentFixedPositionMargins;

  
  nsWeakFrame*              mWeakFrames;

  
  nscolor                   mCanvasBackgroundColor;

  
  
  float                     mResolution;

  int16_t                   mSelectionFlags;

  
  
  
  
  
  RenderFlags               mRenderFlags;

  
  
  
  bool                      mStylesHaveChanged : 1;
  bool                      mDidInitialize : 1;
  bool                      mIsDestroying : 1;
  bool                      mIsZombie : 1;
  bool                      mIsReflowing : 1;

  
  bool                      mPaintingSuppressed : 1;

  
  bool                      mIsThemeSupportDisabled : 1;

  bool                      mIsActive : 1;
  bool                      mFrozen : 1;
  bool                      mIsFirstPaint : 1;
  bool                      mObservesMutationsForPrint : 1;

  
  
  bool                      mReflowScheduled : 1;

  bool                      mSuppressInterruptibleReflows : 1;
  bool                      mScrollPositionClampingScrollPortSizeSet : 1;

  uint32_t                  mPresShellId;

  
  
  
  
  
  
  
  nsAutoTArray<nsRefPtr<mozilla::dom::Element>,1> mChangedScopeStyleRoots;

  static nsIContent*        gKeyDownTarget;

  
  
  uint32_t mFontSizeInflationEmPerLine;
  uint32_t mFontSizeInflationMinTwips;
  uint32_t mFontSizeInflationLineThreshold;
  bool mFontSizeInflationForceEnabled;
  bool mFontSizeInflationDisabledInMasterProcess;
  bool mFontSizeInflationEnabled;
  bool mPaintingIsFrozen;

  
  bool mFontSizeInflationEnabledIsDirty;

  
  
  bool mReflowOnZoomPending;

  
  
  nscoord mMaxLineBoxWidth;

  
  
  
  bool mIsNeverPainting;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPresShell, NS_IPRESSHELL_IID)

#endif 
