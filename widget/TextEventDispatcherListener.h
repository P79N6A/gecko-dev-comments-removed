



#ifndef mozilla_textinputdispatcherlistener_h_
#define mozilla_textinputdispatcherlistener_h_

#include "nsWeakReference.h"

namespace mozilla {
namespace widget {

class TextEventDispatcher;
struct IMENotification;

#define NS_TEXT_INPUT_PROXY_LISTENER_IID \
{ 0xf2226f55, 0x6ddb, 0x40d5, \
  { 0x8a, 0x24, 0xce, 0x4d, 0x5b, 0x38, 0x15, 0xf0 } };

class TextEventDispatcherListener : public nsSupportsWeakReference
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_TEXT_INPUT_PROXY_LISTENER_IID)

  




  NS_IMETHOD NotifyIME(TextEventDispatcher* aTextEventDispatcher,
                       const IMENotification& aNotification) = 0;

  



  NS_IMETHOD_(void) OnRemovedFrom(
                      TextEventDispatcher* aTextEventDispatcher) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(TextEventDispatcherListener,
                              NS_TEXT_INPUT_PROXY_LISTENER_IID)

} 
} 

#endif 
