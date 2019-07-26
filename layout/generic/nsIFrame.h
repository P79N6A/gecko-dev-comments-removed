







#ifndef nsIFrame_h___
#define nsIFrame_h___

#ifndef MOZILLA_INTERNAL_API
#error This header/class should only be used within Mozilla code. It should not be used by extensions.
#endif

#define MAX_REFLOW_DEPTH 200






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
#include "mozilla/layout/FrameChildList.h"
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
class nsBoxLayout;
class nsILineIterator;
#ifdef ACCESSIBILITY
class Accessible;
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




#define NS_FRAME_FIRST_REFLOW                       NS_FRAME_STATE_BIT(1)




#define NS_FRAME_IS_FLUID_CONTINUATION              NS_FRAME_STATE_BIT(2)



#define NS_FRAME_HAS_CONTAINER_LAYER                NS_FRAME_STATE_BIT(3)




#define NS_FRAME_EXTERNAL_REFERENCE                 NS_FRAME_STATE_BIT(4)





#define  NS_FRAME_CONTAINS_RELATIVE_HEIGHT          NS_FRAME_STATE_BIT(5)


#define NS_FRAME_GENERATED_CONTENT                  NS_FRAME_STATE_BIT(6)






#define NS_FRAME_IS_OVERFLOW_CONTAINER              NS_FRAME_STATE_BIT(7)



#define NS_FRAME_OUT_OF_FLOW                        NS_FRAME_STATE_BIT(8)










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
#define NS_FRAME_RESERVED                           ~NS_FRAME_IMPL_RESERVED





#define NS_FRAME_IS_PUSHED_FLOAT                    NS_FRAME_STATE_BIT(32)


#define NS_FRAME_DRAWING_AS_PAINTSERVER             NS_FRAME_STATE_BIT(33)



#define NS_FRAME_HAS_CONTAINER_LAYER_DESCENDANT     NS_FRAME_STATE_BIT(34)


#define NS_FRAME_HAS_CLIP                           NS_FRAME_STATE_BIT(35)





#define NS_FRAME_UPDATE_LAYER_TREE                  NS_FRAME_STATE_BIT(36)


#define NS_FRAME_HAS_ABSPOS_CHILDREN                NS_FRAME_STATE_BIT(37)


#define NS_FRAME_PAINTED_THEBES                     NS_FRAME_STATE_BIT(38)




#define NS_FRAME_IN_CONSTRAINED_HEIGHT              NS_FRAME_STATE_BIT(39)


#define NS_FRAME_FORCE_DISPLAY_LIST_DESCEND_INTO    NS_FRAME_STATE_BIT(40)




#define NS_FRAME_FONT_INFLATION_CONTAINER           NS_FRAME_STATE_BIT(41)




#define NS_FRAME_FONT_INFLATION_FLOW_ROOT           NS_FRAME_STATE_BIT(42)




#define NS_FRAME_SVG_LAYOUT                         NS_FRAME_STATE_BIT(43)


#define NS_FRAME_MAY_HAVE_GENERATED_CONTENT         NS_FRAME_STATE_BIT(44)




#define NS_FRAME_NO_COMPONENT_ALPHA                 NS_FRAME_STATE_BIT(45)



#define NS_FRAME_HAS_CACHED_BACKGROUND              NS_FRAME_STATE_BIT(46)


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






typedef bool nsDidReflowStatus;

#define NS_FRAME_REFLOW_NOT_FINISHED false
#define NS_FRAME_REFLOW_FINISHED     true










#define NS_FRAME_OVERFLOW_DELTA_MAX     0xfe // max delta we can store

#define NS_FRAME_OVERFLOW_NONE    0x00000000 // there are no overflow rects;
                                             
                                             

#define NS_FRAME_OVERFLOW_LARGE   0x000000ff // overflow is stored as a
                                             
























class nsIFrame : public nsQueryFrame
{
public:
  typedef mozilla::FramePropertyDescriptor FramePropertyDescriptor;
  typedef mozilla::FrameProperties FrameProperties;
  typedef mozilla::layers::Layer Layer;
  typedef mozilla::layout::FrameChildList ChildList;
  typedef mozilla::layout::FrameChildListID ChildListID;
  typedef mozilla::layout::FrameChildListIDs ChildListIDs;
  typedef mozilla::layout::FrameChildListIterator ChildListIterator;
  typedef mozilla::layout::FrameChildListArrayIterator ChildListArrayIterator;

