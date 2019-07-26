




#ifndef mozilla_dom_SVGMPathElement_h
#define mozilla_dom_SVGMPathElement_h

#include "nsSVGElement.h"
#include "nsStubMutationObserver.h"
#include "nsSVGString.h"
#include "nsReferencedElement.h"

nsresult NS_NewSVGMPathElement(nsIContent **aResult,
                               already_AddRefed<nsINodeInfo> aNodeInfo);

typedef nsSVGElement SVGMPathElementBase;

namespace mozilla {
namespace dom {
class SVGPathElement;

class SVGMPathElement MOZ_FINAL : public SVGMPathElementBase,
                                  public nsIDOMSVGElement,
                                  public nsStubMutationObserver
{
protected:
  friend nsresult (::NS_NewSVGMPathElement(nsIContent **aResult,
                                           already_AddRefed<nsINodeInfo> aNodeInfo));
  SVGMPathElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  ~SVGMPathElement();

  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap) MOZ_OVERRIDE;

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SVGMPathElement,
                                           SVGMPathElementBase)

  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGMPathElementBase::)

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep, bool aNullParent);

  virtual nsresult UnsetAttr(int32_t aNamespaceID, nsIAtom* aAttribute,
                             bool aNotify);
  
  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);

  
  
  
  SVGPathElement* GetReferencedPath();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  already_AddRefed<nsIDOMSVGAnimatedString> Href();

protected:
  class PathReference : public nsReferencedElement {
  public:
    PathReference(SVGMPathElement* aMpathElement) :
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
    SVGMPathElement* const mMpathElement;
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

} 
} 

#endif 
