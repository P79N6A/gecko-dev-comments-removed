





































#ifndef __NS_SVGSTYLABLEELEMENT_H__
#define __NS_SVGSTYLABLEELEMENT_H__

#include "nsSVGElement.h"
#include "nsIDOMSVGStylable.h"
#include "nsIDOMSVGAnimatedString.h"
#include "nsAutoPtr.h"
#include "nsSVGClassValue.h"

typedef nsSVGElement nsSVGStylableElementBase;

class nsSVGStylableElement : public nsSVGStylableElementBase,
                             public nsIDOMSVGStylable
{
protected:
  nsSVGStylableElement(nsINodeInfo *aNodeInfo);
  nsresult Init();

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGSTYLABLE

  
  virtual const nsAttrValue* GetClasses() const;

protected:
  nsRefPtr<nsSVGClassValue> mClassName;
};


#endif 
