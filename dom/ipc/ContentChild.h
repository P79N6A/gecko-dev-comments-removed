





#ifndef mozilla_dom_ContentChild_h
#define mozilla_dom_ContentChild_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/PContentChild.h"
#include "mozilla/dom/TabContext.h"
#include "mozilla/dom/ipc/Blob.h"

#include "nsTArray.h"
#include "nsIConsoleListener.h"

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
                   , public TabContext
{
    typedef mozilla::dom::ClonedMessageData ClonedMessageData;
    typedef mozilla::ipc::OptionalURIParams OptionalURIParams;
    typedef mozilla::ipc::URIParams URIParams;

public:
    ContentChild();
    virtual ~ContentChild();

    struct AppInfo
    {
        nsCString version;
        nsCString buildID;
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
    const void GetProcessName(nsAString& aName);

    PCompositorChild*
    AllocPCompositor(mozilla::ipc::Transport* aTransport,
                     base::ProcessId aOtherProcess) MOZ_OVERRIDE;
    PImageBridgeChild*
    AllocPImageBridge(mozilla::ipc::Transport* aTransport,
                      base::ProcessId aOtherProcess) MOZ_OVERRIDE;

    virtual PBrowserChild* AllocPBrowser(const IPCTabContext &aContext,
                                         const uint32_t &chromeFlags);
    virtual bool DeallocPBrowser(PBrowserChild*);

    virtual PDeviceStorageRequestChild* AllocPDeviceStorageRequest(const DeviceStorageParams&);
    virtual bool DeallocPDeviceStorageRequest(PDeviceStorageRequestChild*);

    virtual PBlobChild* AllocPBlob(const BlobConstructorParams& aParams);
    virtual bool DeallocPBlob(PBlobChild*);

    virtual PCrashReporterChild*
    AllocPCrashReporter(const mozilla::dom::NativeThreadId& id,
                        const uint32_t& processType);
    virtual bool
    DeallocPCrashReporter(PCrashReporterChild*);

    virtual PHalChild* AllocPHal() MOZ_OVERRIDE;
    virtual bool DeallocPHal(PHalChild*) MOZ_OVERRIDE;

    virtual PIndexedDBChild* AllocPIndexedDB();
    virtual bool DeallocPIndexedDB(PIndexedDBChild* aActor);

    virtual PMemoryReportRequestChild*
    AllocPMemoryReportRequest();

    virtual bool
    DeallocPMemoryReportRequest(PMemoryReportRequestChild* actor);

    virtual bool
    RecvPMemoryReportRequestConstructor(PMemoryReportRequestChild* child);

    virtual bool
    RecvAudioChannelNotify();

    virtual bool
    RecvDumpMemoryReportsToFile(const nsString& aIdentifier,
                                const bool& aMinimizeMemoryUsage,
                                const bool& aDumpChildProcesses);
    virtual bool
    RecvDumpGCAndCCLogsToFile(const nsString& aIdentifier,
                              const bool& aDumpChildProcesses);

    virtual PTestShellChild* AllocPTestShell();
    virtual bool DeallocPTestShell(PTestShellChild*);
    virtual bool RecvPTestShellConstructor(PTestShellChild*);

    virtual PNeckoChild* AllocPNecko();
    virtual bool DeallocPNecko(PNeckoChild*);

    virtual PExternalHelperAppChild *AllocPExternalHelperApp(
            const OptionalURIParams& uri,
            const nsCString& aMimeContentType,
            const nsCString& aContentDisposition,
            const bool& aForceSave,
            const int64_t& aContentLength,
            const OptionalURIParams& aReferrer);
    virtual bool DeallocPExternalHelperApp(PExternalHelperAppChild *aService);

    virtual PSmsChild* AllocPSms();
    virtual bool DeallocPSms(PSmsChild*);

    virtual PStorageChild* AllocPStorage(const StorageConstructData& aData);
    virtual bool DeallocPStorage(PStorageChild* aActor);

    virtual PBluetoothChild* AllocPBluetooth();
    virtual bool DeallocPBluetooth(PBluetoothChild* aActor);

    virtual bool RecvRegisterChrome(const InfallibleTArray<ChromePackage>& packages,
                                    const InfallibleTArray<ResourceMapping>& resources,
                                    const InfallibleTArray<OverrideMapping>& overrides,
                                    const nsCString& locale);

    virtual bool RecvSetOffline(const bool& offline);

    virtual bool RecvNotifyVisited(const URIParams& aURI);
    
    nsresult AddRemoteAlertObserver(const nsString& aData, nsIObserver* aObserver);

    virtual bool RecvPreferenceUpdate(const PrefSetting& aPref);

    virtual bool RecvNotifyAlertsObserver(const nsCString& aType, const nsString& aData);

    virtual bool RecvAsyncMessage(const nsString& aMsg,
                                  const ClonedMessageData& aData);

    virtual bool RecvGeolocationUpdate(const GeoPosition& somewhere);

    virtual bool RecvAddPermission(const IPC::Permission& permission);

    virtual bool RecvScreenSizeChanged(const gfxIntSize &size);

    virtual bool RecvFlushMemory(const nsString& reason);

    virtual bool RecvActivateA11y();

    virtual bool RecvGarbageCollect();
    virtual bool RecvCycleCollect();

    virtual bool RecvAppInfo(const nsCString& version, const nsCString& buildID);

    virtual bool RecvLastPrivateDocShellDestroyed();

    virtual bool RecvFilePathUpdate(const nsString& type, const nsString& path, const nsCString& reason);
    virtual bool RecvFileSystemUpdate(const nsString& aFsName,
                                      const nsString& aName,
                                      const int32_t& aState,
                                      const int32_t& aMountGeneration);

#ifdef ANDROID
    gfxIntSize GetScreenSize() { return mScreenSize; }
#endif

    
    
    nsString &GetIndexedDBPath();

    uint64_t GetID() { return mID; }

    BlobChild* GetOrCreateActorForBlob(nsIDOMBlob* aBlob);

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

    static ContentChild* sSingleton;

    DISALLOW_EVIL_CONSTRUCTORS(ContentChild);
};

} 
} 

#endif
