




#ifndef nsLayoutUtils_h__
#define nsLayoutUtils_h__

#include "mozilla/MemoryReporting.h"
#include "mozilla/ArrayUtils.h"
#include "nsBoundingMetrics.h"
#include "nsChangeHint.h"
#include "nsAutoPtr.h"
#include "nsFrameList.h"
#include "mozilla/layout/FrameChildList.h"
#include "nsThreadUtils.h"
#include "nsIPrincipal.h"
#include "GraphicsFilter.h"
#include "nsCSSPseudoElements.h"
#include "FrameMetrics.h"
#include "gfx3DMatrix.h"
#include "nsIWidget.h"
#include "nsCSSProperty.h"
#include "nsStyleCoord.h"
#include "nsStyleConsts.h"
#include "nsGkAtoms.h"
#include "nsRuleNode.h"
#include "imgIContainer.h"
#include "mozilla/gfx/2D.h"
#include "Units.h"
#include "mozilla/ToString.h"
#include "nsHTMLReflowMetrics.h"

#include <limits>
#include <algorithm>

class nsIFormControlFrame;
class nsPresContext;
class nsIContent;
class nsIAtom;
class nsIScrollableFrame;
class nsIDOMEvent;
class nsRegion;
class nsDisplayListBuilder;
class nsDisplayItem;
class nsFontMetrics;
class nsFontFaceList;
class nsIImageLoadingContent;
class nsStyleContext;
class nsBlockFrame;
class nsContainerFrame;
class nsView;
class nsIFrame;
class nsStyleCoord;
class nsStyleCorners;
class gfxContext;
class nsPIDOMWindow;
class imgIRequest;
class nsIDocument;
struct gfxPoint;
struct nsStyleFont;
struct nsStyleImageOrientation;
struct nsOverflowAreas;

namespace mozilla {
class EventListenerManager;
class SVGImageContext;
struct IntrinsicSize;
struct ContainerLayerParameters;
class WritingMode;
namespace dom {
class DOMRectList;
class Element;
class HTMLImageElement;
class HTMLCanvasElement;
class HTMLVideoElement;
} 
namespace gfx {
struct RectCornerRadii;
} 
namespace layers {
class Layer;
class ClientLayerManager;
}
}

namespace mozilla {

struct DisplayPortPropertyData {
  DisplayPortPropertyData(const nsRect& aRect, uint32_t aPriority)
    : mRect(aRect)
    , mPriority(aPriority)
  {}
  nsRect mRect;
  uint32_t mPriority;
};

struct DisplayPortMarginsPropertyData {
  DisplayPortMarginsPropertyData(const ScreenMargin& aMargins,
                                 uint32_t aPriority)
    : mMargins(aMargins)
    , mPriority(aPriority)
  {}
  ScreenMargin mMargins;
  uint32_t mPriority;
};

} 






class nsLayoutUtils
{
  typedef ::GraphicsFilter GraphicsFilter;
  typedef mozilla::dom::DOMRectList DOMRectList;
  typedef mozilla::layers::Layer Layer;
  typedef mozilla::ContainerLayerParameters ContainerLayerParameters;
  typedef mozilla::IntrinsicSize IntrinsicSize;
  typedef mozilla::gfx::SourceSurface SourceSurface;
  typedef mozilla::gfx::Color Color;
  typedef mozilla::gfx::DrawTarget DrawTarget;
  typedef mozilla::gfx::Float Float;
  typedef mozilla::gfx::Point Point;
  typedef mozilla::gfx::Rect Rect;
  typedef mozilla::gfx::Matrix4x4 Matrix4x4;
  typedef mozilla::gfx::RectCornerRadii RectCornerRadii;
  typedef mozilla::gfx::StrokeOptions StrokeOptions;
  typedef mozilla::image::DrawResult DrawResult;

public:
  typedef mozilla::layers::FrameMetrics FrameMetrics;
  typedef FrameMetrics::ViewID ViewID;
  typedef mozilla::CSSPoint CSSPoint;
  typedef mozilla::CSSSize CSSSize;
  typedef mozilla::CSSIntSize CSSIntSize;
  typedef mozilla::ScreenMargin ScreenMargin;
  typedef mozilla::LayoutDeviceIntSize LayoutDeviceIntSize;

  



  static bool FindIDFor(const nsIContent* aContent, ViewID* aOutViewId);

  



  static ViewID FindOrCreateIDFor(nsIContent* aContent);

  


  static nsIContent* FindContentFor(ViewID aId);

  


  static nsIScrollableFrame* FindScrollableFrameFor(ViewID aId);

  


  static bool GetDisplayPort(nsIContent* aContent, nsRect *aResult = nullptr);

  enum class RepaintMode : uint8_t {
    Repaint,
    DoNotRepaint
  };

  














  static bool SetDisplayPortMargins(nsIContent* aContent,
                                    nsIPresShell* aPresShell,
                                    const ScreenMargin& aMargins,
                                    uint32_t aPriority = 0,
                                    RepaintMode aRepaintMode = RepaintMode::Repaint);

  





  static void SetDisplayPortBase(nsIContent* aContent, const nsRect& aBase);
  static void SetDisplayPortBaseIfNotSet(nsIContent* aContent, const nsRect& aBase);

  


  static bool GetCriticalDisplayPort(nsIContent* aContent, nsRect* aResult = nullptr);

  



  static mozilla::layout::FrameChildListID GetChildListNameFor(nsIFrame* aChildFrame);

  








  static nsIFrame* GetBeforeFrameForContent(nsIFrame* aGenConParentFrame,
                                            nsIContent* aContent);

  







  static nsIFrame* GetBeforeFrame(nsIFrame* aFrame);

  









  static nsIFrame* GetAfterFrameForContent(nsIFrame* aGenConParentFrame,
                                           nsIContent* aContent);

  








  static nsIFrame* GetAfterFrame(nsIFrame* aFrame);

  








  static nsIFrame* GetClosestFrameOfType(nsIFrame* aFrame, nsIAtom* aFrameType);

  







  static nsIFrame* GetPageFrame(nsIFrame* aFrame)
  {
    return GetClosestFrameOfType(aFrame, nsGkAtoms::pageFrame);
  }

  





  static nsIFrame* GetStyleFrame(nsIFrame* aPrimaryFrame);

  





  static nsIFrame* GetStyleFrame(const nsIContent* aContent);

  













  static bool IsGeneratedContentFor(nsIContent* aContent, nsIFrame* aFrame,
                                      nsIAtom* aPseudoElement);

#ifdef DEBUG
  
  static bool gPreventAssertInCompareTreePosition;
#endif 

  














