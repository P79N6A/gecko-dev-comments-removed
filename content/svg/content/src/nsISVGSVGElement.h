






































#ifndef __NS_ISVGSVGELEMENT__
#define __NS_ISVGSVGELEMENT__

#include "nsISupports.h"
#include "nsIDOMSVGSVGElement.h"

class nsSVGSVGElement;
class nsIDOMSVGNumber;
class nsISVGEnum;




#define NS_ISVGSVGELEMENT_IID \
{ 0x65195064, 0x9a5f, 0x42df, { 0xa1, 0x1f, 0x4c, 0x05, 0xbc, 0xa4, 0xe7, 0x28 } }

class nsISVGSVGElement : public nsIDOMSVGSVGElement
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISVGSVGELEMENT_IID)

  NS_IMETHOD GetCurrentScaleNumber(nsIDOMSVGNumber **aResult)=0;
  NS_IMETHOD GetZoomAndPanEnum(nsISVGEnum **aResult)=0;

  




  NS_IMETHOD SetCurrentScaleTranslate(float s, float x, float y)=0;

  




  NS_IMETHOD SetCurrentTranslate(float x, float y)=0;

  



  NS_IMETHOD_(void) RecordCurrentScaleTranslate()=0;

  



  NS_IMETHOD_(float) GetPreviousScale()=0;
  NS_IMETHOD_(float) GetPreviousTranslate_x()=0;
  NS_IMETHOD_(float) GetPreviousTranslate_y()=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISVGSVGElement, NS_ISVGSVGELEMENT_IID)

#endif 
