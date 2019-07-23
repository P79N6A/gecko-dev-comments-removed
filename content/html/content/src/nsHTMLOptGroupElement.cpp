



































#include "nsIDOMHTMLOptGroupElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIEventStateManager.h"
#include "nsIDocument.h"

#include "nsISelectElement.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsEventDispatcher.h"
#include "nsHTMLSelectElement.h"




class nsHTMLOptGroupElement : public nsGenericHTMLElement,
                              public nsIDOMHTMLOptGroupElement
{
public:
  nsHTMLOptGroupElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLOptGroupElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLOPTGROUPELEMENT

  
  virtual nsresult InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                 PRBool aNotify);
  virtual nsresult RemoveChildAt(PRUint32 aIndex, PRBool aNotify, PRBool aMutationEvent = PR_TRUE);

  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

  virtual PRInt32 IntrinsicState() const;
 
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:

  



  nsIContent* GetSelect();
};


NS_IMPL_NS_NEW_HTML_ELEMENT(OptGroup)


nsHTMLOptGroupElement::nsHTMLOptGroupElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLOptGroupElement::~nsHTMLOptGroupElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLOptGroupElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLOptGroupElement, nsGenericElement)



NS_INTERFACE_TABLE_HEAD(nsHTMLOptGroupElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLOptGroupElement,
                                   nsIDOMHTMLOptGroupElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLOptGroupElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLOptGroupElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLOptGroupElement)


NS_IMPL_BOOL_ATTR(nsHTMLOptGroupElement, Disabled, disabled)
NS_IMPL_STRING_ATTR(nsHTMLOptGroupElement, Label, label)


nsresult
nsHTMLOptGroupElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  aVisitor.mCanHandle = PR_FALSE;
  
  
  PRBool disabled;
  nsresult rv = GetDisabled(&disabled);
  if (NS_FAILED(rv) || disabled) {
    return rv;
  }

  nsIFrame* frame = GetPrimaryFrame();
  if (frame) {
    const nsStyleUserInterface* uiStyle = frame->GetStyleUserInterface();
    if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE ||
        uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED) {
      return NS_OK;
    }
  }

  return nsGenericHTMLElement::PreHandleEvent(aVisitor);
}

nsIContent*
nsHTMLOptGroupElement::GetSelect()
{
  nsIContent* parent = this;
  while ((parent = parent->GetParent()) && parent->IsHTML()) {
    if (parent->Tag() == nsGkAtoms::select) {
      return parent;
    }
    if (parent->Tag() != nsGkAtoms::optgroup) {
      break;
    }
  }
  
  return nsnull;
}

nsresult
nsHTMLOptGroupElement::InsertChildAt(nsIContent* aKid,
                                     PRUint32 aIndex,
                                     PRBool aNotify)
{
  nsSafeOptionListMutation safeMutation(GetSelect(), this, aKid, aIndex);
  nsresult rv = nsGenericHTMLElement::InsertChildAt(aKid, aIndex, aNotify);
  if (NS_FAILED(rv)) {
    safeMutation.MutationFailed();
  }
  return rv;
}

nsresult
nsHTMLOptGroupElement::RemoveChildAt(PRUint32 aIndex, PRBool aNotify, PRBool aMutationEvent)
{
  NS_ASSERTION(aMutationEvent, "Someone tried to inhibit mutation events on optgroup child removal.");
  nsSafeOptionListMutation safeMutation(GetSelect(), this, nsnull, aIndex);
  nsresult rv = nsGenericHTMLElement::RemoveChildAt(aIndex, aNotify, aMutationEvent);
  if (NS_FAILED(rv)) {
    safeMutation.MutationFailed();
  }
  return rv;
}

PRInt32
nsHTMLOptGroupElement::IntrinsicState() const
{
  PRInt32 state = nsGenericHTMLElement::IntrinsicState();
  PRBool disabled;
  GetBoolAttr(nsGkAtoms::disabled, &disabled);
  if (disabled) {
    state |= NS_EVENT_STATE_DISABLED;
    state &= ~NS_EVENT_STATE_ENABLED;
  } else {
    state &= ~NS_EVENT_STATE_DISABLED;
    state |= NS_EVENT_STATE_ENABLED;
  }

  return state;
}
