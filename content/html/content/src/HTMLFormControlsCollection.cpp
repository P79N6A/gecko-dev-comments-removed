





#include "mozilla/dom/HTMLFormControlsCollection.h"

#include "mozFlushType.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/HTMLFormControlsCollectionBinding.h"
#include "mozilla/dom/HTMLFormElement.h"
#include "mozilla/dom/UnionTypes.h"
#include "nsGenericHTMLElement.h" 
#include "nsIDocument.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIFormControl.h"
#include "RadioNodeList.h"
#include "jsfriendapi.h"

namespace mozilla {
namespace dom {

 bool
HTMLFormControlsCollection::ShouldBeInElements(nsIFormControl* aFormControl)
{
  
  
  

  switch (aFormControl->GetType()) {
  case NS_FORM_BUTTON_BUTTON :
  case NS_FORM_BUTTON_RESET :
  case NS_FORM_BUTTON_SUBMIT :
  case NS_FORM_INPUT_BUTTON :
  case NS_FORM_INPUT_CHECKBOX :
  case NS_FORM_INPUT_COLOR :
  case NS_FORM_INPUT_EMAIL :
  case NS_FORM_INPUT_FILE :
  case NS_FORM_INPUT_HIDDEN :
  case NS_FORM_INPUT_RESET :
  case NS_FORM_INPUT_PASSWORD :
  case NS_FORM_INPUT_RADIO :
  case NS_FORM_INPUT_SEARCH :
  case NS_FORM_INPUT_SUBMIT :
  case NS_FORM_INPUT_TEXT :
  case NS_FORM_INPUT_TEL :
  case NS_FORM_INPUT_URL :
  case NS_FORM_INPUT_NUMBER :
  case NS_FORM_INPUT_RANGE :
  case NS_FORM_INPUT_DATE :
  case NS_FORM_INPUT_TIME :
  case NS_FORM_SELECT :
  case NS_FORM_TEXTAREA :
  case NS_FORM_FIELDSET :
  case NS_FORM_OBJECT :
  case NS_FORM_OUTPUT :
    return true;
  }

  
  
  
  
  

  return false;
}

HTMLFormControlsCollection::HTMLFormControlsCollection(HTMLFormElement* aForm)
  : mForm(aForm)
  
  
  , mElements(8)
  , mNameLookupTable(HTMLFormElement::FORM_CONTROL_LIST_HASHTABLE_LENGTH)
{
  SetIsDOMBinding();
}

HTMLFormControlsCollection::~HTMLFormControlsCollection()
{
  mForm = nullptr;
  Clear();
}

void
HTMLFormControlsCollection::DropFormReference()
{
  mForm = nullptr;
  Clear();
}

void
HTMLFormControlsCollection::Clear()
{
  
  for (int32_t i = mElements.Length() - 1; i >= 0; i--) {
    mElements[i]->ClearForm(false);
  }
  mElements.Clear();

  for (int32_t i = mNotInElements.Length() - 1; i >= 0; i--) {
    mNotInElements[i]->ClearForm(false);
  }
  mNotInElements.Clear();

  mNameLookupTable.Clear();
}

void
HTMLFormControlsCollection::FlushPendingNotifications()
{
  if (mForm) {
    nsIDocument* doc = mForm->GetCurrentDoc();
    if (doc) {
      doc->FlushPendingNotifications(Flush_Content);
    }
  }
}

NS_IMPL_CYCLE_COLLECTION_CLASS(HTMLFormControlsCollection)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(HTMLFormControlsCollection)
  tmp->Clear();
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(HTMLFormControlsCollection)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mNameLookupTable)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(HTMLFormControlsCollection)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END


NS_INTERFACE_TABLE_HEAD(HTMLFormControlsCollection)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_TABLE(HTMLFormControlsCollection,
                     nsIHTMLCollection,
                     nsIDOMHTMLCollection)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(HTMLFormControlsCollection)
NS_INTERFACE_MAP_END


NS_IMPL_CYCLE_COLLECTING_ADDREF(HTMLFormControlsCollection)
NS_IMPL_CYCLE_COLLECTING_RELEASE(HTMLFormControlsCollection)




NS_IMETHODIMP
HTMLFormControlsCollection::GetLength(uint32_t* aLength)
{
  FlushPendingNotifications();
  *aLength = mElements.Length();
  return NS_OK;
}

NS_IMETHODIMP
HTMLFormControlsCollection::Item(uint32_t aIndex, nsIDOMNode** aReturn)
{
  nsISupports* item = GetElementAt(aIndex);
  if (!item) {
    *aReturn = nullptr;

    return NS_OK;
  }

  return CallQueryInterface(item, aReturn);
}

NS_IMETHODIMP 
HTMLFormControlsCollection::NamedItem(const nsAString& aName,
                                      nsIDOMNode** aReturn)
{
  FlushPendingNotifications();

  *aReturn = nullptr;

  nsCOMPtr<nsISupports> supports;

  if (!mNameLookupTable.Get(aName, getter_AddRefs(supports))) {
    
    return NS_OK;
  }

  if (!supports) {
    return NS_OK;
  }

  
  CallQueryInterface(supports, aReturn);
  if (*aReturn) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIDOMNodeList> nodeList = do_QueryInterface(supports);
  NS_ASSERTION(nodeList, "Huh, what's going one here?");
  if (!nodeList) {
    return NS_OK;
  }

  
  
  return nodeList->Item(0, aReturn);
}

nsISupports*
HTMLFormControlsCollection::NamedItemInternal(const nsAString& aName,
                                              bool aFlushContent)
{
  if (aFlushContent) {
    FlushPendingNotifications();
  }

  return mNameLookupTable.GetWeak(aName);
}

nsresult
HTMLFormControlsCollection::AddElementToTable(nsGenericHTMLFormElement* aChild,
                                              const nsAString& aName)
{
  if (!ShouldBeInElements(aChild)) {
    return NS_OK;
  }

  return mForm->AddElementToTableInternal(mNameLookupTable, aChild, aName);
}

nsresult
HTMLFormControlsCollection::IndexOfControl(nsIFormControl* aControl,
                                           int32_t* aIndex)
{
  
  
  NS_ENSURE_ARG_POINTER(aIndex);

  *aIndex = mElements.IndexOf(aControl);

  return NS_OK;
}

nsresult
HTMLFormControlsCollection::RemoveElementFromTable(
  nsGenericHTMLFormElement* aChild, const nsAString& aName)
{
  if (!ShouldBeInElements(aChild)) {
    return NS_OK;
  }

  return mForm->RemoveElementFromTableInternal(mNameLookupTable, aChild, aName);
}

nsresult
HTMLFormControlsCollection::GetSortedControls(
  nsTArray<nsGenericHTMLFormElement*>& aControls) const
{
#ifdef DEBUG
  HTMLFormElement::AssertDocumentOrder(mElements, mForm);
  HTMLFormElement::AssertDocumentOrder(mNotInElements, mForm);
#endif

  aControls.Clear();

  
  
  uint32_t elementsLen = mElements.Length();
  uint32_t notInElementsLen = mNotInElements.Length();
  aControls.SetCapacity(elementsLen + notInElementsLen);

  uint32_t elementsIdx = 0;
  uint32_t notInElementsIdx = 0;

  while (elementsIdx < elementsLen || notInElementsIdx < notInElementsLen) {
    
    if (elementsIdx == elementsLen) {
      NS_ASSERTION(notInElementsIdx < notInElementsLen,
                   "Should have remaining not-in-elements");
      
      if (!aControls.AppendElements(mNotInElements.Elements() +
                                      notInElementsIdx,
                                    notInElementsLen -
                                      notInElementsIdx)) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      break;
    }
    
    if (notInElementsIdx == notInElementsLen) {
      NS_ASSERTION(elementsIdx < elementsLen,
                   "Should have remaining in-elements");
      
      if (!aControls.AppendElements(mElements.Elements() +
                                      elementsIdx,
                                    elementsLen -
                                      elementsIdx)) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      break;
    }
    
    NS_ASSERTION(mElements[elementsIdx] &&
                 mNotInElements[notInElementsIdx],
                 "Should have remaining elements");
    
    
    nsGenericHTMLFormElement* elementToAdd;
    if (HTMLFormElement::CompareFormControlPosition(
          mElements[elementsIdx], mNotInElements[notInElementsIdx], mForm) < 0) {
      elementToAdd = mElements[elementsIdx];
      ++elementsIdx;
    } else {
      elementToAdd = mNotInElements[notInElementsIdx];
      ++notInElementsIdx;
    }
    
    if (!aControls.AppendElement(elementToAdd)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  NS_ASSERTION(aControls.Length() == elementsLen + notInElementsLen,
               "Not all form controls were added to the sorted list");
#ifdef DEBUG
  HTMLFormElement::AssertDocumentOrder(aControls, mForm);
#endif

  return NS_OK;
}

Element*
HTMLFormControlsCollection::GetElementAt(uint32_t aIndex)
{
  FlushPendingNotifications();

  return mElements.SafeElementAt(aIndex, nullptr);
}

 nsINode*
HTMLFormControlsCollection::GetParentObject()
{
  return mForm;
}

 Element*
HTMLFormControlsCollection::GetFirstNamedElement(const nsAString& aName, bool& aFound)
{
  Nullable<OwningRadioNodeListOrElement> maybeResult;
  NamedGetter(aName, aFound, maybeResult);
  if (!aFound) {
    return nullptr;
  }
  MOZ_ASSERT(!maybeResult.IsNull());
  const OwningRadioNodeListOrElement& result = maybeResult.Value();
  if (result.IsElement()) {
    return result.GetAsElement().get();
  }
  if (result.IsRadioNodeList()) {
    RadioNodeList& nodelist = result.GetAsRadioNodeList();
    return nodelist.Item(0)->AsElement();
  }
  MOZ_ASSERT_UNREACHABLE("Should only have Elements and NodeLists here.");
  return nullptr;
}

void
HTMLFormControlsCollection::NamedGetter(const nsAString& aName,
                                        bool& aFound,
                                        Nullable<OwningRadioNodeListOrElement>& aResult)
{
  nsISupports* item = NamedItemInternal(aName, true);
  if (!item) {
    aFound = false;
    return;
  }
  aFound = true;
  if (nsCOMPtr<Element> element = do_QueryInterface(item)) {
    aResult.SetValue().SetAsElement() = element;
    return;
  }
  if (nsCOMPtr<RadioNodeList> nodelist = do_QueryInterface(item)) {
    aResult.SetValue().SetAsRadioNodeList() = nodelist;
    return;
  }
  MOZ_ASSERT_UNREACHABLE("Should only have Elements and NodeLists here.");
}

static PLDHashOperator
CollectNames(const nsAString& aName,
             nsISupports* ,
             void* aClosure)
{
  static_cast<nsTArray<nsString>*>(aClosure)->AppendElement(aName);
  return PL_DHASH_NEXT;
}

void
HTMLFormControlsCollection::GetSupportedNames(unsigned aFlags,
                                              nsTArray<nsString>& aNames)
{
  if (!(aFlags & JSITER_HIDDEN)) {
    return;
  }

  FlushPendingNotifications();
  
  
  
  mNameLookupTable.EnumerateRead(CollectNames, &aNames);
}

 JSObject*
HTMLFormControlsCollection::WrapObject(JSContext* aCx)
{
  return HTMLFormControlsCollectionBinding::Wrap(aCx, this);
}

} 
} 
