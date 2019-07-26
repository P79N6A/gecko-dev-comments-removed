




#ifndef NS_SVGUTILS_H
#define NS_SVGUTILS_H


#define _USE_MATH_DEFINES
#include <math.h>

#include "gfxFont.h"
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
#include "nsStyleStruct.h"
#include "mozilla/Constants.h"
#include <algorithm>

class gfxASurface;
class gfxContext;
class gfxImageSurface;
class gfxPattern;
class nsFrameList;
class nsIContent;
class nsIDocument;
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
class nsTextFrame;
class gfxTextObjectPaint;

struct nsStyleSVG;
struct nsStyleSVGPaint;

namespace mozilla {
class SVGAnimatedPreserveAspectRatio;
class SVGPreserveAspectRatio;
namespace dom {
class Element;
} 
} 


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

#define SVG_HIT_TEST_FILL        0x01
#define SVG_HIT_TEST_STROKE      0x02
#define SVG_HIT_TEST_CHECK_MRECT 0x04


bool NS_SVGDisplayListHitTestingEnabled();
bool NS_SVGDisplayListPaintingEnabled();
bool NS_SVGTextCSSFramesEnabled();





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
  typedef mozilla::dom::Element Element;

  static void Init();

  


  static void UnPremultiplyImageDataAlpha(uint8_t *data, 
                                          int32_t stride, 
                                          const nsIntRect &rect);
  


  static void PremultiplyImageDataAlpha(uint8_t *data, 
                                        int32_t stride, 
                                        const nsIntRect &rect);
  


  static void ConvertImageDataToLinearRGB(uint8_t *data, 
                                          int32_t stride, 
                                          const nsIntRect &rect);
  


  static void ConvertImageDataFromLinearRGB(uint8_t *data, 
                                            int32_t stride, 
                                            const nsIntRect &rect);

  


  static void ComputesRGBLuminanceMask(uint8_t *aData,
                                       int32_t aStride,
                                       const nsIntRect &aRect,
                                       float aOpacity);

  



  static void ComputeLinearRGBLuminanceMask(uint8_t *aData,
                                            int32_t aStride,
                                            const nsIntRect &aRect,
                                            float aOpacity);
  


  static void ComputeAlphaMask(uint8_t *aData,
                               int32_t aStride,
                               const nsIntRect &aRect,
                               float aOpacity);

  




  static float CoordToFloat(nsPresContext *aPresContext,
                            nsSVGElement *aContent,
                            const nsStyleCoord &aCoord);

  




  static nsSVGDisplayContainerFrame* GetNearestSVGViewport(nsIFrame *aFrame);

  




  static nsRect GetPostFilterVisualOverflowRect(nsIFrame *aFrame,
                                                const nsRect &aUnfilteredRect);

  










  static void InvalidateBounds(nsIFrame *aFrame, bool aDuringUpdate = false,
                               const nsRect *aBoundsSubArea = nullptr,
                               uint32_t aFlags = 0);

  



























  static void ScheduleReflowSVG(nsIFrame *aFrame);

  



  static bool NeedsReflowSVG(nsIFrame *aFrame);

  


  static void NotifyAncestorsOfFilterRegionChange(nsIFrame *aFrame);

  



  static float ObjectSpace(const gfxRect &aRect, const nsSVGLength2 *aLength);

  



  static float UserSpace(nsSVGElement *aSVGElement, const nsSVGLength2 *aLength);

  



  static float UserSpace(nsIFrame *aFrame, const nsSVGLength2 *aLength);

  
  static nsSVGOuterSVGFrame *
  GetOuterSVGFrame(nsIFrame *aFrame);

  




  static nsIFrame*
  GetOuterSVGFrameAndCoveredRegion(nsIFrame* aFrame, nsRect* aRect);

  

  static void
  PaintFrameWithEffects(nsRenderingContext *aContext,
                        const nsIntRect *aDirtyRect,
                        nsIFrame *aFrame);

  

  static bool
  HitTestClip(nsIFrame *aFrame, const nsPoint &aPoint);
  
  

  static nsIFrame *
  HitTestChildren(nsIFrame *aFrame, const nsPoint &aPoint);

  




  static gfxMatrix GetCanvasTM(nsIFrame* aFrame, uint32_t aFor);

  








  static gfxMatrix GetUserToCanvasTM(nsIFrame* aFrame, uint32_t aFor);

  









  static void
  NotifyChildrenOfSVGChange(nsIFrame *aFrame, uint32_t aFlags);

  


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
    eBBoxIncludeFill           = 1 << 0,
    eBBoxIncludeFillGeometry   = 1 << 1,
    eBBoxIncludeStroke         = 1 << 2,
    eBBoxIncludeStrokeGeometry = 1 << 3,
    eBBoxIncludeMarkers        = 1 << 4
  };
  



  static gfxRect GetBBox(nsIFrame *aFrame,
                         uint32_t aFlags = eBBoxIncludeFillGeometry);

  










  static gfxRect
  GetRelativeRect(uint16_t aUnits, const nsSVGLength2 *aXYWH,
                  const gfxRect &aBBox, nsIFrame *aFrame);

  



  static nsIFrame* GetFirstNonAAncestorFrame(nsIFrame* aStartFrame);

  static bool OuterSVGIsCallingReflowSVG(nsIFrame *aFrame);

  



  static gfxMatrix GetStrokeTransform(nsIFrame *aFrame);

  












  static gfxRect PathExtentsToMaxStrokeExtents(const gfxRect& aPathExtents,
                                               nsSVGGeometryFrame* aFrame,
                                               const gfxMatrix& aMatrix);
  static gfxRect PathExtentsToMaxStrokeExtents(const gfxRect& aPathExtents,
                                               nsTextFrame* aFrame,
                                               const gfxMatrix& aMatrix);
  static gfxRect PathExtentsToMaxStrokeExtents(const gfxRect& aPathExtents,
                                               nsSVGPathGeometryFrame* aFrame,
                                               const gfxMatrix& aMatrix);

  



  static int32_t ClampToInt(double aVal)
  {
    return NS_lround(std::max(double(INT32_MIN),
                            std::min(double(INT32_MAX), aVal)));
  }

  static nscolor GetFallbackOrPaintColor(gfxContext *aContext,
                                         nsStyleContext *aStyleContext,
                                         nsStyleSVGPaint nsStyleSVG::*aFillOrStroke);

  


  static bool SetupObjectPaint(gfxContext *aContext,
                               gfxTextObjectPaint *aObjectPaint,
                               const nsStyleSVGPaint& aPaint,
                               float aOpacity);

  



  static bool SetupCairoFillPaint(nsIFrame* aFrame, gfxContext* aContext,
                                  gfxTextObjectPaint *aObjectPaint = nullptr);

  



  static bool SetupCairoStrokePaint(nsIFrame* aFrame, gfxContext* aContext,
                                    gfxTextObjectPaint *aObjectPaint = nullptr);

  static float GetOpacity(nsStyleSVGOpacitySource aOpacityType,
                          const float& aOpacity,
                          gfxTextObjectPaint *aOuterObjectPaint);

  


  static bool HasStroke(nsIFrame* aFrame,
                        gfxTextObjectPaint *aObjectPaint = nullptr);

  static float GetStrokeWidth(nsIFrame* aFrame,
                              gfxTextObjectPaint *aObjectPaint = nullptr);

  


  static void SetupCairoStrokeGeometry(nsIFrame* aFrame, gfxContext *aContext,
                                       gfxTextObjectPaint *aObjectPaint = nullptr);

  


  static void SetupCairoStrokeHitGeometry(nsIFrame* aFrame, gfxContext *aContext,
                                          gfxTextObjectPaint *aObjectPaint = nullptr);

  



  static bool SetupCairoStroke(nsIFrame* aFrame, gfxContext *aContext,
                               gfxTextObjectPaint *aObjectPaint = nullptr);

  





  static uint16_t GetGeometryHitTestFlags(nsIFrame* aFrame);

  






  static bool PaintSVGGlyph(Element* aElement, gfxContext* aContext,
                            gfxFont::DrawMode aDrawMode,
                            gfxTextObjectPaint* aObjectPaint);
  







  static bool GetSVGGlyphExtents(Element* aElement,
                                 const gfxMatrix& aSVGToAppSpace,
                                 gfxRect* aResult);

  




  static nsRect
  ToCanvasBounds(const gfxRect &aUserspaceRect,
                 const gfxMatrix &aToCanvas,
                 const nsPresContext *presContext);
};

#endif
