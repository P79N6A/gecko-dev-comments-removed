






































#include "nsMaiInterfaceSelection.h"

void
selectionInterfaceInitCB(AtkSelectionIface *aIface)
{
    NS_ASSERTION(aIface, "Invalid aIface");
    if (!aIface)
        return;

    aIface->add_selection = addSelectionCB;
    aIface->clear_selection = clearSelectionCB;
    aIface->ref_selection = refSelectionCB;
    aIface->get_selection_count = getSelectionCountCB;
    aIface->is_child_selected = isChildSelectedCB;
    aIface->remove_selection = removeSelectionCB;
    aIface->select_all_selection = selectAllSelectionCB;
}

gboolean
addSelectionCB(AtkSelection *aSelection, gint i)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aSelection));
    if (!accWrap || !accWrap->IsSelect())
        return FALSE;

    return accWrap->AddItemToSelection(i);
}

gboolean
clearSelectionCB(AtkSelection *aSelection)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aSelection));
    if (!accWrap || !accWrap->IsSelect())
        return FALSE;

    return accWrap->UnselectAll();
}

AtkObject *
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

gint
getSelectionCountCB(AtkSelection *aSelection)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aSelection));
    if (!accWrap || !accWrap->IsSelect())
        return -1;

    return accWrap->SelectedItemCount();
}

gboolean
isChildSelectedCB(AtkSelection *aSelection, gint i)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aSelection));
    if (!accWrap || !accWrap->IsSelect())
        return FALSE;

    return accWrap->IsItemSelected(i);
}

gboolean
removeSelectionCB(AtkSelection *aSelection, gint i)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aSelection));
    if (!accWrap || !accWrap->IsSelect())
        return FALSE;

    return accWrap->RemoveItemFromSelection(i);
}

gboolean
selectAllSelectionCB(AtkSelection *aSelection)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aSelection));
    if (!accWrap || !accWrap->IsSelect())
        return FALSE;

    return accWrap->SelectAll();
}
