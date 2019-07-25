






































#include "nsMaiInterfaceAction.h"

#include "nsAccUtils.h"
#include "nsRoleMap.h"
#include "nsString.h"

#include "nsIDOMDOMStringList.h"

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
    if (!accWrap)
        return FALSE;
 
    nsresult rv = accWrap->DoAction(aActionIndex);
    return (NS_FAILED(rv)) ? FALSE : TRUE;
}

gint
getActionCountCB(AtkAction *aAction)
{
  nsAccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aAction));
  return accWrap ? accWrap->ActionCount() : 0;
}

const gchar *
getActionDescriptionCB(AtkAction *aAction, gint aActionIndex)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aAction));
    if (!accWrap)
        return nsnull;

    nsAutoString description;
    nsresult rv = accWrap->GetActionDescription(aActionIndex, description);
    NS_ENSURE_SUCCESS(rv, nsnull);
    return nsAccessibleWrap::ReturnString(description);
}

const gchar *
getActionNameCB(AtkAction *aAction, gint aActionIndex)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aAction));
    if (!accWrap)
        return nsnull;

    nsAutoString autoStr;
    nsresult rv = accWrap->GetActionName(aActionIndex, autoStr);
    NS_ENSURE_SUCCESS(rv, nsnull);
    return nsAccessibleWrap::ReturnString(autoStr);
}

const gchar *
getKeyBindingCB(AtkAction *aAction, gint aActionIndex)
{
  nsAccessibleWrap* acc = GetAccessibleWrap(ATK_OBJECT(aAction));
  if (!acc)
    return nsnull;

  
  nsAutoString keyBindingsStr;

  
  KeyBinding keyBinding = acc->AccessKey();
  if (!keyBinding.IsEmpty()) {
    keyBinding.AppendToString(keyBindingsStr, KeyBinding::eAtkFormat);

    nsAccessible* parent = acc->Parent();
    PRUint32 role = parent ? parent->Role() : 0;
    if (role == nsIAccessibleRole::ROLE_PARENT_MENUITEM ||
        role == nsIAccessibleRole::ROLE_MENUITEM ||
        role == nsIAccessibleRole::ROLE_RADIO_MENU_ITEM ||
        role == nsIAccessibleRole::ROLE_CHECK_MENU_ITEM) {
      
      
      nsAutoString keysInHierarchyStr = keyBindingsStr;
      do {
        KeyBinding parentKeyBinding = parent->AccessKey();
        if (!parentKeyBinding.IsEmpty()) {
          nsAutoString str;
          parentKeyBinding.ToString(str, KeyBinding::eAtkFormat);
          str.Append(':');

          keysInHierarchyStr.Insert(str, 0);
        }
      } while ((parent = parent->Parent()) &&
               parent->Role() != nsIAccessibleRole::ROLE_MENUBAR);

      keyBindingsStr.Append(';');
      keyBindingsStr.Append(keysInHierarchyStr);
    }
  } else {
    
    keyBindingsStr.Append(';');
  }

  
  keyBindingsStr.Append(';');
  keyBinding = acc->KeyboardShortcut();
  if (!keyBinding.IsEmpty()) {
    keyBinding.AppendToString(keyBindingsStr, KeyBinding::eAtkFormat);
  }

  return nsAccessibleWrap::ReturnString(keyBindingsStr);
}
