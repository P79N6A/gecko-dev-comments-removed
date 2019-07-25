





#include "XULSelectControlAccessible.h"

#include "nsAccessibilityService.h"
#include "DocAccessible.h"

#include "nsIDOMXULContainerElement.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsIDOMXULMultSelectCntrlEl.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMElement.h"
#include "nsIDOMXULElement.h"
#include "nsIMutableArray.h"
#include "nsIServiceManager.h"

#include "mozilla/dom/Element.h"

using namespace mozilla;
using namespace mozilla::a11y;





XULSelectControlAccessible::
  XULSelectControlAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
  mSelectControl = do_QueryInterface(aContent);
}




void
XULSelectControlAccessible::Shutdown()
{
  mSelectControl = nullptr;
  AccessibleWrap::Shutdown();
}




bool
XULSelectControlAccessible::IsSelect()
{
  return !!mSelectControl;
}


already_AddRefed<nsIArray>
XULSelectControlAccessible::SelectedItems()
{
  nsCOMPtr<nsIMutableArray> selectedItems =
    do_CreateInstance(NS_ARRAY_CONTRACTID);
  if (!selectedItems || !mDoc)
    return nullptr;

  
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> xulMultiSelect =
    do_QueryInterface(mSelectControl);
  if (xulMultiSelect) {
    int32_t length = 0;
    xulMultiSelect->GetSelectedCount(&length);
    for (int32_t index = 0; index < length; index++) {
      nsCOMPtr<nsIDOMXULSelectControlItemElement> itemElm;
      xulMultiSelect->MultiGetSelectedItem(index, getter_AddRefs(itemElm));
      nsCOMPtr<nsINode> itemNode(do_QueryInterface(itemElm));
      Accessible* item = mDoc->GetAccessible(itemNode);
      if (item)
        selectedItems->AppendElement(static_cast<nsIAccessible*>(item),
                                     false);
    }
  } else {  
    nsCOMPtr<nsIDOMXULSelectControlItemElement> itemElm;
    mSelectControl->GetSelectedItem(getter_AddRefs(itemElm));
    nsCOMPtr<nsINode> itemNode(do_QueryInterface(itemElm));
    if (itemNode) {
      Accessible* item = mDoc->GetAccessible(itemNode);
      if (item)
        selectedItems->AppendElement(static_cast<nsIAccessible*>(item),
                                   false);
    }
  }

  nsIMutableArray* items = nullptr;
  selectedItems.forget(&items);
  return items;
}

Accessible*
XULSelectControlAccessible::GetSelectedItem(uint32_t aIndex)
{
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelectControl =
    do_QueryInterface(mSelectControl);

  nsCOMPtr<nsIDOMXULSelectControlItemElement> itemElm;
  if (multiSelectControl)
    multiSelectControl->MultiGetSelectedItem(aIndex, getter_AddRefs(itemElm));
  else if (aIndex == 0)
    mSelectControl->GetSelectedItem(getter_AddRefs(itemElm));

  nsCOMPtr<nsINode> itemNode(do_QueryInterface(itemElm));
  return itemNode && mDoc ? mDoc->GetAccessible(itemNode) : nullptr;
}

uint32_t
XULSelectControlAccessible::SelectedItemCount()
{
  
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelectControl =
    do_QueryInterface(mSelectControl);
  if (multiSelectControl) {
    int32_t count = 0;
    multiSelectControl->GetSelectedCount(&count);
    return count;
  }

  
  int32_t index;
  mSelectControl->GetSelectedIndex(&index);
  return (index >= 0) ? 1 : 0;
}

bool
XULSelectControlAccessible::AddItemToSelection(uint32_t aIndex)
{
  Accessible* item = GetChildAt(aIndex);
  if (!item)
    return false;

  nsCOMPtr<nsIDOMXULSelectControlItemElement> itemElm =
    do_QueryInterface(item->GetContent());
  if (!itemElm)
    return false;

  bool isItemSelected = false;
  itemElm->GetSelected(&isItemSelected);
  if (isItemSelected)
    return true;

  nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelectControl =
    do_QueryInterface(mSelectControl);

  if (multiSelectControl)
    multiSelectControl->AddItemToSelection(itemElm);
  else
    mSelectControl->SetSelectedItem(itemElm);

  return true;
}

bool
XULSelectControlAccessible::RemoveItemFromSelection(uint32_t aIndex)
{
  Accessible* item = GetChildAt(aIndex);
  if (!item)
    return false;

  nsCOMPtr<nsIDOMXULSelectControlItemElement> itemElm =
      do_QueryInterface(item->GetContent());
  if (!itemElm)
    return false;

  bool isItemSelected = false;
  itemElm->GetSelected(&isItemSelected);
  if (!isItemSelected)
    return true;

  nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelectControl =
    do_QueryInterface(mSelectControl);

  if (multiSelectControl)
    multiSelectControl->RemoveItemFromSelection(itemElm);
  else
    mSelectControl->SetSelectedItem(nullptr);

  return true;
}

bool
XULSelectControlAccessible::IsItemSelected(uint32_t aIndex)
{
  Accessible* item = GetChildAt(aIndex);
  if (!item)
    return false;

  nsCOMPtr<nsIDOMXULSelectControlItemElement> itemElm =
    do_QueryInterface(item->GetContent());
  if (!itemElm)
    return false;

  bool isItemSelected = false;
  itemElm->GetSelected(&isItemSelected);
  return isItemSelected;
}

bool
XULSelectControlAccessible::UnselectAll()
{
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelectControl =
    do_QueryInterface(mSelectControl);
  multiSelectControl ?
    multiSelectControl->ClearSelection() : mSelectControl->SetSelectedIndex(-1);

  return true;
}

bool
XULSelectControlAccessible::SelectAll()
{
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelectControl =
    do_QueryInterface(mSelectControl);
  if (multiSelectControl) {
    multiSelectControl->SelectAll();
    return true;
  }

  
  return false;
}




Accessible*
XULSelectControlAccessible::CurrentItem()
{
  if (!mSelectControl)
    return nullptr;

  nsCOMPtr<nsIDOMXULSelectControlItemElement> currentItemElm;
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelectControl =
    do_QueryInterface(mSelectControl);
  if (multiSelectControl)
    multiSelectControl->GetCurrentItem(getter_AddRefs(currentItemElm));
  else
    mSelectControl->GetSelectedItem(getter_AddRefs(currentItemElm));

  nsCOMPtr<nsINode> DOMNode;
  if (currentItemElm)
    DOMNode = do_QueryInterface(currentItemElm);

  if (DOMNode) {
    DocAccessible* document = Document();
    if (document)
      return document->GetAccessible(DOMNode);
  }

  return nullptr;
}

void
XULSelectControlAccessible::SetCurrentItem(Accessible* aItem)
{
  if (!mSelectControl)
    return;

  nsCOMPtr<nsIDOMXULSelectControlItemElement> itemElm =
    do_QueryInterface(aItem->GetContent());
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelectControl =
    do_QueryInterface(mSelectControl);
  if (multiSelectControl)
    multiSelectControl->SetCurrentItem(itemElm);
  else
    mSelectControl->SetSelectedItem(itemElm);
}
