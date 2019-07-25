




#ifndef __NS_ISVGCHILDFRAME_H__
#define __NS_ISVGCHILDFRAME_H__

#include "gfxRect.h"
#include "nsQueryFrame.h"
#include "nsRect.h"

class nsIFrame;
class nsRenderingContext;

struct gfxMatrix;
struct nsPoint;
class SVGBBox;

namespace mozilla {
class SVGAnimatedLengthList;
class SVGAnimatedNumberList;
class SVGLengthList;
class SVGNumberList;
class SVGUserUnitList;
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

  
  
  NS_IMETHOD PaintSVG(nsRenderingContext* aContext,
                      const nsIntRect *aDirtyRect)=0;

  
  
  
  
  NS_IMETHOD_(nsIFrame*) GetFrameForPoint(const nsPoint &aPoint)=0;

  
  NS_IMETHOD_(nsRect) GetCoveredRegion()=0;

  
  
  
  
  
  virtual void UpdateBounds()=0;

  
  
  
  
  
  
  
  
  
  
  enum SVGChangedFlags {
    DO_NOT_NOTIFY_RENDERING_OBSERVERS = 0x01,
    TRANSFORM_CHANGED     = 0x02,
    COORD_CONTEXT_CHANGED = 0x04,
    FULL_ZOOM_CHANGED     = 0x08
  };
  virtual void NotifySVGChanged(PRUint32 aFlags)=0;

  




















  virtual SVGBBox GetBBoxContribution(const gfxMatrix &aToBBoxUserspace,
                                      PRUint32 aFlags) = 0;

  
  NS_IMETHOD_(bool) IsDisplayContainer()=0;
};

#endif 

