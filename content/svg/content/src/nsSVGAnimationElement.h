





































#ifndef NS_SVGANIMATIONELEMENT_H_
#define NS_SVGANIMATIONELEMENT_H_

#include "nsSVGElement.h"
#include "nsAutoPtr.h"
#include "nsIDOMSVGAnimationElement.h"
#include "nsIDOMElementTimeControl.h"
#include "nsISMILAnimationElement.h"
#include "nsSMILTimedElement.h"

class nsSMILTimeContainer;

typedef nsSVGElement nsSVGAnimationElementBase;

class nsSVGAnimationElement : public nsSVGAnimationElementBase,
                              public nsISMILAnimationElement,
                              public nsIDOMElementTimeControl
{
protected:
  nsSVGAnimationElement(nsINodeInfo *aNodeInfo);
  nsresult Init();

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGANIMATIONELEMENT
  NS_DECL_NSIDOMELEMENTTIMECONTROL

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep, PRBool aNullParent);

  
  virtual nsresult UnsetAttr(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                             PRBool aNotify);

  
  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);

  
  virtual const nsIContent& Content() const;
  virtual nsIContent& Content();
  virtual const nsAttrValue* GetAnimAttr(nsIAtom* aName) const;
  virtual nsIContent* GetTargetElementContent();
  virtual nsIAtom* GetTargetAttributeName() const;
  virtual nsSMILTargetAttrType GetTargetAttributeType() const;
  virtual nsSMILTimedElement& TimedElement();
  virtual nsSMILTimeContainer* GetTimeContainer();

protected:
  nsSMILTimedElement   mTimedElement;
  nsSMILTimeContainer* mTimedDocumentRoot;
};

#endif 