  static int32_t CompareTreePosition(nsIContent* aContent1,
                                     nsIContent* aContent2,
                                     const nsIContent* aCommonAncestor = nullptr)
  {
    return DoCompareTreePosition(aContent1, aContent2, -1, 1, aCommonAncestor);
  }

  





  static int32_t DoCompareTreePosition(nsIContent* aContent1,
                                       nsIContent* aContent2,
                                       int32_t aIf1Ancestor,
                                       int32_t aIf2Ancestor,
                                       const nsIContent* aCommonAncestor = nullptr);

  


















  static int32_t CompareTreePosition(nsIFrame* aFrame1,
                                     nsIFrame* aFrame2,
                                     nsIFrame* aCommonAncestor = nullptr)
  {
    return DoCompareTreePosition(aFrame1, aFrame2, -1, 1, aCommonAncestor);
  }

  static int32_t CompareTreePosition(nsIFrame* aFrame1,
                                     nsIFrame* aFrame2,
                                     nsTArray<nsIFrame*>& aFrame2Ancestors,
                                     nsIFrame* aCommonAncestor = nullptr)
  {
    return DoCompareTreePosition(aFrame1, aFrame2, aFrame2Ancestors,
                                 -1, 1, aCommonAncestor);
  }

  





  static int32_t DoCompareTreePosition(nsIFrame* aFrame1,
                                       nsIFrame* aFrame2,
                                       int32_t aIf1Ancestor,
                                       int32_t aIf2Ancestor,
                                       nsIFrame* aCommonAncestor = nullptr);

  static nsIFrame* FillAncestors(nsIFrame* aFrame,
                                 nsIFrame* aStopAtAncestor,
                                 nsTArray<nsIFrame*>* aAncestors);

  static int32_t DoCompareTreePosition(nsIFrame* aFrame1,
                                       nsIFrame* aFrame2,
                                       nsTArray<nsIFrame*>& aFrame2Ancestors,
                                       int32_t aIf1Ancestor,
                                       int32_t aIf2Ancestor,
                                       nsIFrame* aCommonAncestor);

  



  static nsContainerFrame* LastContinuationWithChild(nsContainerFrame* aFrame);

  



  static nsIFrame* GetLastSibling(nsIFrame* aFrame);

  






  static nsView* FindSiblingViewFor(nsView* aParentView, nsIFrame* aFrame);

  









  static nsIFrame* GetCrossDocParentFrame(const nsIFrame* aFrame,
                                          nsPoint* aCrossDocOffset = nullptr);

  






  static bool IsProperAncestorFrame(nsIFrame* aAncestorFrame, nsIFrame* aFrame,
                                      nsIFrame* aCommonAncestor = nullptr);

  





  static bool IsProperAncestorFrameCrossDoc(nsIFrame* aAncestorFrame, nsIFrame* aFrame,
                                              nsIFrame* aCommonAncestor = nullptr);

  









  static bool IsAncestorFrameCrossDoc(const nsIFrame* aAncestorFrame, const nsIFrame* aFrame,
                                        const nsIFrame* aCommonAncestor = nullptr);

  









  static void SetFixedPositionLayerData(Layer* aLayer, const nsIFrame* aViewportFrame,
                                        const nsRect& aAnchorRect,
                                        const nsIFrame* aFixedPosFrame,
                                        nsPresContext* aPresContext,
                                        const ContainerLayerParameters& aContainerParameters);

  



  static bool ViewportHasDisplayPort(nsPresContext* aPresContext,
                                     nsRect* aDisplayPort = nullptr);

  





  static bool IsFixedPosFrameInDisplayPort(const nsIFrame* aFrame,
                                           nsRect* aDisplayPort = nullptr);

  



  static void SetScrollbarThumbLayerization(nsIFrame* aThumbFrame, bool aLayerize);

  



  static bool IsScrollbarThumbLayerized(nsIFrame* aThumbFrame);

  













  static nsIFrame* GetAnimatedGeometryRootFor(nsDisplayItem* aItem,
                                              nsDisplayListBuilder* aBuilder,
                                              mozilla::layers::LayerManager* aManager);

  




  static nsIFrame* GetAnimatedGeometryRootForFrame(nsDisplayListBuilder* aBuilder,
                                                   nsIFrame* aFrame,
                                                   const nsIFrame* aStopAtAncestor);

  


  static nsIScrollableFrame* GetScrollableFrameFor(const nsIFrame *aScrolledFrame);

  











  enum Direction { eHorizontal, eVertical };
  static nsIScrollableFrame* GetNearestScrollableFrameForDirection(nsIFrame* aFrame,
                                                                   Direction aDirection);

  enum {
    



    SCROLLABLE_SAME_DOC = 0x01,
    



    SCROLLABLE_INCLUDE_HIDDEN = 0x02,
    




    SCROLLABLE_ONLY_ASYNC_SCROLLABLE = 0x04,
    




    SCROLLABLE_ALWAYS_MATCH_ROOT = 0x08,
  };
  










  static nsIScrollableFrame* GetNearestScrollableFrame(nsIFrame* aFrame,
                                                       uint32_t aFlags = 0);

  





  static nsRect GetScrolledRect(nsIFrame* aScrolledFrame,
                                const nsRect& aScrolledFrameOverflowArea,
                                const nsSize& aScrollPortSize,
                                uint8_t aDirection);

  










  static bool HasPseudoStyle(nsIContent* aContent,
                               nsStyleContext* aStyleContext,
                               nsCSSPseudoElements::Type aPseudoElement,
                               nsPresContext* aPresContext);

  



  static nsIFrame* GetFloatFromPlaceholder(nsIFrame* aPlaceholder);

  
  
  static uint8_t CombineBreakType(uint8_t aOrigBreakType, uint8_t aNewBreakType);

  








  static nsPoint GetDOMEventCoordinatesRelativeTo(nsIDOMEvent* aDOMEvent,
                                                  nsIFrame* aFrame);

  








  static nsPoint GetEventCoordinatesRelativeTo(
                   const mozilla::WidgetEvent* aEvent,
                   nsIFrame* aFrame);

  









  static nsPoint GetEventCoordinatesRelativeTo(
                   const mozilla::WidgetEvent* aEvent,
                   const mozilla::LayoutDeviceIntPoint& aPoint,
                   nsIFrame* aFrame);

  









  static nsPoint GetEventCoordinatesRelativeTo(nsIWidget* aWidget,
                                               const mozilla::LayoutDeviceIntPoint& aPoint,
                                               nsIFrame* aFrame);

  






