




#include "mozilla/dom/ContentChild.h"
#include "nsHapticFeedback.h"
#include "AndroidBridge.h"

using namespace mozilla;

NS_IMPL_ISUPPORTS(nsHapticFeedback, nsIHapticFeedback)

NS_IMETHODIMP
nsHapticFeedback::PerformSimpleAction(int32_t aType)
{
    mozilla::widget::android::GeckoAppShell::PerformHapticFeedback(aType == LongPress);
    return NS_OK;
}
