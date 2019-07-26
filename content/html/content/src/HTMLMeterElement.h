




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

  virtual nsIDOMNode* AsDOMNode() { return this; }

  

  
  double Value() const;
  void SetValue(double aValue, ErrorResult& aRv)
  {
    aRv = SetValue(aValue);
  }

  
  double Min() const;
  void SetMin(double aValue, ErrorResult& aRv)
  {
    aRv = SetMin(aValue);
  }

  
  double Max() const;
  void SetMax(double aValue, ErrorResult& aRv)
  {
    aRv = SetMax(aValue);
  }

  
  double Low() const;
  void SetLow(double aValue, ErrorResult& aRv)
  {
    aRv = SetLow(aValue);
  }

  
  double High() const;
  void SetHigh(double aValue, ErrorResult& aRv)
  {
    aRv = SetHigh(aValue);
  }

  
  double Optimum() const;
  void SetOptimum(double aValue, ErrorResult& aRv)
  {
    aRv = SetOptimum(aValue);
  }

protected:
  virtual JSObject* WrapNode(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

private:

  static const double kDefaultValue;
  static const double kDefaultMin;
  static const double kDefaultMax;

  







  nsEventStates GetOptimumState() const;
};

} 
} 

#endif 
