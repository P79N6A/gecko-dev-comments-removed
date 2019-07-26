




#ifndef nsLayoutUtils_h__
#define nsLayoutUtils_h__

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
class gfxASurface;
class gfxDrawable;
class nsView;
class nsIFrame;
class nsStyleCoord;
class nsStyleCorners;
class gfxContext;
class nsPIDOMWindow;
class imgIRequest;
class nsIDocument;
struct nsStyleFont;
struct nsStyleImageOrientation;
struct nsOverflowAreas;

#include "mozilla/MemoryReporting.h"
#include "nsChangeHint.h"
#include "nsAutoPtr.h"
#include "nsFrameList.h"
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

#include <limits>
#include <algorithm>

namespace mozilla {
class SVGImageContext;
struct IntrinsicSize;
struct ContainerLayerParameters;
namespace dom {
class DOMRectList;
class Element;
class HTMLImageElement;
class HTMLCanvasElement;
class HTMLVideoElement;
} 
namespace layers {
class Layer;
}

template <class AnimationsOrTransitions>
extern AnimationsOrTransitions* HasAnimationOrTransition(nsIContent* aContent,
                                                         nsIAtom* aAnimationProperty,
                                                         nsCSSProperty aProperty);

} 






class nsLayoutUtils
{
  typedef ::GraphicsFilter GraphicsFilter;
  typedef mozilla::dom::DOMRectList DOMRectList;
  typedef mozilla::layers::Layer Layer;
  typedef mozilla::ContainerLayerParameters ContainerLayerParameters;
  typedef mozilla::gfx::SourceSurface SourceSurface;
  typedef mozilla::gfx::DrawTarget DrawTarget;

public:
  typedef mozilla::layers::FrameMetrics FrameMetrics;
  typedef FrameMetrics::ViewID ViewID;

  



  static bool FindIDFor(const nsIContent* aContent, ViewID* aOutViewId);

  



  static ViewID FindOrCreateIDFor(nsIContent* aContent);

  


  static nsIContent* FindContentFor(ViewID aId);

  


  static nsIScrollableFrame* FindScrollableFrameFor(ViewID aId);

  


  static bool GetDisplayPort(nsIContent* aContent, nsRect *aResult = nullptr);

  


  static bool GetCriticalDisplayPort(nsIContent* aContent, nsRect* aResult = nullptr);

  



  static mozilla::layout::FrameChildListID GetChildListNameFor(nsIFrame* aChildFrame);

  







  static nsIFrame* GetBeforeFrame(nsIFrame* aFrame);

  








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

  



  static nsIFrame* LastContinuationWithChild(nsIFrame* aFrame);

  



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

  















  static nsIFrame* GetAnimatedGeometryRootFor(nsIFrame* aFrame,
                                              const nsIFrame* aStopAtAncestor = nullptr);

  static nsIFrame* GetAnimatedGeometryRootFor(nsDisplayItem* aItem,
                                              nsDisplayListBuilder* aBuilder);


  


  static nsIScrollableFrame* GetScrollableFrameFor(const nsIFrame *aScrolledFrame);

  











  enum Direction { eHorizontal, eVertical };
  static nsIScrollableFrame* GetNearestScrollableFrameForDirection(nsIFrame* aFrame,
                                                                   Direction aDirection);

  enum {
    SCROLLABLE_SAME_DOC = 0x01,
    SCROLLABLE_INCLUDE_HIDDEN = 0x02
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
                   const nsIntPoint aPoint,
                   nsIFrame* aFrame);

  









  static nsPoint GetEventCoordinatesRelativeTo(nsIWidget* aWidget,
                                               const nsIntPoint aPoint,
                                               nsIFrame* aFrame);

  






  static nsIFrame* GetPopupFrameForEventCoordinates(
                     nsPresContext* aPresContext,
                     const mozilla::WidgetEvent* aEvent);

  







  static nsPoint TranslateWidgetToView(nsPresContext* aPresContext,
                                       nsIWidget* aWidget, nsIntPoint aPt,
                                       nsView* aView);

  











  static gfx3DMatrix ChangeMatrixBasis(const gfxPoint3D &aOrigin, const gfx3DMatrix &aMatrix);

  











  static nsresult GetRemoteContentIds(nsIFrame* aFrame,
                                     const nsRect& aTarget,
                                     nsTArray<ViewID> &aOutIDs,
                                     bool aIgnoreRootScrollFrame);

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


  



  static gfx3DMatrix GetTransformToAncestor(nsIFrame *aFrame, const nsIFrame *aAncestor);

  





  static bool GetLayerTransformForFrame(nsIFrame* aFrame,
                                        gfx3DMatrix* aTransform);

  








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

  







  static nsRect RoundGfxRectToAppRect(const gfxRect &aRect, float aFactor);

  




  static nsRegion RoundedRectIntersectRect(const nsRect& aRoundedRect,
                                           const nscoord aRadii[8],
                                           const nsRect& aContainedRect);

  




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

  



  static int32_t GetZIndex(nsIFrame* aFrame);

  











