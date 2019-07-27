







#ifndef nsIFrame_h___
#define nsIFrame_h___

#ifndef MOZILLA_INTERNAL_API
#error This header/class should only be used within Mozilla code. It should not be used by extensions.
#endif

#define MAX_REFLOW_DEPTH 200






#include <algorithm>
#include <stdio.h>

#include "CaretAssociationHint.h"
#include "FramePropertyTable.h"
#include "mozilla/layout/FrameChildList.h"
#include "mozilla/WritingModes.h"
#include "nsDirection.h"
#include "nsFrameList.h"
#include "nsFrameState.h"
#include "nsHTMLReflowMetrics.h"
#include "nsITheme.h"
#include "nsLayoutUtils.h"
#include "nsQueryFrame.h"
#include "nsStyleContext.h"
#include "nsStyleStruct.h"

#ifdef ACCESSIBILITY
#include "mozilla/a11y/AccTypes.h"
#endif


















struct nsHTMLReflowState;
class nsHTMLReflowCommand;
class nsIAtom;
class nsPresContext;
class nsIPresShell;
class nsRenderingContext;
class nsView;
class nsIWidget;
class nsIDOMRange;
class nsISelectionController;
class nsBoxLayoutState;
class nsBoxLayout;
class nsILineIterator;
class nsDisplayListBuilder;
class nsDisplayListSet;
class nsDisplayList;
class gfxSkipChars;
class gfxSkipCharsIterator;
class gfxContext;
class nsLineList_iterator;
class nsAbsoluteContainingBlock;
class nsIContent;
class nsContainerFrame;

struct nsPeekOffsetStruct;
struct nsPoint;
struct nsRect;
struct nsSize;
struct nsMargin;
struct CharacterDataChangeInfo;

namespace mozilla {

class EventStates;

namespace layers {
class Layer;
}

namespace gfx {
class Matrix;
}
}














typedef uint32_t nsSplittableType;

#define NS_FRAME_NOT_SPLITTABLE             0   // Note: not a bit!
#define NS_FRAME_SPLITTABLE                 0x1
#define NS_FRAME_SPLITTABLE_NON_RECTANGULAR 0x3

#define NS_FRAME_IS_SPLITTABLE(type)\
  (0 != ((type) & NS_FRAME_SPLITTABLE))

#define NS_FRAME_IS_NOT_SPLITTABLE(type)\
  (0 == ((type) & NS_FRAME_SPLITTABLE))

#define NS_INTRINSIC_WIDTH_UNKNOWN nscoord_MIN



#define NS_SUBTREE_DIRTY(_frame)  \
  (((_frame)->GetStateBits() &      \
    (NS_FRAME_IS_DIRTY | NS_FRAME_HAS_DIRTY_CHILDREN)) != 0)






#define NS_UNCONSTRAINEDSIZE NS_MAXSIZE

#define NS_INTRINSICSIZE    NS_UNCONSTRAINEDSIZE
#define NS_AUTOHEIGHT       NS_UNCONSTRAINEDSIZE
#define NS_AUTOMARGIN       NS_UNCONSTRAINEDSIZE
#define NS_AUTOOFFSET       NS_UNCONSTRAINEDSIZE






enum nsSelectionAmount {
  eSelectCharacter = 0, 
                        
                        
  eSelectCluster   = 1, 
                        
                        
  eSelectWord      = 2,
  eSelectWordNoSpace = 3, 
                          
                          
  eSelectLine      = 4, 
  
  
  

  eSelectBeginLine = 5,
  eSelectEndLine   = 6,
  eSelectNoAmount  = 7, 
  eSelectParagraph = 8  
};

enum nsSpread {
  eSpreadNone   = 0,
  eSpreadAcross = 1,
  eSpreadDown   = 2
};


#define NS_CARRIED_TOP_MARGIN_IS_AUTO    0x1
#define NS_CARRIED_BOTTOM_MARGIN_IS_AUTO 0x2





































typedef uint32_t nsReflowStatus;

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






enum class nsDidReflowStatus : uint32_t {
  NOT_FINISHED,
  FINISHED
};










#define NS_FRAME_OVERFLOW_DELTA_MAX     0xfe // max delta we can store

#define NS_FRAME_OVERFLOW_NONE    0x00000000 // there are no overflow rects;
                                             
                                             

#define NS_FRAME_OVERFLOW_LARGE   0x000000ff // overflow is stored as a
                                             

namespace mozilla {







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
}


template<typename T>
static void DeleteValue(void* aPropertyValue)
{
  delete static_cast<T*>(aPropertyValue);
}


template<typename T>
static void ReleaseValue(void* aPropertyValue)
{
  static_cast<T*>(aPropertyValue)->Release();
}
























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
  typedef mozilla::gfx::Matrix Matrix;
  typedef mozilla::gfx::Matrix4x4 Matrix4x4;
  typedef mozilla::Sides Sides;
  typedef mozilla::LogicalSides LogicalSides;

  NS_DECL_QUERYFRAME_TARGET(nsIFrame)

  nsPresContext* PresContext() const {
    return StyleContext()->RuleNode()->PresContext();
  }

  













  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) = 0;

  







  void Destroy() { DestroyFrom(this); }
 
  

  enum FrameSearchResult {
    
    FOUND = 0x00,
    
    CONTINUE = 0x1,
    
    CONTINUE_EMPTY = 0x2 | CONTINUE,
    
    CONTINUE_UNSELECTABLE = 0x4 | CONTINUE,
  };

protected:
  



  virtual bool IsFrameSelected() const;

  








  virtual void DestroyFrom(nsIFrame* aDestructRoot) = 0;
  friend class nsFrameList; 
  friend class nsLineBox;   
  friend class nsContainerFrame; 
  friend class nsFrame; 
