





#ifndef mozilla_dom_ContentChild_h
#define mozilla_dom_ContentChild_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/PContentChild.h"
#include "mozilla/dom/ipc/Blob.h"
#include "nsWeakPtr.h"

struct ChromePackage;
class nsIDOMBlob;
class nsIObserver;
struct ResourceMapping;
struct OverrideMapping;

namespace mozilla {

namespace ipc {
class OptionalURIParams;
class URIParams;
}

namespace jsipc {
class JavaScriptChild;
}

namespace layers {
class PCompositorChild;
} 

namespace dom {

class AlertObserver;
class PrefObserver;
class ConsoleListener;
class PStorageChild;
class ClonedMessageData;

class ContentChild : public PContentChild
{
    typedef mozilla::dom::ClonedMessageData ClonedMessageData;
    typedef mozilla::ipc::OptionalURIParams OptionalURIParams;
    typedef mozilla::ipc::URIParams URIParams;

public:
    ContentChild();
    virtual ~ContentChild();
    nsrefcnt AddRef() { return 1; }
    nsrefcnt Release() { return 1; }

    struct AppInfo
    {
        nsCString version;
        nsCString buildID;
        nsCString name;
        nsCString UAName;
    };

    bool Init(MessageLoop* aIOLoop,
              base::ProcessHandle aParentHandle,
              IPC::Channel* aChannel);
    void InitXPCOM();

    static ContentChild* GetSingleton() {
        return sSingleton;
    }

    const AppInfo& GetAppInfo() {
        return mAppInfo;
    }

    void SetProcessName(const nsAString& aName);
    void GetProcessName(nsAString& aName);
    void GetProcessName(nsACString& aName);
    static void AppendProcessId(nsACString& aName);

    PCompositorChild*
    AllocPCompositorChild(mozilla::ipc::Transport* aTransport,
                          base::ProcessId aOtherProcess) MOZ_OVERRIDE;
    PImageBridgeChild*
    AllocPImageBridgeChild(mozilla::ipc::Transport* aTransport,
                           base::ProcessId aOtherProcess) MOZ_OVERRIDE;

    virtual bool RecvSetProcessPrivileges(const ChildPrivileges& aPrivs);

    virtual PBrowserChild* AllocPBrowserChild(const IPCTabContext &aContext,
                                              const uint32_t &chromeFlags);
    virtual bool DeallocPBrowserChild(PBrowserChild*);

    virtual PDeviceStorageRequestChild* AllocPDeviceStorageRequestChild(const DeviceStorageParams&);
    virtual bool DeallocPDeviceStorageRequestChild(PDeviceStorageRequestChild*);

    virtual PBlobChild* AllocPBlobChild(const BlobConstructorParams& aParams);
    virtual bool DeallocPBlobChild(PBlobChild*);

    virtual PCrashReporterChild*
    AllocPCrashReporterChild(const mozilla::dom::NativeThreadId& id,
                             const uint32_t& processType);
    virtual bool
    DeallocPCrashReporterChild(PCrashReporterChild*);

    virtual PHalChild* AllocPHalChild() MOZ_OVERRIDE;
    virtual bool DeallocPHalChild(PHalChild*) MOZ_OVERRIDE;

    virtual PIndexedDBChild* AllocPIndexedDBChild();
    virtual bool DeallocPIndexedDBChild(PIndexedDBChild* aActor);

    virtual PMemoryReportRequestChild*
    AllocPMemoryReportRequestChild(const uint32_t& generation);

    virtual bool
    DeallocPMemoryReportRequestChild(PMemoryReportRequestChild* actor);

    virtual bool
    RecvPMemoryReportRequestConstructor(PMemoryReportRequestChild* child,
                                        const uint32_t& generation);

    virtual bool
    RecvAudioChannelNotify();

    virtual bool
    RecvDumpMemoryInfoToTempDir(const nsString& aIdentifier,
                                const bool& aMinimizeMemoryUsage,
                                const bool& aDumpChildProcesses);
    virtual bool
    RecvDumpGCAndCCLogsToFile(const nsString& aIdentifier,
                              const bool& aDumpAllTraces,
                              const bool& aDumpChildProcesses);

    virtual PTestShellChild* AllocPTestShellChild();
    virtual bool DeallocPTestShellChild(PTestShellChild*);
    virtual bool RecvPTestShellConstructor(PTestShellChild*);
    jsipc::JavaScriptChild *GetCPOWManager();

    virtual PNeckoChild* AllocPNeckoChild();
    virtual bool DeallocPNeckoChild(PNeckoChild*);

    virtual PExternalHelperAppChild *AllocPExternalHelperAppChild(
            const OptionalURIParams& uri,
            const nsCString& aMimeContentType,
            const nsCString& aContentDisposition,
            const bool& aForceSave,
            const int64_t& aContentLength,
            const OptionalURIParams& aReferrer,
            PBrowserChild* aBrowser);
    virtual bool DeallocPExternalHelperAppChild(PExternalHelperAppChild *aService);

