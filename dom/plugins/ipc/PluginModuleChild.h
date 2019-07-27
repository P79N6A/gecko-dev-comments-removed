





#ifndef dom_plugins_PluginModuleChild_h
#define dom_plugins_PluginModuleChild_h 1

#include "mozilla/Attributes.h"

#include <string>
#include <vector>

#include "base/basictypes.h"

#include "prlink.h"

#include "npapi.h"
#include "npfunctions.h"

#include "nsAutoPtr.h"
#include "nsDataHashtable.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"

#ifdef MOZ_WIDGET_COCOA
#include "PluginInterposeOSX.h"
#endif

#include "mozilla/plugins/PPluginModuleChild.h"
#include "mozilla/plugins/PluginInstanceChild.h"
#include "mozilla/plugins/PluginMessageUtils.h"



#if defined(XP_WIN)
#define NS_NPAPIPLUGIN_CALLBACK(_type, _name) _type (__stdcall * _name)
#else
#define NS_NPAPIPLUGIN_CALLBACK(_type, _name) _type (* _name)
#endif

typedef NS_NPAPIPLUGIN_CALLBACK(NPError, NP_GETENTRYPOINTS) (NPPluginFuncs* pCallbacks);
typedef NS_NPAPIPLUGIN_CALLBACK(NPError, NP_PLUGININIT) (const NPNetscapeFuncs* pCallbacks);
typedef NS_NPAPIPLUGIN_CALLBACK(NPError, NP_PLUGINUNIXINIT) (const NPNetscapeFuncs* pCallbacks, NPPluginFuncs* fCallbacks);
typedef NS_NPAPIPLUGIN_CALLBACK(NPError, NP_PLUGINSHUTDOWN) (void);

namespace mozilla {
namespace dom {
class PCrashReporterChild;
}

namespace plugins {

#ifdef MOZ_WIDGET_QT
class NestedLoopTimer;
static const int kNestedLoopDetectorIntervalMs = 90;
#endif

class PluginScriptableObjectChild;
class PluginInstanceChild;

class PluginModuleChild : public PPluginModuleChild
{
    typedef mozilla::dom::PCrashReporterChild PCrashReporterChild;
protected:
    virtual mozilla::ipc::RacyInterruptPolicy
    MediateInterruptRace(const Message& parent, const Message& child) MOZ_OVERRIDE
    {
        return MediateRace(parent, child);
    }

    virtual bool ShouldContinueFromReplyTimeout() MOZ_OVERRIDE;

    virtual bool RecvSettingChanged(const PluginSettings& aSettings) MOZ_OVERRIDE;

    
    virtual bool RecvDisableFlashProtectedMode() MOZ_OVERRIDE;
    virtual bool AnswerNP_GetEntryPoints(NPError* rv) MOZ_OVERRIDE;
    virtual bool AnswerNP_Initialize(const PluginSettings& aSettings, NPError* rv) MOZ_OVERRIDE;
    virtual bool RecvAsyncNP_Initialize(const PluginSettings& aSettings) MOZ_OVERRIDE;
    virtual bool AnswerSyncNPP_New(PPluginInstanceChild* aActor, NPError* rv)
                                   MOZ_OVERRIDE;
    virtual bool RecvAsyncNPP_New(PPluginInstanceChild* aActor) MOZ_OVERRIDE;

    virtual PPluginModuleChild*
    AllocPPluginModuleChild(mozilla::ipc::Transport* aTransport,
                            base::ProcessId aOtherProcess) MOZ_OVERRIDE;

    virtual PPluginInstanceChild*
    AllocPPluginInstanceChild(const nsCString& aMimeType,
                              const uint16_t& aMode,
                              const InfallibleTArray<nsCString>& aNames,
                              const InfallibleTArray<nsCString>& aValues)
                              MOZ_OVERRIDE;

    virtual bool
    DeallocPPluginInstanceChild(PPluginInstanceChild* aActor) MOZ_OVERRIDE;

    virtual bool
    RecvPPluginInstanceConstructor(PPluginInstanceChild* aActor,
                                   const nsCString& aMimeType,
                                   const uint16_t& aMode,
                                   InfallibleTArray<nsCString>&& aNames,
                                   InfallibleTArray<nsCString>&& aValues)
                                   MOZ_OVERRIDE;
    virtual bool
    AnswerNP_Shutdown(NPError *rv) MOZ_OVERRIDE;

    virtual bool
    AnswerOptionalFunctionsSupported(bool *aURLRedirectNotify,
                                     bool *aClearSiteData,
                                     bool *aGetSitesWithData) MOZ_OVERRIDE;