public:

  


  nsIContent* GetContent() const { return mContent; }

  



  virtual nsContainerFrame* GetContentInsertionFrame() { return nullptr; }

  



  virtual bool DrainSelfOverflowList() { return false; }

  






  virtual nsIScrollableFrame* GetScrollTargetFrame() { return nullptr; }

  



  virtual nsresult GetOffsets(int32_t &start, int32_t &end) const = 0;

  



  virtual void AdjustOffsetsForBidi(int32_t aStart, int32_t aEnd) {}

  


  nsStyleContext* StyleContext() const { return mStyleContext; }
  void SetStyleContext(nsStyleContext* aContext)
  { 
    if (aContext != mStyleContext) {
      nsStyleContext* oldStyleContext = mStyleContext;
      mStyleContext = aContext;
      aContext->AddRef();
#ifdef DEBUG
      aContext->FrameAddRef();
#endif
      DidSetStyleContext(oldStyleContext);
#ifdef DEBUG
      oldStyleContext->FrameRelease();
#endif
      oldStyleContext->Release();
    }
  }

  





  void SetStyleContextWithoutNotification(nsStyleContext* aContext)
  {
    if (aContext != mStyleContext) {
#ifdef DEBUG
      mStyleContext->FrameRelease();
#endif
      mStyleContext->Release();
      mStyleContext = aContext;
      aContext->AddRef();
#ifdef DEBUG
      aContext->FrameAddRef();
#endif
    }
  }

  
  
  
  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) = 0;

  









  #define STYLE_STRUCT(name_, checkdata_cb_)                                  \
    const nsStyle##name_ * Style##name_ () const {                            \
      NS_ASSERTION(mStyleContext, "No style context found!");                 \
      return mStyleContext->Style##name_ ();                                  \
    }
  #include "nsStyleStructList.h"
  #undef STYLE_STRUCT

  
  nscolor GetVisitedDependentColor(nsCSSProperty aProperty)
    { return mStyleContext->GetVisitedDependentColor(aProperty); }

  









  virtual nsStyleContext* GetAdditionalStyleContext(int32_t aIndex) const = 0;

  virtual void SetAdditionalStyleContext(int32_t aIndex,
                                         nsStyleContext* aStyleContext) = 0;

  


  nsContainerFrame* GetParent() const { return mParent; }
  





  void SetParent(nsContainerFrame* aParent);

  


  virtual mozilla::WritingMode GetWritingMode() const {
    return mozilla::WritingMode(StyleContext());
  }

  






  mozilla::WritingMode GetWritingMode(nsIFrame* aSubFrame) const;

  







  nsRect GetRect() const { return mRect; }
  nsPoint GetPosition() const { return mRect.TopLeft(); }
  nsSize GetSize() const { return mRect.Size(); }
  nsRect GetRectRelativeToSelf() const {
    return nsRect(nsPoint(0, 0), mRect.Size());
  }
  



  mozilla::LogicalRect GetLogicalRect(nscoord aContainerWidth) const {
    return GetLogicalRect(GetWritingMode(), aContainerWidth);
  }
  mozilla::LogicalPoint GetLogicalPosition(nscoord aContainerWidth) const {
    return GetLogicalPosition(GetWritingMode(), aContainerWidth);
  }
  mozilla::LogicalSize GetLogicalSize() const {
    return GetLogicalSize(GetWritingMode());
  }
  mozilla::LogicalRect GetLogicalRect(mozilla::WritingMode aWritingMode,
                                      nscoord aContainerWidth) const {
    return mozilla::LogicalRect(aWritingMode, GetRect(), aContainerWidth);
  }
  mozilla::LogicalPoint GetLogicalPosition(mozilla::WritingMode aWritingMode,
                                           nscoord aContainerWidth) const {
    return GetLogicalRect(aWritingMode, aContainerWidth).Origin(aWritingMode);
  }
  mozilla::LogicalSize GetLogicalSize(mozilla::WritingMode aWritingMode) const {
    return mozilla::LogicalSize(aWritingMode, GetSize());
  }
  nscoord IStart(nscoord aContainerWidth) const {
    return IStart(GetWritingMode(), aContainerWidth);
  }
  nscoord IStart(mozilla::WritingMode aWritingMode,
                 nscoord aContainerWidth) const {
    return GetLogicalPosition(aWritingMode, aContainerWidth).I(aWritingMode);
  }
  nscoord BStart(nscoord aContainerWidth) const {
    return BStart(GetWritingMode(), aContainerWidth);
  }
  nscoord BStart(mozilla::WritingMode aWritingMode,
                 nscoord aContainerWidth) const {
    return GetLogicalPosition(aWritingMode, aContainerWidth).B(aWritingMode);
  }
  nscoord ISize() const { return ISize(GetWritingMode()); }
  nscoord ISize(mozilla::WritingMode aWritingMode) const {
    return GetLogicalSize(aWritingMode).ISize(aWritingMode);
  }
  nscoord BSize() const { return BSize(GetWritingMode()); }
  nscoord BSize(mozilla::WritingMode aWritingMode) const {
    return GetLogicalSize(aWritingMode).BSize(aWritingMode);
  }

  





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
  


  void SetRect(const mozilla::LogicalRect& aRect, nscoord aContainerWidth) {
    SetRect(GetWritingMode(), aRect, aContainerWidth);
  }
  



  void SetRect(mozilla::WritingMode aWritingMode,
               const mozilla::LogicalRect& aRect,
               nscoord aContainerWidth) {
    SetRect(aRect.GetPhysicalRect(aWritingMode, aContainerWidth));
  }

  


  void SetSize(const mozilla::LogicalSize& aSize) {
    SetSize(GetWritingMode(), aSize);
  }
  


  void SetSize(mozilla::WritingMode aWritingMode,
               const mozilla::LogicalSize& aSize) {
    SetSize(aSize.GetPhysicalSize(aWritingMode));
  }
  void SetSize(const nsSize& aSize) {
    SetRect(nsRect(mRect.TopLeft(), aSize));
  }
  void SetPosition(const nsPoint& aPt) { mRect.MoveTo(aPt); }
  void SetPosition(mozilla::WritingMode aWritingMode,
                   const mozilla::LogicalPoint& aPt,
                   nscoord aContainerWidth) {
    
    
    
    mRect.MoveTo(aPt.GetPhysicalPoint(aWritingMode,
                                      aContainerWidth - mRect.width));
  }

  










  void MovePositionBy(const nsPoint& aTranslation);

  


  void MovePositionBy(mozilla::WritingMode aWritingMode,
                      const mozilla::LogicalPoint& aTranslation)
  {
    MovePositionBy(aTranslation.GetPhysicalPoint(aWritingMode, 0));
  }

  


  nsRect GetNormalRect() const;

  


  nsPoint GetNormalPosition() const;
  mozilla::LogicalPoint
  GetLogicalNormalPosition(mozilla::WritingMode aWritingMode,
                           nscoord aContainerWidth) const
  {
    
    
    
    return mozilla::LogicalPoint(aWritingMode,
                                 GetNormalPosition(),
                                 aContainerWidth - mRect.width);
  }

  virtual nsPoint GetPositionOfChildIgnoringScrolling(nsIFrame* aChild)
  { return aChild->GetPosition(); }
  
  nsPoint GetPositionIgnoringScrolling();

  static void DestroyContentArray(void* aPropertyValue);

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

  NS_DECLARE_FRAME_PROPERTY(IBSplitSibling, nullptr)
  NS_DECLARE_FRAME_PROPERTY(IBSplitPrevSibling, nullptr)

  NS_DECLARE_FRAME_PROPERTY(NormalPositionProperty, DeleteValue<nsPoint>)
  NS_DECLARE_FRAME_PROPERTY(ComputedOffsetProperty, DeleteValue<nsMargin>)

  NS_DECLARE_FRAME_PROPERTY(OutlineInnerRectProperty, DeleteValue<nsRect>)
  NS_DECLARE_FRAME_PROPERTY(PreEffectsBBoxProperty, DeleteValue<nsRect>)
  NS_DECLARE_FRAME_PROPERTY(PreTransformOverflowAreasProperty,
                            DeleteValue<nsOverflowAreas>)

  
  
  
  NS_DECLARE_FRAME_PROPERTY(InitialOverflowProperty,
                            DeleteValue<nsOverflowAreas>)