  NS_DECL_QUERYFRAME_TARGET(nsIFrame)

  nsPresContext* PresContext() const {
    return GetStyleContext()->GetRuleNode()->GetPresContext();
  }

  















  NS_IMETHOD  Init(nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIFrame*        aPrevInFlow) = 0;

  







  void Destroy() { DestroyFrom(this); }

protected:
  



  virtual bool IsFrameSelected() const;

  








  virtual void DestroyFrom(nsIFrame* aDestructRoot) = 0;
  friend class nsFrameList; 
  friend class nsLineBox;   
public:

  




















  NS_IMETHOD  SetInitialChildList(ChildListID     aListID,
                                  nsFrameList&    aChildList) = 0;

  














  NS_IMETHOD AppendFrames(ChildListID     aListID,
                          nsFrameList&    aFrameList) = 0;

  















  NS_IMETHOD InsertFrames(ChildListID     aListID,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList) = 0;

  














  NS_IMETHOD RemoveFrame(ChildListID     aListID,
                         nsIFrame*       aOldFrame) = 0;

  


  nsIContent* GetContent() const { return mContent; }

  



  virtual nsIFrame* GetContentInsertionFrame() { return this; }

  






  virtual nsIScrollableFrame* GetScrollTargetFrame() { return nullptr; }

  



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

  


  nsPoint GetRelativeOffset(const nsStyleDisplay* aDisplay = nullptr) const;

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

  static void DestroySurface(void* aPropertyValue)
  {
    static_cast<gfxASurface*>(aPropertyValue)->Release();
  }

#ifdef _MSC_VER


#define NS_PROPERTY_DESCRIPTOR_CONST
#else
#define NS_PROPERTY_DESCRIPTOR_CONST const
#endif

#define NS_DECLARE_FRAME_PROPERTY(prop, dtor)                                                  \
  static const FramePropertyDescriptor* prop() {                                               \
    static NS_PROPERTY_DESCRIPTOR_CONST FramePropertyDescriptor descriptor = { dtor, nullptr }; \
    return &descriptor;                                                                        \
  }

#define NS_DECLARE_FRAME_PROPERTY_WITH_FRAME_IN_DTOR(prop, dtor)                               \
  static const FramePropertyDescriptor* prop() {                                               \
    static NS_PROPERTY_DESCRIPTOR_CONST FramePropertyDescriptor descriptor = { nullptr, dtor }; \
    return &descriptor;                                                                        \
  }

  NS_DECLARE_FRAME_PROPERTY(IBSplitSpecialSibling, nullptr)
  NS_DECLARE_FRAME_PROPERTY(IBSplitSpecialPrevSibling, nullptr)

  NS_DECLARE_FRAME_PROPERTY(ComputedOffsetProperty, DestroyPoint)

  NS_DECLARE_FRAME_PROPERTY(OutlineInnerRectProperty, DestroyRect)
  NS_DECLARE_FRAME_PROPERTY(PreEffectsBBoxProperty, DestroyRect)
  NS_DECLARE_FRAME_PROPERTY(PreTransformOverflowAreasProperty,
                            DestroyOverflowAreas)

  
  
  
  NS_DECLARE_FRAME_PROPERTY(InitialOverflowProperty, DestroyOverflowAreas)

  NS_DECLARE_FRAME_PROPERTY(UsedMarginProperty, DestroyMargin)
  NS_DECLARE_FRAME_PROPERTY(UsedPaddingProperty, DestroyMargin)
  NS_DECLARE_FRAME_PROPERTY(UsedBorderProperty, DestroyMargin)

  NS_DECLARE_FRAME_PROPERTY(ScrollLayerCount, nullptr)

  NS_DECLARE_FRAME_PROPERTY(LineBaselineOffset, nullptr)

  NS_DECLARE_FRAME_PROPERTY(CachedBackgroundImage, DestroySurface)

  










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

  

















