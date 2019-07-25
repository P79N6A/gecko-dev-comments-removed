




#ifndef nsHTMLImageElement_h
#define nsHTMLImageElement_h

#include "nsGenericHTMLElement.h"
#include "nsImageLoadingContent.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIJSNativeInitializer.h"

class nsHTMLImageElement : public nsGenericHTMLElement,
                           public nsImageLoadingContent,
                           public nsIDOMHTMLImageElement,
                           public nsIJSNativeInitializer
{
public:
  nsHTMLImageElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLImageElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT_BASIC(nsGenericHTMLElement::)
  NS_IMETHOD Click() {
    return nsGenericHTMLElement::Click();
  }
  NS_IMETHOD GetTabIndex(int32_t* aTabIndex) {
    return nsGenericHTMLElement::GetTabIndex(aTabIndex);
  }
  NS_IMETHOD SetTabIndex(int32_t aTabIndex) {
    return nsGenericHTMLElement::SetTabIndex(aTabIndex);
  }
  NS_IMETHOD Focus() {
    return nsGenericHTMLElement::Focus();
  }
  NS_IMETHOD GetDraggable(bool* aDraggable);
  NS_IMETHOD GetInnerHTML(nsAString& aInnerHTML) {
    return nsGenericHTMLElement::GetInnerHTML(aInnerHTML);
  }
  NS_IMETHOD SetInnerHTML(const nsAString& aInnerHTML) {
    return nsGenericHTMLElement::SetInnerHTML(aInnerHTML);
  }

  
  NS_DECL_NSIDOMHTMLIMAGEELEMENT

  
  mozilla::CORSMode GetCORSMode();

  
  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext* aContext,
                        JSObject* aObj, uint32_t argc, jsval* argv);

  
  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              int32_t aModType) const;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

  bool IsHTMLFocusable(bool aWithMouse, bool *aIsFocusable, int32_t *aTabIndex);

  
  
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify);
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep, bool aNullParent);

  virtual nsEventStates IntrinsicState() const;
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  nsresult CopyInnerTo(nsGenericElement* aDest);

  void MaybeLoadImage();
  virtual nsXPCClassInfo* GetClassInfo();
  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  nsIntPoint GetXY();
  virtual void GetItemValueText(nsAString& text);
  virtual void SetItemValueText(const nsAString& text);
};

#endif 