  static nsIFrame* GetPopupFrameForEventCoordinates(
                     nsPresContext* aPresContext,
                     const mozilla::WidgetEvent* aEvent);

  







  static nsPoint TranslateWidgetToView(nsPresContext* aPresContext,
                                       nsIWidget* aWidget,
                                       const mozilla::LayoutDeviceIntPoint& aPt,
                                       nsView* aView);

  







  static mozilla::LayoutDeviceIntPoint
    TranslateViewToWidget(nsPresContext* aPresContext,
                          nsView* aView, nsPoint aPt,
                          nsIWidget* aWidget);

  enum FrameForPointFlags {
    



    IGNORE_PAINT_SUPPRESSION = 0x01,
    



    IGNORE_ROOT_SCROLL_FRAME = 0x02,
    


    IGNORE_CROSS_DOC = 0x04
  };

  






  static nsIFrame* GetFrameForPoint(nsIFrame* aFrame, nsPoint aPt,
                                    uint32_t aFlags = 0);

  







  static nsresult GetFramesForArea(nsIFrame* aFrame, const nsRect& aRect,
                                   nsTArray<nsIFrame*> &aOutFrames,
                                   uint32_t aFlags = 0);

  





  static nsRect TransformFrameRectToAncestor(nsIFrame* aFrame,
                                             const nsRect& aRect,
                                             const nsIFrame* aAncestor,
                                             bool* aPreservesAxisAlignedRectangles = nullptr);


  



  static Matrix4x4 GetTransformToAncestor(nsIFrame *aFrame, const nsIFrame *aAncestor);

  



  static gfxSize GetTransformToAncestorScale(nsIFrame* aFrame);

  



  static nsIFrame* FindNearestCommonAncestorFrame(nsIFrame* aFrame1,
                                                  nsIFrame* aFrame2);

  









  enum TransformResult {
    TRANSFORM_SUCCEEDED,
    NO_COMMON_ANCESTOR,
    NONINVERTIBLE_TRANSFORM
  };
  static TransformResult TransformPoints(nsIFrame* aFromFrame, nsIFrame* aToFrame,
                                         uint32_t aPointCount, CSSPoint* aPoints);

  



  static TransformResult TransformPoint(nsIFrame* aFromFrame, nsIFrame* aToFrame,
                                        nsPoint& aPoint);

  




  static TransformResult TransformRect(nsIFrame* aFromFrame, nsIFrame* aToFrame,
                                       nsRect& aRect);

  



  static nsRect GetRectRelativeToFrame(mozilla::dom::Element* aElement,
                                       nsIFrame* aFrame);

  



  static bool ContainsPoint(const nsRect& aRect, const nsPoint& aPoint,
                            nscoord aInflateSize);

  



  static bool IsRectVisibleInScrollFrames(nsIFrame* aFrame,
                                          const nsRect& aRect);

  





  static bool GetLayerTransformForFrame(nsIFrame* aFrame,
                                        Matrix4x4* aTransform);

  








  static nsPoint TransformRootPointToFrame(nsIFrame* aFrame,
                                           const nsPoint &aPoint)
  {
    return TransformAncestorPointToFrame(aFrame, aPoint, nullptr);
  }

  



  static nsPoint TransformAncestorPointToFrame(nsIFrame* aFrame,
                                               const nsPoint& aPoint,
                                               nsIFrame* aAncestor);

  








  static nsRect MatrixTransformRect(const nsRect &aBounds,
                                    const gfx3DMatrix &aMatrix, float aFactor);

  









  static nsRect MatrixTransformRectOut(const nsRect &aBounds,
                                    const gfx3DMatrix &aMatrix, float aFactor);
  








  static nsPoint MatrixTransformPoint(const nsPoint &aPoint,
                                      const gfx3DMatrix &aMatrix, float aFactor);

  







  static nsRect RoundGfxRectToAppRect(const Rect &aRect, float aFactor);

  







  static nsRect RoundGfxRectToAppRect(const gfxRect &aRect, float aFactor);

  




  static nsRegion RoundedRectIntersectRect(const nsRect& aRoundedRect,
                                           const nscoord aRadii[8],
                                           const nsRect& aContainedRect);
  static nsIntRegion RoundedRectIntersectIntRect(const nsIntRect& aRoundedRect,
                                                 const RectCornerRadii& aCornerRadii,
                                                 const nsIntRect& aContainedRect);

  




  static bool RoundedRectIntersectsRect(const nsRect& aRoundedRect,
                                        const nscoord aRadii[8],
                                        const nsRect& aTestRect);

  enum {
    PAINT_IN_TRANSFORM = 0x01,
    PAINT_SYNC_DECODE_IMAGES = 0x02,
    PAINT_WIDGET_LAYERS = 0x04,
    PAINT_IGNORE_SUPPRESSION = 0x08,
    PAINT_DOCUMENT_RELATIVE = 0x10,
    PAINT_HIDE_CARET = 0x20,
    PAINT_ALL_CONTINUATIONS = 0x40,
    PAINT_TO_WINDOW = 0x80,
    PAINT_EXISTING_TRANSACTION = 0x100,
    PAINT_NO_COMPOSITE = 0x200,
    PAINT_COMPRESSED = 0x400
  };

  











































  static nsresult PaintFrame(nsRenderingContext* aRenderingContext, nsIFrame* aFrame,
                             const nsRegion& aDirtyRegion, nscolor aBackstop,
                             uint32_t aFlags = 0);

  












  static bool
  BinarySearchForPosition(nsRenderingContext* acx,
                          nsFontMetrics& aFontMetrics,
                          const char16_t* aText,
                          int32_t    aBaseWidth,
                          int32_t    aBaseInx,
                          int32_t    aStartInx,
                          int32_t    aEndInx,
                          int32_t    aCursorPos,
                          int32_t&   aIndex,
                          int32_t&   aTextWidth);

  class BoxCallback {
  public:
    virtual void AddBox(nsIFrame* aFrame) = 0;
  };
  






  static void GetAllInFlowBoxes(nsIFrame* aFrame, BoxCallback* aCallback);

  



  static nsIFrame* GetFirstNonAnonymousFrame(nsIFrame* aFrame);

  class RectCallback {
  public:
    virtual void AddRect(const nsRect& aRect) = 0;
  };

  struct RectAccumulator : public RectCallback {
    nsRect mResultRect;
    nsRect mFirstRect;
    bool mSeenFirstRect;

    RectAccumulator();

    virtual void AddRect(const nsRect& aRect) override;
  };

  struct RectListBuilder : public RectCallback {
    DOMRectList* mRectList;

    explicit RectListBuilder(DOMRectList* aList);
    virtual void AddRect(const nsRect& aRect) override;
  };

