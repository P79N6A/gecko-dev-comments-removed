







































#ifndef nsIFrame_h___
#define nsIFrame_h___

#ifndef MOZILLA_INTERNAL_API
#error This header/class should only be used within Mozilla code. It should not be used by extensions.
#endif






#include <stdio.h>
#include "nsQueryFrame.h"
#include "nsEvent.h"
#include "nsStyleStruct.h"
#include "nsStyleContext.h"
#include "nsIContent.h"
#include "nsHTMLReflowMetrics.h"
#include "gfxMatrix.h"
#include "nsFrameList.h"
#include "nsAlgorithm.h"
#include "FramePropertyTable.h"


















struct nsHTMLReflowState;
class nsHTMLReflowCommand;

class nsIAtom;
class nsPresContext;
class nsIPresShell;
class nsRenderingContext;
class nsIView;
class nsIWidget;
class nsIDOMRange;
class nsISelectionController;
class nsBoxLayoutState;
class nsIBoxLayout;
class nsILineIterator;
#ifdef ACCESSIBILITY
class nsAccessible;
#endif
class nsDisplayListBuilder;
class nsDisplayListSet;
class nsDisplayList;
class gfxSkipChars;
class gfxSkipCharsIterator;
class gfxContext;
class nsLineList_iterator;
class nsAbsoluteContainingBlock;

struct nsPeekOffsetStruct;
struct nsPoint;
struct nsRect;
struct nsSize;
struct nsMargin;
struct CharacterDataChangeInfo;

namespace mozilla {
namespace layers {
class Layer;
}
}

typedef class nsIFrame nsIBox;














typedef PRUint32 nsSplittableType;

#define NS_FRAME_NOT_SPLITTABLE             0   // Note: not a bit!
#define NS_FRAME_SPLITTABLE                 0x1
#define NS_FRAME_SPLITTABLE_NON_RECTANGULAR 0x3

#define NS_FRAME_IS_SPLITTABLE(type)\
  (0 != ((type) & NS_FRAME_SPLITTABLE))

#define NS_FRAME_IS_NOT_SPLITTABLE(type)\
  (0 == ((type) & NS_FRAME_SPLITTABLE))

#define NS_INTRINSIC_WIDTH_UNKNOWN nscoord_MIN







typedef PRUint64 nsFrameState;

#define NS_FRAME_STATE_BIT(n_) (nsFrameState(1) << (n_))

#define NS_FRAME_IN_REFLOW                          NS_FRAME_STATE_BIT(0)


#define NS_FRAME_FORCE_DISPLAY_LIST_DESCEND_INTO    NS_FRAME_STATE_BIT(0)




#define NS_FRAME_FIRST_REFLOW                       NS_FRAME_STATE_BIT(1)




#define NS_FRAME_IS_FLUID_CONTINUATION              NS_FRAME_STATE_BIT(2)



#define NS_FRAME_HAS_CONTAINER_LAYER                NS_FRAME_STATE_BIT(3)




#define NS_FRAME_EXTERNAL_REFERENCE                 NS_FRAME_STATE_BIT(4)





#define  NS_FRAME_CONTAINS_RELATIVE_HEIGHT          NS_FRAME_STATE_BIT(5)


#define NS_FRAME_GENERATED_CONTENT                  NS_FRAME_STATE_BIT(6)






#define NS_FRAME_IS_OVERFLOW_CONTAINER              NS_FRAME_STATE_BIT(7)



#define NS_FRAME_OUT_OF_FLOW                        NS_FRAME_STATE_BIT(8)


#define NS_FRAME_SELECTED_CONTENT                   NS_FRAME_STATE_BIT(9)







#define NS_FRAME_IS_DIRTY                           NS_FRAME_STATE_BIT(10)




#define NS_FRAME_TOO_DEEP_IN_FRAME_TREE             NS_FRAME_STATE_BIT(11)











#define NS_FRAME_HAS_DIRTY_CHILDREN                 NS_FRAME_STATE_BIT(12)


#define NS_FRAME_HAS_VIEW                           NS_FRAME_STATE_BIT(13)


#define NS_FRAME_INDEPENDENT_SELECTION              NS_FRAME_STATE_BIT(14)






#define NS_FRAME_IS_SPECIAL                         NS_FRAME_STATE_BIT(15)





#define  NS_FRAME_MAY_BE_TRANSFORMED                NS_FRAME_STATE_BIT(16)

#ifdef IBMBIDI


#define NS_FRAME_IS_BIDI                            NS_FRAME_STATE_BIT(17)
#endif


#define NS_FRAME_HAS_CHILD_WITH_VIEW                NS_FRAME_STATE_BIT(18)



#define NS_FRAME_REFLOW_ROOT                        NS_FRAME_STATE_BIT(19)


#define NS_FRAME_IMPL_RESERVED                      nsFrameState(0xF0000000FFF00000)





#define NS_FRAME_IS_PUSHED_FLOAT                    NS_FRAME_STATE_BIT(32)


#define NS_FRAME_DRAWING_AS_PAINTSERVER             NS_FRAME_STATE_BIT(33)



#define NS_FRAME_HAS_CONTAINER_LAYER_DESCENDANT     NS_FRAME_STATE_BIT(34)


#define NS_FRAME_HAS_CLIP                           NS_FRAME_STATE_BIT(35)





#define NS_FRAME_UPDATE_LAYER_TREE                  NS_FRAME_STATE_BIT(36)


#define NS_FRAME_HAS_ABSPOS_CHILDREN                NS_FRAME_STATE_BIT(37)



#define NS_FRAME_RESERVED                           ~NS_FRAME_IMPL_RESERVED


#define NS_STATE_IS_HORIZONTAL                      NS_FRAME_STATE_BIT(22)
#define NS_STATE_IS_DIRECTION_NORMAL                NS_FRAME_STATE_BIT(31)


