








































#include <android/log.h>

#include <jni.h>

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "nsTArray.h"
#include "nsString.h"
#include "nsILocalFile.h"
#include "nsAppRunner.h"
#include "AndroidBridge.h"
#include "APKOpen.h"
#include "nsExceptionHandler.h"

#define LOG(args...) __android_log_print(ANDROID_LOG_INFO, MOZ_APP_NAME, args)

struct AutoAttachJavaThread {
    AutoAttachJavaThread() {
        attached = mozilla_AndroidBridge_SetMainThread((void*)pthread_self());
    }
    ~AutoAttachJavaThread() {
        mozilla_AndroidBridge_SetMainThread(nsnull);
        attached = PR_FALSE;
    }

    bool attached;
};

static void*
GeckoStart(void *data)
{
#ifdef MOZ_CRASHREPORTER
    const struct mapping_info *info = getLibraryMapping();
    while (info->name) {
      CrashReporter::AddLibraryMapping(info->name, info->file_id, info->base,
                                       info->len, info->offset);
      info++;
    }
#endif

    AutoAttachJavaThread attacher;
    if (!attacher.attached)
        return 0;

    if (!data) {
        LOG("Failed to get arguments for GeckoStart\n");
        return 0;
    }

    nsresult rv;
    nsCOMPtr<nsILocalFile> appini;
    char* greHome = getenv("GRE_HOME");
    if (!greHome) {
        LOG("Failed to get GRE_HOME from the env vars");
        return 0;
    }
    nsCAutoString appini_path(greHome);
    appini_path.AppendLiteral("/application.ini");
    rv = NS_NewNativeLocalFile(appini_path, PR_FALSE, getter_AddRefs(appini));
    if (NS_FAILED(rv)) {
        LOG("Failed to create nsILocalFile for appdata\n");
        return 0;
    }

    nsXREAppData *appData;
    rv = XRE_CreateAppData(appini, &appData);
    if (NS_FAILED(rv)) {
        LOG("Failed to load application.ini from %s\n", appini_path.get());
        return 0;
    }

    nsCOMPtr<nsILocalFile> xreDir;
    rv = NS_NewNativeLocalFile(nsDependentCString(greHome), PR_FALSE, getter_AddRefs(xreDir));
    if (NS_FAILED(rv)) {
        LOG("Failed to create nsIFile for xreDirectory");
        return 0;
    }

    appData->xreDirectory = xreDir.get();

    nsTArray<char *> targs;
    char *arg = strtok(static_cast<char *>(data), " ");
    while (arg) {
        targs.AppendElement(arg);
        arg = strtok(NULL, " ");
    }
    targs.AppendElement(static_cast<char *>(nsnull));

    int result = XRE_main(targs.Length() - 1, targs.Elements(), appData);

    if (result)
        LOG("XRE_main returned %d", result);

    XRE_FreeAppData(appData);

    mozilla::AndroidBridge::Bridge()->NotifyXreExit();

    free(targs[0]);
    nsMemory::Free(data);
    return 0;
}

extern "C" NS_EXPORT void JNICALL
Java_org_mozilla_gecko_GeckoAppShell_nativeRun(JNIEnv *jenv, jclass jc, jstring jargs)
{
    
    
    
    

    
    

    nsAutoString wargs;
    int len = jenv->GetStringLength(jargs);
    wargs.SetLength(jenv->GetStringLength(jargs));
    jenv->GetStringRegion(jargs, 0, len, wargs.BeginWriting());
    char *args = ToNewUTF8String(wargs);

    GeckoStart(args);
}

