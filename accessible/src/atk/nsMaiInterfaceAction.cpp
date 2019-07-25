






































#include "InterfaceInitFuncs.h"

#include "nsMai.h"
#include "Role.h"

#include "nsString.h"

using namespace mozilla::a11y;

extern "C" {

static gboolean
doActionCB(AtkAction *aAction, gint aActionIndex)
{
    nsAccessibleWrap *accWrap = GetAccessibleWrap(ATK_OBJECT(aAction));
    if (!accWrap)
        return FALSE;
 
    nsresult rv = accWrap->DoAction(aActionIndex);
    return (NS_FAILED(rv)) ? FALSE : TRUE;
}

static gint
getActionCountCB(AtkAction *aAction)
{
  nsAccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aAction));
  return accWrap ? accWrap->ActionCount() : 0;
}

static const gchar*
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

static const gchar*
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

static const gchar*
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
    roles::Role role = parent ? parent->Role() : roles::NOTHING;
    if (role == roles::PARENT_MENUITEM || role == roles::MENUITEM ||
        role == roles::RADIO_MENU_ITEM || role == roles::CHECK_MENU_ITEM) {
      
      
      nsAutoString keysInHierarchyStr = keyBindingsStr;
      do {
        KeyBinding parentKeyBinding = parent->AccessKey();
        if (!parentKeyBinding.IsEmpty()) {
          nsAutoString str;
          parentKeyBinding.ToString(str, KeyBinding::eAtkFormat);
          str.Append(':');

          keysInHierarchyStr.Insert(str, 0);
        }
      } while ((parent = parent->Parent()) && parent->Role() != roles::MENUBAR);

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
}

void
actionInterfaceInitCB(AtkActionIface* aIface)
{
  NS_ASSERTION(aIface, "Invalid aIface");
  if (NS_UNLIKELY(!aIface))
    return;

  aIface->do_action = doActionCB;
  aIface->get_n_actions = getActionCountCB;
  aIface->get_description = getActionDescriptionCB;
  aIface->get_keybinding = getKeyBindingCB;
  aIface->get_name = getActionNameCB;
}