#define NS_SUBTREE_DIRTY(_frame)  \
  (((_frame)->GetStateBits() &      \
    (NS_FRAME_IS_DIRTY | NS_FRAME_HAS_DIRTY_CHILDREN)) != 0)



enum nsSelectionAmount {
  eSelectCharacter = 0, 
                        
                        
  eSelectCluster   = 1, 
                        
                        
  eSelectWord      = 2,
  eSelectLine      = 3, 
  eSelectBeginLine = 4,
  eSelectEndLine   = 5,
  eSelectNoAmount  = 6, 
  eSelectParagraph = 7,  
  eSelectWordNoSpace = 8 
                         
                         
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

#define NS_FRAME_COMPLETE             0       // Note: not a bit!
#define NS_FRAME_NOT_COMPLETE         0x1
#define NS_FRAME_REFLOW_NEXTINFLOW    0x2
#define NS_FRAME_OVERFLOW_INCOMPLETE  0x4

#define NS_FRAME_IS_COMPLETE(status) \
  (0 == ((status) & NS_FRAME_NOT_COMPLETE))

#define NS_FRAME_IS_NOT_COMPLETE(status) \
  (0 != ((status) & NS_FRAME_NOT_COMPLETE))

#define NS_FRAME_OVERFLOW_IS_INCOMPLETE(status) \
  (0 != ((status) & NS_FRAME_OVERFLOW_INCOMPLETE))

#define NS_FRAME_IS_FULLY_COMPLETE(status) \
  (NS_FRAME_IS_COMPLETE(status) && !NS_FRAME_OVERFLOW_IS_INCOMPLETE(status))



#define NS_FRAME_SET_INCOMPLETE(status) \
  status = (status & ~NS_FRAME_OVERFLOW_INCOMPLETE) | NS_FRAME_NOT_COMPLETE

#define NS_FRAME_SET_OVERFLOW_INCOMPLETE(status) \
  status = (status & ~NS_FRAME_NOT_COMPLETE) | NS_FRAME_OVERFLOW_INCOMPLETE



#define NS_IS_REFLOW_ERROR(_status) (PRInt32(_status) < 0)







#define NS_INLINE_BREAK              0x0100




#define NS_INLINE_BREAK_BEFORE       0x0000
#define NS_INLINE_BREAK_AFTER        0x0200


#define NS_INLINE_BREAK_TYPE_MASK    0xF000


#define NS_INLINE_BREAK_FIRST_LETTER_COMPLETE 0x10000




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



void NS_MergeReflowStatusInto(nsReflowStatus* aPrimary,
                              nsReflowStatus aSecondary);






typedef PRBool nsDidReflowStatus;

#define NS_FRAME_REFLOW_NOT_FINISHED PR_FALSE
#define NS_FRAME_REFLOW_FINISHED     PR_TRUE










#define NS_FRAME_OVERFLOW_DELTA_MAX     0xfe // max delta we can store

#define NS_FRAME_OVERFLOW_NONE    0x00000000 // there are no overflow rects;
                                             
                                             

#define NS_FRAME_OVERFLOW_LARGE   0x000000ff // overflow is stored as a
                                             
























class nsIFrame : public nsQueryFrame
{
public:
  typedef mozilla::FramePropertyDescriptor FramePropertyDescriptor;
  typedef mozilla::FrameProperties FrameProperties;
  typedef mozilla::layers::Layer Layer;

  NS_DECL_QUERYFRAME_TARGET(nsIFrame)

  nsPresContext* PresContext() const {
    return GetStyleContext()->GetRuleNode()->GetPresContext();
  }

  















  NS_IMETHOD  Init(nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIFrame*        aPrevInFlow) = 0;

  







  void Destroy() { DestroyFrom(this); }

protected:
  




  virtual void DestroyFrom(nsIFrame* aDestructRoot) = 0;
  friend class nsFrameList; 
  friend class nsLineBox;   
public:

  



















  NS_IMETHOD  SetInitialChildList(nsIAtom*        aListName,
                                  nsFrameList&    aChildList) = 0;

  















  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsFrameList&    aFrameList) = 0;

  
















  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList) = 0;

  















  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame) = 0;

  


  nsIContent* GetContent() const { return mContent; }

  



  virtual nsIFrame* GetContentInsertionFrame() { return this; }

  






  virtual nsIScrollableFrame* GetScrollTargetFrame() { return nsnull; }

  



  NS_IMETHOD GetOffsets(PRInt32 &start, PRInt32 &end) const = 0;

  



  virtual void AdjustOffsetsForBidi(PRInt32 aStart, PRInt32 aEnd) {}

  



  nsStyleContext* GetStyleContext() const { return mStyleContext; }
  void SetStyleContext(nsStyleContext* aContext)
  { 
    if (aContext != mStyleContext) {
      nsStyleContext* oldStyleContext = mStyleContext;
      mStyleContext = aContext;
      if (aContext) {
        aContext->AddRef();
        DidSetStyleContext(oldStyleContext);
      }
      if (oldStyleContext)
        oldStyleContext->Release();
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

  
  
  
  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) = 0;

  







  virtual const void* GetStyleDataExternal(nsStyleStructID aSID) const = 0;

  







#ifdef _IMPL_NS_LAYOUT
  #define STYLE_STRUCT(name_, checkdata_cb_, ctor_args_)                      \
    const nsStyle##name_ * GetStyle##name_ () const {                         \
      NS_ASSERTION(mStyleContext, "No style context found!");                 \
      return mStyleContext->GetStyle##name_ ();                               \
    }
#else
  #define STYLE_STRUCT(name_, checkdata_cb_, ctor_args_)                      \
    const nsStyle##name_ * GetStyle##name_ () const {                         \
      return static_cast<const nsStyle##name_*>(                              \
                            GetStyleDataExternal(eStyleStruct_##name_));      \
    }
