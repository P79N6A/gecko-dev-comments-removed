




















































#ifndef nsIPresShell_h___
#define nsIPresShell_h___

#include "nsISupports.h"
#include "nsCoord.h"
#include "nsRect.h"
#include "nsColor.h"
#include "nsEvent.h"
#include "nsCompatibility.h"
#include "nsFrameManagerBase.h"
#include "mozFlushType.h"
#include "nsWeakReference.h"
#include <stdio.h> 

class nsIAtom;
class nsIContent;
class nsIContentIterator;
class nsIDocument;
class nsIDocumentObserver;
class nsIFrame;
class nsPresContext;
class nsStyleSet;
class nsIViewManager;
class nsIDeviceContext;
class nsIRenderingContext;
class nsIPageSequenceFrame;
class nsString;
class nsAString;
class nsStringArray;
class nsICaret;
class nsStyleContext;
class nsFrameSelection;
class nsFrameManager;
class nsILayoutHistoryState;
class nsIReflowCallback;
class nsISupportsArray;
class nsIDOMNode;
class nsIRegion;
class nsIStyleFrameConstruction;
class nsIStyleSheet;
class nsCSSFrameConstructor;
class nsISelection;
template<class E> class nsCOMArray;
class nsWeakFrame;
class nsIScrollableFrame;
class gfxASurface;
class gfxContext;

typedef short SelectionType;
typedef PRUint32 nsFrameState;


#define NS_IPRESSHELL_IID \
{ 0xd93b931b, 0xd5ef, 0x4d3c, \
  { 0xab, 0x99, 0x44, 0x41, 0x76, 0x96, 0x34, 0x64 } }


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
#define VERIFY_REFLOW_INCLUDE_SPACE_MANAGER 0x40
#define VERIFY_REFLOW_DURING_RESIZE_REFLOW  0x80














class nsIPresShell_base : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPRESSHELL_IID)
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPresShell_base, NS_IPRESSHELL_IID)

class nsIPresShell : public nsIPresShell_base
{
public:
  NS_IMETHOD Init(nsIDocument* aDocument,
                  nsPresContext* aPresContext,
                  nsIViewManager* aViewManager,
                  nsStyleSet* aStyleSet,
                  nsCompatibility aCompatMode) = 0;

  






  NS_IMETHOD Destroy() = 0;

  
  
  
  virtual void* AllocateFrame(size_t aSize) = 0;
  virtual void  FreeFrame(size_t aSize, void* aFreeChunk) = 0;

  














  virtual void PushStackMemory() = 0;
  virtual void PopStackMemory() = 0;
  virtual void* AllocateStackMemory(size_t aSize) = 0;
  
  nsIDocument* GetDocument() { return mDocument; }

  nsPresContext* GetPresContext() { return mPresContext; }

  nsIViewManager* GetViewManager() { return mViewManager; }

#ifdef _IMPL_NS_LAYOUT
  nsStyleSet*  StyleSet() { return mStyleSet; }

  nsCSSFrameConstructor* FrameConstructor()
  {
    return mFrameConstructor;
  }

  nsFrameManager* FrameManager() const {
    return reinterpret_cast<nsFrameManager*>
                           (&const_cast<nsIPresShell*>(this)->mFrameManager);
  }

#endif

  


  
  
  NS_HIDDEN_(void) SetAuthorStyleDisabled(PRBool aDisabled);
  NS_HIDDEN_(PRBool) GetAuthorStyleDisabled();

  









  virtual NS_HIDDEN_(void) ReconstructStyleDataExternal();
  NS_HIDDEN_(void) ReconstructStyleDataInternal();
#ifdef _IMPL_NS_LAYOUT
  void ReconstructStyleData() { ReconstructStyleDataInternal(); }
#else
  void ReconstructStyleData() { ReconstructStyleDataExternal(); }
#endif

  







  NS_IMETHOD SetPreferenceStyleRules(PRBool aForceReflow) = 0;

  




  nsFrameSelection* FrameSelection() { return mSelection; }

  
  
  NS_IMETHOD BeginObservingDocument() = 0;

  
  NS_IMETHOD EndObservingDocument() = 0;

  




  NS_IMETHOD GetDidInitialReflow(PRBool *aDidInitialReflow) = 0;

  










