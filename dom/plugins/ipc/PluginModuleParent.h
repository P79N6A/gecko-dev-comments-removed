





#ifndef mozilla_plugins_PluginModuleParent_h
#define mozilla_plugins_PluginModuleParent_h

#include "base/process.h"
#include "mozilla/FileUtils.h"
#include "mozilla/HangAnnotations.h"
#include "mozilla/PluginLibrary.h"
#include "mozilla/plugins/PluginProcessParent.h"
#include "mozilla/plugins/PPluginModuleParent.h"
#include "mozilla/plugins/PluginMessageUtils.h"
#include "mozilla/plugins/PluginTypes.h"
#include "mozilla/plugins/TaskFactory.h"
#include "mozilla/TimeStamp.h"
#include "npapi.h"
#include "npfunctions.h"
#include "nsAutoPtr.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsIObserver.h"
#ifdef XP_WIN
#include "nsWindowsHelpers.h"
#endif

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif

class nsPluginTag;

namespace mozilla {
namespace dom {
class PCrashReporterParent;
class CrashReporterParent;
}

namespace plugins {


class BrowserStreamParent;
class PluginAsyncSurrogate;
class PluginInstanceParent;

#ifdef XP_WIN
class PluginHangUIParent;
#endif
#ifdef MOZ_CRASHREPORTER_INJECTOR
class FinishInjectorInitTask;
#endif


















class PluginModuleParent
    : public PPluginModuleParent
    , public PluginLibrary
#ifdef MOZ_CRASHREPORTER_INJECTOR
    , public CrashReporter::InjectorCrashCallback
#endif
{
protected:
    typedef mozilla::PluginLibrary PluginLibrary;
    typedef mozilla::dom::PCrashReporterParent PCrashReporterParent;
    typedef mozilla::dom::CrashReporterParent CrashReporterParent;

    PPluginInstanceParent*
    AllocPPluginInstanceParent(const nsCString& aMimeType,
                               const uint16_t& aMode,
                               const InfallibleTArray<nsCString>& aNames,
                               const InfallibleTArray<nsCString>& aValues)
                               MOZ_OVERRIDE;

    virtual bool
    DeallocPPluginInstanceParent(PPluginInstanceParent* aActor) MOZ_OVERRIDE;

public:
    explicit PluginModuleParent(bool aIsChrome);
    virtual ~PluginModuleParent();

    bool RemovePendingSurrogate(const nsRefPtr<PluginAsyncSurrogate>& aSurrogate);

    
    bool IsStartingAsync() const { return mIsStartingAsync; }
    

    bool IsInitialized() const { return mNPInitialized; }
    bool IsChrome() const { return mIsChrome; }

    virtual void SetPlugin(nsNPAPIPlugin* plugin) MOZ_OVERRIDE
    {
        mPlugin = plugin;
    }

    virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

    const NPNetscapeFuncs* GetNetscapeFuncs() {
        return mNPNIface;
    }

    bool OkToCleanup() const {
        return !IsOnCxxStack();
    }

    void ProcessRemoteNativeEventsInInterruptCall() MOZ_OVERRIDE;

    virtual bool WaitForIPCConnection() { return true; }

    nsCString GetHistogramKey() const {
        return mPluginName + mPluginVersion;
    }

protected:
    virtual mozilla::ipc::RacyInterruptPolicy
    MediateInterruptRace(const Message& parent, const Message& child) MOZ_OVERRIDE
    {
        return MediateRace(parent, child);
    }

    virtual bool
    RecvBackUpXResources(const FileDescriptor& aXSocketFd) MOZ_OVERRIDE;

    virtual bool AnswerProcessSomeEvents() MOZ_OVERRIDE;

    virtual bool
    RecvProcessNativeEventsInInterruptCall() MOZ_OVERRIDE;

    virtual bool
    RecvPluginShowWindow(const uint32_t& aWindowId, const bool& aModal,
                         const int32_t& aX, const int32_t& aY,
                         const size_t& aWidth, const size_t& aHeight) MOZ_OVERRIDE;

    virtual bool
    RecvPluginHideWindow(const uint32_t& aWindowId) MOZ_OVERRIDE;

    virtual PCrashReporterParent*
    AllocPCrashReporterParent(mozilla::dom::NativeThreadId* id,
                              uint32_t* processType) MOZ_OVERRIDE;
    virtual bool
    DeallocPCrashReporterParent(PCrashReporterParent* actor) MOZ_OVERRIDE;

    virtual bool
    RecvSetCursor(const NSCursorInfo& aCursorInfo) MOZ_OVERRIDE;

    virtual bool
    RecvShowCursor(const bool& aShow) MOZ_OVERRIDE;

    virtual bool
    RecvPushCursor(const NSCursorInfo& aCursorInfo) MOZ_OVERRIDE;

    virtual bool
    RecvPopCursor() MOZ_OVERRIDE;

    virtual bool
    RecvNPN_SetException(const nsCString& aMessage) MOZ_OVERRIDE;

    virtual bool
    RecvNPN_ReloadPlugins(const bool& aReloadPages) MOZ_OVERRIDE;

    virtual bool
    RecvNP_InitializeResult(const NPError& aError) MOZ_OVERRIDE;

    static BrowserStreamParent* StreamCast(NPP instance, NPStream* s,
                                           PluginAsyncSurrogate** aSurrogate = nullptr);

protected:
    void SetChildTimeout(const int32_t aChildTimeout);
    static void TimeoutChanged(const char* aPref, void* aModule);

    virtual void UpdatePluginTimeout() {}

    virtual bool RecvNotifyContentModuleDestroyed() MOZ_OVERRIDE { return true; }

    void SetPluginFuncs(NPPluginFuncs* aFuncs);

    nsresult NPP_NewInternal(NPMIMEType pluginType, NPP instance, uint16_t mode,
                             InfallibleTArray<nsCString>& names,
                             InfallibleTArray<nsCString>& values,
                             NPSavedData* saved, NPError* error);

    
    
    

    static NPError NPP_Destroy(NPP instance, NPSavedData** save);

    static NPError NPP_SetWindow(NPP instance, NPWindow* window);
    static NPError NPP_NewStream(NPP instance, NPMIMEType type, NPStream* stream,
                                 NPBool seekable, uint16_t* stype);
    static NPError NPP_DestroyStream(NPP instance,
                                     NPStream* stream, NPReason reason);
    static int32_t NPP_WriteReady(NPP instance, NPStream* stream);
    static int32_t NPP_Write(NPP instance, NPStream* stream,
                             int32_t offset, int32_t len, void* buffer);
    static void NPP_StreamAsFile(NPP instance,
                                 NPStream* stream, const char* fname);
    static void NPP_Print(NPP instance, NPPrint* platformPrint);
    static int16_t NPP_HandleEvent(NPP instance, void* event);
    static void NPP_URLNotify(NPP instance, const char* url,
                              NPReason reason, void* notifyData);
    static NPError NPP_GetValue(NPP instance,
                                NPPVariable variable, void *ret_value);
    static NPError NPP_SetValue(NPP instance, NPNVariable variable,
                                void *value);
    static void NPP_URLRedirectNotify(NPP instance, const char* url,
                                      int32_t status, void* notifyData);

    virtual bool HasRequiredFunctions() MOZ_OVERRIDE;
    virtual nsresult AsyncSetWindow(NPP aInstance, NPWindow* aWindow) MOZ_OVERRIDE;
    virtual nsresult GetImageContainer(NPP aInstance, mozilla::layers::ImageContainer** aContainer) MOZ_OVERRIDE;
    virtual nsresult GetImageSize(NPP aInstance, nsIntSize* aSize) MOZ_OVERRIDE;
    virtual bool IsOOP() MOZ_OVERRIDE { return true; }
    virtual nsresult SetBackgroundUnknown(NPP instance) MOZ_OVERRIDE;
    virtual nsresult BeginUpdateBackground(NPP instance,
                                           const nsIntRect& aRect,
                                           gfxContext** aCtx) MOZ_OVERRIDE;
    virtual nsresult EndUpdateBackground(NPP instance,
                                         gfxContext* aCtx,
                                         const nsIntRect& aRect) MOZ_OVERRIDE;

#if defined(XP_UNIX) && !defined(XP_MACOSX) && !defined(MOZ_WIDGET_GONK)
    virtual nsresult NP_Initialize(NPNetscapeFuncs* bFuncs, NPPluginFuncs* pFuncs, NPError* error) MOZ_OVERRIDE;
#else
    virtual nsresult NP_Initialize(NPNetscapeFuncs* bFuncs, NPError* error) MOZ_OVERRIDE;
#endif
    virtual nsresult NP_Shutdown(NPError* error) MOZ_OVERRIDE;

    virtual nsresult NP_GetMIMEDescription(const char** mimeDesc) MOZ_OVERRIDE;
    virtual nsresult NP_GetValue(void *future, NPPVariable aVariable,
                                 void *aValue, NPError* error) MOZ_OVERRIDE;
#if defined(XP_WIN) || defined(XP_MACOSX)
    virtual nsresult NP_GetEntryPoints(NPPluginFuncs* pFuncs, NPError* error) MOZ_OVERRIDE;
#endif
    virtual nsresult NPP_New(NPMIMEType pluginType, NPP instance,
                             uint16_t mode, int16_t argc, char* argn[],
                             char* argv[], NPSavedData* saved,
                             NPError* error) MOZ_OVERRIDE;
    virtual nsresult NPP_ClearSiteData(const char* site, uint64_t flags,
                                       uint64_t maxAge) MOZ_OVERRIDE;
    virtual nsresult NPP_GetSitesWithData(InfallibleTArray<nsCString>& result) MOZ_OVERRIDE;

#if defined(XP_MACOSX)
    virtual nsresult IsRemoteDrawingCoreAnimation(NPP instance, bool *aDrawing) MOZ_OVERRIDE;
    virtual nsresult ContentsScaleFactorChanged(NPP instance, double aContentsScaleFactor) MOZ_OVERRIDE;
#endif

    void InitAsyncSurrogates();

protected:
    void NotifyFlashHang();
    void NotifyPluginCrashed();
    void OnInitFailure();

    bool GetSetting(NPNVariable aVariable);
    void GetSettings(PluginSettings* aSettings);

    bool mIsChrome;
    bool mShutdown;
    bool mClearSiteDataSupported;
    bool mGetSitesWithDataSupported;
    NPNetscapeFuncs* mNPNIface;
    NPPluginFuncs* mNPPIface;
    nsNPAPIPlugin* mPlugin;
    TaskFactory<PluginModuleParent> mTaskFactory;
    nsString mPluginDumpID;
    nsString mBrowserDumpID;
    nsString mHangID;
    nsRefPtr<nsIObserver> mProfilerObserver;
    TimeDuration mTimeBlocked;
    nsCString mPluginName;
    nsCString mPluginVersion;
    bool mIsFlashPlugin;

#ifdef MOZ_X11
    
    
    ScopedClose mPluginXSocketFdDup;
#endif

    bool
    GetPluginDetails();

    friend class mozilla::dom::CrashReporterParent;
    friend class mozilla::plugins::PluginAsyncSurrogate;

    bool              mIsStartingAsync;
    bool              mNPInitialized;
    nsTArray<nsRefPtr<PluginAsyncSurrogate>> mSurrogateInstances;
    nsresult          mAsyncNewRv;
};

class PluginModuleContentParent : public PluginModuleParent
{
  public:
    explicit PluginModuleContentParent();