  static nsIFrame* GetContainingBlockForClientRect(nsIFrame* aFrame);

  enum {
    RECTS_ACCOUNT_FOR_TRANSFORMS = 0x01,
    
    
    RECTS_USE_CONTENT_BOX = 0x02,
    RECTS_USE_PADDING_BOX = 0x04,
    RECTS_USE_MARGIN_BOX = 0x06, 
    RECTS_WHICH_BOX_MASK = 0x06 
  };
  














  static void GetAllInFlowRects(nsIFrame* aFrame, nsIFrame* aRelativeTo,
                                RectCallback* aCallback, uint32_t aFlags = 0);

  









  static nsRect GetAllInFlowRectsUnion(nsIFrame* aFrame, nsIFrame* aRelativeTo,
                                       uint32_t aFlags = 0);

  enum {
    EXCLUDE_BLUR_SHADOWS = 0x01
  };
  




  static nsRect GetTextShadowRectsUnion(const nsRect& aTextAndDecorationsRect,
                                        nsIFrame* aFrame,
                                        uint32_t aFlags = 0);

  























  static nsRect ComputeObjectDestRect(const nsRect& aConstraintRect,
                                      const IntrinsicSize& aIntrinsicSize,
                                      const nsSize& aIntrinsicRatio,
                                      const nsStylePosition* aStylePos,
                                      nsPoint* aAnchorPoint = nullptr);

  






  static nsresult GetFontMetricsForFrame(const nsIFrame* aFrame,
                                         nsFontMetrics** aFontMetrics,
                                         float aSizeInflation = 1.0f);

  






  static nsresult GetFontMetricsForStyleContext(nsStyleContext* aStyleContext,
                                                nsFontMetrics** aFontMetrics,
                                                float aSizeInflation = 1.0f);

  




  static nsIFrame* FindChildContainingDescendant(nsIFrame* aParent, nsIFrame* aDescendantFrame);

  


  static nsBlockFrame* FindNearestBlockAncestor(nsIFrame* aFrame);

  



  static nsIFrame* GetNonGeneratedAncestor(nsIFrame* aFrame);

  



  static nsBlockFrame* GetAsBlock(nsIFrame* aFrame);

  


  static bool IsNonWrapperBlock(nsIFrame* aFrame);

  



  static nsIFrame* GetParentOrPlaceholderFor(nsIFrame* aFrame);

  



  static nsIFrame* GetParentOrPlaceholderForCrossDoc(nsIFrame* aFrame);

  



  static nsIFrame*
  GetNextContinuationOrIBSplitSibling(nsIFrame *aFrame);

  



  static nsIFrame*
  FirstContinuationOrIBSplitSibling(nsIFrame *aFrame);

  



  static nsIFrame*
  LastContinuationOrIBSplitSibling(nsIFrame *aFrame);

  



  static bool
  IsFirstContinuationOrIBSplitSibling(nsIFrame *aFrame);

  






  static bool IsViewportScrollbarFrame(nsIFrame* aFrame);

  





  enum IntrinsicISizeType { MIN_ISIZE, PREF_ISIZE };
  enum {
    IGNORE_PADDING = 0x01
  };
  static nscoord IntrinsicForContainer(nsRenderingContext* aRenderingContext,
                                       nsIFrame* aFrame,
                                       IntrinsicISizeType aType,
                                       uint32_t aFlags = 0);

  





  static nscoord ComputeCBDependentValue(nscoord aPercentBasis,
                                         const nsStyleCoord& aCoord);

  
















  
  static nscoord ComputeWidthValue(
                   nsRenderingContext* aRenderingContext,
                   nsIFrame*            aFrame,
                   nscoord              aContainingBlockWidth,
                   nscoord              aContentEdgeToBoxSizing,
                   nscoord              aBoxSizingToMarginEdge,
                   const nsStyleCoord&  aCoord)
  {
    return ComputeISizeValue(aRenderingContext,
                             aFrame,
                             aContainingBlockWidth,
                             aContentEdgeToBoxSizing,
                             aBoxSizingToMarginEdge,
                             aCoord);
  }

  static nscoord ComputeISizeValue(
                   nsRenderingContext* aRenderingContext,
                   nsIFrame*           aFrame,
                   nscoord             aContainingBlockISize,
                   nscoord             aContentEdgeToBoxSizing,
                   nscoord             aBoxSizingToMarginEdge,
                   const nsStyleCoord& aCoord);

  



  
  static nscoord ComputeHeightDependentValue(
                   nscoord              aContainingBlockHeight,
                   const nsStyleCoord&  aCoord)
  {
    return ComputeBSizeDependentValue(aContainingBlockHeight, aCoord);
  }

  static nscoord ComputeBSizeDependentValue(
                   nscoord              aContainingBlockBSize,
                   const nsStyleCoord&  aCoord);

  


  
  static nscoord ComputeHeightValue(nscoord aContainingBlockHeight,
                                    nscoord aContentEdgeToBoxSizingBoxEdge,
                                    const nsStyleCoord& aCoord)
  {
    return ComputeBSizeValue(aContainingBlockHeight,
                             aContentEdgeToBoxSizingBoxEdge,
                             aCoord);
  }

  static nscoord ComputeBSizeValue(nscoord aContainingBlockBSize,
                                    nscoord aContentEdgeToBoxSizingBoxEdge,
                                    const nsStyleCoord& aCoord)
  {
    MOZ_ASSERT(aContainingBlockBSize != nscoord_MAX || !aCoord.HasPercent(),
               "caller must deal with %% of unconstrained block-size");
    MOZ_ASSERT(aCoord.IsCoordPercentCalcUnit());

    nscoord result =
      nsRuleNode::ComputeCoordPercentCalc(aCoord, aContainingBlockBSize);
    
    return std::max(0, result - aContentEdgeToBoxSizingBoxEdge);
  }

  
  static bool IsAutoHeight(const nsStyleCoord &aCoord, nscoord aCBHeight)
  {
    return IsAutoBSize(aCoord, aCBHeight);
  }

  static bool IsAutoBSize(const nsStyleCoord &aCoord, nscoord aCBBSize)
  {
    nsStyleUnit unit = aCoord.GetUnit();
    return unit == eStyleUnit_Auto ||  
           unit == eStyleUnit_None ||  
           
           
           
           
           
           
           
           
           
           unit == eStyleUnit_Enumerated ||
           (aCBBSize == nscoord_MAX && aCoord.HasPercent());
  }