#ifdef DEBUG
  
  
  
  NS_DECLARE_FRAME_PROPERTY(DebugInitialOverflowPropertyApplied, nullptr)
#endif

  NS_DECLARE_FRAME_PROPERTY(UsedMarginProperty, DeleteValue<nsMargin>)
  NS_DECLARE_FRAME_PROPERTY(UsedPaddingProperty, DeleteValue<nsMargin>)
  NS_DECLARE_FRAME_PROPERTY(UsedBorderProperty, DeleteValue<nsMargin>)

  NS_DECLARE_FRAME_PROPERTY(ScrollLayerCount, nullptr)

  NS_DECLARE_FRAME_PROPERTY(LineBaselineOffset, nullptr)

  NS_DECLARE_FRAME_PROPERTY(CachedBackgroundImage, ReleaseValue<gfxASurface>)
  NS_DECLARE_FRAME_PROPERTY(CachedBackgroundImageDT,
                            ReleaseValue<mozilla::gfx::DrawTarget>)

  NS_DECLARE_FRAME_PROPERTY(InvalidationRect, DeleteValue<nsRect>)

  NS_DECLARE_FRAME_PROPERTY(RefusedAsyncAnimation, nullptr)

  NS_DECLARE_FRAME_PROPERTY(GenConProperty, DestroyContentArray)

  nsTArray<nsIContent*>* GetGenConPseudos() {
    const FramePropertyDescriptor* prop = GenConProperty();
    return static_cast<nsTArray<nsIContent*>*>(Properties().Get(prop));
  }

  










  virtual nsMargin GetUsedMargin() const;
  virtual mozilla::LogicalMargin
  GetLogicalUsedMargin(mozilla::WritingMode aWritingMode) const {
    return mozilla::LogicalMargin(aWritingMode, GetUsedMargin());
  }

  









  virtual nsMargin GetUsedBorder() const;
  virtual mozilla::LogicalMargin
  GetLogicalUsedBorder(mozilla::WritingMode aWritingMode) const {
    return mozilla::LogicalMargin(aWritingMode, GetUsedBorder());
  }

  




  virtual nsMargin GetUsedPadding() const;
  virtual mozilla::LogicalMargin
  GetLogicalUsedPadding(mozilla::WritingMode aWritingMode) const {
    return mozilla::LogicalMargin(aWritingMode, GetUsedPadding());
  }

  nsMargin GetUsedBorderAndPadding() const {
    return GetUsedBorder() + GetUsedPadding();
  }
  mozilla::LogicalMargin
  GetLogicalUsedBorderAndPadding(mozilla::WritingMode aWritingMode) const {
    return mozilla::LogicalMargin(aWritingMode, GetUsedBorderAndPadding());
  }

  



  nsRect GetPaddingRect() const;
  nsRect GetPaddingRectRelativeToSelf() const;
  nsRect GetContentRect() const;
  nsRect GetContentRectRelativeToSelf() const;
  nsRect GetMarginRectRelativeToSelf() const;

  



  virtual nsRect VisualBorderRectRelativeToSelf() const {
    return nsRect(0, 0, mRect.width, mRect.height);
  }

  

















  static bool ComputeBorderRadii(const nsStyleCorners& aBorderRadius,
                                 const nsSize& aFrameSize,
                                 const nsSize& aBorderArea,
                                 Sides aSkipSides,
                                 nscoord aRadii[8]);

  












  static void InsetBorderRadii(nscoord aRadii[8], const nsMargin &aOffsets);
  static void OutsetBorderRadii(nscoord aRadii[8], const nsMargin &aOffsets);

  





  virtual bool GetBorderRadii(const nsSize& aFrameSize,
                              const nsSize& aBorderArea,
                              Sides aSkipSides,
                              nscoord aRadii[8]) const;
  bool GetBorderRadii(nscoord aRadii[8]) const;

  bool GetPaddingBoxBorderRadii(nscoord aRadii[8]) const;
  bool GetContentBoxBorderRadii(nscoord aRadii[8]) const;

  




  virtual nscoord GetLogicalBaseline(mozilla::WritingMode aWritingMode) const = 0;

  





  virtual nscoord GetCaretBaseline() const {
    return GetLogicalBaseline(GetWritingMode());
  }

  






  virtual const nsFrameList& GetChildList(ChildListID aListID) const = 0;
  const nsFrameList& PrincipalChildList() { return GetChildList(kPrincipalList); }
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const = 0;

  



  void GetCrossDocChildLists(nsTArray<ChildList>* aLists);

  
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

  













  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) {}
  





  void DisplayCaret(nsDisplayListBuilder* aBuilder,
                    const nsRect&         aDirtyRect,
                    nsDisplayList*        aList);

  




  virtual nscolor GetCaretColorAt(int32_t aOffset);

 
  bool IsThemed(nsITheme::Transparency* aTransparencyState = nullptr) const {
    return IsThemed(StyleDisplay(), aTransparencyState);
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
  
  





  void BuildDisplayListForStackingContext(nsDisplayListBuilder* aBuilder,
                                          const nsRect&         aDirtyRect,
                                          nsDisplayList*        aList);

  enum {
    DISPLAY_CHILD_FORCE_PSEUDO_STACKING_CONTEXT = 0x01,
    DISPLAY_CHILD_FORCE_STACKING_CONTEXT = 0x02,
    DISPLAY_CHILD_INLINE = 0x04
  };
  








  void BuildDisplayListForChild(nsDisplayListBuilder*   aBuilder,
                                nsIFrame*               aChild,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists,
                                uint32_t                aFlags = 0);

  


  virtual bool NeedsView() { return false; }

  




  bool IsTransformed() const;

  



  bool HasOpacity() const
  {
    return HasOpacityInternal(1.0f);
  }
  


  bool HasVisualOpacity() const
  {
    
    
    
    return HasOpacityInternal(0.99f);
  }

   


  virtual bool HasTransformGetter() const { return false; }

  








  virtual bool IsSVGTransformed(Matrix *aOwnTransforms = nullptr,
                                Matrix *aFromParentTransforms = nullptr) const;

  




  bool Preserves3DChildren() const;

  




  bool Preserves3D() const;

  bool HasPerspective() const;

  bool ChildrenHavePerspective() const;

  
  void ComputePreserve3DChildrenOverflow(nsOverflowAreas& aOverflowAreas, const nsRect& aBounds);

  void RecomputePerspectiveChildrenOverflow(const nsStyleContext* aStartStyle, const nsRect* aBounds);

  


  uint32_t GetDepthInFrameTree() const;

  
















  virtual nsresult  HandleEvent(nsPresContext* aPresContext,
                                mozilla::WidgetGUIEvent* aEvent,
                                nsEventStatus* aEventStatus) = 0;

  virtual nsresult  GetContentForEvent(mozilla::WidgetEvent* aEvent,
                                       nsIContent** aContent) = 0;

  
  
  
  
  
  
  
  
  
  struct MOZ_STACK_CLASS ContentOffsets {
    ContentOffsets();
    ContentOffsets(const ContentOffsets&);
    ~ContentOffsets();
    nsCOMPtr<nsIContent> content;
    bool IsNull() { return !content; }
    int32_t offset;
    int32_t secondaryOffset;
    
    
    int32_t StartOffset() { return std::min(offset, secondaryOffset); }
    int32_t EndOffset() { return std::max(offset, secondaryOffset); }
    
    
    
    mozilla::CaretAssociationHint associate;
  };
  enum {
    IGNORE_SELECTION_STYLE = 0x01,
    
    SKIP_HIDDEN = 0x02
  };
  






  ContentOffsets GetContentOffsetsFromPoint(nsPoint aPoint,
                                            uint32_t aFlags = 0);

  virtual ContentOffsets GetContentOffsetsFromPointExternal(nsPoint aPoint,
                                                            uint32_t aFlags = 0)
  { return GetContentOffsetsFromPoint(aPoint, aFlags); }

  



  void AssociateImage(const nsStyleImage& aImage, nsPresContext* aPresContext);

  




  struct MOZ_STACK_CLASS Cursor {
    nsCOMPtr<imgIContainer> mContainer;
    int32_t                 mCursor;
    bool                    mHaveHotspot;
    float                   mHotspotX, mHotspotY;
  };
  


  virtual nsresult  GetCursor(const nsPoint&  aPoint,
                              Cursor&         aCursor) = 0;

  




  virtual nsresult  GetPointFromOffset(int32_t                  inOffset,
                                       nsPoint*                 outPoint) = 0;
  
  







  virtual nsresult GetChildFrameContainingOffset(int32_t    inContentOffset,
                                                 bool       inHint,
                                                 int32_t*   outFrameContentOffset,
                                                 nsIFrame** outChildFrame) = 0;

 



  nsFrameState GetStateBits() const { return mState; }

  


  void AddStateBits(nsFrameState aBits) { mState |= aBits; }
  void RemoveStateBits(nsFrameState aBits) { mState &= ~aBits; }

  


  bool HasAllStateBits(nsFrameState aBits) const
  {
    return (mState & aBits) == aBits;
  }
  
  


  bool HasAnyStateBits(nsFrameState aBits) const
  {
    return mState & aBits;
  }

  



  virtual nsresult  CharacterDataChanged(CharacterDataChangeInfo* aInfo) = 0;

  










  virtual nsresult  AttributeChanged(int32_t         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     int32_t         aModType) = 0;

  





  virtual void ContentStatesChanged(mozilla::EventStates aStates);

  


  virtual nsSplittableType GetSplittableType() const = 0;

  


  virtual nsIFrame* GetPrevContinuation() const = 0;
  virtual void SetPrevContinuation(nsIFrame*) = 0;
  virtual nsIFrame* GetNextContinuation() const = 0;
  virtual void SetNextContinuation(nsIFrame*) = 0;
  virtual nsIFrame* FirstContinuation() const {
    return const_cast<nsIFrame*>(this);
  }
  virtual nsIFrame* LastContinuation() const {
    return const_cast<nsIFrame*>(this);
  }

  




  nsIFrame* GetTailContinuation();

  


  virtual nsIFrame* GetPrevInFlowVirtual() const = 0;
  nsIFrame* GetPrevInFlow() const { return GetPrevInFlowVirtual(); }
  virtual void SetPrevInFlow(nsIFrame*) = 0;

  virtual nsIFrame* GetNextInFlowVirtual() const = 0;
  nsIFrame* GetNextInFlow() const { return GetNextInFlowVirtual(); }
  virtual void SetNextInFlow(nsIFrame*) = 0;

  


  virtual nsIFrame* FirstInFlow() const {
    return const_cast<nsIFrame*>(this);
  }

  


  virtual nsIFrame* LastInFlow() const {
    return const_cast<nsIFrame*>(this);
  }

  




  




  virtual void MarkIntrinsicISizesDirty() = 0;

  




















  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) = 0;

  





  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) = 0;

  






  struct InlineIntrinsicISizeData {
    InlineIntrinsicISizeData()
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

    
    class FloatInfo {
    public:
      FloatInfo(const nsIFrame* aFrame, nscoord aWidth)
        : mFrame(aFrame), mWidth(aWidth)
      { }
      const nsIFrame* Frame() const { return mFrame; }
      nscoord         Width() const { return mWidth; }

    private:
      const nsIFrame* mFrame;
      nscoord         mWidth;
    };

    nsTArray<FloatInfo> floats;
  };

  struct InlineMinISizeData : public InlineIntrinsicISizeData {
    InlineMinISizeData()
      : trailingTextFrame(nullptr)
      , atStartOfLine(true)
    {}

    
    
    
    
    void ForceBreak(nsRenderingContext *aRenderingContext);

    
    
    void OptionallyBreak(nsRenderingContext *aRenderingContext,
                         nscoord aHyphenWidth = 0);

    
    
    
    nsIFrame *trailingTextFrame;

    
    
    
    bool atStartOfLine;
  };

  struct InlinePrefISizeData : public InlineIntrinsicISizeData {
    void ForceBreak(nsRenderingContext *aRenderingContext);
  };

  


















  virtual void
  AddInlineMinISize(nsRenderingContext *aRenderingContext,
                    InlineMinISizeData *aData) = 0;

  









  virtual void
  AddInlinePrefISize(nsRenderingContext *aRenderingContext,
                     InlinePrefISizeData *aData) = 0;

  



  struct IntrinsicISizeOffsetData {
    nscoord hPadding, hBorder, hMargin;
    float hPctPadding, hPctMargin;

    IntrinsicISizeOffsetData()
      : hPadding(0), hBorder(0), hMargin(0)
      , hPctPadding(0.0f), hPctMargin(0.0f)
    {}
  };
  virtual IntrinsicISizeOffsetData
    IntrinsicISizeOffsets(nsRenderingContext* aRenderingContext) = 0;

  virtual mozilla::IntrinsicSize GetIntrinsicSize() = 0;

  








  virtual nsSize GetIntrinsicRatio() = 0;

  


  enum ComputeSizeFlags {
    eDefault =           0,
    


    eShrinkWrap =        1 << 0,
    


    eUseAutoHeight =     1 << 1
  };

  







































  virtual mozilla::LogicalSize
  ComputeSize(nsRenderingContext *aRenderingContext,
              mozilla::WritingMode aWritingMode,
              const mozilla::LogicalSize& aCBSize,
              nscoord aAvailableISize,
              const mozilla::LogicalSize& aMargin,
              const mozilla::LogicalSize& aBorder,
              const mozilla::LogicalSize& aPadding,
              ComputeSizeFlags aFlags) = 0;

  












  virtual nsRect ComputeTightBounds(gfxContext* aContext) const;

  















  virtual nsresult GetPrefWidthTightBounds(nsRenderingContext* aContext,
                                           nscoord* aX,
                                           nscoord* aXMost);

  










































  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aReflowMetrics,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) = 0;

  














  virtual void DidReflow(nsPresContext*           aPresContext,
                         const nsHTMLReflowState* aReflowState,
                         nsDidReflowStatus        aStatus) = 0;

  

  




  virtual bool UpdateOverflow() = 0;

  








  virtual bool CanContinueTextRun() const = 0;

  














  virtual nsresult GetRenderedText(nsAString* aAppendToString = nullptr,
                                   gfxSkipChars* aSkipChars = nullptr,
                                   gfxSkipCharsIterator* aSkipIter = nullptr,
                                   uint32_t aSkippedStartOffset = 0,
                                   uint32_t aSkippedMaxLength = UINT32_MAX)
  { return NS_ERROR_NOT_IMPLEMENTED; }

  




  virtual bool HasAnyNoncollapsedCharacters()
  { return false; }

  




  bool HasView() const { return !!(mState & NS_FRAME_HAS_VIEW); }
  nsView* GetView() const;
  virtual nsView* GetViewExternal() const;
  nsresult SetView(nsView* aView);

  




  nsView* GetClosestView(nsPoint* aOffset = nullptr) const;

  


  nsIFrame* GetAncestorWithView() const;
  virtual nsIFrame* GetAncestorWithViewExternal() const;

  















  nsPoint GetOffsetTo(const nsIFrame* aOther) const;
  virtual nsPoint GetOffsetToExternal(const nsIFrame* aOther) const;

  



















  nsPoint GetOffsetToCrossDoc(const nsIFrame* aOther) const;

  



  nsPoint GetOffsetToCrossDoc(const nsIFrame* aOther, const int32_t aAPD) const;

  



  nsIntRect GetScreenRect() const;
  virtual nsIntRect GetScreenRectExternal() const;

  



  nsRect GetScreenRectInAppUnits() const;
  virtual nsRect GetScreenRectInAppUnitsExternal() const;

  



  void GetOffsetFromView(nsPoint& aOffset, nsView** aView) const;

  






  virtual nsIWidget* GetNearestWidget() const;

  




  virtual nsIWidget* GetNearestWidget(nsPoint& aOffset) const;

  




  virtual nsIAtom* GetType() const = 0;

  
















  Matrix4x4 GetTransformMatrix(const nsIFrame* aStopAtAncestor,
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
    eTablePart =                        1 << 13,
    
    
    
    eExcludesIgnorableWhitespace =      1 << 14,
    eSupportsCSSTransforms =            1 << 15,

    
    
    
    eDEBUGAllFrames =                   1 << 30,
    eDEBUGNoFrames =                    1 << 31
  };

  






  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
