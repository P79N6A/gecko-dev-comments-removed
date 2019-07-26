





#ifndef nsHTMLOptionElement_h__
#define nsHTMLOptionElement_h__

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLOptionElement.h"
#include "nsIJSNativeInitializer.h"

class nsHTMLSelectElement;

class nsHTMLOptionElement : public nsGenericHTMLElement,
                            public nsIDOMHTMLOptionElement,
                            public nsIJSNativeInitializer
{
public:
  nsHTMLOptionElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLOptionElement();

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(nsHTMLOptionElement, option)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  using nsGenericElement::SetText;
  using nsGenericElement::GetText;
  NS_DECL_NSIDOMHTMLOPTIONELEMENT

  bool Selected() const;
  bool DefaultSelected() const;

  
  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext* aContext,
                        JSObject *aObj, uint32_t argc, jsval *argv);

  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              int32_t aModType) const;

  virtual nsresult BeforeSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                 const nsAttrValueOrString* aValue,
                                 bool aNotify);

  void SetSelectedInternal(bool aValue, bool aNotify);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);

  
  virtual nsEventStates IntrinsicState() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  nsresult CopyInnerTo(nsGenericElement* aDest);

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  virtual bool IsDisabled() const {
    return HasAttr(kNameSpaceID_None, nsGkAtoms::disabled);
  }
protected:
  




  nsHTMLSelectElement* GetSelect();

  bool mSelectedChanged;
  bool mIsSelected;

  
  
  bool mIsInSetDefaultSelected;
};

#endif