  static bool IsPaddingZero(const nsStyleCoord &aCoord)
  {
    return (aCoord.GetUnit() == eStyleUnit_Coord &&
            aCoord.GetCoordValue() == 0) ||
           (aCoord.GetUnit() == eStyleUnit_Percent &&
            aCoord.GetPercentValue() == 0.0f) ||
           (aCoord.IsCalcUnit() &&
            
            nsRuleNode::ComputeCoordPercentCalc(aCoord, nscoord_MAX) <= 0 &&
            nsRuleNode::ComputeCoordPercentCalc(aCoord, 0) <= 0);
  }

  static bool IsMarginZero(const nsStyleCoord &aCoord)
  {
    return (aCoord.GetUnit() == eStyleUnit_Coord &&
            aCoord.GetCoordValue() == 0) ||
           (aCoord.GetUnit() == eStyleUnit_Percent &&
            aCoord.GetPercentValue() == 0.0f) ||
           (aCoord.IsCalcUnit() &&
            nsRuleNode::ComputeCoordPercentCalc(aCoord, nscoord_MAX) == 0 &&
            nsRuleNode::ComputeCoordPercentCalc(aCoord, 0) == 0);
  }

  static void MarkDescendantsDirty(nsIFrame *aSubtreeRoot);

  




  static mozilla::LogicalSize
  ComputeSizeWithIntrinsicDimensions(mozilla::WritingMode aWM,
                    nsRenderingContext* aRenderingContext, nsIFrame* aFrame,
                    const mozilla::IntrinsicSize& aIntrinsicSize,
                    nsSize aIntrinsicRatio,
                    const mozilla::LogicalSize& aCBSize,
                    const mozilla::LogicalSize& aMargin,
                    const mozilla::LogicalSize& aBorder,
                    const mozilla::LogicalSize& aPadding);

  





  static nsSize ComputeAutoSizeWithIntrinsicDimensions(nscoord minWidth, nscoord minHeight,
                                                       nscoord maxWidth, nscoord maxHeight,
                                                       nscoord tentWidth, nscoord tentHeight);

  
  static nscoord PrefISizeFromInline(nsIFrame* aFrame,
                                     nsRenderingContext* aRenderingContext);

  
  static nscoord MinISizeFromInline(nsIFrame* aFrame,
                                    nsRenderingContext* aRenderingContext);

  
  static nscolor GetColor(nsIFrame* aFrame, nsCSSProperty aProperty);

  
  static gfxFloat GetSnappedBaselineY(nsIFrame* aFrame, gfxContext* aContext,
                                      nscoord aY, nscoord aAscent);
  
  
  static gfxFloat GetSnappedBaselineX(nsIFrame* aFrame, gfxContext* aContext,
                                      nscoord aX, nscoord aAscent);

  static nscoord AppUnitWidthOfString(char16_t aC,
                                      nsFontMetrics& aFontMetrics,
                                      nsRenderingContext& aContext) {
    return AppUnitWidthOfString(&aC, 1, aFontMetrics, aContext);
  }
  static nscoord AppUnitWidthOfString(const nsString& aString,
                                      nsFontMetrics& aFontMetrics,
                                      nsRenderingContext& aContext) {
    return nsLayoutUtils::AppUnitWidthOfString(aString.get(), aString.Length(),
                                               aFontMetrics, aContext);
  }
  static nscoord AppUnitWidthOfString(const char16_t *aString,
                                      uint32_t aLength,
                                      nsFontMetrics& aFontMetrics,
                                      nsRenderingContext& aContext);
  static nscoord AppUnitWidthOfStringBidi(const nsString& aString,
                                          const nsIFrame* aFrame,
                                          nsFontMetrics& aFontMetrics,
                                          nsRenderingContext& aContext) {
    return nsLayoutUtils::AppUnitWidthOfStringBidi(aString.get(),
                                                   aString.Length(), aFrame,
                                                   aFontMetrics, aContext);
  }
  static nscoord AppUnitWidthOfStringBidi(const char16_t* aString,
                                          uint32_t aLength,
                                          const nsIFrame* aFrame,
                                          nsFontMetrics& aFontMetrics,
                                          nsRenderingContext& aContext);

  static bool StringWidthIsGreaterThan(const nsString& aString,
                                       nsFontMetrics& aFontMetrics,
                                       nsRenderingContext& aContext,
                                       nscoord aWidth);

  static nsBoundingMetrics AppUnitBoundsOfString(const char16_t* aString,
                                                 uint32_t aLength,
                                                 nsFontMetrics& aFontMetrics,
                                                 nsRenderingContext& aContext);

  static void DrawString(const nsIFrame*       aFrame,
                         nsFontMetrics&        aFontMetrics,
                         nsRenderingContext*   aContext,
                         const char16_t*      aString,
                         int32_t               aLength,
                         nsPoint               aPoint,
                         nsStyleContext*       aStyleContext = nullptr);

  


  static void DrawUniDirString(const char16_t* aString,
                               uint32_t aLength,
                               nsPoint aPoint,
                               nsFontMetrics& aFontMetrics,
                               nsRenderingContext& aContext);

  



  typedef void (* TextShadowCallback)(nsRenderingContext* aCtx,
                                      nsPoint aShadowOffset,
                                      const nscolor& aShadowColor,
                                      void* aData);

  static void PaintTextShadow(const nsIFrame*     aFrame,
                              nsRenderingContext* aContext,
                              const nsRect&       aTextRect,
                              const nsRect&       aDirtyRect,
                              const nscolor&      aForegroundColor,
                              TextShadowCallback  aCallback,
                              void*               aCallbackData);

  








  static nscoord GetCenteredFontBaseline(nsFontMetrics* aFontMetrics,
                                         nscoord        aLineHeight,
                                         bool           aIsInverted);

  







  static bool GetFirstLineBaseline(mozilla::WritingMode aWritingMode,
                                   const nsIFrame* aFrame, nscoord* aResult);

  






  struct LinePosition {
    nscoord mBStart, mBaseline, mBEnd;

    LinePosition operator+(nscoord aOffset) const {
      LinePosition result;
      result.mBStart = mBStart + aOffset;
      result.mBaseline = mBaseline + aOffset;
      result.mBEnd = mBEnd + aOffset;
      return result;
    }
  };
  static bool GetFirstLinePosition(mozilla::WritingMode aWritingMode,
                                   const nsIFrame* aFrame,
                                   LinePosition* aResult);


  







  static bool GetLastLineBaseline(mozilla::WritingMode aWritingMode,
                                  const nsIFrame* aFrame, nscoord* aResult);

  








  static nscoord CalculateContentBEnd(mozilla::WritingMode aWritingMode,
                                      nsIFrame* aFrame);

  





