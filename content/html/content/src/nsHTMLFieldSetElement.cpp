




#include "nsHTMLFieldSetElement.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMEventTarget.h"
#include "nsStyleConsts.h"
#include "nsIForm.h"
#include "nsIFormControl.h"
#include "nsGUIEvent.h"
#include "nsEventDispatcher.h"


NS_IMPL_NS_NEW_HTML_ELEMENT(FieldSet)


nsHTMLFieldSetElement::nsHTMLFieldSetElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLFormElement(aNodeInfo)
  , mElements(nullptr)
  , mFirstLegend(nullptr)
{
  
  SetBarredFromConstraintValidation(true);

  
  AddStatesSilently(NS_EVENT_STATE_ENABLED);
}

nsHTMLFieldSetElement::~nsHTMLFieldSetElement()
{
  uint32_t length = mDependentElements.Length();
  for (uint32_t i = 0; i < length; ++i) {
    mDependentElements[i]->ForgetFieldSet(this);
  }
}



NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsHTMLFieldSetElement,
                                                nsGenericHTMLFormElement)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mElements)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_CLASS(nsHTMLFieldSetElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHTMLFieldSetElement,
                                                  nsGenericHTMLFormElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mElements, nsIDOMNodeList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsHTMLFieldSetElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLFieldSetElement, nsGenericElement)

DOMCI_NODE_DATA(HTMLFieldSetElement, nsHTMLFieldSetElement)


NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsHTMLFieldSetElement)
  NS_HTML_CONTENT_INTERFACE_TABLE2(nsHTMLFieldSetElement,
                                   nsIDOMHTMLFieldSetElement,
                                   nsIConstraintValidation)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLFieldSetElement,
                                               nsGenericHTMLFormElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLFieldSetElement)

NS_IMPL_ELEMENT_CLONE(nsHTMLFieldSetElement)


NS_IMPL_BOOL_ATTR(nsHTMLFieldSetElement, Disabled, disabled)
NS_IMPL_STRING_ATTR(nsHTMLFieldSetElement, Name, name)


NS_IMPL_NSICONSTRAINTVALIDATION(nsHTMLFieldSetElement)


nsresult
nsHTMLFieldSetElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  
  aVisitor.mCanHandle = false;
  if (IsElementDisabledForEvents(aVisitor.mEvent->message, NULL)) {
    return NS_OK;
  }

  return nsGenericHTMLFormElement::PreHandleEvent(aVisitor);
}

nsresult
nsHTMLFieldSetElement::AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                    const nsAttrValue* aValue, bool aNotify)
{
  if (aNameSpaceID == kNameSpaceID_None && aName == nsGkAtoms::disabled &&
      nsINode::GetFirstChild()) {
    if (!mElements) {
      mElements = new nsContentList(this, MatchListedElements, nullptr, nullptr,
                                    true);
    }

    uint32_t length = mElements->Length(true);
    for (uint32_t i=0; i<length; ++i) {
      static_cast<nsGenericHTMLFormElement*>(mElements->GetNodeAt(i))
        ->FieldSetDisabledChanged(aNotify);
    }
  }

  return nsGenericHTMLFormElement::AfterSetAttr(aNameSpaceID, aName,
                                                aValue, aNotify);
}



NS_IMETHODIMP
nsHTMLFieldSetElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElement::GetForm(aForm);
}

NS_IMETHODIMP
nsHTMLFieldSetElement::GetType(nsAString& aType)
{
  aType.AssignLiteral("fieldset");
  return NS_OK;
}


bool
nsHTMLFieldSetElement::MatchListedElements(nsIContent* aContent, int32_t aNamespaceID,
                                           nsIAtom* aAtom, void* aData)
{
  nsCOMPtr<nsIFormControl> formControl = do_QueryInterface(aContent);
  return formControl && formControl->GetType() != NS_FORM_LABEL;
}

NS_IMETHODIMP
nsHTMLFieldSetElement::GetElements(nsIDOMHTMLCollection** aElements)
{
  if (!mElements) {
    mElements = new nsContentList(this, MatchListedElements, nullptr, nullptr,
                                  true);
  }

  NS_ADDREF(*aElements = mElements);
  return NS_OK;
}



nsresult
nsHTMLFieldSetElement::Reset()
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFieldSetElement::SubmitNamesValues(nsFormSubmission* aFormSubmission)
{
  return NS_OK;
}

nsresult
nsHTMLFieldSetElement::InsertChildAt(nsIContent* aChild, uint32_t aIndex,
                                     bool aNotify)
{
  bool firstLegendHasChanged = false;

  if (aChild->IsHTML(nsGkAtoms::legend)) {
    if (!mFirstLegend) {
      mFirstLegend = aChild;
      
    } else {
      
      
      if (int32_t(aIndex) <= IndexOf(mFirstLegend)) {
        mFirstLegend = aChild;
        firstLegendHasChanged = true;
      }
    }
  }

  nsresult rv = nsGenericHTMLFormElement::InsertChildAt(aChild, aIndex, aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  if (firstLegendHasChanged) {
    NotifyElementsForFirstLegendChange(aNotify);
  }

  return rv;
}

void
nsHTMLFieldSetElement::RemoveChildAt(uint32_t aIndex, bool aNotify)
{
  bool firstLegendHasChanged = false;

  if (mFirstLegend && (GetChildAt(aIndex) == mFirstLegend)) {
    
    nsIContent* child = mFirstLegend->GetNextSibling();
    mFirstLegend = nullptr;
    firstLegendHasChanged = true;

    for (; child; child = child->GetNextSibling()) {
      if (child->IsHTML(nsGkAtoms::legend)) {
        mFirstLegend = child;
        break;
      }
    }
  }

  nsGenericHTMLFormElement::RemoveChildAt(aIndex, aNotify);

  if (firstLegendHasChanged) {
    NotifyElementsForFirstLegendChange(aNotify);
  }
}

void
nsHTMLFieldSetElement::NotifyElementsForFirstLegendChange(bool aNotify)
{
  





  if (!mElements) {
    mElements = new nsContentList(this, MatchListedElements, nullptr, nullptr,
                                  true);
  }

  uint32_t length = mElements->Length(true);
  for (uint32_t i = 0; i < length; ++i) {
    static_cast<nsGenericHTMLFormElement*>(mElements->GetNodeAt(i))
      ->FieldSetFirstLegendChanged(aNotify);
  }
}

