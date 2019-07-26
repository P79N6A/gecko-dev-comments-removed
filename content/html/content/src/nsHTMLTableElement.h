



#include "nsIDOMHTMLTableElement.h"
#include "nsGenericHTMLElement.h"
#include "nsMappedAttributes.h"

#define TABLE_ATTRS_DIRTY ((nsMappedAttributes*)0x1)


class TableRowsCollection;

class nsHTMLTableElement :  public nsGenericHTMLElement,
                            public nsIDOMHTMLTableElement
{
public:
  nsHTMLTableElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLTableElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLTABLEELEMENT

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
  virtual nsIDOMNode* AsDOMNode() { return this; }
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  


  virtual nsresult BeforeSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                 const nsAttrValueOrString* aValue,
                                 bool aNotify);
  


  virtual nsresult AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify);

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsHTMLTableElement,
                                           nsGenericHTMLElement)
  nsMappedAttributes* GetAttributesMappedForCell();
  already_AddRefed<nsIDOMHTMLTableSectionElement> GetTHead() {
    return GetSection(nsGkAtoms::thead);
  }
  already_AddRefed<nsIDOMHTMLTableSectionElement> GetTFoot() {
    return GetSection(nsGkAtoms::tfoot);
  }
  already_AddRefed<nsIDOMHTMLTableCaptionElement> GetCaption();
  nsContentList* TBodies();
protected:
  already_AddRefed<nsIDOMHTMLTableSectionElement> GetSection(nsIAtom *aTag);

  nsRefPtr<nsContentList> mTBodies;
  nsRefPtr<TableRowsCollection> mRows;
  
  
  nsMappedAttributes *mTableInheritedAttributes;
  void BuildInheritedAttributes();
  void ReleaseInheritedAttributes() {
    if (mTableInheritedAttributes &&
        mTableInheritedAttributes != TABLE_ATTRS_DIRTY)
      NS_RELEASE(mTableInheritedAttributes);
      mTableInheritedAttributes = TABLE_ATTRS_DIRTY;
  }
};