  static bool ComputeBorderRadii(const nsStyleCorners& aBorderRadius,
                                   const nsSize& aFrameSize,
                                   const nsSize& aBorderArea,
                                   PRIntn aSkipSides,
                                   nscoord aRadii[8]);

  












  static void InsetBorderRadii(nscoord aRadii[8], const nsMargin &aOffsets);
  static void OutsetBorderRadii(nscoord aRadii[8], const nsMargin &aOffsets);

  





  virtual bool GetBorderRadii(nscoord aRadii[8]) const;

  bool GetPaddingBoxBorderRadii(nscoord aRadii[8]) const;
  bool GetContentBoxBorderRadii(nscoord aRadii[8]) const;

  




  virtual nscoord GetBaseline() const = 0;

  





  virtual nscoord GetCaretBaseline() const {
    return GetBaseline();
  }

  






  virtual const nsFrameList& GetChildList(ChildListID aListID) const = 0;
  const nsFrameList& PrincipalChildList() { return GetChildList(kPrincipalList); }
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const = 0;
  
  nsIFrame* GetFirstChild(ChildListID aListID) const {
    return GetChildList(aListID).FirstChild();
  }
  
  nsIFrame* GetLastChild(ChildListID aListID) const {
    return GetChildList(aListID).LastChild();
  }
  nsIFrame* GetFirstPrincipalChild() const {
    return GetFirstChild(kPrincipalList);
  }

  
  static const ChildListID kPrincipalList = mozilla::layout::kPrincipalList;
  static const ChildListID kAbsoluteList = mozilla::layout::kAbsoluteList;
  static const ChildListID kBulletList = mozilla::layout::kBulletList;
  static const ChildListID kCaptionList = mozilla::layout::kCaptionList;
  static const ChildListID kColGroupList = mozilla::layout::kColGroupList;
  static const ChildListID kExcessOverflowContainersList = mozilla::layout::kExcessOverflowContainersList;
  static const ChildListID kFixedList = mozilla::layout::kFixedList;
  static const ChildListID kFloatList = mozilla::layout::kFloatList;
  static const ChildListID kOverflowContainersList = mozilla::layout::kOverflowContainersList;
  static const ChildListID kOverflowList = mozilla::layout::kOverflowList;
  static const ChildListID kOverflowOutOfFlowList = mozilla::layout::kOverflowOutOfFlowList;
  static const ChildListID kPopupList = mozilla::layout::kPopupList;
  static const ChildListID kPushedFloatsList = mozilla::layout::kPushedFloatsList;
  static const ChildListID kSelectPopupList = mozilla::layout::kSelectPopupList;
  
  static const ChildListID kNoReflowPrincipalList = mozilla::layout::kNoReflowPrincipalList;

  


