



#import <Cocoa/Cocoa.h>

#include "nsIXPConnect.h"
#include "nsIObserver.h"
#include "nsIDOMWindow.h"
#include "nsIJSContextStack.h"
#include "nsServiceManagerUtils.h"

@interface ObserverPair : NSObject
{
@public
  nsIObserver *observer;
  nsIDOMWindow *window;
}

- (id) initWithObserver:(nsIObserver *)aObserver window:(nsIDOMWindow *)aWindow;
- (void) dealloc;

@end









static already_AddRefed<nsIDOMWindow> __attribute__((unused))
GetWindowOfObserver(nsIObserver* aObserver)
{
  nsCOMPtr<nsIXPConnectWrappedJS> wrappedJS(do_QueryInterface(aObserver));
  if (!wrappedJS) {
    
    return nullptr;
  }

  JSObject* obj;
  nsresult rv = wrappedJS->GetJSObject(&obj);
  NS_ENSURE_SUCCESS(rv, nullptr);

  nsCOMPtr<nsIThreadJSContextStack> stack =
    do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
  NS_ENSURE_SUCCESS(rv, nullptr);

  JSContext* cx = stack->GetSafeJSContext();
  NS_ENSURE_TRUE(cx, nullptr);

  JSAutoRequest ar(cx);
  JSAutoCompartment ac(cx, obj);

  JSObject* global = JS_GetGlobalForObject(cx, obj);
  NS_ENSURE_TRUE(global, nullptr);

  nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID()));
  NS_ENSURE_TRUE(xpc, nullptr);

  nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
  rv = xpc->GetWrappedNativeOfJSObject(cx, global, getter_AddRefs(wrapper));
  NS_ENSURE_SUCCESS(rv, nullptr);

  nsCOMPtr<nsIDOMWindow> window = do_QueryWrappedNative(wrapper);
  NS_ENSURE_TRUE(window, nullptr);

  return window.forget();
}
