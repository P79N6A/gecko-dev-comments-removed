




#ifndef mozilla_dom_HTMLMenuItemElement_h
#define mozilla_dom_HTMLMenuItemElement_h

#include "nsIDOMHTMLMenuItemElement.h"
#include "nsGenericHTMLElement.h"

namespace mozilla {
namespace dom {

class Visitor;

class HTMLMenuItemElement : public nsGenericHTMLElement,
                            public nsIDOMHTMLMenuItemElement
{
public:
  using mozilla::dom::Element::GetText;

  HTMLMenuItemElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                      mozilla::dom::FromParser aFromParser);
  virtual ~HTMLMenuItemElement();

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLMenuItemElement, menuitem)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLCOMMANDELEMENT

  
  NS_DECL_NSIDOMHTMLMENUITEMELEMENT

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);

  virtual void DoneCreatingElement();

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  uint8_t GetType() const { return mType; }

  


  bool IsChecked() const { return mChecked; }
  bool IsCheckedDirty() const { return mCheckedDirty; }

  void GetText(nsAString& aText);

protected:
  virtual nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify);

  void WalkRadioGroup(Visitor* aVisitor);

  HTMLMenuItemElement* GetSelectedRadio();

  void AddedToRadioGroup();

  void InitChecked();

  friend class ClearCheckedVisitor;
  friend class SetCheckedDirtyVisitor;

  void ClearChecked() { mChecked = false; }
  void SetCheckedDirty() { mCheckedDirty = true; }

private:
  uint8_t mType : 2;
  bool mParserCreating : 1;
  bool mShouldInitChecked : 1;
  bool mCheckedDirty : 1;
  bool mChecked : 1;
};

} 
} 

#endif 
