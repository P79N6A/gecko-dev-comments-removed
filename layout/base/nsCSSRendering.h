






































#ifndef nsCSSRendering_h___
#define nsCSSRendering_h___

#include "nsIRenderingContext.h"
#include "nsStyleConsts.h"
#include "gfxBlur.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"

struct nsPoint;
class nsStyleContext;
class nsPresContext;

struct nsCSSRendering {
  


  static nsresult Init();
  
  


  static void Shutdown();
  
  static void PaintBoxShadow(nsPresContext* aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             nsIFrame* aForFrame,
                             const nsPoint& aForFramePt);

  







  static void PaintBorder(nsPresContext* aPresContext,
                          nsIRenderingContext& aRenderingContext,
                          nsIFrame* aForFrame,
                          const nsRect& aDirtyRect,
                          const nsRect& aBorderArea,
                          const nsStyleBorder& aBorderStyle,
                          nsStyleContext* aStyleContext,
                          PRIntn aSkipSides = 0);

  







  static void PaintOutline(nsPresContext* aPresContext,
                          nsIRenderingContext& aRenderingContext,
                          nsIFrame* aForFrame,
                          const nsRect& aDirtyRect,
                          const nsRect& aBorderArea,
                          const nsStyleBorder& aBorderStyle,
                          const nsStyleOutline& aOutlineStyle,
                          nsStyleContext* aStyleContext);

  





  static void PaintFocus(nsPresContext* aPresContext,
                         nsIRenderingContext& aRenderingContext,
                         const nsRect& aFocusRect,
                         nscolor aColor);

  






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
                              PRBool aUsePrintSettings,
                              nsRect* aBGClipRect = nsnull);

  




  static void PaintBackgroundWithSC(nsPresContext* aPresContext,
                                    nsIRenderingContext& aRenderingContext,
                                    nsIFrame* aForFrame,
                                    const nsRect& aDirtyRect,
                                    const nsRect& aBorderArea,
                                    const nsStyleBackground& aColor,
                                    const nsStyleBorder& aBorder,
                                    PRBool aUsePrintSettings = PR_FALSE,
                                    nsRect* aBGClipRect = nsnull);

  



  static void DidPaint();

  
  
  static void DrawTableBorderSegment(nsIRenderingContext& aContext,
                                     PRUint8              aBorderStyle,  
                                     nscolor              aBorderColor,
                                     const nsStyleBackground* aBGColor,
                                     const nsRect&        aBorderRect,
                                     PRInt32              aAppUnitsPerCSSPixel,
                                     PRUint8              aStartBevelSide = 0,
                                     nscoord              aStartBevelOffset = 0,
                                     PRUint8              aEndBevelSide = 0,
                                     nscoord              aEndBevelOffset = 0);

  























  static void PaintDecorationLine(gfxContext* aGfxContext,
                                  const nscolor aColor,
                                  const gfxPoint& aPt,
                                  const gfxSize& aLineSize,
                                  const gfxFloat aAscent,
                                  const gfxFloat aOffset,
                                  const PRUint8 aDecoration,
                                  const PRUint8 aStyle);

  
























  static nsRect GetTextDecorationRect(nsPresContext* aPresContext,
                                      const gfxSize& aLineSize,
                                      const gfxFloat aAscent,
                                      const gfxFloat aOffset,
                                      const PRUint8 aDecoration,
                                      const PRUint8 aStyle);
};














class nsContextBoxBlur {
public:
  





































  gfxContext* Init(const gfxRect& aRect, nscoord aBlurRadius,
                   PRInt32 aAppUnitsPerDevPixel, gfxContext* aDestinationCtx);

  




  void DoPaint();

  




  gfxContext* GetContext();

protected:
  gfxAlphaBoxBlur blur;
  nsRefPtr<gfxContext> mContext;
  gfxContext* mDestinationCtx;
  
};

#endif 
