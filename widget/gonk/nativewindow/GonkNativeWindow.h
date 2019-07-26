














#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
# include "GonkNativeWindowJB.h"
#elif defined(MOZ_WIDGET_GONK) && ANDROID_VERSION == 15
# include "GonkNativeWindowICS.h"
#endif
