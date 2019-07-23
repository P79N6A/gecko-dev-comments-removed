





































#ifndef __NS_SVGVIEWBOX_H__
#define __NS_SVGVIEWBOX_H__

#include "nsIDOMSVGRect.h"
#include "nsIDOMSVGAnimatedRect.h"
#include "nsSVGElement.h"
#include "nsDOMError.h"

struct nsSVGViewBoxRect
{
  float x, y;
  float width, height;

  nsSVGViewBoxRect() : x(0), y(0), width(0), height(0) {}
  nsSVGViewBoxRect(float aX, float aY, float aWidth, float aHeight) :
    x(aX), y(aY), width(aWidth), height(aHeight) {}
  nsSVGViewBoxRect(const nsSVGViewBoxRect& rhs) :
    x(rhs.x), y(rhs.y), width(rhs.width), height(rhs.height) {}
};

class nsSVGViewBox
{

public:

  void Init();

  
  PRBool IsValid() const
    { return (mHasBaseVal || mAnimVal); }

  const nsSVGViewBoxRect& GetBaseValue() const
    { return mBaseVal; }
  void SetBaseValue(float aX, float aY, float aWidth, float aHeight,
                    nsSVGElement *aSVGElement, PRBool aDoSetAttr);

  const nsSVGViewBoxRect& GetAnimValue(nsSVGElement *aSVGElement) const;
  void SetAnimValue(float aX, float aY, float aWidth, float aHeight,
                    nsSVGElement *aSVGElement);

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement,
                              PRBool aDoSetAttr);
  void GetBaseValueString(nsAString& aValue) const;

  nsresult ToDOMAnimatedRect(nsIDOMSVGAnimatedRect **aResult,
                             nsSVGElement *aSVGElement);
#ifdef MOZ_SMIL
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);
#endif 
  
private:

  nsSVGViewBoxRect mBaseVal;
  nsAutoPtr<nsSVGViewBoxRect> mAnimVal;
  PRPackedBool mHasBaseVal;

  struct DOMBaseVal : public nsIDOMSVGRect
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMBaseVal)

    DOMBaseVal(nsSVGViewBox *aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    nsSVGViewBox* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetX(float *aX)
      { *aX = mVal->GetBaseValue().x; return NS_OK; }
    NS_IMETHOD GetY(float *aY)
      { *aY = mVal->GetBaseValue().y; return NS_OK; }
    NS_IMETHOD GetWidth(float *aWidth)
      { *aWidth = mVal->GetBaseValue().width; return NS_OK; }
    NS_IMETHOD GetHeight(float *aHeight)
      { *aHeight = mVal->GetBaseValue().height; return NS_OK; }

    NS_IMETHOD SetX(float aX);
    NS_IMETHOD SetY(float aY);
    NS_IMETHOD SetWidth(float aWidth);
    NS_IMETHOD SetHeight(float aHeight);
  };

  struct DOMAnimVal : public nsIDOMSVGRect
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimVal)

    DOMAnimVal(nsSVGViewBox *aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    nsSVGViewBox* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetX(float *aX)
      { *aX = mVal->GetAnimValue(mSVGElement).x; return NS_OK; }
    NS_IMETHOD GetY(float *aY)
      { *aY = mVal->GetAnimValue(mSVGElement).y; return NS_OK; }
    NS_IMETHOD GetWidth(float *aWidth)
      { *aWidth = mVal->GetAnimValue(mSVGElement).width; return NS_OK; }
    NS_IMETHOD GetHeight(float *aHeight)
      { *aHeight = mVal->GetAnimValue(mSVGElement).height; return NS_OK; }

    NS_IMETHOD SetX(float aX)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
    NS_IMETHOD SetY(float aY)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
    NS_IMETHOD SetWidth(float aWidth)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
    NS_IMETHOD SetHeight(float aHeight)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
  };

  struct DOMAnimatedRect : public nsIDOMSVGAnimatedRect
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedRect)

    DOMAnimatedRect(nsSVGViewBox *aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    nsSVGViewBox* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetBaseVal(nsIDOMSVGRect **aResult);
    NS_IMETHOD GetAnimVal(nsIDOMSVGRect **aResult);
  };

#ifdef MOZ_SMIL
  struct SMILViewBox : public nsISMILAttr
  {
  public:
    SMILViewBox(nsSVGViewBox* aVal, nsSVGElement* aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    nsSVGViewBox* mVal;
    nsSVGElement* mSVGElement;

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const nsISMILAnimationElement* aSrcElement,
                                     nsSMILValue& aValue) const;
    virtual nsSMILValue GetBaseValue() const;
    virtual void ClearAnimValue();
    virtual nsresult SetAnimValue(const nsSMILValue& aValue);
  };
#endif 
};

#endif 
