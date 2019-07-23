





































#ifndef __NS_SVGANIMATEDSTRING_H__
#define __NS_SVGANIMATEDSTRING_H__

#include "nsIDOMSVGAnimatedString.h"
#include "nsSVGValue.h"

nsresult NS_NewSVGAnimatedString(nsIDOMSVGAnimatedString** result);




class nsSVGAnimatedString : public nsIDOMSVGAnimatedString,
                            public nsSVGValue
{
protected:
  friend nsresult NS_NewSVGAnimatedString(nsIDOMSVGAnimatedString** result);

public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGANIMATEDSTRING

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

  void Clear();

protected:
  nsString mBaseVal;
};

#endif 
