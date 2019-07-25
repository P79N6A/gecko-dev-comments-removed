




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
class nsClientRectList;
class nsFontFaceList;
class nsHTMLCanvasElement;
class nsHTMLVideoElement;
class nsIImageLoadingContent;
class nsHTMLImageElement;

#include "prtypes.h"
#include "nsChangeHint.h"
#include "nsStyleContext.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"
#include "nsIView.h"
#include "nsIFrame.h"
#include "nsThreadUtils.h"
#include "nsIPresShell.h"
#include "nsIPrincipal.h"
#include "gfxPattern.h"
#include "imgIContainer.h"
#include "nsCSSPseudoElements.h"
#include "nsHTMLReflowState.h"
#include "nsIFrameLoader.h"
#include "Layers.h"

class nsBlockFrame;
class gfxDrawable;

namespace mozilla {
namespace dom {
class Element;
} 
} 






class nsLayoutUtils
{
  typedef gfxPattern::GraphicsFilter GraphicsFilter;

public:
  typedef mozilla::layers::FrameMetrics::ViewID ViewID;

  



  static ViewID FindIDFor(nsIContent* aContent);

  


  static nsIContent* FindContentFor(ViewID aId);

  


  static bool GetDisplayPort(nsIContent* aContent, nsRect *aResult);

  



  static nsIFrame::ChildListID GetChildListNameFor(nsIFrame* aChildFrame);

  







  static nsIFrame* GetBeforeFrame(nsIFrame* aFrame);

  








  static nsIFrame* GetAfterFrame(nsIFrame* aFrame);

  








  static nsIFrame* GetClosestFrameOfType(nsIFrame* aFrame, nsIAtom* aFrameType);

  







  static nsIFrame* GetPageFrame(nsIFrame* aFrame)
  {
    return GetClosestFrameOfType(aFrame, nsGkAtoms::pageFrame);
  }

  





  static nsIFrame* GetStyleFrame(nsIFrame* aPrimaryFrame);

  













  static bool IsGeneratedContentFor(nsIContent* aContent, nsIFrame* aFrame,
                                      nsIAtom* aPseudoElement);

#ifdef DEBUG
  
  static bool gPreventAssertInCompareTreePosition;
#endif 

  














  static PRInt32 CompareTreePosition(nsIContent* aContent1,
                                     nsIContent* aContent2,
                                     const nsIContent* aCommonAncestor = nsnull)
  {
    return DoCompareTreePosition(aContent1, aContent2, -1, 1, aCommonAncestor);
  }

  





  static PRInt32 DoCompareTreePosition(nsIContent* aContent1,
                                       nsIContent* aContent2,
                                       PRInt32 aIf1Ancestor,
                                       PRInt32 aIf2Ancestor,
                                       const nsIContent* aCommonAncestor = nsnull);

  


















  static PRInt32 CompareTreePosition(nsIFrame* aFrame1,
                                     nsIFrame* aFrame2,
                                     nsIFrame* aCommonAncestor = nsnull)
  {
    return DoCompareTreePosition(aFrame1, aFrame2, -1, 1, aCommonAncestor);
  }

  





  static PRInt32 DoCompareTreePosition(nsIFrame* aFrame1,
                                       nsIFrame* aFrame2,
                                       PRInt32 aIf1Ancestor,
                                       PRInt32 aIf2Ancestor,
                                       nsIFrame* aCommonAncestor = nsnull);

  



  static nsIFrame* GetLastContinuationWithChild(nsIFrame* aFrame);

  



  static nsIFrame* GetLastSibling(nsIFrame* aFrame);

  






  static nsIView* FindSiblingViewFor(nsIView* aParentView, nsIFrame* aFrame);

  









  static nsIFrame* GetCrossDocParentFrame(const nsIFrame* aFrame,
                                          nsPoint* aCrossDocOffset = nsnull);

  






  static bool IsProperAncestorFrame(nsIFrame* aAncestorFrame, nsIFrame* aFrame,
                                      nsIFrame* aCommonAncestor = nsnull);

  





  static bool IsProperAncestorFrameCrossDoc(nsIFrame* aAncestorFrame, nsIFrame* aFrame,
                                              nsIFrame* aCommonAncestor = nsnull);

  