    virtual PSmsChild* AllocPSmsChild();
    virtual bool DeallocPSmsChild(PSmsChild*);

    virtual PTelephonyChild* AllocPTelephonyChild();
    virtual bool DeallocPTelephonyChild(PTelephonyChild*);

    virtual PStorageChild* AllocPStorageChild();
    virtual bool DeallocPStorageChild(PStorageChild* aActor);

    virtual PBluetoothChild* AllocPBluetoothChild();
    virtual bool DeallocPBluetoothChild(PBluetoothChild* aActor);

    virtual PFMRadioChild* AllocPFMRadioChild();
    virtual bool DeallocPFMRadioChild(PFMRadioChild* aActor);

    virtual PSpeechSynthesisChild* AllocPSpeechSynthesisChild();
    virtual bool DeallocPSpeechSynthesisChild(PSpeechSynthesisChild* aActor);

    virtual bool RecvRegisterChrome(const InfallibleTArray<ChromePackage>& packages,
                                    const InfallibleTArray<ResourceMapping>& resources,
                                    const InfallibleTArray<OverrideMapping>& overrides,
                                    const nsCString& locale);

    virtual mozilla::jsipc::PJavaScriptChild* AllocPJavaScriptChild();
    virtual bool DeallocPJavaScriptChild(mozilla::jsipc::PJavaScriptChild*);

    virtual bool RecvSetOffline(const bool& offline);

    virtual bool RecvNotifyVisited(const URIParams& aURI);
    
    nsresult AddRemoteAlertObserver(const nsString& aData, nsIObserver* aObserver);

    virtual bool RecvPreferenceUpdate(const PrefSetting& aPref);

    virtual bool RecvNotifyAlertsObserver(const nsCString& aType, const nsString& aData);

    virtual bool RecvAsyncMessage(const nsString& aMsg,
                                  const ClonedMessageData& aData,
                                  const InfallibleTArray<CpowEntry>& aCpows,
                                  const IPC::Principal& aPrincipal);

    virtual bool RecvGeolocationUpdate(const GeoPosition& somewhere);

    virtual bool RecvAddPermission(const IPC::Permission& permission);

    virtual bool RecvScreenSizeChanged(const gfxIntSize &size);

    virtual bool RecvFlushMemory(const nsString& reason);

    virtual bool RecvActivateA11y();

    virtual bool RecvGarbageCollect();
    virtual bool RecvCycleCollect();

    virtual bool RecvAppInfo(const nsCString& version, const nsCString& buildID,
                             const nsCString& name, const nsCString& UAName);

    virtual bool RecvLastPrivateDocShellDestroyed();

    virtual bool RecvFilePathUpdate(const nsString& aStorageType,
                                    const nsString& aStorageName,
                                    const nsString& aPath,
                                    const nsCString& aReason);
    virtual bool RecvFileSystemUpdate(const nsString& aFsName,
                                      const nsString& aVolumeName,
                                      const int32_t& aState,
                                      const int32_t& aMountGeneration,
                                      const bool& aIsMediaPresent,
                                      const bool& aIsSharing);

    virtual bool RecvNuwaFork() MOZ_OVERRIDE;

    virtual bool RecvNotifyProcessPriorityChanged(const hal::ProcessPriority& aPriority);
    virtual bool RecvMinimizeMemoryUsage();
    virtual bool RecvCancelMinimizeMemoryUsage();

    virtual bool RecvLoadAndRegisterSheet(const URIParams& aURI, const uint32_t& aType);
    virtual bool RecvUnregisterSheet(const URIParams& aURI, const uint32_t& aType);

    virtual bool RecvNotifyPhoneStateChange(const nsString& state);
#ifdef ANDROID
    gfxIntSize GetScreenSize() { return mScreenSize; }
#endif

    
    
    nsString &GetIndexedDBPath();

    uint64_t GetID() { return mID; }

    bool IsForApp() { return mIsForApp; }
    bool IsForBrowser() { return mIsForBrowser; }

    BlobChild* GetOrCreateActorForBlob(nsIDOMBlob* aBlob);

protected:
    virtual bool RecvPBrowserConstructor(PBrowserChild* actor,
                                         const IPCTabContext& context,
                                         const uint32_t& chromeFlags);

private:
    virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

    virtual void ProcessingError(Result what) MOZ_OVERRIDE;

    



    MOZ_NORETURN void QuickExit();

    InfallibleTArray<nsAutoPtr<AlertObserver> > mAlertObservers;
    nsRefPtr<ConsoleListener> mConsoleListener;

    






    uint64_t mID;

    AppInfo mAppInfo;

#ifdef ANDROID
    gfxIntSize mScreenSize;
#endif

    bool mIsForApp;
    bool mIsForBrowser;
    nsString mProcessName;
    nsWeakPtr mMemoryMinimizerRunnable;

    static ContentChild* sSingleton;

    DISALLOW_EVIL_CONSTRUCTORS(ContentChild);
};

} 
} 

#endif
