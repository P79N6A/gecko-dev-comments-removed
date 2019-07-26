




#ifndef mozilla_dom_HTMLMeterElement_h
#define mozilla_dom_HTMLMeterElement_h

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"
#include "nsAttrValue.h"
#include "nsAttrValueInlines.h"
#include "nsEventStateManager.h"
#include "nsAlgorithm.h"
#include <algorithm>

namespace mozilla {
namespace dom {

class HTMLMeterElement MOZ_FINAL : public nsGenericHTMLElement,
                                   public nsIDOMHTMLElement
{
public:
  HTMLMeterElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLMeterElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  virtual nsEventStates IntrinsicState() const MOZ_OVERRIDE;

  nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const MOZ_OVERRIDE;

  bool ParseAttribute(int32_t aNamespaceID, nsIAtom* aAttribute,
                      const nsAString& aValue, nsAttrValue& aResult) MOZ_OVERRIDE;

  virtual nsIDOMNode* AsDOMNode() MOZ_OVERRIDE { return this; }

  

  
  double Value() const;
  void SetValue(double aValue, ErrorResult& aRv)
  {
    aRv = SetDoubleAttr(nsGkAtoms::value, aValue);
  }

  
  double Min() const;
  void SetMin(double aValue, ErrorResult& aRv)
  {
    aRv = SetDoubleAttr(nsGkAtoms::min, aValue);
  }

  
  double Max() const;
  void SetMax(double aValue, ErrorResult& aRv)
  {
    aRv = SetDoubleAttr(nsGkAtoms::max, aValue);
  }

  
  double Low() const;
  void SetLow(double aValue, ErrorResult& aRv)
  {
    aRv = SetDoubleAttr(nsGkAtoms::low, aValue);
  }

  
  double High() const;
  void SetHigh(double aValue, ErrorResult& aRv)
  {
    aRv = SetDoubleAttr(nsGkAtoms::high, aValue);
  }

  
  double Optimum() const;
  void SetOptimum(double aValue, ErrorResult& aRv)
  {
    aRv = SetDoubleAttr(nsGkAtoms::optimum, aValue);
  }

protected:
  virtual JSObject* WrapNode(JSContext* aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

private:

  static const double kDefaultValue;
  static const double kDefaultMin;
  static const double kDefaultMax;

  







  nsEventStates GetOptimumState() const;
};

} 
} 

#endif 
