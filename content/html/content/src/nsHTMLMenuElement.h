




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

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(nsHTMLMenuElement, menu)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
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