  NS_IMETHOD InitialReflow(nscoord aWidth, nscoord aHeight) = 0;

  



  NS_IMETHOD ResizeReflow(nscoord aWidth, nscoord aHeight) = 0;

  


  NS_IMETHOD StyleChangeReflow() = 0;

  




  virtual NS_HIDDEN_(nsIFrame*) GetRootFrame() const;

  


  nsIFrame* GetRootScrollFrame() const;

  


  nsIScrollableFrame* GetRootScrollFrameAsScrollable() const;

  



  NS_IMETHOD GetPageSequenceFrame(nsIPageSequenceFrame** aResult) const = 0;

  













  virtual NS_HIDDEN_(nsIFrame*) GetPrimaryFrameFor(nsIContent* aContent) const = 0;

  





  NS_IMETHOD GetLayoutObjectFor(nsIContent*   aContent,
                                nsISupports** aResult) const = 0;

  



  NS_IMETHOD GetPlaceholderFrameFor(nsIFrame*  aFrame,
                                    nsIFrame** aPlaceholderFrame) const = 0;

  






  enum IntrinsicDirty {
    
    eResize,     
    eTreeChange, 
    eStyleChange 
  };
  NS_IMETHOD FrameNeedsReflow(nsIFrame *aFrame,
                              IntrinsicDirty aIntrinsicDirty,
                              nsFrameState aBitToAdd) = 0;

  NS_IMETHOD CancelAllPendingReflows() = 0;

  


  NS_IMETHOD RecreateFramesFor(nsIContent* aContent) = 0;

  




  NS_IMETHOD IsSafeToFlush(PRBool& aIsSafeToFlush) = 0;

  








  NS_IMETHOD FlushPendingNotifications(mozFlushType aType) = 0;

  



  NS_IMETHOD PostReflowCallback(nsIReflowCallback* aCallback) = 0;
  NS_IMETHOD CancelReflowCallback(nsIReflowCallback* aCallback) = 0;

  NS_IMETHOD ClearFrameRefs(nsIFrame* aFrame) = 0;

  



  NS_IMETHOD CreateRenderingContext(nsIFrame *aFrame,
                                    nsIRenderingContext** aContext) = 0;

  







  NS_IMETHOD GoToAnchor(const nsAString& aAnchorName, PRBool aScroll) = 0;

  




























  NS_IMETHOD ScrollContentIntoView(nsIContent* aContent,
                                   PRIntn      aVPercent,
                                   PRIntn      aHPercent) const = 0;

  



  NS_IMETHOD SetIgnoreFrameDestruction(PRBool aIgnore) = 0;

  




  NS_IMETHOD NotifyDestroyingFrame(nsIFrame* aFrame) = 0;

  


  NS_IMETHOD DoCopy() = 0;

  



  NS_IMETHOD GetSelectionForCopy(nsISelection** outSelection) = 0;

  


  NS_IMETHOD GetLinkLocation(nsIDOMNode* aNode, nsAString& aLocation) = 0;

  


  NS_IMETHOD DoGetContents(const nsACString& aMimeType, PRUint32 aFlags, PRBool aSelectionOnly, nsAString& outValue) = 0;

  


  NS_IMETHOD GetCaret(nsICaret **aOutCaret) = 0;

  




  NS_IMETHOD_(void) MaybeInvalidateCaretPosition() = 0;

  


  virtual already_AddRefed<nsICaret> SetCaret(nsICaret *aNewCaret) = 0;

  








  NS_IMETHOD SetSelectionFlags(PRInt16 aInEnable) = 0;

  






  NS_IMETHOD GetSelectionFlags(PRInt16 *aOutEnabled) = 0;
  
  virtual nsISelection* GetCurrentSelection(SelectionType aType) = 0;

  



  NS_IMETHOD HandleEventWithTarget(nsEvent* aEvent,
                                   nsIFrame* aFrame,
                                   nsIContent* aContent,
                                   nsEventStatus* aStatus) = 0;

  



  NS_IMETHOD HandleDOMEventWithTarget(nsIContent* aTargetContent,
                                      nsEvent* aEvent,
                                      nsEventStatus* aStatus) = 0;

  


