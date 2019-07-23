



































#ifndef __NS_SVGPAINTSERVERFRAME_H__
#define __NS_SVGPAINTSERVERFRAME_H__

#include "nsSVGContainerFrame.h"
#include "nsSVGValue.h"

class gfxContext;
class nsSVGGeometryFrame;

typedef nsSVGContainerFrame nsSVGPaintServerFrameBase;

class nsSVGPaintServerFrame : public nsSVGPaintServerFrameBase,
                              public nsSVGValue
{
public:
  nsSVGPaintServerFrame(nsStyleContext* aContext) :
      nsSVGPaintServerFrameBase(aContext) {}

  



  virtual PRBool SetupPaintServer(gfxContext *aContext,
                                  nsSVGGeometryFrame *aSource,
                                  float aOpacity,
                                  void **aClosure) = 0;
  
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

  
  NS_IMETHOD SetValueString(const nsAString &aValue) { return NS_OK; }
  NS_IMETHOD GetValueString(nsAString& aValue) { return NS_ERROR_NOT_IMPLEMENTED; }
};

#endif 
