







































#ifndef nsIFrame_h___
#define nsIFrame_h___






#include <stdio.h>
#include "nsISupports.h"
#include "nsEvent.h"
#include "nsStyleStruct.h"
#include "nsStyleContext.h"
#include "nsIContent.h"
#include "nsHTMLReflowMetrics.h"


















struct nsHTMLReflowState;
class nsHTMLReflowCommand;

class nsIAtom;
class nsPresContext;
class nsIPresShell;
class nsIRenderingContext;
class nsIView;
class nsIWidget;
class nsIDOMRange;
class nsISelectionController;
class nsBoxLayoutState;
class nsIBoxLayout;
#ifdef ACCESSIBILITY
class nsIAccessible;
#endif
class nsDisplayListBuilder;
class nsDisplayListSet;
class nsDisplayList;

struct nsPeekOffsetStruct;
struct nsPoint;
struct nsRect;
struct nsSize;
struct nsMargin;

typedef class nsIFrame nsIBox;



#define NS_IFRAME_IID \
{ 0x4c0cfb5b, 0x864d, 0x46c5, \
  { 0xad, 0x78, 0xb1, 0xb4, 0xde, 0x35, 0xa4, 0xc3 } }














typedef PRUint32 nsSplittableType;

#define NS_FRAME_NOT_SPLITTABLE             0   // Note: not a bit!
#define NS_FRAME_SPLITTABLE                 0x1
#define NS_FRAME_SPLITTABLE_NON_RECTANGULAR 0x3

#define NS_FRAME_IS_SPLITTABLE(type)\
  (0 != ((type) & NS_FRAME_SPLITTABLE))

#define NS_FRAME_IS_NOT_SPLITTABLE(type)\
  (0 == ((type) & NS_FRAME_SPLITTABLE))

#define NS_INTRINSIC_WIDTH_UNKNOWN nscoord_MIN







typedef PRUint32 nsFrameState;

#define NS_FRAME_IN_REFLOW                            0x00000001

#define NS_FRAME_FORCE_DISPLAY_LIST_DESCEND_INTO      0x00000001




#define NS_FRAME_FIRST_REFLOW                         0x00000002




#define NS_FRAME_IS_FLUID_CONTINUATION                0x00000004



#define NS_FRAME_OUTSIDE_CHILDREN                     0x00000008




#define NS_FRAME_EXTERNAL_REFERENCE                   0x00000010





#define NS_FRAME_CONTAINS_RELATIVE_HEIGHT             0x00000020


#define NS_FRAME_GENERATED_CONTENT                    0x00000040



#define NS_FRAME_IS_BOX                               0x00000080



#define NS_FRAME_OUT_OF_FLOW                          0x00000100


#define NS_FRAME_SELECTED_CONTENT                     0x00000200







#define NS_FRAME_IS_DIRTY                             0x00000400


#define NS_FRAME_IS_UNFLOWABLE                        0x00000800











#define NS_FRAME_HAS_DIRTY_CHILDREN                   0x00001000


#define NS_FRAME_HAS_VIEW                             0x00002000


#define NS_FRAME_INDEPENDENT_SELECTION                0x00004000




#define NS_FRAME_IS_SPECIAL                           0x00008000






#define NS_FRAME_EXCLUDE_IGNORABLE_WHITESPACE         0x00010000

#ifdef IBMBIDI


#define NS_FRAME_IS_BIDI                              0x00020000
#endif


#define NS_FRAME_HAS_CHILD_WITH_VIEW                  0x00040000



#define NS_FRAME_REFLOW_ROOT                          0x00080000


#define NS_FRAME_RESERVED                             0x000FFFFF



#define NS_FRAME_IMPL_RESERVED                        0xFFF00000


#define NS_STATE_IS_HORIZONTAL                        0x00400000
#define NS_STATE_IS_DIRECTION_NORMAL                  0x80000000


#define NS_SUBTREE_DIRTY(_frame)  \
  (((_frame)->GetStateBits() &      \
    (NS_FRAME_IS_DIRTY | NS_FRAME_HAS_DIRTY_CHILDREN)) != 0)



enum nsSelectionAmount {
  eSelectCharacter = 0,
  eSelectWord      = 1,
  eSelectLine      = 2,  
  eSelectBeginLine = 3,
  eSelectEndLine   = 4,
  eSelectNoAmount  = 5,   
  eSelectParagraph = 6    
};

enum nsDirection {
  eDirNext    = 0,
  eDirPrevious= 1
};