#ifdef DEBUG
    return !(aFlags & ~(nsIFrame::eDEBUGAllFrames | nsIFrame::eSupportsCSSTransforms));
#else
    return !(aFlags & ~nsIFrame::eSupportsCSSTransforms);
#endif
  }

  


  bool IsBlockWrapper() const;

  








  nsIFrame* GetContainingBlock() const;

  



  virtual bool IsFloatContainingBlock() const { return false; }

  








  virtual bool IsLeaf() const;

  










  virtual void InvalidateFrame(uint32_t aDisplayItemKey = 0);

  









  virtual void InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey = 0);
  
  










  void InvalidateFrameSubtree(uint32_t aDisplayItemKey = 0);

  



  virtual void InvalidateFrameForRemoval() {}

  




  static void* LayerIsPrerenderedDataKey() { 
    return &sLayerIsPrerenderedDataKey;
  }
  static uint8_t sLayerIsPrerenderedDataKey;

   







  bool TryUpdateTransformOnly(Layer** aLayerResult);

  








  bool IsInvalid(nsRect& aRect);
 
  



  bool HasInvalidFrameInSubtree()
  {
    return HasAnyStateBits(NS_FRAME_NEEDS_PAINT | NS_FRAME_DESCENDANT_NEEDS_PAINT);
  }

  



  void ClearInvalidationStateBits();

  



















  enum PaintType {
    PAINT_DEFAULT = 0,
    PAINT_COMPOSITE_ONLY,
    PAINT_DELAYED_COMPRESS
  };
  void SchedulePaint(PaintType aType = PAINT_DEFAULT);

  
















  enum {
    UPDATE_IS_ASYNC = 1 << 0
  };
  Layer* InvalidateLayer(uint32_t aDisplayItemKey,
                         const nsIntRect* aDamageRect = nullptr,
                         const nsRect* aFrameDamageRect = nullptr,
                         uint32_t aFlags = 0);

  





















  nsRect GetVisualOverflowRect() const {
    return GetOverflowRect(eVisualOverflow);
  }

  



















  nsRect GetScrollableOverflowRect() const {
    return GetOverflowRect(eScrollableOverflow);
  }

  nsRect GetOverflowRect(nsOverflowType aType) const;

  nsOverflowAreas GetOverflowAreas() const;

  






  nsOverflowAreas GetOverflowAreasRelativeToSelf() const;

  






  nsRect GetScrollableOverflowRectRelativeToParent() const;

  






  nsRect GetScrollableOverflowRectRelativeToSelf() const;

  






  nsRect GetVisualOverflowRectRelativeToSelf() const;

  






  nsRect GetVisualOverflowRectRelativeToParent() const;

  




  nsRect GetPreEffectsVisualOverflowRect() const;

  





  bool FinishAndStoreOverflow(nsOverflowAreas& aOverflowAreas,
                              nsSize aNewSize, nsSize* aOldSize = nullptr);

  bool FinishAndStoreOverflow(nsHTMLReflowMetrics* aMetrics) {
    return FinishAndStoreOverflow(aMetrics->mOverflowAreas,
                                  nsSize(aMetrics->Width(), aMetrics->Height()));
  }

  



  bool HasOverflowAreas() const {
    return mOverflow.mType != NS_FRAME_OVERFLOW_NONE;
  }

  



  bool ClearOverflowRects();

  













  Sides GetSkipSides(const nsHTMLReflowState* aReflowState = nullptr) const;
  virtual LogicalSides
  GetLogicalSkipSides(const nsHTMLReflowState* aReflowState = nullptr) const {
    return LogicalSides();
  }

  


  bool IsSelected() const;

  








  virtual nsresult  IsSelectable(bool* aIsSelectable, uint8_t* aSelectStyle) const = 0;

  




  virtual nsresult  GetSelectionController(nsPresContext *aPresContext, nsISelectionController **aSelCon) = 0;

  


  already_AddRefed<nsFrameSelection> GetFrameSelection();

  



  const nsFrameSelection* GetConstFrameSelection() const;

  






  virtual nsresult PeekOffset(nsPeekOffsetStruct *aPos);

  












  nsresult GetFrameFromDirection(nsDirection aDirection, bool aVisual,
                                 bool aJumpLines, bool aScrollViewStop,
                                 nsIFrame** aOutFrame, int32_t* aOutOffset,
                                 bool* aOutJumpedLine, bool* aOutMovedOverNonSelectableText);

  










  virtual nsresult CheckVisibility(nsPresContext* aContext, int32_t aStartIndex, int32_t aEndIndex, bool aRecurse, bool *aFinished, bool *_retval)=0;

  





  virtual void ChildIsDirty(nsIFrame* aChild) = 0;

  






