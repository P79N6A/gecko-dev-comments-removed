





#include "nsXPCOM.h"
#include "nsXULAppAPI.h"


#if !defined(OS_WIN)
#include <unistd.h>
#endif

#ifdef XP_WIN
#include <windows.h>


#define XRE_DONT_PROTECT_DLL_LOAD
#include "nsWindowsWMain.cpp"
#include "nsSetDllDirectory.h"
#endif

#if defined(XP_WIN) && defined(MOZ_SANDBOX)
#include "sandbox/chromium/base/basictypes.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_factory.h"
#include "mozilla/sandboxTarget.h"
#endif

#ifdef MOZ_WIDGET_GONK
# include <sys/time.h>
# include <sys/resource.h> 

# include <binder/ProcessState.h>

# ifdef LOGE_IF
#  undef LOGE_IF
# endif

# include <android/log.h>
# define LOGE_IF(cond, ...) \
     ( (CONDITION(cond)) \
     ? ((void)__android_log_print(ANDROID_LOG_ERROR, \
       "Gecko:MozillaRntimeMain", __VA_ARGS__)) \
     : (void)0 )

#endif

#ifdef MOZ_NUWA_PROCESS
#include <binder/ProcessState.h>
#include "ipc/Nuwa.h"
#endif

#ifdef MOZ_WIDGET_GONK
static void
InitializeBinder(void *aDummy) {
    
    
    
    
    int curPrio = getpriority(PRIO_PROCESS, 0);
    int err = setpriority(PRIO_PROCESS, 0, 0);
    MOZ_ASSERT(!err);
    LOGE_IF(err, "setpriority failed. Current process needs root permission.");
    android::ProcessState::self()->startThreadPool();
    setpriority(PRIO_PROCESS, 0, curPrio);
}
#endif

#if defined(XP_WIN) && defined(MOZ_SANDBOX)
static bool gIsSandboxEnabled = false;
void StartSandboxCallback()
{
    if (gIsSandboxEnabled) {
        sandbox::TargetServices* target_service =
            sandbox::SandboxFactory::GetTargetServices();
        target_service->LowerToken();
    }
}
#endif

int
content_process_main(int argc, char* argv[])
{
    
    
    if (argc < 1) {
      return 3;
    }
    XRE_SetProcessType(argv[--argc]);

    bool isNuwa = false;
    for (int i = 1; i < argc; i++) {
        isNuwa |= strcmp(argv[i], "-nuwa") == 0;
#if defined(XP_WIN) && defined(MOZ_SANDBOX)
        gIsSandboxEnabled |= strcmp(argv[i], "-sandbox") == 0;
#endif
    }

#ifdef MOZ_NUWA_PROCESS
    if (isNuwa) {
        PrepareNuwaProcess();
    }
#endif

#ifdef MOZ_WIDGET_GONK
    
    
    
    

#ifdef MOZ_NUWA_PROCESS
    if (!isNuwa) {
        InitializeBinder(nullptr);
    } else {
        NuwaAddFinalConstructor(&InitializeBinder, nullptr);
    }
#else
    InitializeBinder(nullptr);
#endif
#endif

#ifdef XP_WIN
    
    
    
    if (XRE_GetProcessType() != GeckoProcessType_Plugin) {
        mozilla::SanitizeEnvironmentVariables();
        SetDllDirectory(L"");
    }

#ifdef MOZ_SANDBOX
    if (gIsSandboxEnabled) {
        sandbox::TargetServices* target_service =
            sandbox::SandboxFactory::GetTargetServices();
        if (!target_service) {
            return 1;
        }

        sandbox::ResultCode result = target_service->Init();
        if (result != sandbox::SBOX_ALL_OK) {
           return 2;
        }
        mozilla::SandboxTarget::Instance()->SetStartSandboxCallback(StartSandboxCallback);
    }
#endif
#endif

    nsresult rv = XRE_InitChildProcess(argc, argv);
    NS_ENSURE_SUCCESS(rv, 1);

    return 0;
}
