






































#include "nsAEUtils.h"


#ifdef __cplusplus
extern "C" {
#endif

OSErr CreateAEHandlerClasses(Boolean suspendFirstEvent);
OSErr GetSuspendedEvent(AppleEvent *event, AppleEvent *reply);
OSErr ResumeAEHandling(AppleEvent *event, AppleEvent *reply, Boolean dispatchEvent);
OSErr ShutdownAEHandlerClasses(void);

#ifdef __cplusplus
}
#endif