enum nsSpread {
  eSpreadNone   = 0,
  eSpreadAcross = 1,
  eSpreadDown   = 2
};


#define NS_CARRIED_TOP_MARGIN_IS_AUTO    0x1
#define NS_CARRIED_BOTTOM_MARGIN_IS_AUTO 0x2






















typedef PRUint32 nsReflowStatus;

#define NS_FRAME_COMPLETE          0            // Note: not a bit!
#define NS_FRAME_NOT_COMPLETE      0x1
#define NS_FRAME_REFLOW_NEXTINFLOW 0x2

#define NS_FRAME_IS_COMPLETE(status) \
  (0 == ((status) & NS_FRAME_NOT_COMPLETE))

#define NS_FRAME_IS_NOT_COMPLETE(status) \
  (0 != ((status) & NS_FRAME_NOT_COMPLETE))



#define NS_IS_REFLOW_ERROR(_status) (PRInt32(_status) < 0)







#define NS_INLINE_BREAK              0x0100




#define NS_INLINE_BREAK_BEFORE       0x0000
#define NS_INLINE_BREAK_AFTER        0x0200


#define NS_INLINE_BREAK_TYPE_MASK    0xF000




#define NS_INLINE_IS_BREAK(_status) \
  (0 != ((_status) & NS_INLINE_BREAK))

#define NS_INLINE_IS_BREAK_AFTER(_status) \
  (0 != ((_status) & NS_INLINE_BREAK_AFTER))

#define NS_INLINE_IS_BREAK_BEFORE(_status) \
  (NS_INLINE_BREAK == ((_status) & (NS_INLINE_BREAK|NS_INLINE_BREAK_AFTER)))

#define NS_INLINE_GET_BREAK_TYPE(_status) (((_status) >> 12) & 0xF)

#define NS_INLINE_MAKE_BREAK_TYPE(_type)  ((_type) << 12)





#define NS_INLINE_LINE_BREAK_BEFORE()                                   \
  (NS_INLINE_BREAK | NS_INLINE_BREAK_BEFORE |                           \
   NS_INLINE_MAKE_BREAK_TYPE(NS_STYLE_CLEAR_LINE))





#define NS_INLINE_LINE_BREAK_AFTER(_completionStatus)                   \
  ((_completionStatus) | NS_INLINE_BREAK | NS_INLINE_BREAK_AFTER |      \
   NS_INLINE_MAKE_BREAK_TYPE(NS_STYLE_CLEAR_LINE))




#define NS_FRAME_TRUNCATED  0x0010
#define NS_FRAME_IS_TRUNCATED(status) \
  (0 != ((status) & NS_FRAME_TRUNCATED))
#define NS_FRAME_SET_TRUNCATION(status, aReflowState, aMetrics) \
  aReflowState.SetTruncated(aMetrics, &status);






typedef PRBool nsDidReflowStatus;

#define NS_FRAME_REFLOW_NOT_FINISHED PR_FALSE
#define NS_FRAME_REFLOW_FINISHED     PR_TRUE
























class nsIFrame : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFRAME_IID)

  nsPresContext* PresContext() const {
    return GetStyleContext()->GetRuleNode()->GetPresContext();
  }

  

















  NS_IMETHOD  Init(nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIFrame*        aPrevInFlow) = 0;

  



  virtual void Destroy() = 0;

  


  virtual void RemovedAsPrimaryFrame() {}

  

















  NS_IMETHOD  SetInitialChildList(nsIAtom*        aListName,
                                  nsIFrame*       aChildList) = 0;

  













  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsIFrame*       aFrameList) = 0;

  














  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsIFrame*       aFrameList) = 0;

  















  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame) = 0;

  


  nsIContent* GetContent() const { return mContent; }

  


  virtual nsIFrame* GetContentInsertionFrame() { return this; }

  



  NS_IMETHOD GetOffsets(PRInt32 &start, PRInt32 &end) const = 0;

  



  virtual void AdjustOffsetsForBidi(PRInt32 aStart, PRInt32 aEnd) {}

  



  nsStyleContext* GetStyleContext() const { return mStyleContext; }
  void SetStyleContext(nsStyleContext* aContext)
  { 
    if (aContext != mStyleContext) {
      if (mStyleContext)
        mStyleContext->Release();
      mStyleContext = aContext;
      if (aContext) {
        aContext->AddRef();
        DidSetStyleContext();
      }
    }
  }
  
  void SetStyleContextWithoutNotification(nsStyleContext* aContext)
  {
    if (aContext != mStyleContext) {
      if (mStyleContext)
        mStyleContext->Release();
      mStyleContext = aContext;
      if (aContext) {
        aContext->AddRef();
      }
    }
  }

  
  NS_IMETHOD DidSetStyleContext() = 0;

  







  virtual const nsStyleStruct* GetStyleDataExternal(nsStyleStructID aSID) const = 0;

  const nsStyleStruct* GetStyleData(nsStyleStructID aSID) const {
#ifdef _IMPL_NS_LAYOUT
    NS_ASSERTION(mStyleContext, "No style context found!");
    return mStyleContext->GetStyleData(aSID);
#else
    return GetStyleDataExternal(aSID);
#endif
  }

  







