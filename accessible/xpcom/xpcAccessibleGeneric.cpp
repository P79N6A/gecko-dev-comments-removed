





#include "xpcAccessibleGeneric.h"

using namespace mozilla::a11y;




NS_IMPL_CYCLE_COLLECTION_0(xpcAccessibleGeneric)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(xpcAccessibleGeneric)
  NS_INTERFACE_MAP_ENTRY(nsIAccessible)
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIAccessibleSelectable,
                                     mSupportedIfaces & eSelectable)
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIAccessibleValue,
                                     mSupportedIfaces & eValue)
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIAccessibleHyperLink,
                                     mSupportedIfaces & eHyperLink)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIAccessible)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(xpcAccessibleGeneric)
NS_IMPL_CYCLE_COLLECTING_RELEASE(xpcAccessibleGeneric)




Accessible*
xpcAccessibleGeneric::ToInternalAccessible() const
{
  return mIntl;
}




void
xpcAccessibleGeneric::Shutdown()
{
  mIntl = nullptr;
}
