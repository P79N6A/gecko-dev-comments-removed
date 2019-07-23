






































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
  
  static void PaintBoxShadowInner(nsPresContext* aPresContext,
                                  nsIRenderingContext& aRenderingContext,
                                  nsIFrame* aForFrame,
                                  const nsRect& aFrameArea,
                                  const nsRect& aDirtyRect);

  static void PaintBoxShadowOuter(nsPresContext* aPresContext,
                                  nsIRenderingContext& aRenderingContext,
                                  nsIFrame* aForFrame,
                                  const nsRect& aFrameArea,
                                  const nsRect& aDirtyRect);

  




  static PRBool GetBorderRadiusTwips(const nsStyleCorners& aBorderRadius,
                                     const nscoord& aFrameWidth,
                                     nscoord aRadii[8]);

  







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

  


  static void PaintGradient(nsPresContext* aPresContext,
                            nsIRenderingContext& aRenderingContext,
                            nsStyleGradient* aGradient,
                            const nsRect& aDirtyRect,
                            const nsRect& aOneCellArea,
                            const nsRect& aFillArea,
                            PRBool aRepeat);

  


  static nsIFrame* FindRootFrame(nsIFrame* aForFrame);

  


  static PRBool IsCanvasFrame(nsIFrame* aFrame);

  





  static PRBool FindBackground(nsPresContext* aPresContext,
                               nsIFrame* aForFrame,
                               const nsStyleBackground** aBackground);

  




  static const nsStyleBackground* FindRootFrameBackground(nsIFrame* aForFrame);

  








  static nsStyleContext*
  FindNonTransparentBackground(nsStyleContext* aContext,
                               PRBool aStartAtParent = PR_FALSE);

  


  static nscolor
  DetermineBackgroundColor(nsPresContext* aPresContext,
                           const nsStyleBackground& aBackground,
                           nsIFrame* aFrame);

  






  enum {
    



    PAINT_WILL_PAINT_BORDER = 0x01
  };
  static void PaintBackground(nsPresContext* aPresContext,
                              nsIRenderingContext& aRenderingContext,
                              nsIFrame* aForFrame,
                              const nsRect& aDirtyRect,
                              const nsRect& aBorderArea,
                              PRUint32 aFlags,
                              nsRect* aBGClipRect = nsnull);

  




  static void PaintBackgroundWithSC(nsPresContext* aPresContext,
                                    nsIRenderingContext& aRenderingContext,
                                    nsIFrame* aForFrame,
                                    const nsRect& aDirtyRect,
                                    const nsRect& aBorderArea,
                                    const nsStyleBackground& aBackground,
                                    const nsStyleBorder& aBorder,
                                    PRUint32 aFlags,
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

  enum {
    DECORATION_STYLE_NONE   = 0,
    DECORATION_STYLE_SOLID  = 1,
    DECORATION_STYLE_DOTTED = 2,
    DECORATION_STYLE_DASHED = 3,
    DECORATION_STYLE_DOUBLE = 4,
    DECORATION_STYLE_WAVY   = 5
  };

  































  static void PaintDecorationLine(gfxContext* aGfxContext,
                                  const nscolor aColor,
                                  const gfxPoint& aPt,
                                  const gfxSize& aLineSize,
                                  const gfxFloat aAscent,
                                  const gfxFloat aOffset,
                                  const PRUint8 aDecoration,
                                  const PRUint8 aStyle,
                                  const gfxFloat aDescentLimit = -1.0);

  
































  static nsRect GetTextDecorationRect(nsPresContext* aPresContext,
                                      const gfxSize& aLineSize,
                                      const gfxFloat aAscent,
                                      const gfxFloat aOffset,
                                      const PRUint8 aDecoration,
                                      const PRUint8 aStyle,
                                      const gfxFloat aDescentLimit = -1.0);

protected:
  static gfxRect GetTextDecorationRectInternal(const gfxPoint& aPt,
                                               const gfxSize& aLineSize,
                                               const gfxFloat aAscent,
                                               const gfxFloat aOffset,
                                               const PRUint8 aDecoration,
                                               const PRUint8 aStyle,
                                               const gfxFloat aDscentLimit);
};














class nsContextBoxBlur {
public:
  








































  gfxContext* Init(const gfxRect& aRect, nscoord aBlurRadius,
                   PRInt32 aAppUnitsPerDevPixel, gfxContext* aDestinationCtx,
                   const gfxRect& aDirtyRect);

  




  void DoPaint();

  




  gfxContext* GetContext();

protected:
  gfxAlphaBoxBlur blur;
  nsRefPtr<gfxContext> mContext;
  gfxContext* mDestinationCtx;
  
};

#endif 
