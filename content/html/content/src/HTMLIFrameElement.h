




#ifndef mozilla_dom_HTMLIFrameElement_h
#define mozilla_dom_HTMLIFrameElement_h

#include "nsGenericHTMLFrameElement.h"
#include "nsIDOMHTMLIFrameElement.h"
#include "nsIDOMGetSVGDocument.h"

namespace mozilla {
namespace dom {

class HTMLIFrameElement MOZ_FINAL : public nsGenericHTMLFrameElement
                                  , public nsIDOMHTMLIFrameElement
                                  , public nsIDOMGetSVGDocument
{
public:
  HTMLIFrameElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                    FromParser aFromParser = NOT_FROM_PARSER);
  virtual ~HTMLIFrameElement();

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLIFrameElement, iframe)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLIFRAMEELEMENT

  
  NS_DECL_NSIDOMGETSVGDOCUMENT

  
  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsIDOMNode* AsDOMNode() { return this; }

  virtual nsresult AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue,
                                bool aNotify);

  uint32_t GetSandboxFlags();

  
  
  void SetSrc(const nsAString& aSrc, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::src, aSrc, aError);
  }
  void GetName(DOMString& aName)
  {
    GetHTMLAttr(nsGkAtoms::name, aName);
  }
  void SetName(const nsAString& aName, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::name, aName, aError);
  }
  void GetSandbox(DOMString& aSandbox)
  {
    GetHTMLAttr(nsGkAtoms::sandbox, aSandbox);
  }
  bool AllowFullscreen() const
  {
    return GetBoolAttr(nsGkAtoms::allowfullscreen);
  }
  void SetAllowFullscreen(bool aAllow, ErrorResult& aError)
  {
    SetHTMLBoolAttr(nsGkAtoms::allowfullscreen, aAllow, aError);
  }
  void GetWidth(DOMString& aWidth)
  {
    GetHTMLAttr(nsGkAtoms::width, aWidth);
  }
  void SetWidth(const nsAString& aWidth, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::width, aWidth, aError);
  }
  void GetHeight(DOMString& aHeight)
  {
    GetHTMLAttr(nsGkAtoms::height, aHeight);
  }
  void SetHeight(const nsAString& aHeight, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::height, aHeight, aError);
  }
  using nsGenericHTMLFrameElement::GetContentDocument;
  using nsGenericHTMLFrameElement::GetContentWindow;
  void GetAlign(DOMString& aAlign)
  {
    GetHTMLAttr(nsGkAtoms::align, aAlign);
  }
  void SetAlign(const nsAString& aAlign, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::align, aAlign, aError);
  }
  void GetScrolling(DOMString& aScrolling)
  {
    GetHTMLAttr(nsGkAtoms::scrolling, aScrolling);
  }
  void SetScrolling(const nsAString& aScrolling, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::scrolling, aScrolling, aError);
  }
  void GetFrameBorder(DOMString& aFrameBorder)
  {
    GetHTMLAttr(nsGkAtoms::frameborder, aFrameBorder);
  }
  void SetFrameBorder(const nsAString& aFrameBorder, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::frameborder, aFrameBorder, aError);
  }
  
  void SetLongDesc(const nsAString& aLongDesc, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::longdesc, aLongDesc, aError);
  }
  void GetMarginWidth(DOMString& aMarginWidth)
  {
    GetHTMLAttr(nsGkAtoms::marginwidth, aMarginWidth);
  }
  void SetMarginWidth(const nsAString& aMarginWidth, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::marginwidth, aMarginWidth, aError);
  }
  void GetMarginHeight(DOMString& aMarginHeight)
  {
    GetHTMLAttr(nsGkAtoms::marginheight, aMarginHeight);
  }
  void SetMarginHeight(const nsAString& aMarginHeight, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::marginheight, aMarginHeight, aError);
  }
  nsIDocument* GetSVGDocument()
  {
    return GetContentDocument();
  }
  bool Mozbrowser() const
  {
    return GetBoolAttr(nsGkAtoms::mozbrowser);
  }
  void SetMozbrowser(bool aAllow, ErrorResult& aError)
  {
    SetHTMLBoolAttr(nsGkAtoms::mozbrowser, aAllow, aError);
  }
  using nsGenericHTMLFrameElement::SetMozbrowser;
  
  

protected:
  virtual void GetItemValueText(nsAString& text);
  virtual void SetItemValueText(const nsAString& text);

  virtual JSObject* WrapNode(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;
};

} 
} 

#endif
