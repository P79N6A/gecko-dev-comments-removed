





#ifndef __NS_SVGVIEWBOX_H__
#define __NS_SVGVIEWBOX_H__

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsError.h"
#include "mozilla/dom/SVGAnimatedRect.h"
#include "mozilla/dom/SVGIRect.h"
#include "nsISMILAttr.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"
#include "nsSVGAttrTearoffTable.h"

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

  already_AddRefed<mozilla::dom::SVGAnimatedRect>
  ToSVGAnimatedRect(nsSVGElement *aSVGElement);

  already_AddRefed<mozilla::dom::SVGIRect>
  ToDOMBaseVal(nsSVGElement* aSVGElement);

  already_AddRefed<mozilla::dom::SVGIRect>
  ToDOMAnimVal(nsSVGElement* aSVGElement);

  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);

private:

  nsSVGViewBoxRect mBaseVal;
  nsAutoPtr<nsSVGViewBoxRect> mAnimVal;
  bool mHasBaseVal;

public:
  struct DOMBaseVal final : public mozilla::dom::SVGIRect
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMBaseVal)

    DOMBaseVal(nsSVGViewBox *aVal, nsSVGElement *aSVGElement)
      : mozilla::dom::SVGIRect()
      , mVal(aVal)
      , mSVGElement(aSVGElement)
    {}

    nsSVGViewBox* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    float X() const override final
    {
      return mVal->GetBaseValue().x;
    }

    float Y() const override final
    {
      return mVal->GetBaseValue().y;
    }

    float Width() const override final
    {
      return mVal->GetBaseValue().width;
    }

    float Height() const override final
    {
      return mVal->GetBaseValue().height;
    }

    void SetX(float aX, mozilla::ErrorResult& aRv) final override;
    void SetY(float aY, mozilla::ErrorResult& aRv) final override;
    void SetWidth(float aWidth, mozilla::ErrorResult& aRv) final override;
    void SetHeight(float aHeight, mozilla::ErrorResult& aRv) final override;

    virtual nsIContent* GetParentObject() const override
    {
      return mSVGElement;
    }

  private:
    virtual ~DOMBaseVal();
  };

  struct DOMAnimVal final : public mozilla::dom::SVGIRect
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMAnimVal)

    DOMAnimVal(nsSVGViewBox *aVal, nsSVGElement *aSVGElement)
      : mozilla::dom::SVGIRect()
      , mVal(aVal)
      , mSVGElement(aSVGElement)
    {}

    nsSVGViewBox* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    
    
    float X() const override final
    {
      mSVGElement->FlushAnimations();
      return mVal->GetAnimValue().x;
    }

    float Y() const override final
    {
      mSVGElement->FlushAnimations();
      return mVal->GetAnimValue().y;
    }

    float Width() const override final
    {
      mSVGElement->FlushAnimations();
      return mVal->GetAnimValue().width;
    }

    float Height() const override final
    {
      mSVGElement->FlushAnimations();
      return mVal->GetAnimValue().height;
    }

    void SetX(float aX, mozilla::ErrorResult& aRv) final override
    {
      aRv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    }

    void SetY(float aY, mozilla::ErrorResult& aRv) final override
    {
      aRv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    }

    void SetWidth(float aWidth, mozilla::ErrorResult& aRv) final override
    {
      aRv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    }

    void SetHeight(float aHeight, mozilla::ErrorResult& aRv) final override
    {
      aRv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    }

    virtual nsIContent* GetParentObject() const override
    {
      return mSVGElement;
    }

  private:
    virtual ~DOMAnimVal();

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
                                     bool& aPreventCachingOfSandwich) const override;
    virtual nsSMILValue GetBaseValue() const override;
    virtual void ClearAnimValue() override;
    virtual nsresult SetAnimValue(const nsSMILValue& aValue) override;
  };

  static nsSVGAttrTearoffTable<nsSVGViewBox, mozilla::dom::SVGAnimatedRect>
    sSVGAnimatedRectTearoffTable;
};

#endif 
