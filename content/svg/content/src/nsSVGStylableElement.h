





































#ifndef __NS_SVGSTYLABLEELEMENT_H__
#define __NS_SVGSTYLABLEELEMENT_H__

#include "nsAutoPtr.h"
#include "nsIDOMSVGStylable.h"
#include "nsSVGClass.h"
#include "nsSVGElement.h"

typedef nsSVGElement nsSVGStylableElementBase;

class nsSVGStylableElement : public nsSVGStylableElementBase,
                             public nsIDOMSVGStylable
{
protected:
  nsSVGStylableElement(already_AddRefed<nsNodeInfo> aNodeInfo);

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGSTYLABLE

  
  virtual const nsAttrValue* DoGetClasses() const;

  nsIDOMCSSStyleDeclaration* GetStyle(nsresult* retval)
  {
    return nsSVGStylableElementBase::GetStyle(retval);
  }
  virtual bool ParseAttribute(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                              const nsAString& aValue, nsAttrValue& aResult);
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify);

  virtual nsISMILAttr* GetAnimatedAttr(PRInt32 aNamespaceID, nsIAtom* aName);

  void DidAnimateClass();

protected:
  nsSVGClass mClassAttribute;
  nsAutoPtr<nsAttrValue> mClassAnimAttr;
};


#endif 
