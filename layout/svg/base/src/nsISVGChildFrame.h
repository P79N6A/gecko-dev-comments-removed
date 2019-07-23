





































#ifndef __NS_ISVGCHILDFRAME_H__
#define __NS_ISVGCHILDFRAME_H__


#include "nsISupports.h"
#include "nsCOMPtr.h"

class gfxContext;
class nsPresContext;
class nsIDOMSVGRect;
class nsIDOMSVGMatrix;
class nsSVGRenderState;
struct nsRect;

#define NS_ISVGCHILDFRAME_IID \
{ 0x93560e72, 0x6818, 0x4218, \
 { 0xa1, 0xe9, 0xf3, 0xb9, 0x63, 0x6a, 0xff, 0xc2 } }

class nsISVGChildFrame : public nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISVGCHILDFRAME_IID)

  
  
  NS_IMETHOD PaintSVG(nsSVGRenderState* aContext, nsRect *aDirtyRect)=0;

  
  
  
  
  
  
  
  
  
  NS_IMETHOD GetFrameForPointSVG(float x, float y, nsIFrame** hit)=0;

  NS_IMETHOD_(nsRect) GetCoveredRegion()=0;
  NS_IMETHOD UpdateCoveredRegion()=0;
  NS_IMETHOD InitialUpdate()=0;
  NS_IMETHOD NotifyCanvasTMChanged(PRBool suppressInvalidation)=0;
  NS_IMETHOD NotifyRedrawSuspended()=0;
  NS_IMETHOD NotifyRedrawUnsuspended()=0;

  
  
  NS_IMETHOD SetMatrixPropagation(PRBool aPropagate)=0;

  
  
  
  NS_IMETHOD SetOverrideCTM(nsIDOMSVGMatrix *aCTM)=0;
  virtual already_AddRefed<nsIDOMSVGMatrix> GetOverrideCTM()=0;

  
  NS_IMETHOD GetBBox(nsIDOMSVGRect **_retval)=0; 

  
  NS_IMETHOD_(PRBool) IsDisplayContainer()=0;

  
  NS_IMETHOD_(PRBool) HasValidCoveredRect()=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISVGChildFrame, NS_ISVGCHILDFRAME_IID)

#endif 