#ifdef _IMPL_NS_LAYOUT
  #define STYLE_STRUCT(name_, checkdata_cb_, ctor_args_)                      \
    const nsStyle##name_ * GetStyle##name_ () const {                         \
      NS_ASSERTION(mStyleContext, "No style context found!");                 \
      return mStyleContext->GetStyle##name_ ();                               \
    }
#else
  #define STYLE_STRUCT(name_, checkdata_cb_, ctor_args_)                      \
    const nsStyle##name_ * GetStyle##name_ () const {                         \
      return NS_STATIC_CAST(const nsStyle##name_*,                            \
                            GetStyleDataExternal(eStyleStruct_##name_));      \
    }
#endif
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT

  









  virtual nsStyleContext* GetAdditionalStyleContext(PRInt32 aIndex) const = 0;

  virtual void SetAdditionalStyleContext(PRInt32 aIndex,
                                         nsStyleContext* aStyleContext) = 0;

  


  nsIFrame* GetParent() const { return mParent; }
  NS_IMETHOD SetParent(const nsIFrame* aParent) { mParent = (nsIFrame*)aParent; return NS_OK; }

  







  nsRect GetRect() const { return mRect; }
  nsPoint GetPosition() const { return nsPoint(mRect.x, mRect.y); }
  nsSize GetSize() const { return nsSize(mRect.width, mRect.height); }
  void SetRect(const nsRect& aRect) { mRect = aRect; }
  void SetPosition(const nsPoint& aPt) { mRect.MoveTo(aPt); }
  void SetSize(const nsSize& aSize) { mRect.SizeTo(aSize); }

  virtual nsPoint GetPositionOfChildIgnoringScrolling(nsIFrame* aChild)
  { return aChild->GetPosition(); }
  
  nsPoint GetPositionIgnoringScrolling() {
    return mParent ? mParent->GetPositionOfChildIgnoringScrolling(this)
      : GetPosition();
  }

  






  virtual nsMargin GetUsedMargin() const;

  









  virtual nsMargin GetUsedBorder() const;

  




  virtual nsMargin GetUsedPadding() const;

  nsMargin GetUsedBorderAndPadding() const {
    return GetUsedBorder() + GetUsedPadding();
  }

  



  void ApplySkipSides(nsMargin& aMargin) const;

  



  nsRect GetMarginRect() const;
  nsRect GetPaddingRect() const;
  nsRect GetContentRect() const;

  





  virtual nscoord GetBaseline() const = 0;

  







  virtual nsIAtom* GetAdditionalChildListName(PRInt32 aIndex) const = 0;

  







  virtual nsIFrame* GetFirstChild(nsIAtom* aListName) const = 0;

  


  nsIFrame* GetNextSibling() const { return mNextSibling; }
  void SetNextSibling(nsIFrame* aNextSibling) {
    NS_ASSERTION(this != aNextSibling, "Creating a circular frame list, this is very bad."); 
    mNextSibling = aNextSibling;
  }

  













  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) { return NS_OK; }
  





  nsresult DisplayCaret(nsDisplayListBuilder*       aBuilder,
                        const nsRect&               aDirtyRect,
                        const nsDisplayListSet&     aLists);

  PRBool IsThemed() {
    return IsThemed(GetStyleDisplay());
  }
  PRBool IsThemed(const nsStyleDisplay* aDisp) {
    if (!aDisp->mAppearance)
      return PR_FALSE;
    nsPresContext* pc = PresContext();
    nsITheme *theme = pc->GetTheme();
    return theme && theme->ThemeSupportsWidget(pc, this, aDisp->mAppearance);
  }
  
  





  nsresult BuildDisplayListForStackingContext(nsDisplayListBuilder* aBuilder,
                                              const nsRect&         aDirtyRect,
                                              nsDisplayList*        aList);

  








  nsresult OverflowClip(nsDisplayListBuilder*   aBuilder,
                        const nsDisplayListSet& aFromSet,
                        const nsDisplayListSet& aToSet,
                        const nsRect&           aClipRect,
                        PRBool                  aClipBorderBackground = PR_FALSE,
                        PRBool                  aClipAll = PR_FALSE);

  



  nsresult Clip(nsDisplayListBuilder* aBuilder,
                const nsDisplayListSet& aFromSet,
                const nsDisplayListSet& aToSet,
                const nsRect& aClipRect);

  enum {
    DISPLAY_CHILD_FORCE_PSEUDO_STACKING_CONTEXT = 0x01,
    DISPLAY_CHILD_FORCE_STACKING_CONTEXT = 0x02,
    DISPLAY_CHILD_INLINE = 0x04
  };
  








  nsresult BuildDisplayListForChild(nsDisplayListBuilder*   aBuilder,
                                    nsIFrame*               aChild,
                                    const nsRect&           aDirtyRect,
                                    const nsDisplayListSet& aLists,
                                    PRUint32                aFlags = 0);

  


  virtual PRBool NeedsView() { return PR_FALSE; }

  





  virtual nsresult CreateWidgetForView(nsIView* aView);

  
















  NS_IMETHOD  HandleEvent(nsPresContext* aPresContext,
                          nsGUIEvent*     aEvent,
                          nsEventStatus*  aEventStatus) = 0;

  NS_IMETHOD  GetContentForEvent(nsPresContext* aPresContext,
                                 nsEvent* aEvent,
                                 nsIContent** aContent) = 0;

  
  
  
  
  
  
  
  
  
  struct ContentOffsets {
    nsCOMPtr<nsIContent> content;
    PRBool IsNull() { return !content; }
    PRInt32 offset;
    PRInt32 secondaryOffset;
    
    
    PRInt32 StartOffset() { return PR_MIN(offset, secondaryOffset); }
    PRInt32 EndOffset() { return PR_MAX(offset, secondaryOffset); }
    
    
    
    PRBool associateWithNext;
  };
  






  ContentOffsets GetContentOffsetsFromPoint(nsPoint aPoint,
                                            PRBool aIgnoreSelectionStyle = PR_FALSE);

  virtual ContentOffsets GetContentOffsetsFromPointExternal(nsPoint aPoint,
                                                            PRBool aIgnoreSelectionStyle = PR_FALSE)
  { return GetContentOffsetsFromPoint(aPoint, aIgnoreSelectionStyle); }

  




  struct Cursor {
    nsCOMPtr<imgIContainer> mContainer;
    PRInt32                 mCursor;
    PRBool                  mHaveHotspot;
    float                   mHotspotX, mHotspotY;
  };
  


  NS_IMETHOD  GetCursor(const nsPoint&  aPoint,
                        Cursor&         aCursor) = 0;

  




  NS_IMETHOD  GetPointFromOffset(nsPresContext*          inPresContext,
                                 nsIRenderingContext*     inRendContext,
                                 PRInt32                  inOffset,
                                 nsPoint*                 outPoint) = 0;
  
  







  NS_IMETHOD  GetChildFrameContainingOffset(PRInt32       inContentOffset,
                                 PRBool                   inHint,
                                 PRInt32*                 outFrameContentOffset,
                                 nsIFrame*                *outChildFrame) = 0;

 



  nsFrameState GetStateBits() const { return mState; }

  


  void AddStateBits(nsFrameState aBits) { mState |= aBits; }
  void RemoveStateBits(nsFrameState aBits) { mState &= ~aBits; }

  








  NS_IMETHOD  CharacterDataChanged(nsPresContext* aPresContext,
                                   nsIContent*     aChild,
                                   PRBool          aAppend) = 0;

  










  NS_IMETHOD  AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType) = 0;

  


  virtual nsSplittableType GetSplittableType() const = 0;

  


  virtual nsIFrame* GetPrevContinuation() const = 0;
  NS_IMETHOD SetPrevContinuation(nsIFrame*) = 0;
  virtual nsIFrame* GetNextContinuation() const = 0;
  NS_IMETHOD SetNextContinuation(nsIFrame*) = 0;
  virtual nsIFrame* GetFirstContinuation() const {
    return NS_CONST_CAST(nsIFrame*, this);
  }
  virtual nsIFrame* GetLastContinuation() const {
    return NS_CONST_CAST(nsIFrame*, this);
  }
  
  


  virtual nsIFrame* GetPrevInFlowVirtual() const = 0;
  nsIFrame* GetPrevInFlow() const { return GetPrevInFlowVirtual(); }
  NS_IMETHOD SetPrevInFlow(nsIFrame*) = 0;

  virtual nsIFrame* GetNextInFlowVirtual() const = 0;
  nsIFrame* GetNextInFlow() const { return GetNextInFlowVirtual(); }
  NS_IMETHOD SetNextInFlow(nsIFrame*) = 0;

  


  virtual nsIFrame* GetFirstInFlow() const {
    return NS_CONST_CAST(nsIFrame*, this);
  }

  


  virtual nsIFrame* GetLastInFlow() const {
    return NS_CONST_CAST(nsIFrame*, this);
  }


  




  virtual void MarkIntrinsicWidthsDirty() = 0;

  


















  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext) = 0;

  





  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext) = 0;

  






  struct InlineIntrinsicWidthData {
    InlineIntrinsicWidthData()
      : prevLines(0)
      , currentLine(0)
      , skipWhitespace(PR_TRUE)
      , trailingWhitespace(0)
    {}

    
    nscoord prevLines;

    
    
    
    nscoord currentLine;

    
    
    
    PRBool skipWhitespace;

    
    
    nscoord trailingWhitespace;

    
    nsVoidArray floats; 
  };

  struct InlineMinWidthData : public InlineIntrinsicWidthData {
    InlineMinWidthData()
      : trailingTextFrame(nsnull)
    {}

    void Break(nsIRenderingContext *aRenderingContext);

    
    
    
    nsIFrame *trailingTextFrame;
  };

  struct InlinePrefWidthData : public InlineIntrinsicWidthData {
    void Break(nsIRenderingContext *aRenderingContext);
  };

  


















  virtual void
  AddInlineMinWidth(nsIRenderingContext *aRenderingContext,
                    InlineMinWidthData *aData) = 0;

  









  virtual void
  AddInlinePrefWidth(nsIRenderingContext *aRenderingContext,
                     InlinePrefWidthData *aData) = 0;

  



  struct IntrinsicWidthOffsetData {
    nscoord hPadding, hBorder, hMargin;
    float hPctPadding, hPctMargin;

    IntrinsicWidthOffsetData()
      : hPadding(0), hBorder(0), hMargin(0)
      , hPctPadding(0.0f), hPctMargin(0.0f)
    {}
  };
  virtual IntrinsicWidthOffsetData
    IntrinsicWidthOffsets(nsIRenderingContext* aRenderingContext) = 0;

  








  virtual nsSize GetIntrinsicRatio() = 0;

  






































  virtual nsSize ComputeSize(nsIRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             PRBool aShrinkWrap) = 0;

  








  NS_IMETHOD  WillReflow(nsPresContext* aPresContext) = 0;

  










































  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aReflowMetrics,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus) = 0;

  














  NS_IMETHOD  DidReflow(nsPresContext*           aPresContext,
                        const nsHTMLReflowState*  aReflowState,
                        nsDidReflowStatus         aStatus) = 0;

  

  








  virtual PRBool CanContinueTextRun() const = 0;

  
  
  NS_IMETHOD TrimTrailingWhiteSpace(nsPresContext* aPresContext,
                                    nsIRenderingContext& aRC,
                                    nscoord& aDeltaWidth,
                                    PRBool& aLastCharIsJustifiable) = 0;

  




  PRBool HasView() const { return mState & NS_FRAME_HAS_VIEW; }
  nsIView* GetView() const;
  virtual nsIView* GetViewExternal() const;
  nsresult SetView(nsIView* aView);

  




  virtual nsIView* GetParentViewForChildFrame(nsIFrame* aFrame) const;

  




  nsIView* GetClosestView(nsPoint* aOffset = nsnull) const;

  


  nsIFrame* GetAncestorWithView() const;
  virtual nsIFrame* GetAncestorWithViewExternal() const;

  












  nsPoint GetOffsetTo(const nsIFrame* aOther) const;
  virtual nsPoint GetOffsetToExternal(const nsIFrame* aOther) const;

  



  nsIntRect GetScreenRect() const;
  virtual nsIntRect GetScreenRectExternal() const;

  



  NS_IMETHOD  GetOffsetFromView(nsPoint&  aOffset,
                                nsIView** aView) const = 0;

  







  NS_IMETHOD  GetOriginToViewOffset(nsPoint&        aOffset,
                                    nsIView**       aView) const = 0;

  



  virtual PRBool AreAncestorViewsVisible() const;

  






  virtual nsIWidget* GetWindow() const;

  




  virtual nsIAtom* GetType() const = 0;
  

  


  enum {
    eMathML =                           1 << 0,
    eSVG =                              1 << 1,
    eSVGForeignObject =                 1 << 2,
    eBidiInlineContainer =              1 << 3,
    
    eReplaced =                         1 << 4,
    
    
    eReplacedContainsBlock =            1 << 5,
    
    
    eLineParticipant =                  1 << 6,


    
    
    
    eDEBUGAllFrames =                   1 << 30,
    eDEBUGNoFrames =                    1 << 31
  };

  






  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
