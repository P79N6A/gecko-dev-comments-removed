




#pragma once

#include "nsIDOMSVGAngle.h"
#include "nsWrapperCache.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"

class nsSVGAngle;

namespace mozilla {
namespace dom {

class SVGAngle MOZ_FINAL : public nsIDOMSVGAngle,
                           public nsWrapperCache
{
public:
  typedef enum {
    BaseValue,
    AnimValue,
    CreatedValue
  } AngleType;

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(SVGAngle)

  SVGAngle(nsSVGAngle* aVal, nsSVGElement *aSVGElement, AngleType aType)
    : mVal(aVal), mSVGElement(aSVGElement), mType(aType)
  {
    SetIsDOMBinding();
  }

  ~SVGAngle();

  NS_IMETHOD GetUnitType(uint16_t* aResult)
    { *aResult = UnitType(); return NS_OK; }

  NS_IMETHOD GetValue(float* aResult)
    { *aResult = Value(); return NS_OK; }
  NS_IMETHOD SetValue(float aValue)
    { ErrorResult rv; SetValue(aValue, rv); return rv.ErrorCode(); }
  NS_IMETHOD GetValueInSpecifiedUnits(float* aResult)
    { *aResult = ValueInSpecifiedUnits(); return NS_OK; }
  NS_IMETHOD SetValueInSpecifiedUnits(float aValue)
    { ErrorResult rv; SetValueInSpecifiedUnits(aValue, rv); return rv.ErrorCode(); }
  NS_IMETHOD SetValueAsString(const nsAString& aValue)
    { ErrorResult rv; SetValueAsString(aValue, rv); return rv.ErrorCode(); }
  NS_IMETHOD GetValueAsString(nsAString& aValue);
  NS_IMETHOD NewValueSpecifiedUnits(uint16_t unitType,
                                    float valueInSpecifiedUnits);
  NS_IMETHOD ConvertToSpecifiedUnits(uint16_t unitType);

  
  nsSVGElement* GetParentObject() { return mSVGElement; }
  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap);
  uint16_t UnitType() const;
  float Value() const;
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

} 
} 

