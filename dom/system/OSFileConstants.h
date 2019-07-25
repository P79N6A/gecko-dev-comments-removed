



#ifndef mozilla_osfileconstants_h__
#define mozilla_osfileconstants_h__

#include "jspubtd.h"

namespace mozilla {









nsresult InitOSFileConstants();









nsresult CleanupOSFileConstants();







bool DefineOSFileConstants(JSContext *cx, JSObject *global);

}

#endif 
