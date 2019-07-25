





















































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

class nsIContent;
class nsIDocument;
class nsIFrame;
class nsPresContext;
class nsStyleSet;
class nsIViewManager;
class nsIView;
class nsIRenderingContext;
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
class nsRefreshDriver;
class nsARefreshObserver;
#ifdef ACCESSIBILITY
class nsAccessibilityService;
#endif

typedef short SelectionType;
typedef PRUint64 nsFrameState;

namespace mozilla {
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

typedef struct CapturingContentInfo {
  
  PRPackedBool mAllowed;
  PRPackedBool mRetargetToElement;
  PRPackedBool mPreventDrag;
  nsIContent* mContent;
} CapturingContentInfo;

#define NS_IPRESSHELL_IID     \
 { 0x3a8030b5, 0x8d2c, 0x4cb3, \
    { 0xb5, 0xae, 0xb2, 0x43, 0xa9, 0x28, 0x02, 0x82 } }


#define NS_PRESSHELL_SCROLL_TOP      0
#define NS_PRESSHELL_SCROLL_BOTTOM   100
#define NS_PRESSHELL_SCROLL_LEFT     0
#define NS_PRESSHELL_SCROLL_RIGHT    100
#define NS_PRESSHELL_SCROLL_CENTER   50
#define NS_PRESSHELL_SCROLL_ANYWHERE -1
#define NS_PRESSHELL_SCROLL_IF_NOT_VISIBLE -2


#define VERIFY_REFLOW_ON              0x01
#define VERIFY_REFLOW_NOISY           0x02
#define VERIFY_REFLOW_ALL             0x04
#define VERIFY_REFLOW_DUMP_COMMANDS   0x08
#define VERIFY_REFLOW_NOISY_RC        0x10
#define VERIFY_REFLOW_REALLY_NOISY_RC 0x20
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

  enum {
    STATE_IGNORING_VIEWPORT_SCROLLING = 0x1,
    STATE_USING_DISPLAYPORT = 0x2
  };

public:
  virtual NS_HIDDEN_(nsresult) Init(nsIDocument* aDocument,
                                   nsPresContext* aPresContext,
                                   nsIViewManager* aViewManager,
                                   nsStyleSet* aStyleSet,
                                   nsCompatibility aCompatMode) = 0;

  






  virtual NS_HIDDEN_(void) Destroy() = 0;

  PRBool IsDestroying() { return mIsDestroying; }

  
  
  
  
  
  virtual void* AllocateFrame(nsQueryFrame::FrameIID aCode, size_t aSize) = 0;
  virtual void  FreeFrame(nsQueryFrame::FrameIID aCode, void* aChunk) = 0;

  
  
  
  
  virtual void* AllocateMisc(size_t aSize) = 0;
  virtual void  FreeMisc(size_t aSize, void* aChunk) = 0;

  














  virtual void PushStackMemory() = 0;
  virtual void PopStackMemory() = 0;
  virtual void* AllocateStackMemory(size_t aSize) = 0;

  nsIDocument* GetDocument() const { return mDocument; }

  nsPresContext* GetPresContext() const { return mPresContext; }

  nsIViewManager* GetViewManager() const { return mViewManager; }

#ifdef _IMPL_NS_LAYOUT
  nsStyleSet* StyleSet() const { return mStyleSet; }

  nsCSSFrameConstructor* FrameConstructor() const { return mFrameConstructor; }

  nsFrameManager* FrameManager() const {
    return reinterpret_cast<nsFrameManager*>
                           (&const_cast<nsIPresShell*>(this)->mFrameManager);
  }

#endif

  


  
  
  NS_HIDDEN_(void) SetAuthorStyleDisabled(PRBool aDisabled);
  NS_HIDDEN_(PRBool) GetAuthorStyleDisabled() const;

  









  virtual NS_HIDDEN_(void) ReconstructStyleDataExternal();
  NS_HIDDEN_(void) ReconstructStyleDataInternal();
#ifdef _IMPL_NS_LAYOUT
  void ReconstructStyleData() { ReconstructStyleDataInternal(); }
#else
  void ReconstructStyleData() { ReconstructStyleDataExternal(); }
#endif

  







  virtual NS_HIDDEN_(nsresult) SetPreferenceStyleRules(PRBool aForceReflow) = 0;

  




  already_AddRefed<nsFrameSelection> FrameSelection();

  



  const nsFrameSelection* ConstFrameSelection() const { return mSelection; }

  
  