  nsIFrame* GetNextSibling() const { return mNextSibling; }
  void SetNextSibling(nsIFrame* aNextSibling) {
    NS_ASSERTION(this != aNextSibling, "Creating a circular frame list, this is very bad.");
    if (mNextSibling && mNextSibling->GetPrevSibling() == this) {
      mNextSibling->mPrevSibling = nullptr;
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

 
  bool IsThemed(nsITheme::Transparency* aTransparencyState = nullptr) const {
    return IsThemed(GetStyleDisplay(), aTransparencyState);
  }
  bool IsThemed(const nsStyleDisplay* aDisp,
                  nsITheme::Transparency* aTransparencyState = nullptr) const {
    nsIFrame* mutable_this = const_cast<nsIFrame*>(this);
    if (!aDisp->mAppearance)
      return false;
    nsPresContext* pc = PresContext();
    nsITheme *theme = pc->GetTheme();
    if(!theme ||
       !theme->ThemeSupportsWidget(pc, mutable_this, aDisp->mAppearance))
      return false;
    if (aTransparencyState) {
      *aTransparencyState =
        theme->GetWidgetTransparency(mutable_this, aDisp->mAppearance);
    }
    return true;
  }
  
  





  nsresult BuildDisplayListForStackingContext(nsDisplayListBuilder* aBuilder,
                                              const nsRect&         aDirtyRect,
                                              nsDisplayList*        aList);

  










  nsresult OverflowClip(nsDisplayListBuilder*   aBuilder,
                        const nsDisplayListSet& aFromSet,
                        const nsDisplayListSet& aToSet,
                        const nsRect&           aClipRect,
                        const nscoord           aClipRadii[8],
                        bool                    aClipBorderBackground = false,
                        bool                    aClipAll = false);

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

  


  virtual bool NeedsView() { return false; }

  




  bool IsTransformed() const;

  








  virtual bool IsSVGTransformed(gfxMatrix *aOwnTransforms = nullptr,
                                gfxMatrix *aFromParentTransforms = nullptr) const;

  



  bool Preserves3DChildren() const;

  



  bool Preserves3D() const;

  bool HasPerspective() const;

  bool ChildrenHavePerspective() const;

  
  void ComputePreserve3DChildrenOverflow(nsOverflowAreas& aOverflowAreas, const nsRect& aBounds);

  void RecomputePerspectiveChildrenOverflow(const nsStyleContext* aStartStyle, const nsRect* aBounds);

  
















  NS_IMETHOD  HandleEvent(nsPresContext* aPresContext,
                          nsGUIEvent*     aEvent,
                          nsEventStatus*  aEventStatus) = 0;

  NS_IMETHOD  GetContentForEvent(nsEvent* aEvent,
                                 nsIContent** aContent) = 0;

  
  
  
  
  
  
  
  
  
  struct NS_STACK_CLASS ContentOffsets {
    nsCOMPtr<nsIContent> content;
    bool IsNull() { return !content; }
    PRInt32 offset;
    PRInt32 secondaryOffset;
    
    
    PRInt32 StartOffset() { return NS_MIN(offset, secondaryOffset); }
    PRInt32 EndOffset() { return NS_MAX(offset, secondaryOffset); }
    
    
    
    bool associateWithNext;
  };
  enum {
    IGNORE_SELECTION_STYLE = 0x01,
    
    SKIP_HIDDEN = 0x02
  };
  






  ContentOffsets GetContentOffsetsFromPoint(nsPoint aPoint,
                                            PRUint32 aFlags = 0);

  virtual ContentOffsets GetContentOffsetsFromPointExternal(nsPoint aPoint,
                                                            PRUint32 aFlags = 0)
  { return GetContentOffsetsFromPoint(aPoint, aFlags); }

  




  struct NS_STACK_CLASS Cursor {
    nsCOMPtr<imgIContainer> mContainer;
    PRInt32                 mCursor;
    bool                    mHaveHotspot;
    float                   mHotspotX, mHotspotY;
  };
  


  NS_IMETHOD  GetCursor(const nsPoint&  aPoint,
                        Cursor&         aCursor) = 0;

  




  NS_IMETHOD  GetPointFromOffset(PRInt32                  inOffset,
                                 nsPoint*                 outPoint) = 0;
  
  







  NS_IMETHOD  GetChildFrameContainingOffset(PRInt32       inContentOffset,
                                 bool                     inHint,
                                 PRInt32*                 outFrameContentOffset,
                                 nsIFrame*                *outChildFrame) = 0;

 



  nsFrameState GetStateBits() const { return mState; }

  


  void AddStateBits(nsFrameState aBits) { mState |= aBits; }
  void RemoveStateBits(nsFrameState aBits) { mState &= ~aBits; }

  



  NS_IMETHOD  CharacterDataChanged(CharacterDataChangeInfo* aInfo) = 0;

  










  NS_IMETHOD  AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType) = 0;

  





  virtual void ContentStatesChanged(nsEventStates aStates) { }

  


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
      : line(nullptr)
      , lineContainer(nullptr)
      , prevLines(0)
      , currentLine(0)
      , skipWhitespace(true)
      , trailingWhitespace(0)
    {}

    
    
    const nsLineList_iterator* line;

    
    nsIFrame* lineContainer;

    
    nscoord prevLines;

    
    
    
    nscoord currentLine;

    
    
    
    bool skipWhitespace;

    
    
    nscoord trailingWhitespace;

    
    nsTArray<nsIFrame*> floats;
  };

  struct InlineMinWidthData : public InlineIntrinsicWidthData {
    InlineMinWidthData()
      : trailingTextFrame(nullptr)
      , atStartOfLine(true)
    {}

    
    
    
    
