





































#ifndef __NS_ISVGCHILDFRAME_H__
#define __NS_ISVGCHILDFRAME_H__


#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsRect.h"

class gfxContext;
class nsPresContext;
class nsIDOMSVGRect;
class nsIDOMSVGMatrix;
class nsSVGRenderState;

#define NS_ISVGCHILDFRAME_IID \
{ 0xfc3ee9b2, 0xaf40, 0x416d, \
  { 0xa8, 0x51, 0xb4, 0x68, 0xa4, 0xe4, 0x8b, 0xcd } }

class nsISVGChildFrame : public nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISVGCHILDFRAME_IID)

  
  
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

  
  NS_IMETHOD GetBBox(nsIDOMSVGRect **_retval)=0; 

  
  NS_IMETHOD_(PRBool) IsDisplayContainer()=0;

  
  NS_IMETHOD_(PRBool) HasValidCoveredRect()=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISVGChildFrame, NS_ISVGCHILDFRAME_IID)

#endif 

