




#include "mozilla/dom/HTMLFieldSetElement.h"
#include "nsEventDispatcher.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(FieldSet)
DOMCI_NODE_DATA(HTMLFieldSetElement, mozilla::dom::HTMLFieldSetElement)


namespace mozilla {
namespace dom {

HTMLFieldSetElement::HTMLFieldSetElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLFormElement(aNodeInfo)
  , mElements(nullptr)
  , mFirstLegend(nullptr)
{
  
  SetBarredFromConstraintValidation(true);

  
  AddStatesSilently(NS_EVENT_STATE_ENABLED);
}

HTMLFieldSetElement::~HTMLFieldSetElement()
{
  uint32_t length = mDependentElements.Length();
  for (uint32_t i = 0; i < length; ++i) {
    mDependentElements[i]->ForgetFieldSet(this);
  }
}



NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(HTMLFieldSetElement,
                                                nsGenericHTMLFormElement)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mValidity)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mElements)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(HTMLFieldSetElement,
                                                  nsGenericHTMLFormElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mValidity)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mElements)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(HTMLFieldSetElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLFieldSetElement, Element)


NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(HTMLFieldSetElement)
  NS_HTML_CONTENT_INTERFACE_TABLE2(HTMLFieldSetElement,
                                   nsIDOMHTMLFieldSetElement,
                                   nsIConstraintValidation)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLFieldSetElement,
                                               nsGenericHTMLFormElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLFieldSetElement)

NS_IMPL_ELEMENT_CLONE(HTMLFieldSetElement)


NS_IMPL_BOOL_ATTR(HTMLFieldSetElement, Disabled, disabled)
NS_IMPL_STRING_ATTR(HTMLFieldSetElement, Name, name)


NS_IMPL_NSICONSTRAINTVALIDATION(HTMLFieldSetElement)

bool
HTMLFieldSetElement::IsDisabledForEvents(uint32_t aMessage)
{
  return IsElementDisabledForEvents(aMessage, nullptr);
}


nsresult
HTMLFieldSetElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  
  aVisitor.mCanHandle = false;
  if (IsDisabledForEvents(aVisitor.mEvent->message)) {
    return NS_OK;
  }

  return nsGenericHTMLFormElement::PreHandleEvent(aVisitor);
}

nsresult
HTMLFieldSetElement::AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
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
      static_cast<nsGenericHTMLFormElement*>(mElements->Item(i))
        ->FieldSetDisabledChanged(aNotify);
    }
  }

  return nsGenericHTMLFormElement::AfterSetAttr(aNameSpaceID, aName,
                                                aValue, aNotify);
}



NS_IMETHODIMP
HTMLFieldSetElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElement::GetForm(aForm);
}

NS_IMETHODIMP
HTMLFieldSetElement::GetType(nsAString& aType)
{
  aType.AssignLiteral("fieldset");
  return NS_OK;
}


bool
HTMLFieldSetElement::MatchListedElements(nsIContent* aContent, int32_t aNamespaceID,
                                         nsIAtom* aAtom, void* aData)
{
  nsCOMPtr<nsIFormControl> formControl = do_QueryInterface(aContent);
  return formControl && formControl->GetType() != NS_FORM_LABEL;
}

NS_IMETHODIMP
HTMLFieldSetElement::GetElements(nsIDOMHTMLCollection** aElements)
{
  if (!mElements) {
    mElements = new nsContentList(this, MatchListedElements, nullptr, nullptr,
                                  true);
  }

  NS_ADDREF(*aElements = mElements);
  return NS_OK;
}



nsresult
HTMLFieldSetElement::Reset()
{
  return NS_OK;
}

NS_IMETHODIMP
HTMLFieldSetElement::SubmitNamesValues(nsFormSubmission* aFormSubmission)
{
  return NS_OK;
}

nsresult
HTMLFieldSetElement::InsertChildAt(nsIContent* aChild, uint32_t aIndex,
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
HTMLFieldSetElement::RemoveChildAt(uint32_t aIndex, bool aNotify)
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
HTMLFieldSetElement::NotifyElementsForFirstLegendChange(bool aNotify)
{
  





  if (!mElements) {
    mElements = new nsContentList(this, MatchListedElements, nullptr, nullptr,
                                  true);
  }

  uint32_t length = mElements->Length(true);
  for (uint32_t i = 0; i < length; ++i) {
    static_cast<nsGenericHTMLFormElement*>(mElements->Item(i))
      ->FieldSetFirstLegendChanged(aNotify);
  }
}

} 
} 
