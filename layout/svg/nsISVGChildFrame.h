




#ifndef __NS_ISVGCHILDFRAME_H__
#define __NS_ISVGCHILDFRAME_H__

#include "gfxRect.h"
#include "nsQueryFrame.h"

class nsIFrame;
class nsRenderingContext;

struct nsPoint;
class SVGBBox;
struct nsRect;
struct nsIntRect;

namespace mozilla {
class SVGAnimatedLengthList;
class SVGAnimatedNumberList;
class SVGLengthList;
class SVGNumberList;
class SVGUserUnitList;

namespace gfx {
class Matrix;
}
}








class nsISVGChildFrame : public nsQueryFrame
{
public:
  typedef mozilla::SVGAnimatedNumberList SVGAnimatedNumberList;
  typedef mozilla::SVGNumberList SVGNumberList;
  typedef mozilla::SVGAnimatedLengthList SVGAnimatedLengthList;
  typedef mozilla::SVGLengthList SVGLengthList;
  typedef mozilla::SVGUserUnitList SVGUserUnitList;

  NS_DECL_QUERYFRAME_TARGET(nsISVGChildFrame)

  
  
  
  
  
  
  
  
  
  
  
  virtual nsresult PaintSVG(nsRenderingContext* aContext,
                            const nsIntRect *aDirtyRect,
                            nsIFrame* aTransformRoot = nullptr) = 0;

  





  virtual nsIFrame* GetFrameForPoint(const gfxPoint& aPoint) = 0;

  
  virtual nsRect GetCoveredRegion()=0;

  
  
  
  
  
  virtual void ReflowSVG()=0;

  





  enum RequestingCanvasTMFor {
    FOR_PAINTING = 1,
    FOR_HIT_TESTING,
    FOR_OUTERSVG_TM
  };

  
  
  
  
  
  
  
  
  
  
  enum SVGChangedFlags {
    TRANSFORM_CHANGED     = 0x01,
    COORD_CONTEXT_CHANGED = 0x02,
    FULL_ZOOM_CHANGED     = 0x04
  };
  








  virtual void NotifySVGChanged(uint32_t aFlags)=0;

  




















  virtual SVGBBox GetBBoxContribution(const mozilla::gfx::Matrix &aToBBoxUserspace,
                                      uint32_t aFlags) = 0;

  
  virtual bool IsDisplayContainer()=0;
};

#endif 

