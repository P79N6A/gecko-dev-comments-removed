




#include "mozilla/dom/HTMLSelectElement.h"

#include "mozAutoDocUpdate.h"
#include "mozilla/Attributes.h"
#include "mozilla/BasicEvents.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/EventStates.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/HTMLOptGroupElement.h"
#include "mozilla/dom/HTMLOptionElement.h"
#include "mozilla/dom/HTMLSelectElementBinding.h"
#include "mozilla/dom/UnionTypes.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentList.h"
#include "nsError.h"
#include "nsFormSubmission.h"
#include "nsGkAtoms.h"
#include "nsIComboboxControlFrame.h"
#include "nsIDocument.h"
#include "nsIFormControlFrame.h"
#include "nsIForm.h"
#include "nsIFormProcessor.h"
#include "nsIFrame.h"
#include "nsIListControlFrame.h"
#include "nsISelectControlFrame.h"
#include "nsLayoutUtils.h"
#include "nsMappedAttributes.h"
#include "nsPresState.h"
#include "nsRuleData.h"
#include "nsServiceManagerUtils.h"
#include "nsStyleConsts.h"
#include "nsTextNode.h"

NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(Select)

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS(SelectState, SelectState)






SafeOptionListMutation::SafeOptionListMutation(nsIContent* aSelect,
                                               nsIContent* aParent,
                                               nsIContent* aKid,
                                               uint32_t aIndex,
                                               bool aNotify)
  : mSelect(HTMLSelectElement::FromContentOrNull(aSelect))
  , mTopLevelMutation(false)
  , mNeedsRebuild(false)
{
  if (mSelect) {
    mTopLevelMutation = !mSelect->mMutating;
    if (mTopLevelMutation) {
      mSelect->mMutating = true;
    } else {
      
      
      
      
      mSelect->RebuildOptionsArray(aNotify);
    }
    nsresult rv;
    if (aKid) {
      rv = mSelect->WillAddOptions(aKid, aParent, aIndex, aNotify);
    } else {
      rv = mSelect->WillRemoveOptions(aParent, aIndex, aNotify);
    }
    mNeedsRebuild = NS_FAILED(rv);
  }
}

SafeOptionListMutation::~SafeOptionListMutation()
{
  if (mSelect) {
    if (mNeedsRebuild || (mTopLevelMutation && mGuard.Mutated(1))) {
      mSelect->RebuildOptionsArray(true);
    }
    if (mTopLevelMutation) {
      mSelect->mMutating = false;
    }
#ifdef DEBUG
    mSelect->VerifyOptionsArray();
#endif
  }
}









HTMLSelectElement::HTMLSelectElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo,
                                     FromParser aFromParser)
  : nsGenericHTMLFormElementWithState(aNodeInfo),
    mOptions(new HTMLOptionsCollection(this)),
    mAutocompleteAttrState(nsContentUtils::eAutocompleteAttrState_Unknown),
    mIsDoneAddingChildren(!aFromParser),
    mDisabledChanged(false),
    mMutating(false),
    mInhibitStateRestoration(!!(aFromParser & FROM_PARSER_FRAGMENT)),
    mSelectionHasChanged(false),
    mDefaultSelectionSet(false),
    mCanShowInvalidUI(true),
    mCanShowValidUI(true),
    mNonOptionChildren(0),
    mOptGroupCount(0),
    mSelectedIndex(-1)
{
  SetHasWeirdParserInsertionMode();

  
  

  
  AddStatesSilently(NS_EVENT_STATE_ENABLED |
                    NS_EVENT_STATE_OPTIONAL |
                    NS_EVENT_STATE_VALID);
}

HTMLSelectElement::~HTMLSelectElement()
{
  mOptions->DropReference();
}



NS_IMPL_CYCLE_COLLECTION_CLASS(HTMLSelectElement)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(HTMLSelectElement,
                                                  nsGenericHTMLFormElementWithState)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mValidity)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOptions)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mSelectedOptions)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(HTMLSelectElement,
                                                nsGenericHTMLFormElementWithState)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mValidity)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mSelectedOptions)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(HTMLSelectElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLSelectElement, Element)


NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(HTMLSelectElement)
  NS_INTERFACE_TABLE_INHERITED(HTMLSelectElement,
                               nsIDOMHTMLSelectElement,
                               nsIConstraintValidation)
NS_INTERFACE_TABLE_TAIL_INHERITING(nsGenericHTMLFormElementWithState)





NS_IMPL_ELEMENT_CLONE(HTMLSelectElement)


NS_IMPL_NSICONSTRAINTVALIDATION_EXCEPT_SETCUSTOMVALIDITY(HTMLSelectElement)

NS_IMETHODIMP
HTMLSelectElement::SetCustomValidity(const nsAString& aError)
{
  nsIConstraintValidation::SetCustomValidity(aError);

  UpdateState(true);

  return NS_OK;
}

void
HTMLSelectElement::GetAutocomplete(DOMString& aValue)
{
  const nsAttrValue* attributeVal = GetParsedAttr(nsGkAtoms::autocomplete);

  mAutocompleteAttrState =
    nsContentUtils::SerializeAutocompleteAttribute(attributeVal, aValue,
                                                   mAutocompleteAttrState);
}

NS_IMETHODIMP
HTMLSelectElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElementWithState::GetForm(aForm);
}

nsresult
HTMLSelectElement::InsertChildAt(nsIContent* aKid,
                                 uint32_t aIndex,
                                 bool aNotify)
{
  SafeOptionListMutation safeMutation(this, this, aKid, aIndex, aNotify);
  nsresult rv = nsGenericHTMLFormElementWithState::InsertChildAt(aKid, aIndex,
                                                                 aNotify);
  if (NS_FAILED(rv)) {
    safeMutation.MutationFailed();
  }
  return rv;
}

void
HTMLSelectElement::RemoveChildAt(uint32_t aIndex, bool aNotify)
{
  SafeOptionListMutation safeMutation(this, this, nullptr, aIndex, aNotify);
  nsGenericHTMLFormElementWithState::RemoveChildAt(aIndex, aNotify);
}



