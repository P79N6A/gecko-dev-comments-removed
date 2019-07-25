




































#include "mozilla/dom/ContentChild.h"
#include "nsHapticFeedback.h"
#include "AndroidBridge.h"

using namespace mozilla;

NS_IMPL_ISUPPORTS1(nsHapticFeedback, nsIHapticFeedback)

NS_IMETHODIMP
nsHapticFeedback::PerformSimpleAction(PRInt32 aType)
{
    AndroidBridge* bridge = AndroidBridge::Bridge();
    if (bridge) {
        bridge->PerformHapticFeedback(aType == LongPress);
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}
