





































#ifndef __NS_ISVGCHILDFRAME_H__
#define __NS_ISVGCHILDFRAME_H__


#include "nsQueryFrame.h"
#include "nsCOMPtr.h"
#include "nsRect.h"
#include "gfxRect.h"
#include "gfxMatrix.h"

class gfxContext;
class nsPresContext;
class nsSVGRenderState;

class nsISVGChildFrame : public nsQueryFrame
{
public:
  NS_DECLARE_FRAME_ACCESSOR(nsISVGChildFrame)

  
  
  NS_IMETHOD PaintSVG(nsSVGRenderState* aContext,
                      const nsIntRect *aDirtyRect)=0;

  
  
  
  
  NS_IMETHOD_(nsIFrame*) GetFrameForPoint(const nsPoint &aPoint)=0;

  
  NS_IMETHOD_(nsRect) GetCoveredRegion()=0;
  NS_IMETHOD UpdateCoveredRegion()=0;

  
  
  
  
  NS_IMETHOD InitialUpdate()=0;

  
  
  
  
  
  
  
  enum SVGChangedFlags {
    SUPPRESS_INVALIDATION = 0x01,
    TRANSFORM_CHANGED     = 0x02,
    COORD_CONTEXT_CHANGED = 0x04
  };
  virtual void NotifySVGChanged(PRUint32 aFlags)=0;
  NS_IMETHOD NotifyRedrawSuspended()=0;
  NS_IMETHOD NotifyRedrawUnsuspended()=0;

  
  
  NS_IMETHOD SetMatrixPropagation(PRBool aPropagate)=0;
  virtual PRBool GetMatrixPropagation()=0;

  

















  virtual gfxRect GetBBoxContribution(const gfxMatrix &aToBBoxUserspace) = 0;

  
  NS_IMETHOD_(PRBool) IsDisplayContainer()=0;

  
  NS_IMETHOD_(PRBool) HasValidCoveredRect()=0;
};

#endif 