    void ForceBreak(nsRenderingContext *aRenderingContext);

    
    
    void OptionallyBreak(nsRenderingContext *aRenderingContext,
                         nscoord aHyphenWidth = 0);

    
    
    
    nsIFrame *trailingTextFrame;

    
    
    
    bool atStartOfLine;
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
    bool operator==(const IntrinsicSize& rhs) {
      return width == rhs.width && height == rhs.height;
    }
    bool operator!=(const IntrinsicSize& rhs) {
      return !(*this == rhs);
    }
  };
  virtual IntrinsicSize GetIntrinsicSize() = 0;

  








  virtual nsSize GetIntrinsicRatio() = 0;

  


  enum {
    


    eShrinkWrap =        1 << 0
  };

  



































  virtual nsSize ComputeSize(nsRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             PRUint32 aFlags) = 0;

  












  virtual nsRect ComputeTightBounds(gfxContext* aContext) const;

  








  NS_IMETHOD  WillReflow(nsPresContext* aPresContext) = 0;

  










































  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aReflowMetrics,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus) = 0;

  














  NS_IMETHOD  DidReflow(nsPresContext*           aPresContext,
                        const nsHTMLReflowState*  aReflowState,
                        nsDidReflowStatus         aStatus) = 0;

  

  




  virtual bool UpdateOverflow() = 0;

  








  virtual bool CanContinueTextRun() const = 0;

  














  virtual nsresult GetRenderedText(nsAString* aAppendToString = nullptr,
                                   gfxSkipChars* aSkipChars = nullptr,
                                   gfxSkipCharsIterator* aSkipIter = nullptr,
                                   PRUint32 aSkippedStartOffset = 0,
                                   PRUint32 aSkippedMaxLength = PR_UINT32_MAX)
  { return NS_ERROR_NOT_IMPLEMENTED; }

  




  virtual bool HasAnyNoncollapsedCharacters()
  { return false; }

  




  bool HasView() const { return !!(mState & NS_FRAME_HAS_VIEW); }
  nsIView* GetView() const;
  virtual nsIView* GetViewExternal() const;
  nsresult SetView(nsIView* aView);

  




  nsIView* GetClosestView(nsPoint* aOffset = nullptr) const;

  


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

  






  virtual nsIWidget* GetNearestWidget() const;

  




  virtual nsIWidget* GetNearestWidget(nsPoint& aOffset) const;

  




  virtual nsIAtom* GetType() const = 0;

  












  gfx3DMatrix GetTransformMatrix(nsIFrame* aStopAtAncestor,
                                 nsIFrame **aOutAncestor);

  


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

  






  virtual bool IsFrameOfType(PRUint32 aFlags) const
  {
#ifdef DEBUG
    return !(aFlags & ~(nsIFrame::eDEBUGAllFrames));
#else
    return !aFlags;
#endif
  }

  


  bool IsBlockWrapper() const;

  





  nsIFrame* GetContainingBlock() const;

  



  virtual bool IsFloatContainingBlock() const { return false; }

  








  virtual bool IsLeaf() const;

  










  void BeginDeferringInvalidatesForDisplayRoot(const nsRegion& aExcludeRegion);

  


  void EndDeferringInvalidatesForDisplayRoot();

  









  void MarkLayersActive(nsChangeHint aHint);
  


  bool AreLayersMarkedActive();
  






  bool AreLayersMarkedActive(nsChangeHint aChangeHint);

  


  void InvalidateWithFlags(const nsRect& aDamageRect, PRUint32 aFlags);

  









  void Invalidate(const nsRect& aDamageRect)
  { return InvalidateWithFlags(aDamageRect, 0); }

  








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
    INVALIDATE_NO_UPDATE_LAYER_TREE = 0x80,
    INVALIDATE_ALREADY_TRANSFORMED = 0x100
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

  




  nsRect GetPreEffectsVisualOverflowRect() const;

  





  bool FinishAndStoreOverflow(nsOverflowAreas& aOverflowAreas,
                              nsSize aNewSize);

  bool FinishAndStoreOverflow(nsHTMLReflowMetrics* aMetrics) {
    return FinishAndStoreOverflow(aMetrics->mOverflowAreas,
                                  nsSize(aMetrics->width, aMetrics->height));
  }

  



  bool HasOverflowAreas() const {
    return mOverflow.mType != NS_FRAME_OVERFLOW_NONE;
  }

  



  bool ClearOverflowRects();

  



  virtual PRIntn GetSkipSides() const { return 0; }

  


  bool IsSelected() const {
    return (GetContent() && GetContent()->IsSelectionDescendant()) ?
      IsFrameSelected() : false;
  }

  








  NS_IMETHOD  IsSelectable(bool* aIsSelectable, PRUint8* aSelectStyle) const = 0;

  




  NS_IMETHOD  GetSelectionController(nsPresContext *aPresContext, nsISelectionController **aSelCon) = 0;

  


  already_AddRefed<nsFrameSelection> GetFrameSelection();

  



  const nsFrameSelection* GetConstFrameSelection() const;

  






  NS_IMETHOD PeekOffset(nsPeekOffsetStruct *aPos);

  










  nsresult GetFrameFromDirection(nsDirection aDirection, bool aVisual,
                                 bool aJumpLines, bool aScrollViewStop, 
                                 nsIFrame** aOutFrame, PRInt32* aOutOffset, bool* aOutJumpedLine);

  










  NS_IMETHOD CheckVisibility(nsPresContext* aContext, PRInt32 aStartIndex, PRInt32 aEndIndex, bool aRecurse, bool *aFinished, bool *_retval)=0;

  





  virtual void ChildIsDirty(nsIFrame* aChild) = 0;

  