#ifdef DEBUG
    return !(aFlags & ~(nsIFrame::eDEBUGAllFrames));
#else
    return !aFlags;
#endif
  }

  


  virtual PRBool IsContainingBlock() const = 0;

  



  virtual PRBool IsFloatContainingBlock() const { return PR_FALSE; }

  








  virtual PRBool IsLeaf() const;

  





  virtual nsIView* GetMouseCapturer() const { return nsnull; }

  









  void Invalidate(const nsRect& aDamageRect, PRBool aImmediate = PR_FALSE);

  













  
  virtual void InvalidateInternal(const nsRect& aDamageRect,
                                  nscoord aOffsetX, nscoord aOffsetY,
                                  nsIFrame* aForChild, PRBool aImmediate);

  







  nsRect GetOverflowRect() const;

  




  void FinishAndStoreOverflow(nsRect* aOverflowArea, nsSize aNewSize);

  void FinishAndStoreOverflow(nsHTMLReflowMetrics* aMetrics) {
    FinishAndStoreOverflow(&aMetrics->mOverflowArea, nsSize(aMetrics->width, aMetrics->height));
  }

  



  virtual PRIntn GetSkipSides() const { return 0; }

  

  







  NS_IMETHOD  SetSelected(nsPresContext* aPresContext,
                          nsIDOMRange*    aRange,
                          PRBool          aSelected,
                          nsSpread        aSpread) = 0;

  NS_IMETHOD  GetSelected(PRBool *aSelected) const = 0;

  








  NS_IMETHOD  IsSelectable(PRBool* aIsSelectable, PRUint8* aSelectStyle) const = 0;

  




  NS_IMETHOD  GetSelectionController(nsPresContext *aPresContext, nsISelectionController **aSelCon) = 0;

  


  nsFrameSelection* GetFrameSelection();

  


  





  NS_IMETHOD CaptureMouse(nsPresContext* aPresContext, PRBool aGrabMouseEvents) = 0;

  






  NS_IMETHOD PeekOffset(nsPeekOffsetStruct *aPos);

  










  nsresult GetFrameFromDirection(nsDirection aDirection, PRBool aVisual,
                                 PRBool aJumpLines, PRBool aScrollViewStop, 
                                 nsIFrame** aOutFrame, PRInt32* aOutOffset, PRBool* aOutJumpedLine);

  










  NS_IMETHOD CheckVisibility(nsPresContext* aContext, PRInt32 aStartIndex, PRInt32 aEndIndex, PRBool aRecurse, PRBool *aFinished, PRBool *_retval)=0;

  





  virtual void ChildIsDirty(nsIFrame* aChild) = 0;

  






