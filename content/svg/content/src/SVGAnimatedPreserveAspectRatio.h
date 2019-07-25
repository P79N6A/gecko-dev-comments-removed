




#ifndef MOZILLA_SVGANIMATEDPRESERVEASPECTRATIO_H__
#define MOZILLA_SVGANIMATEDPRESERVEASPECTRATIO_H__

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsError.h"
#include "nsIDOMSVGAnimPresAspRatio.h"
#include "nsIDOMSVGPresAspectRatio.h"
#include "nsISMILAttr.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"

class nsISMILAnimationElement;
class nsSMILValue;

namespace mozilla {

class SVGAnimatedPreserveAspectRatio;

class SVGPreserveAspectRatio
{
  friend class SVGAnimatedPreserveAspectRatio;

public:
  SVGPreserveAspectRatio(uint16_t aAlign, uint16_t aMeetOrSlice, bool aDefer = false)
    : mAlign(aAlign)
    , mMeetOrSlice(aMeetOrSlice)
    , mDefer(aDefer)
  {}

  SVGPreserveAspectRatio()
    : mAlign(0)
    , mMeetOrSlice(0)
    , mDefer(false)
  {}

  bool operator==(const SVGPreserveAspectRatio& aOther) const;

  nsresult SetAlign(uint16_t aAlign) {
    if (aAlign < nsIDOMSVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_NONE ||
        aAlign > nsIDOMSVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMAXYMAX)
      return NS_ERROR_FAILURE;
    mAlign = static_cast<uint8_t>(aAlign);
    return NS_OK;
  }

  uint16_t GetAlign() const {
    return mAlign;
  }

  nsresult SetMeetOrSlice(uint16_t aMeetOrSlice) {
    if (aMeetOrSlice < nsIDOMSVGPreserveAspectRatio::SVG_MEETORSLICE_MEET ||
        aMeetOrSlice > nsIDOMSVGPreserveAspectRatio::SVG_MEETORSLICE_SLICE)
      return NS_ERROR_FAILURE;
    mMeetOrSlice = static_cast<uint8_t>(aMeetOrSlice);
    return NS_OK;
  }

  uint16_t GetMeetOrSlice() const {
    return mMeetOrSlice;
  }

  void SetDefer(bool aDefer) {
    mDefer = aDefer;
  }

  bool GetDefer() const {
    return mDefer;
  }

private:
  uint8_t mAlign;
  uint8_t mMeetOrSlice;
  bool mDefer;
};

class SVGAnimatedPreserveAspectRatio
{
public:
  void Init() {
    mBaseVal.mAlign = nsIDOMSVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMIDYMID;
    mBaseVal.mMeetOrSlice = nsIDOMSVGPreserveAspectRatio::SVG_MEETORSLICE_MEET;
    mBaseVal.mDefer = false;
    mAnimVal = mBaseVal;
    mIsAnimated = false;
    mIsBaseSet = false;
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement);
  void GetBaseValueString(nsAString& aValue) const;

  void SetBaseValue(const SVGPreserveAspectRatio &aValue,
                    nsSVGElement *aSVGElement);
  nsresult SetBaseAlign(uint16_t aAlign, nsSVGElement *aSVGElement) {
    if (aAlign < nsIDOMSVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_NONE ||
        aAlign > nsIDOMSVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMAXYMAX) {
      return NS_ERROR_FAILURE;
    }
    SetBaseValue(SVGPreserveAspectRatio(
                   aAlign, mBaseVal.GetMeetOrSlice(), mBaseVal.GetDefer()),
                 aSVGElement);
    return NS_OK;
  }
  nsresult SetBaseMeetOrSlice(uint16_t aMeetOrSlice, nsSVGElement *aSVGElement) {
    if (aMeetOrSlice < nsIDOMSVGPreserveAspectRatio::SVG_MEETORSLICE_MEET ||
        aMeetOrSlice > nsIDOMSVGPreserveAspectRatio::SVG_MEETORSLICE_SLICE) {
      return NS_ERROR_FAILURE;
    }
    SetBaseValue(SVGPreserveAspectRatio(
                   mBaseVal.GetAlign(), aMeetOrSlice, mBaseVal.GetDefer()),
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
    nsIDOMSVGAnimatedPreserveAspectRatio **aResult,
    nsSVGElement* aSVGElement);
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);

private:

