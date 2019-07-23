





































#ifndef __NS_SVGPRESERVEASPECTRATIO_H__
#define __NS_SVGPRESERVEASPECTRATIO_H__

#include "nsIDOMSVGPresAspectRatio.h"
#include "nsIDOMSVGAnimPresAspRatio.h"
#include "nsSVGElement.h"
#include "nsDOMError.h"

class nsSVGPreserveAspectRatio
{
public:
  class PreserveAspectRatio
  {
  friend class nsSVGPreserveAspectRatio;

  public:
    nsresult SetAlign(PRUint16 aAlign) {
      if (aAlign < nsIDOMSVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_NONE ||
          aAlign > nsIDOMSVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMAXYMAX)
        return NS_ERROR_FAILURE;
      mAlign = static_cast<PRUint8>(aAlign);
      return NS_OK;
    };

    PRUint16 GetAlign() const {
      return mAlign;
    };

    nsresult SetMeetOrSlice(PRUint16 aMeetOrSlice) {
      if (aMeetOrSlice < nsIDOMSVGPreserveAspectRatio::SVG_MEETORSLICE_MEET ||
          aMeetOrSlice > nsIDOMSVGPreserveAspectRatio::SVG_MEETORSLICE_SLICE)
        return NS_ERROR_FAILURE;
      mMeetOrSlice = static_cast<PRUint8>(aMeetOrSlice);
      return NS_OK;
    };

    PRUint16 GetMeetOrSlice() const {
      return mMeetOrSlice;
    };

    void SetDefer(PRBool aDefer) {
      mDefer = aDefer;
    };

    PRBool GetDefer() const {
      return mDefer;
    };

  private:
    PRUint8 mAlign;
    PRUint8 mMeetOrSlice;
    PRPackedBool mDefer;
  };

  void Init() {
    mBaseVal.mAlign = nsIDOMSVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMIDYMID;
    mBaseVal.mMeetOrSlice = nsIDOMSVGPreserveAspectRatio::SVG_MEETORSLICE_MEET;
    mBaseVal.mDefer = PR_FALSE;
    mAnimVal = mBaseVal;
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement,
                              PRBool aDoSetAttr);
  void GetBaseValueString(nsAString& aValue);

  nsresult SetBaseAlign(PRUint16 aAlign, nsSVGElement *aSVGElement);
  nsresult SetBaseMeetOrSlice(PRUint16 aMeetOrSlice, nsSVGElement *aSVGElement);
  const PreserveAspectRatio &GetBaseValue() const
    { return mBaseVal; }
  const PreserveAspectRatio &GetAnimValue() const
    { return mAnimVal; }

  nsresult ToDOMAnimatedPreserveAspectRatio(
    nsIDOMSVGAnimatedPreserveAspectRatio **aResult,
    nsSVGElement* aSVGElement);

private:

  PreserveAspectRatio mAnimVal;
  PreserveAspectRatio mBaseVal;

  nsresult ToDOMBaseVal(nsIDOMSVGPreserveAspectRatio **aResult,
                        nsSVGElement* aSVGElement);
  nsresult ToDOMAnimVal(nsIDOMSVGPreserveAspectRatio **aResult,
                        nsSVGElement* aSVGElement);

  struct DOMBaseVal;
  friend struct DOMBaseVal;
  struct DOMBaseVal : public nsIDOMSVGPreserveAspectRatio
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMBaseVal)

    DOMBaseVal(nsSVGPreserveAspectRatio* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    
    nsSVGPreserveAspectRatio* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;
    
    NS_IMETHOD GetAlign(PRUint16* aAlign)
      { *aAlign = mVal->GetBaseValue().mAlign; return NS_OK; }
    NS_IMETHOD SetAlign(PRUint16 aAlign)
      { return mVal->SetBaseAlign(aAlign, mSVGElement); }

    NS_IMETHOD GetMeetOrSlice(PRUint16* aMeetOrSlice)
      { *aMeetOrSlice = mVal->GetBaseValue().mMeetOrSlice; return NS_OK; }
    NS_IMETHOD SetMeetOrSlice(PRUint16 aMeetOrSlice)
      { return mVal->SetBaseMeetOrSlice(aMeetOrSlice, mSVGElement); }
  };

  struct DOMAnimVal;
  friend struct DOMAnimVal;
  struct DOMAnimVal : public nsIDOMSVGPreserveAspectRatio
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimVal)

    DOMAnimVal(nsSVGPreserveAspectRatio* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    
    nsSVGPreserveAspectRatio* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;
    
    NS_IMETHOD GetAlign(PRUint16* aAlign)
      { *aAlign = mVal->GetBaseValue().mAlign; return NS_OK; }
    NS_IMETHOD SetAlign(PRUint16 aAlign)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }

    NS_IMETHOD GetMeetOrSlice(PRUint16* aMeetOrSlice)
      { *aMeetOrSlice = mVal->GetBaseValue().mMeetOrSlice; return NS_OK; }
    NS_IMETHOD SetMeetOrSlice(PRUint16 aValue)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
  };

  struct DOMAnimPAspectRatio : public nsIDOMSVGAnimatedPreserveAspectRatio
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimPAspectRatio)

    DOMAnimPAspectRatio(nsSVGPreserveAspectRatio* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    
    nsSVGPreserveAspectRatio* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetBaseVal(nsIDOMSVGPreserveAspectRatio **aBaseVal)
      { return mVal->ToDOMBaseVal(aBaseVal, mSVGElement); }

    NS_IMETHOD GetAnimVal(nsIDOMSVGPreserveAspectRatio **aAnimVal)
      { return mVal->ToDOMAnimVal(aAnimVal, mSVGElement); }
  };

};

#endif 
