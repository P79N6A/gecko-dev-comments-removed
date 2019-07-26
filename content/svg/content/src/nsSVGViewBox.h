




#ifndef __NS_SVGVIEWBOX_H__
#define __NS_SVGVIEWBOX_H__

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsError.h"
#include "nsIDOMSVGAnimatedRect.h"
#include "mozilla/dom/SVGIRect.h"
#include "nsISMILAttr.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"

class nsSMILValue;

namespace mozilla {
namespace dom {
class SVGAnimationElement;
}
}

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
  struct DOMBaseVal MOZ_FINAL : public mozilla::dom::SVGIRect
  {
    using mozilla::dom::SVGIRect::SetX;
    using mozilla::dom::SVGIRect::SetY;
    using mozilla::dom::SVGIRect::SetWidth;
    using mozilla::dom::SVGIRect::SetHeight;

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMBaseVal)

    DOMBaseVal(nsSVGViewBox *aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    virtual ~DOMBaseVal();

    nsSVGViewBox* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    float X() const MOZ_OVERRIDE MOZ_FINAL
    {
      return mVal->GetBaseValue().x;
    }

    float Y() const MOZ_OVERRIDE MOZ_FINAL
    {
      return mVal->GetBaseValue().y;
    }

    float Width() const MOZ_OVERRIDE MOZ_FINAL
    {
      return mVal->GetBaseValue().width;
    }

    float Height() const MOZ_OVERRIDE MOZ_FINAL
    {
      return mVal->GetBaseValue().height;
    }

    void SetX(float aX, mozilla::ErrorResult& aRv) MOZ_FINAL;
    void SetY(float aY, mozilla::ErrorResult& aRv) MOZ_FINAL;
    void SetWidth(float aWidth, mozilla::ErrorResult& aRv) MOZ_FINAL;
    void SetHeight(float aHeight, mozilla::ErrorResult& aRv) MOZ_FINAL;
  };

  struct DOMAnimVal MOZ_FINAL : public mozilla::dom::SVGIRect
  {
    using mozilla::dom::SVGIRect::SetX;
    using mozilla::dom::SVGIRect::SetY;
    using mozilla::dom::SVGIRect::SetWidth;
    using mozilla::dom::SVGIRect::SetHeight;

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimVal)

    DOMAnimVal(nsSVGViewBox *aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    virtual ~DOMAnimVal();

    nsSVGViewBox* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    
    
    float X() const MOZ_OVERRIDE MOZ_FINAL
    {
      mSVGElement->FlushAnimations();
      return mVal->GetAnimValue().x;
    }

    float Y() const MOZ_OVERRIDE MOZ_FINAL
    {
      mSVGElement->FlushAnimations();
      return mVal->GetAnimValue().y;
    }

    float Width() const MOZ_OVERRIDE MOZ_FINAL
    {
      mSVGElement->FlushAnimations();
      return mVal->GetAnimValue().width;
    }

    float Height() const MOZ_OVERRIDE MOZ_FINAL
    {
      mSVGElement->FlushAnimations();
      return mVal->GetAnimValue().height;
    }

    void SetX(float aX, mozilla::ErrorResult& aRv) MOZ_FINAL
    {
      aRv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    }

    void SetY(float aY, mozilla::ErrorResult& aRv) MOZ_FINAL
    {
      aRv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    }

    void SetWidth(float aWidth, mozilla::ErrorResult& aRv) MOZ_FINAL
    {
      aRv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    }

    void SetHeight(float aHeight, mozilla::ErrorResult& aRv) MOZ_FINAL
    {
      aRv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    }
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
                                     const mozilla::dom::SVGAnimationElement* aSrcElement,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const;
    virtual nsSMILValue GetBaseValue() const;
    virtual void ClearAnimValue();
    virtual nsresult SetAnimValue(const nsSMILValue& aValue);
  };
};

#endif 
