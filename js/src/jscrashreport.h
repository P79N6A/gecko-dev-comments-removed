







































#ifndef jscrashreport_h___
#define jscrashreport_h___

#include "jstypes.h"

JS_BEGIN_EXTERN_C

JS_FRIEND_API(void)
js_SnapshotGCStack();

JS_FRIEND_API(void)
js_SnapshotErrorStack();

JS_FRIEND_API(void)
js_SaveCrashData(uint64 tag, void *ptr, size_t size);

JS_END_EXTERN_C

#endif 
