




































#ifndef NS_SVGMPATHELEMENT_H_
#define NS_SVGMPATHELEMENT_H_

#include "nsIDOMSVGMpathElement.h"
#include "nsIDOMSVGURIReference.h"
#include "nsSVGElement.h"
#include "nsStubMutationObserver.h"
#include "nsSVGPathElement.h"
#include "nsSVGString.h"
#include "nsReferencedElement.h"


typedef nsSVGElement nsSVGMpathElementBase;

class nsSVGMpathElement : public nsSVGMpathElementBase,
                          public nsIDOMSVGMpathElement,
                          public nsIDOMSVGURIReference,
                          public nsStubMutationObserver
{
protected:
  friend nsresult NS_NewSVGMpathElement(nsIContent **aResult,
                                        already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGMpathElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  ~nsSVGMpathElement();


public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGMPATHELEMENT
  NS_DECL_NSIDOMSVGURIREFERENCE

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsSVGMpathElement,
                                           nsSVGMpathElementBase)

  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED

  
  NS_FORWARD_NSIDOMNODE(nsSVGMpathElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGMpathElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGMpathElementBase::)

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep, bool aNullParent);

  virtual nsresult UnsetAttr(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                             bool aNotify);
  
  virtual bool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);

  
  
  
  nsSVGPathElement* GetReferencedPath();

  virtual nsXPCClassInfo* GetClassInfo();
protected:
  class PathReference : public nsReferencedElement {
  public:
    PathReference(nsSVGMpathElement* aMpathElement) :
      mMpathElement(aMpathElement) {}
  protected:
    
    
    
    virtual void ElementChanged(Element* aFrom, Element* aTo) {
      nsReferencedElement::ElementChanged(aFrom, aTo);
      if (aFrom) {
        aFrom->RemoveMutationObserver(mMpathElement);
      }
      if (aTo) {
        aTo->AddMutationObserver(mMpathElement);
      }
      mMpathElement->NotifyParentOfMpathChange(mMpathElement->GetParent());
    }

    
    
    virtual bool IsPersistent() { return true; }
  private:
    nsSVGMpathElement* const mMpathElement;
  };

  virtual StringAttributesInfo GetStringInfo();

  void UpdateHrefTarget(nsIContent* aParent, const nsAString& aHrefStr);
  void UnlinkHrefTarget(bool aNotifyParent);
  void NotifyParentOfMpathChange(nsIContent* aParent);

  enum { HREF };
  nsSVGString        mStringAttributes[1];
  static StringInfo  sStringInfo[1];
  PathReference      mHrefTarget;
};

#endif 