#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() = 0;
#endif

  













  virtual nsStyleContext* GetParentStyleContext(nsIFrame** aProviderFrame) const = 0;

  



  bool IsVisibleForPainting(nsDisplayListBuilder* aBuilder);
  



  bool IsVisibleOrCollapsedForPainting(nsDisplayListBuilder* aBuilder);
  



  bool IsVisibleForPainting();
  



  bool IsVisibleInSelection(nsDisplayListBuilder* aBuilder);

  


  
  virtual bool IsVisibleInSelection(nsISelection* aSelection);

  





  bool IsPseudoStackingContextFromStyle();

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

  

















  virtual bool IsFocusable(int32_t *aTabIndex = nullptr, bool aWithMouse = false);

  
  
  
  bool IsBoxFrame() const
  {
    return IsFrameOfType(nsIFrame::eXULBox);
  }

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

  
  uint32_t GetOrdinal();

  virtual nscoord GetFlex(nsBoxLayoutState& aBoxLayoutState) = 0;
  virtual nscoord GetBoxAscent(nsBoxLayoutState& aBoxLayoutState) = 0;
  virtual bool IsCollapsed() = 0;
  
  
  
  
  
  virtual void SetBounds(nsBoxLayoutState& aBoxLayoutState, const nsRect& aRect,
                         bool aRemoveOverflowAreas = false) = 0;
  nsresult Layout(nsBoxLayoutState& aBoxLayoutState);
  
  
  virtual nsresult GetBorderAndPadding(nsMargin& aBorderAndPadding);
  virtual nsresult GetBorder(nsMargin& aBorder)=0;
  virtual nsresult GetPadding(nsMargin& aBorderAndPadding)=0;
  virtual nsresult GetMargin(nsMargin& aMargin)=0;
  virtual void SetLayoutManager(nsBoxLayout* aLayout) { }
  virtual nsBoxLayout* GetLayoutManager() { return nullptr; }
  nsresult GetClientRect(nsRect& aContentRect);

  
  virtual Valignment GetVAlign() const = 0;
  virtual Halignment GetHAlign() const = 0;

  bool IsHorizontal() const { return (mState & NS_STATE_IS_HORIZONTAL) != 0; }
  bool IsNormalDirection() const { return (mState & NS_STATE_IS_DIRECTION_NORMAL) != 0; }

  nsresult Redraw(nsBoxLayoutState& aState);
  virtual nsresult RelayoutChildAtOrdinal(nsBoxLayoutState& aState, nsIFrame* aChild)=0;
  
  virtual bool GetMouseThrough() const { return false; }