    virtual bool
    AnswerNPP_ClearSiteData(const nsCString& aSite,
                            const uint64_t& aFlags,
                            const uint64_t& aMaxAge,
                            NPError* aResult) MOZ_OVERRIDE;

    virtual bool
    AnswerNPP_GetSitesWithData(InfallibleTArray<nsCString>* aResult) MOZ_OVERRIDE;

    virtual bool
    RecvSetAudioSessionData(const nsID& aId,
                            const nsString& aDisplayName,
                            const nsString& aIconPath) MOZ_OVERRIDE;

    virtual bool
    RecvSetParentHangTimeout(const uint32_t& aSeconds) MOZ_OVERRIDE;

    virtual PCrashReporterChild*
    AllocPCrashReporterChild(mozilla::dom::NativeThreadId* id,
                             uint32_t* processType) MOZ_OVERRIDE;
    virtual bool
    DeallocPCrashReporterChild(PCrashReporterChild* actor) MOZ_OVERRIDE;
    virtual bool
    AnswerPCrashReporterConstructor(PCrashReporterChild* actor,
                                    mozilla::dom::NativeThreadId* id,
                                    uint32_t* processType) MOZ_OVERRIDE;

    virtual void
    ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

    MOZ_NORETURN void QuickExit();

    virtual bool
    RecvProcessNativeEventsInInterruptCall() MOZ_OVERRIDE;

    virtual bool RecvStartProfiler(const uint32_t& aEntries,
                                   const double& aInterval,
                                   nsTArray<nsCString>&& aFeatures,
                                   nsTArray<nsCString>&& aThreadNameFilters) MOZ_OVERRIDE;
    virtual bool RecvStopProfiler() MOZ_OVERRIDE;
    virtual bool AnswerGetProfile(nsCString* aProfile) MOZ_OVERRIDE;

public:
    explicit PluginModuleChild(bool aIsChrome);
    virtual ~PluginModuleChild();

    bool CommonInit(base::ProcessHandle aParentProcessHandle,
                    MessageLoop* aIOLoop,
                    IPC::Channel* aChannel);

    
    bool InitForChrome(const std::string& aPluginFilename,
                       base::ProcessHandle aParentProcessHandle,
                       MessageLoop* aIOLoop,
                       IPC::Channel* aChannel);

    bool InitForContent(base::ProcessHandle aParentProcessHandle,
                        MessageLoop* aIOLoop,
                        IPC::Channel* aChannel);

    static PluginModuleChild*
    CreateForContentProcess(mozilla::ipc::Transport* aTransport,
                            base::ProcessId aOtherProcess);

    void CleanUp();

    const char* GetUserAgent();

    static const NPNetscapeFuncs sBrowserFuncs;

    static PluginModuleChild* GetChrome();

    


    static NPObject* NPN_CreateObject(NPP aNPP, NPClass* aClass);
    


    static NPObject* NPN_RetainObject(NPObject* aNPObj);
    


    static void NPN_ReleaseObject(NPObject* aNPObj);

    


    static NPIdentifier NPN_GetStringIdentifier(const NPUTF8* aName);
    static void NPN_GetStringIdentifiers(const NPUTF8** aNames,
                                                     int32_t aNameCount,
                                                     NPIdentifier* aIdentifiers);
    static NPIdentifier NPN_GetIntIdentifier(int32_t aIntId);
    static bool NPN_IdentifierIsString(NPIdentifier aIdentifier);
    static NPUTF8* NPN_UTF8FromIdentifier(NPIdentifier aIdentifier);
    static int32_t NPN_IntFromIdentifier(NPIdentifier aIdentifier);

#ifdef MOZ_WIDGET_COCOA
    void ProcessNativeEvents();
    
    void PluginShowWindow(uint32_t window_id, bool modal, CGRect r) {
        SendPluginShowWindow(window_id, modal, r.origin.x, r.origin.y, r.size.width, r.size.height);
    }

    void PluginHideWindow(uint32_t window_id) {
        SendPluginHideWindow(window_id);
    }

    void SetCursor(NSCursorInfo& cursorInfo) {
        SendSetCursor(cursorInfo);
    }

    void ShowCursor(bool show) {
        SendShowCursor(show);
    }

    void PushCursor(NSCursorInfo& cursorInfo) {
        SendPushCursor(cursorInfo);
    }

    void PopCursor() {
        SendPopCursor();
    }

    bool GetNativeCursorsSupported() {
        return Settings().nativeCursorsSupported();
    }
#endif

    
    enum PluginQuirks {
        QUIRKS_NOT_INITIALIZED                          = 0,
        
        
        QUIRK_SILVERLIGHT_DEFAULT_TRANSPARENT           = 1 << 0,
        
        
        
