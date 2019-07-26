





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
#include "mozilla/plugins/PluginIdentifierChild.h"









#define NP_CALLBACK NP_LOADDS

#if defined(XP_WIN)
#define NS_NPAPIPLUGIN_CALLBACK(_type, _name) _type (__stdcall * _name)
#elif defined(XP_OS2)
#define NS_NPAPIPLUGIN_CALLBACK(_type, _name) _type (_System * _name)
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
    virtual mozilla::ipc::RPCChannel::RacyRPCPolicy
    MediateRPCRace(const Message& parent, const Message& child) MOZ_OVERRIDE
    {
        return MediateRace(parent, child);
    }

    virtual bool ShouldContinueFromReplyTimeout() MOZ_OVERRIDE;

    
    virtual bool AnswerNP_GetEntryPoints(NPError* rv);
    virtual bool AnswerNP_Initialize(const uint32_t& aFlags, NPError* rv);

    virtual PPluginIdentifierChild*
    AllocPPluginIdentifierChild(const nsCString& aString,
                                const int32_t& aInt,
                                const bool& aTemporary);

    virtual bool
    RecvPPluginIdentifierConstructor(PPluginIdentifierChild* actor,
                                     const nsCString& aString,
                                     const int32_t& aInt,
                                     const bool& aTemporary);

    virtual bool
    DeallocPPluginIdentifierChild(PPluginIdentifierChild* aActor);

    virtual PPluginInstanceChild*
    AllocPPluginInstanceChild(const nsCString& aMimeType,
                              const uint16_t& aMode,
                              const InfallibleTArray<nsCString>& aNames,
                              const InfallibleTArray<nsCString>& aValues,
                              NPError* rv);

    virtual bool
    DeallocPPluginInstanceChild(PPluginInstanceChild* aActor);

    virtual bool
    AnswerPPluginInstanceConstructor(PPluginInstanceChild* aActor,
                                     const nsCString& aMimeType,
                                     const uint16_t& aMode,
                                     const InfallibleTArray<nsCString>& aNames,
                                     const InfallibleTArray<nsCString>& aValues,
                                     NPError* rv);
    virtual bool
    AnswerNP_Shutdown(NPError *rv);

    virtual bool
    AnswerOptionalFunctionsSupported(bool *aURLRedirectNotify,
                                     bool *aClearSiteData,
                                     bool *aGetSitesWithData);

    virtual bool
    AnswerNPP_ClearSiteData(const nsCString& aSite,
                            const uint64_t& aFlags,
                            const uint64_t& aMaxAge,
                            NPError* aResult);

    virtual bool
    AnswerNPP_GetSitesWithData(InfallibleTArray<nsCString>* aResult);

    virtual bool
    RecvSetAudioSessionData(const nsID& aId,
                            const nsString& aDisplayName,
                            const nsString& aIconPath);

    virtual bool
    RecvSetParentHangTimeout(const uint32_t& aSeconds);

    virtual PCrashReporterChild*
    AllocPCrashReporterChild(mozilla::dom::NativeThreadId* id,
                             uint32_t* processType);
    virtual bool
    DeallocPCrashReporterChild(PCrashReporterChild* actor);
    virtual bool
    AnswerPCrashReporterConstructor(PCrashReporterChild* actor,
                                    mozilla::dom::NativeThreadId* id,
                                    uint32_t* processType);

    virtual void
    ActorDestroy(ActorDestroyReason why);

    MOZ_NORETURN void QuickExit();

    virtual bool
    RecvProcessNativeEventsInRPCCall() MOZ_OVERRIDE;

    virtual bool
    AnswerGeckoGetProfile(nsCString* aProfile);

public:
    PluginModuleChild();
    virtual ~PluginModuleChild();

    
    bool Init(const std::string& aPluginFilename,
              base::ProcessHandle aParentProcessHandle,
              MessageLoop* aIOLoop,
              IPC::Channel* aChannel);

    void CleanUp();

    const char* GetUserAgent();

    static const NPNetscapeFuncs sBrowserFuncs;

    static PluginModuleChild* current();

    bool RegisterActorForNPObject(NPObject* aObject,
                                  PluginScriptableObjectChild* aActor);

    void UnregisterActorForNPObject(NPObject* aObject);

    PluginScriptableObjectChild* GetActorForNPObject(NPObject* aObject);

