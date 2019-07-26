


















#ifndef nsIPresShell_h___
#define nsIPresShell_h___

#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsISupports.h"
#include "nsQueryFrame.h"
#include "nsCoord.h"
#include "nsColor.h"
#include "nsEvent.h"
#include "nsCompatibility.h"
#include "nsFrameManagerBase.h"
#include "nsRect.h"
#include "mozFlushType.h"
#include "nsWeakReference.h"
#include <stdio.h> 
#include "nsChangeHint.h"
#include "nsGUIEvent.h"
#include "nsInterfaceHashtable.h"
#include "nsEventStates.h"
#include "nsPresArena.h"

class nsIContent;
class nsIDocument;
class nsIFrame;
class nsPresContext;
class nsStyleSet;
class nsViewManager;
class nsView;
class nsRenderingContext;
class nsIPageSequenceFrame;
class nsAString;
class nsCaret;
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
class gfxASurface;
class gfxContext;
class nsIDOMEvent;
class nsDisplayList;
class nsDisplayListBuilder;
class nsPIDOMWindow;
struct nsPoint;
struct nsIntPoint;
struct nsIntRect;
class nsRegion;
class nsRefreshDriver;
class nsARefreshObserver;
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

typedef short SelectionType;
typedef uint64_t nsFrameState;

namespace mozilla {
class Selection;

namespace dom {
class Element;
} 

namespace layers{
class LayerManager;
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
{ 0x15f6c268, 0xe40f, 0x42a9, \
  { 0xa7, 0xeb, 0xe5, 0xe1, 0x0a, 0x58, 0x40, 0xa1 } }


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














class nsIPresShell_base : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPRESSHELL_IID)
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPresShell_base, NS_IPRESSHELL_IID)

class nsIPresShell : public nsIPresShell_base
{
protected:
  typedef mozilla::layers::LayerManager LayerManager;

  enum eRenderFlag {
    STATE_IGNORING_VIEWPORT_SCROLLING = 0x1
  };
  typedef uint8_t RenderFlags; 

public:
  virtual NS_HIDDEN_(nsresult) Init(nsIDocument* aDocument,
                                   nsPresContext* aPresContext,
                                   nsViewManager* aViewManager,
                                   nsStyleSet* aStyleSet,
                                   nsCompatibility aCompatMode) = 0;

  






  virtual NS_HIDDEN_(void) Destroy() = 0;

  bool IsDestroying() { return mIsDestroying; }

  









  virtual NS_HIDDEN_(void) MakeZombie() = 0;

  






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
    if (PRESARENA_MUST_FREE_DURING_DESTROY || !mIsDestroying)
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
    if (PRESARENA_MUST_FREE_DURING_DESTROY || !mIsDestroying)
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
    if (PRESARENA_MUST_FREE_DURING_DESTROY || !mIsDestroying)
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

#ifdef _IMPL_NS_LAYOUT
  nsStyleSet* StyleSet() const { return mStyleSet; }

  nsCSSFrameConstructor* FrameConstructor() const { return mFrameConstructor; }

  nsFrameManager* FrameManager() const {
    
    
    return reinterpret_cast<nsFrameManager*>
                           (const_cast<nsIPresShell*>(this)->mFrameManager);
  }

#endif

  


  
  
  NS_HIDDEN_(void) SetAuthorStyleDisabled(bool aDisabled);
  NS_HIDDEN_(bool) GetAuthorStyleDisabled() const;

  









  virtual NS_HIDDEN_(void) ReconstructStyleDataExternal();
  NS_HIDDEN_(void) ReconstructStyleDataInternal();
#ifdef _IMPL_NS_LAYOUT
  void ReconstructStyleData() { ReconstructStyleDataInternal(); }
#else
  void ReconstructStyleData() { ReconstructStyleDataExternal(); }
#endif

  







  virtual NS_HIDDEN_(nsresult) SetPreferenceStyleRules(bool aForceReflow) = 0;

  