  static bool IsAncestorFrameCrossDoc(nsIFrame* aAncestorFrame, nsIFrame* aFrame,
                                        nsIFrame* aCommonAncestor = nsnull);

  






  static nsIFrame* GetActiveScrolledRootFor(nsIFrame* aFrame,
                                            nsIFrame* aStopAtAncestor);

  static nsIFrame* GetActiveScrolledRootFor(nsDisplayItem* aItem,
                                            nsDisplayListBuilder* aBuilder,
                                            bool* aShouldFixToViewport = nsnull);

  




  static bool IsScrolledByRootContentDocumentDisplayportScrolling(nsIFrame* aActiveScrolledRoot,
                                                                  nsDisplayListBuilder* aBuilder);

  


  static nsIScrollableFrame* GetScrollableFrameFor(nsIFrame *aScrolledFrame);

  











  enum Direction { eHorizontal, eVertical };
  static nsIScrollableFrame* GetNearestScrollableFrameForDirection(nsIFrame* aFrame,
                                                                   Direction aDirection);

  








  static nsIScrollableFrame* GetNearestScrollableFrame(nsIFrame* aFrame);

  










  static bool HasPseudoStyle(nsIContent* aContent,
                               nsStyleContext* aStyleContext,
                               nsCSSPseudoElements::Type aPseudoElement,
                               nsPresContext* aPresContext);

  



  static nsIFrame* GetFloatFromPlaceholder(nsIFrame* aPlaceholder);

  
  
  static PRUint8 CombineBreakType(PRUint8 aOrigBreakType, PRUint8 aNewBreakType);

  








  static nsPoint GetDOMEventCoordinatesRelativeTo(nsIDOMEvent* aDOMEvent,
                                                  nsIFrame* aFrame);

  








  static nsPoint GetEventCoordinatesRelativeTo(const nsEvent* aEvent,
                                               nsIFrame* aFrame);

  









  static nsPoint GetEventCoordinatesRelativeTo(const nsEvent* aEvent,
                                               const nsIntPoint aPoint,
                                               nsIFrame* aFrame);

  






  static nsIFrame* GetPopupFrameForEventCoordinates(nsPresContext* aPresContext,
                                                    const nsEvent* aEvent);









  static nsPoint TranslateWidgetToView(nsPresContext* aPresContext,
                                       nsIWidget* aWidget, nsIntPoint aPt,
                                       nsIView* aView);

  











  static gfx3DMatrix ChangeMatrixBasis(const gfxPoint3D &aOrigin, const gfx3DMatrix &aMatrix);

  











  static nsresult GetRemoteContentIds(nsIFrame* aFrame,
                                     const nsRect& aTarget,
                                     nsTArray<ViewID> &aOutIDs,
                                     bool aIgnoreRootScrollFrame);

  









  static nsIFrame* GetFrameForPoint(nsIFrame* aFrame, nsPoint aPt,
                                    bool aShouldIgnoreSuppression = false,
                                    bool aIgnoreRootScrollFrame = false);

  










  static nsresult GetFramesForArea(nsIFrame* aFrame, const nsRect& aRect,
                                   nsTArray<nsIFrame*> &aOutFrames,
                                   bool aShouldIgnoreSuppression = false,
                                   bool aIgnoreRootScrollFrame = false);

  



  static nsRect TransformAncestorRectToFrame(nsIFrame* aFrame,
                                             const nsRect& aRect,
                                             nsIFrame* aAncestor);

  



  static nsRect TransformFrameRectToAncestor(nsIFrame* aFrame,
                                             const nsRect& aRect,
                                             nsIFrame* aAncestor);


  



  static gfx3DMatrix GetTransformToAncestor(nsIFrame *aFrame, nsIFrame *aAncestor);

  








  static nsPoint TransformRootPointToFrame(nsIFrame* aFrame,
                                           const nsPoint &aPt);

  








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

  enum {
    PAINT_IN_TRANSFORM = 0x01,
    PAINT_SYNC_DECODE_IMAGES = 0x02,
    PAINT_WIDGET_LAYERS = 0x04,
    PAINT_IGNORE_SUPPRESSION = 0x08,
    PAINT_DOCUMENT_RELATIVE = 0x10,
    PAINT_HIDE_CARET = 0x20,
    PAINT_ALL_CONTINUATIONS = 0x40,
    PAINT_TO_WINDOW = 0x80,
    PAINT_EXISTING_TRANSACTION = 0x100
  };

  









































