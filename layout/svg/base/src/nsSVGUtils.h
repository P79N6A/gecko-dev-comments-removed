



































#ifndef NS_SVGUTILS_H
#define NS_SVGUTILS_H


#define _USE_MATH_DEFINES
#include <math.h>

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsISVGValue.h"
#include "nsRect.h"

class nsIDocument;
class nsPresContext;
class nsIContent;
class nsStyleCoord;
class nsIDOMSVGRect;
class nsFrameList;
class nsIFrame;
struct nsStyleSVGPaint;
class nsIDOMSVGLength;
class nsIDOMSVGMatrix;
class nsIURI;
class nsSVGOuterSVGFrame;
class nsIPresShell;
class nsIDOMSVGAnimatedPreserveAspectRatio;
class nsISVGValueObserver;
class nsIAtom;
class nsSVGLength2;
class nsSVGElement;
class nsSVGSVGElement;
class nsAttrValue;
class gfxContext;
class gfxASurface;
class nsIRenderingContext;
struct gfxRect;
struct gfxMatrix;
struct gfxSize;
struct gfxIntSize;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


#define NS_STATE_IS_OUTER_SVG         0x00100000

#define NS_STATE_SVG_CLIPPED          0x00200000
#define NS_STATE_SVG_FILTERED         0x00400000
#define NS_STATE_SVG_MASKED           0x00800000

#define NS_STATE_SVG_HAS_MARKERS      0x01000000

#define NS_STATE_SVG_DIRTY            0x02000000

#define NS_STATE_SVG_FILL_PSERVER     0x04000000
#define NS_STATE_SVG_STROKE_PSERVER   0x08000000
#define NS_STATE_SVG_PSERVER_MASK     0x0c000000


#define NS_STATE_SVG_NONDISPLAY_CHILD 0x10000000




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

class nsSVGRenderState
{
public:
  enum RenderMode { NORMAL, CLIP, CLIP_MASK };

  nsSVGRenderState(nsIRenderingContext *aContext);
  nsSVGRenderState(gfxContext *aContext);

  nsIRenderingContext *GetRenderingContext() { return mRenderingContext; }
  gfxContext *GetGfxContext() { return mGfxContext; }

  void SetRenderMode(RenderMode aMode) { mRenderMode = aMode; }
  RenderMode GetRenderMode() { return mRenderMode; }

private:
  RenderMode           mRenderMode;
  nsIRenderingContext *mRenderingContext;
  gfxContext          *mGfxContext;
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

class nsSVGUtils
{
public:
  


  static void UnPremultiplyImageDataAlpha(PRUint8 *data, 
                                          PRInt32 stride, 
                                          const nsRect &rect);
  


  static void PremultiplyImageDataAlpha(PRUint8 *data, 
                                        PRInt32 stride, 
                                        const nsRect &rect);
  


  static void ConvertImageDataToLinearRGB(PRUint8 *data, 
                                          PRInt32 stride, 
                                          const nsRect &rect);
  


  static void ConvertImageDataFromLinearRGB(PRUint8 *data, 
                                            PRInt32 stride, 
                                            const nsRect &rect);

  


  static nsresult ReportToConsole(nsIDocument* doc,
                                  const char* aWarning,
                                  const PRUnichar **aParams,
                                  PRUint32 aParamsLength);

  




  static float CoordToFloat(nsPresContext *aPresContext,
                            nsSVGElement *aContent,
                            const nsStyleCoord &aCoord);
  



  static nsresult GetReferencedFrame(nsIFrame **aRefFrame, nsIURI* aURI,
                                     nsIContent *aContent, nsIPresShell *aPresShell);

  


  static nsresult GetBBox(nsFrameList *aFrames, nsIDOMSVGRect **_retval);

  



  static nsRect FindFilterInvalidation(nsIFrame *aFrame);

  


  static void UpdateFilterRegion(nsIFrame *aFrame);

  
  enum ctxDirection { X, Y, XY };

  



  static float ObjectSpace(nsIDOMSVGRect *aRect, nsSVGLength2 *aLength);

  



  static float UserSpace(nsSVGElement *aSVGElement, nsSVGLength2 *aLength);

  
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
                        nsRect *aDirtyRect,
                        nsIFrame *aFrame);

  


  static void
  StyleEffects(nsIFrame *aFrame);

  


  static PRBool
  HitTestClip(nsIFrame *aFrame, float x, float y);

  
  
  static void
  HitTestChildren(nsIFrame *aFrame, float x, float y, nsIFrame **aResult);

  
  static void
  AddObserver(nsISupports *aObserver, nsISupports *aTarget);

  
  static void
  RemoveObserver(nsISupports *aObserver, nsISupports *aTarget);

  



  static already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM(nsIFrame *aFrame);

  


  static nsRect
  GetCoveredRegion(const nsFrameList &aFrames);

  


  static nsRect
  ToBoundingPixelRect(double xmin, double ymin, double xmax, double ymax);
  static nsRect
  ToBoundingPixelRect(const gfxRect& rect);

  







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

  static void SetClipRect(gfxContext *aContext,
                          nsIDOMSVGMatrix *aCTM, float aX, float aY,
                          float aWidth, float aHeight);

  




  static PRBool
  CanOptimizeOpacity(nsIFrame *aFrame);

private:
  
  static gfxASurface *mThebesComputationalSurface;
};

#endif