  static nsIFrame* GetClosestLayer(nsIFrame* aFrame);

  


  static GraphicsFilter GetGraphicsFilterForFrame(nsIFrame* aFrame);

  



  




















  static DrawResult DrawBackgroundImage(gfxContext&         aContext,
                                        nsPresContext*      aPresContext,
                                        imgIContainer*      aImage,
                                        const CSSIntSize&   aImageSize,
                                        GraphicsFilter      aGraphicsFilter,
                                        const nsRect&       aDest,
                                        const nsRect&       aFill,
                                        const nsPoint&      aAnchor,
                                        const nsRect&       aDirty,
                                        uint32_t            aImageFlags);

  














  static DrawResult DrawImage(gfxContext&         aContext,
                              nsPresContext*      aPresContext,
                              imgIContainer*      aImage,
                              GraphicsFilter      aGraphicsFilter,
                              const nsRect&       aDest,
                              const nsRect&       aFill,
                              const nsPoint&      aAnchor,
                              const nsRect&       aDirty,
                              uint32_t            aImageFlags);

  static inline void InitDashPattern(StrokeOptions& aStrokeOptions,
                                     uint8_t aBorderStyle) {
    if (aBorderStyle == NS_STYLE_BORDER_STYLE_DOTTED) {
      static Float dot[] = { 1.f, 1.f };
      aStrokeOptions.mDashLength = MOZ_ARRAY_LENGTH(dot);
      aStrokeOptions.mDashPattern = dot;
    } else if (aBorderStyle == NS_STYLE_BORDER_STYLE_DASHED) {
      static Float dash[] = { 5.f, 5.f };
      aStrokeOptions.mDashLength = MOZ_ARRAY_LENGTH(dash);
      aStrokeOptions.mDashPattern = dash;
    } else {
      aStrokeOptions.mDashLength = 0;
      aStrokeOptions.mDashPattern = nullptr;
    }
  }

  


  static gfxRect RectToGfxRect(const nsRect& aRect,
                               int32_t aAppUnitsPerDevPixel);

  static gfxPoint PointToGfxPoint(const nsPoint& aPoint,
                                  int32_t aAppUnitsPerPixel) {
    return gfxPoint(gfxFloat(aPoint.x) / aAppUnitsPerPixel,
                    gfxFloat(aPoint.y) / aAppUnitsPerPixel);
  }

  















  static DrawResult DrawSingleUnscaledImage(gfxContext&          aContext,
                                            nsPresContext*       aPresContext,
                                            imgIContainer*       aImage,
                                            GraphicsFilter       aGraphicsFilter,
                                            const nsPoint&       aDest,
                                            const nsRect*        aDirty,
                                            uint32_t             aImageFlags,
                                            const nsRect*        aSourceArea = nullptr);

  





















  static DrawResult DrawSingleImage(gfxContext&         aContext,
                                    nsPresContext*      aPresContext,
                                    imgIContainer*      aImage,
                                    GraphicsFilter      aGraphicsFilter,
                                    const nsRect&       aDest,
                                    const nsRect&       aDirty,
                                    const mozilla::SVGImageContext* aSVGContext,
                                    uint32_t            aImageFlags,
                                    const nsPoint*      aAnchorPoint = nullptr,
                                    const nsRect*       aSourceArea = nullptr);

  















  static void ComputeSizeForDrawing(imgIContainer* aImage,
                                    CSSIntSize&    aImageSize,
                                    nsSize&        aIntrinsicRatio,
                                    bool&          aGotWidth,
                                    bool&          aGotHeight);

  







  static CSSIntSize
  ComputeSizeForDrawingWithFallback(imgIContainer* aImage,
                                    const nsSize&  aFallbackSize);

  






  static nsRect GetWholeImageDestination(const nsSize& aWholeImageSize,
                                         const nsRect& aImageSourceArea,
                                         const nsRect& aDestArea);

  







  static already_AddRefed<imgIContainer>
  OrientImage(imgIContainer* aContainer,
              const nsStyleImageOrientation& aOrientation);

  










  static bool HasNonZeroCorner(const nsStyleCorners& aCorners);

  



  static bool HasNonZeroCornerOnSide(const nsStyleCorners& aCorners,
                                       mozilla::css::Side aSide);

  









  static nsTransparencyMode GetFrameTransparency(nsIFrame* aBackgroundFrame,
                                                 nsIFrame* aCSSRootFrame);

  



  static bool IsPopup(nsIFrame* aFrame);

  



  static nsIFrame* GetDisplayRootFrame(nsIFrame* aFrame);

  












  static nsIFrame* GetReferenceFrame(nsIFrame* aFrame);

  







  static nsIFrame* GetTransformRootFrame(nsIFrame* aFrame);

  






  static uint32_t GetTextRunFlagsForStyle(nsStyleContext* aStyleContext,
                                          const nsStyleFont* aStyleFont,
                                          const nsStyleText* aStyleText,
                                          nscoord aLetterSpacing);

  





  static void GetRectDifferenceStrips(const nsRect& aR1, const nsRect& aR2,
                                      nsRect* aHStrip, nsRect* aVStrip);

  




  static nsDeviceContext*
  GetDeviceContextForScreenInfo(nsPIDOMWindow* aWindow);

  





  static bool IsReallyFixedPos(nsIFrame* aFrame);

  

















  enum {
    
    SFE_WANT_IMAGE_SURFACE = 1 << 0,
    

    SFE_WANT_FIRST_FRAME = 1 << 1,
    
    SFE_NO_COLORSPACE_CONVERSION = 1 << 2,
    


    SFE_PREFER_NO_PREMULTIPLY_ALPHA = 1 << 3,
    

    SFE_NO_RASTERIZING_VECTORS = 1 << 4
  };

  struct DirectDrawInfo {
    
    nsCOMPtr<imgIContainer> mImgContainer;
    
    uint32_t mWhichFrame;
    
    uint32_t mDrawingFlags;
  };

  struct SurfaceFromElementResult {
    SurfaceFromElementResult();

    
    mozilla::RefPtr<SourceSurface> mSourceSurface;
    
    DirectDrawInfo mDrawInfo;

    
    gfxIntSize mSize;
    

    nsCOMPtr<nsIPrincipal> mPrincipal;
    
    nsCOMPtr<imgIRequest> mImageRequest;
    
    bool mIsWriteOnly;
    

    bool mIsStillLoading;
    
    bool mCORSUsed;
    
    bool mIsPremultiplied;
  };