#endif
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT

#ifdef _IMPL_NS_LAYOUT
  
  nscolor GetVisitedDependentColor(nsCSSProperty aProperty)
    { return mStyleContext->GetVisitedDependentColor(aProperty); }
#endif

  









  virtual nsStyleContext* GetAdditionalStyleContext(PRInt32 aIndex) const = 0;

  virtual void SetAdditionalStyleContext(PRInt32 aIndex,
                                         nsStyleContext* aStyleContext) = 0;

  

                 
  PRBool HasBorder() const;

  


  nsIFrame* GetParent() const { return mParent; }
  virtual void SetParent(nsIFrame* aParent) = 0;

  







  nsRect GetRect() const { return mRect; }
  nsPoint GetPosition() const { return nsPoint(mRect.x, mRect.y); }
  nsSize GetSize() const { return nsSize(mRect.width, mRect.height); }

  





  void SetRect(const nsRect& aRect) {
    if (mOverflow.mType != NS_FRAME_OVERFLOW_LARGE &&
        mOverflow.mType != NS_FRAME_OVERFLOW_NONE) {
      nsOverflowAreas overflow = GetOverflowAreas();
      mRect = aRect;
      SetOverflowAreas(overflow);
    } else {
      mRect = aRect;
    }
  }
  void SetSize(const nsSize& aSize) {
    SetRect(nsRect(mRect.TopLeft(), aSize));
  }
  void SetPosition(const nsPoint& aPt) { mRect.MoveTo(aPt); }

  


  nsPoint GetRelativeOffset(const nsStyleDisplay* aDisplay = nsnull) const;

  virtual nsPoint GetPositionOfChildIgnoringScrolling(nsIFrame* aChild)
  { return aChild->GetPosition(); }
  
  nsPoint GetPositionIgnoringScrolling() {
    return mParent ? mParent->GetPositionOfChildIgnoringScrolling(this)
      : GetPosition();
  }

  static void DestroyRegion(void* aPropertyValue)
  {
    delete static_cast<nsRegion*>(aPropertyValue);
  }

  static void DestroyMargin(void* aPropertyValue)
  {
    delete static_cast<nsMargin*>(aPropertyValue);
  }

  static void DestroyRect(void* aPropertyValue)
  {
    delete static_cast<nsRect*>(aPropertyValue);
  }

  static void DestroyPoint(void* aPropertyValue)
  {
    delete static_cast<nsPoint*>(aPropertyValue);
  }

  static void DestroyOverflowAreas(void* aPropertyValue)
  {
    delete static_cast<nsOverflowAreas*>(aPropertyValue);
  }

#ifdef _MSC_VER


#define NS_PROPERTY_DESCRIPTOR_CONST
#else
#define NS_PROPERTY_DESCRIPTOR_CONST const
#endif

#define NS_DECLARE_FRAME_PROPERTY(prop, dtor)                                                  \
  static const FramePropertyDescriptor* prop() {                                               \
    static NS_PROPERTY_DESCRIPTOR_CONST FramePropertyDescriptor descriptor = { dtor, nsnull }; \
    return &descriptor;                                                                        \
  }

