




#ifndef _mozilla_uevent_poller_h_
#define _mozilla_uevent_poller_h_

#include <sysutils/NetlinkEvent.h>
#include "mozilla/Observer.h"

class NetlinkEvent;

namespace mozilla {
namespace hal_impl {

typedef mozilla::Observer<NetlinkEvent> IUeventObserver;







void RegisterUeventListener(IUeventObserver *aObserver);






void UnregisterUeventListener(IUeventObserver *aObserver);

}
}

#endif