#ifdef ACCESSIBILITY
  virtual already_AddRefed<Accessible> CreateAccessible() = 0;
#endif

  











  virtual nsIFrame* GetParentStyleContextFrame() const = 0;

  



  bool IsVisibleForPainting(nsDisplayListBuilder* aBuilder);
  



  bool IsVisibleOrCollapsedForPainting(nsDisplayListBuilder* aBuilder);
  



  bool IsVisibleForPainting();
  



  bool IsVisibleInSelection(nsDisplayListBuilder* aBuilder);

  


  
  virtual bool IsVisibleInSelection(nsISelection* aSelection);

  





  bool IsPseudoStackingContextFromStyle() {
    const nsStyleDisplay* disp = GetStyleDisplay();
    return disp->mOpacity != 1.0f || disp->IsPositioned() || disp->IsFloating();
  }
  
  virtual bool HonorPrintBackgroundSettings() { return true; }

  








  virtual bool IsEmpty() = 0;
  



  virtual bool CachedIsEmpty();
  



  virtual bool IsSelfEmpty() = 0;

  





  bool IsGeneratedContentFrame() {
    return (mState & NS_FRAME_GENERATED_CONTENT) != 0;
  }

  






   
  bool IsPseudoFrame(nsIContent* aParentContent) {
    return mContent == aParentContent;
  }

  FrameProperties Properties() const {
    return FrameProperties(PresContext()->PropertyTable(), this);
  }

  NS_DECLARE_FRAME_PROPERTY(BaseLevelProperty, nullptr)
  NS_DECLARE_FRAME_PROPERTY(EmbeddingLevelProperty, nullptr)
  NS_DECLARE_FRAME_PROPERTY(ParagraphDepthProperty, nullptr)

#define NS_GET_BASE_LEVEL(frame) \
NS_PTR_TO_INT32(frame->Properties().Get(nsIFrame::BaseLevelProperty()))

#define NS_GET_EMBEDDING_LEVEL(frame) \
NS_PTR_TO_INT32(frame->Properties().Get(nsIFrame::EmbeddingLevelProperty()))