  static SurfaceFromElementResult SurfaceFromElement(mozilla::dom::Element *aElement,
                                                     uint32_t aSurfaceFlags = 0,
                                                     DrawTarget *aTarget = nullptr);
  static SurfaceFromElementResult SurfaceFromElement(nsIImageLoadingContent *aElement,
                                                     uint32_t aSurfaceFlags = 0,
                                                     DrawTarget *aTarget = nullptr);
  
  
  
  static SurfaceFromElementResult SurfaceFromElement(mozilla::dom::HTMLImageElement *aElement,
                                                     uint32_t aSurfaceFlags = 0,
                                                     DrawTarget *aTarget = nullptr);
  static SurfaceFromElementResult SurfaceFromElement(mozilla::dom::HTMLCanvasElement *aElement,
                                                     uint32_t aSurfaceFlags = 0,
                                                     DrawTarget *aTarget = nullptr);
  static SurfaceFromElementResult SurfaceFromElement(mozilla::dom::HTMLVideoElement *aElement,
                                                     uint32_t aSurfaceFlags = 0,
                                                     DrawTarget *aTarget = nullptr);

  



















  static nsIContent*
    GetEditableRootContentByContentEditable(nsIDocument* aDocument);

  



  static bool NeedsPrintPreviewBackground(nsPresContext* aPresContext);

  



  static nsresult GetFontFacesForFrames(nsIFrame* aFrame,
                                        nsFontFaceList* aFontFaceList);

  





  static nsresult GetFontFacesForText(nsIFrame* aFrame,
                                      int32_t aStartOffset,
                                      int32_t aEndOffset,
                                      bool aFollowContinuations,
                                      nsFontFaceList* aFontFaceList);

  










  static size_t SizeOfTextRunsForFrames(nsIFrame* aFrame,
                                        mozilla::MallocSizeOf aMallocSizeOf,
                                        bool clear);

  



  static bool HasAnimationsForCompositor(nsIContent* aContent,
                                         nsCSSProperty aProperty);

  



  static bool HasAnimations(nsIContent* aContent, nsCSSProperty aProperty);

  





  static bool HasCurrentAnimations(nsIContent* aContent,
                                   nsIAtom* aAnimationProperty);

  



  static bool HasCurrentAnimationsForProperty(nsIContent* aContent,
                                              nsCSSProperty aProperty);

  


  static bool AreAsyncAnimationsEnabled();

  


  static bool IsAnimationLoggingEnabled();

  







  static gfxSize ComputeSuitableScaleForAnimation(nsIContent* aContent);

  



  static bool UseBackgroundNearestFiltering();

  



  static bool GPUImageScalingEnabled();

  


  static bool AnimatedImageLayersEnabled();

  


  static bool CSSFiltersEnabled();

  


  static bool CSSClipPathShapesEnabled();

  


  static bool UnsetValueEnabled();

  



  static bool IsTextAlignTrueValueEnabled();

  


  static bool CSSVariablesEnabled()
  {
    return sCSSVariablesEnabled;
  }

  static bool InterruptibleReflowEnabled()
  {
    return sInterruptibleReflowEnabled;
  }

  




  static void UnionChildOverflow(nsIFrame* aFrame,
                                 nsOverflowAreas& aOverflowAreas,
                                 mozilla::layout::FrameChildListIDs aSkipChildLists =
                                     mozilla::layout::FrameChildListIDs());

  




  static float FontSizeInflationFor(const nsIFrame *aFrame);

  












  static nscoord InflationMinFontSizeFor(const nsIFrame *aFrame);

  






  static float FontSizeInflationInner(const nsIFrame *aFrame,
                                      nscoord aMinFontSize);

  static bool FontSizeInflationEnabled(nsPresContext *aPresContext);

  



  static uint32_t FontSizeInflationMaxRatio() {
    return sFontSizeInflationMaxRatio;
  }

  



  static uint32_t FontSizeInflationEmPerLine() {
    return sFontSizeInflationEmPerLine;
  }

  



  static uint32_t FontSizeInflationMinTwips() {
    return sFontSizeInflationMinTwips;
  }

  



  static uint32_t FontSizeInflationLineThreshold() {
    return sFontSizeInflationLineThreshold;
  }

  static bool FontSizeInflationForceEnabled() {
    return sFontSizeInflationForceEnabled;
  }

  static bool FontSizeInflationDisabledInMasterProcess() {
    return sFontSizeInflationDisabledInMasterProcess;
  }

  



  static int32_t FontSizeInflationMappingIntercept() {
    return sFontSizeInflationMappingIntercept;
  }

  




  static bool InvalidationDebuggingIsEnabled() {
    return sInvalidationDebuggingIsEnabled || getenv("MOZ_DUMP_INVALIDATION") != 0;
  }

  static void Initialize();
  static void Shutdown();

  














  static void RegisterImageRequest(nsPresContext* aPresContext,
                                   imgIRequest* aRequest,
                                   bool* aRequestRegistered);

  















  static void RegisterImageRequestIfAnimated(nsPresContext* aPresContext,
                                             imgIRequest* aRequest,
                                             bool* aRequestRegistered);

  














  static void DeregisterImageRequest(nsPresContext* aPresContext,
                                     imgIRequest* aRequest,
                                     bool* aRequestRegistered);

  




  static void PostRestyleEvent(mozilla::dom::Element* aElement,
                               nsRestyleHint aRestyleHint,
                               nsChangeHint aMinChangeHint);

  










  template<typename PointType, typename RectType, typename CoordType>
  static bool PointIsCloserToRect(PointType aPoint, const RectType& aRect,
                                  CoordType& aClosestXDistance,
                                  CoordType& aClosestYDistance);
  






  static nsRect GetBoxShadowRectForFrame(nsIFrame* aFrame, const nsSize& aFrameSize);

#ifdef DEBUG
  




  static void
  AssertNoDuplicateContinuations(nsIFrame* aContainer,
                                 const nsFrameList& aFrameList);

  



  static void
  AssertTreeOnlyEmptyNextInFlows(nsIFrame *aSubtreeRoot);
#endif

  


  static uint32_t
  GetTouchActionFromFrame(nsIFrame* aFrame);

  




  static void
  TransformToAncestorAndCombineRegions(
    const nsRect& aBounds,
    nsIFrame* aFrame,
    const nsIFrame* aAncestorFrame,
    nsRegion* aPreciseTargetDest,
    nsRegion* aImpreciseTargetDest);

  




  static void
  UpdateImageVisibilityForFrame(nsIFrame* aImageFrame);

  




  static bool
  GetContentViewerSize(nsPresContext* aPresContext,
                       LayoutDeviceIntSize& aOutSize);

 