void
HTMLSelectElement::InsertOptionsIntoList(nsIContent* aOptions,
                                         int32_t aListIndex,
                                         int32_t aDepth,
                                         bool aNotify)
{
  int32_t insertIndex = aListIndex;
  InsertOptionsIntoListRecurse(aOptions, &insertIndex, aDepth);

  
  if (insertIndex - aListIndex) {
    
    if (aListIndex <= mSelectedIndex) {
      mSelectedIndex += (insertIndex - aListIndex);
      SetSelectionChanged(true, aNotify);
    }

    
    
    
    nsISelectControlFrame* selectFrame = nullptr;
    nsWeakFrame weakSelectFrame;
    bool didGetFrame = false;

    
    for (int32_t i = aListIndex; i < insertIndex; i++) {
      
      if (!didGetFrame || (selectFrame && !weakSelectFrame.IsAlive())) {
        selectFrame = GetSelectFrame();
        weakSelectFrame = do_QueryFrame(selectFrame);
        didGetFrame = true;
      }

      if (selectFrame) {
        selectFrame->AddOption(i);
      }

      nsRefPtr<HTMLOptionElement> option = Item(i);
      if (option && option->Selected()) {
        
        if (!HasAttr(kNameSpaceID_None, nsGkAtoms::multiple)) {
          uint32_t mask = IS_SELECTED | CLEAR_ALL | SET_DISABLED | NOTIFY;
          SetOptionsSelectedByIndex(i, i, mask);
        }

        
        
        
        OnOptionSelected(selectFrame, i, true, false, false);
      }
    }

    CheckSelectSomething(aNotify);
  }
}

nsresult
HTMLSelectElement::RemoveOptionsFromList(nsIContent* aOptions,
                                         int32_t aListIndex,
                                         int32_t aDepth,
                                         bool aNotify)
{
  int32_t numRemoved = 0;
  nsresult rv = RemoveOptionsFromListRecurse(aOptions, aListIndex, &numRemoved,
                                             aDepth);
  NS_ENSURE_SUCCESS(rv, rv);

  if (numRemoved) {
    
    nsISelectControlFrame* selectFrame = GetSelectFrame();
    if (selectFrame) {
      nsAutoScriptBlocker scriptBlocker;
      for (int32_t i = aListIndex; i < aListIndex + numRemoved; ++i) {
        selectFrame->RemoveOption(i);
      }
    }

    
    if (aListIndex <= mSelectedIndex) {
      if (mSelectedIndex < (aListIndex+numRemoved)) {
        
        
        FindSelectedIndex(aListIndex, aNotify);
      } else {
        
        
        mSelectedIndex -= numRemoved;
        SetSelectionChanged(true, aNotify);
      }
    }

    
    
    if (!CheckSelectSomething(aNotify) && mSelectedIndex == -1) {
      
      
      UpdateValueMissingValidityState();

      UpdateState(aNotify);
    }
  }

  return NS_OK;
}




void
HTMLSelectElement::InsertOptionsIntoListRecurse(nsIContent* aOptions,
                                                int32_t* aInsertIndex,
                                                int32_t aDepth)
{
  
  
  
  

  HTMLOptionElement* optElement = HTMLOptionElement::FromContent(aOptions);
  if (optElement) {
    mOptions->InsertOptionAt(optElement, *aInsertIndex);
    (*aInsertIndex)++;
    return;
  }

  
  
  if (aDepth == 0) {
    mNonOptionChildren++;
  }

  
  if (aOptions->IsHTMLElement(nsGkAtoms::optgroup)) {
    mOptGroupCount++;

    for (nsIContent* child = aOptions->GetFirstChild();
         child;
         child = child->GetNextSibling()) {
      InsertOptionsIntoListRecurse(child, aInsertIndex, aDepth + 1);
    }
  }
}



