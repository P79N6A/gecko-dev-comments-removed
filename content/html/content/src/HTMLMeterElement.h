




#ifndef mozilla_dom_HTMLMeterElement_h
#define mozilla_dom_HTMLMeterElement_h

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"
#include "nsAttrValue.h"
#include "nsAttrValueInlines.h"
#include "nsAlgorithm.h"
#include <algorithm>

namespace mozilla {
namespace dom {

class HTMLMeterElement MOZ_FINAL : public nsGenericHTMLElement
{
public:
  explicit HTMLMeterElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  virtual EventStates IntrinsicState() const MOZ_OVERRIDE;

  nsresult Clone(mozilla::dom::NodeInfo* aNodeInfo, nsINode** aResult) const MOZ_OVERRIDE;

  bool ParseAttribute(int32_t aNamespaceID, nsIAtom* aAttribute,
                      const nsAString& aValue, nsAttrValue& aResult) MOZ_OVERRIDE;

  

  
  double Value() const;
  void SetValue(double aValue, ErrorResult& aRv)
  {
    SetDoubleAttr(nsGkAtoms::value, aValue, aRv);
  }

  
  double Min() const;
  void SetMin(double aValue, ErrorResult& aRv)
  {
    SetDoubleAttr(nsGkAtoms::min, aValue, aRv);
  }

  
  double Max() const;
  void SetMax(double aValue, ErrorResult& aRv)
  {
    SetDoubleAttr(nsGkAtoms::max, aValue, aRv);
  }

  
  double Low() const;
  void SetLow(double aValue, ErrorResult& aRv)
  {
    SetDoubleAttr(nsGkAtoms::low, aValue, aRv);
  }

  
  double High() const;
  void SetHigh(double aValue, ErrorResult& aRv)
  {
    SetDoubleAttr(nsGkAtoms::high, aValue, aRv);
  }

  
  double Optimum() const;
  void SetOptimum(double aValue, ErrorResult& aRv)
  {
    SetDoubleAttr(nsGkAtoms::optimum, aValue, aRv);
  }

protected:
  virtual ~HTMLMeterElement();

  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;

private:

  static const double kDefaultValue;
  static const double kDefaultMin;
  static const double kDefaultMax;

  







  EventStates GetOptimumState() const;
};

} 
} 

#endif 
