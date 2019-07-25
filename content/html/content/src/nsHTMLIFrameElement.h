




#include "nsGenericHTMLFrameElement.h"
#include "nsIDOMHTMLIFrameElement.h"
#include "nsIDOMGetSVGDocument.h"
#include "nsContentUtils.h"

class nsHTMLIFrameElement : public nsGenericHTMLFrameElement
                          , public nsIDOMHTMLIFrameElement
                          , public nsIDOMGetSVGDocument
{
public:
  nsHTMLIFrameElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                      mozilla::dom::FromParser aFromParser = mozilla::dom::NOT_FROM_PARSER);
  virtual ~nsHTMLIFrameElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFrameElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFrameElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFrameElement::)

  
  NS_DECL_NSIDOMHTMLIFRAMEELEMENT

  
  NS_DECL_NSIDOMGETSVGDOCUMENT

  
  virtual bool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsXPCClassInfo* GetClassInfo();
  virtual nsIDOMNode* AsDOMNode() { return this; }

  virtual nsresult AfterSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue,
                                bool aNotify);

  PRUint32 GetSandboxFlags()
  {
    nsAutoString sandboxAttr;

    if (GetAttr(kNameSpaceID_None, nsGkAtoms::sandbox, sandboxAttr)) {
      return nsContentUtils::ParseSandboxAttributeToFlags(sandboxAttr);
    }

    
    return 0;
  }

  static nsHTMLIFrameElement* FromContent(nsIContent *aContent)
  {
    if (aContent->IsHTML(nsGkAtoms::iframe)) {
      return static_cast<nsHTMLIFrameElement*>(aContent);
    }
    return nullptr;
  }

protected:
  virtual void GetItemValueText(nsAString& text);
  virtual void SetItemValueText(const nsAString& text);
};
