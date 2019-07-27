




#ifndef mozilla_dom_SVGAngle_h
#define mozilla_dom_SVGAngle_h

#include "nsWrapperCache.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"

class nsSVGAngle;

namespace mozilla {
namespace dom {

class SVGAngle MOZ_FINAL : public nsWrapperCache
{
public:
  typedef enum {
    BaseValue,
    AnimValue,
    CreatedValue
  } AngleType;

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(SVGAngle)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(SVGAngle)

  SVGAngle(nsSVGAngle* aVal, nsSVGElement *aSVGElement, AngleType aType)
    : mVal(aVal), mSVGElement(aSVGElement), mType(aType)
  {
    SetIsDOMBinding();
  }

  
  nsSVGElement* GetParentObject() { return mSVGElement; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;
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
  ~SVGAngle();

  nsSVGAngle* mVal; 
  nsRefPtr<nsSVGElement> mSVGElement;
  AngleType mType;
};

} 
} 

#endif 