  static nsresult PaintFrame(nsRenderingContext* aRenderingContext, nsIFrame* aFrame,
                             const nsRegion& aDirtyRegion, nscolor aBackstop,
                             PRUint32 aFlags = 0);

  



  static PRInt32 GetZIndex(nsIFrame* aFrame);

  











  static bool
  BinarySearchForPosition(nsRenderingContext* acx,
                          const PRUnichar* aText,
                          PRInt32    aBaseWidth,
                          PRInt32    aBaseInx,
                          PRInt32    aStartInx,
                          PRInt32    aEndInx,
                          PRInt32    aCursorPos,
                          PRInt32&   aIndex,
                          PRInt32&   aTextWidth);

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
    nsClientRectList* mRectList;
    nsresult          mRV;

    RectListBuilder(nsClientRectList* aList);
     virtual void AddRect(const nsRect& aRect);
  };

  static nsIFrame* GetContainingBlockForClientRect(nsIFrame* aFrame);

  enum {
    RECTS_ACCOUNT_FOR_TRANSFORMS = 0x01
  };
  











  static void GetAllInFlowRects(nsIFrame* aFrame, nsIFrame* aRelativeTo,
                                RectCallback* aCallback, PRUint32 aFlags = 0);

  






  static nsRect GetAllInFlowRectsUnion(nsIFrame* aFrame, nsIFrame* aRelativeTo,
                                       PRUint32 aFlags = 0);

  enum {
    EXCLUDE_BLUR_SHADOWS = 0x01
  };
  




  static nsRect GetTextShadowRectsUnion(const nsRect& aTextAndDecorationsRect,
                                        nsIFrame* aFrame,
                                        PRUint32 aFlags = 0);

  






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

  


  static bool IsNonWrapperBlock(nsIFrame* aFrame) {
    return GetAsBlock(aFrame) && !aFrame->IsBlockWrapper();
  }

  



  static nsIFrame* GetParentOrPlaceholderFor(nsIFrame* aFrame);

  


  static nsIFrame*
  GetNextContinuationOrSpecialSibling(nsIFrame *aFrame);

  



  static nsIFrame*
  GetFirstContinuationOrSpecialSibling(nsIFrame *aFrame);

  






  static bool IsViewportScrollbarFrame(nsIFrame* aFrame);

  





  enum IntrinsicWidthType { MIN_WIDTH, PREF_WIDTH };
  static nscoord IntrinsicForContainer(nsRenderingContext* aRenderingContext,
                                       nsIFrame* aFrame,
                                       IntrinsicWidthType aType);

  



  static nscoord ComputeWidthDependentValue(
                   nscoord              aContainingBlockWidth,
                   const nsStyleCoord&  aCoord);

  
















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
                                    const nsStyleCoord& aCoord)
  {
    nscoord result =
      ComputeHeightDependentValue(aContainingBlockHeight, aCoord);
    if (result < 0)
      result = 0; 
    return result;
  }

  static bool IsAutoHeight(const nsStyleCoord &aCoord, nscoord aCBHeight)
  {
    nsStyleUnit unit = aCoord.GetUnit();
    return unit == eStyleUnit_Auto ||  
           unit == eStyleUnit_None ||  
           (aCBHeight == NS_AUTOHEIGHT && aCoord.HasPercent());
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
                    const nsIFrame::IntrinsicSize& aIntrinsicSize,
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

  static void DrawString(const nsIFrame*      aFrame,
                         nsRenderingContext* aContext,
                         const PRUnichar*     aString,
                         PRInt32              aLength,
                         nsPoint              aPoint,
                         PRUint8              aDirection = NS_STYLE_DIRECTION_INHERIT);

  static nscoord GetStringWidth(const nsIFrame*      aFrame,
                                nsRenderingContext* aContext,
                                const PRUnichar*     aString,
                                PRInt32              aLength);

  



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
                                      PRUint32            aImageFlags);

  














  static nsresult DrawImage(nsRenderingContext* aRenderingContext,
                            imgIContainer*       aImage,
                            GraphicsFilter       aGraphicsFilter,
                            const nsRect&        aDest,
                            const nsRect&        aFill,
                            const nsPoint&       aAnchor,
                            const nsRect&        aDirty,
                            PRUint32             aImageFlags);

  


  static gfxRect RectToGfxRect(const nsRect& aRect,
                               PRInt32 aAppUnitsPerDevPixel);

  














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
                                          PRUint32             aImageFlags,
                                          const nsRect*        aSourceArea = nsnull);

  














  static nsresult DrawSingleImage(nsRenderingContext* aRenderingContext,
                                  imgIContainer*       aImage,
                                  GraphicsFilter       aGraphicsFilter,
                                  const nsRect&        aDest,
                                  const nsRect&        aDirty,
                                  PRUint32             aImageFlags,
                                  const nsRect*        aSourceArea = nsnull);

  















  static void ComputeSizeForDrawing(imgIContainer* aImage,
                                    nsIntSize&     aImageSize,
                                    nsSize&        aIntrinsicRatio,
                                    bool&          aGotWidth,
                                    bool&          aGotHeight);

  






  static nsRect GetWholeImageDestination(const nsIntSize& aWholeImageSize,
                                         const nsRect& aImageSourceArea,
                                         const nsRect& aDestArea);

  










  static bool HasNonZeroCorner(const nsStyleCorners& aCorners);

  



  static bool HasNonZeroCornerOnSide(const nsStyleCorners& aCorners,
                                       mozilla::css::Side aSide);

  









  static nsTransparencyMode GetFrameTransparency(nsIFrame* aBackgroundFrame,
                                                 nsIFrame* aCSSRootFrame);

  



  static bool IsPopup(nsIFrame* aFrame);

  



  static nsIFrame* GetDisplayRootFrame(nsIFrame* aFrame);

  






  static PRUint32 GetTextRunFlagsForStyle(nsStyleContext* aStyleContext,
                                          const nsStyleText* aStyleText,
                                          const nsStyleFont* aStyleFont);

  





  static void GetRectDifferenceStrips(const nsRect& aR1, const nsRect& aR2,
                                      nsRect* aHStrip, nsRect* aVStrip);

  




  static nsDeviceContext*
  GetDeviceContextForScreenInfo(nsPIDOMWindow* aWindow);

  





  static bool IsReallyFixedPos(nsIFrame* aFrame);

  



  static bool FrameIsNonFirstInIBSplit(const nsIFrame* aFrame) {
    return (aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL) &&
      aFrame->GetFirstContinuation()->
        Properties().Get(nsIFrame::IBSplitSpecialPrevSibling());
  }

  



  static bool FrameIsNonLastInIBSplit(const nsIFrame* aFrame) {
    return (aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL) &&
      aFrame->GetFirstContinuation()->
        Properties().Get(nsIFrame::IBSplitSpecialSibling());
  }

  

















  enum {
    
    SFE_WANT_NEW_SURFACE   = 1 << 0,
    
    SFE_WANT_IMAGE_SURFACE = 1 << 1,
    

    SFE_WANT_FIRST_FRAME = 1 << 2,
    
    SFE_NO_COLORSPACE_CONVERSION = 1 << 3,
    


    SFE_NO_PREMULTIPLY_ALPHA = 1 << 4
  };

  struct SurfaceFromElementResult {
    SurfaceFromElementResult() :
      
      mIsWriteOnly(true), mIsStillLoading(false), mCORSUsed(false) {}

    
    nsRefPtr<gfxASurface> mSurface;
    
    gfxIntSize mSize;
    

    nsCOMPtr<nsIPrincipal> mPrincipal;
    
    nsCOMPtr<imgIRequest> mImageRequest;
    
    bool mIsWriteOnly;
    

    bool mIsStillLoading;
    
    bool mCORSUsed;
  };

  static SurfaceFromElementResult SurfaceFromElement(mozilla::dom::Element *aElement,
                                                     PRUint32 aSurfaceFlags = 0);
  static SurfaceFromElementResult SurfaceFromElement(nsIImageLoadingContent *aElement,
                                                     PRUint32 aSurfaceFlags = 0);
  
  
  
  static SurfaceFromElementResult SurfaceFromElement(nsHTMLImageElement *aElement,
                                                     PRUint32 aSurfaceFlags = 0);
  static SurfaceFromElementResult SurfaceFromElement(nsHTMLCanvasElement *aElement,
                                                     PRUint32 aSurfaceFlags = 0);
  static SurfaceFromElementResult SurfaceFromElement(nsHTMLVideoElement *aElement,
                                                     PRUint32 aSurfaceFlags = 0);

  



















  static nsIContent*
    GetEditableRootContentByContentEditable(nsIDocument* aDocument);

  



  static bool NeedsPrintPreviewBackground(nsPresContext* aPresContext) {
    return aPresContext->IsRootPaginatedDocument() &&
      (aPresContext->Type() == nsPresContext::eContext_PrintPreview ||
       aPresContext->Type() == nsPresContext::eContext_PageLayout);
  }

  



  static nsresult GetFontFacesForFrames(nsIFrame* aFrame,
                                        nsFontFaceList* aFontFaceList);

  





  static nsresult GetFontFacesForText(nsIFrame* aFrame,
                                      PRInt32 aStartOffset,
                                      PRInt32 aEndOffset,
                                      bool aFollowContinuations,
                                      nsFontFaceList* aFontFaceList);

  










  static size_t SizeOfTextRunsForFrames(nsIFrame* aFrame,
                                        nsMallocSizeOfFun aMallocSizeOf,
                                        bool clear);

  


  static bool Are3DTransformsEnabled();

  



  static bool UseBackgroundNearestFiltering();

  



  static bool GPUImageScalingEnabled();

  



  static void UnionChildOverflow(nsIFrame* aFrame,
                                 nsOverflowAreas& aOverflowAreas);

  



  static bool IsContainerForFontSizeInflation(const nsIFrame *aFrame)
  {
    return aFrame->GetStateBits() & NS_FRAME_FONT_INFLATION_CONTAINER;
  }

  




  static float FontSizeInflationFor(const nsIFrame *aFrame);

  












  static nscoord InflationMinFontSizeFor(const nsIFrame *aFrame);

  






  static float FontSizeInflationInner(const nsIFrame *aFrame,
                                      nscoord aMinFontSize);

  static bool FontSizeInflationEnabled(nsPresContext *aPresContext);

  



  static PRUint32 FontSizeInflationEmPerLine() {
    return sFontSizeInflationEmPerLine;
  }

  



  static PRUint32 FontSizeInflationMinTwips() {
    return sFontSizeInflationMinTwips;
  }

  



  static PRUint32 FontSizeInflationLineThreshold() {
    return sFontSizeInflationLineThreshold;
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

#ifdef DEBUG
  




  static void
  AssertNoDuplicateContinuations(nsIFrame* aContainer,
                                 const nsFrameList& aFrameList);

  



  static void
  AssertTreeOnlyEmptyNextInFlows(nsIFrame *aSubtreeRoot);
#endif

private:
  static PRUint32 sFontSizeInflationEmPerLine;
  static PRUint32 sFontSizeInflationMinTwips;
  static PRUint32 sFontSizeInflationLineThreshold;
};

namespace mozilla {
  namespace layout {

    





    class AutoMaybeDisableFontInflation {
    public:
      AutoMaybeDisableFontInflation(nsIFrame *aFrame)
      {
        
        
        
        
        if (nsLayoutUtils::IsContainerForFontSizeInflation(aFrame)) {
          mPresContext = aFrame->PresContext();
          mOldValue = mPresContext->mInflationDisabledForShrinkWrap;
          mPresContext->mInflationDisabledForShrinkWrap = true;
        } else {
          
          mPresContext = nsnull;
        }
      }

      ~AutoMaybeDisableFontInflation()
      {
        if (mPresContext) {
          mPresContext->mInflationDisabledForShrinkWrap = mOldValue;
        }
      }
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
                    PRInt32 aValue);

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

class nsReflowFrameRunnable : public nsRunnable
{
public:
  nsReflowFrameRunnable(nsIFrame* aFrame,
                        nsIPresShell::IntrinsicDirty aIntrinsicDirty,
                        nsFrameState aBitToAdd);

  NS_DECL_NSIRUNNABLE

  nsWeakFrame mWeakFrame;
  nsIPresShell::IntrinsicDirty mIntrinsicDirty;
  nsFrameState mBitToAdd;
};

#endif 