#define NS_DECLARE_FRAME_PROPERTY_WITH_FRAME_IN_DTOR(prop, dtor)                               \
  static const FramePropertyDescriptor* prop() {                                               \
    static NS_PROPERTY_DESCRIPTOR_CONST FramePropertyDescriptor descriptor = { nsnull, dtor }; \
    return &descriptor;                                                                        \
  }

  NS_DECLARE_FRAME_PROPERTY(IBSplitSpecialSibling, nsnull)
  NS_DECLARE_FRAME_PROPERTY(IBSplitSpecialPrevSibling, nsnull)

  NS_DECLARE_FRAME_PROPERTY(ComputedOffsetProperty, DestroyPoint)

  NS_DECLARE_FRAME_PROPERTY(OutlineInnerRectProperty, DestroyRect)
  NS_DECLARE_FRAME_PROPERTY(PreEffectsBBoxProperty, DestroyRect)
  NS_DECLARE_FRAME_PROPERTY(PreTransformBBoxProperty, DestroyRect)

  NS_DECLARE_FRAME_PROPERTY(UsedMarginProperty, DestroyMargin)
  NS_DECLARE_FRAME_PROPERTY(UsedPaddingProperty, DestroyMargin)
  NS_DECLARE_FRAME_PROPERTY(UsedBorderProperty, DestroyMargin)

  NS_DECLARE_FRAME_PROPERTY(ScrollLayerCount, nsnull)

  










  virtual nsMargin GetUsedMargin() const;

  









  virtual nsMargin GetUsedBorder() const;

  




  virtual nsMargin GetUsedPadding() const;

  nsMargin GetUsedBorderAndPadding() const {
    return GetUsedBorder() + GetUsedPadding();
  }

  



  void ApplySkipSides(nsMargin& aMargin) const;

  



  nsRect GetPaddingRect() const;
  nsRect GetPaddingRectRelativeToSelf() const;
  nsRect GetContentRect() const;
  nsRect GetContentRectRelativeToSelf() const;

  

















  static PRBool ComputeBorderRadii(const nsStyleCorners& aBorderRadius,
                                   const nsSize& aFrameSize,
                                   const nsSize& aBorderArea,
                                   PRIntn aSkipSides,
                                   nscoord aRadii[8]);

  












  static void InsetBorderRadii(nscoord aRadii[8], const nsMargin &aOffsets);
  static void OutsetBorderRadii(nscoord aRadii[8], const nsMargin &aOffsets);

  





  virtual PRBool GetBorderRadii(nscoord aRadii[8]) const;

  PRBool GetPaddingBoxBorderRadii(nscoord aRadii[8]) const;
  PRBool GetContentBoxBorderRadii(nscoord aRadii[8]) const;

  




  virtual nscoord GetBaseline() const = 0;

  





  virtual nscoord GetCaretBaseline() const {
    return GetBaseline();
  }

  







  virtual nsIAtom* GetAdditionalChildListName(PRInt32 aIndex) const = 0;

  








  
  
  
  
  virtual nsFrameList GetChildList(nsIAtom* aListName) const = 0;
  
  nsIFrame* GetFirstChild(nsIAtom* aListName) const {
    return GetChildList(aListName).FirstChild();
  }
  
  nsIFrame* GetLastChild(nsIAtom* aListName) const {
    return GetChildList(aListName).LastChild();
  }

  


  nsIFrame* GetNextSibling() const { return mNextSibling; }
  void SetNextSibling(nsIFrame* aNextSibling) {
    NS_ASSERTION(this != aNextSibling, "Creating a circular frame list, this is very bad.");
    if (mNextSibling && mNextSibling->GetPrevSibling() == this) {
      mNextSibling->mPrevSibling = nsnull;
    }
    mNextSibling = aNextSibling;
    if (mNextSibling) {
      mNextSibling->mPrevSibling = this;
    }
  }

  nsIFrame* GetPrevSibling() const { return mPrevSibling; }

  













  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) { return NS_OK; }
  





  nsresult DisplayCaret(nsDisplayListBuilder*       aBuilder,
                        const nsRect&               aDirtyRect,
                        nsDisplayList*              aList);

  




  virtual nscolor GetCaretColorAt(PRInt32 aOffset);

 
  PRBool IsThemed(nsITheme::Transparency* aTransparencyState = nsnull) const {
    return IsThemed(GetStyleDisplay(), aTransparencyState);
  }
  PRBool IsThemed(const nsStyleDisplay* aDisp,
                  nsITheme::Transparency* aTransparencyState = nsnull) const {
    nsIFrame* mutable_this = const_cast<nsIFrame*>(this);
    if (!aDisp->mAppearance)
      return PR_FALSE;
    nsPresContext* pc = PresContext();
    nsITheme *theme = pc->GetTheme();
    if(!theme ||
       !theme->ThemeSupportsWidget(pc, mutable_this, aDisp->mAppearance))
      return PR_FALSE;
    if (aTransparencyState) {
      *aTransparencyState =
        theme->GetWidgetTransparency(mutable_this, aDisp->mAppearance);
    }
    return PR_TRUE;
  }
  
  





  nsresult BuildDisplayListForStackingContext(nsDisplayListBuilder* aBuilder,
                                              const nsRect&         aDirtyRect,
                                              nsDisplayList*        aList);

  










  nsresult OverflowClip(nsDisplayListBuilder*   aBuilder,
                        const nsDisplayListSet& aFromSet,
                        const nsDisplayListSet& aToSet,
                        const nsRect&           aClipRect,
                        const nscoord           aClipRadii[8],
                        PRBool                  aClipBorderBackground = PR_FALSE,
                        PRBool                  aClipAll = PR_FALSE);

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

  




  void WrapReplacedContentForBorderRadius(nsDisplayListBuilder* aBuilder,
                                          nsDisplayList* aFromList,
                                          const nsDisplayListSet& aToLists);

  


  virtual PRBool NeedsView() { return PR_FALSE; }

  



  virtual PRBool IsTransformed() const;

  
















  NS_IMETHOD  HandleEvent(nsPresContext* aPresContext,
                          nsGUIEvent*     aEvent,
                          nsEventStatus*  aEventStatus) = 0;

  NS_IMETHOD  GetContentForEvent(nsPresContext* aPresContext,
                                 nsEvent* aEvent,
                                 nsIContent** aContent) = 0;

  
  
  
  
  
  
  
  
  
  struct NS_STACK_CLASS ContentOffsets {
    nsCOMPtr<nsIContent> content;
    PRBool IsNull() { return !content; }
    PRInt32 offset;
    PRInt32 secondaryOffset;
    
    
    PRInt32 StartOffset() { return NS_MIN(offset, secondaryOffset); }
    PRInt32 EndOffset() { return NS_MAX(offset, secondaryOffset); }
    
    
    
    PRBool associateWithNext;
  };
  






  ContentOffsets GetContentOffsetsFromPoint(nsPoint aPoint,
                                            PRBool aIgnoreSelectionStyle = PR_FALSE);

  virtual ContentOffsets GetContentOffsetsFromPointExternal(nsPoint aPoint,
                                                            PRBool aIgnoreSelectionStyle = PR_FALSE)
  { return GetContentOffsetsFromPoint(aPoint, aIgnoreSelectionStyle); }

  




  struct NS_STACK_CLASS Cursor {
    nsCOMPtr<imgIContainer> mContainer;
    PRInt32                 mCursor;
    PRBool                  mHaveHotspot;
    float                   mHotspotX, mHotspotY;
  };
  


  NS_IMETHOD  GetCursor(const nsPoint&  aPoint,
                        Cursor&         aCursor) = 0;

  




  NS_IMETHOD  GetPointFromOffset(PRInt32                  inOffset,
                                 nsPoint*                 outPoint) = 0;
  
  







  NS_IMETHOD  GetChildFrameContainingOffset(PRInt32       inContentOffset,
                                 PRBool                   inHint,
                                 PRInt32*                 outFrameContentOffset,
                                 nsIFrame*                *outChildFrame) = 0;

 



  nsFrameState GetStateBits() const { return mState; }

  


  void AddStateBits(nsFrameState aBits) { mState |= aBits; }
  void RemoveStateBits(nsFrameState aBits) { mState &= ~aBits; }

  



  NS_IMETHOD  CharacterDataChanged(CharacterDataChangeInfo* aInfo) = 0;

  










  NS_IMETHOD  AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType) = 0;

  





  virtual void ContentStatesChanged(nsEventStates aStates) { };

  


  virtual nsSplittableType GetSplittableType() const = 0;

  


  virtual nsIFrame* GetPrevContinuation() const = 0;
  NS_IMETHOD SetPrevContinuation(nsIFrame*) = 0;
  virtual nsIFrame* GetNextContinuation() const = 0;
  NS_IMETHOD SetNextContinuation(nsIFrame*) = 0;
  virtual nsIFrame* GetFirstContinuation() const {
    return const_cast<nsIFrame*>(this);
  }
  virtual nsIFrame* GetLastContinuation() const {
    return const_cast<nsIFrame*>(this);
  }

  




  nsIFrame* GetTailContinuation();

  


  virtual nsIFrame* GetPrevInFlowVirtual() const = 0;
  nsIFrame* GetPrevInFlow() const { return GetPrevInFlowVirtual(); }
  NS_IMETHOD SetPrevInFlow(nsIFrame*) = 0;

  virtual nsIFrame* GetNextInFlowVirtual() const = 0;
  nsIFrame* GetNextInFlow() const { return GetNextInFlowVirtual(); }
  NS_IMETHOD SetNextInFlow(nsIFrame*) = 0;

  


  virtual nsIFrame* GetFirstInFlow() const {
    return const_cast<nsIFrame*>(this);
  }

  


  virtual nsIFrame* GetLastInFlow() const {
    return const_cast<nsIFrame*>(this);
  }


  




  virtual void MarkIntrinsicWidthsDirty() = 0;

  




















  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext) = 0;

  





  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext) = 0;

  






  struct InlineIntrinsicWidthData {
    InlineIntrinsicWidthData()
      : line(nsnull)
      , lineContainer(nsnull)
      , prevLines(0)
      , currentLine(0)
      , skipWhitespace(PR_TRUE)
      , trailingWhitespace(0)
    {}

    
    
    const nsLineList_iterator* line;

    
    nsIFrame* lineContainer;

    
    nscoord prevLines;

    
    
    
    nscoord currentLine;

    
    
    
    PRBool skipWhitespace;

    
    
    nscoord trailingWhitespace;

    
    nsTArray<nsIFrame*> floats;
  };

  struct InlineMinWidthData : public InlineIntrinsicWidthData {
    InlineMinWidthData()
      : trailingTextFrame(nsnull)
      , atStartOfLine(PR_TRUE)
    {}

    
    
    
    
    void ForceBreak(nsRenderingContext *aRenderingContext);

    
    
    void OptionallyBreak(nsRenderingContext *aRenderingContext,
                         nscoord aHyphenWidth = 0);

    
    
    
    nsIFrame *trailingTextFrame;

    
    
    
    PRBool atStartOfLine;
  };

  struct InlinePrefWidthData : public InlineIntrinsicWidthData {
    void ForceBreak(nsRenderingContext *aRenderingContext);
  };

  


















  virtual void
  AddInlineMinWidth(nsRenderingContext *aRenderingContext,
                    InlineMinWidthData *aData) = 0;

  









  virtual void
  AddInlinePrefWidth(nsRenderingContext *aRenderingContext,
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
    IntrinsicWidthOffsets(nsRenderingContext* aRenderingContext) = 0;

  







  struct IntrinsicSize {
    nsStyleCoord width, height;

    IntrinsicSize()
      : width(eStyleUnit_None), height(eStyleUnit_None)
    {}
    IntrinsicSize(const IntrinsicSize& rhs)
      : width(rhs.width), height(rhs.height)
    {}
    IntrinsicSize& operator=(const IntrinsicSize& rhs) {
      width = rhs.width; height = rhs.height; return *this;
    }
    PRBool operator==(const IntrinsicSize& rhs) {
      return width == rhs.width && height == rhs.height;
    }
    PRBool operator!=(const IntrinsicSize& rhs) {
      return !(*this == rhs);
    }
  };
  virtual IntrinsicSize GetIntrinsicSize() = 0;

  








  virtual nsSize GetIntrinsicRatio() = 0;

  






































  virtual nsSize ComputeSize(nsRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             PRBool aShrinkWrap) = 0;

  












  virtual nsRect ComputeTightBounds(gfxContext* aContext) const;

  








  NS_IMETHOD  WillReflow(nsPresContext* aPresContext) = 0;

  










































  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aReflowMetrics,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus) = 0;

  














  NS_IMETHOD  DidReflow(nsPresContext*           aPresContext,
                        const nsHTMLReflowState*  aReflowState,
                        nsDidReflowStatus         aStatus) = 0;

  

  








  virtual PRBool CanContinueTextRun() const = 0;

  














  virtual nsresult GetRenderedText(nsAString* aAppendToString = nsnull,
                                   gfxSkipChars* aSkipChars = nsnull,
                                   gfxSkipCharsIterator* aSkipIter = nsnull,
                                   PRUint32 aSkippedStartOffset = 0,
                                   PRUint32 aSkippedMaxLength = PR_UINT32_MAX)
  { return NS_ERROR_NOT_IMPLEMENTED; }

  




  virtual PRBool HasAnyNoncollapsedCharacters()
  { return PR_FALSE; }

  




  PRBool HasView() const { return !!(mState & NS_FRAME_HAS_VIEW); }
  nsIView* GetView() const;
  virtual nsIView* GetViewExternal() const;
  nsresult SetView(nsIView* aView);

  




  nsIView* GetClosestView(nsPoint* aOffset = nsnull) const;

  


  nsIFrame* GetAncestorWithView() const;
  virtual nsIFrame* GetAncestorWithViewExternal() const;

  















  nsPoint GetOffsetTo(const nsIFrame* aOther) const;
  virtual nsPoint GetOffsetToExternal(const nsIFrame* aOther) const;

  



















  nsPoint GetOffsetToCrossDoc(const nsIFrame* aOther) const;

  



  nsIntRect GetScreenRect() const;
  virtual nsIntRect GetScreenRectExternal() const;

  



  nsRect GetScreenRectInAppUnits() const;
  virtual nsRect GetScreenRectInAppUnitsExternal() const;

  



  NS_IMETHOD  GetOffsetFromView(nsPoint&  aOffset,
                                nsIView** aView) const = 0;

  



  virtual PRBool AreAncestorViewsVisible() const;

  






  virtual nsIWidget* GetNearestWidget() const;

  




  virtual nsIWidget* GetNearestWidget(nsPoint& aOffset) const;

  




  virtual nsIAtom* GetType() const = 0;

  










  virtual gfxMatrix GetTransformMatrix(nsIFrame **aOutAncestor);

  


  enum {
    eMathML =                           1 << 0,
    eSVG =                              1 << 1,
    eSVGForeignObject =                 1 << 2,
    eSVGContainer =                     1 << 3,
    eSVGGeometry =                      1 << 4,
    eSVGPaintServer =                   1 << 5,
    eBidiInlineContainer =              1 << 6,
    
    eReplaced =                         1 << 7,
    
    
    eReplacedContainsBlock =            1 << 8,
    
    
    eLineParticipant =                  1 << 9,
    eXULBox =                           1 << 10,
    eCanContainOverflowContainers =     1 << 11,
    eBlockFrame =                       1 << 12,
    
    
    
    eExcludesIgnorableWhitespace =      1 << 13,

    
    
    
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

  










  void BeginDeferringInvalidatesForDisplayRoot(const nsRegion& aExcludeRegion);

  


  void EndDeferringInvalidatesForDisplayRoot();

  





  void MarkLayersActive();

  


  PRBool AreLayersMarkedActive();
  
  


  void InvalidateWithFlags(const nsRect& aDamageRect, PRUint32 aFlags);

  









  void Invalidate(const nsRect& aDamageRect)
  { return InvalidateWithFlags(aDamageRect, 0); }

#ifndef MOZ_ENABLE_LIBXUL
  


  virtual void InvalidateOverflowRectExternal()
  { return InvalidateOverflowRect(); }
#endif

  








  Layer* InvalidateLayer(const nsRect& aDamageRect, PRUint32 aDisplayItemKey);

  




  void InvalidateTransformLayer();

  












































  enum {
    INVALIDATE_IMMEDIATE = 0x01,
    INVALIDATE_CROSS_DOC = 0x02,
    INVALIDATE_REASON_SCROLL_BLIT = 0x04,
    INVALIDATE_REASON_SCROLL_REPAINT = 0x08,
    INVALIDATE_REASON_MASK = INVALIDATE_REASON_SCROLL_BLIT |
                             INVALIDATE_REASON_SCROLL_REPAINT,
    INVALIDATE_NO_THEBES_LAYERS = 0x10,
    INVALIDATE_ONLY_THEBES_LAYERS = 0x20,
    INVALIDATE_EXCLUDE_CURRENT_PAINT = 0x40,
    INVALIDATE_NO_UPDATE_LAYER_TREE = 0x80
  };
  virtual void InvalidateInternal(const nsRect& aDamageRect,
                                  nscoord aOffsetX, nscoord aOffsetY,
                                  nsIFrame* aForChild, PRUint32 aFlags);

  










  void InvalidateInternalAfterResize(const nsRect& aDamageRect, nscoord aX,
                                     nscoord aY, PRUint32 aFlags);

  







  void InvalidateRectDifference(const nsRect& aR1, const nsRect& aR2);

  





  void InvalidateFrameSubtree();

  




  void InvalidateOverflowRect();

  





















  nsRect GetVisualOverflowRect() const {
    return GetOverflowRect(eVisualOverflow);
  }

  



















  nsRect GetScrollableOverflowRect() const {
    return GetOverflowRect(eScrollableOverflow);
  }

  nsRect GetOverflowRect(nsOverflowType aType) const;

  nsOverflowAreas GetOverflowAreas() const;

  






  nsRect GetScrollableOverflowRectRelativeToParent() const;

  






  nsRect GetVisualOverflowRectRelativeToSelf() const;

  




  void FinishAndStoreOverflow(nsOverflowAreas& aOverflowAreas,
                              nsSize aNewSize);

  void FinishAndStoreOverflow(nsHTMLReflowMetrics* aMetrics) {
    FinishAndStoreOverflow(aMetrics->mOverflowAreas,
                           nsSize(aMetrics->width, aMetrics->height));
  }

  



  PRBool HasOverflowAreas() const {
    return mOverflow.mType != NS_FRAME_OVERFLOW_NONE;
  }

  


  void ClearOverflowRects();

  



  virtual PRIntn GetSkipSides() const { return 0; }

  

  












  virtual void SetSelected(PRBool        aSelected,
                           SelectionType aType);

  NS_IMETHOD  GetSelected(PRBool *aSelected) const = 0;

  








  NS_IMETHOD  IsSelectable(PRBool* aIsSelectable, PRUint8* aSelectStyle) const = 0;

  




  NS_IMETHOD  GetSelectionController(nsPresContext *aPresContext, nsISelectionController **aSelCon) = 0;

  


  already_AddRefed<nsFrameSelection> GetFrameSelection();

  



  const nsFrameSelection* GetConstFrameSelection();

  


  






  NS_IMETHOD PeekOffset(nsPeekOffsetStruct *aPos);

  










  nsresult GetFrameFromDirection(nsDirection aDirection, PRBool aVisual,
                                 PRBool aJumpLines, PRBool aScrollViewStop, 
                                 nsIFrame** aOutFrame, PRInt32* aOutOffset, PRBool* aOutJumpedLine);

  










  NS_IMETHOD CheckVisibility(nsPresContext* aContext, PRInt32 aStartIndex, PRInt32 aEndIndex, PRBool aRecurse, PRBool *aFinished, PRBool *_retval)=0;

  





  virtual void ChildIsDirty(nsIFrame* aChild) = 0;

  






#ifdef ACCESSIBILITY
  virtual already_AddRefed<nsAccessible> CreateAccessible() = 0;
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
    return disp->mOpacity != 1.0f || disp->IsPositioned() || disp->IsFloating();
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

  FrameProperties Properties() const {
    return FrameProperties(PresContext()->PropertyTable(), this);
  }

  NS_DECLARE_FRAME_PROPERTY(BaseLevelProperty, nsnull)
  NS_DECLARE_FRAME_PROPERTY(EmbeddingLevelProperty, nsnull)

#define NS_GET_BASE_LEVEL(frame) \
NS_PTR_TO_INT32(frame->Properties().Get(nsIFrame::BaseLevelProperty()))

#define NS_GET_EMBEDDING_LEVEL(frame) \
NS_PTR_TO_INT32(frame->Properties().Get(nsIFrame::EmbeddingLevelProperty()))

  




  virtual PRBool SupportsVisibilityHidden() { return PR_TRUE; }

  





  PRBool GetAbsPosClipRect(const nsStyleDisplay* aDisp, nsRect* aRect,
                           const nsSize& aSize);

  

















  virtual PRBool IsFocusable(PRInt32 *aTabIndex = nsnull, PRBool aWithMouse = PR_FALSE);

  
  
  
  PRBool IsBoxFrame() const
  {
    return IsFrameOfType(nsIFrame::eXULBox);
  }
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
                         PRBool aRemoveOverflowAreas = PR_FALSE) = 0;
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

  NS_HIDDEN_(nsresult) Redraw(nsBoxLayoutState& aState, const nsRect* aRect = nsnull);
  NS_IMETHOD RelayoutChildAtOrdinal(nsBoxLayoutState& aState, nsIBox* aChild)=0;
  
  virtual PRBool GetMouseThrough() const { return PR_FALSE; };

