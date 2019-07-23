





































#ifndef NS_SVGANIMATIONELEMENT_H_
#define NS_SVGANIMATIONELEMENT_H_

#include "nsSVGElement.h"
#include "nsAutoPtr.h"
#include "nsReferencedElement.h"
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
  void UpdateHrefTarget(nsIContent* aNodeForContext,
                        const nsAString& aHrefStr);

  class TargetReference : public nsReferencedElement {
  public:
    TargetReference(nsSVGAnimationElement* aAnimationElement) :
      mAnimationElement(aAnimationElement) {}
  protected:
    
    
    
    virtual void ContentChanged(nsIContent* aFrom, nsIContent* aTo) {
      nsReferencedElement::ContentChanged(aFrom, aTo);
      mAnimationElement->AnimationNeedsResample();
    }

    
    
    virtual PRBool IsPersistent() { return PR_TRUE; }
  private:
    nsSVGAnimationElement* const mAnimationElement;
  };

  TargetReference      mHrefTarget;
  nsSMILTimedElement   mTimedElement;
  nsSMILTimeContainer* mTimedDocumentRoot;
};

#endif 