#define NS_GET_PARAGRAPH_DEPTH(frame) \
NS_PTR_TO_INT32(frame->Properties().Get(nsIFrame::ParagraphDepthProperty()))

  




  virtual bool SupportsVisibilityHidden() { return true; }

  










  bool GetClipPropClipRect(const nsStyleDisplay* aDisp, nsRect* aRect,
                           const nsSize& aSize) const;

  

















  virtual bool IsFocusable(PRInt32 *aTabIndex = nullptr, bool aWithMouse = false);

  void ClearDisplayItemCache();

  
  
  
  bool IsBoxFrame() const
  {
    return IsFrameOfType(nsIFrame::eXULBox);
  }
  bool IsBoxWrapped() const
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
  virtual bool IsCollapsed() = 0;
  
  
  
  
  
  virtual void SetBounds(nsBoxLayoutState& aBoxLayoutState, const nsRect& aRect,
                         bool aRemoveOverflowAreas = false) = 0;
  NS_HIDDEN_(nsresult) Layout(nsBoxLayoutState& aBoxLayoutState);
  nsIBox* GetChildBox() const
  {
    
    
    return IsBoxFrame() ? GetFirstPrincipalChild() : nullptr;
  }
  nsIBox* GetNextBox() const
  {
    return (mParent && mParent->IsBoxFrame()) ? mNextSibling : nullptr;
  }
  nsIBox* GetParentBox() const
  {
    return (mParent && mParent->IsBoxFrame()) ? mParent : nullptr;
  }
  
  
  NS_IMETHOD GetBorderAndPadding(nsMargin& aBorderAndPadding);
  NS_IMETHOD GetBorder(nsMargin& aBorder)=0;
  NS_IMETHOD GetPadding(nsMargin& aBorderAndPadding)=0;
  NS_IMETHOD GetMargin(nsMargin& aMargin)=0;
  virtual void SetLayoutManager(nsBoxLayout* aLayout) { }
  virtual nsBoxLayout* GetLayoutManager() { return nullptr; }
  NS_HIDDEN_(nsresult) GetClientRect(nsRect& aContentRect);

  
  virtual Valignment GetVAlign() const = 0;
  virtual Halignment GetHAlign() const = 0;

  bool IsHorizontal() const { return (mState & NS_STATE_IS_HORIZONTAL) != 0; }
  bool IsNormalDirection() const { return (mState & NS_STATE_IS_DIRECTION_NORMAL) != 0; }

  NS_HIDDEN_(nsresult) Redraw(nsBoxLayoutState& aState, const nsRect* aRect = nullptr);
  NS_IMETHOD RelayoutChildAtOrdinal(nsBoxLayoutState& aState, nsIBox* aChild)=0;
  
  virtual bool GetMouseThrough() const { return false; }

#ifdef DEBUG_LAYOUT
  NS_IMETHOD SetDebug(nsBoxLayoutState& aState, bool aDebug)=0;
  NS_IMETHOD GetDebug(bool& aDebug)=0;

  NS_IMETHOD DumpBox(FILE* out)=0;
#endif

  



  virtual bool HasTerminalNewline() const;

  static bool AddCSSPrefSize(nsIBox* aBox, nsSize& aSize, bool& aWidth, bool& aHeightSet);
  static bool AddCSSMinSize(nsBoxLayoutState& aState, nsIBox* aBox,
                              nsSize& aSize, bool& aWidth, bool& aHeightSet);
  static bool AddCSSMaxSize(nsIBox* aBox, nsSize& aSize, bool& aWidth, bool& aHeightSet);
  static bool AddCSSFlex(nsBoxLayoutState& aState, nsIBox* aBox, nscoord& aFlex);

  
  
  

  struct CaretPosition {
    CaretPosition() :
      mContentOffset(0)
    {}

    nsCOMPtr<nsIContent> mResultContent;
    PRInt32              mContentOffset;
  };

  









  CaretPosition GetExtremeCaretPosition(bool aStart);

  






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

  


  bool IsAbsoluteContainer() const { return !!(mState & NS_FRAME_HAS_ABSPOS_CHILDREN); }
  bool HasAbsolutelyPositionedChildren() const;
  nsAbsoluteContainingBlock* GetAbsoluteContainingBlock() const;
  virtual void MarkAsAbsoluteContainingBlock();
  
  virtual mozilla::layout::FrameChildListID GetAbsoluteListID() const { return kAbsoluteList; }

  
  
  bool CheckAndClearPaintedState();

  
  
  
  
  
  
  
  
  enum {
    VISIBILITY_CROSS_CHROME_CONTENT_BOUNDARY = 0x01
  };
  bool IsVisibleConsideringAncestors(PRUint32 aFlags = 0) const;