#ifdef DEBUG_LAYOUT
  virtual nsresult SetDebug(nsBoxLayoutState& aState, bool aDebug)=0;
  virtual nsresult GetDebug(bool& aDebug)=0;

  virtual nsresult DumpBox(FILE* out)=0;
#endif

  



  virtual bool HasSignificantTerminalNewline() const;

  static bool AddCSSPrefSize(nsIFrame* aBox, nsSize& aSize, bool& aWidth, bool& aHeightSet);
  static bool AddCSSMinSize(nsBoxLayoutState& aState, nsIFrame* aBox,
                            nsSize& aSize, bool& aWidth, bool& aHeightSet);
  static bool AddCSSMaxSize(nsIFrame* aBox, nsSize& aSize, bool& aWidth, bool& aHeightSet);
  static bool AddCSSFlex(nsBoxLayoutState& aState, nsIFrame* aBox, nscoord& aFlex);

  
  
  

  struct CaretPosition {
    CaretPosition();
    ~CaretPosition();

    nsCOMPtr<nsIContent> mResultContent;
    int32_t              mContentOffset;
  };

  









  CaretPosition GetExtremeCaretPosition(bool aStart);

  





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
  void MarkAsAbsoluteContainingBlock();
  void MarkAsNotAbsoluteContainingBlock();
  
  virtual mozilla::layout::FrameChildListID GetAbsoluteListID() const { return kAbsoluteList; }

  
  
  bool CheckAndClearPaintedState();

  
  
  
  
  
  
  
  
  enum {
    VISIBILITY_CROSS_CHROME_CONTENT_BOUNDARY = 0x01
  };
  bool IsVisibleConsideringAncestors(uint32_t aFlags = 0) const;

  struct FrameWithDistance
  {
    nsIFrame* mFrame;
    nscoord mXDistance;
    nscoord mYDistance;
  };

  



















  virtual void FindCloserFrameForSelection(nsPoint aPoint,
                                           FrameWithDistance* aCurrentBestFrame);

  


  inline bool IsFlexItem() const;
  


  inline bool IsFlexOrGridItem() const;

  


  inline bool IsTableCaption() const;

  inline bool IsBlockInside() const;
  inline bool IsBlockOutside() const;
  inline bool IsInlineOutside() const;
  inline uint8_t GetDisplay() const;
  inline bool IsFloating() const;
  inline bool IsAbsPosContaininingBlock() const;
  inline bool IsRelativelyPositioned() const;
  inline bool IsAbsolutelyPositioned() const;

  






  uint8_t VerticalAlignEnum() const;
  enum { eInvalidVerticalAlign = 0xFF };

  bool IsSVGText() const { return mState & NS_FRAME_IS_SVG_TEXT; }

  void CreateOwnLayerIfNeeded(nsDisplayListBuilder* aBuilder, nsDisplayList* aList);

  



  static void AddInPopupStateBitToDescendants(nsIFrame* aFrame);
  




  static void RemoveInPopupStateBitFromDescendants(nsIFrame* aFrame);

  






  template<bool IsLessThanOrEqual(nsIFrame*, nsIFrame*)>
  static void SortFrameList(nsFrameList& aFrameList);

  



  template<bool IsLessThanOrEqual(nsIFrame*, nsIFrame*)>
  static bool IsFrameListSorted(nsFrameList& aFrameList);

  



  bool FrameIsNonFirstInIBSplit() const {
    return (GetStateBits() & NS_FRAME_PART_OF_IBSPLIT) &&
      FirstContinuation()->Properties().Get(nsIFrame::IBSplitPrevSibling());
  }

  



  bool FrameIsNonLastInIBSplit() const {
    return (GetStateBits() & NS_FRAME_PART_OF_IBSPLIT) &&
      FirstContinuation()->Properties().Get(nsIFrame::IBSplitSibling());
  }

  



  bool IsContainerForFontSizeInflation() const {
    return GetStateBits() & NS_FRAME_FONT_INFLATION_CONTAINER;
  }

  




  virtual mozilla::dom::Element* GetPseudoElement(nsCSSPseudoElements::Type aType);

