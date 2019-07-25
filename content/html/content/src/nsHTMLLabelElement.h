







































#ifndef nsHTMLLabelElement_h
#define nsHTMLLabelElement_h

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLLabelElement.h"

class nsHTMLLabelElement : public nsGenericHTMLFormElement,
                           public nsIDOMHTMLLabelElement
{
public:
  nsHTMLLabelElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLLabelElement();

  static nsHTMLLabelElement* FromContent(nsIContent* aPossibleLabel)
  {
    if (aPossibleLabel->IsHTML(nsGkAtoms::label)) {
      return static_cast<nsHTMLLabelElement*>(aPossibleLabel);
    }

    return nsnull;
  }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFormElement::)

  
  NS_DECL_NSIDOMHTMLLABELELEMENT

  
  NS_FORWARD_NSIDOMHTMLELEMENT_NOFOCUSCLICK(nsGenericHTMLFormElement::)
  NS_IMETHOD Click() { 
    return nsGenericHTMLFormElement::Click(); 
  }
  NS_IMETHOD Focus();

  
  NS_IMETHOD_(PRUint32) GetType() const { return NS_FORM_LABEL; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission);

  virtual bool IsDisabled() const { return PR_FALSE; }

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);

  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

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
  virtual void PerformAccesskey(PRBool aKeyCausesActivation,
                                PRBool aIsTrustedEvent);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  mozilla::dom::Element* GetLabeledElement();
protected:
  mozilla::dom::Element* GetFirstDescendantFormControl();

  
  PRPackedBool mHandlingEvent;
};

#endif 