nsresult
HTMLSelectElement::RemoveOptionsFromListRecurse(nsIContent* aOptions,
                                                int32_t aRemoveIndex,
                                                int32_t* aNumRemoved,
                                                int32_t aDepth)
{
  
  
  
  

  nsCOMPtr<nsIDOMHTMLOptionElement> optElement(do_QueryInterface(aOptions));
  if (optElement) {
    if (mOptions->ItemAsOption(aRemoveIndex) != optElement) {
      NS_ERROR("wrong option at index");
      return NS_ERROR_UNEXPECTED;
    }
    mOptions->RemoveOptionAt(aRemoveIndex);
    (*aNumRemoved)++;
    return NS_OK;
  }

  
  if (aDepth == 0) {
    mNonOptionChildren--;
  }

  
  if (mOptGroupCount && aOptions->IsHTMLElement(nsGkAtoms::optgroup)) {
    mOptGroupCount--;

    for (nsIContent* child = aOptions->GetFirstChild();
         child;
         child = child->GetNextSibling()) {
      nsresult rv = RemoveOptionsFromListRecurse(child,
                                                 aRemoveIndex,
                                                 aNumRemoved,
                                                 aDepth + 1);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}






NS_IMETHODIMP
HTMLSelectElement::WillAddOptions(nsIContent* aOptions,
                                  nsIContent* aParent,
                                  int32_t aContentIndex,
                                  bool aNotify)
{
  int32_t level = GetContentDepth(aParent);
  if (level == -1) {
    return NS_ERROR_FAILURE;
  }

  
  int32_t ind = -1;
  if (!mNonOptionChildren) {
    
    ind = aContentIndex;
  } else {
    
    
    int32_t children = aParent->GetChildCount();

    if (aContentIndex >= children) {
      
      
      ind = GetOptionIndexAfter(aParent);
    } else {
      
      
      
      nsIContent* currentKid = aParent->GetChildAt(aContentIndex);
      NS_ASSERTION(currentKid, "Child not found!");
      if (currentKid) {
        ind = GetOptionIndexAt(currentKid);
      } else {
        ind = -1;
      }
    }
  }

  InsertOptionsIntoList(aOptions, ind, level, aNotify);
  return NS_OK;
}

NS_IMETHODIMP
HTMLSelectElement::WillRemoveOptions(nsIContent* aParent,
                                     int32_t aContentIndex,
                                     bool aNotify)
{
  int32_t level = GetContentDepth(aParent);
  NS_ASSERTION(level >= 0, "getting notified by unexpected content");
  if (level == -1) {
    return NS_ERROR_FAILURE;
  }

  
  nsIContent* currentKid = aParent->GetChildAt(aContentIndex);
  if (currentKid) {
    int32_t ind;
    if (!mNonOptionChildren) {
      
      ind = aContentIndex;
    } else {
      
      
      ind = GetFirstOptionIndex(currentKid);
    }
    if (ind != -1) {
      nsresult rv = RemoveOptionsFromList(currentKid, ind, level, aNotify);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

int32_t
HTMLSelectElement::GetContentDepth(nsIContent* aContent)
{
  nsIContent* content = aContent;

  int32_t retval = 0;
  while (content != this) {
    retval++;
    content = content->GetParent();
    if (!content) {
      retval = -1;
      break;
    }
  }

  return retval;
}

int32_t
HTMLSelectElement::GetOptionIndexAt(nsIContent* aOptions)
{
  
  
  int32_t retval = GetFirstOptionIndex(aOptions);
  if (retval == -1) {
    retval = GetOptionIndexAfter(aOptions);
  }

  return retval;
}

int32_t
HTMLSelectElement::GetOptionIndexAfter(nsIContent* aOptions)
{
  
  
  
  
  if (aOptions == this) {
    uint32_t len;
    GetLength(&len);
    return len;
  }

  int32_t retval = -1;

  nsCOMPtr<nsIContent> parent = aOptions->GetParent();

  if (parent) {
    int32_t index = parent->IndexOf(aOptions);
    int32_t count = parent->GetChildCount();

    retval = GetFirstChildOptionIndex(parent, index+1, count);

    if (retval == -1) {
      retval = GetOptionIndexAfter(parent);
    }
  }

  return retval;
}

int32_t
HTMLSelectElement::GetFirstOptionIndex(nsIContent* aOptions)
{
  int32_t listIndex = -1;
  HTMLOptionElement* optElement = HTMLOptionElement::FromContent(aOptions);
  if (optElement) {
    GetOptionIndex(optElement, 0, true, &listIndex);
    
    
    return listIndex;
  }

  listIndex = GetFirstChildOptionIndex(aOptions, 0, aOptions->GetChildCount());

  return listIndex;
}

int32_t
HTMLSelectElement::GetFirstChildOptionIndex(nsIContent* aOptions,
                                            int32_t aStartIndex,
                                            int32_t aEndIndex)
{
  int32_t retval = -1;

  for (int32_t i = aStartIndex; i < aEndIndex; ++i) {
    retval = GetFirstOptionIndex(aOptions->GetChildAt(i));
    if (retval != -1) {
      break;
    }
  }

  return retval;
}

nsISelectControlFrame*
HTMLSelectElement::GetSelectFrame()
{
  nsIFormControlFrame* form_control_frame = GetFormControlFrame(false);

  nsISelectControlFrame* select_frame = nullptr;

  if (form_control_frame) {
    select_frame = do_QueryFrame(form_control_frame);
  }

  return select_frame;
}

void
HTMLSelectElement::Add(const HTMLOptionElementOrHTMLOptGroupElement& aElement,
                       const Nullable<HTMLElementOrLong>& aBefore,
                       ErrorResult& aRv)
{
  nsGenericHTMLElement& element =
    aElement.IsHTMLOptionElement() ?
    static_cast<nsGenericHTMLElement&>(aElement.GetAsHTMLOptionElement()) :
    static_cast<nsGenericHTMLElement&>(aElement.GetAsHTMLOptGroupElement());

  if (aBefore.IsNull()) {
    Add(element, static_cast<nsGenericHTMLElement*>(nullptr), aRv);
  } else if (aBefore.Value().IsHTMLElement()) {
    Add(element, &aBefore.Value().GetAsHTMLElement(), aRv);
  } else {
    Add(element, aBefore.Value().GetAsLong(), aRv);
  }
}

void
HTMLSelectElement::Add(nsGenericHTMLElement& aElement,
                       nsGenericHTMLElement* aBefore,
                       ErrorResult& aError)
{
  if (!aBefore) {
    Element::AppendChild(aElement, aError);
    return;
  }

  
  
  nsINode* parent = aBefore->Element::GetParentNode();
  if (!parent || !nsContentUtils::ContentIsDescendantOf(parent, this)) {
    
    
    aError.Throw(NS_ERROR_DOM_NOT_FOUND_ERR);
    return;
  }

  
  
  parent->InsertBefore(aElement, aBefore, aError);
}

NS_IMETHODIMP
HTMLSelectElement::Add(nsIDOMHTMLElement* aElement,
                       nsIVariant* aBefore)
{
  uint16_t dataType;
  nsresult rv = aBefore->GetDataType(&dataType);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContent> element = do_QueryInterface(aElement);
  nsGenericHTMLElement* htmlElement =
    nsGenericHTMLElement::FromContentOrNull(element);
  if (!htmlElement) {
    return NS_ERROR_NULL_POINTER;
  }

  
  if (dataType == nsIDataType::VTYPE_EMPTY ||
      dataType == nsIDataType::VTYPE_VOID) {
    ErrorResult error;
    Add(*htmlElement, (nsGenericHTMLElement*)nullptr, error);
    return error.StealNSResult();
  }

  nsCOMPtr<nsISupports> supports;
  nsCOMPtr<nsIDOMHTMLElement> beforeElement;

  
  if (NS_SUCCEEDED(aBefore->GetAsISupports(getter_AddRefs(supports)))) {
    nsCOMPtr<nsIContent> beforeElement = do_QueryInterface(supports);
    nsGenericHTMLElement* beforeHTMLElement =
      nsGenericHTMLElement::FromContentOrNull(beforeElement);

    NS_ENSURE_TRUE(beforeHTMLElement, NS_ERROR_DOM_SYNTAX_ERR);

    ErrorResult error;
    Add(*htmlElement, beforeHTMLElement, error);
    return error.StealNSResult();
  }

  
  int32_t index;
  NS_ENSURE_SUCCESS(aBefore->GetAsInt32(&index), NS_ERROR_DOM_SYNTAX_ERR);

  ErrorResult error;
  Add(*htmlElement, index, error);
  return error.StealNSResult();
}

NS_IMETHODIMP
HTMLSelectElement::Remove(int32_t aIndex)
{
  nsCOMPtr<nsINode> option = Item(static_cast<uint32_t>(aIndex));
  if (!option) {
    return NS_OK;
  }

  option->Remove();
  return NS_OK;
}

NS_IMETHODIMP
HTMLSelectElement::GetOptions(nsIDOMHTMLOptionsCollection** aValue)
{
  NS_IF_ADDREF(*aValue = GetOptions());

  return NS_OK;
}

NS_IMETHODIMP
HTMLSelectElement::GetType(nsAString& aType)
{
  if (HasAttr(kNameSpaceID_None, nsGkAtoms::multiple)) {
    aType.AssignLiteral("select-multiple");
  }
  else {
    aType.AssignLiteral("select-one");
  }

  return NS_OK;
}

NS_IMETHODIMP
HTMLSelectElement::GetLength(uint32_t* aLength)
{
  return mOptions->GetLength(aLength);
}

#define MAX_DYNAMIC_SELECT_LENGTH 10000

NS_IMETHODIMP
HTMLSelectElement::SetLength(uint32_t aLength)
{
  ErrorResult rv;
  SetLength(aLength, rv);
  return rv.StealNSResult();
}

void
HTMLSelectElement::SetLength(uint32_t aLength, ErrorResult& aRv)
{
  uint32_t curlen = Length();

  if (curlen > aLength) { 
    for (uint32_t i = curlen; i > aLength; --i) {
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(Remove(i - 1)));
    }
  } else if (aLength > curlen) {
    if (aLength > MAX_DYNAMIC_SELECT_LENGTH) {
      aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
      return;
    }

    nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;

    nsContentUtils::NameChanged(mNodeInfo, nsGkAtoms::option,
                                getter_AddRefs(nodeInfo));

    nsCOMPtr<nsINode> node = NS_NewHTMLOptionElement(nodeInfo.forget());

    nsRefPtr<nsTextNode> text = new nsTextNode(mNodeInfo->NodeInfoManager());

    aRv = node->AppendChildTo(text, false);
    if (aRv.Failed()) {
      return;
    }

    for (uint32_t i = curlen; i < aLength; i++) {
      nsINode::AppendChild(*node, aRv);
      if (aRv.Failed()) {
        return;
      }

      if (i + 1 < aLength) {
        node = node->CloneNode(true, aRv);
        if (aRv.Failed()) {
          return;
        }
        MOZ_ASSERT(node);
      }
    }
  }
}


bool
HTMLSelectElement::MatchSelectedOptions(nsIContent* aContent,
                                        int32_t ,
                                        nsIAtom* ,
                                        void* )
{
  HTMLOptionElement* option = HTMLOptionElement::FromContent(aContent);
  return option && option->Selected();
}

nsIHTMLCollection*
HTMLSelectElement::SelectedOptions()
{
  if (!mSelectedOptions) {
    mSelectedOptions = new nsContentList(this, MatchSelectedOptions, nullptr,
                                         nullptr,  true);
  }
  return mSelectedOptions;
}

NS_IMETHODIMP
HTMLSelectElement::GetSelectedOptions(nsIDOMHTMLCollection** aSelectedOptions)
{
  NS_ADDREF(*aSelectedOptions = SelectedOptions());
  return NS_OK;
}



NS_IMETHODIMP
HTMLSelectElement::GetSelectedIndex(int32_t* aValue)
{
  *aValue = SelectedIndex();

  return NS_OK;
}

nsresult
HTMLSelectElement::SetSelectedIndexInternal(int32_t aIndex, bool aNotify)
{
  int32_t oldSelectedIndex = mSelectedIndex;
  uint32_t mask = IS_SELECTED | CLEAR_ALL | SET_DISABLED;
  if (aNotify) {
    mask |= NOTIFY;
  }

  SetOptionsSelectedByIndex(aIndex, aIndex, mask);

  nsresult rv = NS_OK;
  nsISelectControlFrame* selectFrame = GetSelectFrame();
  if (selectFrame) {
    rv = selectFrame->OnSetSelectedIndex(oldSelectedIndex, mSelectedIndex);
  }

  SetSelectionChanged(true, aNotify);

  return rv;
}

NS_IMETHODIMP
HTMLSelectElement::SetSelectedIndex(int32_t aIndex)
{
  return SetSelectedIndexInternal(aIndex, true);
}

NS_IMETHODIMP
HTMLSelectElement::GetOptionIndex(nsIDOMHTMLOptionElement* aOption,
                                  int32_t aStartIndex, bool aForward,
                                  int32_t* aIndex)
{
  nsCOMPtr<nsINode> option = do_QueryInterface(aOption);
  return mOptions->GetOptionIndex(option->AsElement(), aStartIndex, aForward, aIndex);
}

bool
HTMLSelectElement::IsOptionSelectedByIndex(int32_t aIndex)
{
  HTMLOptionElement* option = Item(static_cast<uint32_t>(aIndex));
  return option && option->Selected();
}

void
HTMLSelectElement::OnOptionSelected(nsISelectControlFrame* aSelectFrame,
                                    int32_t aIndex,
                                    bool aSelected,
                                    bool aChangeOptionState,
                                    bool aNotify)
{
  
  if (aSelected && (aIndex < mSelectedIndex || mSelectedIndex < 0)) {
    mSelectedIndex = aIndex;
    SetSelectionChanged(true, aNotify);
  } else if (!aSelected && aIndex == mSelectedIndex) {
    FindSelectedIndex(aIndex + 1, aNotify);
  }

  if (aChangeOptionState) {
    
    nsRefPtr<HTMLOptionElement> option = Item(static_cast<uint32_t>(aIndex));
    if (option) {
      option->SetSelectedInternal(aSelected, aNotify);
    }
  }

  
  if (aSelectFrame) {
    aSelectFrame->OnOptionSelected(aIndex, aSelected);
  }

  UpdateSelectedOptions();
  UpdateValueMissingValidityState();
  UpdateState(aNotify);
}

void
HTMLSelectElement::FindSelectedIndex(int32_t aStartIndex, bool aNotify)
{
  mSelectedIndex = -1;
  SetSelectionChanged(true, aNotify);
  uint32_t len = Length();
  for (int32_t i = aStartIndex; i < int32_t(len); i++) {
    if (IsOptionSelectedByIndex(i)) {
      mSelectedIndex = i;
      SetSelectionChanged(true, aNotify);
      break;
    }
  }
}

























bool
HTMLSelectElement::SetOptionsSelectedByIndex(int32_t aStartIndex,
                                             int32_t aEndIndex,
                                             uint32_t aOptionsMask)
{
#if 0
  printf("SetOption(%d-%d, %c, ClearAll=%c)\n", aStartIndex, aEndIndex,
                                      (aOptionsMask & IS_SELECTED ? 'Y' : 'N'),
                                      (aOptionsMask & CLEAR_ALL ? 'Y' : 'N'));
#endif
  
  if (!(aOptionsMask & SET_DISABLED) && IsDisabled()) {
    return false;
  }

  
  uint32_t numItems = Length();
  if (numItems == 0) {
    return false;
  }

  
  bool isMultiple = Multiple();

  
  
  bool optionsSelected = false;
  bool optionsDeselected = false;

  nsISelectControlFrame* selectFrame = nullptr;
  bool didGetFrame = false;
  nsWeakFrame weakSelectFrame;

  if (aOptionsMask & IS_SELECTED) {
    
    if (aStartIndex < 0 || AssertedCast<uint32_t>(aStartIndex) >= numItems ||
        aEndIndex < 0 || AssertedCast<uint32_t>(aEndIndex) >= numItems) {
      aStartIndex = -1;
      aEndIndex = -1;
    }

    
    if (!isMultiple) {
      aEndIndex = aStartIndex;
    }

    
    
    
    
    bool allDisabled = !(aOptionsMask & SET_DISABLED);

    
    
    
    int32_t previousSelectedIndex = mSelectedIndex;

    
    
    
    
    if (aStartIndex != -1) {
      MOZ_ASSERT(aStartIndex >= 0);
      MOZ_ASSERT(aEndIndex >= 0);
      
      
      for (uint32_t optIndex = AssertedCast<uint32_t>(aStartIndex);
           optIndex <= AssertedCast<uint32_t>(aEndIndex);
           optIndex++) {
        nsRefPtr<HTMLOptionElement> option = Item(optIndex);

        
        if (!(aOptionsMask & SET_DISABLED)) {
          if (option && IsOptionDisabled(option)) {
            continue;
          }
          allDisabled = false;
        }

        
        if (option && !option->Selected()) {
          
          
          
          
          selectFrame = GetSelectFrame();
          weakSelectFrame = do_QueryFrame(selectFrame);
          didGetFrame = true;

          OnOptionSelected(selectFrame, optIndex, true, true,
                           aOptionsMask & NOTIFY);
          optionsSelected = true;
        }
      }
    }

    
    
    if (((!isMultiple && optionsSelected)
       || ((aOptionsMask & CLEAR_ALL) && !allDisabled)
       || aStartIndex == -1)
       && previousSelectedIndex != -1) {
      for (uint32_t optIndex = AssertedCast<uint32_t>(previousSelectedIndex);
           optIndex < numItems;
           optIndex++) {
        if (static_cast<int32_t>(optIndex) < aStartIndex ||
            static_cast<int32_t>(optIndex) > aEndIndex) {
          HTMLOptionElement* option = Item(optIndex);
          
          if (option && option->Selected()) {
            if (!didGetFrame || (selectFrame && !weakSelectFrame.IsAlive())) {
              
              
              
              selectFrame = GetSelectFrame();
              weakSelectFrame = do_QueryFrame(selectFrame);

              didGetFrame = true;
            }

            OnOptionSelected(selectFrame, optIndex, false, true,
                             aOptionsMask & NOTIFY);
            optionsDeselected = true;

            
            if (!isMultiple) {
              break;
            }
          }
        }
      }
    }
  } else {
    
    
    for (int32_t optIndex = aStartIndex; optIndex <= aEndIndex; optIndex++) {
      HTMLOptionElement* option = Item(optIndex);
      if (!(aOptionsMask & SET_DISABLED) && IsOptionDisabled(option)) {
        continue;
      }

      
      if (option && option->Selected()) {
        if (!didGetFrame || (selectFrame && !weakSelectFrame.IsAlive())) {
          
          
          
          selectFrame = GetSelectFrame();
          weakSelectFrame = do_QueryFrame(selectFrame);

          didGetFrame = true;
        }

        OnOptionSelected(selectFrame, optIndex, false, true,
                         aOptionsMask & NOTIFY);
        optionsDeselected = true;
      }
    }
  }

  
  if (optionsDeselected && aStartIndex != -1) {
    optionsSelected =
      CheckSelectSomething(aOptionsMask & NOTIFY) || optionsSelected;
  }

  
  return optionsSelected || optionsDeselected;
}

NS_IMETHODIMP
HTMLSelectElement::IsOptionDisabled(int32_t aIndex, bool* aIsDisabled)
{
  *aIsDisabled = false;
  nsRefPtr<HTMLOptionElement> option = Item(aIndex);
  NS_ENSURE_TRUE(option, NS_ERROR_FAILURE);

  *aIsDisabled = IsOptionDisabled(option);
  return NS_OK;
}

bool
HTMLSelectElement::IsOptionDisabled(HTMLOptionElement* aOption)
{
  MOZ_ASSERT(aOption);
  if (aOption->Disabled()) {
    return true;
  }

  
  
  if (mNonOptionChildren) {
    for (nsCOMPtr<Element> node = static_cast<nsINode*>(aOption)->GetParentElement();
         node;
         node = node->GetParentElement()) {
      
      if (node->IsHTMLElement(nsGkAtoms::select)) {
        return false;
      }

      nsRefPtr<HTMLOptGroupElement> optGroupElement =
        HTMLOptGroupElement::FromContent(node);

      if (!optGroupElement) {
        
        
        return false;
      }

      if (optGroupElement->Disabled()) {
        return true;
      }
    }
  }

  return false;
}

NS_IMETHODIMP
HTMLSelectElement::GetValue(nsAString& aValue)
{
  DOMString value;
  GetValue(value);
  value.ToString(aValue);
  return NS_OK;
}

void
HTMLSelectElement::GetValue(DOMString& aValue)
{
  int32_t selectedIndex = SelectedIndex();
  if (selectedIndex < 0) {
    return;
  }

  nsRefPtr<HTMLOptionElement> option =
    Item(static_cast<uint32_t>(selectedIndex));

  if (!option) {
    return;
  }

  DebugOnly<nsresult> rv = option->GetValue(aValue);
  MOZ_ASSERT(NS_SUCCEEDED(rv));
}

NS_IMETHODIMP
HTMLSelectElement::SetValue(const nsAString& aValue)
{
  uint32_t length = Length();

  for (uint32_t i = 0; i < length; i++) {
    nsRefPtr<HTMLOptionElement> option = Item(i);
    if (!option) {
      continue;
    }

    nsAutoString optionVal;
    option->GetValue(optionVal);
    if (optionVal.Equals(aValue)) {
      SetSelectedIndexInternal(int32_t(i), true);
      break;
    }
  }
  return NS_OK;
}


NS_IMPL_BOOL_ATTR(HTMLSelectElement, Autofocus, autofocus)
NS_IMPL_BOOL_ATTR(HTMLSelectElement, Disabled, disabled)
NS_IMPL_BOOL_ATTR(HTMLSelectElement, Multiple, multiple)
NS_IMPL_STRING_ATTR(HTMLSelectElement, Name, name)
NS_IMPL_BOOL_ATTR(HTMLSelectElement, Required, required)
NS_IMPL_UINT_ATTR(HTMLSelectElement, Size, size)

int32_t
HTMLSelectElement::TabIndexDefault()
{
  return 0;
}

bool
HTMLSelectElement::IsHTMLFocusable(bool aWithMouse,
                                   bool* aIsFocusable, int32_t* aTabIndex)
{
  if (nsGenericHTMLFormElementWithState::IsHTMLFocusable(aWithMouse, aIsFocusable,
      aTabIndex))
  {
    return true;
  }

  *aIsFocusable = !IsDisabled();

  return false;
}

NS_IMETHODIMP
HTMLSelectElement::Item(uint32_t aIndex, nsIDOMNode** aReturn)
{
  return mOptions->Item(aIndex, aReturn);
}

NS_IMETHODIMP
HTMLSelectElement::NamedItem(const nsAString& aName, nsIDOMNode** aReturn)
{
  return mOptions->NamedItem(aName, aReturn);
}

bool
HTMLSelectElement::CheckSelectSomething(bool aNotify)
{
  if (mIsDoneAddingChildren) {
    if (mSelectedIndex < 0 && IsCombobox()) {
      return SelectSomething(aNotify);
    }
  }
  return false;
}

bool
HTMLSelectElement::SelectSomething(bool aNotify)
{
  
  if (!mIsDoneAddingChildren) {
    return false;
  }

  uint32_t count;
  GetLength(&count);
  for (uint32_t i = 0; i < count; i++) {
    bool disabled;
    nsresult rv = IsOptionDisabled(i, &disabled);

    if (NS_FAILED(rv) || !disabled) {
      rv = SetSelectedIndexInternal(i, aNotify);
      NS_ENSURE_SUCCESS(rv, false);

      UpdateValueMissingValidityState();
      UpdateState(aNotify);

      return true;
    }
  }

  return false;
}

nsresult
HTMLSelectElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLFormElementWithState::BindToTree(aDocument, aParent,
                                                              aBindingParent,
                                                              aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  UpdateBarredFromConstraintValidation();

  
  UpdateState(false);

  return rv;
}

void
HTMLSelectElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  nsGenericHTMLFormElementWithState::UnbindFromTree(aDeep, aNullParent);

  
  
  
  UpdateBarredFromConstraintValidation();

  
  UpdateState(false);
}

nsresult
HTMLSelectElement::BeforeSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                 const nsAttrValueOrString* aValue,
                                 bool aNotify)
{
  if (aNotify && aName == nsGkAtoms::disabled &&
      aNameSpaceID == kNameSpaceID_None) {
    mDisabledChanged = true;
  }

  return nsGenericHTMLFormElementWithState::BeforeSetAttr(aNameSpaceID, aName,
                                                          aValue, aNotify);
}

nsresult
HTMLSelectElement::AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    if (aName == nsGkAtoms::disabled) {
      UpdateBarredFromConstraintValidation();
    } else if (aName == nsGkAtoms::required) {
      UpdateValueMissingValidityState();
    } else if (aName == nsGkAtoms::autocomplete) {
      
      mAutocompleteAttrState = nsContentUtils::eAutocompleteAttrState_Unknown;
    }

    UpdateState(aNotify);
  }

  return nsGenericHTMLFormElementWithState::AfterSetAttr(aNameSpaceID, aName,
                                                         aValue, aNotify);
}

