






































#ifndef nsCSSRendering_h___
#define nsCSSRendering_h___

#include "nsStyleConsts.h"
#include "gfxBlur.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"

struct nsPoint;
class nsStyleContext;
class nsPresContext;
class nsRenderingContext;

struct nsCSSRendering {
  


  static nsresult Init();
  
  


  static void Shutdown();
  
  static void PaintBoxShadowInner(nsPresContext* aPresContext,
                                  nsRenderingContext& aRenderingContext,
                                  nsIFrame* aForFrame,
                                  const nsRect& aFrameArea,
                                  const nsRect& aDirtyRect);

  static void PaintBoxShadowOuter(nsPresContext* aPresContext,
                                  nsRenderingContext& aRenderingContext,
                                  nsIFrame* aForFrame,
                                  const nsRect& aFrameArea,
                                  const nsRect& aDirtyRect);

  static void ComputePixelRadii(const nscoord *aAppUnitsRadii,
                                nscoord aAppUnitsPerPixel,
                                gfxCornerSizes *oBorderRadii);

  




  static void PaintBorder(nsPresContext* aPresContext,
                          nsRenderingContext& aRenderingContext,
                          nsIFrame* aForFrame,
                          const nsRect& aDirtyRect,
                          const nsRect& aBorderArea,
                          nsStyleContext* aStyleContext,
                          PRIntn aSkipSides = 0);

  



  static void PaintBorderWithStyleBorder(nsPresContext* aPresContext,
                                         nsRenderingContext& aRenderingContext,
                                         nsIFrame* aForFrame,
                                         const nsRect& aDirtyRect,
                                         const nsRect& aBorderArea,
                                         const nsStyleBorder& aBorderStyle,
                                         nsStyleContext* aStyleContext,
                                         PRIntn aSkipSides = 0);


  




  static void PaintOutline(nsPresContext* aPresContext,
                          nsRenderingContext& aRenderingContext,
                          nsIFrame* aForFrame,
                          const nsRect& aDirtyRect,
                          const nsRect& aBorderArea,
                          nsStyleContext* aStyleContext);

  





  static void PaintFocus(nsPresContext* aPresContext,
                         nsRenderingContext& aRenderingContext,
                         const nsRect& aFocusRect,
                         nscolor aColor);

  


  static void PaintGradient(nsPresContext* aPresContext,
                            nsRenderingContext& aRenderingContext,
                            nsStyleGradient* aGradient,
                            const nsRect& aDirtyRect,
                            const nsRect& aOneCellArea,
                            const nsRect& aFillArea);

  






  static nsIFrame* FindBackgroundStyleFrame(nsIFrame* aForFrame);

  


  static PRBool IsCanvasFrame(nsIFrame* aFrame);

  





  static PRBool FindBackground(nsPresContext* aPresContext,
                               nsIFrame* aForFrame,
                               nsStyleContext** aBackgroundSC);

  




  static nsStyleContext* FindRootFrameBackground(nsIFrame* aForFrame);

  










  static nsStyleContext*
  FindCanvasBackground(nsIFrame* aForFrame, nsIFrame* aRootElementFrame)
  {
    NS_ABORT_IF_FALSE(IsCanvasFrame(aForFrame), "not a canvas frame");
    if (aRootElementFrame)
      return FindRootFrameBackground(aRootElementFrame);

    
    
    
    return aForFrame->GetStyleContext();
  }

  








  static nsIFrame*
  FindNonTransparentBackgroundFrame(nsIFrame* aFrame,
                                    PRBool aStartAtParent = PR_FALSE);

  


  static nscolor
  DetermineBackgroundColor(nsPresContext* aPresContext,
                           nsStyleContext* aStyleContext,
                           nsIFrame* aFrame);

  



  enum {
    



    PAINTBG_WILL_PAINT_BORDER = 0x01,
    


    PAINTBG_SYNC_DECODE_IMAGES = 0x02,
    



    PAINTBG_TO_WINDOW = 0x04
  };
  static void PaintBackground(nsPresContext* aPresContext,
                              nsRenderingContext& aRenderingContext,
                              nsIFrame* aForFrame,
                              const nsRect& aDirtyRect,
                              const nsRect& aBorderArea,
                              PRUint32 aFlags,
                              nsRect* aBGClipRect = nsnull);

  




  static void PaintBackgroundWithSC(nsPresContext* aPresContext,
                                    nsRenderingContext& aRenderingContext,
                                    nsIFrame* aForFrame,
                                    const nsRect& aDirtyRect,
                                    const nsRect& aBorderArea,
                                    nsStyleContext *aStyleContext,
                                    const nsStyleBorder& aBorder,
                                    PRUint32 aFlags,
                                    nsRect* aBGClipRect = nsnull);

  




  static nsRect GetBackgroundLayerRect(nsPresContext* aPresContext,
                                       nsIFrame* aForFrame,
                                       const nsRect& aBorderArea,
                                       const nsStyleBackground& aBackground,
                                       const nsStyleBackground::Layer& aLayer);

  



  static void DidPaint();

  
  
  static void DrawTableBorderSegment(nsRenderingContext& aContext,
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
  enum {
    FORCE_MASK = 0x01
  };
  















































  gfxContext* Init(const nsRect& aRect, nscoord aSpreadRadius,
                   nscoord aBlurRadius,
                   PRInt32 aAppUnitsPerDevPixel, gfxContext* aDestinationCtx,
                   const nsRect& aDirtyRect, const gfxRect* aSkipRect,
                   PRUint32 aFlags = 0);

  




  void DoEffects();
  
  




  void DoPaint();

  




  gfxContext* GetContext();


  




  static nsMargin GetBlurRadiusMargin(nscoord aBlurRadius,
                                      PRInt32 aAppUnitsPerDevPixel);

protected:
  gfxAlphaBoxBlur blur;
  nsRefPtr<gfxContext> mContext;
  gfxContext* mDestinationCtx;
  
};

#endif