  already_AddRefed<nsFrameSelection> FrameSelection();

  



  const nsFrameSelection* ConstFrameSelection() const { return mSelection; }

  
  
  virtual NS_HIDDEN_(void) BeginObservingDocument() = 0;

  
  virtual NS_HIDDEN_(void) EndObservingDocument() = 0;

  


  bool DidInitialize() const { return mDidInitialize; }

  










  virtual NS_HIDDEN_(nsresult) Initialize(nscoord aWidth, nscoord aHeight) = 0;

  



  virtual NS_HIDDEN_(nsresult) ResizeReflow(nscoord aWidth, nscoord aHeight) = 0;
  




  virtual NS_HIDDEN_(nsresult) ResizeReflowOverride(nscoord aWidth, nscoord aHeight) = 0;

  


  virtual bool GetIsViewportOverridden() = 0;

  


  virtual bool IsLayoutFlushObserver() = 0;

  


  virtual NS_HIDDEN_(void) StyleChangeReflow() = 0;

  


  virtual NS_HIDDEN_(nsIFrame*) GetRootFrameExternal() const;
  nsIFrame* GetRootFrame() const {
#ifdef _IMPL_NS_LAYOUT
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

  



  virtual NS_HIDDEN_(nsIPageSequenceFrame*) GetPageSequenceFrame() const = 0;

  






  virtual NS_HIDDEN_(nsIFrame*) GetRealPrimaryFrameFor(nsIContent* aContent) const = 0;

  



  virtual NS_HIDDEN_(nsIFrame*) GetPlaceholderFrameFor(nsIFrame* aFrame) const = 0;

  






  enum IntrinsicDirty {
    
    eResize,     
    eTreeChange, 
    eStyleChange 
  };
  virtual NS_HIDDEN_(void) FrameNeedsReflow(nsIFrame *aFrame,
                                            IntrinsicDirty aIntrinsicDirty,
                                            nsFrameState aBitToAdd) = 0;

  










  virtual NS_HIDDEN_(void) FrameNeedsToContinueReflow(nsIFrame *aFrame) = 0;

  virtual NS_HIDDEN_(void) CancelAllPendingReflows() = 0;

  


  virtual NS_HIDDEN_(nsresult) RecreateFramesFor(nsIContent* aContent) = 0;

  void PostRecreateFramesFor(mozilla::dom::Element* aElement);
  void RestyleForAnimation(mozilla::dom::Element* aElement,
                           nsRestyleHint aHint);

  




  virtual NS_HIDDEN_(bool) IsSafeToFlush() const = 0;

  








  virtual NS_HIDDEN_(void) FlushPendingNotifications(mozFlushType aType) = 0;
  virtual NS_HIDDEN_(void) FlushPendingNotifications(mozilla::ChangesToFlush aType) = 0;

  



  virtual NS_HIDDEN_(nsresult) PostReflowCallback(nsIReflowCallback* aCallback) = 0;
  virtual NS_HIDDEN_(void) CancelReflowCallback(nsIReflowCallback* aCallback) = 0;

  virtual NS_HIDDEN_(void) ClearFrameRefs(nsIFrame* aFrame) = 0;

  




  virtual already_AddRefed<nsRenderingContext> GetReferenceRenderingContext() = 0;

  







  virtual NS_HIDDEN_(nsresult) GoToAnchor(const nsAString& aAnchorName, bool aScroll) = 0;

  







  virtual NS_HIDDEN_(nsresult) ScrollToAnchor() = 0;

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
  


































    ScrollAxis(int16_t aWhere = SCROLL_MINIMUM,
               WhenToScroll aWhen = SCROLL_IF_NOT_FULLY_VISIBLE,
               bool aOnlyIfPerceivedScrollableDirection = false) :
      mWhereToScroll(aWhere), mWhenToScroll(aWhen),
      mOnlyIfPerceivedScrollableDirection(aOnlyIfPerceivedScrollableDirection)
    {}
  } ScrollAxis;
  




















