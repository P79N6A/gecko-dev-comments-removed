







































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
class nsIFontMetrics;
class nsClientRectList;

#include "prtypes.h"
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

class nsBlockFrame;






class nsLayoutUtils
{
public:

  



  static nsIAtom* GetChildListNameFor(nsIFrame* aChildFrame);

  







  static nsIFrame* GetBeforeFrame(nsIFrame* aFrame);

  








  static nsIFrame* GetAfterFrame(nsIFrame* aFrame);

  








  static nsIFrame* GetClosestFrameOfType(nsIFrame* aFrame, nsIAtom* aFrameType);

  







  static nsIFrame* GetPageFrame(nsIFrame* aFrame)
  {
    return GetClosestFrameOfType(aFrame, nsGkAtoms::pageFrame);
  }

  





  static nsIFrame* GetStyleFrame(nsIFrame* aPrimaryFrame);

  













  static PRBool IsGeneratedContentFor(nsIContent* aContent, nsIFrame* aFrame,
                                      nsIAtom* aPseudoElement);

  














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

  






  static PRBool IsProperAncestorFrame(nsIFrame* aAncestorFrame, nsIFrame* aFrame,
                                      nsIFrame* aCommonAncestor = nsnull);

  





  static PRBool IsProperAncestorFrameCrossDoc(nsIFrame* aAncestorFrame, nsIFrame* aFrame,
                                              nsIFrame* aCommonAncestor = nsnull);

  









  static PRBool IsAncestorFrameCrossDoc(nsIFrame* aAncestorFrame, nsIFrame* aFrame,
                                        nsIFrame* aCommonAncestor = nsnull);

  




  static nsIFrame* GetFrameFor(nsIView *aView)
  { return static_cast<nsIFrame*>(aView->GetClientData()); }

  


  static nsIScrollableFrame* GetScrollableFrameFor(nsIFrame *aScrolledFrame);

  











  enum Direction { eHorizontal, eVertical };
  static nsIScrollableFrame* GetNearestScrollableFrameForDirection(nsIFrame* aFrame,
                                                                   Direction aDirection);

  








  static nsIScrollableFrame* GetNearestScrollableFrame(nsIFrame* aFrame);

  










  static PRBool HasPseudoStyle(nsIContent* aContent,
                               nsStyleContext* aStyleContext,
                               nsCSSPseudoElements::Type aPseudoElement,
                               nsPresContext* aPresContext)
  {
    NS_PRECONDITION(aPresContext, "Must have a prescontext");

    nsRefPtr<nsStyleContext> pseudoContext;
    if (aContent) {
      pseudoContext = aPresContext->StyleSet()->
        ProbePseudoElementStyle(aContent->AsElement(), aPseudoElement,
                                aStyleContext);
    }
    return pseudoContext != nsnull;
  }

  



  static nsIFrame* GetFloatFromPlaceholder(nsIFrame* aPlaceholder);

  
  
  static PRUint8 CombineBreakType(PRUint8 aOrigBreakType, PRUint8 aNewBreakType);

  








  static nsPoint GetDOMEventCoordinatesRelativeTo(nsIDOMEvent* aDOMEvent,
                                                  nsIFrame* aFrame);

  








  static nsPoint GetEventCoordinatesRelativeTo(const nsEvent* aEvent,
                                               nsIFrame* aFrame);

  





  static nsIFrame* GetPopupFrameForEventCoordinates(const nsEvent* aEvent);









  static nsPoint TranslateWidgetToView(nsPresContext* aPresContext,
                                       nsIWidget* aWidget, nsIntPoint aPt,
                                       nsIView* aView);

  











  static gfxMatrix ChangeMatrixBasis(const gfxPoint &aOrigin, const gfxMatrix &aMatrix);

  









  static nsIFrame* GetFrameForPoint(nsIFrame* aFrame, nsPoint aPt,
                                    PRBool aShouldIgnoreSuppression = PR_FALSE,
                                    PRBool aIgnoreRootScrollFrame = PR_FALSE);

  










  static nsresult GetFramesForArea(nsIFrame* aFrame, const nsRect& aRect,
                                   nsTArray<nsIFrame*> &aOutFrames,
                                   PRBool aShouldIgnoreSuppression = PR_FALSE,
                                   PRBool aIgnoreRootScrollFrame = PR_FALSE);

  








  static nsPoint InvertTransformsToRoot(nsIFrame* aFrame,
                                        const nsPoint &aPt);


  








  static nsRect MatrixTransformRect(const nsRect &aBounds,
                                    const gfxMatrix &aMatrix, float aFactor);

  








  static nsPoint MatrixTransformPoint(const nsPoint &aPoint,
                                      const gfxMatrix &aMatrix, float aFactor);

  







