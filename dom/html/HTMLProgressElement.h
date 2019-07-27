




#ifndef mozilla_dom_HTMLProgressElement_h
#define mozilla_dom_HTMLProgressElement_h

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"
#include "nsAttrValue.h"
#include "nsAttrValueInlines.h"
#include <algorithm>

namespace mozilla {
namespace dom {

class HTMLProgressElement final : public nsGenericHTMLElement
{
public:
  explicit HTMLProgressElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  EventStates IntrinsicState() const override;

  nsresult Clone(mozilla::dom::NodeInfo* aNodeInfo, nsINode** aResult) const override;

  bool ParseAttribute(int32_t aNamespaceID, nsIAtom* aAttribute,
                        const nsAString& aValue, nsAttrValue& aResult) override;

  
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

  virtual JSObject* WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

protected:
  






  bool IsIndeterminate() const;

  static const double kIndeterminatePosition;
  static const double kDefaultValue;
  static const double kDefaultMax;
};

} 
} 

#endif 