#ifdef DEBUG_LAYOUT
  NS_IMETHOD SetDebug(nsBoxLayoutState& aState, PRBool aDebug)=0;
  NS_IMETHOD GetDebug(PRBool& aDebug)=0;

  NS_IMETHOD DumpBox(FILE* out)=0;
#endif

  



  virtual PRBool HasTerminalNewline() const;

  static PRBool AddCSSPrefSize(nsIBox* aBox, nsSize& aSize, PRBool& aWidth, PRBool& aHeightSet);
  static PRBool AddCSSMinSize(nsBoxLayoutState& aState, nsIBox* aBox,
                              nsSize& aSize, PRBool& aWidth, PRBool& aHeightSet);
  static PRBool AddCSSMaxSize(nsIBox* aBox, nsSize& aSize, PRBool& aWidth, PRBool& aHeightSet);
  static PRBool AddCSSFlex(nsBoxLayoutState& aState, nsIBox* aBox, nscoord& aFlex);

  
  
  

  










  nsPeekOffsetStruct GetExtremeCaretPosition(PRBool aStart);

  






  void CheckInvalidateSizeChange(const nsRect& aOldRect,
                                 const nsRect& aOldVisualOverflowRect,
                                 const nsSize& aNewDesiredSize);

  





  virtual nsILineIterator* GetLineIterator() = 0;

  



  virtual void PullOverflowsFromPrevInFlow() {}

  


  
  void ClearPresShellsFromLastPaint() { 
    PaintedPresShellList()->Clear(); 
  }
  
  


  
  void AddPaintedPresShell(nsIPresShell* shell) { 
    PaintedPresShellList()->AppendElement(do_GetWeakReference(shell)); 
  }
  
  


  
  void UpdatePaintCountForPaintedPresShells() {
    nsTArray<nsWeakPtr> * list = PaintedPresShellList();
    for (int i = 0, l = list->Length(); i < l; i++) {
      nsCOMPtr<nsIPresShell> shell = do_QueryReferent(list->ElementAt(i));
      
      if (shell) {
        shell->IncrementPaintCount();
      }
    }
  }  

  


  PRBool IsAbsoluteContainer() const { return !!(mState & NS_FRAME_HAS_ABSPOS_CHILDREN); }
  PRBool HasAbsolutelyPositionedChildren() const;
  nsAbsoluteContainingBlock* GetAbsoluteContainingBlock() const;
  virtual void MarkAsAbsoluteContainingBlock();
  
  virtual nsIAtom* GetAbsoluteListName() const { return nsGkAtoms::absoluteList; }

