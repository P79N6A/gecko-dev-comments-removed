




































#ifndef nsHTMLLegendElement_h___
#define nsHTMLLegendElement_h___

#include "nsIDOMHTMLLegendElement.h"
#include "nsGenericHTMLElement.h"

class nsHTMLLegendElement : public nsGenericHTMLElement,
                            public nsIDOMHTMLLegendElement
{
public:
  nsHTMLLegendElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLLegendElement();

  static nsHTMLLegendElement* FromContent(nsIContent *aContent)
  {
    if (aContent->IsHTML() && aContent->Tag() == nsGkAtoms::legend)
      return static_cast<nsHTMLLegendElement*>(aContent);
    return nsnull;
  }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLLEGENDELEMENT

  
  NS_IMETHODIMP Focus();

  virtual void PerformAccesskey(PRBool aKeyCausesActivation,
                                PRBool aIsTrustedEvent);

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              PRInt32 aModType) const;
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify);
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  mozilla::dom::Element *GetFormElement()
  {
    nsCOMPtr<nsIFormControl> fieldsetControl = do_QueryInterface(GetFieldSet());

    return fieldsetControl ? fieldsetControl->GetFormElement() : nsnull;
  }

  virtual nsXPCClassInfo* GetClassInfo();
protected:
  



  nsIContent* GetFieldSet();
};

#endif 
