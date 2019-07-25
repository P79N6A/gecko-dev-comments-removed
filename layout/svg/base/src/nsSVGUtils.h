



































#ifndef NS_SVGUTILS_H
#define NS_SVGUTILS_H


#define _USE_MATH_DEFINES
#include <math.h>

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsRect.h"
#include "gfxContext.h"
#include "nsRenderingContext.h"
#include "gfxRect.h"
#include "gfxMatrix.h"
#include "nsSVGMatrix.h"

class nsIDocument;
class nsPresContext;
class nsIContent;
class nsStyleContext;
class nsStyleCoord;
class nsFrameList;
class nsIFrame;
struct nsStyleSVGPaint;
class nsIDOMSVGElement;
class nsIDOMSVGLength;
class nsIURI;
class nsSVGOuterSVGFrame;
class nsIAtom;
class nsSVGLength2;
class nsSVGElement;
class nsSVGSVGElement;
class nsAttrValue;
class gfxContext;
class gfxASurface;
class gfxPattern;
class gfxImageSurface;
struct gfxSize;
struct nsStyleFont;
class nsSVGEnum;
class nsISVGChildFrame;
class nsSVGGeometryFrame;
class nsSVGDisplayContainerFrame;

namespace mozilla {
class SVGAnimatedPreserveAspectRatio;
class SVGPreserveAspectRatio;
namespace dom {
class Element;
} 
} 

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


#define NS_STATE_IS_OUTER_SVG                    NS_FRAME_STATE_BIT(20)

#define NS_STATE_SVG_DIRTY                       NS_FRAME_STATE_BIT(21)


#define NS_STATE_SVG_NONDISPLAY_CHILD            NS_FRAME_STATE_BIT(22)


#define NS_STATE_SVG_CLIPPATH_CHILD              NS_FRAME_STATE_BIT(23)




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





#define NS_SVG_OFFSCREEN_MAX_DIMENSION 4096

#define SVG_WSP_DELIM       "\x20\x9\xD\xA"
#define SVG_COMMA_WSP_DELIM "," SVG_WSP_DELIM

inline PRBool
IsSVGWhitespace(char aChar)
{
  return aChar == '\x20' || aChar == '\x9' ||
         aChar == '\xD'  || aChar == '\xA';
}

inline PRBool
IsSVGWhitespace(PRUnichar aChar)
{
  return aChar == PRUnichar('\x20') || aChar == PRUnichar('\x9') ||
         aChar == PRUnichar('\xD')  || aChar == PRUnichar('\xA');
}

#ifdef MOZ_SMIL




PRBool NS_SMILEnabled();
#endif 


#undef CLIP_MASK

class nsSVGRenderState
{
public:
  enum RenderMode { NORMAL, CLIP, CLIP_MASK };

  


  nsSVGRenderState(nsRenderingContext *aContext);
  


  nsSVGRenderState(gfxContext *aContext);
  


  nsSVGRenderState(gfxASurface *aSurface);

  nsRenderingContext *GetRenderingContext(nsIFrame *aFrame);
  gfxContext *GetGfxContext() { return mGfxContext; }

  void SetRenderMode(RenderMode aMode) { mRenderMode = aMode; }
  RenderMode GetRenderMode() { return mRenderMode; }

  void SetPaintingToWindow(PRBool aPaintingToWindow) {
    mPaintingToWindow = aPaintingToWindow;
  }
  PRBool IsPaintingToWindow() { return mPaintingToWindow; }

private:
  RenderMode                    mRenderMode;
  nsRefPtr<nsRenderingContext> mRenderingContext;
  nsRefPtr<gfxContext>          mGfxContext;
  PRPackedBool                  mPaintingToWindow;
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
  typedef mozilla::SVGAnimatedPreserveAspectRatio SVGAnimatedPreserveAspectRatio;
  typedef mozilla::SVGPreserveAspectRatio SVGPreserveAspectRatio;

  


  static mozilla::dom::Element *GetParentElement(nsIContent *aContent);

  


  static nsSVGSVGElement *GetOuterSVGElement(nsSVGElement *aSVGElement);

  






  static float GetFontSize(mozilla::dom::Element *aElement);
  static float GetFontSize(nsIFrame *aFrame);
  static float GetFontSize(nsStyleContext *aStyleContext);
  






  static float GetFontXHeight(mozilla::dom::Element *aElement);
  static float GetFontXHeight(nsIFrame *aFrame);
  static float GetFontXHeight(nsStyleContext *aStyleContext);

  


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

  static gfxMatrix GetCTM(nsSVGElement *aElement, PRBool aScreenCTM);

  



  static PRBool EstablishesViewport(nsIContent *aContent);

  static already_AddRefed<nsIDOMSVGElement>
  GetNearestViewportElement(nsIContent *aContent);

  




  static nsSVGDisplayContainerFrame* GetNearestSVGViewport(nsIFrame *aFrame);
  
  








  static nsRect FindFilterInvalidation(nsIFrame *aFrame, const nsRect& aRect);

  


  static void InvalidateCoveredRegion(nsIFrame *aFrame);

  


  static void UpdateGraphic(nsISVGChildFrame *aSVGFrame);

  


  static void NotifyAncestorsOfFilterRegionChange(nsIFrame *aFrame);

  
  enum ctxDirection { X, Y, XY };

  


  static double ComputeNormalizedHypotenuse(double aWidth, double aHeight);

  