nsresult
HTMLSelectElement::UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify)
{
  if (aNotify && aNameSpaceID == kNameSpaceID_None &&
      aAttribute == nsGkAtoms::multiple) {
    
    
    
    
    
    
    if (mSelectedIndex >= 0) {
      SetSelectedIndexInternal(mSelectedIndex, aNotify);
    }
  }

  nsresult rv = nsGenericHTMLFormElementWithState::UnsetAttr(aNameSpaceID, aAttribute,
                                                             aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aNotify && aNameSpaceID == kNameSpaceID_None &&
      aAttribute == nsGkAtoms::multiple) {
    
    
    CheckSelectSomething(aNotify);
  }

  return rv;
}

void
HTMLSelectElement::DoneAddingChildren(bool aHaveNotified)
{
  mIsDoneAddingChildren = true;

  nsISelectControlFrame* selectFrame = GetSelectFrame();

  
  
  if (mRestoreState) {
    RestoreStateTo(mRestoreState);
    mRestoreState = nullptr;
  }

  
  if (selectFrame) {
    selectFrame->DoneAddingChildren(true);
  }

  if (!mInhibitStateRestoration) {
    nsresult rv = GenerateStateKey();
    if (NS_SUCCEEDED(rv)) {
      RestoreFormControlState();
    }
  }

  
  
  if (!CheckSelectSomething(false)) {
    
    
    
    UpdateValueMissingValidityState();

    
    UpdateState(aHaveNotified);
  }

  mDefaultSelectionSet = true;
}

