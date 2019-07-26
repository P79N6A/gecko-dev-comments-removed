




#ifndef __NS_SVGVIEWBOX_H__
#define __NS_SVGVIEWBOX_H__

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsError.h"
#include "nsIDOMSVGAnimatedRect.h"
#include "nsIDOMSVGRect.h"
#include "nsISMILAttr.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"

class nsISMILAnimationElement;
class nsSMILValue;

struct nsSVGViewBoxRect
{
  float x, y;
  float width, height;
  bool none;

  nsSVGViewBoxRect() : none(true) {}
  nsSVGViewBoxRect(float aX, float aY, float aWidth, float aHeight) :
    x(aX), y(aY), width(aWidth), height(aHeight), none(false) {}
  nsSVGViewBoxRect(const nsSVGViewBoxRect& rhs) :
    x(rhs.x), y(rhs.y), width(rhs.width), height(rhs.height), none(rhs.none) {}
  bool operator==(const nsSVGViewBoxRect& aOther) const;
};

class nsSVGViewBox
{

public:

  void Init();

  









  bool HasRect() const
    { return (mAnimVal && !mAnimVal->none) ||
             (!mAnimVal && mHasBaseVal && !mBaseVal.none); }

  



  bool IsExplicitlySet() const
    { return mAnimVal || mHasBaseVal; }

  const nsSVGViewBoxRect& GetBaseValue() const
    { return mBaseVal; }
  void SetBaseValue(const nsSVGViewBoxRect& aRect,
                    nsSVGElement *aSVGElement);
  const nsSVGViewBoxRect& GetAnimValue() const
    { return mAnimVal ? *mAnimVal : mBaseVal; }
  void SetAnimValue(const nsSVGViewBoxRect& aRect,
                    nsSVGElement *aSVGElement);

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement,
                              bool aDoSetAttr);
  void GetBaseValueString(nsAString& aValue) const;

  nsresult ToDOMAnimatedRect(nsIDOMSVGAnimatedRect **aResult,
                             nsSVGElement *aSVGElement);
  nsresult ToDOMBaseVal(nsIDOMSVGRect **aResult, nsSVGElement* aSVGElement);
  nsresult ToDOMAnimVal(nsIDOMSVGRect **aResult, nsSVGElement* aSVGElement);
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);

private:

  nsSVGViewBoxRect mBaseVal;
  nsAutoPtr<nsSVGViewBoxRect> mAnimVal;
  bool mHasBaseVal;

public:
  struct DOMBaseVal MOZ_FINAL : public nsIDOMSVGRect
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMBaseVal)

    DOMBaseVal(nsSVGViewBox *aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    virtual ~DOMBaseVal();

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

  struct DOMAnimVal MOZ_FINAL : public nsIDOMSVGRect
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimVal)

    DOMAnimVal(nsSVGViewBox *aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    virtual ~DOMAnimVal();

    nsSVGViewBox* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    
    
    NS_IMETHOD GetX(float *aX)
    {
      mSVGElement->FlushAnimations();
      *aX = mVal->GetAnimValue().x;
      return NS_OK;
    }
    NS_IMETHOD GetY(float *aY)
    {
      mSVGElement->FlushAnimations();
      *aY = mVal->GetAnimValue().y;
      return NS_OK;
    }
    NS_IMETHOD GetWidth(float *aWidth)
    {
      mSVGElement->FlushAnimations();
      *aWidth = mVal->GetAnimValue().width;
      return NS_OK;
    }
    NS_IMETHOD GetHeight(float *aHeight)
    {
      mSVGElement->FlushAnimations();
      *aHeight = mVal->GetAnimValue().height;
      return NS_OK;
    }

    NS_IMETHOD SetX(float aX)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
    NS_IMETHOD SetY(float aY)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
    NS_IMETHOD SetWidth(float aWidth)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
    NS_IMETHOD SetHeight(float aHeight)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
  };

  struct DOMAnimatedRect MOZ_FINAL : public nsIDOMSVGAnimatedRect
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedRect)

    DOMAnimatedRect(nsSVGViewBox *aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    virtual ~DOMAnimatedRect();

    nsSVGViewBox* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetBaseVal(nsIDOMSVGRect **aBaseVal)
      { return mVal->ToDOMBaseVal(aBaseVal, mSVGElement); }

    NS_IMETHOD GetAnimVal(nsIDOMSVGRect **aAnimVal)
      { return mVal->ToDOMAnimVal(aAnimVal, mSVGElement); }
  };

  struct SMILViewBox : public nsISMILAttr
  {
  public:
    SMILViewBox(nsSVGViewBox* aVal, nsSVGElement* aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    nsSVGViewBox* mVal;
    nsSVGElement* mSVGElement;

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const nsISMILAnimationElement* aSrcElement,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const;
    virtual nsSMILValue GetBaseValue() const;
    virtual void ClearAnimValue();
    virtual nsresult SetAnimValue(const nsSMILValue& aValue);
  };
};

#endif 
