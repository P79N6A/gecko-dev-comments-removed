





#include "InterfaceInitFuncs.h"

#include "Accessible-inl.h"
#include "nsMai.h"
#include "Role.h"
#include "mozilla/Likely.h"

#include "nsString.h"

using namespace mozilla::a11y;

extern "C" {

static gboolean
doActionCB(AtkAction *aAction, gint aActionIndex)
{
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aAction));
  return accWrap && accWrap->DoAction(aActionIndex);
}

static gint
getActionCountCB(AtkAction *aAction)
{
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aAction));
  return accWrap ? accWrap->ActionCount() : 0;
}

static const gchar*
getActionDescriptionCB(AtkAction *aAction, gint aActionIndex)
{
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aAction));
  if (!accWrap)
    return nullptr;

  nsAutoString description;
  nsresult rv = accWrap->GetActionDescription(aActionIndex, description);
  NS_ENSURE_SUCCESS(rv, nullptr);
  return AccessibleWrap::ReturnString(description);
}

static const gchar*
getActionNameCB(AtkAction *aAction, gint aActionIndex)
{
  AccessibleWrap* accWrap = GetAccessibleWrap(ATK_OBJECT(aAction));
  if (!accWrap)
    return nullptr;

  nsAutoString autoStr;
  accWrap->ActionNameAt(aActionIndex, autoStr);
  return AccessibleWrap::ReturnString(autoStr);
}

static const gchar*
getKeyBindingCB(AtkAction *aAction, gint aActionIndex)
{
  AccessibleWrap* acc = GetAccessibleWrap(ATK_OBJECT(aAction));
  if (!acc)
    return nullptr;

  
  nsAutoString keyBindingsStr;

  
  KeyBinding keyBinding = acc->AccessKey();
  if (!keyBinding.IsEmpty()) {
    keyBinding.AppendToString(keyBindingsStr, KeyBinding::eAtkFormat);

    Accessible* parent = acc->Parent();
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

  return AccessibleWrap::ReturnString(keyBindingsStr);
}
}

void
actionInterfaceInitCB(AtkActionIface* aIface)
{
  NS_ASSERTION(aIface, "Invalid aIface");
  if (MOZ_UNLIKELY(!aIface))
    return;

  aIface->do_action = doActionCB;
  aIface->get_n_actions = getActionCountCB;
  aIface->get_description = getActionDescriptionCB;
  aIface->get_keybinding = getKeyBindingCB;
  aIface->get_name = getActionNameCB;
}
