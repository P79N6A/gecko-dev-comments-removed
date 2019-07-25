





































#ifndef __NS_SVGSTYLABLEELEMENT_H__
#define __NS_SVGSTYLABLEELEMENT_H__

#include "nsSVGElement.h"
#include "nsIDOMSVGStylable.h"
#include "nsSVGClass.h"
#include "nsAutoPtr.h"

typedef nsSVGElement nsSVGStylableElementBase;

class nsSVGStylableElement : public nsSVGStylableElementBase,
                             public nsIDOMSVGStylable
{
protected:
  nsSVGStylableElement(already_AddRefed<nsINodeInfo> aNodeInfo);

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

#ifdef MOZ_SMIL
  virtual nsISMILAttr* GetAnimatedAttr(PRInt32 aNamespaceID, nsIAtom* aName);
#endif

  void DidAnimateClass();

protected:
  nsSVGClass mClassAttribute;
  nsAutoPtr<nsAttrValue> mClassAnimAttr;
};


#endif 
