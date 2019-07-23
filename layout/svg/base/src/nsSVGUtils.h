



































#ifndef NS_SVGUTILS_H
#define NS_SVGUTILS_H


#define _USE_MATH_DEFINES
#include <math.h>

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsRect.h"
#include "gfxContext.h"
#include "nsIRenderingContext.h"

class nsIDocument;
class nsPresContext;
class nsIContent;
class nsStyleCoord;
class nsIDOMSVGRect;
class nsFrameList;
class nsIFrame;
struct nsStyleSVGPaint;
class nsIDOMSVGElement;
class nsIDOMSVGLength;
class nsIDOMSVGMatrix;
class nsIURI;
class nsSVGOuterSVGFrame;
class nsIPresShell;
class nsIDOMSVGAnimatedPreserveAspectRatio;
class nsIAtom;
class nsSVGLength2;
class nsSVGElement;
class nsSVGSVGElement;
class nsAttrValue;
class gfxContext;
class gfxASurface;
class gfxPattern;
class gfxImageSurface;
struct gfxRect;
struct gfxMatrix;
struct gfxSize;
struct gfxIntSize;
struct nsStyleFont;
class nsSVGEnum;
class nsISVGChildFrame;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


#define NS_STATE_IS_OUTER_SVG         0x00100000

#define NS_STATE_SVG_HAS_MARKERS      0x00200000

#define NS_STATE_SVG_DIRTY            0x00400000


#define NS_STATE_SVG_FILL_PSERVER     0x00800000

#define NS_STATE_SVG_STROKE_PSERVER   0x01000000

#define NS_STATE_SVG_PSERVER_MASK     0x01800000


#define NS_STATE_SVG_NONDISPLAY_CHILD 0x02000000

#define NS_STATE_SVG_PROPAGATE_TRANSFORM 0x04000000




#ifdef IS_BIG_ENDIAN
#define GFX_ARGB32_OFFSET_A 0
#define GFX_ARGB32_OFFSET_R 1
#define GFX_ARGB32_OFFSET_G 2
#define GFX_ARGB32_OFFSET_B 3
#else
#define GFX_ARGB32_OFFSET_A 3
#define GFX_ARGB32_OFFSET_R 2
#define GFX_ARGB32_OFFSET_G 1
#define GFX_ARGB32_OFFSET_B 0
#endif




#define NS_SVG_OFFSCREEN_MAX_DIMENSION 16384







PRBool NS_SVGEnabled();


#undef CLIP_MASK

class nsSVGRenderState
{
public:
  enum RenderMode { NORMAL, CLIP, CLIP_MASK };

  


  nsSVGRenderState(nsIRenderingContext *aContext);
  


  nsSVGRenderState(gfxASurface *aSurface);

  nsIRenderingContext *GetRenderingContext(nsIFrame *aFrame);
  gfxContext *GetGfxContext() { return mGfxContext; }

  void SetRenderMode(RenderMode aMode) { mRenderMode = aMode; }
  RenderMode GetRenderMode() { return mRenderMode; }

private:
  RenderMode                    mRenderMode;
  nsCOMPtr<nsIRenderingContext> mRenderingContext;
  nsRefPtr<gfxContext>          mGfxContext;
};

class nsAutoSVGRenderMode
{
public:
  nsAutoSVGRenderMode(nsSVGRenderState *aState,
                      nsSVGRenderState::RenderMode aMode) : mState(aState) {
    mOriginalMode = aState->GetRenderMode();
    aState->SetRenderMode(aMode);
  }
  ~nsAutoSVGRenderMode() { mState->SetRenderMode(mOriginalMode); }

private:
  nsSVGRenderState            *mState;
  nsSVGRenderState::RenderMode mOriginalMode;
};

#define NS_ISVGFILTERPROPERTY_IID \
{ 0x9744ee20, 0x1bcf, 0x4c62, \
 { 0x86, 0x7d, 0xd3, 0x7a, 0x91, 0x60, 0x3e, 0xef } }

class nsISVGFilterProperty : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISVGFILTERPROPERTY_IID)
  virtual void Invalidate() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISVGFilterProperty, NS_ISVGFILTERPROPERTY_IID)

class nsSVGUtils
{
public:
  


  static float GetFontSize(nsIContent *aContent);
  static float GetFontSize(nsIFrame *aFrame);
  


  static float GetFontXHeight(nsIContent *aContent);
  static float GetFontXHeight(nsIFrame *aFrame);

  


  static void UnPremultiplyImageDataAlpha(PRUint8 *data, 
                                          PRInt32 stride, 
                                          const nsIntRect &rect);
  


  static void PremultiplyImageDataAlpha(PRUint8 *data, 
                                        PRInt32 stride, 
                                        const nsIntRect &rect);
  


  static void ConvertImageDataToLinearRGB(PRUint8 *data, 
                                          PRInt32 stride, 
                                          const nsIntRect &rect);
  


  static void ConvertImageDataFromLinearRGB(PRUint8 *data, 
                                            PRInt32 stride, 
                                            const nsIntRect &rect);

  


  static nsresult ReportToConsole(nsIDocument* doc,
                                  const char* aWarning,
                                  const PRUnichar **aParams,
                                  PRUint32 aParamsLength);

  




  static float CoordToFloat(nsPresContext *aPresContext,
                            nsSVGElement *aContent,
                            const nsStyleCoord &aCoord);
  



  static nsresult GetReferencedFrame(nsIFrame **aRefFrame, nsIURI* aURI,
                                     nsIContent *aContent, nsIPresShell *aPresShell);

  


