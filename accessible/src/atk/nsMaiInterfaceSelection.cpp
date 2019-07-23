







































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
    if (!accWrap)
        return FALSE;

    nsCOMPtr<nsIAccessibleSelectable> accSelection;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleSelectable),
                            getter_AddRefs(accSelection));
    NS_ENSURE_TRUE(accSelection, FALSE);

    return NS_SUCCEEDED(accSelection->AddChildToSelection(i));
}

gboolean
clearSelectionCB(AtkSelection *aSelection)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aSelection));
    if (!accWrap)
        return FALSE;

    nsCOMPtr<nsIAccessibleSelectable> accSelection;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleSelectable),
                            getter_AddRefs(accSelection));
    NS_ENSURE_TRUE(accSelection, FALSE);

    return NS_SUCCEEDED(accSelection->ClearSelection());
}

AtkObject *
refSelectionCB(AtkSelection *aSelection, gint i)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aSelection));
    if (!accWrap)
        return nsnull;

    nsCOMPtr<nsIAccessibleSelectable> accSelection;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleSelectable),
                            getter_AddRefs(accSelection));
    NS_ENSURE_TRUE(accSelection, nsnull);

    nsCOMPtr<nsIAccessible> accSelect;
    accSelection->RefSelection(i, getter_AddRefs(accSelect));
    if (!accSelect) {
        return nsnull;
    }

    AtkObject *atkObj = nsAccessibleWrap::GetAtkObject(accSelect);
    if (atkObj) {
        g_object_ref(atkObj);
    }
    return atkObj;
}

gint
getSelectionCountCB(AtkSelection *aSelection)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aSelection));
    if (!accWrap)
        return -1;

    nsCOMPtr<nsIAccessibleSelectable> accSelection;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleSelectable),
                            getter_AddRefs(accSelection));
    NS_ENSURE_TRUE(accSelection, -1);

    PRInt32 num = 0;
    nsresult rv = accSelection->GetSelectionCount(&num);
    return (NS_FAILED(rv)) ? -1 : num;
}

gboolean
isChildSelectedCB(AtkSelection *aSelection, gint i)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aSelection));
    if (!accWrap)
        return FALSE;

    nsCOMPtr<nsIAccessibleSelectable> accSelection;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleSelectable),
                            getter_AddRefs(accSelection));
    NS_ENSURE_TRUE(accSelection, FALSE);

    PRBool result = FALSE;
    nsresult rv = accSelection->IsChildSelected(i, &result);
    return (NS_FAILED(rv)) ? FALSE : result;
}

gboolean
removeSelectionCB(AtkSelection *aSelection, gint i)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aSelection));
    if (!accWrap)
        return FALSE;

    nsCOMPtr<nsIAccessibleSelectable> accSelection;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleSelectable),
                            getter_AddRefs(accSelection));
    NS_ENSURE_TRUE(accSelection, FALSE);

    nsresult rv = accSelection->RemoveChildFromSelection(i);
    return (NS_FAILED(rv)) ? FALSE : TRUE;
}

gboolean
selectAllSelectionCB(AtkSelection *aSelection)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aSelection));
    if (!accWrap)
        return FALSE;

    nsCOMPtr<nsIAccessibleSelectable> accSelection;
    accWrap->QueryInterface(NS_GET_IID(nsIAccessibleSelectable),
                            getter_AddRefs(accSelection));
    NS_ENSURE_TRUE(accSelection, FALSE);

    PRBool result = FALSE;
    nsresult rv = accSelection->SelectAllSelection(&result);
    return (NS_FAILED(rv)) ? FALSE : result;
}
