





#include "nsXPCOM.h"
#include "nsXULAppAPI.h"
#include "nsAutoPtr.h"


#if !defined(OS_WIN)
#include <unistd.h>
#endif

#ifdef XP_WIN
#include <windows.h>


#define XRE_DONT_PROTECT_DLL_LOAD
#include "nsWindowsWMain.cpp"
#include "nsSetDllDirectory.h"
#endif

#include "GMPLoader.h"

#if defined(XP_WIN) && defined(MOZ_SANDBOX)
#include "mozilla/sandboxTarget.h"
#include "mozilla/sandboxing/loggingCallbacks.h"
#include "sandbox/win/src/sandbox_factory.h"
#endif

#if defined(XP_LINUX) && defined(MOZ_GMP_SANDBOX)
#include "mozilla/Sandbox.h"
#include "mozilla/SandboxInfo.h"
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

# ifdef MOZ_CONTENT_SANDBOX
# include "mozilla/Sandbox.h"
# endif

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

class WinSandboxStarter : public mozilla::gmp::SandboxStarter {
public:
    virtual bool Start(const char *aLibPath) override {
        if (gIsSandboxEnabled) {
            sandbox::SandboxFactory::GetTargetServices()->LowerToken();
        }
        return true;
    }
};
#endif

#if defined(XP_LINUX) && defined(MOZ_GMP_SANDBOX)
class LinuxSandboxStarter : public mozilla::gmp::SandboxStarter {
    LinuxSandboxStarter() { }
public:
    static SandboxStarter* Make() {
        if (mozilla::SandboxInfo::Get().CanSandboxMedia()) {
            return new LinuxSandboxStarter();
        } else {
            
            
            return nullptr;
        }
    }
    virtual bool Start(const char *aLibPath) override {
        mozilla::SetMediaPluginSandbox(aLibPath);
        return true;
    }
};
#endif

#if defined(XP_MACOSX) && defined(MOZ_GMP_SANDBOX)
class MacSandboxStarter : public mozilla::gmp::SandboxStarter {
public:
    virtual bool Start(const char *aLibPath) override {
      std::string err;
      bool rv = mozilla::StartMacSandbox(mInfo, err);
      if (!rv) {
        fprintf(stderr, "sandbox_init() failed! Error \"%s\"\n", err.c_str());
      }
      return rv;
    }
    virtual void SetSandboxInfo(MacSandboxInfo* aSandboxInfo) override {
      mInfo = *aSandboxInfo;
    }
private:
  MacSandboxInfo mInfo;
};
#endif

mozilla::gmp::SandboxStarter*
MakeSandboxStarter()
{
#if defined(XP_WIN) && defined(MOZ_SANDBOX)
    return new WinSandboxStarter();
#elif defined(XP_LINUX) && defined(MOZ_GMP_SANDBOX)
    return LinuxSandboxStarter::Make();
#elif defined(XP_MACOSX) && defined(MOZ_GMP_SANDBOX)
    return new MacSandboxStarter();
#else
    return nullptr;
#endif
}

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

#if defined(XP_LINUX) && defined(MOZ_SANDBOX)
    
    
    
    
    
    
    mozilla::SandboxEarlyInit(XRE_GetProcessType(), isNuwa);
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

        sandbox::ResultCode result =
            mozilla::SandboxTarget::Instance()->InitTargetServices(target_service);
        if (result != sandbox::SBOX_ALL_OK) {
           return 2;
        }

        mozilla::sandboxing::PrepareForLogging();
    }
#endif
#endif
    nsAutoPtr<mozilla::gmp::GMPLoader> loader;
#if !defined(MOZ_WIDGET_ANDROID) && !defined(MOZ_WIDGET_GONK)
    
    
    nsAutoPtr<mozilla::gmp::SandboxStarter> starter(MakeSandboxStarter());
    if (XRE_GetProcessType() == GeckoProcessType_GMPlugin) {
        loader = mozilla::gmp::CreateGMPLoader(starter);
    }
#endif
    nsresult rv = XRE_InitChildProcess(argc, argv, loader);
    NS_ENSURE_SUCCESS(rv, 1);

    return 0;
}
