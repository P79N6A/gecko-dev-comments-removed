







































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

  
  static nsHTMLOptionElement* FromContent(nsIContent *aContent)
  {
    if (aContent && aContent->IsHTML(nsGkAtoms::option))
      return static_cast<nsHTMLOptionElement*>(aContent);
    return nsnull;
  }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  using nsGenericElement::SetText;
  using nsGenericElement::GetText;
  NS_DECL_NSIDOMHTMLOPTIONELEMENT

  bool Selected() const;
  bool DefaultSelected() const;

  
  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext* aContext,
                        JSObject *aObj, PRUint32 argc, jsval *argv);

  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              PRInt32 aModType) const;

  virtual nsresult BeforeSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                 const nsAString* aValue, PRBool aNotify);
  
  void SetSelectedInternal(PRBool aValue, PRBool aNotify);

  
  virtual nsEventStates IntrinsicState() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  nsresult CopyInnerTo(nsGenericElement* aDest) const;

  virtual nsXPCClassInfo* GetClassInfo();
protected:
  




  nsHTMLSelectElement* GetSelect();

  PRPackedBool mSelectedChanged;
  PRPackedBool mIsSelected;

  
  
  PRPackedBool mIsInSetDefaultSelected;
};

#endif
