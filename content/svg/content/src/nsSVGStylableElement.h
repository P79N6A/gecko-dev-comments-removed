





































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

protected:
  virtual nsSVGClass *GetClass()
  {
    return &mClassAttribute;
  }
  virtual void DidAnimateClass();

  nsSVGClass mClassAttribute;
  nsAutoPtr<nsAttrValue> mClassAnimAttr;
};


#endif 
