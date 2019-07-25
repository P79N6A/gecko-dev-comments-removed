






































#ifndef dom_plugins_PluginModuleChild_h
#define dom_plugins_PluginModuleChild_h 1

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

#ifdef OS_MACOSX
#include "PluginInterposeOSX.h"
#endif

#include "mozilla/plugins/PPluginModuleChild.h"
#include "mozilla/plugins/PluginInstanceChild.h"
#include "mozilla/plugins/PluginIdentifierChild.h"









#ifdef XP_OS2
#define NP_CALLBACK _System
#else
#define NP_CALLBACK
#endif

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
namespace plugins {

#ifdef MOZ_WIDGET_QT
class NestedLoopTimer;
static const int kNestedLoopDetectorIntervalMs = 90;
#endif

class PluginScriptableObjectChild;
class PluginInstanceChild;

class PluginModuleChild : public PPluginModuleChild
{
protected:
    NS_OVERRIDE
    virtual mozilla::ipc::RPCChannel::RacyRPCPolicy
    MediateRPCRace(const Message& parent, const Message& child)
    {
        return MediateRace(parent, child);
    }

    
    virtual bool AnswerNP_GetEntryPoints(NPError* rv);
    virtual bool AnswerNP_Initialize(NativeThreadId* tid, NPError* rv);

    virtual PPluginIdentifierChild*
    AllocPPluginIdentifier(const nsCString& aString,
                           const int32_t& aInt);

    virtual bool
    DeallocPPluginIdentifier(PPluginIdentifierChild* aActor);

    virtual PPluginInstanceChild*
    AllocPPluginInstance(const nsCString& aMimeType,
                         const uint16_t& aMode,
                         const InfallibleTArray<nsCString>& aNames,
                         const InfallibleTArray<nsCString>& aValues,
                         NPError* rv);

    virtual bool
    DeallocPPluginInstance(PPluginInstanceChild* aActor);

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

    virtual void
    ActorDestroy(ActorDestroyReason why);

    NS_NORETURN void QuickExit();

    NS_OVERRIDE virtual bool
    RecvProcessNativeEventsInRPCCall();

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

#ifdef OS_MACOSX
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
#if defined(MOZ_WIDGET_GTK2)
    static gboolean DetectNestedEventLoop(gpointer data);
    static gboolean ProcessBrowserEvents(gpointer data);

    NS_OVERRIDE
    virtual void EnteredCxxStack();
    NS_OVERRIDE
    virtual void ExitedCxxStack();
#elif defined(MOZ_WIDGET_QT)

    NS_OVERRIDE
    virtual void EnteredCxxStack();
    NS_OVERRIDE
    virtual void ExitedCxxStack();
#endif

    PRLibrary* mLibrary;
    nsCString mPluginFilename; 
    nsCString mUserAgent;
    int mQuirks;

    
    NP_PLUGINSHUTDOWN mShutdownFunc;
#ifdef OS_LINUX
    NP_PLUGINUNIXINIT mInitializeFunc;
#elif defined(OS_WIN) || defined(OS_MACOSX)
    NP_PLUGININIT mInitializeFunc;
    NP_GETENTRYPOINTS mGetEntryPointsFunc;
#endif

    NPPluginFuncs mFunctions;
    NPSavedData mSavedData;

#if defined(MOZ_WIDGET_GTK2)
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
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

    nsDataHashtable<nsCStringHashKey, PluginIdentifierChild*> mStringIdentifiers;
    nsDataHashtable<nsUint32HashKey, PluginIdentifierChild*> mIntIdentifiers;

public: 
    



    static void DeallocNPObject(NPObject* o);

    NPError NPP_Destroy(PluginInstanceChild* instance) {
        return mFunctions.destroy(instance->GetNPP(), 0);
    }

    



    void FindNPObjectsForInstance(PluginInstanceChild* instance);

private:
    static PLDHashOperator CollectForInstance(NPObjectData* d, void* userArg);

#if defined(OS_WIN)
    NS_OVERRIDE
    virtual void EnteredCall();
    NS_OVERRIDE
    virtual void ExitedCall();

    
    
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
