





































#include "nsIGenericFactory.h"
#include "EmbedGlobalHistory.h"
#include "nsIGlobalHistory.h"

#if 0
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(EmbedGlobalHistory, Init)

static const nsModuleComponentInfo components[] =
{
    { "Global History",
      NS_EMBEDGLOBALHISTORY_CID,
      "@mozilla.org/browser/global-history;1",
      EmbedGlobalHistoryConstructor,
    }
};

NS_IMPL_NSGETMODULE(QtComponents, components)
#endif
