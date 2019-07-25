





































#ifndef NS_SVGANIMATIONELEMENT_H_
#define NS_SVGANIMATIONELEMENT_H_

#include "nsSVGElement.h"
#include "nsAutoPtr.h"
#include "nsReferencedElement.h"
#include "nsIDOMSVGAnimationElement.h"
#include "nsIDOMElementTimeControl.h"
#include "nsISMILAnimationElement.h"
#include "nsSMILTimedElement.h"

typedef nsSVGElement nsSVGAnimationElementBase;

class nsSVGAnimationElement : public nsSVGAnimationElementBase,
                              public nsISMILAnimationElement,
                              public nsIDOMElementTimeControl
{
protected:
  nsSVGAnimationElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  nsresult Init();

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsSVGAnimationElement,
                                           nsSVGAnimationElementBase)
  NS_DECL_NSIDOMSVGANIMATIONELEMENT
  NS_DECL_NSIDOMELEMENTTIMECONTROL

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep, PRBool aNullParent);

  virtual nsresult UnsetAttr(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                             PRBool aNotify);

  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;

  
  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsresult AfterSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                const nsAString* aValue, PRBool aNotify);

  
  virtual const Element& AsElement() const;
  virtual Element& AsElement();
  virtual const nsAttrValue* GetAnimAttr(nsIAtom* aName) const;
  virtual PRBool GetAnimAttr(nsIAtom* aAttName, nsAString& aResult) const;
  virtual PRBool HasAnimAttr(nsIAtom* aAttName) const;
  virtual Element* GetTargetElementContent();
  virtual PRBool GetTargetAttributeName(PRInt32* aNamespaceID,
                                        nsIAtom** aLocalName) const;
  virtual nsSMILTargetAttrType GetTargetAttributeType() const;
  virtual nsSMILTimedElement& TimedElement();
  virtual nsSMILTimeContainer* GetTimeContainer();

protected:
  
  PRBool IsEventName(nsIAtom* aName);

  void UpdateHrefTarget(nsIContent* aNodeForContext,
                        const nsAString& aHrefStr);
  void AnimationTargetChanged();

  class TargetReference : public nsReferencedElement {
  public:
    TargetReference(nsSVGAnimationElement* aAnimationElement) :
      mAnimationElement(aAnimationElement) {}
  protected:
    
    
    
    virtual void ElementChanged(Element* aFrom, Element* aTo) {
      nsReferencedElement::ElementChanged(aFrom, aTo);
      mAnimationElement->AnimationTargetChanged();
    }

    
    
    virtual PRBool IsPersistent() { return PR_TRUE; }
  private:
    nsSVGAnimationElement* const mAnimationElement;
  };

  TargetReference      mHrefTarget;
  nsSMILTimedElement   mTimedElement;
};

#endif 
