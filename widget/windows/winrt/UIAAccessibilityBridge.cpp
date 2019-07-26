




#if defined(ACCESSIBILITY)

#include "UIAAccessibilityBridge.h"
#include "MetroUtils.h"

#include <OAIdl.h>

#include "nsIAccessibleEvent.h"
#include "nsIAccessibleEditableText.h"
#include "nsIPersistentProperties2.h"


#include "UIABridge.h"

#include <wrl/implements.h>


#if !defined(DEBUG_BRIDGE)
#undef LogThread
#undef LogFunction
#undef Log
#define LogThread() 
#define LogFunction()
#define Log(...)
#endif

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

  switch (eventType) {
    case nsIAccessibleEvent::EVENT_FOCUS:
      Log(L"EVENT_FOCUS");
      {
        nsCOMPtr<nsIAccessible> item;
        ev->GetAccessible(getter_AddRefs(item));
        Accessible* access = (Accessible*)item.get();
        Log(L"Focus element flags: %d %d %d",
          ((access->NativeState() & mozilla::a11y::states::EDITABLE) > 0),
          ((access->NativeState() & mozilla::a11y::states::FOCUSABLE) > 0),
          ((access->NativeState() & mozilla::a11y::states::READONLY) > 0)
          );
        bool focusable = (((access->NativeState() & mozilla::a11y::states::EDITABLE) > 0) &&
                          ((access->NativeState() & mozilla::a11y::states::READONLY) == 0));

        if (focusable) {
          Log(L"focus item can be focused");
          
          Microsoft::WRL::ComPtr<IUIAElement> bridgePtr;
          mBridge.As(&bridgePtr);
          if (bridgePtr) {
            bridgePtr->SetFocusInternal(reinterpret_cast<LONG_PTR>(item.get()));
          }
        } else {
          Log(L"focus item can't have focus");
          Microsoft::WRL::ComPtr<IUIAElement> bridgePtr;
          mBridge.As(&bridgePtr);
          if (bridgePtr) {
            bridgePtr->ClearFocus();
          }
        }
      }
    break;
    














































    default:
    return NS_OK;
  }

  return NS_OK;
}

} } }

#endif 