#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible) = 0;
#endif

  
















  NS_IMETHOD GetParentStyleContextFrame(nsPresContext* aPresContext,
                                        nsIFrame**      aProviderFrame,
                                        PRBool*         aIsChild) = 0;

  



  PRBool IsVisibleForPainting(nsDisplayListBuilder* aBuilder);
  



  PRBool IsVisibleOrCollapsedForPainting(nsDisplayListBuilder* aBuilder);
  



  PRBool IsVisibleForPainting();
  



  PRBool IsVisibleInSelection(nsDisplayListBuilder* aBuilder);

  


  
  virtual PRBool IsVisibleInSelection(nsISelection* aSelection);

  




  PRBool IsPseudoStackingContextFromStyle() {
    const nsStyleDisplay* disp = GetStyleDisplay();
    return disp->mOpacity != 1.0f || disp->IsPositioned();
  }
  
  virtual PRBool HonorPrintBackgroundSettings() { return PR_TRUE; }

  








  virtual PRBool IsEmpty() = 0;
  



  virtual PRBool CachedIsEmpty();
  



  virtual PRBool IsSelfEmpty() = 0;

  





  PRBool IsGeneratedContentFrame() {
    return (mState & NS_FRAME_GENERATED_CONTENT) != 0;
  }

  






   
  PRBool IsPseudoFrame(nsIContent* aParentContent) {
    return mContent == aParentContent;
  }


  NS_HIDDEN_(void*) GetProperty(nsIAtom* aPropertyName,
                                nsresult* aStatus = nsnull) const;
  virtual NS_HIDDEN_(void*) GetPropertyExternal(nsIAtom*  aPropertyName,
                                                nsresult* aStatus) const;
  NS_HIDDEN_(nsresult) SetProperty(nsIAtom*           aPropertyName,
                                   void*              aValue,
                                   NSPropertyDtorFunc aDestructor = nsnull,
                                   void*              aDtorData = nsnull);
  NS_HIDDEN_(nsresult) DeleteProperty(nsIAtom* aPropertyName) const;
  NS_HIDDEN_(void*) UnsetProperty(nsIAtom* aPropertyName,
                                  nsresult* aStatus = nsnull) const;

