






































#include "XULSelectControlAccessible.h"

#include "nsAccessibilityService.h"
#include "nsDocAccessible.h"

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
  XULSelectControlAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsAccessibleWrap(aContent, aDoc)
{
  mSelectControl = do_QueryInterface(aContent);
}




void
XULSelectControlAccessible::Shutdown()
{
  mSelectControl = nsnull;
  nsAccessibleWrap::Shutdown();
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
    return nsnull;

  
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> xulMultiSelect =
    do_QueryInterface(mSelectControl);
  if (xulMultiSelect) {
    PRInt32 length = 0;
    xulMultiSelect->GetSelectedCount(&length);
    for (PRInt32 index = 0; index < length; index++) {
      nsCOMPtr<nsIDOMXULSelectControlItemElement> itemElm;
      xulMultiSelect->GetSelectedItem(index, getter_AddRefs(itemElm));
      nsCOMPtr<nsINode> itemNode(do_QueryInterface(itemElm));
      nsAccessible* item = mDoc->GetAccessible(itemNode);
      if (item)
        selectedItems->AppendElement(static_cast<nsIAccessible*>(item),
                                     false);
    }
  } else {  
      nsCOMPtr<nsIDOMXULSelectControlItemElement> itemElm;
      mSelectControl->GetSelectedItem(getter_AddRefs(itemElm));
      nsCOMPtr<nsINode> itemNode(do_QueryInterface(itemElm));
      if(itemNode) {
        nsAccessible* item = mDoc->GetAccessible(itemNode);
        if (item)
          selectedItems->AppendElement(static_cast<nsIAccessible*>(item),
                                     false);
      }
  }

  nsIMutableArray* items = nsnull;
  selectedItems.forget(&items);
  return items;
}

nsAccessible*
XULSelectControlAccessible::GetSelectedItem(PRUint32 aIndex)
{
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelectControl =
    do_QueryInterface(mSelectControl);

  nsCOMPtr<nsIDOMXULSelectControlItemElement> itemElm;
  if (multiSelectControl)
    multiSelectControl->GetSelectedItem(aIndex, getter_AddRefs(itemElm));
  else if (aIndex == 0)
    mSelectControl->GetSelectedItem(getter_AddRefs(itemElm));

  nsCOMPtr<nsINode> itemNode(do_QueryInterface(itemElm));
  return itemNode && mDoc ? mDoc->GetAccessible(itemNode) : nsnull;
}

PRUint32
XULSelectControlAccessible::SelectedItemCount()
{
  
  nsCOMPtr<nsIDOMXULMultiSelectControlElement> multiSelectControl =
    do_QueryInterface(mSelectControl);
  if (multiSelectControl) {
    PRInt32 count = 0;
    multiSelectControl->GetSelectedCount(&count);
    return count;
  }

  
  PRInt32 index;
  mSelectControl->GetSelectedIndex(&index);
  return (index >= 0) ? 1 : 0;
}

bool
XULSelectControlAccessible::AddItemToSelection(PRUint32 aIndex)
{
  nsAccessible* item = GetChildAt(aIndex);
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
XULSelectControlAccessible::RemoveItemFromSelection(PRUint32 aIndex)
{
  nsAccessible* item = GetChildAt(aIndex);
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
    mSelectControl->SetSelectedItem(nsnull);

  return true;
}

bool
XULSelectControlAccessible::IsItemSelected(PRUint32 aIndex)
{
  nsAccessible* item = GetChildAt(aIndex);
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




nsAccessible*
XULSelectControlAccessible::CurrentItem()
{
  if (!mSelectControl)
    return nsnull;

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
    nsDocAccessible* document = Document();
    if (document)
      return document->GetAccessible(DOMNode);
  }

  return nsnull;
}

void
XULSelectControlAccessible::SetCurrentItem(nsAccessible* aItem)
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
