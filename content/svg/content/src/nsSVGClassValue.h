





































#ifndef __NS_SVGCLASSVALUE_H__
#define __NS_SVGCLASSVALUE_H__

#include "nsIDOMSVGAnimatedString.h"
#include "nsSVGValue.h"
#include "nsAttrValue.h"

class nsSVGClassValue : public nsIDOMSVGAnimatedString,
                        public nsSVGValue
{
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGANIMATEDSTRING

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

  const nsAttrValue* GetAttrValue()
  {
    return &mBaseVal;
  }

protected:
  nsAttrValue mBaseVal;
};

#endif 