        QUIRK_WINLESS_TRACKPOPUP_HOOK                   = 1 << 1,
        
        
        
        QUIRK_FLASH_THROTTLE_WMUSER_EVENTS              = 1 << 2,
        
        QUIRK_FLASH_HOOK_SETLONGPTR                     = 1 << 3,
        
        
        
        QUIRK_FLASH_EXPOSE_COORD_TRANSLATION            = 1 << 4,
        
        
        
        QUIRK_FLASH_HOOK_GETWINDOWINFO                  = 1 << 5,
        
        
        QUIRK_FLASH_FIXUP_MOUSE_CAPTURE                 = 1 << 6,
        
        
        QUIRK_QUICKTIME_AVOID_SETWINDOW                 = 1 << 7,
        
        
        
        QUIRK_SILVERLIGHT_FOCUS_CHECK_PARENT            = 1 << 8,
        
        
        QUIRK_ALLOW_OFFLINE_RENDERER                    = 1 << 9,
        
        
        
        
        QUIRK_FLASH_AVOID_CGMODE_CRASHES                = 1 << 10,
        
        
        
        
        
        QUIRK_FLASH_HIDE_HIDPI_SUPPORT                  = 1 << 11,
    };

    int GetQuirks() { return mQuirks; }

    const PluginSettings& Settings() const { return mCachedSettings; }

private:
    NPError DoNP_Initialize(const PluginSettings& aSettings);
    void AddQuirk(PluginQuirks quirk) {
      if (mQuirks == QUIRKS_NOT_INITIALIZED)
        mQuirks = 0;
      mQuirks |= quirk;
    }
    void InitQuirksModes(const nsCString& aMimeType);
    bool InitGraphics();
    void DeinitGraphics();

#if defined(OS_WIN)
    void HookProtectedMode();
    void CleanupProtectedModeHook();
#endif

#if defined(MOZ_WIDGET_GTK)
    static gboolean DetectNestedEventLoop(gpointer data);
    static gboolean ProcessBrowserEvents(gpointer data);

    virtual void EnteredCxxStack() MOZ_OVERRIDE;
    virtual void ExitedCxxStack() MOZ_OVERRIDE;
#elif defined(MOZ_WIDGET_QT)

    virtual void EnteredCxxStack() MOZ_OVERRIDE;
    virtual void ExitedCxxStack() MOZ_OVERRIDE;
#endif

    PRLibrary* mLibrary;
    nsCString mPluginFilename; 
    nsCString mUserAgent;
    int mQuirks;

    bool mIsChrome;
    Transport* mTransport;

    
    NP_PLUGINSHUTDOWN mShutdownFunc;
#if defined(OS_LINUX) || defined(OS_BSD)
    NP_PLUGINUNIXINIT mInitializeFunc;
#elif defined(OS_WIN) || defined(OS_MACOSX)
    NP_PLUGININIT mInitializeFunc;
    NP_GETENTRYPOINTS mGetEntryPointsFunc;
#endif

    NPPluginFuncs mFunctions;

    PluginSettings mCachedSettings;

#if defined(MOZ_WIDGET_GTK)
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    guint mNestedLoopTimerId;
#  ifdef DEBUG
    
    
    
    
    int mTopLoopDepth;
#  endif
#elif defined (MOZ_WIDGET_QT)
    NestedLoopTimer *mNestedLoopTimerObject;
#endif

public: 
    



    static void DeallocNPObject(NPObject* o);

    NPError NPP_Destroy(PluginInstanceChild* instance) {
        return mFunctions.destroy(instance->GetNPP(), 0);
    }

private:
#if defined(OS_WIN)
    virtual void EnteredCall() MOZ_OVERRIDE;
    virtual void ExitedCall() MOZ_OVERRIDE;

    
    
    struct IncallFrame
    {
        IncallFrame()
            : _spinning(false)
            , _savedNestableTasksAllowed(false)
        { }

        bool _spinning;
        bool _savedNestableTasksAllowed;
    };

    nsAutoTArray<IncallFrame, 8> mIncallPumpingStack;

    static LRESULT CALLBACK NestedInputEventHook(int code,
                                                 WPARAM wParam,
                                                 LPARAM lParam);
    static LRESULT CALLBACK CallWindowProcHook(int code,
                                               WPARAM wParam,
                                               LPARAM lParam);
    void SetEventHooks();
    void ResetEventHooks();
    HHOOK mNestedEventHook;
    HHOOK mGlobalCallWndProcHook;
#endif
};

} 
} 

#endif  
