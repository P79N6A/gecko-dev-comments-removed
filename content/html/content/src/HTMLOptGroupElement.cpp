




#include "mozilla/dom/HTMLOptGroupElement.h"
#include "mozilla/dom/HTMLOptGroupElementBinding.h"
#include "nsIDOMEventTarget.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsIFrame.h"
#include "nsIFormControlFrame.h"
#include "nsEventStates.h"

#include "nsEventDispatcher.h"
#include "nsHTMLSelectElement.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(OptGroup)

namespace mozilla {
namespace dom {







HTMLOptGroupElement::HTMLOptGroupElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
  SetIsDOMBinding();

  
  AddStatesSilently(NS_EVENT_STATE_ENABLED);
}

HTMLOptGroupElement::~HTMLOptGroupElement()
{
}


NS_IMPL_ADDREF_INHERITED(HTMLOptGroupElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLOptGroupElement, Element)




NS_INTERFACE_TABLE_HEAD(HTMLOptGroupElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(HTMLOptGroupElement,
                                   nsIDOMHTMLOptGroupElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLOptGroupElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


NS_IMPL_ELEMENT_CLONE(HTMLOptGroupElement)


NS_IMPL_BOOL_ATTR(HTMLOptGroupElement, Disabled, disabled)
NS_IMPL_STRING_ATTR(HTMLOptGroupElement, Label, label)


nsresult
HTMLOptGroupElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  aVisitor.mCanHandle = false;
  
  
  if (HasAttr(kNameSpaceID_None, nsGkAtoms::disabled)) {
    return NS_OK;
  }

  nsIFrame* frame = GetPrimaryFrame();
  if (frame) {
    const nsStyleUserInterface* uiStyle = frame->StyleUserInterface();
    if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE ||
        uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED) {
      return NS_OK;
    }
  }

  return nsGenericHTMLElement::PreHandleEvent(aVisitor);
}

nsIContent*
HTMLOptGroupElement::GetSelect()
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
  
  return nullptr;
}

nsresult
HTMLOptGroupElement::InsertChildAt(nsIContent* aKid,
                                   uint32_t aIndex,
                                   bool aNotify)
{
  nsSafeOptionListMutation safeMutation(GetSelect(), this, aKid, aIndex, aNotify);
  nsresult rv = nsGenericHTMLElement::InsertChildAt(aKid, aIndex, aNotify);
  if (NS_FAILED(rv)) {
    safeMutation.MutationFailed();
  }
  return rv;
}

void
HTMLOptGroupElement::RemoveChildAt(uint32_t aIndex, bool aNotify)
{
  nsSafeOptionListMutation safeMutation(GetSelect(), this, nullptr, aIndex,
                                        aNotify);
  nsGenericHTMLElement::RemoveChildAt(aIndex, aNotify);
}

nsresult
HTMLOptGroupElement::AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                  const nsAttrValue* aValue, bool aNotify)
{
  if (aNameSpaceID == kNameSpaceID_None && aName == nsGkAtoms::disabled) {
    
    
    for (nsIContent* child = nsINode::GetFirstChild(); child;
         child = child->GetNextSibling()) {
      if (child->IsHTML(nsGkAtoms::option)) {
        
        child->AsElement()->UpdateState(true);
      }
    }
  }

  return nsGenericHTMLElement::AfterSetAttr(aNameSpaceID, aName, aValue,
                                            aNotify);
}

nsEventStates
HTMLOptGroupElement::IntrinsicState() const
{
  nsEventStates state = nsGenericHTMLElement::IntrinsicState();

  if (HasAttr(kNameSpaceID_None, nsGkAtoms::disabled)) {
    state |= NS_EVENT_STATE_DISABLED;
    state &= ~NS_EVENT_STATE_ENABLED;
  } else {
    state &= ~NS_EVENT_STATE_DISABLED;
    state |= NS_EVENT_STATE_ENABLED;
  }

  return state;
}

JSObject*
HTMLOptGroupElement::WrapNode(JSContext* aCx, JSObject* aScope,
                              bool* aTriedToWrap)
{
  return HTMLOptGroupElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

} 
} 