  static nsSize
  CalculateCompositionSizeForFrame(nsIFrame* aFrame);

 










  static CSSSize
  CalculateRootCompositionSize(nsIFrame* aFrame,
                               bool aIsRootContentDocRootScrollFrame,
                               const FrameMetrics& aMetrics);

 





  static nsRect
  CalculateScrollableRectForFrame(nsIScrollableFrame* aScrollableFrame, nsIFrame* aRootFrame);

 



  static nsRect
  CalculateExpandedScrollableRect(nsIFrame* aFrame);

  



  static bool UsesAsyncScrolling();

  







  static void LogTestDataForPaint(mozilla::layers::LayerManager* aManager,
                                  ViewID aScrollId,
                                  const std::string& aKey,
                                  const std::string& aValue) {
    if (IsAPZTestLoggingEnabled()) {
      DoLogTestDataForPaint(aManager, aScrollId, aKey, aValue);
    }
  }

  




  template <typename Value>
  static void LogTestDataForPaint(mozilla::layers::LayerManager* aManager,
                                  ViewID aScrollId,
                                  const std::string& aKey,
                                  const Value& aValue) {
    if (IsAPZTestLoggingEnabled()) {
      DoLogTestDataForPaint(aManager, aScrollId, aKey,
          mozilla::ToString(aValue));
    }
  }

  







  static bool CalculateAndSetDisplayPortMargins(nsIScrollableFrame* aScrollFrame,
                                                RepaintMode aRepaintMode);

  














  static bool GetOrMaybeCreateDisplayPort(nsDisplayListBuilder& aBuilder,
                                          nsIFrame* aScrollFrame,
                                          nsRect aDisplayPortBase,
                                          nsRect* aOutDisplayport);

  static bool IsOutlineStyleAutoEnabled();

  static void SetBSizeFromFontMetrics(const nsIFrame* aFrame,
                                      nsHTMLReflowMetrics& aMetrics,
                                      const mozilla::LogicalMargin& aFramePadding,
                                      mozilla::WritingMode aLineWM,
                                      mozilla::WritingMode aFrameWM);

  static bool HasApzAwareListeners(mozilla::EventListenerManager* aElm);
  static bool HasDocumentLevelListenersForApzAwareEvents(nsIPresShell* aShell);

private:
  static uint32_t sFontSizeInflationEmPerLine;
  static uint32_t sFontSizeInflationMinTwips;
  static uint32_t sFontSizeInflationLineThreshold;
  static int32_t  sFontSizeInflationMappingIntercept;
  static uint32_t sFontSizeInflationMaxRatio;
  static bool sFontSizeInflationForceEnabled;
  static bool sFontSizeInflationDisabledInMasterProcess;
  static bool sInvalidationDebuggingIsEnabled;
  static bool sCSSVariablesEnabled;
  static bool sInterruptibleReflowEnabled;

  


  static void DoLogTestDataForPaint(mozilla::layers::LayerManager* aManager,
                                    ViewID aScrollId,
                                    const std::string& aKey,
                                    const std::string& aValue);

  static bool IsAPZTestLoggingEnabled();
};

template<typename PointType, typename RectType, typename CoordType>
 bool
nsLayoutUtils::PointIsCloserToRect(PointType aPoint, const RectType& aRect,
                                   CoordType& aClosestXDistance,
                                   CoordType& aClosestYDistance)
{
  CoordType fromLeft = aPoint.x - aRect.x;
  CoordType fromRight = aPoint.x - aRect.XMost();

  CoordType xDistance;
  if (fromLeft >= 0 && fromRight <= 0) {
    xDistance = 0;
  } else {
    xDistance = std::min(abs(fromLeft), abs(fromRight));
  }

  if (xDistance <= aClosestXDistance) {
    if (xDistance < aClosestXDistance) {
      aClosestYDistance = std::numeric_limits<CoordType>::max();
    }

    CoordType fromTop = aPoint.y - aRect.y;
    CoordType fromBottom = aPoint.y - aRect.YMost();

    CoordType yDistance;
    if (fromTop >= 0 && fromBottom <= 0) {
      yDistance = 0;
    } else {
      yDistance = std::min(abs(fromTop), abs(fromBottom));
    }

    if (yDistance < aClosestYDistance) {
      aClosestXDistance = xDistance;
      aClosestYDistance = yDistance;
      return true;
    }
  }

  return false;
}

namespace mozilla {






inline gfx::Point NSPointToPoint(const nsPoint& aPoint,
                                 int32_t aAppUnitsPerPixel) {
  return gfx::Point(gfx::Float(aPoint.x) / aAppUnitsPerPixel,
                    gfx::Float(aPoint.y) / aAppUnitsPerPixel);
}






gfx::Rect NSRectToRect(const nsRect& aRect, double aAppUnitsPerPixel);











gfx::Rect NSRectToSnappedRect(const nsRect& aRect, double aAppUnitsPerPixel,
                              const gfx::DrawTarget& aSnapDT);

void StrokeLineWithSnapping(const nsPoint& aP1, const nsPoint& aP2,
                            int32_t aAppUnitsPerDevPixel,
                            gfx::DrawTarget& aDrawTarget,
                            const gfx::Pattern& aPattern,
                            const gfx::StrokeOptions& aStrokeOptions = gfx::StrokeOptions(),
                            const gfx::DrawOptions& aDrawOptions = gfx::DrawOptions());

  namespace layout {

    





    class AutoMaybeDisableFontInflation {
    public:
      explicit AutoMaybeDisableFontInflation(nsIFrame *aFrame);

      ~AutoMaybeDisableFontInflation();
    private:
      nsPresContext *mPresContext;
      bool mOldValue;
    };

    void MaybeSetupTransactionIdAllocator(layers::LayerManager* aManager, nsView* aView);

  }
}

class nsSetAttrRunnable : public nsRunnable
{
public:
  nsSetAttrRunnable(nsIContent* aContent, nsIAtom* aAttrName,
                    const nsAString& aValue);
  nsSetAttrRunnable(nsIContent* aContent, nsIAtom* aAttrName,
                    int32_t aValue);

  NS_DECL_NSIRUNNABLE

  nsCOMPtr<nsIContent> mContent;
  nsCOMPtr<nsIAtom> mAttrName;
  nsAutoString mValue;
};

class nsUnsetAttrRunnable : public nsRunnable
{
public:
  nsUnsetAttrRunnable(nsIContent* aContent, nsIAtom* aAttrName);

  NS_DECL_NSIRUNNABLE

  nsCOMPtr<nsIContent> mContent;
  nsCOMPtr<nsIAtom> mAttrName;
};

#endif 
