




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

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(nsHTMLIFrameElement, iframe)

  
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
  virtual nsXPCClassInfo* GetClassInfo();
  virtual nsIDOMNode* AsDOMNode() { return this; }

  virtual nsresult AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue,
                                bool aNotify);

  uint32_t GetSandboxFlags()
  {
    nsAutoString sandboxAttr;

    if (GetAttr(kNameSpaceID_None, nsGkAtoms::sandbox, sandboxAttr)) {
      return nsContentUtils::ParseSandboxAttributeToFlags(sandboxAttr);
    }

    
    return 0;
  }

protected:
  virtual void GetItemValueText(nsAString& text);
  virtual void SetItemValueText(const nsAString& text);
};