protected:
  
  nsRect           mRect;
  nsIContent*      mContent;
  nsStyleContext*  mStyleContext;
private:
  nsContainerFrame* mParent;
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
  void MarkInReflow() {
#ifdef DEBUG_dbaron_off
    
    NS_ASSERTION(!(mState & NS_FRAME_IN_REFLOW), "frame is already in reflow");
#endif
    mState |= NS_FRAME_IN_REFLOW;
  }

  nsFrameState     mState;

  
  
  
  
  
  
  
  
  
  struct VisualDeltas {
    uint8_t mLeft;
    uint8_t mTop;
    uint8_t mRight;
    uint8_t mBottom;
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
    uint32_t     mType;
    VisualDeltas mVisualDeltas;
  } mOverflow;

  
  









  virtual FrameSearchResult PeekOffsetNoAmount(bool aForward, int32_t* aOffset) = 0;
  
  












  virtual FrameSearchResult PeekOffsetCharacter(bool aForward, int32_t* aOffset,
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
  virtual FrameSearchResult PeekOffsetWord(bool aForward, bool aWordSelectEatSpace, bool aIsKeyboardSelect,
                                int32_t* aOffset, PeekWordState* aState) = 0;

  






  nsresult PeekOffsetParagraph(nsPeekOffsetStruct *aPos);

private:
  nsOverflowAreas* GetOverflowAreasProperty();
  nsRect GetVisualOverflowFromDeltas() const {
    MOZ_ASSERT(mOverflow.mType != NS_FRAME_OVERFLOW_LARGE,
               "should not be called when overflow is in a property");
    
    
    
    
    return nsRect(-(int32_t)mOverflow.mVisualDeltas.mLeft,
                  -(int32_t)mOverflow.mVisualDeltas.mTop,
                  mRect.width + mOverflow.mVisualDeltas.mRight +
                                mOverflow.mVisualDeltas.mLeft,
                  mRect.height + mOverflow.mVisualDeltas.mBottom +
                                 mOverflow.mVisualDeltas.mTop);
  }
  


  bool SetOverflowAreas(const nsOverflowAreas& aOverflowAreas);

  
  template<bool IsLessThanOrEqual(nsIFrame*, nsIFrame*)>
  static nsIFrame* SortedMerge(nsIFrame *aLeft, nsIFrame *aRight);

  template<bool IsLessThanOrEqual(nsIFrame*, nsIFrame*)>
  static nsIFrame* MergeSort(nsIFrame *aSource);

  bool HasOpacityInternal(float aThreshold) const;

#ifdef DEBUG_FRAME_DUMP
public:
  static void IndentBy(FILE* out, int32_t aIndent) {
    while (--aIndent >= 0) fputs("  ", out);
  }
  void ListTag(FILE* out) const {
    ListTag(out, this);
  }
  static void ListTag(FILE* out, const nsIFrame* aFrame) {
    nsAutoCString t;
    ListTag(t, aFrame);
    fputs(t.get(), out);
  }
  void ListTag(nsACString& aTo) const;
  static void ListTag(nsACString& aTo, const nsIFrame* aFrame);
  void ListGeneric(nsACString& aTo, const char* aPrefix = "", uint32_t aFlags = 0) const;
  enum {
    TRAVERSE_SUBDOCUMENT_FRAMES = 0x01
  };
  virtual void List(FILE* out = stderr, const char* aPrefix = "", uint32_t aFlags = 0) const;
  



  static void RootFrameList(nsPresContext* aPresContext,
                            FILE* out = stderr, const char* aPrefix = "");
  virtual void DumpFrameTree();
  void DumpFrameTreeLimited();

  virtual nsresult  GetFrameName(nsAString& aResult) const = 0;
#endif

#ifdef DEBUG
public:
  virtual nsFrameState  GetDebugStateBits() const = 0;
  virtual nsresult  DumpRegressionData(nsPresContext* aPresContext,
                                       FILE* out, int32_t aIndent) = 0;
#endif
};
















