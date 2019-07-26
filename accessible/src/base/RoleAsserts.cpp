





#include "nsIAccessibleRole.h"
#include "Role.h"

using namespace mozilla::a11y;

#define ROLE(geckoRole, stringRole, atkRole, macRole, msaaRole, ia2Role, nameRule) \
  static_assert(static_cast<uint32_t>(roles::geckoRole) \
                == static_cast<uint32_t>(nsIAccessibleRole::ROLE_ ## geckoRole), \
                "internal and xpcom roles differ!");
#include "RoleMap.h"
#undef ROLE
