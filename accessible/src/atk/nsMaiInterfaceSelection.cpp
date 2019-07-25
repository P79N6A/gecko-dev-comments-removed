






































#include "InterfaceInitFuncs.h"

#include "nsAccessibleWrap.h"
#include "nsMai.h"

#include <atk/atk.h>

extern "C" {

static gboolean
addSelectionCB(AtkSelection *aSelection, gint i)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aSelection));
    if (!accWrap || !accWrap->IsSelect())
        return FALSE;

    return accWrap->AddItemToSelection(i);
}

static gboolean
clearSelectionCB(AtkSelection *aSelection)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aSelection));
    if (!accWrap || !accWrap->IsSelect())
        return FALSE;

    return accWrap->UnselectAll();
}

static AtkObject*
refSelectionCB(AtkSelection *aSelection, gint i)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aSelection));
    if (!accWrap || !accWrap->IsSelect())
        return nsnull;

    nsAccessible* selectedItem = accWrap->GetSelectedItem(i);
    if (!selectedItem)
        return nsnull;

    AtkObject* atkObj = nsAccessibleWrap::GetAtkObject(selectedItem);
    if (atkObj) {
        g_object_ref(atkObj);
    }
    return atkObj;
}

static gint
getSelectionCountCB(AtkSelection *aSelection)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aSelection));
    if (!accWrap || !accWrap->IsSelect())
        return -1;

    return accWrap->SelectedItemCount();
}

static gboolean
isChildSelectedCB(AtkSelection *aSelection, gint i)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aSelection));
    if (!accWrap || !accWrap->IsSelect())
        return FALSE;

    return accWrap->IsItemSelected(i);
}

static gboolean
removeSelectionCB(AtkSelection *aSelection, gint i)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aSelection));
    if (!accWrap || !accWrap->IsSelect())
        return FALSE;

    return accWrap->RemoveItemFromSelection(i);
}

static gboolean
selectAllSelectionCB(AtkSelection *aSelection)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aSelection));
    if (!accWrap || !accWrap->IsSelect())
        return FALSE;

    return accWrap->SelectAll();
}
}

void
selectionInterfaceInitCB(AtkSelectionIface* aIface)
{
  NS_ASSERTION(aIface, "Invalid aIface");
  if (NS_UNLIKELY(!aIface))
    return;

  aIface->add_selection = addSelectionCB;
  aIface->clear_selection = clearSelectionCB;
  aIface->ref_selection = refSelectionCB;
  aIface->get_selection_count = getSelectionCountCB;
  aIface->is_child_selected = isChildSelectedCB;
  aIface->remove_selection = removeSelectionCB;
  aIface->select_all_selection = selectAllSelectionCB;
}
