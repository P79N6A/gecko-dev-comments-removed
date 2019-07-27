




#ifndef mozilla_dom_SVGMPathElement_h
#define mozilla_dom_SVGMPathElement_h

#include "nsSVGElement.h"
#include "nsStubMutationObserver.h"
#include "nsSVGString.h"
#include "nsReferencedElement.h"

nsresult NS_NewSVGMPathElement(nsIContent **aResult,
                               already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

typedef nsSVGElement SVGMPathElementBase;

namespace mozilla {
namespace dom {
class SVGPathElement;

class SVGMPathElement final : public SVGMPathElementBase,
                              public nsStubMutationObserver
{
protected:
  friend nsresult (::NS_NewSVGMPathElement(nsIContent **aResult,
                                           already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));
  explicit SVGMPathElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);
  ~SVGMPathElement();

  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SVGMPathElement,
                                           SVGMPathElementBase)

  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED

  
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) override;
  virtual void UnbindFromTree(bool aDeep, bool aNullParent) override;

  virtual nsresult UnsetAttr(int32_t aNamespaceID, nsIAtom* aAttribute,
                             bool aNotify) override;
  
  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult) override;

  
  
  
  SVGPathElement* GetReferencedPath();

  
  already_AddRefed<SVGAnimatedString> Href();

protected:
  class PathReference : public nsReferencedElement {
  public:
    explicit PathReference(SVGMPathElement* aMpathElement) :
      mMpathElement(aMpathElement) {}
  protected:
    
    
    
    virtual void ElementChanged(Element* aFrom, Element* aTo) override {
      nsReferencedElement::ElementChanged(aFrom, aTo);
      if (aFrom) {
        aFrom->RemoveMutationObserver(mMpathElement);
      }
      if (aTo) {
        aTo->AddMutationObserver(mMpathElement);
      }
      mMpathElement->NotifyParentOfMpathChange(mMpathElement->GetParent());
    }

    
    
    virtual bool IsPersistent() override { return true; }
  private:
    SVGMPathElement* const mMpathElement;
  };

  virtual StringAttributesInfo GetStringInfo() override;

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