#define NS_GET_BASE_LEVEL(frame) \
NS_PTR_TO_INT32(frame->GetProperty(nsGkAtoms::baseLevel))

#define NS_GET_EMBEDDING_LEVEL(frame) \
NS_PTR_TO_INT32(frame->GetProperty(nsGkAtoms::embeddingLevel))

  





  nsRect* GetOverflowAreaProperty(PRBool aCreateIfNecessary = PR_FALSE);

  




  virtual PRBool SupportsVisibilityHidden() { return PR_TRUE; }

  





  PRBool GetAbsPosClipRect(const nsStyleDisplay* aDisp, nsRect* aRect,
                           const nsSize& aSize);

  

















  virtual PRBool IsFocusable(PRInt32 *aTabIndex = nsnull, PRBool aWithMouse = PR_FALSE);

  
  
  
  PRBool IsBoxFrame() const { return (mState & NS_FRAME_IS_BOX) != 0; }
  PRBool IsBoxWrapped() const
  { return (!IsBoxFrame() && mParent && mParent->IsBoxFrame()); }

  enum Halignment {
    hAlign_Left,
    hAlign_Right,
    hAlign_Center
  };

  enum Valignment {
    vAlign_Top,
    vAlign_Middle,
    vAlign_BaseLine,
    vAlign_Bottom
  };

  




  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState) = 0;

  




  virtual nsSize GetPrefSize(nsBoxLayoutState& aBoxLayoutState) = 0;

  



    
  virtual nsSize GetMaxSize(nsBoxLayoutState& aBoxLayoutState) = 0;

  



  virtual nsSize GetMinSizeForScrollArea(nsBoxLayoutState& aBoxLayoutState) = 0;

  
  PRUint32 GetOrdinal(nsBoxLayoutState& aBoxLayoutState);

  virtual nscoord GetFlex(nsBoxLayoutState& aBoxLayoutState) = 0;
  virtual nscoord GetBoxAscent(nsBoxLayoutState& aBoxLayoutState) = 0;
  virtual PRBool IsCollapsed(nsBoxLayoutState& aBoxLayoutState) = 0;
  
  
  
  
  
  virtual void SetBounds(nsBoxLayoutState& aBoxLayoutState, const nsRect& aRect,
                         PRBool aRemoveOverflowArea = PR_FALSE)=0;
  NS_HIDDEN_(nsresult) Layout(nsBoxLayoutState& aBoxLayoutState);
  nsIBox* GetChildBox() const
  {
    
    
    return IsBoxFrame() ? GetFirstChild(nsnull) : nsnull;
  }
  nsIBox* GetNextBox() const
  {
    return (mParent && mParent->IsBoxFrame()) ? mNextSibling : nsnull;
  }
  nsIBox* GetParentBox() const
  {
    return (mParent && mParent->IsBoxFrame()) ? mParent : nsnull;
  }
  
  
  NS_IMETHOD GetBorderAndPadding(nsMargin& aBorderAndPadding);
  NS_IMETHOD GetBorder(nsMargin& aBorder)=0;
  NS_IMETHOD GetPadding(nsMargin& aBorderAndPadding)=0;
  NS_IMETHOD GetMargin(nsMargin& aMargin)=0;
  NS_IMETHOD SetLayoutManager(nsIBoxLayout* aLayout)=0;
  NS_IMETHOD GetLayoutManager(nsIBoxLayout** aLayout)=0;
  NS_HIDDEN_(nsresult) GetClientRect(nsRect& aContentRect);

  
  virtual Valignment GetVAlign() const = 0;
  virtual Halignment GetHAlign() const = 0;

  PRBool IsHorizontal() const { return (mState & NS_STATE_IS_HORIZONTAL) != 0; }
  PRBool IsNormalDirection() const { return (mState & NS_STATE_IS_DIRECTION_NORMAL) != 0; }

  NS_HIDDEN_(nsresult) Redraw(nsBoxLayoutState& aState, const nsRect* aRect = nsnull, PRBool aImmediate = PR_FALSE);
  NS_IMETHOD RelayoutChildAtOrdinal(nsBoxLayoutState& aState, nsIBox* aChild)=0;
  virtual PRBool GetMouseThrough() const = 0;