bool
HTMLSelectElement::ParseAttribute(int32_t aNamespaceID,
                                  nsIAtom* aAttribute,
                                  const nsAString& aValue,
                                  nsAttrValue& aResult)
{
  if (kNameSpaceID_None == aNamespaceID) {
    if (aAttribute == nsGkAtoms::size) {
      return aResult.ParsePositiveIntValue(aValue);
    } else if (aAttribute == nsGkAtoms::autocomplete) {
      aResult.ParseAtomArray(aValue);
      return true;
    }
  }
  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

void
HTMLSelectElement::MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                         nsRuleData* aData)
{
  nsGenericHTMLFormElementWithState::MapImageAlignAttributeInto(aAttributes, aData);
  nsGenericHTMLFormElementWithState::MapCommonAttributesInto(aAttributes, aData);
}

nsChangeHint
HTMLSelectElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                          int32_t aModType) const
{
  nsChangeHint retval =
      nsGenericHTMLFormElementWithState::GetAttributeChangeHint(aAttribute, aModType);
  if (aAttribute == nsGkAtoms::multiple ||
      aAttribute == nsGkAtoms::size) {
    NS_UpdateHint(retval, NS_STYLE_HINT_FRAMECHANGE);
  }
  return retval;
}

NS_IMETHODIMP_(bool)
HTMLSelectElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry* const map[] = {
    sCommonAttributeMap,
    sImageAlignAttributeMap
  };

  return FindAttributeDependence(aAttribute, map);
}

