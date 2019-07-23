





































#ifndef __NS_SVGSTYLABLEELEMENT_H__
#define __NS_SVGSTYLABLEELEMENT_H__

#include "nsSVGElement.h"
#include "nsIDOMSVGStylable.h"
#include "nsIDOMSVGAnimatedString.h"
#include "nsAutoPtr.h"

typedef nsSVGElement nsSVGStylableElementBase;

class nsSVGStylableElement : public nsSVGStylableElementBase,
                             public nsIDOMSVGStylable
{
protected:
  nsSVGStylableElement(nsINodeInfo *aNodeInfo);

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGSTYLABLE

  
  virtual const nsAttrValue* DoGetClasses() const;

  
  virtual nsresult UnsetAttr(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                             PRBool aNotify);

protected:

  
  virtual PRBool ParseAttribute(PRInt32 aNamespaceID, nsIAtom* aName,
                                const nsAString& aValue,
                                nsAttrValue& aResult);

private:
  nsAutoPtr<nsAttrValue> mClassAnimAttr;

  const nsAttrValue* GetClassAnimAttr() const;
 
  void GetClassBaseValString(nsAString &aResult) const;
  void SetClassBaseValString(const nsAString& aValue);
  void GetClassAnimValString(nsAString& aResult) const;

  struct DOMAnimatedClassString : public nsIDOMSVGAnimatedString
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedClassString)

    DOMAnimatedClassString(nsSVGStylableElement *aSVGElement)
      : mSVGElement(aSVGElement) {}

    nsRefPtr<nsSVGStylableElement> mSVGElement;

    NS_IMETHOD GetBaseVal(nsAString& aResult)
      { mSVGElement->GetClassBaseValString(aResult); return NS_OK; }
    NS_IMETHOD SetBaseVal(const nsAString& aValue)
      { mSVGElement->SetClassBaseValString(aValue); return NS_OK; }
    NS_IMETHOD GetAnimVal(nsAString& aResult)
      { mSVGElement->GetClassAnimValString(aResult); return NS_OK; }
  };
};


#endif 
