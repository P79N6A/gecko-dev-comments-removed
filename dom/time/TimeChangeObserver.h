




#ifndef _mozilla_time_change_observer_h_
#define _mozilla_time_change_observer_h_

#include "nscore.h"

class nsPIDOMWindow;

namespace mozilla {
namespace time {

nsresult AddWindowListener(nsPIDOMWindow* aWindow);
nsresult RemoveWindowListener(nsPIDOMWindow* aWindow);

} 
} 

#endif 