#ifdef DEBUG_LAYOUT
  NS_IMETHOD SetDebug(nsBoxLayoutState& aState, PRBool aDebug)=0;
  NS_IMETHOD GetDebug(PRBool& aDebug)=0;

  NS_IMETHOD DumpBox(FILE* out)=0;
#endif

  
  virtual PRBool ChildrenMustHaveWidgets() const { return PR_FALSE; }

  



  virtual PRBool HasTerminalNewline() const;

  static PRBool AddCSSPrefSize(nsBoxLayoutState& aState, nsIBox* aBox, nsSize& aSize);
  static PRBool AddCSSMinSize(nsBoxLayoutState& aState, nsIBox* aBox, nsSize& aSize);
  static PRBool AddCSSMaxSize(nsBoxLayoutState& aState, nsIBox* aBox, nsSize& aSize);
  static PRBool AddCSSFlex(nsBoxLayoutState& aState, nsIBox* aBox, nscoord& aFlex);
  static PRBool AddCSSCollapsed(nsBoxLayoutState& aState, nsIBox* aBox, PRBool& aCollapsed);
  static PRBool AddCSSOrdinal(nsBoxLayoutState& aState, nsIBox* aBox, PRUint32& aOrdinal);

  
  
  

  










  nsPeekOffsetStruct GetExtremeCaretPosition(PRBool aStart);