  virtual NS_HIDDEN_(nsresult) ScrollContentIntoView(nsIContent* aContent,
                                                     ScrollAxis  aVertical,
                                                     ScrollAxis  aHorizontal,
                                                     uint32_t    aFlags) = 0;

  enum {
    SCROLL_FIRST_ANCESTOR_ONLY = 0x01,
    SCROLL_OVERFLOW_HIDDEN = 0x02,
    SCROLL_NO_PARENT_FRAMES = 0x04
  };
  


















  virtual bool ScrollFrameRectIntoView(nsIFrame*     aFrame,
                                       const nsRect& aRect,
                                       ScrollAxis    aVertical,
                                       ScrollAxis    aHorizontal,
                                       uint32_t      aFlags) = 0;

  













  virtual nsRectVisibility GetRectVisibility(nsIFrame *aFrame,
                                             const nsRect &aRect,
                                             nscoord aMinTwips) const = 0;

  



  virtual NS_HIDDEN_(void) SetIgnoreFrameDestruction(bool aIgnore) = 0;

  




  virtual NS_HIDDEN_(void) NotifyDestroyingFrame(nsIFrame* aFrame) = 0;

  


  virtual NS_HIDDEN_(already_AddRefed<nsCaret>) GetCaret() const = 0;

  




  virtual NS_HIDDEN_(void) MaybeInvalidateCaretPosition() = 0;

  


  virtual void SetCaret(nsCaret *aNewCaret) = 0;

  



  virtual void RestoreCaret() = 0;

  







  NS_IMETHOD SetSelectionFlags(int16_t aInEnable) = 0;

  




  int16_t GetSelectionFlags() const { return mSelectionFlags; }

  virtual mozilla::Selection* GetCurrentSelection(SelectionType aType) = 0;

  



  virtual NS_HIDDEN_(nsresult) HandleEventWithTarget(nsEvent* aEvent,
                                                     nsIFrame* aFrame,
                                                     nsIContent* aContent,
                                                     nsEventStatus* aStatus) = 0;

  



  virtual NS_HIDDEN_(nsresult) HandleDOMEventWithTarget(nsIContent* aTargetContent,
                                                        nsEvent* aEvent,
                                                        nsEventStatus* aStatus) = 0;

  



  virtual NS_HIDDEN_(nsresult) HandleDOMEventWithTarget(nsIContent* aTargetContent,
                                                        nsIDOMEvent* aEvent,
                                                        nsEventStatus* aStatus) = 0;

  


  virtual NS_HIDDEN_(nsIFrame*) GetEventTargetFrame() = 0;

  


  virtual NS_HIDDEN_(already_AddRefed<nsIContent>) GetEventTargetContent(nsEvent* aEvent) = 0;

  



  virtual NS_HIDDEN_(nsresult) CaptureHistoryState(nsILayoutHistoryState** aLayoutHistoryState) = 0;

  



  bool IsReflowLocked() const { return mIsReflowing; }

  




  bool IsPaintingSuppressed() const { return mPaintingSuppressed; }

  


  virtual NS_HIDDEN_(void) UnsuppressPainting() = 0;

  


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
                                   nsEventStates aStateMask) = 0;

  





  static bool GetVerifyReflowEnable();

  


  static void SetVerifyReflowEnable(bool aEnabled);

  virtual nsIFrame* GetAbsoluteContainingBlock(nsIFrame* aFrame);

