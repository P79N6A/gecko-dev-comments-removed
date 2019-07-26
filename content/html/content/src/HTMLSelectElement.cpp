




#include "mozilla/dom/HTMLSelectElement.h"

#include "mozAutoDocUpdate.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/HTMLOptGroupElement.h"
#include "mozilla/dom/HTMLOptionElement.h"
#include "mozilla/dom/HTMLSelectElementBinding.h"
#include "mozilla/Util.h"
#include "base/compiler_specific.h"
#include "nsContentCreatorFunctions.h"
#include "nsError.h"
#include "nsEventDispatcher.h"
#include "nsEventStates.h"
#include "nsFormSubmission.h"
#include "nsGkAtoms.h"
#include "nsGUIEvent.h"
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

NS_IMPL_ISUPPORTS1(SelectState, SelectState)
NS_DEFINE_STATIC_IID_ACCESSOR(SelectState, NS_SELECT_STATE_IID)






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









HTMLSelectElement::HTMLSelectElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                                     FromParser aFromParser)
  : nsGenericHTMLFormElement(aNodeInfo),
    ALLOW_THIS_IN_INITIALIZER_LIST(mOptions(new HTMLOptionsCollection(this))),
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
  
  

  
  AddStatesSilently(NS_EVENT_STATE_ENABLED |
                    NS_EVENT_STATE_OPTIONAL |
                    NS_EVENT_STATE_VALID);

  SetIsDOMBinding();
}

HTMLSelectElement::~HTMLSelectElement()
{
  mOptions->DropReference();
}



NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(HTMLSelectElement,
                                                  nsGenericHTMLFormElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mValidity)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOptions)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(HTMLSelectElement,
                                                nsGenericHTMLFormElement)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mValidity)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(HTMLSelectElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLSelectElement, Element)


NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(HTMLSelectElement)
  NS_HTML_CONTENT_INTERFACES(nsGenericHTMLFormElement)
  NS_INTERFACE_TABLE_INHERITED2(HTMLSelectElement,
                                nsIDOMHTMLSelectElement,
                                nsIConstraintValidation)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE
NS_ELEMENT_INTERFACE_MAP_END





NS_IMPL_ELEMENT_CLONE(HTMLSelectElement)


NS_IMPL_NSICONSTRAINTVALIDATION_EXCEPT_SETCUSTOMVALIDITY(HTMLSelectElement)

NS_IMETHODIMP
HTMLSelectElement::SetCustomValidity(const nsAString& aError)
{
  nsIConstraintValidation::SetCustomValidity(aError);

  UpdateState(true);

  return NS_OK;
}

NS_IMETHODIMP
HTMLSelectElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElement::GetForm(aForm);
}

nsresult
HTMLSelectElement::InsertChildAt(nsIContent* aKid,
                                 uint32_t aIndex,
                                 bool aNotify)
{
  SafeOptionListMutation safeMutation(this, this, aKid, aIndex, aNotify);
  nsresult rv = nsGenericHTMLFormElement::InsertChildAt(aKid, aIndex, aNotify);
  if (NS_FAILED(rv)) {
    safeMutation.MutationFailed();
  }
  return rv;
}

void
HTMLSelectElement::RemoveChildAt(uint32_t aIndex, bool aNotify)
{
  SafeOptionListMutation safeMutation(this, this, nullptr, aIndex, aNotify);
  nsGenericHTMLFormElement::RemoveChildAt(aIndex, aNotify);
}




