




#pragma once

#include "nsWrapperCache.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"

class nsSVGAngle;






#define MOZILLA_SVGANGLE_IID \
{0x2cd27ef5, 0x81d8, 0x4720, \
  {0x81, 0x42, 0x66, 0xc6, 0xa9, 0xbe, 0xc3, 0xeb } }


namespace mozilla {
namespace dom {

class SVGAngle MOZ_FINAL : public nsISupports,
                           public nsWrapperCache
{
public:
  typedef enum {
    BaseValue,
    AnimValue,
    CreatedValue
  } AngleType;

  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_SVGANGLE_IID)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SVGAngle)

  SVGAngle(nsSVGAngle* aVal, nsSVGElement *aSVGElement, AngleType aType)
    : mVal(aVal), mSVGElement(aSVGElement), mType(aType)
  {
    SetIsDOMBinding();
  }

  ~SVGAngle();

  
  nsSVGElement* GetParentObject() { return mSVGElement; }
  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;
  uint16_t UnitType() const;
  float Value() const;
  void GetValueAsString(nsAString& aValue);
  void SetValue(float aValue, ErrorResult& rv);
  float ValueInSpecifiedUnits() const;
  void SetValueInSpecifiedUnits(float aValue, ErrorResult& rv);
  void SetValueAsString(const nsAString& aValue, ErrorResult& rv);
  void NewValueSpecifiedUnits(uint16_t unitType, float value, ErrorResult& rv);
  void ConvertToSpecifiedUnits(uint16_t unitType, ErrorResult& rv);

protected:
  nsSVGAngle* mVal; 
  nsRefPtr<nsSVGElement> mSVGElement;
  AngleType mType;
};

NS_DEFINE_STATIC_IID_ACCESSOR(SVGAngle, MOZILLA_SVGANGLE_IID)

} 
} 