protected:
  
  nsRect           mRect;
  nsIContent*      mContent;
  nsStyleContext*  mStyleContext;
  nsIFrame*        mParent;
  nsIFrame*        mNextSibling;  
  nsFrameState     mState;
  
  
  



  void InvalidateRoot(const nsRect& aDamageRect,
                      nscoord aOffsetX, nscoord aOffsetY,
                      PRBool aImmediate);

  








  virtual PRBool PeekOffsetNoAmount(PRBool aForward, PRInt32* aOffset) = 0;
  
  








  virtual PRBool PeekOffsetCharacter(PRBool aForward, PRInt32* aOffset) = 0;
  
  


















  virtual PRBool PeekOffsetWord(PRBool aForward, PRBool aWordSelectEatSpace, PRBool aIsKeyboardSelect,
                                PRInt32* aOffset, PRBool* aSawBeforeType) = 0;

  






   nsresult PeekOffsetParagraph(nsPeekOffsetStruct *aPos);

private:
  NS_IMETHOD_(nsrefcnt) AddRef(void) = 0;
  NS_IMETHOD_(nsrefcnt) Release(void) = 0;
};
















class nsWeakFrame {
public:
  nsWeakFrame(nsIFrame* aFrame) : mPrev(nsnull), mFrame(nsnull)
  {
    Init(aFrame);
  }

  nsWeakFrame& operator=(nsWeakFrame& aOther) {
    Init(aOther.GetFrame());
    return *this;
  }

  nsWeakFrame& operator=(nsIFrame* aFrame) {
    Init(aFrame);
    return *this;
  }

  nsIFrame* operator->()
  {
    return mFrame;
  }

  operator nsIFrame*()
  {
    return mFrame;
  }

  void Clear(nsIPresShell* aShell) {
    if (aShell) {
      aShell->RemoveWeakFrame(this);
    }
    mFrame = nsnull;
    mPrev = nsnull;
  }

  PRBool IsAlive() { return !!mFrame; }

  nsIFrame* GetFrame() { return mFrame; }

  nsWeakFrame* GetPreviousWeakFrame() { return mPrev; }

  void SetPreviousWeakFrame(nsWeakFrame* aPrev) { mPrev = aPrev; }

  ~nsWeakFrame()
  {
    Clear(mFrame ? mFrame->PresContext()->GetPresShell() : nsnull);
  }
private:
  void Init(nsIFrame* aFrame);

  nsWeakFrame*  mPrev;
  nsIFrame*     mFrame;
};


NS_DEFINE_STATIC_IID_ACCESSOR(nsIFrame, NS_IFRAME_IID)

#endif 