nsMapRuleToAttributesFunc
HTMLSelectElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}

bool
HTMLSelectElement::IsDisabledForEvents(uint32_t aMessage)
{
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(false);
  nsIFrame* formFrame = nullptr;
  if (formControlFrame) {
    formFrame = do_QueryFrame(formControlFrame);
  }
  return IsElementDisabledForEvents(aMessage, formFrame);
}

nsresult
HTMLSelectElement::PreHandleEvent(EventChainPreVisitor& aVisitor)
{
  aVisitor.mCanHandle = false;
  if (IsDisabledForEvents(aVisitor.mEvent->message)) {
    return NS_OK;
  }

  return nsGenericHTMLFormElementWithState::PreHandleEvent(aVisitor);
}

nsresult
HTMLSelectElement::PostHandleEvent(EventChainPostVisitor& aVisitor)
{
  if (aVisitor.mEvent->message == NS_FOCUS_CONTENT) {
    
    
    mCanShowInvalidUI = !IsValid() && ShouldShowValidityUI();

    
    
    mCanShowValidUI = ShouldShowValidityUI();

    
    
  } else if (aVisitor.mEvent->message == NS_BLUR_CONTENT) {
    mCanShowInvalidUI = true;
    mCanShowValidUI = true;

    UpdateState(true);
  }

  return nsGenericHTMLFormElementWithState::PostHandleEvent(aVisitor);
}

