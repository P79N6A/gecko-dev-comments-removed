




#include "SVGAngle.h"
#include "nsSVGAngle.h"
#include "mozilla/dom/SVGAngleBinding.h"

using namespace mozilla;
using namespace mozilla::dom;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION_WRAPPERCACHED(SVGAngle, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(SVGAngle)
NS_IMPL_CYCLE_COLLECTING_RELEASE(SVGAngle)

DOMCI_DATA(SVGAngle, SVGAngle)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(SVGAngle)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAngle)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAngle)
NS_INTERFACE_MAP_END

JSObject*
SVGAngle::WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap)
{
  return SVGAngleBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

uint16_t
SVGAngle::UnitType() const
{
  if (mType == AnimValue) {
    return mVal->mAnimValUnit;
  }
  return mVal->mBaseValUnit;
}

float
SVGAngle::Value() const
{
  if (mType == AnimValue) {
    return mVal->GetAnimValue();
  }
  return mVal->GetBaseValue();
}

void
SVGAngle::SetValue(float aValue, ErrorResult& rv)
{
  if (mType == AnimValue) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }
  bool isBaseVal = mType == BaseValue;
  mVal->SetBaseValue(aValue, isBaseVal ? mSVGElement : nullptr, isBaseVal);
}

float
SVGAngle::ValueInSpecifiedUnits() const
{
  if (mType == AnimValue) {
    return mVal->mAnimVal;
  }
  return mVal->mBaseVal;
}

void
SVGAngle::SetValueInSpecifiedUnits(float aValue, ErrorResult& rv)
{
  if (mType == AnimValue) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  } else if (mType == BaseValue) {
    mVal->SetBaseValueInSpecifiedUnits(aValue, mSVGElement);
  } else {
    mVal->mBaseVal = aValue;
  }
}

NS_IMETHODIMP
SVGAngle::NewValueSpecifiedUnits(uint16_t unitType,
                                 float valueInSpecifiedUnits)
{
  ErrorResult rv;
  NewValueSpecifiedUnits(unitType, valueInSpecifiedUnits, rv);
  return rv.ErrorCode();
}

void
SVGAngle::NewValueSpecifiedUnits(uint16_t unitType,
                                 float valueInSpecifiedUnits,
                                 ErrorResult& rv)
{
  if (mType == AnimValue) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }
  rv = mVal->NewValueSpecifiedUnits(unitType, valueInSpecifiedUnits,
                                    mType == BaseValue ? mSVGElement : nullptr);
}

NS_IMETHODIMP
SVGAngle::ConvertToSpecifiedUnits(uint16_t unitType)
{
  ErrorResult rv;
  ConvertToSpecifiedUnits(unitType, rv);
  return rv.ErrorCode();
}

void
SVGAngle::ConvertToSpecifiedUnits(uint16_t unitType, ErrorResult& rv)
{
  if (mType == AnimValue) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }
  rv = mVal->ConvertToSpecifiedUnits(unitType, mType == BaseValue ? mSVGElement : nullptr);
}

void
SVGAngle::SetValueAsString(const nsAString& aValue, ErrorResult& rv)
{
  if (mType == AnimValue) {
    rv.Throw(NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR);
    return;
  }
  bool isBaseVal = mType == BaseValue;
  rv = mVal->SetBaseValueString(aValue, isBaseVal ? mSVGElement : nullptr, isBaseVal);
}

NS_IMETHODIMP
SVGAngle::GetValueAsString(nsAString& aValue)
{
  if (mType == AnimValue) {
    mVal->GetAnimValueString(aValue);
  } else {
    mVal->GetBaseValueString(aValue);
  }
  return NS_OK;
}

