




#ifndef mozilla_dom_HTMLProgressElement_h
#define mozilla_dom_HTMLProgressElement_h

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"
#include "nsAttrValue.h"
#include "nsAttrValueInlines.h"
#include <algorithm>

namespace mozilla {
namespace dom {

class HTMLProgressElement MOZ_FINAL : public nsGenericHTMLElement
{
public:
  explicit HTMLProgressElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  EventStates IntrinsicState() const MOZ_OVERRIDE;

  nsresult Clone(mozilla::dom::NodeInfo* aNodeInfo, nsINode** aResult) const MOZ_OVERRIDE;

  bool ParseAttribute(int32_t aNamespaceID, nsIAtom* aAttribute,
                        const nsAString& aValue, nsAttrValue& aResult) MOZ_OVERRIDE;

  
  double Value() const;
  void SetValue(double aValue, ErrorResult& aRv)
  {
    SetDoubleAttr(nsGkAtoms::value, aValue, aRv);
  }
  double Max() const;
  void SetMax(double aValue, ErrorResult& aRv)
  {
    SetDoubleAttr(nsGkAtoms::max, aValue, aRv);
  }
  double Position() const;

protected:
  virtual ~HTMLProgressElement();

  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;

protected:
  






  bool IsIndeterminate() const;

  static const double kIndeterminatePosition;
  static const double kDefaultValue;
  static const double kDefaultMax;
};

} 
} 

#endif 