#ifdef MOZ_REFLOW_PERF
  virtual NS_HIDDEN_(void) DumpReflows() = 0;
  virtual NS_HIDDEN_(void) CountReflows(const char * aName, nsIFrame * aFrame) = 0;
  virtual NS_HIDDEN_(void) PaintCount(const char * aName,
                                      nsRenderingContext* aRenderingContext,
                                      nsPresContext * aPresContext,
                                      nsIFrame * aFrame,
                                      const nsPoint& aOffset,
                                      uint32_t aColor) = 0;
  virtual NS_HIDDEN_(void) SetPaintFrameCount(bool aOn) = 0;
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

  





  void SetForwardingContainer(nsWeakPtr aContainer)
  {
    mForwardingContainer = aContainer;
  }
  
  


































  enum {
    RENDER_IS_UNTRUSTED = 0x01,
    RENDER_IGNORE_VIEWPORT_SCROLLING = 0x02,
    RENDER_CARET = 0x04,
    RENDER_USE_WIDGET_LAYERS = 0x08,
    RENDER_ASYNC_DECODE_IMAGES = 0x10,
    RENDER_DOCUMENT_RELATIVE = 0x20
  };
  virtual NS_HIDDEN_(nsresult) RenderDocument(const nsRect& aRect, uint32_t aFlags,
                                              nscolor aBackgroundColor,
                                              gfxContext* aRenderedContext) = 0;

  





  virtual already_AddRefed<gfxASurface> RenderNode(nsIDOMNode* aNode,
                                                   nsIntRegion* aRegion,
                                                   nsIntPoint& aPoint,
                                                   nsIntRect* aScreenRect) = 0;

  














  virtual already_AddRefed<gfxASurface> RenderSelection(nsISelection* aSelection,
                                                        nsIntPoint& aPoint,
                                                        nsIntRect* aScreenRect) = 0;

  void AddWeakFrameInternal(nsWeakFrame* aWeakFrame);
  virtual void AddWeakFrameExternal(nsWeakFrame* aWeakFrame);

  void AddWeakFrame(nsWeakFrame* aWeakFrame)
  {
#ifdef _IMPL_NS_LAYOUT
    AddWeakFrameInternal(aWeakFrame);
#else
    AddWeakFrameExternal(aWeakFrame);
#endif
  }

  void RemoveWeakFrameInternal(nsWeakFrame* aWeakFrame);
  virtual void RemoveWeakFrameExternal(nsWeakFrame* aWeakFrame);

  void RemoveWeakFrame(nsWeakFrame* aWeakFrame)
  {
#ifdef _IMPL_NS_LAYOUT
    RemoveWeakFrameInternal(aWeakFrame);
#else
    RemoveWeakFrameExternal(aWeakFrame);
#endif
  }

#ifdef DEBUG
  nsIFrame* GetDrawEventTargetFrame() { return mDrawEventTargetFrame; }
#endif

  






  virtual NS_HIDDEN_(void) DisableNonTestMouseEvents(bool aDisable) = 0;

  






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

  static nsInterfaceHashtable<nsUint32HashKey, nsIDOMTouch> gCaptureTouchList;
  static bool gPreventMouseEvents;

  





















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

   








  virtual nsresult SetResolution(float aXResolution, float aYResolution) = 0;
  gfxSize GetResolution() { return gfxSize(mXResolution, mYResolution); }
  float GetXResolution() { return mXResolution; }
  float GetYResolution() { return mYResolution; }

  


  void SetIsFirstPaint(bool aIsFirstPaint) { mIsFirstPaint = aIsFirstPaint; }

  


  bool GetIsFirstPaint() const { return mIsFirstPaint; }

  




  virtual void SynthesizeMouseMove(bool aFromScroll) = 0;

  enum PaintFlags {
    


    PAINT_LAYERS = 0x01,
    
    PAINT_COMPOSITE = 0x02,
  };
  virtual void Paint(nsView* aViewToPaint, const nsRegion& aDirtyRegion,
                     uint32_t aFlags) = 0;
  virtual nsresult HandleEvent(nsIFrame*       aFrame,
                               nsGUIEvent*     aEvent,
                               bool            aDontRetargetEvents,
                               nsEventStatus*  aEventStatus) = 0;
  virtual bool ShouldIgnoreInvalidation() = 0;
  






  virtual void WillPaint() = 0;
  




  virtual void WillPaintWindow() = 0;
  




  virtual void DidPaintWindow() = 0;

  



  virtual void ScheduleViewManagerFlush() = 0;
  virtual void ClearMouseCaptureOnView(nsView* aView) = 0;
  virtual bool IsVisible() = 0;
  virtual void DispatchSynthMouseMove(nsGUIEvent *aEvent, bool aFlushOnHoverChange) = 0;

  virtual void SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
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

  virtual void AddInvalidateHiddenPresShellObserver(nsRefreshDriver *aDriver) = 0;

  void InvalidatePresShellIfHidden();

  
  virtual void ScheduleImageVisibilityUpdate() = 0;

  
  
  virtual void RebuildImageVisibility(const nsDisplayList& aList) = 0;

  