  static float ObjectSpace(const gfxRect &aRect, const nsSVGLength2 *aLength);

  



  static float UserSpace(nsSVGElement *aSVGElement, const nsSVGLength2 *aLength);

  



  static float UserSpace(nsIFrame *aFrame, const nsSVGLength2 *aLength);

  
  static float
  AngleBisect(float a1, float a2);

  
  static nsSVGOuterSVGFrame *
  GetOuterSVGFrame(nsIFrame *aFrame);

  




  static nsIFrame*
  GetOuterSVGFrameAndCoveredRegion(nsIFrame* aFrame, nsRect* aRect);

  

  static gfxMatrix
  GetViewBoxTransform(nsSVGElement* aElement,
                      float aViewportWidth, float aViewportHeight,
                      float aViewboxX, float aViewboxY,
                      float aViewboxWidth, float aViewboxHeight,
                      const SVGAnimatedPreserveAspectRatio &aPreserveAspectRatio);

  static gfxMatrix
  GetViewBoxTransform(nsSVGElement* aElement,
                      float aViewportWidth, float aViewportHeight,
                      float aViewboxX, float aViewboxY,
                      float aViewboxWidth, float aViewboxHeight,
                      const SVGPreserveAspectRatio &aPreserveAspectRatio);

  

  static void
  PaintFrameWithEffects(nsSVGRenderState *aContext,
                        const nsIntRect *aDirtyRect,
                        nsIFrame *aFrame);

  

  static PRBool
  HitTestClip(nsIFrame *aFrame, const nsPoint &aPoint);
  
  

  static nsIFrame *
  HitTestChildren(nsIFrame *aFrame, const nsPoint &aPoint);

  




  static gfxMatrix GetCanvasTM(nsIFrame* aFrame);

  


  static void
  NotifyChildrenOfSVGChange(nsIFrame *aFrame, PRUint32 aFlags);

  


  static nsRect
  GetCoveredRegion(const nsFrameList &aFrames);

  


  static nsRect
  ToAppPixelRect(nsPresContext *aPresContext,
                 double xmin, double ymin, double xmax, double ymax);
  static nsRect
  ToAppPixelRect(nsPresContext *aPresContext, const gfxRect& rect);

  












  static gfxIntSize ConvertToSurfaceSize(const gfxSize& aSize,
                                  PRBool *aResultOverflows)
  {
    gfxIntSize surfaceSize(ClampToInt(aSize.width), ClampToInt(aSize.height));

    *aResultOverflows = surfaceSize.width != NS_round(aSize.width) ||
      surfaceSize.height != NS_round(aSize.height);

    if (!gfxASurface::CheckSurfaceSize(surfaceSize)) {
      surfaceSize.width = NS_MIN(NS_SVG_OFFSCREEN_MAX_DIMENSION,
                                 surfaceSize.width);
      surfaceSize.height = NS_MIN(NS_SVG_OFFSCREEN_MAX_DIMENSION,
                                  surfaceSize.height);
      *aResultOverflows = PR_TRUE;
    }

    return surfaceSize;
  }

  


  static gfxMatrix
  ConvertSVGMatrixToThebes(nsIDOMSVGMatrix *aMatrix);

  


  static PRBool
  HitTestRect(const gfxMatrix &aMatrix,
              float aRX, float aRY, float aRWidth, float aRHeight,
              float aX, float aY);


  






  static gfxRect
  GetClipRectForFrame(nsIFrame *aFrame,
                      float aX, float aY, float aWidth, float aHeight);

  static void CompositeSurfaceMatrix(gfxContext *aContext,
                                     gfxASurface *aSurface,
                                     const gfxMatrix &aCTM, float aOpacity);

  static void CompositePatternMatrix(gfxContext *aContext,
                                     gfxPattern *aPattern,
                                     const gfxMatrix &aCTM, float aWidth, float aHeight, float aOpacity);

  static void SetClipRect(gfxContext *aContext,
                          const gfxMatrix &aCTM,
                          const gfxRect &aRect);

  


  static void ClipToGfxRect(nsIntRect* aRect, const gfxRect& aGfxRect);

  




  static PRBool
  CanOptimizeOpacity(nsIFrame *aFrame);

  
  static float
  MaxExpansion(const gfxMatrix &aMatrix);

  







  static gfxMatrix
  AdjustMatrixForUnits(const gfxMatrix &aMatrix,
                       nsSVGEnum *aUnits,
                       nsIFrame *aFrame);

  



  static gfxRect GetBBox(nsIFrame *aFrame);
  








  static gfxRect
  GetRelativeRect(PRUint16 aUnits, const nsSVGLength2 *aXYWH,
                  const gfxRect &aBBox, nsIFrame *aFrame);

  



  static nsIFrame* GetFirstNonAAncestorFrame(nsIFrame* aStartFrame);

#ifdef DEBUG
  static void
  WritePPM(const char *fname, gfxImageSurface *aSurface);
#endif

  












  static gfxRect PathExtentsToMaxStrokeExtents(const gfxRect& aPathExtents,
                                               nsSVGGeometryFrame* aFrame);

  



  static PRInt32 ClampToInt(double aVal)
  {
    return NS_lround(NS_MAX(double(PR_INT32_MIN),
                            NS_MIN(double(PR_INT32_MAX), aVal)));
  }

  







  static PRBool RootSVGElementHasViewbox(const nsIContent *aRootSVGElem);
};

#endif