  virtual NS_HIDDEN_(void) BeginObservingDocument() = 0;

  
  virtual NS_HIDDEN_(void) EndObservingDocument() = 0;

  


  PRBool DidInitialReflow() const { return mDidInitialReflow; }

  










  virtual NS_HIDDEN_(nsresult) InitialReflow(nscoord aWidth, nscoord aHeight) = 0;

  



  virtual NS_HIDDEN_(nsresult) ResizeReflow(nscoord aWidth, nscoord aHeight) = 0;
  




  virtual NS_HIDDEN_(nsresult) ResizeReflowOverride(nscoord aWidth, nscoord aHeight) = 0;

  


  virtual PRBool GetIsViewportOverridden() = 0;

  


  virtual PRBool IsLayoutFlushObserver() = 0;

  


  virtual NS_HIDDEN_(void) StyleChangeReflow() = 0;

  


  virtual NS_HIDDEN_(nsIFrame*) GetRootFrameExternal() const;
  nsIFrame* GetRootFrame() const {
#ifdef _IMPL_NS_LAYOUT
    return mFrameManager.GetRootFrame();
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

  




  virtual NS_HIDDEN_(PRBool) IsSafeToFlush() const = 0;

  








  virtual NS_HIDDEN_(void) FlushPendingNotifications(mozFlushType aType) = 0;

  



  virtual NS_HIDDEN_(nsresult) PostReflowCallback(nsIReflowCallback* aCallback) = 0;
  virtual NS_HIDDEN_(void) CancelReflowCallback(nsIReflowCallback* aCallback) = 0;

  virtual NS_HIDDEN_(void) ClearFrameRefs(nsIFrame* aFrame) = 0;

  




  virtual already_AddRefed<nsIRenderingContext> GetReferenceRenderingContext() = 0;

  







  virtual NS_HIDDEN_(nsresult) GoToAnchor(const nsAString& aAnchorName, PRBool aScroll) = 0;

  







  virtual NS_HIDDEN_(nsresult) ScrollToAnchor() = 0;

  






































  virtual NS_HIDDEN_(nsresult) ScrollContentIntoView(nsIContent* aContent,
                                                     PRIntn      aVPercent,
                                                     PRIntn      aHPercent,
                                                     PRUint32    aFlags) = 0;

  enum {
    SCROLL_FIRST_ANCESTOR_ONLY = 0x01,
    SCROLL_OVERFLOW_HIDDEN = 0x02,
    SCROLL_NO_PARENT_FRAMES = 0x04
  };
  


















  virtual PRBool ScrollFrameRectIntoView(nsIFrame*     aFrame,
                                         const nsRect& aRect,
                                         PRIntn        aVPercent,
                                         PRIntn        aHPercent,
                                         PRUint32      aFlags) = 0;

  













  virtual nsRectVisibility GetRectVisibility(nsIFrame *aFrame,
                                             const nsRect &aRect,
                                             nscoord aMinTwips) const = 0;

  



  virtual NS_HIDDEN_(void) SetIgnoreFrameDestruction(PRBool aIgnore) = 0;

  




  virtual NS_HIDDEN_(void) NotifyDestroyingFrame(nsIFrame* aFrame) = 0;

  


  virtual NS_HIDDEN_(nsresult) GetLinkLocation(nsIDOMNode* aNode, nsAString& aLocation) const = 0;

  


  virtual NS_HIDDEN_(already_AddRefed<nsCaret>) GetCaret() const = 0;

  




  virtual NS_HIDDEN_(void) MaybeInvalidateCaretPosition() = 0;

  


  virtual void SetCaret(nsCaret *aNewCaret) = 0;

  



  virtual void RestoreCaret() = 0;

  







  NS_IMETHOD SetSelectionFlags(PRInt16 aInEnable) = 0;

  




  PRInt16 GetSelectionFlags() const { return mSelectionFlags; }

  virtual nsISelection* GetCurrentSelection(SelectionType aType) = 0;

  



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

  



  virtual NS_HIDDEN_(nsresult) CaptureHistoryState(nsILayoutHistoryState** aLayoutHistoryState, PRBool aLeavingPage = PR_FALSE) = 0;

  



  PRBool IsReflowLocked() const { return mIsReflowing; }

  




  PRBool IsPaintingSuppressed() const { return mPaintingSuppressed; }

  


  virtual NS_HIDDEN_(void) UnsuppressPainting() = 0;

  


  void DisableThemeSupport()
  {
    
    mIsThemeSupportDisabled = PR_TRUE;
  }

  


  PRBool IsThemeSupportEnabled() const { return !mIsThemeSupportDisabled; }

  


  virtual nsresult GetAgentStyleSheets(nsCOMArray<nsIStyleSheet>& aSheets) = 0;

  


  virtual nsresult SetAgentStyleSheets(const nsCOMArray<nsIStyleSheet>& aSheets) = 0;

  


  virtual nsresult AddOverrideStyleSheet(nsIStyleSheet *aSheet) = 0;

  


  virtual nsresult RemoveOverrideStyleSheet(nsIStyleSheet *aSheet) = 0;

  


  virtual nsresult ReconstructFrames() = 0;

  





  virtual nsIFrame* GetFrameForPoint(nsIFrame* aFrame, nsPoint aPt) = 0;

  





  static PRBool GetVerifyReflowEnable();

  


  static void SetVerifyReflowEnable(PRBool aEnabled);

  virtual nsIFrame* GetAbsoluteContainingBlock(nsIFrame* aFrame);

#ifdef MOZ_REFLOW_PERF
  virtual NS_HIDDEN_(void) DumpReflows() = 0;
  virtual NS_HIDDEN_(void) CountReflows(const char * aName, nsIFrame * aFrame) = 0;
  virtual NS_HIDDEN_(void) PaintCount(const char * aName,
                                      nsIRenderingContext* aRenderingContext,
                                      nsPresContext * aPresContext,
                                      nsIFrame * aFrame,
                                      PRUint32 aColor) = 0;
  virtual NS_HIDDEN_(void) SetPaintFrameCount(PRBool aOn) = 0;
  virtual PRBool IsPaintingFrameCounts() = 0;
#endif

#ifdef DEBUG
  
  virtual void ListStyleContexts(nsIFrame *aRootFrame, FILE *out,
                                 PRInt32 aIndent = 0) = 0;

  virtual void ListStyleSheets(FILE *out, PRInt32 aIndent = 0) = 0;
  virtual void VerifyStyleTree() = 0;
#endif

  static PRBool gIsAccessibilityActive;
  static PRBool IsAccessibilityActive() { return gIsAccessibilityActive; }

#ifdef ACCESSIBILITY
  


  static nsAccessibilityService* AccService();
#endif

  




  virtual void Freeze() = 0;
  PRBool IsFrozen() { return mFrozen; }

  



  virtual void Thaw() = 0;

  virtual void FireOrClearDelayedEvents(PRBool aFireEvents) = 0;

  





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
  virtual NS_HIDDEN_(nsresult) RenderDocument(const nsRect& aRect, PRUint32 aFlags,
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

#ifdef NS_DEBUG
  nsIFrame* GetDrawEventTargetFrame() { return mDrawEventTargetFrame; }
#endif

  






  virtual NS_HIDDEN_(void) DisableNonTestMouseEvents(PRBool aDisable) = 0;

  






  void SetCanvasBackground(nscolor aColor) { mCanvasBackgroundColor = aColor; }
  nscolor GetCanvasBackground() { return mCanvasBackgroundColor; }

  



  virtual void UpdateCanvasBackground() = 0;

  







  enum {
    FORCE_DRAW = 0x01,
    ROOT_CONTENT_DOC_BG = 0x02
  };
  virtual nsresult AddCanvasBackgroundColorItem(nsDisplayListBuilder& aBuilder,
                                                nsDisplayList& aList,
                                                nsIFrame* aFrame,
                                                const nsRect& aBounds,
                                                nscolor aBackstopColor = NS_RGBA(0,0,0,0),
                                                PRUint32 aFlags = 0) = 0;


  




  virtual nsresult AddPrintPreviewBackgroundItem(nsDisplayListBuilder& aBuilder,
                                                 nsDisplayList& aList,
                                                 nsIFrame* aFrame,
                                                 const nsRect& aBounds) = 0;

  




  virtual nscolor ComputeBackstopColor(nsIView* aDisplayRoot) = 0;

  void ObserveNativeAnonMutationsForPrint(PRBool aObserve)
  {
    mObservesMutationsForPrint = aObserve;
  }
  PRBool ObservesNativeAnonMutationsForPrint()
  {
    return mObservesMutationsForPrint;
  }

  virtual nsresult SetIsActive(PRBool aIsActive) = 0;

  PRBool IsActive()
  {
    return mIsActive;
  }

  

  static CapturingContentInfo gCaptureInfo;

  
















  static void SetCapturingContent(nsIContent* aContent, PRUint8 aFlags);

  


  static nsIContent* GetCapturingContent()
  {
    return gCaptureInfo.mContent;
  }

  


  static void AllowMouseCapture(PRBool aAllowed)
  {
    gCaptureInfo.mAllowed = aAllowed;
  }

  



  static PRBool IsMouseCapturePreventingDrag()
  {
    return gCaptureInfo.mPreventDrag && gCaptureInfo.mContent;
  }

  



  PRUint64 GetPaintCount() { return mPaintCount; }
  void IncrementPaintCount() { ++mPaintCount; }

  


  virtual already_AddRefed<nsPIDOMWindow> GetRootWindow() = 0;

  



  virtual LayerManager* GetLayerManager() = 0;

  




  virtual void SetIgnoreViewportScrolling(PRBool aIgnore) = 0;
  PRBool IgnoringViewportScrolling() const
  { return mRenderFlags & STATE_IGNORING_VIEWPORT_SCROLLING; }

   








  virtual nsresult SetResolution(float aXResolution, float aYResolution) = 0;
  float GetXResolution() { return mXResolution; }
  float GetYResolution() { return mYResolution; }

  




  virtual void SynthesizeMouseMove(PRBool aFromScroll) = 0;

  


protected:
  virtual PRBool AddRefreshObserverExternal(nsARefreshObserver* aObserver,
                                            mozFlushType aFlushType);
  PRBool AddRefreshObserverInternal(nsARefreshObserver* aObserver,
                                    mozFlushType aFlushType);
  virtual PRBool RemoveRefreshObserverExternal(nsARefreshObserver* aObserver,
                                               mozFlushType aFlushType);
  PRBool RemoveRefreshObserverInternal(nsARefreshObserver* aObserver,
                                       mozFlushType aFlushType);
public:
  PRBool AddRefreshObserver(nsARefreshObserver* aObserver,
                            mozFlushType aFlushType) {
#ifdef _IMPL_NS_LAYOUT
    return AddRefreshObserverInternal(aObserver, aFlushType);
#else
    return AddRefreshObserverExternal(aObserver, aFlushType);
#endif
  }

  PRBool RemoveRefreshObserver(nsARefreshObserver* aObserver,
                               mozFlushType aFlushType) {
#ifdef _IMPL_NS_LAYOUT
    return RemoveRefreshObserverInternal(aObserver, aFlushType);
#else
    return RemoveRefreshObserverExternal(aObserver, aFlushType);
#endif
  }

  


  static void InitializeStatics();
  static void ReleaseStatics();

protected:
  friend class nsRefreshDriver;

  
  
  

  
  
  nsIDocument*              mDocument;      
  nsPresContext*            mPresContext;   
  nsStyleSet*               mStyleSet;      
  nsCSSFrameConstructor*    mFrameConstructor; 
  nsIViewManager*           mViewManager;   
  nsFrameSelection*         mSelection;
  nsFrameManagerBase        mFrameManager;  
  nsWeakPtr                 mForwardingContainer;

#ifdef NS_DEBUG
  nsIFrame*                 mDrawEventTargetFrame;
#endif

  
  
  PRUint64                  mPaintCount;

  PRInt16                   mSelectionFlags;

  PRPackedBool              mStylesHaveChanged;
  PRPackedBool              mDidInitialReflow;
  PRPackedBool              mIsDestroying;
  PRPackedBool              mIsReflowing;
  PRPackedBool              mPaintingSuppressed;  
  PRPackedBool              mIsThemeSupportDisabled;  
  PRPackedBool              mIsActive;
  PRPackedBool              mFrozen;

  PRPackedBool              mObservesMutationsForPrint;

  PRPackedBool              mReflowScheduled; 
                                              
                                              
                                              

  PRPackedBool              mSuppressInterruptibleReflows;

  
  nsWeakFrame*              mWeakFrames;

  
  nscolor                   mCanvasBackgroundColor;

  
  
  
  
  
  PRUint32                  mRenderFlags;

  
  
  float                     mXResolution;
  float                     mYResolution;

  
  typedef nsPtrHashKey<nsIPresShell> PresShellPtrKey;
  static nsTHashtable<PresShellPtrKey> *sLiveShells;

  static nsIContent* gKeyDownTarget;
};





nsresult
NS_NewPresShell(nsIPresShell** aInstancePtrResult);

#endif
