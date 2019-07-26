




#ifndef mozilla_dom_HTMLProgressElement_h
#define mozilla_dom_HTMLProgressElement_h

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"
#include "nsAttrValue.h"
#include "nsAttrValueInlines.h"
#include "nsEventStateManager.h"
#include <algorithm>

namespace mozilla {
namespace dom {

class HTMLProgressElement MOZ_FINAL : public nsGenericHTMLElement
{
public:
  HTMLProgressElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLProgressElement();

  nsEventStates IntrinsicState() const MOZ_OVERRIDE;

  nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const MOZ_OVERRIDE;

  bool ParseAttribute(int32_t aNamespaceID, nsIAtom* aAttribute,
                        const nsAString& aValue, nsAttrValue& aResult) MOZ_OVERRIDE;

  
  double Value() const;
  void SetValue(double aValue, ErrorResult& aRv)
  {
    aRv = SetDoubleAttr(nsGkAtoms::value, aValue);
  }
  double Max() const;
  void SetMax(double aValue, ErrorResult& aRv)
  {
    aRv = SetDoubleAttr(nsGkAtoms::max, aValue);
  }
  double Position() const;

protected:
  virtual JSObject* WrapNode(JSContext* aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

protected:
  






  bool IsIndeterminate() const;

  static const double kIndeterminatePosition;
  static const double kDefaultValue;
  static const double kDefaultMax;
};

} 
} 

#endif 
