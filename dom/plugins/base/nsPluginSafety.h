




#ifndef nsPluginSafety_h_
#define nsPluginSafety_h_

#include <prinrval.h>



#ifdef MOZ_WIDGET_ANDROID
  #include "AndroidBridge.h"

  #define MAIN_THREAD_JNI_REF_GUARD mozilla::AutoLocalJNIFrame jniFrame
#else
  #define MAIN_THREAD_JNI_REF_GUARD
#endif

PRIntervalTime NS_NotifyBeginPluginCall(NSPluginCallReentry aReentryState);
void NS_NotifyPluginCall(PRIntervalTime aTime, NSPluginCallReentry aReentryState);

#define NS_TRY_SAFE_CALL_RETURN(ret, fun, pluginInst, pluginCallReentry) \
PR_BEGIN_MACRO                                     \
  MAIN_THREAD_JNI_REF_GUARD;                       \
  PRIntervalTime startTime = NS_NotifyBeginPluginCall(pluginCallReentry); \
  ret = fun;                                       \
  NS_NotifyPluginCall(startTime, pluginCallReentry); \
PR_END_MACRO

#define NS_TRY_SAFE_CALL_VOID(fun, pluginInst, pluginCallReentry) \
PR_BEGIN_MACRO                                     \
  MAIN_THREAD_JNI_REF_GUARD;                       \
  PRIntervalTime startTime = NS_NotifyBeginPluginCall(pluginCallReentry); \
  fun;                                             \
  NS_NotifyPluginCall(startTime, pluginCallReentry); \
PR_END_MACRO

#endif 
