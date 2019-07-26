





#ifndef __NS_SVGGEOMETRYFRAME_H__
#define __NS_SVGGEOMETRYFRAME_H__

#include "mozilla/Attributes.h"
#include "gfxMatrix.h"
#include "gfxTypes.h"
#include "nsFrame.h"
#include "nsIFrame.h"
#include "nsQueryFrame.h"

class gfxContext;
class nsIContent;
class nsStyleContext;
class nsSVGPaintServerFrame;

struct nsStyleSVGPaint;

typedef nsFrame nsSVGGeometryFrameBase;







class nsSVGGeometryFrame : public nsSVGGeometryFrameBase
{
protected:
  NS_DECL_FRAMEARENA_HELPERS

  nsSVGGeometryFrame(nsStyleContext *aContext)
    : nsSVGGeometryFrameBase(aContext)
  {
    AddStateBits(NS_FRAME_SVG_LAYOUT);
  }

public:
  
  virtual void Init(nsIContent* aContent,
		    nsIFrame* aParent,
		    nsIFrame* aPrevInFlow) MOZ_OVERRIDE;

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return nsSVGGeometryFrameBase::IsFrameOfType(aFlags & ~(nsIFrame::eSVG | nsIFrame::eSVGGeometry));
  }

  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor,
                                nsIFrame* aTransformRoot = nullptr) = 0;
  uint16_t GetClipRule();

protected:
  





  virtual uint16_t GetHitTestFlags();
};

#endif 