class nsWeakFrame {
public:
  nsWeakFrame() : mPrev(nullptr), mFrame(nullptr) { }

  nsWeakFrame(const nsWeakFrame& aOther) : mPrev(nullptr), mFrame(nullptr)
  {
    Init(aOther.GetFrame());
  }

  MOZ_IMPLICIT nsWeakFrame(nsIFrame* aFrame) : mPrev(nullptr), mFrame(nullptr)
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
  void Init(nsIFrame* aFrame);

  nsWeakFrame*  mPrev;
  nsIFrame*     mFrame;
};

inline bool
nsFrameList::ContinueRemoveFrame(nsIFrame* aFrame)
{
  MOZ_ASSERT(!aFrame->GetPrevSibling() || !aFrame->GetNextSibling(),
             "Forgot to call StartRemoveFrame?");
  if (aFrame == mLastChild) {
    MOZ_ASSERT(!aFrame->GetNextSibling(), "broken frame list");
    nsIFrame* prevSibling = aFrame->GetPrevSibling();
    if (!prevSibling) {
      MOZ_ASSERT(aFrame == mFirstChild, "broken frame list");
      mFirstChild = mLastChild = nullptr;
      return true;
    }
    MOZ_ASSERT(prevSibling->GetNextSibling() == aFrame, "Broken frame linkage");
    prevSibling->SetNextSibling(nullptr);
    mLastChild = prevSibling;
    return true;
  }
  if (aFrame == mFirstChild) {
    MOZ_ASSERT(!aFrame->GetPrevSibling(), "broken frame list");
    mFirstChild = aFrame->GetNextSibling();
    aFrame->SetNextSibling(nullptr);
    MOZ_ASSERT(mFirstChild, "broken frame list");
    return true;
  }
  return false;
}

inline bool
nsFrameList::StartRemoveFrame(nsIFrame* aFrame)
{
  if (aFrame->GetPrevSibling() && aFrame->GetNextSibling()) {
    UnhookFrameFromSiblings(aFrame);
    return true;
  }
  return ContinueRemoveFrame(aFrame);
}

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

inline void
nsFrameList::FrameLinkEnumerator::Next()
{
  mPrev = mFrame;
  Enumerator::Next();
}




inline nsFrameList::Iterator&
nsFrameList::Iterator::operator++()
{
  mCurrent = mCurrent->GetNextSibling();
  return *this;
}

inline nsFrameList::Iterator&
nsFrameList::Iterator::operator--()
{
  if (!mCurrent) {
    mCurrent = mList.LastChild();
  } else {
    mCurrent = mCurrent->GetPrevSibling();
  }
  return *this;
}




template<bool IsLessThanOrEqual(nsIFrame*, nsIFrame*)>
 nsIFrame*
nsIFrame::SortedMerge(nsIFrame *aLeft, nsIFrame *aRight)
{
  NS_PRECONDITION(aLeft && aRight, "SortedMerge must have non-empty lists");

  nsIFrame *result;
  
  if (IsLessThanOrEqual(aLeft, aRight)) {
    result = aLeft;
    aLeft = aLeft->GetNextSibling();
    if (!aLeft) {
      result->SetNextSibling(aRight);
      return result;
    }
  }
  else {
    result = aRight;
    aRight = aRight->GetNextSibling();
    if (!aRight) {
      result->SetNextSibling(aLeft);
      return result;
    }
  }

  nsIFrame *last = result;
  for (;;) {
    if (IsLessThanOrEqual(aLeft, aRight)) {
      last->SetNextSibling(aLeft);
      last = aLeft;
      aLeft = aLeft->GetNextSibling();
      if (!aLeft) {
        last->SetNextSibling(aRight);
        return result;
      }
    }
    else {
      last->SetNextSibling(aRight);
      last = aRight;
      aRight = aRight->GetNextSibling();
      if (!aRight) {
        last->SetNextSibling(aLeft);
        return result;
      }
    }
  }
}

template<bool IsLessThanOrEqual(nsIFrame*, nsIFrame*)>
 nsIFrame*
nsIFrame::MergeSort(nsIFrame *aSource)
{
  NS_PRECONDITION(aSource, "MergeSort null arg");

  nsIFrame *sorted[32] = { nullptr };
  nsIFrame **fill = &sorted[0];
  nsIFrame **left;
  nsIFrame *rest = aSource;

  do {
    nsIFrame *current = rest;
    rest = rest->GetNextSibling();
    current->SetNextSibling(nullptr);

    
    
    
    
    for (left = &sorted[0]; left != fill && *left; ++left) {
      current = SortedMerge<IsLessThanOrEqual>(*left, current);
      *left = nullptr;
    }

    
    *left = current;

    if (left == fill)
      ++fill;
  } while (rest);

  
  nsIFrame *result = nullptr;
  for (left = &sorted[0]; left != fill; ++left) {
    if (*left) {
      result = result ? SortedMerge<IsLessThanOrEqual>(*left, result) : *left;
    }
  }
  return result;
}

template<bool IsLessThanOrEqual(nsIFrame*, nsIFrame*)>
 void
nsIFrame::SortFrameList(nsFrameList& aFrameList)
{
  nsIFrame* head = MergeSort<IsLessThanOrEqual>(aFrameList.FirstChild());
  aFrameList = nsFrameList(head, nsLayoutUtils::GetLastSibling(head));
  MOZ_ASSERT(IsFrameListSorted<IsLessThanOrEqual>(aFrameList),
             "After we sort a frame list, it should be in sorted order...");
}

template<bool IsLessThanOrEqual(nsIFrame*, nsIFrame*)>
 bool
nsIFrame::IsFrameListSorted(nsFrameList& aFrameList)
{
  if (aFrameList.IsEmpty()) {
    
    return true;
  }

  
  
  nsFrameList::Enumerator trailingIter(aFrameList);
  nsFrameList::Enumerator iter(aFrameList);
  iter.Next(); 

  
  while (!iter.AtEnd()) {
    MOZ_ASSERT(!trailingIter.AtEnd(), "trailing iter shouldn't finish first");
    if (!IsLessThanOrEqual(trailingIter.get(), iter.get())) {
      return false;
    }
    trailingIter.Next();
    iter.Next();
  }

  
  return true;
}

#endif 