  static nsresult GetNearestViewportElement(nsIContent *aContent,
                                            nsIDOMSVGElement * *aNearestViewportElement);

  


  static nsresult GetFarthestViewportElement(nsIContent *aContent,
                                             nsIDOMSVGElement * *aFarthestViewportElement);

  


  static nsresult GetBBox(nsFrameList *aFrames, nsIDOMSVGRect **_retval);

  






  static nsRect FindFilterInvalidation(nsIFrame *aFrame, const nsRect& aRect);

  


  static void UpdateFilterRegion(nsIFrame *aFrame);

  


  static void UpdateGraphic(nsISVGChildFrame *aSVGFrame);

  


  static void NotifyAncestorsOfFilterRegionChange(nsIFrame *aFrame);

  
  enum ctxDirection { X, Y, XY };

  


  static double ComputeNormalizedHypotenuse(double aWidth, double aHeight);

  



  static float ObjectSpace(nsIDOMSVGRect *aRect, const nsSVGLength2 *aLength);

  



  static float UserSpace(nsSVGElement *aSVGElement, const nsSVGLength2 *aLength);

  



  static float UserSpace(nsIFrame *aFrame, const nsSVGLength2 *aLength);

  
  static void
  TransformPoint(nsIDOMSVGMatrix *matrix,
                 float *x, float *y);

  
  static float
  AngleBisect(float a1, float a2);

  
  static nsSVGOuterSVGFrame *
  GetOuterSVGFrame(nsIFrame *aFrame);

  




  static nsIFrame*
  GetOuterSVGFrameAndCoveredRegion(nsIFrame* aFrame, nsRect* aRect);

  
  
  static already_AddRefed<nsIDOMSVGMatrix>
  GetViewBoxTransform(float aViewportWidth, float aViewportHeight,
                      float aViewboxX, float aViewboxY,
                      float aViewboxWidth, float aViewboxHeight,
                      nsIDOMSVGAnimatedPreserveAspectRatio *aPreserveAspectRatio,
                      PRBool aIgnoreAlign = PR_FALSE);

  

  static void
  PaintChildWithEffects(nsSVGRenderState *aContext,
                        nsIntRect *aDirtyRect,
                        nsIFrame *aFrame);

  



  static void
  UpdateEffects(nsIFrame *aFrame);

  

  static PRBool
  HitTestClip(nsIFrame *aFrame, const nsPoint &aPoint);
  
  

  static nsIFrame *
  HitTestChildren(nsIFrame *aFrame, const nsPoint &aPoint);

  
  static void
  AddObserver(nsISupports *aObserver, nsISupports *aTarget);

  
  static void
  RemoveObserver(nsISupports *aObserver, nsISupports *aTarget);

  




  static already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM(nsIFrame *aFrame);

  


  static void
  NotifyChildrenOfSVGChange(nsIFrame *aFrame, PRUint32 aFlags);

  


  static nsRect
  GetCoveredRegion(const nsFrameList &aFrames);

  


  static nsRect
  ToAppPixelRect(nsPresContext *aPresContext,
                 double xmin, double ymin, double xmax, double ymax);
  static nsRect
  ToAppPixelRect(nsPresContext *aPresContext, const gfxRect& rect);

  







  static gfxIntSize
  ConvertToSurfaceSize(const gfxSize& aSize, PRBool *aResultOverflows);

  



  static gfxASurface *
  GetThebesComputationalSurface();

  


  static gfxMatrix
  ConvertSVGMatrixToThebes(nsIDOMSVGMatrix *aMatrix);

  


  static PRBool
  HitTestRect(nsIDOMSVGMatrix *aMatrix,
              float aRX, float aRY, float aRWidth, float aRHeight,
              float aX, float aY);


  static void CompositeSurfaceMatrix(gfxContext *aContext,
                                     gfxASurface *aSurface,
                                     nsIDOMSVGMatrix *aCTM, float aOpacity);

  static void CompositePatternMatrix(gfxContext *aContext,
                                     gfxPattern *aPattern,
                                     nsIDOMSVGMatrix *aCTM, float aWidth, float aHeight, float aOpacity);

  static void SetClipRect(gfxContext *aContext,
                          nsIDOMSVGMatrix *aCTM, float aX, float aY,
                          float aWidth, float aHeight);

  




  static nsresult GfxRectToIntRect(const gfxRect& aIn, nsIntRect* aOut);

  


  static void ClipToGfxRect(nsIntRect* aRect, const gfxRect& aGfxRect);

  




  static PRBool
  CanOptimizeOpacity(nsIFrame *aFrame);

  
  static float
  MaxExpansion(nsIDOMSVGMatrix *aMatrix);

  
  static already_AddRefed<nsIDOMSVGMatrix>
  AdjustMatrixForUnits(nsIDOMSVGMatrix *aMatrix,
                       nsSVGEnum *aUnits,
                       nsIFrame *aFrame);

  



  static already_AddRefed<nsIDOMSVGRect>
  GetBBox(nsIFrame *aFrame);
  








  static gfxRect
  GetRelativeRect(PRUint16 aUnits, const nsSVGLength2 *aXYWH, nsIDOMSVGRect *aBBox,
                  nsIFrame *aFrame);

#ifdef DEBUG
  static void
  WritePPM(const char *fname, gfxImageSurface *aSurface);
#endif

private:
  
  static gfxASurface *mThebesComputationalSurface;
};

#endif
