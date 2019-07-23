







































#include "nsMaiInterfaceAction.h"
#include "nsString.h"

void
actionInterfaceInitCB(AtkActionIface *aIface)
{
    NS_ASSERTION(aIface, "Invalid aIface");
    if (!aIface)
        return;

    aIface->do_action = doActionCB;
    aIface->get_n_actions = getActionCountCB;
    aIface->get_description = getActionDescriptionCB;
    aIface->get_keybinding = getKeyBindingCB;
    aIface->get_name = getActionNameCB;
}

gboolean
doActionCB(AtkAction *aAction, gint aActionIndex)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aAction));
    NS_ENSURE_TRUE(accWrap, FALSE);
 
    nsresult rv = accWrap->DoAction(aActionIndex);
    return (NS_FAILED(rv)) ? FALSE : TRUE;
}

gint
getActionCountCB(AtkAction *aAction)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aAction));
    NS_ENSURE_TRUE(accWrap, 0);

    PRUint8 num = 0;
    nsresult rv = accWrap->GetNumActions(&num);
    return (NS_FAILED(rv)) ? 0 : NS_STATIC_CAST(gint, num);
}

const gchar *
getActionDescriptionCB(AtkAction *aAction, gint aActionIndex)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aAction));
    NS_ENSURE_TRUE(accWrap, nsnull);

    nsAutoString description;
    nsresult rv = accWrap->GetActionDescription(aActionIndex, description);
    NS_ENSURE_SUCCESS(rv, nsnull);
    return nsAccessibleWrap::ReturnString(description);
}

const gchar *
getActionNameCB(AtkAction *aAction, gint aActionIndex)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aAction));
    NS_ENSURE_TRUE(accWrap, nsnull);

    nsAutoString autoStr;
    nsresult rv = accWrap->GetActionName(aActionIndex, autoStr);
    NS_ENSURE_SUCCESS(rv, nsnull);
    return nsAccessibleWrap::ReturnString(autoStr);
}

const gchar *
getKeyBindingCB(AtkAction *aAction, gint aActionIndex)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aAction));
    NS_ENSURE_TRUE(accWrap, nsnull);

    
    nsAutoString allKeyBinding;

    
    nsAutoString accessKey;
    nsresult rv = accWrap->GetKeyboardShortcut(accessKey);

    if (NS_SUCCEEDED(rv) && !accessKey.IsEmpty()) {
        nsCOMPtr<nsIAccessible> parentAccessible;
        accWrap->GetParent(getter_AddRefs(parentAccessible));
        if (parentAccessible) {
            PRUint32 role;
            parentAccessible->GetRole(&role);

            if (role == ATK_ROLE_MENU_BAR) {
                
                nsAutoString rightChar;
                accessKey.Right(rightChar, 1);
                allKeyBinding = rightChar + NS_LITERAL_STRING(";<Alt>") +
                                rightChar;
            }
            else if ((role == ATK_ROLE_MENU) || (role == ATK_ROLE_MENU_ITEM)) {
                
                nsAutoString allKey = accessKey;
                nsCOMPtr<nsIAccessible> grandParentAcc = parentAccessible;

                while ((grandParentAcc) && (role != ATK_ROLE_MENU_BAR)) {
                    nsAutoString grandParentKey;
                    grandParentAcc->GetKeyboardShortcut(grandParentKey);

                    if (!grandParentKey.IsEmpty()) {
                        nsAutoString rightChar;
                        grandParentKey.Right(rightChar, 1);
                        allKey = rightChar + NS_LITERAL_STRING(":") + allKey;
                    }

                    nsCOMPtr<nsIAccessible> tempAcc = grandParentAcc;
                    tempAcc->GetParent(getter_AddRefs(grandParentAcc));
                    if (grandParentAcc)
                        grandParentAcc->GetRole(&role);
                }
                allKeyBinding = accessKey + NS_LITERAL_STRING(";<Alt>") +
                                allKey;
            }
        }
        else {
            
            nsAutoString rightChar;
            accessKey.Right(rightChar, 1);
            allKeyBinding = rightChar + NS_LITERAL_STRING(";<Alt>") + rightChar;
        }
    }
    else  
        allKeyBinding.AssignLiteral(";");

    
    nsAutoString subShortcut;
    nsCOMPtr<nsIDOMDOMStringList> keyBindings;
    rv = accWrap->GetKeyBindings(aActionIndex, getter_AddRefs(keyBindings));

    if (NS_SUCCEEDED(rv) && keyBindings) {
        PRUint32 length = 0;
        keyBindings->GetLength(&length);
        for (PRUint32 i = 0; i < length; i++) {
            nsAutoString keyBinding;
            keyBindings->Item(i, keyBinding);

            
            PRInt32 oldPos, curPos=0;
            while ((curPos != -1) && (curPos < (PRInt32)keyBinding.Length())) {
                oldPos = curPos;
                nsAutoString subString;
                curPos = keyBinding.FindChar('+', oldPos);
                if (curPos == -1) {
                    keyBinding.Mid(subString, oldPos, keyBinding.Length() - oldPos);
                    subShortcut += subString;
                } else {
                    keyBinding.Mid(subString, oldPos, curPos - oldPos);

                    
                    if (subString.LowerCaseEqualsLiteral("ctrl"))
                        subString.AssignLiteral("Control");

                    subShortcut += NS_LITERAL_STRING("<") + subString +
                                   NS_LITERAL_STRING(">");
                    curPos++;
                }
            }
        }
    }

    allKeyBinding += NS_LITERAL_STRING(";") + subShortcut;
    return nsAccessibleWrap::ReturnString(allKeyBinding);
}