protected:
  
  nsRect           mRect;
  nsIContent*      mContent;
  nsStyleContext*  mStyleContext;
  nsIFrame*        mParent;
private:
  nsIFrame*        mNextSibling;  
  nsIFrame*        mPrevSibling;  

  void MarkAbsoluteFramesForDisplayList(nsDisplayListBuilder* aBuilder, const nsRect& aDirtyRect);

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

  
  
  
  
  
  
  
  
  
  struct VisualDeltas {
    PRUint8 mLeft;
    PRUint8 mTop;
    PRUint8 mRight;
    PRUint8 mBottom;
    bool operator==(const VisualDeltas& aOther) const
    {
      return mLeft == aOther.mLeft && mTop == aOther.mTop &&
             mRight == aOther.mRight && mBottom == aOther.mBottom;
    }
    bool operator!=(const VisualDeltas& aOther) const
    {
      return !(*this == aOther);
    }
  };
  union {
    PRUint32     mType;
    VisualDeltas mVisualDeltas;
  } mOverflow;

  
  



  void InvalidateRoot(const nsRect& aDamageRect, PRUint32 aFlags);

  








  virtual bool PeekOffsetNoAmount(bool aForward, PRInt32* aOffset) = 0;
  
  











  virtual bool PeekOffsetCharacter(bool aForward, PRInt32* aOffset,
                                     bool aRespectClusters = true) = 0;
  
  

















  struct PeekWordState {
    
    
    bool mAtStart;
    
    
    bool mSawBeforeType;
    
    bool mLastCharWasPunctuation;
    
    bool mLastCharWasWhitespace;
    
    bool mSeenNonPunctuationSinceWhitespace;
    
    
    
    nsAutoString mContext;

    PeekWordState() : mAtStart(true), mSawBeforeType(false),
        mLastCharWasPunctuation(false), mLastCharWasWhitespace(false),
        mSeenNonPunctuationSinceWhitespace(false) {}
    void SetSawBeforeType() { mSawBeforeType = true; }
    void Update(bool aAfterPunctuation, bool aAfterWhitespace) {
      mLastCharWasPunctuation = aAfterPunctuation;
      mLastCharWasWhitespace = aAfterWhitespace;
      if (aAfterWhitespace) {
        mSeenNonPunctuationSinceWhitespace = false;
      } else if (!aAfterPunctuation) {
        mSeenNonPunctuationSinceWhitespace = true;
      }
      mAtStart = false;
    }
  };
  virtual bool PeekOffsetWord(bool aForward, bool aWordSelectEatSpace, bool aIsKeyboardSelect,
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
  


  bool SetOverflowAreas(const nsOverflowAreas& aOverflowAreas);
  nsPoint GetOffsetToCrossDoc(const nsIFrame* aOther, const PRInt32 aAPD) const;

#ifdef DEBUG
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
  nsWeakFrame() : mPrev(nullptr), mFrame(nullptr) { }

  nsWeakFrame(const nsWeakFrame& aOther) : mPrev(nullptr), mFrame(nullptr)
  {
    Init(aOther.GetFrame());
  }

  nsWeakFrame(nsIFrame* aFrame) : mPrev(nullptr), mFrame(nullptr)
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
    mFrame = nullptr;
    mPrev = nullptr;
  }

  bool IsAlive() { return !!mFrame; }

  nsIFrame* GetFrame() const { return mFrame; }

  nsWeakFrame* GetPreviousWeakFrame() { return mPrev; }

  void SetPreviousWeakFrame(nsWeakFrame* aPrev) { mPrev = aPrev; }

  ~nsWeakFrame()
  {
    Clear(mFrame ? mFrame->PresContext()->GetPresShell() : nullptr);
  }
private:
  void InitInternal(nsIFrame* aFrame);

  void InitExternal(nsIFrame* aFrame) {
    Clear(mFrame ? mFrame->PresContext()->GetPresShell() : nullptr);
    mFrame = aFrame;
    if (mFrame) {
      nsIPresShell* shell = mFrame->PresContext()->GetPresShell();
      NS_WARN_IF_FALSE(shell, "Null PresShell in nsWeakFrame!");
      if (shell) {
        shell->AddWeakFrame(this);
      } else {
        mFrame = nullptr;
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