  SVGPreserveAspectRatio mAnimVal;
  SVGPreserveAspectRatio mBaseVal;
  bool mIsAnimated;
  bool mIsBaseSet;

  nsresult ToDOMBaseVal(nsIDOMSVGPreserveAspectRatio **aResult,
                        nsSVGElement* aSVGElement);
  nsresult ToDOMAnimVal(nsIDOMSVGPreserveAspectRatio **aResult,
                        nsSVGElement* aSVGElement);

public:
  struct DOMBaseVal MOZ_FINAL : public nsIDOMSVGPreserveAspectRatio
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMBaseVal)

    DOMBaseVal(SVGAnimatedPreserveAspectRatio* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    
    SVGAnimatedPreserveAspectRatio* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;
    
    NS_IMETHOD GetAlign(uint16_t* aAlign)
      { *aAlign = mVal->GetBaseValue().GetAlign(); return NS_OK; }
    NS_IMETHOD SetAlign(uint16_t aAlign)
      { return mVal->SetBaseAlign(aAlign, mSVGElement); }

    NS_IMETHOD GetMeetOrSlice(uint16_t* aMeetOrSlice)
      { *aMeetOrSlice = mVal->GetBaseValue().GetMeetOrSlice(); return NS_OK; }
    NS_IMETHOD SetMeetOrSlice(uint16_t aMeetOrSlice)
      { return mVal->SetBaseMeetOrSlice(aMeetOrSlice, mSVGElement); }
  };

  struct DOMAnimVal MOZ_FINAL : public nsIDOMSVGPreserveAspectRatio
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimVal)

    DOMAnimVal(SVGAnimatedPreserveAspectRatio* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    
    SVGAnimatedPreserveAspectRatio* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;
    
    
    
    NS_IMETHOD GetAlign(uint16_t* aAlign)
    {
      mSVGElement->FlushAnimations();
      *aAlign = mVal->GetAnimValue().GetAlign();
      return NS_OK;
    }
    NS_IMETHOD SetAlign(uint16_t aAlign)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }

    NS_IMETHOD GetMeetOrSlice(uint16_t* aMeetOrSlice)
    {
      mSVGElement->FlushAnimations();
      *aMeetOrSlice = mVal->GetAnimValue().GetMeetOrSlice();
      return NS_OK;
    }
    NS_IMETHOD SetMeetOrSlice(uint16_t aValue)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
  };

  struct DOMAnimPAspectRatio MOZ_FINAL : public nsIDOMSVGAnimatedPreserveAspectRatio
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimPAspectRatio)

    DOMAnimPAspectRatio(SVGAnimatedPreserveAspectRatio* aVal,
                        nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    
    SVGAnimatedPreserveAspectRatio* mVal;

    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetBaseVal(nsIDOMSVGPreserveAspectRatio **aBaseVal)
      { return mVal->ToDOMBaseVal(aBaseVal, mSVGElement); }

    NS_IMETHOD GetAnimVal(nsIDOMSVGPreserveAspectRatio **aAnimVal)
      { return mVal->ToDOMAnimVal(aAnimVal, mSVGElement); }
  };

  struct SMILPreserveAspectRatio MOZ_FINAL : public nsISMILAttr
  {
  public:
    SMILPreserveAspectRatio(SVGAnimatedPreserveAspectRatio* aVal,
                            nsSVGElement* aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    SVGAnimatedPreserveAspectRatio* mVal;
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

} 

#endif 