protected:
  
  nsRect           mRect;
  nsIContent*      mContent;
  nsStyleContext*  mStyleContext;
  nsIFrame*        mParent;
private:
  nsIFrame*        mNextSibling;  
  nsIFrame*        mPrevSibling;  

  static void DestroyPaintedPresShellList(void* propertyValue) {
    nsTArray<nsWeakPtr>* list = static_cast<nsTArray<nsWeakPtr>*>(propertyValue);
    list->Clear();
    delete list;
  }

  
  
  
  NS_DECLARE_FRAME_PROPERTY(PaintedPresShellsProperty, DestroyPaintedPresShellList)
  
  nsTArray<nsWeakPtr>* PaintedPresShellList() {
    nsTArray<nsWeakPtr>* list = static_cast<nsTArray<nsWeakPtr>*>(
      Properties().Get(PaintedPresShellsProperty())
    );
    
    if (!list) {
      list = new nsTArray<nsWeakPtr>();
      Properties().Set(PaintedPresShellsProperty(), list);
    }
    
    return list;
  }

protected:
  nsFrameState     mState;

  
  
  
  
  
  
  
  
  
  union {
    PRUint32  mType;
    struct {
      PRUint8 mLeft;
      PRUint8 mTop;
      PRUint8 mRight;
      PRUint8 mBottom;
    } mVisualDeltas;
  } mOverflow;

  
  



  void InvalidateRoot(const nsRect& aDamageRect, PRUint32 aFlags);

  








  virtual PRBool PeekOffsetNoAmount(PRBool aForward, PRInt32* aOffset) = 0;
  
  











  virtual PRBool PeekOffsetCharacter(PRBool aForward, PRInt32* aOffset,
                                     PRBool aRespectClusters = PR_TRUE) = 0;
  
  

















  struct PeekWordState {
    
    
    PRPackedBool mAtStart;
    
    
    PRPackedBool mSawBeforeType;
    
    PRPackedBool mLastCharWasPunctuation;
    
    PRPackedBool mLastCharWasWhitespace;
    
    PRPackedBool mSeenNonPunctuationSinceWhitespace;
    
    
    
    nsAutoString mContext;

    PeekWordState() : mAtStart(PR_TRUE), mSawBeforeType(PR_FALSE),
        mLastCharWasPunctuation(PR_FALSE), mLastCharWasWhitespace(PR_FALSE),
        mSeenNonPunctuationSinceWhitespace(PR_FALSE) {}
    void SetSawBeforeType() { mSawBeforeType = PR_TRUE; }
    void Update(PRBool aAfterPunctuation, PRBool aAfterWhitespace) {
      mLastCharWasPunctuation = aAfterPunctuation;
      mLastCharWasWhitespace = aAfterWhitespace;
      if (aAfterWhitespace) {
        mSeenNonPunctuationSinceWhitespace = PR_FALSE;
      } else if (!aAfterPunctuation) {
        mSeenNonPunctuationSinceWhitespace = PR_TRUE;
      }
      mAtStart = PR_FALSE;
    }
  };
  virtual PRBool PeekOffsetWord(PRBool aForward, PRBool aWordSelectEatSpace, PRBool aIsKeyboardSelect,
                                PRInt32* aOffset, PeekWordState* aState) = 0;

  






   nsresult PeekOffsetParagraph(nsPeekOffsetStruct *aPos);