nsresult
HTMLSelectElement::InsertOptionsIntoList(nsIContent* aOptions,
                                         int32_t aListIndex,
                                         int32_t aDepth,
                                         bool aNotify)
{
  int32_t insertIndex = aListIndex;
  nsresult rv = InsertOptionsIntoListRecurse(aOptions, &insertIndex, aDepth);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (insertIndex - aListIndex) {
    
    if (aListIndex <= mSelectedIndex) {
      mSelectedIndex += (insertIndex - aListIndex);
      SetSelectionChanged(true, aNotify);
    }

    
    
    
    nsISelectControlFrame* selectFrame = nullptr;
    nsWeakFrame weakSelectFrame;
    bool didGetFrame = false;

    
    nsCOMPtr<nsIDOMNode> optionNode;
    nsCOMPtr<nsIDOMHTMLOptionElement> option;
    for (int32_t i = aListIndex; i < insertIndex; i++) {
      
      if (!didGetFrame || (selectFrame && !weakSelectFrame.IsAlive())) {
        selectFrame = GetSelectFrame();
        weakSelectFrame = do_QueryFrame(selectFrame);
        didGetFrame = true;
      }

      if (selectFrame) {
        selectFrame->AddOption(i);
      }

      Item(i, getter_AddRefs(optionNode));
      option = do_QueryInterface(optionNode);
      if (option) {
        bool selected;
        option->GetSelected(&selected);
        if (selected) {
          
          if (!HasAttr(kNameSpaceID_None, nsGkAtoms::multiple)) {
            SetOptionsSelectedByIndex(i, i, true, true, true, true, nullptr);
          }

          
          
          
          OnOptionSelected(selectFrame, i, true, false, false);
        }
      }
    }

    CheckSelectSomething(aNotify);
  }

  return NS_OK;
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




nsresult
HTMLSelectElement::InsertOptionsIntoListRecurse(nsIContent* aOptions,
                                                int32_t* aInsertIndex,
                                                int32_t aDepth)
{
  
  
  
  

  HTMLOptionElement* optElement = HTMLOptionElement::FromContent(aOptions);
  if (optElement) {
    mOptions->InsertOptionAt(optElement, *aInsertIndex);
    (*aInsertIndex)++;
    return NS_OK;
  }

  
  
  if (aDepth == 0) {
    mNonOptionChildren++;
  }

  
  if (aOptions->IsHTML(nsGkAtoms::optgroup)) {
    mOptGroupCount++;

    for (nsIContent* child = aOptions->GetFirstChild();
         child;
         child = child->GetNextSibling()) {
      nsresult rv = InsertOptionsIntoListRecurse(child,
                                                 aInsertIndex, aDepth + 1);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
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

  
  if (mOptGroupCount && aOptions->IsHTML(nsGkAtoms::optgroup)) {
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

  return InsertOptionsIntoList(aOptions, ind, level, aNotify);
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
    nsGenericHTMLElement::AppendChild(aElement, aError);
    return;
  }

  
  
  nsINode* parent = aBefore->GetParentNode();
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
    return error.ErrorCode();
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
    return error.ErrorCode();
  }

  
  int32_t index;
  NS_ENSURE_SUCCESS(aBefore->GetAsInt32(&index), NS_ERROR_DOM_SYNTAX_ERR);

  ErrorResult error;
  Add(*htmlElement, index, error);
  return error.ErrorCode();
}

NS_IMETHODIMP
HTMLSelectElement::Remove(int32_t aIndex)
{
  nsCOMPtr<nsIDOMNode> option;
  Item(aIndex, getter_AddRefs(option));

  if (option) {
    nsCOMPtr<nsIDOMNode> parent;

    option->GetParentNode(getter_AddRefs(parent));
    if (parent) {
      nsCOMPtr<nsIDOMNode> ret;
      parent->RemoveChild(option, getter_AddRefs(ret));
    }
  }

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
  uint32_t curlen;
  nsresult rv = GetLength(&curlen);
  if (NS_FAILED(rv)) {
    curlen = 0;
  }

  if (curlen > aLength) { 
    for (uint32_t i = curlen; i > aLength && NS_SUCCEEDED(rv); --i) {
      rv = Remove(i - 1);
    }
  } else if (aLength > curlen) {
    if (aLength > MAX_DYNAMIC_SELECT_LENGTH) {
      return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
    }
    
    
    nsCOMPtr<nsINodeInfo> nodeInfo;

    nsContentUtils::NameChanged(mNodeInfo, nsGkAtoms::option,
                                getter_AddRefs(nodeInfo));

    nsCOMPtr<nsIContent> element = NS_NewHTMLOptionElement(nodeInfo.forget());
    if (!element) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsRefPtr<nsTextNode> text = new nsTextNode(mNodeInfo->NodeInfoManager());

    rv = element->AppendChildTo(text, false);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMNode> node(do_QueryInterface(element));

    for (uint32_t i = curlen; i < aLength; i++) {
      nsCOMPtr<nsIDOMNode> tmpNode;

      rv = AppendChild(node, getter_AddRefs(tmpNode));
      NS_ENSURE_SUCCESS(rv, rv);

      if (i + 1 < aLength) {
        nsCOMPtr<nsIDOMNode> newNode;

        rv = node->CloneNode(true, 1, getter_AddRefs(newNode));
        NS_ENSURE_SUCCESS(rv, rv);

        node = newNode;
      }
    }
  }

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

  nsresult rv = SetOptionsSelectedByIndex(aIndex, aIndex, true,
                                          true, true, aNotify, nullptr);

  if (NS_SUCCEEDED(rv)) {
    nsISelectControlFrame* selectFrame = GetSelectFrame();
    if (selectFrame) {
      rv = selectFrame->OnSetSelectedIndex(oldSelectedIndex, mSelectedIndex);
    }
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
  nsIDOMHTMLOptionElement* option = mOptions->ItemAsOption(aIndex);
  bool isSelected = false;
  if (option) {
    option->GetSelected(&isSelected);
  }
  return isSelected;
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
    
    nsCOMPtr<nsIDOMNode> option;
    Item(aIndex, getter_AddRefs(option));
    if (option) {
      nsRefPtr<HTMLOptionElement> optionElement =
        static_cast<HTMLOptionElement*>(option.get());
      optionElement->SetSelectedInternal(aSelected, aNotify);
    }
  }

  
  if (aSelectFrame) {
    aSelectFrame->OnOptionSelected(aIndex, aSelected);
  }

  UpdateValueMissingValidityState();
  UpdateState(aNotify);
}

void
HTMLSelectElement::FindSelectedIndex(int32_t aStartIndex, bool aNotify)
{
  mSelectedIndex = -1;
  SetSelectionChanged(true, aNotify);
  uint32_t len;
  GetLength(&len);
  for (int32_t i = aStartIndex; i < int32_t(len); i++) {
    if (IsOptionSelectedByIndex(i)) {
      mSelectedIndex = i;
      SetSelectionChanged(true, aNotify);
      break;
    }
  }
}

























NS_IMETHODIMP
HTMLSelectElement::SetOptionsSelectedByIndex(int32_t aStartIndex,
                                             int32_t aEndIndex,
                                             bool aIsSelected,
                                             bool aClearAll,
                                             bool aSetDisabled,
                                             bool aNotify,
                                             bool* aChangedSomething)
{
#if 0
  printf("SetOption(%d-%d, %c, ClearAll=%c)\n", aStartIndex, aEndIndex,
                                       (aIsSelected ? 'Y' : 'N'),
                                       (aClearAll ? 'Y' : 'N'));
#endif
  if (aChangedSomething) {
    *aChangedSomething = false;
  }

  
  if (!aSetDisabled && IsDisabled()) {
    return NS_OK;
  }

  
  uint32_t numItems = 0;
  GetLength(&numItems);
  if (numItems == 0) {
    return NS_OK;
  }

  
  bool isMultiple = HasAttr(kNameSpaceID_None, nsGkAtoms::multiple);

  
  
  bool optionsSelected = false;
  bool optionsDeselected = false;

  nsISelectControlFrame* selectFrame = nullptr;
  bool didGetFrame = false;
  nsWeakFrame weakSelectFrame;

  if (aIsSelected) {
    
    if (aStartIndex >= (int32_t)numItems || aStartIndex < 0 ||
        aEndIndex >= (int32_t)numItems || aEndIndex < 0) {
      aStartIndex = -1;
      aEndIndex = -1;
    }

    
    if (!isMultiple) {
      aEndIndex = aStartIndex;
    }

    
    
    
    
    bool allDisabled = !aSetDisabled;

    
    
    
    int32_t previousSelectedIndex = mSelectedIndex;

    
    
    
    
    if (aStartIndex != -1) {
      
      
      for (int32_t optIndex = aStartIndex; optIndex <= aEndIndex; optIndex++) {

        
        if (!aSetDisabled) {
          bool isDisabled;
          IsOptionDisabled(optIndex, &isDisabled);

          if (isDisabled) {
            continue;
          } else {
            allDisabled = false;
          }
        }

        nsIDOMHTMLOptionElement* option = mOptions->ItemAsOption(optIndex);
        if (option) {
          
          bool isSelected = false;
          option->GetSelected(&isSelected);
          if (!isSelected) {
            
            
            
            
            selectFrame = GetSelectFrame();
            weakSelectFrame = do_QueryFrame(selectFrame);
            didGetFrame = true;

            OnOptionSelected(selectFrame, optIndex, true, true, aNotify);
            optionsSelected = true;
          }
        }
      }
    }

    
    
    if (((!isMultiple && optionsSelected)
       || (aClearAll && !allDisabled)
       || aStartIndex == -1)
       && previousSelectedIndex != -1) {
      for (int32_t optIndex = previousSelectedIndex;
           optIndex < int32_t(numItems);
           optIndex++) {
        if (optIndex < aStartIndex || optIndex > aEndIndex) {
          nsIDOMHTMLOptionElement* option = mOptions->ItemAsOption(optIndex);
          if (option) {
            
            bool isSelected = false;
            option->GetSelected(&isSelected);
            if (isSelected) {
              if (!didGetFrame || (selectFrame && !weakSelectFrame.IsAlive())) {
                
                
                
                selectFrame = GetSelectFrame();
                weakSelectFrame = do_QueryFrame(selectFrame);

                didGetFrame = true;
              }

              OnOptionSelected(selectFrame, optIndex, false, true,
                               aNotify);
              optionsDeselected = true;

              
              if (!isMultiple) {
                break;
              }
            }
          }
        }
      }
    }

  } else {

    
    
    for (int32_t optIndex = aStartIndex; optIndex <= aEndIndex; optIndex++) {
      if (!aSetDisabled) {
        bool isDisabled;
        IsOptionDisabled(optIndex, &isDisabled);
        if (isDisabled) {
          continue;
        }
      }

      nsIDOMHTMLOptionElement* option = mOptions->ItemAsOption(optIndex);
      if (option) {
        
        bool isSelected = false;
        option->GetSelected(&isSelected);
        if (isSelected) {
          if (!didGetFrame || (selectFrame && !weakSelectFrame.IsAlive())) {
            
            
            
            selectFrame = GetSelectFrame();
            weakSelectFrame = do_QueryFrame(selectFrame);

            didGetFrame = true;
          }

          OnOptionSelected(selectFrame, optIndex, false, true, aNotify);
          optionsDeselected = true;
        }
      }
    }
  }

  
  if (optionsDeselected && aStartIndex != -1) {
    optionsSelected = CheckSelectSomething(aNotify) || optionsSelected;
  }

  
  if (optionsSelected || optionsDeselected) {
    if (aChangedSomething)
      *aChangedSomething = true;
  }

  return NS_OK;
}

NS_IMETHODIMP
HTMLSelectElement::IsOptionDisabled(int32_t aIndex, bool* aIsDisabled)
{
  *aIsDisabled = false;
  nsCOMPtr<nsIDOMNode> optionNode;
  Item(aIndex, getter_AddRefs(optionNode));
  NS_ENSURE_TRUE(optionNode, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMHTMLOptionElement> option = do_QueryInterface(optionNode);
  if (option) {
    bool isDisabled;
    option->GetDisabled(&isDisabled);
    if (isDisabled) {
      *aIsDisabled = true;
      return NS_OK;
    }
  }

  
  
  if (mNonOptionChildren) {
    nsCOMPtr<nsIDOMNode> parent;
    while (1) {
      optionNode->GetParentNode(getter_AddRefs(parent));

      
      if (!parent) {
        break;
      }

      
      nsCOMPtr<nsIDOMHTMLSelectElement> selectElement =
        do_QueryInterface(parent);
      if (selectElement) {
        break;
      }

      nsCOMPtr<nsIDOMHTMLOptGroupElement> optGroupElement =
        do_QueryInterface(parent);

      if (optGroupElement) {
        bool isDisabled;
        optGroupElement->GetDisabled(&isDisabled);

        if (isDisabled) {
          *aIsDisabled = true;
          return NS_OK;
        }
      } else {
        
        
        break;
      }

      optionNode = parent;
    }
  }

  return NS_OK;
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
  if (nsGenericHTMLFormElement::IsHTMLFocusable(aWithMouse, aIsFocusable, aTabIndex)) {
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
  nsresult rv = nsGenericHTMLFormElement::BindToTree(aDocument, aParent,
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
  nsGenericHTMLFormElement::UnbindFromTree(aDeep, aNullParent);

  
  
  
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

  return nsGenericHTMLFormElement::BeforeSetAttr(aNameSpaceID, aName,
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
    }

    UpdateState(aNotify);
  }

  return nsGenericHTMLFormElement::AfterSetAttr(aNameSpaceID, aName,
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

  nsresult rv = nsGenericHTMLFormElement::UnsetAttr(aNameSpaceID, aAttribute,
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
    RestoreFormControlState(this, this);
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
  if (aAttribute == nsGkAtoms::size && kNameSpaceID_None == aNamespaceID) {
    return aResult.ParsePositiveIntValue(aValue);
  }
  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

static void
MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                      nsRuleData* aData)
{
  nsGenericHTMLFormElement::MapImageAlignAttributeInto(aAttributes, aData);
  nsGenericHTMLFormElement::MapCommonAttributesInto(aAttributes, aData);
}

nsChangeHint
HTMLSelectElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                          int32_t aModType) const
{
  nsChangeHint retval =
      nsGenericHTMLFormElement::GetAttributeChangeHint(aAttribute, aModType);
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
HTMLSelectElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  aVisitor.mCanHandle = false;
  if (IsDisabledForEvents(aVisitor.mEvent->message)) {
    return NS_OK;
  }

  return nsGenericHTMLFormElement::PreHandleEvent(aVisitor);
}

nsresult
HTMLSelectElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  if (aVisitor.mEvent->message == NS_FOCUS_CONTENT) {
    
    
    mCanShowInvalidUI = !IsValid() && ShouldShowValidityUI();

    
    
    mCanShowValidUI = ShouldShowValidityUI();

    
    
  } else if (aVisitor.mEvent->message == NS_BLUR_CONTENT) {
    mCanShowInvalidUI = true;
    mCanShowValidUI = true;

    UpdateState(true);
  }

  return nsGenericHTMLFormElement::PostHandleEvent(aVisitor);
}

nsEventStates
HTMLSelectElement::IntrinsicState() const
{
  nsEventStates state = nsGenericHTMLFormElement::IntrinsicState();

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

  uint32_t len;
  GetLength(&len);

  for (uint32_t optIndex = 0; optIndex < len; optIndex++) {
    nsIDOMHTMLOptionElement* option = mOptions->ItemAsOption(optIndex);
    if (option) {
      bool isSelected;
      option->GetSelected(&isSelected);
      if (isSelected) {
        nsAutoString value;
        option->GetValue(value);
        state->PutOption(optIndex, value);
      }
    }
  }

  nsPresState* presState = nullptr;
  nsresult rv = GetPrimaryPresState(this, &presState);
  if (presState) {
    presState->SetStateProperty(state);

    if (mDisabledChanged) {
      
      
      presState->SetDisabled(HasAttr(kNameSpaceID_None, nsGkAtoms::disabled));
    }
  }

  return rv;
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

  uint32_t len;
  GetLength(&len);

  
  SetOptionsSelectedByIndex(-1, -1, true, true, true, true, nullptr);

  
  for (int32_t i = 0; i < int32_t(len); i++) {
    nsIDOMHTMLOptionElement* option = mOptions->ItemAsOption(i);
    if (option) {
      nsAutoString value;
      nsresult rv = option->GetValue(value);
      if (NS_SUCCEEDED(rv) && aNewSelected->ContainsOption(i, value)) {
        SetOptionsSelectedByIndex(i, i, true, false, true, true, nullptr);
      }
    }
  }
}

NS_IMETHODIMP
HTMLSelectElement::Reset()
{
  uint32_t numSelected = 0;

  
  
  
  uint32_t numOptions;
  nsresult rv = GetLength(&numOptions);
  NS_ENSURE_SUCCESS(rv, rv);

  for (uint32_t i = 0; i < numOptions; i++) {
    nsCOMPtr<nsIDOMNode> node;
    rv = Item(i, getter_AddRefs(node));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMHTMLOptionElement> option(do_QueryInterface(node));

    NS_ASSERTION(option, "option not an OptionElement");
    if (option) {
      
      
      
      bool selected = false;
      option->GetDefaultSelected(&selected);
      SetOptionsSelectedByIndex(i, i, selected,
                                false, true, true, nullptr);
      if (selected) {
        numSelected++;
      }
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

  
  
  
  uint32_t len;
  GetLength(&len);

  nsAutoString mozType;
  nsCOMPtr<nsIFormProcessor> keyGenProcessor;
  if (GetAttr(kNameSpaceID_None, nsGkAtoms::_moz_type, mozType) &&
      mozType.EqualsLiteral("-mozilla-keygen")) {
    keyGenProcessor = do_GetService(kFormProcessorCID);
  }

  for (uint32_t optIndex = 0; optIndex < len; optIndex++) {
    
    bool disabled;
    nsresult rv = IsOptionDisabled(optIndex, &disabled);
    if (NS_FAILED(rv) || disabled) {
      continue;
    }

    nsIDOMHTMLOptionElement* option = mOptions->ItemAsOption(optIndex);
    NS_ENSURE_TRUE(option, NS_ERROR_UNEXPECTED);

    bool isSelected;
    rv = option->GetSelected(&isSelected);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!isSelected) {
      continue;
    }

    nsCOMPtr<nsIDOMHTMLOptionElement> optionElement = do_QueryInterface(option);
    NS_ENSURE_TRUE(optionElement, NS_ERROR_UNEXPECTED);

    nsAutoString value;
    rv = optionElement->GetValue(value);
    NS_ENSURE_SUCCESS(rv, rv);

    if (keyGenProcessor) {
      nsAutoString tmp(value);
      rv = keyGenProcessor->ProcessValue(this, name, tmp);
      if (NS_SUCCEEDED(rv)) {
        value = tmp;
      }
    }

    rv = aFormSubmission->AddNameValuePair(name, value);
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
    } else if (cur->IsHTML(nsGkAtoms::optgroup)) {
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
  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::required)) {
    return false;
  }

  uint32_t length;
  mOptions->GetLength(&length);

  for (uint32_t i = 0; i < length; ++i) {
    nsIDOMHTMLOptionElement* option = mOptions->ItemAsOption(i);
    bool selected;
    NS_ENSURE_SUCCESS(option->GetSelected(&selected), false);

    if (!selected) {
      continue;
    }

    bool disabled;
    IsOptionDisabled(i, &disabled);
    if (disabled) {
      continue;
    }

    nsAutoString value;
    NS_ENSURE_SUCCESS(option->GetValue(value), false);
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
    } else if (cur->IsHTML(nsGkAtoms::optgroup)) {
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

  nsGenericHTMLFormElement::FieldSetDisabledChanged(aNotify);
}

void
HTMLSelectElement::SetSelectionChanged(bool aValue, bool aNotify)
{
  if (!mDefaultSelectionSet) {
    return;
  }

  bool previousSelectionChangedValue = mSelectionHasChanged;
  mSelectionHasChanged = aValue;

  if (mSelectionHasChanged != previousSelectionChangedValue) {
    UpdateState(aNotify);
  }
}

JSObject*
HTMLSelectElement::WrapNode(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return HTMLSelectElementBinding::Wrap(aCx, aScope, this);
}

} 
} 
