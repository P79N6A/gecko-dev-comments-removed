





#ifndef nsProperties_h___
#define nsProperties_h___

#include "nsIProperties.h"
#include "nsInterfaceHashtable.h"
#include "nsHashKeys.h"
#include "nsAgg.h"
#include "mozilla/Attributes.h"

#define NS_PROPERTIES_CID                            \
{ /* 4de2bc90-b1bf-11d3-93b6-00104ba0fd40 */         \
    0x4de2bc90,                                      \
    0xb1bf,                                          \
    0x11d3,                                          \
    {0x93, 0xb6, 0x00, 0x10, 0x4b, 0xa0, 0xfd, 0x40} \
}

typedef nsInterfaceHashtable<nsCharPtrHashKey,
                             nsISupports> nsProperties_HashBase;

class nsProperties final
  : public nsIProperties
  , public nsProperties_HashBase
{
public:
  NS_DECL_AGGREGATED
  NS_DECL_NSIPROPERTIES

  explicit nsProperties(nsISupports *aOuter) { NS_INIT_AGGREGATED(aOuter); }
  ~nsProperties() {}
};

#endif 
