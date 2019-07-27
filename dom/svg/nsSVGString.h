





#ifndef __NS_SVGSTRING_H__
#define __NS_SVGSTRING_H__

#include "nsError.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/SVGAnimatedString.h"

class nsSVGString
{

public:
  void Init(uint8_t aAttrEnum) {
    mAnimVal = nullptr;
    mAttrEnum = aAttrEnum;
    mIsBaseSet = false;
  }

  void SetBaseValue(const nsAString& aValue,
                    nsSVGElement *aSVGElement,
                    bool aDoSetAttr);
  void GetBaseValue(nsAString& aValue, const nsSVGElement *aSVGElement) const
    { aSVGElement->GetStringBaseValue(mAttrEnum, aValue); }

  void SetAnimValue(const nsAString& aValue, nsSVGElement *aSVGElement);
  void GetAnimValue(nsAString& aValue, const nsSVGElement *aSVGElement) const;

  
  
  
  
  
  bool IsExplicitlySet() const
    { return !!mAnimVal || mIsBaseSet; }

  already_AddRefed<mozilla::dom::SVGAnimatedString>
  ToDOMAnimatedString(nsSVGElement* aSVGElement);

  
  nsISMILAttr* ToSMILAttr(nsSVGElement *aSVGElement);

private:

  nsAutoPtr<nsString> mAnimVal;
  uint8_t mAttrEnum; 
  bool mIsBaseSet;

public:
  struct DOMAnimatedString final : public mozilla::dom::SVGAnimatedString
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMAnimatedString)

    DOMAnimatedString(nsSVGString* aVal, nsSVGElement* aSVGElement)
      : mozilla::dom::SVGAnimatedString(aSVGElement)
      , mVal(aVal)
    {}

    nsSVGString* mVal; 

    void GetBaseVal(nsAString & aResult) override
    {
      mVal->GetBaseValue(aResult, mSVGElement);
    }

    void SetBaseVal(const nsAString & aValue) override
    {
      mVal->SetBaseValue(aValue, mSVGElement, true);
    }

    void GetAnimVal(nsAString & aResult) override
    {
      mSVGElement->FlushAnimations();
      mVal->GetAnimValue(aResult, mSVGElement);
    }

  private:
    virtual ~DOMAnimatedString();
  };
  struct SMILString : public nsISMILAttr
  {
  public:
    SMILString(nsSVGString *aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    nsSVGString* mVal;
    nsSVGElement* mSVGElement;

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const mozilla::dom::SVGAnimationElement *aSrcElement,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const override;
    virtual nsSMILValue GetBaseValue() const override;
    virtual void ClearAnimValue() override;
    virtual nsresult SetAnimValue(const nsSMILValue& aValue) override;
  };
};
#endif 
