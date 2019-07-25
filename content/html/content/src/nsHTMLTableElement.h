



































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

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLTABLEELEMENT

  virtual bool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  


  virtual nsresult BeforeSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                 const nsAString* aValue, bool aNotify);
  


  virtual nsresult AfterSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                const nsAString* aValue, bool aNotify);

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsHTMLTableElement,
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