private:
  nsOverflowAreas* GetOverflowAreasProperty();
  nsRect GetVisualOverflowFromDeltas() const {
    NS_ABORT_IF_FALSE(mOverflow.mType != NS_FRAME_OVERFLOW_LARGE,
                      "should not be called when overflow is in a property");
    
    
    
    
    return nsRect(-(PRInt32)mOverflow.mVisualDeltas.mLeft,
                  -(PRInt32)mOverflow.mVisualDeltas.mTop,
                  mRect.width + mOverflow.mVisualDeltas.mRight +
                                mOverflow.mVisualDeltas.mLeft,
                  mRect.height + mOverflow.mVisualDeltas.mBottom +
                                 mOverflow.mVisualDeltas.mTop);
  }
  void SetOverflowAreas(const nsOverflowAreas& aOverflowAreas);
  nsPoint GetOffsetToCrossDoc(const nsIFrame* aOther, const PRInt32 aAPD) const;

#ifdef NS_DEBUG
public:
  
  NS_IMETHOD  List(FILE* out, PRInt32 aIndent) const = 0;
  NS_IMETHOD  GetFrameName(nsAString& aResult) const = 0;
  NS_IMETHOD_(nsFrameState)  GetDebugStateBits() const = 0;
  NS_IMETHOD  DumpRegressionData(nsPresContext* aPresContext,
                                 FILE* out, PRInt32 aIndent) = 0;
