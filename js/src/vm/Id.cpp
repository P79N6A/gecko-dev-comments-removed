





#include "js/Id.h"
#include "js/RootingAPI.h"

static MOZ_CONSTEXPR_VAR jsid voidIdValue = jsid::voidId();
static MOZ_CONSTEXPR_VAR jsid emptyIdValue = jsid::emptyId();

const JS::HandleId JSID_VOIDHANDLE = JS::HandleId::fromMarkedLocation(&voidIdValue);
const JS::HandleId JSID_EMPTYHANDLE = JS::HandleId::fromMarkedLocation(&emptyIdValue);

