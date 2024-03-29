





#ifndef MOZILLA_CONTENT_SVGPRESERVEASPECTRATIO_H_
#define MOZILLA_CONTENT_SVGPRESERVEASPECTRATIO_H_

#include "mozilla/HashFunctions.h"  

#include "nsWrapperCache.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/ErrorResult.h"
#include "nsSVGElement.h"

namespace mozilla {

enum SVGAlign : uint8_t {
  SVG_PRESERVEASPECTRATIO_UNKNOWN = 0,
  SVG_PRESERVEASPECTRATIO_NONE = 1,
  SVG_PRESERVEASPECTRATIO_XMINYMIN = 2,
  SVG_PRESERVEASPECTRATIO_XMIDYMIN = 3,
  SVG_PRESERVEASPECTRATIO_XMAXYMIN = 4,
  SVG_PRESERVEASPECTRATIO_XMINYMID = 5,
  SVG_PRESERVEASPECTRATIO_XMIDYMID = 6,
  SVG_PRESERVEASPECTRATIO_XMAXYMID = 7,
  SVG_PRESERVEASPECTRATIO_XMINYMAX = 8,
  SVG_PRESERVEASPECTRATIO_XMIDYMAX = 9,
  SVG_PRESERVEASPECTRATIO_XMAXYMAX = 10
};



const uint16_t SVG_ALIGN_MIN_VALID = SVG_PRESERVEASPECTRATIO_NONE;
const uint16_t SVG_ALIGN_MAX_VALID = SVG_PRESERVEASPECTRATIO_XMAXYMAX;


enum SVGMeetOrSlice : uint8_t {
  SVG_MEETORSLICE_UNKNOWN = 0,
  SVG_MEETORSLICE_MEET = 1,
  SVG_MEETORSLICE_SLICE = 2
};



const uint16_t SVG_MEETORSLICE_MIN_VALID = SVG_MEETORSLICE_MEET;
const uint16_t SVG_MEETORSLICE_MAX_VALID = SVG_MEETORSLICE_SLICE;

class SVGAnimatedPreserveAspectRatio;

class SVGPreserveAspectRatio final
{
  friend class SVGAnimatedPreserveAspectRatio;
public:
  SVGPreserveAspectRatio(SVGAlign aAlign, SVGMeetOrSlice aMeetOrSlice,
                         bool aDefer = false)
    : mAlign(aAlign)
    , mMeetOrSlice(aMeetOrSlice)
    , mDefer(aDefer)
  {}

  bool operator==(const SVGPreserveAspectRatio& aOther) const;

  explicit SVGPreserveAspectRatio()
    : mAlign(SVG_PRESERVEASPECTRATIO_UNKNOWN)
    , mMeetOrSlice(SVG_MEETORSLICE_UNKNOWN)
    , mDefer(false)
  {}

  nsresult SetAlign(uint16_t aAlign) {
    if (aAlign < SVG_ALIGN_MIN_VALID || aAlign > SVG_ALIGN_MAX_VALID)
      return NS_ERROR_FAILURE;
    mAlign = static_cast<uint8_t>(aAlign);
    return NS_OK;
  }

  SVGAlign GetAlign() const {
    return static_cast<SVGAlign>(mAlign);
  }

  nsresult SetMeetOrSlice(uint16_t aMeetOrSlice) {
    if (aMeetOrSlice < SVG_MEETORSLICE_MIN_VALID ||
        aMeetOrSlice > SVG_MEETORSLICE_MAX_VALID)
      return NS_ERROR_FAILURE;
    mMeetOrSlice = static_cast<uint8_t>(aMeetOrSlice);
    return NS_OK;
  }

  SVGMeetOrSlice GetMeetOrSlice() const {
    return static_cast<SVGMeetOrSlice>(mMeetOrSlice);
  }

  void SetDefer(bool aDefer) {
    mDefer = aDefer;
  }

  bool GetDefer() const {
    return mDefer;
  }

  uint32_t Hash() const {
    return HashGeneric(mAlign, mMeetOrSlice, mDefer);
  }

private:
  
  uint8_t mAlign;
  uint8_t mMeetOrSlice;
  bool mDefer;
};

namespace dom {

class DOMSVGPreserveAspectRatio final : public nsISupports,
                                        public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMSVGPreserveAspectRatio)

  DOMSVGPreserveAspectRatio(SVGAnimatedPreserveAspectRatio* aVal,
                            nsSVGElement *aSVGElement,
                            bool aIsBaseValue)
    : mVal(aVal), mSVGElement(aSVGElement), mIsBaseValue(aIsBaseValue)
  {
  }

  
  nsSVGElement* GetParentObject() const { return mSVGElement; }
  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  uint16_t Align();
  void SetAlign(uint16_t aAlign, ErrorResult& rv);
  uint16_t MeetOrSlice();
  void SetMeetOrSlice(uint16_t aMeetOrSlice, ErrorResult& rv);

protected:
  ~DOMSVGPreserveAspectRatio();

  SVGAnimatedPreserveAspectRatio* mVal; 
  nsRefPtr<nsSVGElement> mSVGElement;
  const bool mIsBaseValue;
};

} 
} 

#endif 