  NS_IMETHOD GetEventTargetFrame(nsIFrame** aFrame) = 0;

  


  NS_IMETHOD GetEventTargetContent(nsEvent* aEvent, nsIContent** aContent) = 0;

  



  NS_IMETHOD CaptureHistoryState(nsILayoutHistoryState** aLayoutHistoryState, PRBool aLeavingPage = PR_FALSE) = 0;

  



  NS_IMETHOD IsReflowLocked(PRBool* aIsLocked) = 0;  

  




  NS_IMETHOD IsPaintingSuppressed(PRBool* aResult)=0;

  


  NS_IMETHOD UnsuppressPainting() = 0;

  


  NS_IMETHOD DisableThemeSupport() = 0;

  


  virtual PRBool IsThemeSupportEnabled() = 0;

  


  virtual nsresult GetAgentStyleSheets(nsCOMArray<nsIStyleSheet>& aSheets) = 0;

  


  virtual nsresult SetAgentStyleSheets(const nsCOMArray<nsIStyleSheet>& aSheets) = 0;

  


  virtual nsresult AddOverrideStyleSheet(nsIStyleSheet *aSheet) = 0;

  


  virtual nsresult RemoveOverrideStyleSheet(nsIStyleSheet *aSheet) = 0;

  


  virtual nsresult ReconstructFrames() = 0;

  





  static PRBool GetVerifyReflowEnable();

  


  static void SetVerifyReflowEnable(PRBool aEnabled);

  


  static PRInt32 GetVerifyReflowFlags();

#ifdef MOZ_REFLOW_PERF
  NS_IMETHOD DumpReflows() = 0;
  NS_IMETHOD CountReflows(const char * aName, nsIFrame * aFrame) = 0;
  NS_IMETHOD PaintCount(const char * aName, 
                        nsIRenderingContext* aRenderingContext, 
                        nsPresContext * aPresContext, 
                        nsIFrame * aFrame,
                        PRUint32 aColor) = 0;
  NS_IMETHOD SetPaintFrameCount(PRBool aOn) = 0;
#endif

#ifdef DEBUG
  
  virtual void ListStyleContexts(nsIFrame *aRootFrame, FILE *out,
                                 PRInt32 aIndent = 0) = 0;

  virtual void ListStyleSheets(FILE *out, PRInt32 aIndent = 0) = 0;
  virtual void VerifyStyleTree() = 0;
#endif

  static PRBool gIsAccessibilityActive;
  static PRBool IsAccessibilityActive() { return gIsAccessibilityActive; }

  




  virtual void Freeze() = 0;

  



  virtual void Thaw() = 0;

  





  void SetForwardingContainer(nsWeakPtr aContainer)
  {
    mForwardingContainer = aContainer;
  }
  
  

















  NS_IMETHOD RenderDocument(const nsRect& aRect, PRBool aUntrusted,
                            PRBool aIgnoreViewportScrolling,
                            nscolor aBackgroundColor,
                            gfxContext* aRenderedContext) = 0;

  





  virtual already_AddRefed<gfxASurface> RenderNode(nsIDOMNode* aNode,
                                                   nsIRegion* aRegion,
                                                   nsPoint& aPoint,
                                                   nsRect* aScreenRect) = 0;

  














  virtual already_AddRefed<gfxASurface> RenderSelection(nsISelection* aSelection,
                                                        nsPoint& aPoint,
                                                        nsRect* aScreenRect) = 0;

  void AddWeakFrame(nsWeakFrame* aWeakFrame);
  void RemoveWeakFrame(nsWeakFrame* aWeakFrame);

#ifdef NS_DEBUG
  nsIFrame* GetDrawEventTargetFrame() { return mDrawEventTargetFrame; }
#endif

protected:
  
  
  

  
  
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

  PRPackedBool              mStylesHaveChanged;
  PRPackedBool              mDidInitialReflow;
  PRPackedBool              mIsDestroying;

#ifdef ACCESSIBILITY
  



  void InvalidateAccessibleSubtree(nsIContent *aContent);
#endif

  
  
  PRPackedBool              mIsAccessibilityActive;

  
  nsWeakFrame*              mWeakFrames;
};





nsresult
NS_NewPresShell(nsIPresShell** aInstancePtrResult);

#endif
