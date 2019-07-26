




#ifndef mozilla_dom_HTMLProgressElement_h
#define mozilla_dom_HTMLProgressElement_h

#include "nsIDOMHTMLProgressElement.h"
#include "nsGenericHTMLElement.h"
#include "nsAttrValue.h"
#include "nsAttrValueInlines.h"
#include "nsEventStateManager.h"
#include <algorithm>

namespace mozilla {
namespace dom {

class HTMLProgressElement : public nsGenericHTMLElement,
                            public nsIDOMHTMLProgressElement
{
public:
  HTMLProgressElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLProgressElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLPROGRESSELEMENT

  nsEventStates IntrinsicState() const;

  nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const;

  bool ParseAttribute(int32_t aNamespaceID, nsIAtom* aAttribute,
                        const nsAString& aValue, nsAttrValue& aResult);

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
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
  virtual JSObject* WrapNode(JSContext* aCx, JSObject* aScope,
                             bool* aTriedToWrap) MOZ_OVERRIDE;

protected:
  






  bool IsIndeterminate() const;

  static const double kIndeterminatePosition;
  static const double kDefaultValue;
  static const double kDefaultMax;
};

} 
} 

#endif 
