




#ifndef mozilla_dom_HTMLFrameElement_h
#define mozilla_dom_HTMLFrameElement_h

#include "mozilla/Util.h"
#include "nsIDOMHTMLFrameElement.h"
#include "nsGenericHTMLFrameElement.h"
#include "nsGkAtoms.h"

class nsIDOMDocument;

namespace mozilla {
namespace dom {

class HTMLFrameElement : public nsGenericHTMLFrameElement,
                         public nsIDOMHTMLFrameElement
{
public:
  HTMLFrameElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                   FromParser aFromParser = NOT_FROM_PARSER);
  virtual ~HTMLFrameElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLFRAMEELEMENT

  
  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult);
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsXPCClassInfo* GetClassInfo();
  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  
  void SetFrameBorder(const nsAString& aFrameBorder, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::frameborder, aFrameBorder, aError);
  }

  
  void SetLongDesc(const nsAString& aLongDesc, ErrorResult& aError)
  {
    SetAttrHelper(nsGkAtoms::longdesc, aLongDesc);
  }

  
  void SetMarginHeight(const nsAString& aMarginHeight, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::marginheight, aMarginHeight, aError);
  }

  
  void SetMarginWidth(const nsAString& aMarginWidth, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::marginwidth, aMarginWidth, aError);
  }

  
  void SetName(const nsAString& aName, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::name, aName, aError);
  }

  bool NoResize() const
  {
   return GetBoolAttr(nsGkAtoms::noresize);
  }
  void SetNoResize(bool& aNoResize, ErrorResult& aError)
  {
    SetHTMLBoolAttr(nsGkAtoms::noresize, aNoResize, aError);
  }

  
  void SetScrolling(const nsAString& aScrolling, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::scrolling, aScrolling, aError);
  }

  
  void SetSrc(const nsAString& aSrc, ErrorResult& aError)
  {
    SetAttrHelper(nsGkAtoms::src, aSrc);
  }

  already_AddRefed<nsIDocument> GetContentDocument(ErrorResult& aRv);

  already_AddRefed<nsIDOMWindow> GetContentWindow(ErrorResult& aRv);

protected:
  virtual JSObject* WrapNode(JSContext* aCx, JSObject* aScope,
                             bool* aTriedToWrap) MOZ_OVERRIDE;
};

} 
} 

#endif 