protected:
  virtual bool AddRefreshObserverExternal(nsARefreshObserver* aObserver,
                                            mozFlushType aFlushType);
  bool AddRefreshObserverInternal(nsARefreshObserver* aObserver,
                                    mozFlushType aFlushType);
  virtual bool RemoveRefreshObserverExternal(nsARefreshObserver* aObserver,
                                               mozFlushType aFlushType);
  bool RemoveRefreshObserverInternal(nsARefreshObserver* aObserver,
                                       mozFlushType aFlushType);
public:
  bool AddRefreshObserver(nsARefreshObserver* aObserver,
                            mozFlushType aFlushType) {
#ifdef _IMPL_NS_LAYOUT
    return AddRefreshObserverInternal(aObserver, aFlushType);
#else
    return AddRefreshObserverExternal(aObserver, aFlushType);
#endif
  }

  bool RemoveRefreshObserver(nsARefreshObserver* aObserver,
                               mozFlushType aFlushType) {
#ifdef _IMPL_NS_LAYOUT
    return RemoveRefreshObserverInternal(aObserver, aFlushType);
#else
    return RemoveRefreshObserverExternal(aObserver, aFlushType);
#endif
  }

  


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

  virtual void WindowSizeMoveDone() = 0;
  virtual void SysColorChanged() = 0;
  virtual void ThemeChanged() = 0;
  virtual void BackingScaleFactorChanged() = 0;

  nscoord MaxLineBoxWidth() {
    return mMaxLineBoxWidth;
  }

  void SetMaxLineBoxWidth(nscoord aMaxLineBoxWidth);

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
  nsWeakPtr                 mForwardingContainer;
  nsRefreshDriver*          mHiddenInvalidationObserverRefreshDriver;
#ifdef ACCESSIBILITY
  mozilla::a11y::DocAccessible* mDocAccessible;
#endif

#ifdef DEBUG
  nsIFrame*                 mDrawEventTargetFrame;
  
  uint32_t                  mPresArenaAllocCount;
#endif

  
  uint64_t                  mPaintCount;

  nsSize                    mScrollPositionClampingScrollPortSize;

  
  nsWeakFrame*              mWeakFrames;

  
  nscolor                   mCanvasBackgroundColor;

  
  
  float                     mXResolution;
  float                     mYResolution;

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

  
  
  
  
  
  
  
  nsAutoTArray<nsRefPtr<mozilla::dom::Element>,1> mChangedScopeStyleRoots;

  static nsIContent*        gKeyDownTarget;

  
  
  uint32_t mFontSizeInflationEmPerLine;
  uint32_t mFontSizeInflationMinTwips;
  uint32_t mFontSizeInflationLineThreshold;
  bool mFontSizeInflationForceEnabled;
  bool mFontSizeInflationDisabledInMasterProcess;

  
  
  nscoord mMaxLineBoxWidth;
};





nsresult
NS_NewPresShell(nsIPresShell** aInstancePtrResult);

#endif
