






































#ifndef nsCSSRendering_h___
#define nsCSSRendering_h___

#include "nsIRenderingContext.h"
struct nsPoint;
class nsStyleContext;
class nsPresContext;

class nsCSSRendering {
public:
  


  static nsresult Init();
  
  


  static void Shutdown();
  
  







  static void PaintBorder(nsPresContext* aPresContext,
                          nsIRenderingContext& aRenderingContext,
                          nsIFrame* aForFrame,
                          const nsRect& aDirtyRect,
                          const nsRect& aBorderArea,
                          const nsStyleBorder& aBorderStyle,
                          nsStyleContext* aStyleContext,
                          PRIntn aSkipSides,
                          nsRect* aGap = 0,
                          nscoord aHardBorderSize = 0,
                          PRBool aShouldIgnoreRounded = PR_FALSE);

  







  static void PaintOutline(nsPresContext* aPresContext,
                          nsIRenderingContext& aRenderingContext,
                          nsIFrame* aForFrame,
                          const nsRect& aDirtyRect,
                          const nsRect& aBorderArea,
                          const nsStyleBorder& aBorderStyle,
                          const nsStyleOutline& aOutlineStyle,
                          nsStyleContext* aStyleContext,
                          nsRect* aGap = 0);

  






  static PRBool FindBackground(nsPresContext* aPresContext,
                               nsIFrame* aForFrame,
                               const nsStyleBackground** aBackground,
                               PRBool* aIsCanvas);
                               
  





  static const nsStyleBackground*
  FindNonTransparentBackground(nsStyleContext* aContext,
                               PRBool aStartAtParent = PR_FALSE);

  






  static void PaintBackground(nsPresContext* aPresContext,
                              nsIRenderingContext& aRenderingContext,
                              nsIFrame* aForFrame,
                              const nsRect& aDirtyRect,
                              const nsRect& aBorderArea,
                              const nsStyleBorder& aBorder,
                              const nsStylePadding& aPadding,
                              PRBool aUsePrintSettings,
                              nsRect* aBGClipRect = nsnull);

  




  static void PaintBackgroundWithSC(nsPresContext* aPresContext,
                                    nsIRenderingContext& aRenderingContext,
                                    nsIFrame* aForFrame,
                                    const nsRect& aDirtyRect,
                                    const nsRect& aBorderArea,
                                    const nsStyleBackground& aColor,
                                    const nsStyleBorder& aBorder,
                                    const nsStylePadding& aPadding,
                                    PRBool aUsePrintSettings = PR_FALSE,
                                    nsRect* aBGClipRect = nsnull);

  



  static void DidPaint();


  static void DrawDashedSides(PRIntn startSide,
                              nsIRenderingContext& aContext,
                              const nsRect& aDirtyRect,
                              const PRUint8 borderStyles[],
                              const nscolor borderColors[],    
                              const nsRect& borderOutside,
                              const nsRect& borderInside,
                              PRIntn aSkipSides,
                              nsRect* aGap);

  static void DrawDashedSides(PRIntn startSide,
                              nsIRenderingContext& aContext,
                              const nsRect& aDirtyRect,
                              const nsStyleColor* aColorStyle,
                              const nsStyleBorder* aBorderStyle,  
                              const nsStyleOutline* aOutlineStyle,  
                              PRBool aDoOutline,
                              const nsRect& borderOutside,
                              const nsRect& borderInside,
                              PRIntn aSkipSides,
                              nsRect* aGap);

  
  static void DrawTableBorderSegment(nsIRenderingContext&     aContext,
                                     PRUint8                  aBorderStyle,  
                                     nscolor                  aBorderColor,
                                     const nsStyleBackground* aBGColor,
                                     const nsRect&            aBorderRect,
                                     PRInt32                  aAppUnitsPerCSSPixel,
                                     PRUint8                  aStartBevelSide = 0,
                                     nscoord                  aStartBevelOffset = 0,
                                     PRUint8                  aEndBevelSide = 0,
                                     nscoord                  aEndBevelOffset = 0);
  





  static nscolor TransformColor(nscolor  aMapColor,PRBool aNoBackGround);

protected:

  static void PaintBackgroundColor(nsPresContext* aPresContext,
                                   nsIRenderingContext& aRenderingContext,
                                   nsIFrame* aForFrame,
                                   const nsRect& aBgClipArea,
                                   const nsStyleBackground& aColor,
                                   const nsStyleBorder& aBorder,
                                   const nsStylePadding& aPadding,
                                   PRBool aCanPaintNonWhite);

  static void PaintRoundedBackground(nsPresContext* aPresContext,
                                     nsIRenderingContext& aRenderingContext,
                                     nsIFrame* aForFrame,
                                     const nsRect& aBorderArea,
                                     const nsStyleBackground& aColor,
                                     const nsStyleBorder& aBorder,
                                     PRInt16 aTheRadius[4],
                                     PRBool aCanPaintNonWhite);

  static nscolor MakeBevelColor(PRIntn whichSide, PRUint8 style,
                                nscolor aBackgroundColor,
                                nscolor aBorderColor);

  static void DrawLine (nsIRenderingContext& aContext, 
                        nscoord aX1, nscoord aY1, nscoord aX2, nscoord aY2,
                        nsRect* aGap);

  static void FillPolygon (nsIRenderingContext& aContext, 
                           const nsPoint aPoints[],
                           PRInt32 aNumPoints,
                           nsRect* aGap);

};







class QBCurve
{

public:
	nsFloatPoint	mAnc1;
	nsFloatPoint	mCon;
	nsFloatPoint mAnc2;

  QBCurve() {mAnc1.x=0;mAnc1.y=0;mCon=mAnc2=mAnc1;}
  void SetControls(nsFloatPoint &aAnc1,nsFloatPoint &aCon,nsFloatPoint &aAnc2) { mAnc1 = aAnc1; mCon = aCon; mAnc2 = aAnc2;}
  void SetPoints(float a1x,float a1y,float acx,float acy,float a2x,float a2y) {mAnc1.MoveTo(a1x,a1y),mCon.MoveTo(acx,acy),mAnc2.MoveTo(a2x,a2y);}









  void SubDivide(nsIRenderingContext *aRenderingContext,nsPoint  aPointArray[],PRInt32 *aCurIndex);







  void MidPointDivide(QBCurve *A,QBCurve *B);
};







class RoundedRect
{

public:
  PRInt32 mRoundness[4];

  PRBool  mDoRound;

  PRInt32 mLeft;
  PRInt32 mRight;
  PRInt32 mTop;
  PRInt32 mBottom;

  



  void  RoundRect() {mRoundness[0]=0;}

  








  void  Set(nscoord aLeft,nscoord aTop,PRInt32  aWidth,PRInt32 aHeight,PRInt16 aRadius[4],PRInt16 aNumTwipPerPix);


  





  void  CalcInsetCurves(QBCurve &aULCurve,QBCurve &aURCurve,QBCurve &aLLCurve,QBCurve &aLRCurve,nsMargin &aBorder);

  







  void GetRoundedBorders(QBCurve &aULCurve,QBCurve &aURCurve,QBCurve &aLLCurve,QBCurve &aLRCurve);

};


#endif 
