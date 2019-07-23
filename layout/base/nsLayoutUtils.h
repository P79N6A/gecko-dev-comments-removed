







































#ifndef nsLayoutUtils_h__
#define nsLayoutUtils_h__

class nsIFormControlFrame;
class nsPresContext;
class nsIContent;
class nsIAtom;
class nsIScrollableView;
class nsIScrollableFrame;
class nsIDOMEvent;
class nsRegion;
class nsDisplayListBuilder;
class nsIFontMetrics;

#include "prtypes.h"
#include "nsStyleContext.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"
#include "nsIView.h"
#include "nsIFrame.h"
#include "nsThreadUtils.h"

class nsBlockFrame;






class nsLayoutUtils
{
public:
  







  static nsIFrame* GetBeforeFrame(nsIFrame* aFrame);

  








  static nsIFrame* GetAfterFrame(nsIFrame* aFrame);

  








  static nsIFrame* GetClosestFrameOfType(nsIFrame* aFrame, nsIAtom* aFrameType);

  







  static nsIFrame* GetPageFrame(nsIFrame* aFrame)
  {
    return GetClosestFrameOfType(aFrame, nsGkAtoms::pageFrame);
  }

  













  static PRBool IsGeneratedContentFor(nsIContent* aContent, nsIFrame* aFrame,
                                      nsIAtom* aPseudoElement);

  














  static PRInt32 CompareTreePosition(nsIContent* aContent1,
                                     nsIContent* aContent2,
                                     nsIContent* aCommonAncestor = nsnull)
  {
    return DoCompareTreePosition(aContent1, aContent2, -1, 1, aCommonAncestor);
  }

  





  static PRInt32 DoCompareTreePosition(nsIContent* aContent1,
                                       nsIContent* aContent2,
                                       PRInt32 aIf1Ancestor,
                                       PRInt32 aIf2Ancestor,
                                       nsIContent* aCommonAncestor = nsnull);

  


















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

  




  static nsIFrame* GetCrossDocParentFrame(nsIFrame* aFrame);
  
  






  static PRBool IsProperAncestorFrame(nsIFrame* aAncestorFrame, nsIFrame* aFrame,
                                      nsIFrame* aCommonAncestor = nsnull);

  


  static PRBool IsProperAncestorFrameCrossDoc(nsIFrame* aAncestorFrame, nsIFrame* aFrame,
                                              nsIFrame* aCommonAncestor = nsnull);

  




  static nsIFrame* GetFrameFor(nsIView *aView)
  { return static_cast<nsIFrame*>(aView->GetClientData()); }
  
  





  static nsIScrollableFrame* GetScrollableFrameFor(nsIScrollableView *aScrollableView);

  


  static nsIScrollableFrame* GetScrollableFrameFor(nsIFrame *aScrolledFrame);

  static nsPresContext::ScrollbarStyles
    ScrollbarStylesOfView(nsIScrollableView *aScrollableView);

  










  enum Direction { eHorizontal, eVertical, eEither };
  static nsIScrollableView* GetNearestScrollingView(nsIView* aView,
                                                    Direction aDirection);

  










  static PRBool HasPseudoStyle(nsIContent* aContent,
                               nsStyleContext* aStyleContext,
                               nsIAtom* aPseudoElement,
                               nsPresContext* aPresContext)
  {
    NS_PRECONDITION(aPresContext, "Must have a prescontext");
    NS_PRECONDITION(aPseudoElement, "Must have a pseudo name");

    nsRefPtr<nsStyleContext> pseudoContext;
    if (aContent) {
      pseudoContext = aPresContext->StyleSet()->
        ProbePseudoStyleFor(aContent, aPseudoElement, aStyleContext);
    }
    return pseudoContext != nsnull;
  }

  



  static nsIFrame* GetFloatFromPlaceholder(nsIFrame* aPossiblePlaceholder);

  
  
  static PRUint8 CombineBreakType(PRUint8 aOrigBreakType, PRUint8 aNewBreakType);

  



  static PRBool IsInitialContainingBlock(nsIFrame* aFrame);

  








  static nsPoint GetDOMEventCoordinatesRelativeTo(nsIDOMEvent* aDOMEvent,
                                                  nsIFrame* aFrame);

  








