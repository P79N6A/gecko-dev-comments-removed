




#ifndef __NS_SVGCLASS_H__
#define __NS_SVGCLASS_H__

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsError.h"
#include "nsISMILAttr.h"
#include "nsString.h"
#include "mozilla/Attributes.h"

class nsSVGElement;

namespace mozilla {
namespace dom {
class SVGAnimatedString;
}
}

class nsSVGClass
{

public:
  void Init() {
    mAnimVal = nullptr;
  }

  void SetBaseValue(const nsAString& aValue,
                    nsSVGElement *aSVGElement,
                    bool aDoSetAttr);
  void GetBaseValue(nsAString& aValue, const nsSVGElement *aSVGElement) const;

  void SetAnimValue(const nsAString& aValue, nsSVGElement *aSVGElement);
  void GetAnimValue(nsAString& aValue, const nsSVGElement *aSVGElement) const;
  bool IsAnimated() const
    { return !!mAnimVal; }

  already_AddRefed<mozilla::dom::SVGAnimatedString>
  ToDOMAnimatedString(nsSVGElement* aSVGElement);

  
  nsISMILAttr* ToSMILAttr(nsSVGElement *aSVGElement);

private:

  nsAutoPtr<nsString> mAnimVal;

public:
  struct SMILString : public nsISMILAttr
  {
  public:
    SMILString(nsSVGClass *aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    nsSVGClass* mVal;
    nsSVGElement* mSVGElement;

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const mozilla::dom::SVGAnimationElement *aSrcElement,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const MOZ_OVERRIDE;
    virtual nsSMILValue GetBaseValue() const MOZ_OVERRIDE;
    virtual void ClearAnimValue() MOZ_OVERRIDE;
    virtual nsresult SetAnimValue(const nsSMILValue& aValue) MOZ_OVERRIDE;
  };
};
#endif 
