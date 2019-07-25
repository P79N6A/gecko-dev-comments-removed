




#ifndef NS_SVGUTILS_H
#define NS_SVGUTILS_H


#define _USE_MATH_DEFINES
#include <math.h>

#include "gfxMatrix.h"
#include "gfxPoint.h"
#include "gfxRect.h"
#include "nsAlgorithm.h"
#include "nsChangeHint.h"
#include "nsColor.h"
#include "nsCOMPtr.h"
#include "nsID.h"
#include "nsISupportsBase.h"
#include "nsMathUtils.h"
#include "nsPoint.h"
#include "nsRect.h"

class gfxASurface;
class gfxContext;
class gfxImageSurface;
class gfxPattern;
class nsFrameList;
class nsIContent;
class nsIDocument;
class nsIDOMSVGElement;
class nsIFrame;
class nsPresContext;
class nsRenderingContext;
class nsStyleContext;
class nsStyleCoord;
class nsSVGDisplayContainerFrame;
class nsSVGElement;
class nsSVGEnum;
class nsSVGGeometryFrame;
class nsSVGLength2;
class nsSVGOuterSVGFrame;
class nsSVGPathGeometryFrame;
class nsSVGSVGElement;

struct nsStyleSVG;
struct nsStyleSVGPaint;

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

inline bool
IsSVGWhitespace(char aChar)
{
  return aChar == '\x20' || aChar == '\x9' ||
         aChar == '\xD'  || aChar == '\xA';
}

inline bool
IsSVGWhitespace(PRUnichar aChar)
{
  return aChar == PRUnichar('\x20') || aChar == PRUnichar('\x9') ||
         aChar == PRUnichar('\xD')  || aChar == PRUnichar('\xA');
}





bool NS_SMILEnabled();

bool NS_SVGDisplayListHitTestingEnabled();
bool NS_SVGDisplayListPaintingEnabled();





class SVGBBox {
public:
  SVGBBox() 
    : mIsEmpty(true) {}

  SVGBBox(const gfxRect& aRect) 
    : mBBox(aRect), mIsEmpty(false) {}

  SVGBBox& operator=(const gfxRect& aRect) {
    mBBox = aRect;
    mIsEmpty = false;
    return *this;
  }

  operator const gfxRect& () const {
    return mBBox;
  }

  bool IsEmpty() const {
    return mIsEmpty;
  }

  void UnionEdges(const SVGBBox& aSVGBBox) {
    if (aSVGBBox.mIsEmpty) {
      return;
    }
    mBBox = mIsEmpty ? aSVGBBox.mBBox : mBBox.UnionEdges(aSVGBBox.mBBox);
    mIsEmpty = false;
  }

private:
  gfxRect mBBox;
  bool    mIsEmpty;
};


#undef CLIP_MASK

class NS_STACK_CLASS SVGAutoRenderState
{
public:
  enum RenderMode {
    


    NORMAL, 
    




    CLIP, 
    





    CLIP_MASK 
  };

  SVGAutoRenderState(nsRenderingContext *aContext, RenderMode aMode);
  ~SVGAutoRenderState();

  void SetPaintingToWindow(bool aPaintingToWindow);

  static RenderMode GetRenderMode(nsRenderingContext *aContext);
  static bool IsPaintingToWindow(nsRenderingContext *aContext);

private:
  nsRenderingContext *mContext;
  void *mOriginalRenderState;
  RenderMode mMode;
  bool mPaintingToWindow;
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

  static void Init();

  


  static mozilla::dom::Element *GetParentElement(nsIContent *aContent);

  


  static nsSVGSVGElement *GetOuterSVGElement(nsSVGElement *aSVGElement);

  







  static void ActivateByHyperlink(nsIContent *aContent);

  






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

  static gfxMatrix GetCTM(nsSVGElement *aElement, bool aScreenCTM);

  



  static bool EstablishesViewport(nsIContent *aContent);

  static already_AddRefed<nsIDOMSVGElement>
  GetNearestViewportElement(nsIContent *aContent);

  




  static nsSVGDisplayContainerFrame* GetNearestSVGViewport(nsIFrame *aFrame);

  




  static nsRect GetPostFilterVisualOverflowRect(nsIFrame *aFrame,
                                                const nsRect &aUnfilteredRect);

  








  static nsRect FindFilterInvalidation(nsIFrame *aFrame, const nsRect& aRect);

  