    static PluginLibrary* LoadModule(uint32_t aPluginId);

    static PluginModuleContentParent* Initialize(mozilla::ipc::Transport* aTransport,
                                                 base::ProcessId aOtherProcess);

    static void OnLoadPluginResult(const uint32_t& aPluginId, const bool& aResult);
    static void AssociatePluginId(uint32_t aPluginId, base::ProcessId aProcessId);

    virtual ~PluginModuleContentParent();

#if defined(XP_WIN) || defined(XP_MACOSX)
    nsresult NP_Initialize(NPNetscapeFuncs* bFuncs, NPError* error) MOZ_OVERRIDE;
#endif

  private:
    virtual bool ShouldContinueFromReplyTimeout() MOZ_OVERRIDE;
    virtual void OnExitedSyncSend() MOZ_OVERRIDE;

#ifdef MOZ_CRASHREPORTER_INJECTOR
    void OnCrash(DWORD processID) MOZ_OVERRIDE {}
#endif

    static PluginModuleContentParent* sSavedModuleParent;

    uint32_t mPluginId;
};

class PluginModuleChromeParent
    : public PluginModuleParent
    , public mozilla::HangMonitor::Annotator
{
  public:
    





    static PluginLibrary* LoadModule(const char* aFilePath, uint32_t aPluginId,
                                     nsPluginTag* aPluginTag);

    




    static void ClearInstantiationFlag() { sInstantiated = false; }
    static bool DidInstantiate() { return sInstantiated; }

    virtual ~PluginModuleChromeParent();

    void TerminateChildProcess(MessageLoop* aMsgLoop);

#ifdef XP_WIN
    



    void
    OnHangUIContinue();

    void
    EvaluateHangUIState(const bool aReset);
#endif 

    virtual bool WaitForIPCConnection() MOZ_OVERRIDE;

    virtual bool
    RecvNP_InitializeResult(const NPError& aError) MOZ_OVERRIDE;

    void
    SetContentParent(dom::ContentParent* aContentParent);

    bool
    SendAssociatePluginId();

    void CachedSettingChanged();

    void OnEnteredCall() MOZ_OVERRIDE;
    void OnExitedCall() MOZ_OVERRIDE;
    void OnEnteredSyncSend() MOZ_OVERRIDE;
    void OnExitedSyncSend() MOZ_OVERRIDE;

private:
    virtual void
    EnteredCxxStack() MOZ_OVERRIDE;

    void
    ExitedCxxStack() MOZ_OVERRIDE;

    mozilla::ipc::IProtocol* GetInvokingProtocol();
    PluginInstanceParent* GetManagingInstance(mozilla::ipc::IProtocol* aProtocol);

    virtual void
    AnnotateHang(mozilla::HangMonitor::HangAnnotations& aAnnotations) MOZ_OVERRIDE;

    virtual bool ShouldContinueFromReplyTimeout() MOZ_OVERRIDE;

#ifdef MOZ_CRASHREPORTER
    void ProcessFirstMinidump();
    void WriteExtraDataForMinidump(CrashReporter::AnnotationTable& notes);
#endif

    virtual PCrashReporterParent*
    AllocPCrashReporterParent(mozilla::dom::NativeThreadId* id,
                              uint32_t* processType) MOZ_OVERRIDE;
    virtual bool
    DeallocPCrashReporterParent(PCrashReporterParent* actor) MOZ_OVERRIDE;

    PluginProcessParent* Process() const { return mSubprocess; }
    base::ProcessHandle ChildProcessHandle() { return mSubprocess->GetChildProcessHandle(); }

#if defined(XP_UNIX) && !defined(XP_MACOSX) && !defined(MOZ_WIDGET_GONK)
    virtual nsresult NP_Initialize(NPNetscapeFuncs* bFuncs, NPPluginFuncs* pFuncs, NPError* error) MOZ_OVERRIDE;
#else
    virtual nsresult NP_Initialize(NPNetscapeFuncs* bFuncs, NPError* error) MOZ_OVERRIDE;
#endif

#if defined(XP_WIN) || defined(XP_MACOSX)
    virtual nsresult NP_GetEntryPoints(NPPluginFuncs* pFuncs, NPError* error) MOZ_OVERRIDE;
#endif

    virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

    
    explicit PluginModuleChromeParent(const char* aFilePath, uint32_t aPluginId);

    CrashReporterParent* CrashReporter();

    void CleanupFromTimeout(const bool aByHangUI);

    virtual void UpdatePluginTimeout() MOZ_OVERRIDE;

#ifdef MOZ_ENABLE_PROFILER_SPS
    void InitPluginProfiling();
    void ShutdownPluginProfiling();
#endif

    void RegisterSettingsCallbacks();
    void UnregisterSettingsCallbacks();

    virtual bool RecvNotifyContentModuleDestroyed() MOZ_OVERRIDE;

    static void CachedSettingChanged(const char* aPref, void* aModule);

    PluginProcessParent* mSubprocess;
    uint32_t mPluginId;

    TaskFactory<PluginModuleChromeParent> mChromeTaskFactory;

    enum HangAnnotationFlags
    {
        kInPluginCall = (1u << 0),
        kHangUIShown = (1u << 1),
        kHangUIContinued = (1u << 2),
        kHangUIDontShow = (1u << 3)
    };
    Atomic<uint32_t> mHangAnnotationFlags;
    mozilla::Mutex mHangAnnotatorMutex;
    InfallibleTArray<mozilla::ipc::IProtocol*> mProtocolCallStack;
#ifdef XP_WIN
    InfallibleTArray<float> mPluginCpuUsageOnHang;
    PluginHangUIParent *mHangUIParent;
    bool mHangUIEnabled;
    bool mIsTimerReset;
#ifdef MOZ_CRASHREPORTER
    





    mozilla::Mutex mCrashReporterMutex;
    CrashReporterParent* mCrashReporter;
#endif 


    






    bool
    LaunchHangUI();

    


    void
    FinishHangUI();
#endif

    friend class mozilla::dom::CrashReporterParent;
    friend class mozilla::plugins::PluginAsyncSurrogate;

#ifdef MOZ_CRASHREPORTER_INJECTOR
    friend class mozilla::plugins::FinishInjectorInitTask;

    void InitializeInjector();
    void DoInjection(const nsAutoHandle& aSnapshot);
    static DWORD WINAPI GetToolhelpSnapshot(LPVOID aContext);

    void OnCrash(DWORD processID) MOZ_OVERRIDE;

    DWORD mFlashProcess1;
    DWORD mFlashProcess2;
    mozilla::plugins::FinishInjectorInitTask* mFinishInitTask;
#endif

    void OnProcessLaunched(const bool aSucceeded);

    class LaunchedTask : public LaunchCompleteTask
    {
    public:
        explicit LaunchedTask(PluginModuleChromeParent* aModule)
            : mModule(aModule)
        {
            MOZ_ASSERT(aModule);
        }

        void Run() MOZ_OVERRIDE
        {
            mModule->OnProcessLaunched(mLaunchSucceeded);
        }

    private:
        PluginModuleChromeParent* mModule;
    };

    friend class LaunchedTask;

    bool                mInitOnAsyncConnect;
    nsresult            mAsyncInitRv;
    NPError             mAsyncInitError;
    dom::ContentParent* mContentParent;
    nsCOMPtr<nsIObserver> mOfflineObserver;
    bool mIsBlocklisted;
    static bool sInstantiated;
};

} 
} 

#endif 
