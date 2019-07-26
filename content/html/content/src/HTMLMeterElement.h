




#ifndef mozilla_dom_HTMLMeterElement_h
#define mozilla_dom_HTMLMeterElement_h

#include "nsIDOMHTMLMeterElement.h"
#include "nsGenericHTMLElement.h"
#include "nsAttrValue.h"
#include "nsAttrValueInlines.h"
#include "nsEventStateManager.h"
#include "nsAlgorithm.h"
#include <algorithm>

namespace mozilla {
namespace dom {

class HTMLMeterElement : public nsGenericHTMLElement,
                         public nsIDOMHTMLMeterElement
{
public:
  HTMLMeterElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLMeterElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLMETERELEMENT

  virtual nsEventStates IntrinsicState() const;

  nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const;

  bool ParseAttribute(int32_t aNamespaceID, nsIAtom* aAttribute,
                      const nsAString& aValue, nsAttrValue& aResult);

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

private:

  static const double kDefaultValue;
  static const double kDefaultMin;
  static const double kDefaultMax;

  







  nsEventStates GetOptimumState() const;

  
  double GetMin() const;

  
  double GetMax() const;

  
  double GetValue() const;

  
  double GetLow() const;

  
  double GetHigh() const;

  
  double GetOptimum() const;
};

} 
} 

#endif 