  static nsRect RoundGfxRectToAppRect(const gfxRect &aRect, float aFactor);

  




  static nsresult GfxRectToIntRect(const gfxRect& aIn, nsIntRect* aOut);

  enum {
    PAINT_IN_TRANSFORM = 0x01,
    PAINT_SYNC_DECODE_IMAGES = 0x02,
    PAINT_WIDGET_LAYERS = 0x04
  };

  


































  static nsresult PaintFrame(nsIRenderingContext* aRenderingContext, nsIFrame* aFrame,
                             const nsRegion& aDirtyRegion, nscolor aBackstop,
                             PRUint32 aFlags = 0);

  













































  static nsresult ComputeRepaintRegionForCopy(nsIFrame* aRootFrame,
                                              nsIFrame* aMovingFrame,
                                              nsPoint aDelta,
                                              const nsRect& aUpdateRect,
                                              nsRegion* aBlitRegion,
                                              nsRegion* aRepaintRegion);

  



  static PRInt32 GetZIndex(nsIFrame* aFrame);

  











  static PRBool
  BinarySearchForPosition(nsIRenderingContext* acx,
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
    nsRect       mResultRect;
    nsRect       mFirstRect;
    PRPackedBool mSeenFirstRect;

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

  








  static void GetAllInFlowRects(nsIFrame* aFrame, nsIFrame* aRelativeTo,
                                RectCallback* aCallback);

  



  static nsRect GetAllInFlowRectsUnion(nsIFrame* aFrame, nsIFrame* aRelativeTo);

  




  static nsRect GetTextShadowRectsUnion(const nsRect& aTextAndDecorationsRect,
                                        nsIFrame* aFrame);

  





  static nsresult GetFontMetricsForFrame(nsIFrame* aFrame,
                                         nsIFontMetrics** aFontMetrics);

  





  static nsresult GetFontMetricsForStyleContext(nsStyleContext* aStyleContext,
                                                nsIFontMetrics** aFontMetrics);

  




  static nsIFrame* FindChildContainingDescendant(nsIFrame* aParent, nsIFrame* aDescendantFrame);

  


  static nsBlockFrame* FindNearestBlockAncestor(nsIFrame* aFrame);

  



  static nsIFrame* GetNonGeneratedAncestor(nsIFrame* aFrame);

  



  static nsBlockFrame* GetAsBlock(nsIFrame* aFrame);

  



  static nsIFrame* GetParentOrPlaceholderFor(nsFrameManager* aFrameManager,
                                             nsIFrame* aFrame);

  


  static nsIFrame*
  GetNextContinuationOrSpecialSibling(nsIFrame *aFrame);

  



  static nsIFrame*
  GetFirstContinuationOrSpecialSibling(nsIFrame *aFrame);

  






  static PRBool IsViewportScrollbarFrame(nsIFrame* aFrame);

  





  enum IntrinsicWidthType { MIN_WIDTH, PREF_WIDTH };
  static nscoord IntrinsicForContainer(nsIRenderingContext* aRenderingContext,
                                       nsIFrame* aFrame,
                                       IntrinsicWidthType aType);

  



  static nscoord ComputeWidthDependentValue(
                   nscoord              aContainingBlockWidth,
                   const nsStyleCoord&  aCoord);

  
















  static nscoord ComputeWidthValue(
                   nsIRenderingContext* aRenderingContext,
                   nsIFrame*            aFrame,
                   nscoord              aContainingBlockWidth,
                   nscoord              aContentEdgeToBoxSizing,
                   nscoord              aBoxSizingToMarginEdge,
                   const nsStyleCoord&  aCoord);

  



  static nscoord ComputeHeightDependentValue(
                   nscoord              aContainingBlockHeight,
                   const nsStyleCoord&  aCoord);

  




  static nsSize ComputeSizeWithIntrinsicDimensions(
                    nsIRenderingContext* aRenderingContext, nsIFrame* aFrame,
                    const nsIFrame::IntrinsicSize& aIntrinsicSize,
                    nsSize aIntrinsicRatio, nsSize aCBSize,
                    nsSize aMargin, nsSize aBorder, nsSize aPadding);

  
  static nscoord PrefWidthFromInline(nsIFrame* aFrame,
                                     nsIRenderingContext* aRenderingContext);

  
  static nscoord MinWidthFromInline(nsIFrame* aFrame,
                                    nsIRenderingContext* aRenderingContext);

  static void DrawString(const nsIFrame*      aFrame,
                         nsIRenderingContext* aContext,
                         const PRUnichar*     aString,
                         PRInt32              aLength,
                         nsPoint              aPoint,
                         PRUint8              aDirection = NS_STYLE_DIRECTION_INHERIT);

  static nscoord GetStringWidth(const nsIFrame*      aFrame,
                                nsIRenderingContext* aContext,
                                const PRUnichar*     aString,
                                PRInt32              aLength);

  





  static nscoord GetCenteredFontBaseline(nsIFontMetrics* aFontMetrics,
                                         nscoord         aLineHeight);

  







  static PRBool GetFirstLineBaseline(const nsIFrame* aFrame, nscoord* aResult);

  






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
  static PRBool GetFirstLinePosition(const nsIFrame* aFrame,
                                     LinePosition* aResult);


  







  static PRBool GetLastLineBaseline(const nsIFrame* aFrame, nscoord* aResult);

  







  static nscoord CalculateContentBottom(nsIFrame* aFrame);

  





  static nsIFrame* GetClosestLayer(nsIFrame* aFrame);

  


  static gfxPattern::GraphicsFilter GetGraphicsFilterForFrame(nsIFrame* aFrame);

  



  














  static nsresult DrawImage(nsIRenderingContext* aRenderingContext,
                            imgIContainer*       aImage,
                            gfxPattern::GraphicsFilter aGraphicsFilter,
                            const nsRect&        aDest,
                            const nsRect&        aFill,
                            const nsPoint&       aAnchor,
                            const nsRect&        aDirty,
                            PRUint32             aImageFlags);

  














  static nsresult DrawSingleUnscaledImage(nsIRenderingContext* aRenderingContext,
                                          imgIContainer*       aImage,
                                          const nsPoint&       aDest,
                                          const nsRect&        aDirty,
                                          PRUint32             aImageFlags,
                                          const nsRect*        aSourceArea = nsnull);

  














  static nsresult DrawSingleImage(nsIRenderingContext* aRenderingContext,
                                  imgIContainer*       aImage,
                                  gfxPattern::GraphicsFilter aGraphicsFilter,
                                  const nsRect&        aDest,
                                  const nsRect&        aDirty,
                                  PRUint32             aImageFlags,
                                  const nsRect*        aSourceArea = nsnull);

  






  static nsRect GetWholeImageDestination(const nsIntSize& aWholeImageSize,
                                         const nsRect& aImageSourceArea,
                                         const nsRect& aDestArea);

  


  static void SetFontFromStyle(nsIRenderingContext* aRC, nsStyleContext* aSC);

  










  static PRBool HasNonZeroCorner(const nsStyleCorners& aCorners);

  



  static PRBool HasNonZeroCornerOnSide(const nsStyleCorners& aCorners,
                                       mozilla::css::Side aSide);

  









  static nsTransparencyMode GetFrameTransparency(nsIFrame* aBackgroundFrame,
                                                 nsIFrame* aCSSRootFrame);

  



  static PRBool IsPopup(nsIFrame* aFrame);

  



  static nsIFrame* GetDisplayRootFrame(nsIFrame* aFrame);

  






  static PRUint32 GetTextRunFlagsForStyle(nsStyleContext* aStyleContext,
                                          const nsStyleText* aStyleText,
                                          const nsStyleFont* aStyleFont);

  





  static void GetRectDifferenceStrips(const nsRect& aR1, const nsRect& aR2,
                                      nsRect* aHStrip, nsRect* aVStrip);

  




  static nsIDeviceContext*
  GetDeviceContextForScreenInfo(nsIDocShell* aDocShell);

  





  static PRBool IsReallyFixedPos(nsIFrame* aFrame);

  



  static PRBool FrameIsNonFirstInIBSplit(const nsIFrame* aFrame) {
    return (aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL) &&
      aFrame->GetFirstContinuation()->
        Properties().Get(nsIFrame::IBSplitSpecialPrevSibling());
  }

  



  static PRBool FrameIsNonLastInIBSplit(const nsIFrame* aFrame) {
    return (aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL) &&
      aFrame->GetFirstContinuation()->
        Properties().Get(nsIFrame::IBSplitSpecialSibling());
  }

  

















  enum {
    
    SFE_WANT_NEW_SURFACE   = 1 << 0,
    
    SFE_WANT_IMAGE_SURFACE = 1 << 1,
    

    SFE_WANT_FIRST_FRAME = 1 << 2
  };

  struct SurfaceFromElementResult {
    SurfaceFromElementResult() : mIsStillLoading(PR_FALSE) {}

    
    nsRefPtr<gfxASurface> mSurface;
    
    gfxIntSize mSize;
    
    nsCOMPtr<nsIPrincipal> mPrincipal;
    
    PRBool mIsWriteOnly;
    

    PRBool mIsStillLoading;
  };

  static SurfaceFromElementResult SurfaceFromElement(nsIDOMElement *aElement,
                                                     PRUint32 aSurfaceFlags = 0);

  



















  static nsIContent*
    GetEditableRootContentByContentEditable(nsIDocument* aDocument);
};

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