  static bool
  BinarySearchForPosition(nsRenderingContext* acx,
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

  class RectCallback {
  public:
    virtual void AddRect(const nsRect& aRect) = 0;
  };

  struct RectAccumulator : public RectCallback {
    nsRect mResultRect;
    nsRect mFirstRect;
    bool mSeenFirstRect;

    RectAccumulator();

    virtual void AddRect(const nsRect& aRect);
  };

  struct RectListBuilder : public RectCallback {
    DOMRectList* mRectList;

    RectListBuilder(DOMRectList* aList);
    virtual void AddRect(const nsRect& aRect);
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
  GetNextContinuationOrSpecialSibling(nsIFrame *aFrame);

  



  static nsIFrame*
  FirstContinuationOrSpecialSibling(nsIFrame *aFrame);

  



  static bool
  IsFirstContinuationOrSpecialSibling(nsIFrame *aFrame);

  






  static bool IsViewportScrollbarFrame(nsIFrame* aFrame);

  





  enum IntrinsicWidthType { MIN_WIDTH, PREF_WIDTH };
  enum {
    IGNORE_PADDING = 0x01
  };
  static nscoord IntrinsicForContainer(nsRenderingContext* aRenderingContext,
                                       nsIFrame* aFrame,
                                       IntrinsicWidthType aType,
                                       uint32_t aFlags = 0);

  





  static nscoord ComputeCBDependentValue(nscoord aPercentBasis,
                                         const nsStyleCoord& aCoord);

  
















  static nscoord ComputeWidthValue(
                   nsRenderingContext* aRenderingContext,
                   nsIFrame*            aFrame,
                   nscoord              aContainingBlockWidth,
                   nscoord              aContentEdgeToBoxSizing,
                   nscoord              aBoxSizingToMarginEdge,
                   const nsStyleCoord&  aCoord);

  



  static nscoord ComputeHeightDependentValue(
                   nscoord              aContainingBlockHeight,
                   const nsStyleCoord&  aCoord);

  


  static nscoord ComputeHeightValue(nscoord aContainingBlockHeight,
                                    nscoord aContentEdgeToBoxSizingBoxEdge,
                                    const nsStyleCoord& aCoord)
  {
    MOZ_ASSERT(aContainingBlockHeight != nscoord_MAX || !aCoord.HasPercent(),
               "caller must deal with %% of unconstrained height");
    MOZ_ASSERT(aCoord.IsCoordPercentCalcUnit());

    nscoord result =
      nsRuleNode::ComputeCoordPercentCalc(aCoord, aContainingBlockHeight);
    
    return std::max(0, result - aContentEdgeToBoxSizingBoxEdge);
  }

  static bool IsAutoHeight(const nsStyleCoord &aCoord, nscoord aCBHeight)
  {
    nsStyleUnit unit = aCoord.GetUnit();
    return unit == eStyleUnit_Auto ||  
           unit == eStyleUnit_None ||  
           (aCBHeight == nscoord_MAX && aCoord.HasPercent());
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

  




  static nsSize ComputeSizeWithIntrinsicDimensions(
                    nsRenderingContext* aRenderingContext, nsIFrame* aFrame,
                    const mozilla::IntrinsicSize& aIntrinsicSize,
                    nsSize aIntrinsicRatio, nsSize aCBSize,
                    nsSize aMargin, nsSize aBorder, nsSize aPadding);

  





  static nsSize ComputeAutoSizeWithIntrinsicDimensions(nscoord minWidth, nscoord minHeight,
                                                       nscoord maxWidth, nscoord maxHeight,
                                                       nscoord tentWidth, nscoord tentHeight);

  
  static nscoord PrefWidthFromInline(nsIFrame* aFrame,
                                     nsRenderingContext* aRenderingContext);

  
  static nscoord MinWidthFromInline(nsIFrame* aFrame,
                                    nsRenderingContext* aRenderingContext);

  
  static nscolor GetColor(nsIFrame* aFrame, nsCSSProperty aProperty);

  
  static gfxFloat GetSnappedBaselineY(nsIFrame* aFrame, gfxContext* aContext,
                                      nscoord aY, nscoord aAscent);

  static void DrawString(const nsIFrame*       aFrame,
                         nsRenderingContext*   aContext,
                         const char16_t*      aString,
                         int32_t               aLength,
                         nsPoint               aPoint,
                         nsStyleContext*       aStyleContext = nullptr);

  static nscoord GetStringWidth(const nsIFrame*      aFrame,
                                nsRenderingContext* aContext,
                                const char16_t*     aString,
                                int32_t              aLength);

  



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
                                         nscoord         aLineHeight);

  







  static bool GetFirstLineBaseline(const nsIFrame* aFrame, nscoord* aResult);

  






  struct LinePosition {
    nscoord mTop, mBaseline, mBottom;

    LinePosition operator+(nscoord aOffset) const {
      LinePosition result;
      result.mTop = mTop + aOffset;
      result.mBaseline = mBaseline + aOffset;
      result.mBottom = mBottom + aOffset;
      return result;
    }
  };
  static bool GetFirstLinePosition(const nsIFrame* aFrame,
                                   LinePosition* aResult);


  







  static bool GetLastLineBaseline(const nsIFrame* aFrame, nscoord* aResult);

  







  static nscoord CalculateContentBottom(nsIFrame* aFrame);

  





  static nsIFrame* GetClosestLayer(nsIFrame* aFrame);

  


  static GraphicsFilter GetGraphicsFilterForFrame(nsIFrame* aFrame);

  



  





















  static nsresult DrawBackgroundImage(nsRenderingContext* aRenderingContext,
                                      imgIContainer*      aImage,
                                      const nsIntSize&    aImageSize,
                                      GraphicsFilter      aGraphicsFilter,
                                      const nsRect&       aDest,
                                      const nsRect&       aFill,
                                      const nsPoint&      aAnchor,
                                      const nsRect&       aDirty,
                                      uint32_t            aImageFlags);

  














  static nsresult DrawImage(nsRenderingContext* aRenderingContext,
                            imgIContainer*       aImage,
                            GraphicsFilter       aGraphicsFilter,
                            const nsRect&        aDest,
                            const nsRect&        aFill,
                            const nsPoint&       aAnchor,
                            const nsRect&        aDirty,
                            uint32_t             aImageFlags);

  


  static gfxRect RectToGfxRect(const nsRect& aRect,
                               int32_t aAppUnitsPerDevPixel);

  














  static void DrawPixelSnapped(nsRenderingContext* aRenderingContext,
                               gfxDrawable*         aDrawable,
                               GraphicsFilter       aFilter,
                               const nsRect&        aDest,
                               const nsRect&        aFill,
                               const nsPoint&       aAnchor,
                               const nsRect&        aDirty);

  















  static nsresult DrawSingleUnscaledImage(nsRenderingContext* aRenderingContext,
                                          imgIContainer*       aImage,
                                          GraphicsFilter       aGraphicsFilter,
                                          const nsPoint&       aDest,
                                          const nsRect*        aDirty,
                                          uint32_t             aImageFlags,
                                          const nsRect*        aSourceArea = nullptr);

  



















  static nsresult DrawSingleImage(nsRenderingContext*    aRenderingContext,
                                  imgIContainer*         aImage,
                                  GraphicsFilter         aGraphicsFilter,
                                  const nsRect&          aDest,
                                  const nsRect&          aDirty,
                                  const mozilla::SVGImageContext* aSVGContext,
                                  uint32_t               aImageFlags,
                                  const nsRect*          aSourceArea = nullptr);

  















  static void ComputeSizeForDrawing(imgIContainer* aImage,
                                    nsIntSize&     aImageSize,
                                    nsSize&        aIntrinsicRatio,
                                    bool&          aGotWidth,
                                    bool&          aGotHeight);

  






  static nsRect GetWholeImageDestination(const nsIntSize& aWholeImageSize,
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

  






  static uint32_t GetTextRunFlagsForStyle(nsStyleContext* aStyleContext,
                                          const nsStyleFont* aStyleFont,
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
    


    SFE_NO_PREMULTIPLY_ALPHA = 1 << 3,
    

    SFE_NO_RASTERIZING_VECTORS = 1 << 4
  };

  struct DirectDrawInfo {
    
    nsCOMPtr<imgIContainer> mImgContainer;
    
    uint32_t mWhichFrame;
    
    uint32_t mDrawingFlags;
  };

  struct SurfaceFromElementResult {
    SurfaceFromElementResult();

    
    nsRefPtr<gfxASurface> mSurface;
    mozilla::RefPtr<SourceSurface> mSourceSurface;
    
    DirectDrawInfo mDrawInfo;

    
    gfxIntSize mSize;
    

    nsCOMPtr<nsIPrincipal> mPrincipal;
    
    nsCOMPtr<imgIRequest> mImageRequest;
    
    bool mIsWriteOnly;
    

    bool mIsStillLoading;
    
    bool mCORSUsed;
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

  


  static bool AreAsyncAnimationsEnabled();

  


  static bool IsAnimationLoggingEnabled();

  




  static gfxSize GetMaximumAnimatedScale(nsIContent* aContent);

  



  static bool UseBackgroundNearestFiltering();

  



  static bool GPUImageScalingEnabled();

  


  static bool AnimatedImageLayersEnabled();

  


  static bool CSSFiltersEnabled();

  


  static bool UnsetValueEnabled();

  



  static bool IsTextAlignTrueValueEnabled();

  


  static bool CSSVariablesEnabled()
  {
    return sCSSVariablesEnabled;
  }

  



  static void UnionChildOverflow(nsIFrame* aFrame,
                                 nsOverflowAreas& aOverflowAreas);

  




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

  




  static void
  UpdateImageVisibilityForFrame(nsIFrame* aImageFrame);

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
  namespace layout {

    





    class AutoMaybeDisableFontInflation {
    public:
      AutoMaybeDisableFontInflation(nsIFrame *aFrame);

      ~AutoMaybeDisableFontInflation();
    private:
      nsPresContext *mPresContext;
      bool mOldValue;
    };

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