  static nsPoint GetEventCoordinatesRelativeTo(const nsEvent* aEvent,
                                               nsIFrame* aFrame);














  static nsPoint GetEventCoordinatesForNearestView(nsEvent* aEvent,
                                                   nsIFrame* aFrame,
                                                   nsIView** aView = nsnull);









  static nsPoint TranslateWidgetToView(nsPresContext* aPresContext, 
                                       nsIWidget* aWidget, nsIntPoint aPt,
                                       nsIView* aView);

  







  static nsIFrame* GetFrameForPoint(nsIFrame* aFrame, nsPoint aPt,
                                    PRBool aShouldIgnoreSuppression = PR_FALSE);

  










  static nsresult PaintFrame(nsIRenderingContext* aRenderingContext, nsIFrame* aFrame,
                             const nsRegion& aDirtyRegion, nscolor aBackground);

  







































  static nsresult ComputeRepaintRegionForCopy(nsIFrame* aRootFrame,
                                              nsIFrame* aMovingFrame,
                                              nsPoint aDelta,
                                              const nsRect& aCopyRect,
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

  class RectCallback {
  public:
    virtual void AddRect(const nsRect& aRect) = 0;
  };
  








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

  



  static nsBlockFrame* GetAsBlock(nsIFrame* aFrame);
  
  



  static nsIFrame* GetParentOrPlaceholderFor(nsFrameManager* aFrameManager,
                                             nsIFrame* aFrame);

  









  static nsIFrame*
  GetClosestCommonAncestorViaPlaceholders(nsIFrame* aFrame1, nsIFrame* aFrame2,
                                          nsIFrame* aKnownCommonAncestorHint);

  


  static nsIFrame*
  GetNextContinuationOrSpecialSibling(nsIFrame *aFrame);

  






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
                         nsPoint              aPoint);

  static nscoord GetStringWidth(const nsIFrame*      aFrame,
                                nsIRenderingContext* aContext,
                                const PRUnichar*     aString,
                                PRInt32              aLength);

  







  static PRBool GetFirstLineBaseline(const nsIFrame* aFrame, nscoord* aResult);

  







  static PRBool GetLastLineBaseline(const nsIFrame* aFrame, nscoord* aResult);

  







  static nscoord CalculateContentBottom(nsIFrame* aFrame);

  







  static nsIFrame* GetClosestLayer(nsIFrame* aFrame);

  
















  static nsresult DrawImage(nsIRenderingContext* aRenderingContext,
                            imgIContainer* aImage,
                            const nsRect& aDestRect,
                            const nsRect& aDirtyRect,
                            const nsRect* aSourceRect = nsnull);

  


  static void SetFontFromStyle(nsIRenderingContext* aRC, nsStyleContext* aSC);

  




  static PRBool HasNonZeroSide(const nsStyleSides& aSides);

  




  static nsTransparencyMode GetFrameTransparency(nsIFrame* aFrame);

  






  static PRUint32 GetTextRunFlagsForStyle(nsStyleContext* aStyleContext,
                                          const nsStyleText* aStyleText,
                                          const nsStyleFont* aStyleFont);

  





  static void GetRectDifferenceStrips(const nsRect& aR1, const nsRect& aR2,
                                      nsRect* aHStrip, nsRect* aVStrip);

  




  static nsIDeviceContext*
  GetDeviceContextForScreenInfo(nsIDocShell* aDocShell);

  



#ifdef DEBUG
  static PRBool sDisableGetUsedXAssertions;
#endif
};

class nsAutoDisableGetUsedXAssertions
{
#ifdef DEBUG
public:
  nsAutoDisableGetUsedXAssertions()
    : mOldValue(nsLayoutUtils::sDisableGetUsedXAssertions)
  {
    nsLayoutUtils::sDisableGetUsedXAssertions = PR_TRUE;
  }
  ~nsAutoDisableGetUsedXAssertions()
  {
    nsLayoutUtils::sDisableGetUsedXAssertions = mOldValue;
  }

private:
  PRBool mOldValue;
#endif  
};

class nsSetAttrRunnable : public nsRunnable
{
public:
  nsSetAttrRunnable(nsIContent* aContent, nsIAtom* aAttrName,
                    const nsAString& aValue);

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