#ifdef DEBUG
    bool NPObjectIsRegistered(NPObject* aObject);
#endif

    bool AsyncDrawingAllowed() { return mAsyncDrawingAllowed; }

    


    static NPObject* NP_CALLBACK NPN_CreateObject(NPP aNPP, NPClass* aClass);
    


    static NPObject* NP_CALLBACK NPN_RetainObject(NPObject* aNPObj);
    


    static void NP_CALLBACK NPN_ReleaseObject(NPObject* aNPObj);

    


    static NPIdentifier NP_CALLBACK NPN_GetStringIdentifier(const NPUTF8* aName);
    static void NP_CALLBACK NPN_GetStringIdentifiers(const NPUTF8** aNames,
                                                     int32_t aNameCount,
                                                     NPIdentifier* aIdentifiers);
    static NPIdentifier NP_CALLBACK NPN_GetIntIdentifier(int32_t aIntId);
    static bool NP_CALLBACK NPN_IdentifierIsString(NPIdentifier aIdentifier);
    static NPUTF8* NP_CALLBACK NPN_UTF8FromIdentifier(NPIdentifier aIdentifier);
    static int32_t NP_CALLBACK NPN_IntFromIdentifier(NPIdentifier aIdentifier);

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
        bool supported = false;
        SendGetNativeCursorsSupported(&supported);
        return supported;
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
    };

    int GetQuirks() { return mQuirks; }

private:
    void AddQuirk(PluginQuirks quirk) {
      if (mQuirks == QUIRKS_NOT_INITIALIZED)
        mQuirks = 0;
      mQuirks |= quirk;
    }
    void InitQuirksModes(const nsCString& aMimeType);
    bool InitGraphics();
    void DeinitGraphics();
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
    bool mAsyncDrawingAllowed;

    
    NP_PLUGINSHUTDOWN mShutdownFunc;
#if defined(OS_LINUX) || defined(OS_BSD)
    NP_PLUGINUNIXINIT mInitializeFunc;
#elif defined(OS_WIN) || defined(OS_MACOSX)
    NP_PLUGININIT mInitializeFunc;
    NP_GETENTRYPOINTS mGetEntryPointsFunc;
#endif

    NPPluginFuncs mFunctions;
    NPSavedData mSavedData;

#if defined(MOZ_WIDGET_GTK)
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    guint mNestedLoopTimerId;
#  ifdef DEBUG
    
    
    
    
    int mTopLoopDepth;
#  endif
#elif defined (MOZ_WIDGET_QT)
    NestedLoopTimer *mNestedLoopTimerObject;
#endif

    struct NPObjectData : public nsPtrHashKey<NPObject>
    {
        NPObjectData(const NPObject* key)
            : nsPtrHashKey<NPObject>(key)
            , instance(NULL)
            , actor(NULL)
        { }

        
        PluginInstanceChild* instance;

        
        PluginScriptableObjectChild* actor;
    };
    



    nsTHashtable<NPObjectData> mObjectMap;

    friend class PluginIdentifierChild;
    friend class PluginIdentifierChildString;
    friend class PluginIdentifierChildInt;
    nsDataHashtable<nsCStringHashKey, PluginIdentifierChildString*> mStringIdentifiers;
    nsDataHashtable<nsUint32HashKey, PluginIdentifierChildInt*> mIntIdentifiers;

public: 
    



    static void DeallocNPObject(NPObject* o);

    NPError NPP_Destroy(PluginInstanceChild* instance) {
        return mFunctions.destroy(instance->GetNPP(), 0);
    }

    



    void FindNPObjectsForInstance(PluginInstanceChild* instance);

private:
    static PLDHashOperator CollectForInstance(NPObjectData* d, void* userArg);

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