EventStates
HTMLSelectElement::IntrinsicState() const
{
  EventStates state = nsGenericHTMLFormElementWithState::IntrinsicState();

  if (IsCandidateForConstraintValidation()) {
    if (IsValid()) {
      state |= NS_EVENT_STATE_VALID;
    } else {
      state |= NS_EVENT_STATE_INVALID;

      if ((!mForm || !mForm->HasAttr(kNameSpaceID_None, nsGkAtoms::novalidate)) &&
          (GetValidityState(VALIDITY_STATE_CUSTOM_ERROR) ||
           (mCanShowInvalidUI && ShouldShowValidityUI()))) {
        state |= NS_EVENT_STATE_MOZ_UI_INVALID;
      }
    }

    
    
    
    
    
    
    
    
    
    if ((!mForm || !mForm->HasAttr(kNameSpaceID_None, nsGkAtoms::novalidate)) &&
        (mCanShowValidUI && ShouldShowValidityUI() &&
         (IsValid() || (state.HasState(NS_EVENT_STATE_MOZ_UI_INVALID) &&
                        !mCanShowInvalidUI)))) {
      state |= NS_EVENT_STATE_MOZ_UI_VALID;
    }
  }

  if (HasAttr(kNameSpaceID_None, nsGkAtoms::required)) {
    state |= NS_EVENT_STATE_REQUIRED;
  } else {
    state |= NS_EVENT_STATE_OPTIONAL;
  }

  return state;
}



NS_IMETHODIMP
HTMLSelectElement::SaveState()
{
  nsRefPtr<SelectState> state = new SelectState();

  uint32_t len = Length();

  for (uint32_t optIndex = 0; optIndex < len; optIndex++) {
    HTMLOptionElement* option = Item(optIndex);
    if (option && option->Selected()) {
      nsAutoString value;
      option->GetValue(value);
      state->PutOption(optIndex, value);
    }
  }

  nsPresState* presState = GetPrimaryPresState();
  if (presState) {
    presState->SetStateProperty(state);

    if (mDisabledChanged) {
      
      
      presState->SetDisabled(HasAttr(kNameSpaceID_None, nsGkAtoms::disabled));
    }
  }

  return NS_OK;
}

bool
HTMLSelectElement::RestoreState(nsPresState* aState)
{
  
  nsCOMPtr<SelectState> state(
    do_QueryInterface(aState->GetStateProperty()));

  if (state) {
    RestoreStateTo(state);

    
    
    DispatchContentReset();
  }

  if (aState->IsDisabledSet()) {
    SetDisabled(aState->GetDisabled());
  }

  return false;
}

void
HTMLSelectElement::RestoreStateTo(SelectState* aNewSelected)
{
  if (!mIsDoneAddingChildren) {
    mRestoreState = aNewSelected;
    return;
  }

  uint32_t len = Length();
  uint32_t mask = IS_SELECTED | CLEAR_ALL | SET_DISABLED | NOTIFY;

  
  SetOptionsSelectedByIndex(-1, -1, mask);

  
  for (uint32_t i = 0; i < len; i++) {
    HTMLOptionElement* option = Item(i);
    if (option) {
      nsAutoString value;
      nsresult rv = option->GetValue(value);
      if (NS_SUCCEEDED(rv) && aNewSelected->ContainsOption(i, value)) {
        SetOptionsSelectedByIndex(i, i, IS_SELECTED | SET_DISABLED | NOTIFY);
      }
    }
  }
}

NS_IMETHODIMP
HTMLSelectElement::Reset()
{
  uint32_t numSelected = 0;

  
  
  
  uint32_t numOptions = Length();

  for (uint32_t i = 0; i < numOptions; i++) {
    nsRefPtr<HTMLOptionElement> option = Item(i);
    if (option) {
      
      
      

      uint32_t mask = SET_DISABLED | NOTIFY;
      if (option->DefaultSelected()) {
        mask |= IS_SELECTED;
        numSelected++;
      }

      SetOptionsSelectedByIndex(i, i, mask);
    }
  }

  
  
  
  if (numSelected == 0 && IsCombobox()) {
    SelectSomething(true);
  }

  SetSelectionChanged(false, true);

  
  
  
  
  
  
  DispatchContentReset();

  return NS_OK;
}

