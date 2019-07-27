




#include <android/log.h>

#include <jni.h>

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "nsTArray.h"
#include "nsString.h"
#include "nsIFile.h"
#include "nsAppRunner.h"
#include "AndroidBridge.h"
#include "APKOpen.h"
#include "nsExceptionHandler.h"

#define LOG(args...) __android_log_print(ANDROID_LOG_INFO, MOZ_APP_NAME, args)









struct AutoAttachJavaThread {
    AutoAttachJavaThread() {
        attached = mozilla_AndroidBridge_SetMainThread(pthread_self());
    }
    ~AutoAttachJavaThread() {
        mozilla_AndroidBridge_SetMainThread(-1);
        attached = false;
    }

    bool attached;
};

extern "C" NS_EXPORT void
GeckoStart(void *data, const nsXREAppData *appData)
{
#ifdef MOZ_CRASHREPORTER
    const struct mapping_info *info = getLibraryMapping();
    while (info->name) {
      CrashReporter::AddLibraryMapping(info->name, info->base,
                                       info->len, info->offset);
      info++;
    }
#endif

    AutoAttachJavaThread attacher;
    if (!attacher.attached)
        return;

    if (!data) {
        LOG("Failed to get arguments for GeckoStart\n");
        return;
    }

    nsTArray<char *> targs;
    char *arg = strtok(static_cast<char *>(data), " ");
    while (arg) {
        targs.AppendElement(arg);
        arg = strtok(nullptr, " ");
    }
    targs.AppendElement(static_cast<char *>(nullptr));

    int result = XRE_main(targs.Length() - 1, targs.Elements(), appData, 0);

    if (result)
        LOG("XRE_main returned %d", result);

    mozilla::widget::GeckoAppShell::NotifyXreExit();
}
