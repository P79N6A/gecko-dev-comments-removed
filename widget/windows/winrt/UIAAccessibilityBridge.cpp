




#if defined(ACCESSIBILITY)

#include "UIAAccessibilityBridge.h"
#include "MetroUtils.h"

#include <OAIdl.h>

#include "nsIAccessibleEvent.h"
#include "nsIAccessibleEditableText.h"
#include "nsIPersistentProperties2.h"


#include "UIABridge.h"

#include <wrl/implements.h>

using namespace mozilla::a11y;

namespace mozilla {
namespace widget {
namespace winrt {

using namespace Microsoft::WRL;

NS_IMPL_ISUPPORTS1(AccessibilityBridge, nsIObserver)

nsresult
AccessibilityBridge::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *aData)
{
  nsCOMPtr<nsIAccessibleEvent> ev = do_QueryInterface(aSubject);
  if (!ev) {
    return NS_OK;
  }

  uint32_t eventType = 0;
  ev->GetEventType(&eventType);
  if (eventType == nsIAccessibleEvent::EVENT_FOCUS) {
    Microsoft::WRL::ComPtr<IUIABridge> bridgePtr;
    mBridge.As(&bridgePtr);
    if (bridgePtr) {
      bridgePtr->FocusChangeEvent();
    }
  }

  return NS_OK;
}

} } }

#endif 