static NS_DEFINE_CID(kFormProcessorCID, NS_FORMPROCESSOR_CID);

NS_IMETHODIMP
HTMLSelectElement::SubmitNamesValues(nsFormSubmission* aFormSubmission)
{
  
  if (IsDisabled()) {
    return NS_OK;
  }

  
  
  
  nsAutoString name;
  GetAttr(kNameSpaceID_None, nsGkAtoms::name, name);
  if (name.IsEmpty()) {
    return NS_OK;
  }

  
  
  
  uint32_t len = Length();

  nsAutoString mozType;
  nsCOMPtr<nsIFormProcessor> keyGenProcessor;
  if (GetAttr(kNameSpaceID_None, nsGkAtoms::moztype, mozType) &&
      mozType.EqualsLiteral("-mozilla-keygen")) {
    keyGenProcessor = do_GetService(kFormProcessorCID);
  }

  for (uint32_t optIndex = 0; optIndex < len; optIndex++) {
    HTMLOptionElement* option = Item(optIndex);

    
    if (!option || IsOptionDisabled(option)) {
      continue;
    }

    if (!option->Selected()) {
      continue;
    }

    nsString value;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(option->GetValue(value)));

    if (keyGenProcessor) {
      nsString tmp(value);
      if (NS_SUCCEEDED(keyGenProcessor->ProcessValue(this, name, tmp))) {
        value = tmp;
      }
    }

    aFormSubmission->AddNameValuePair(name, value);
  }

  return NS_OK;
}

void
HTMLSelectElement::DispatchContentReset()
{
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(false);
  if (formControlFrame) {
    
    
    if (IsCombobox()) {
      nsIComboboxControlFrame* comboFrame = do_QueryFrame(formControlFrame);
      if (comboFrame) {
        comboFrame->OnContentReset();
      }
    } else {
      nsIListControlFrame* listFrame = do_QueryFrame(formControlFrame);
      if (listFrame) {
        listFrame->OnContentReset();
      }
    }
  }
}

static void
AddOptionsRecurse(nsIContent* aRoot, HTMLOptionsCollection* aArray)
{
  for (nsIContent* cur = aRoot->GetFirstChild();
       cur;
       cur = cur->GetNextSibling()) {
    HTMLOptionElement* opt = HTMLOptionElement::FromContent(cur);
    if (opt) {
      aArray->AppendOption(opt);
    } else if (cur->IsHTMLElement(nsGkAtoms::optgroup)) {
      AddOptionsRecurse(cur, aArray);
    }
  }
}

void
HTMLSelectElement::RebuildOptionsArray(bool aNotify)
{
  mOptions->Clear();
  AddOptionsRecurse(this, mOptions);
  FindSelectedIndex(0, aNotify);
}

bool
HTMLSelectElement::IsValueMissing()
{
  if (!Required()) {
    return false;
  }

  uint32_t length = Length();

  for (uint32_t i = 0; i < length; ++i) {
    nsRefPtr<HTMLOptionElement> option = Item(i);
    if (!option->Selected()) {
      continue;
    }

    if (IsOptionDisabled(option)) {
      continue;
    }

    nsAutoString value;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(option->GetValue(value)));
    if (!value.IsEmpty()) {
      return false;
    }
  }

  return true;
}

void
HTMLSelectElement::UpdateValueMissingValidityState()
{
  SetValidityState(VALIDITY_STATE_VALUE_MISSING, IsValueMissing());
}

nsresult
HTMLSelectElement::GetValidationMessage(nsAString& aValidationMessage,
                                        ValidityStateType aType)
{
  switch (aType) {
    case VALIDITY_STATE_VALUE_MISSING: {
      nsXPIDLString message;
      nsresult rv = nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                       "FormValidationSelectMissing",
                                                       message);
      aValidationMessage = message;
      return rv;
    }
    default: {
      return nsIConstraintValidation::GetValidationMessage(aValidationMessage, aType);
    }
  }
}

#ifdef DEBUG

static void
VerifyOptionsRecurse(nsIContent* aRoot, int32_t& aIndex,
                     HTMLOptionsCollection* aArray)
{
  for (nsIContent* cur = aRoot->GetFirstChild();
       cur;
       cur = cur->GetNextSibling()) {
    nsCOMPtr<nsIDOMHTMLOptionElement> opt = do_QueryInterface(cur);
    if (opt) {
      NS_ASSERTION(opt == aArray->ItemAsOption(aIndex++),
                   "Options collection broken");
    } else if (cur->IsHTMLElement(nsGkAtoms::optgroup)) {
      VerifyOptionsRecurse(cur, aIndex, aArray);
    }
  }
}

void
HTMLSelectElement::VerifyOptionsArray()
{
  int32_t aIndex = 0;
  VerifyOptionsRecurse(this, aIndex, mOptions);
}

#endif

void
HTMLSelectElement::UpdateBarredFromConstraintValidation()
{
  SetBarredFromConstraintValidation(IsDisabled());
}

void
HTMLSelectElement::FieldSetDisabledChanged(bool aNotify)
{
  UpdateBarredFromConstraintValidation();

  nsGenericHTMLFormElementWithState::FieldSetDisabledChanged(aNotify);
}

void
HTMLSelectElement::SetSelectionChanged(bool aValue, bool aNotify)
{
  if (!mDefaultSelectionSet) {
    return;
  }

  UpdateSelectedOptions();

  bool previousSelectionChangedValue = mSelectionHasChanged;
  mSelectionHasChanged = aValue;

  if (mSelectionHasChanged != previousSelectionChangedValue) {
    UpdateState(aNotify);
  }
}

void
HTMLSelectElement::UpdateSelectedOptions()
{
  if (mSelectedOptions) {
    mSelectedOptions->SetDirty();
  }
}

JSObject*
HTMLSelectElement::WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return HTMLSelectElementBinding::Wrap(aCx, this, aGivenProto);
}

} 
} 
