




#include "nsIDOMHTMLMenuElement.h"
#include "nsIHTMLMenu.h"
#include "nsGenericHTMLElement.h"

class nsHTMLMenuElement : public nsGenericHTMLElement,
                          public nsIDOMHTMLMenuElement,
                          public nsIHTMLMenu
{
public:
  nsHTMLMenuElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLMenuElement();

  
  static nsHTMLMenuElement* FromContent(nsIContent* aContent)
  {
    if (aContent && aContent->IsHTML(nsGkAtoms::menu))
      return static_cast<nsHTMLMenuElement*>(aContent);
    return nullptr;
  }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLMENUELEMENT

  
  NS_DECL_NSIHTMLMENU

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  uint8_t GetType() const { return mType; }

protected:
  static bool CanLoadIcon(nsIContent* aContent, const nsAString& aIcon);

  void BuildSubmenu(const nsAString& aLabel,
                    nsIContent* aContent,
                    nsIMenuBuilder* aBuilder);

  void TraverseContent(nsIContent* aContent,
                       nsIMenuBuilder* aBuilder,
                       int8_t& aSeparator);

  void AddSeparator(nsIMenuBuilder* aBuilder, int8_t& aSeparator);

  uint8_t mType;
};