  static void InvalidateBounds(nsIFrame *aFrame, bool aDuringUpdate = false);

  



























  static void ScheduleBoundsUpdate(nsIFrame *aFrame);

  




  static void InvalidateAndScheduleBoundsUpdate(nsIFrame *aFrame);

  



  static bool NeedsUpdatedBounds(nsIFrame *aFrame);

  


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
  GetViewBoxTransform(const nsSVGElement* aElement,
                      float aViewportWidth, float aViewportHeight,
                      float aViewboxX, float aViewboxY,
                      float aViewboxWidth, float aViewboxHeight,
                      const SVGAnimatedPreserveAspectRatio &aPreserveAspectRatio);

  static gfxMatrix
  GetViewBoxTransform(const nsSVGElement* aElement,
                      float aViewportWidth, float aViewportHeight,
                      float aViewboxX, float aViewboxY,
                      float aViewboxWidth, float aViewboxHeight,
                      const SVGPreserveAspectRatio &aPreserveAspectRatio);

  

  static void
  PaintFrameWithEffects(nsRenderingContext *aContext,
                        const nsIntRect *aDirtyRect,
                        nsIFrame *aFrame);

  

  static bool
  HitTestClip(nsIFrame *aFrame, const nsPoint &aPoint);
  
  

  static nsIFrame *
  HitTestChildren(nsIFrame *aFrame, const nsPoint &aPoint);

  




  static gfxMatrix GetCanvasTM(nsIFrame* aFrame);

  









  static void
  NotifyChildrenOfSVGChange(nsIFrame *aFrame, PRUint32 aFlags);

  


  static nsRect
  GetCoveredRegion(const nsFrameList &aFrames);

  
  
  
  
  static nsPoint
  TransformOuterSVGPointToChildFrame(nsPoint aPoint,
                                     const gfxMatrix& aFrameToCanvasTM,
                                     nsPresContext* aPresContext);

  static nsRect
  TransformFrameRectToOuterSVG(const nsRect& aRect,
                               const gfxMatrix& aMatrix,
                               nsPresContext* aPresContext);

  








  static gfxIntSize ConvertToSurfaceSize(const gfxSize& aSize,
                                         bool *aResultOverflows);

  


  static bool
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

  




  static bool
  CanOptimizeOpacity(nsIFrame *aFrame);

  
  static float
  MaxExpansion(const gfxMatrix &aMatrix);

  







  static gfxMatrix
  AdjustMatrixForUnits(const gfxMatrix &aMatrix,
                       nsSVGEnum *aUnits,
                       nsIFrame *aFrame);

  enum BBoxFlags {
    eBBoxIncludeFill          = 1 << 0,
    eBBoxIgnoreFillIfNone     = 1 << 1,
    eBBoxIncludeStroke        = 1 << 2,
    eBBoxIgnoreStrokeIfNone   = 1 << 3,
    eBBoxIncludeMarkers       = 1 << 4
  };
  



  static gfxRect GetBBox(nsIFrame *aFrame, PRUint32 aFlags = eBBoxIncludeFill);

  










  static gfxRect
  GetRelativeRect(PRUint16 aUnits, const nsSVGLength2 *aXYWH,
                  const gfxRect &aBBox, nsIFrame *aFrame);

  



  static nsIFrame* GetFirstNonAAncestorFrame(nsIFrame* aStartFrame);

#ifdef DEBUG
  static void
  WritePPM(const char *fname, gfxImageSurface *aSurface);

  static bool OuterSVGIsCallingUpdateBounds(nsIFrame *aFrame);
#endif

  



  static gfxMatrix GetStrokeTransform(nsIFrame *aFrame);

  












  static gfxRect PathExtentsToMaxStrokeExtents(const gfxRect& aPathExtents,
                                               nsSVGGeometryFrame* aFrame,
                                               const gfxMatrix& aMatrix);
  static gfxRect PathExtentsToMaxStrokeExtents(const gfxRect& aPathExtents,
                                               nsSVGPathGeometryFrame* aFrame,
                                               const gfxMatrix& aMatrix);

  



  static PRInt32 ClampToInt(double aVal)
  {
    return NS_lround(NS_MAX(double(PR_INT32_MIN),
                            NS_MIN(double(PR_INT32_MAX), aVal)));
  }

  static void GetFallbackOrPaintColor(gfxContext *aContext,
                                      nsStyleContext *aStyleContext,
                                      nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                                      float *aOpacity, nscolor *color);
};

#endif
