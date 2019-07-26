




#ifndef MOZILLA_SVGANIMATEDPRESERVEASPECTRATIO_H__
#define MOZILLA_SVGANIMATEDPRESERVEASPECTRATIO_H__

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsError.h"
#include "nsISMILAttr.h"
#include "nsSVGElement.h"
#include "SVGPreserveAspectRatio.h"
#include "mozilla/Attributes.h"

class nsSMILValue;

namespace mozilla {
namespace dom {
class DOMSVGAnimatedPreserveAspectRatio;
class SVGAnimationElement;
}

class SVGAnimatedPreserveAspectRatio MOZ_FINAL
{
public:
  void Init() {
    mBaseVal.mAlign = SVG_PRESERVEASPECTRATIO_XMIDYMID;
    mBaseVal.mMeetOrSlice = SVG_MEETORSLICE_MEET;
    mBaseVal.mDefer = false;
    mAnimVal = mBaseVal;
    mIsAnimated = false;
    mIsBaseSet = false;
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement,
                              bool aDoSetAttr);
  void GetBaseValueString(nsAString& aValue) const;

  void SetBaseValue(const SVGPreserveAspectRatio &aValue,
                    nsSVGElement *aSVGElement);
  nsresult SetBaseAlign(uint16_t aAlign, nsSVGElement *aSVGElement) {
    if (aAlign < SVG_ALIGN_MIN_VALID || aAlign > SVG_ALIGN_MAX_VALID) {
      return NS_ERROR_FAILURE;
    }
    SetBaseValue(SVGPreserveAspectRatio(
                   static_cast<SVGAlign>(aAlign), mBaseVal.GetMeetOrSlice(),
                   mBaseVal.GetDefer()),
                 aSVGElement);
    return NS_OK;
  }
  nsresult SetBaseMeetOrSlice(uint16_t aMeetOrSlice, nsSVGElement *aSVGElement) {
    if (aMeetOrSlice < SVG_MEETORSLICE_MIN_VALID ||
        aMeetOrSlice > SVG_MEETORSLICE_MAX_VALID) {
      return NS_ERROR_FAILURE;
    }
    SetBaseValue(SVGPreserveAspectRatio(
                   mBaseVal.GetAlign(), static_cast<SVGMeetOrSlice>(aMeetOrSlice),
                   mBaseVal.GetDefer()),
                 aSVGElement);
    return NS_OK;
  }
  void SetAnimValue(uint64_t aPackedValue, nsSVGElement *aSVGElement);

  const SVGPreserveAspectRatio &GetBaseValue() const
    { return mBaseVal; }
  const SVGPreserveAspectRatio &GetAnimValue() const
    { return mAnimVal; }
  bool IsAnimated() const
    { return mIsAnimated; }
  bool IsExplicitlySet() const
    { return mIsAnimated || mIsBaseSet; }

  nsresult ToDOMAnimatedPreserveAspectRatio(
    mozilla::dom::DOMSVGAnimatedPreserveAspectRatio **aResult,
    nsSVGElement* aSVGElement);
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);

private:

  SVGPreserveAspectRatio mAnimVal;
  SVGPreserveAspectRatio mBaseVal;
  bool mIsAnimated;
  bool mIsBaseSet;

public:
  struct SMILPreserveAspectRatio MOZ_FINAL : public nsISMILAttr
  {
  public:
    SMILPreserveAspectRatio(SVGAnimatedPreserveAspectRatio* aVal,
                            nsSVGElement* aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    SVGAnimatedPreserveAspectRatio* mVal;
    nsSVGElement* mSVGElement;

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const dom::SVGAnimationElement* aSrcElement,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const;
    virtual nsSMILValue GetBaseValue() const;
    virtual void ClearAnimValue();
    virtual nsresult SetAnimValue(const nsSMILValue& aValue);
  };
};

namespace dom {
class DOMSVGAnimatedPreserveAspectRatio MOZ_FINAL : public nsISupports,
                                                    public nsWrapperCache
{
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMSVGAnimatedPreserveAspectRatio)

  DOMSVGAnimatedPreserveAspectRatio(SVGAnimatedPreserveAspectRatio* aVal,
                                    nsSVGElement *aSVGElement)
    : mVal(aVal), mSVGElement(aSVGElement)
  {
    SetIsDOMBinding();
  }
  ~DOMSVGAnimatedPreserveAspectRatio();

  
  nsSVGElement* GetParentObject() const { return mSVGElement; }
  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

  
  already_AddRefed<DOMSVGPreserveAspectRatio> BaseVal();
  already_AddRefed<DOMSVGPreserveAspectRatio> AnimVal();

protected:
  
  SVGAnimatedPreserveAspectRatio* mVal;
  nsRefPtr<nsSVGElement> mSVGElement;
};

} 
} 

#endif 