#endif
};
















class nsWeakFrame {
public:
  nsWeakFrame() : mPrev(nsnull), mFrame(nsnull) { }

  nsWeakFrame(const nsWeakFrame& aOther) : mPrev(nsnull), mFrame(nsnull)
  {
    Init(aOther.GetFrame());
  }

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

  nsIFrame* GetFrame() const { return mFrame; }

  nsWeakFrame* GetPreviousWeakFrame() { return mPrev; }

  void SetPreviousWeakFrame(nsWeakFrame* aPrev) { mPrev = aPrev; }

  ~nsWeakFrame()
  {
    Clear(mFrame ? mFrame->PresContext()->GetPresShell() : nsnull);
  }
private:
  void InitInternal(nsIFrame* aFrame);

  void InitExternal(nsIFrame* aFrame) {
    Clear(mFrame ? mFrame->PresContext()->GetPresShell() : nsnull);
    mFrame = aFrame;
    if (mFrame) {
      nsIPresShell* shell = mFrame->PresContext()->GetPresShell();
      NS_WARN_IF_FALSE(shell, "Null PresShell in nsWeakFrame!");
      if (shell) {
        shell->AddWeakFrame(this);
      } else {
        mFrame = nsnull;
      }
    }
  }

  void Init(nsIFrame* aFrame) {
#ifdef _IMPL_NS_LAYOUT
    InitInternal(aFrame);
#else
    InitExternal(aFrame);
#endif
  }

  nsWeakFrame*  mPrev;
  nsIFrame*     mFrame;
};

inline void
nsFrameList::Enumerator::Next()
{
  NS_ASSERTION(!AtEnd(), "Should have checked AtEnd()!");
  mFrame = mFrame->GetNextSibling();
}

inline
nsFrameList::FrameLinkEnumerator::
FrameLinkEnumerator(const nsFrameList& aList, nsIFrame* aPrevFrame)
  : Enumerator(aList)
{
  mPrev = aPrevFrame;
  mFrame = aPrevFrame ? aPrevFrame->GetNextSibling() : aList.FirstChild();
}
#endif 
