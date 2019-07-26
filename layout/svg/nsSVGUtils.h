




#ifndef NS_SVGUTILS_H
#define NS_SVGUTILS_H


#define _USE_MATH_DEFINES
#include <math.h>

#include "DrawMode.h"
#include "gfx2DGlue.h"
#include "gfxMatrix.h"
#include "gfxPoint.h"
#include "gfxRect.h"
#include "mozilla/gfx/Rect.h"
#include "nsAlgorithm.h"
#include "nsChangeHint.h"
#include "nsColor.h"
#include "nsCOMPtr.h"
#include "nsID.h"
#include "nsISupportsBase.h"
#include "nsMathUtils.h"
#include "nsStyleStruct.h"
#include "mozilla/Constants.h"
#include <algorithm>

class gfxContext;
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
class nsSVGLength2;
class nsSVGOuterSVGFrame;
class nsSVGPathGeometryFrame;
class nsTextFrame;
class gfxTextContextPaint;

struct nsStyleSVG;
struct nsStyleSVGPaint;
struct nsRect;
struct nsIntRect;
struct nsPoint;

namespace mozilla {
class SVGAnimatedPreserveAspectRatio;
class SVGPreserveAspectRatio;
namespace dom {
class Element;
} 
namespace gfx {
class SourceSurface;
}
} 





#define NS_SVG_OFFSCREEN_MAX_DIMENSION 4096

#define SVG_HIT_TEST_FILL        0x01
#define SVG_HIT_TEST_STROKE      0x02
#define SVG_HIT_TEST_CHECK_MRECT 0x04


bool NS_SVGDisplayListHitTestingEnabled();
bool NS_SVGDisplayListPaintingEnabled();





class SVGBBox {
  typedef mozilla::gfx::Rect Rect;

public:
  SVGBBox() 
    : mIsEmpty(true) {}

  SVGBBox(const Rect& aRect)
    : mBBox(aRect), mIsEmpty(false) {}

  SVGBBox(const gfxRect& aRect)
    : mBBox(ToRect(aRect)), mIsEmpty(false) {}

  gfxRect ToThebesRect() const {
    return ThebesRect(mBBox);
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
  Rect mBBox;
  bool mIsEmpty;
};


#undef CLIP_MASK

class MOZ_STACK_CLASS SVGAutoRenderState
{
public:
  enum RenderMode {
    


    NORMAL, 
    




    CLIP, 
    





    CLIP_MASK 
  };

  SVGAutoRenderState(nsRenderingContext *aContext, RenderMode aMode
                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM);
  ~SVGAutoRenderState();

  void SetPaintingToWindow(bool aPaintingToWindow);

  static RenderMode GetRenderMode(nsRenderingContext *aContext);
  static bool IsPaintingToWindow(nsRenderingContext *aContext);

private:
  nsRenderingContext *mContext;
  void *mOriginalRenderState;
  RenderMode mMode;
  bool mPaintingToWindow;
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};


#define NS_ISVGFILTERREFERENCE_IID \
{ 0x9744ee20, 0x1bcf, 0x4c62, \
 { 0x86, 0x7d, 0xd3, 0x7a, 0x91, 0x60, 0x3e, 0xef } }

class nsISVGFilterReference : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISVGFILTERREFERENCE_IID)
  virtual void Invalidate() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISVGFilterReference, NS_ISVGFILTERREFERENCE_IID)






class nsSVGUtils
{
public:
  typedef mozilla::dom::Element Element;

  static void Init();

  




  static nsSVGDisplayContainerFrame* GetNearestSVGViewport(nsIFrame *aFrame);

  




  static nsRect GetPostFilterVisualOverflowRect(nsIFrame *aFrame,
                                                const nsRect &aUnfilteredRect);

  



























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
                        nsIFrame *aFrame,
                        nsIFrame* aTransformRoot = nullptr);

  

  static bool
  HitTestClip(nsIFrame *aFrame, const nsPoint &aPoint);
  
  

  static nsIFrame *
  HitTestChildren(nsIFrame *aFrame, const nsPoint &aPoint);

  




  static gfxMatrix GetCanvasTM(nsIFrame* aFrame, uint32_t aFor,
                               nsIFrame* aTransformRoot = nullptr);

  








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
  HitTestRect(const mozilla::gfx::Matrix &aMatrix,
              float aRX, float aRY, float aRWidth, float aRHeight,
              float aX, float aY);


  






  static gfxRect
  GetClipRectForFrame(nsIFrame *aFrame,
                      float aX, float aY, float aWidth, float aHeight);

  static void SetClipRect(gfxContext *aContext,
                          const gfxMatrix &aCTM,
                          const gfxRect &aRect);

  




  static bool
  CanOptimizeOpacity(nsIFrame *aFrame);

  







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
  static bool AnyOuterSVGIsCallingReflowSVG(nsIFrame *aFrame);

  



  static gfxMatrix GetStrokeTransform(nsIFrame *aFrame);

  












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

  


  static bool SetupContextPaint(gfxContext *aContext,
                                gfxTextContextPaint *aContextPaint,
                                const nsStyleSVGPaint& aPaint,
                                float aOpacity);

  



  static bool SetupCairoFillPaint(nsIFrame* aFrame, gfxContext* aContext,
                                  gfxTextContextPaint *aContextPaint = nullptr);

  



  static bool SetupCairoStrokePaint(nsIFrame* aFrame, gfxContext* aContext,
                                    gfxTextContextPaint *aContextPaint = nullptr);

  static float GetOpacity(nsStyleSVGOpacitySource aOpacityType,
                          const float& aOpacity,
                          gfxTextContextPaint *aOuterContextPaint);

  


  static bool HasStroke(nsIFrame* aFrame,
                        gfxTextContextPaint *aContextPaint = nullptr);

  static float GetStrokeWidth(nsIFrame* aFrame,
                              gfxTextContextPaint *aContextPaint = nullptr);

  


  static void SetupCairoStrokeBBoxGeometry(nsIFrame* aFrame,
                                           gfxContext *aContext,
                                           gfxTextContextPaint *aContextPaint = nullptr);

  



  static void SetupCairoStrokeGeometry(nsIFrame* aFrame, gfxContext *aContext,
                                       gfxTextContextPaint *aContextPaint = nullptr);

  



  static bool SetupCairoStroke(nsIFrame* aFrame, gfxContext *aContext,
                               gfxTextContextPaint *aContextPaint = nullptr);

  





  static uint16_t GetGeometryHitTestFlags(nsIFrame* aFrame);

  






  static bool PaintSVGGlyph(Element* aElement, gfxContext* aContext,
                            DrawMode aDrawMode,
                            gfxTextContextPaint* aContextPaint);
  







  static bool GetSVGGlyphExtents(Element* aElement,
                                 const gfxMatrix& aSVGToAppSpace,
                                 gfxRect* aResult);

  




  static nsRect
  ToCanvasBounds(const gfxRect &aUserspaceRect,
                 const gfxMatrix &aToCanvas,
                 const nsPresContext *presContext);
};

#endif
