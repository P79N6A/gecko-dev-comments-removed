





#ifndef mozilla_dom_ContentParent_h
#define mozilla_dom_ContentParent_h

#include "mozilla/dom/PContentParent.h"
#include "mozilla/dom/nsIContentParent.h"
#include "mozilla/ipc/GeckoChildProcessHost.h"
#include "mozilla/Attributes.h"
#include "mozilla/FileUtils.h"
#include "mozilla/HalTypes.h"
#include "mozilla/LinkedList.h"
#include "mozilla/StaticPtr.h"

#include "nsDataHashtable.h"
#include "nsFrameMessageManager.h"
#include "nsHashKeys.h"
#include "nsIObserver.h"
#include "nsIThreadInternal.h"
#include "nsIDOMGeoPositionCallback.h"
#include "nsIDOMGeoPositionErrorCallback.h"
#include "PermissionMessageUtils.h"

#define CHILD_PROCESS_SHUTDOWN_MESSAGE NS_LITERAL_STRING("child-process-shutdown")

class mozIApplication;
class nsConsoleService;
class nsICycleCollectorLogSink;
class nsIDOMBlob;
class nsIDumpGCAndCCLogsCallback;
class nsIMemoryReporter;
class nsITimer;
class ParentIdleListener;
class nsIWidget;

namespace mozilla {
class PRemoteSpellcheckEngineParent;

namespace ipc {
class OptionalURIParams;
class PFileDescriptorSetParent;
class URIParams;
class TestShellParent;
} 

namespace jsipc {
class JavaScriptShared;
class PJavaScriptParent;
}

namespace layers {
class PCompositorParent;
class PSharedBufferManagerParent;
} 

namespace dom {

class Element;
class TabParent;
class PStorageParent;
class ClonedMessageData;
class MemoryReport;
class TabContext;
class ContentBridgeParent;

class ContentParent final : public PContentParent
                          , public nsIContentParent
                          , public nsIObserver
                          , public nsIDOMGeoPositionCallback
                          , public nsIDOMGeoPositionErrorCallback
                          , public mozilla::LinkedListElement<ContentParent>
{
    typedef mozilla::ipc::GeckoChildProcessHost GeckoChildProcessHost;
    typedef mozilla::ipc::OptionalURIParams OptionalURIParams;
    typedef mozilla::ipc::PFileDescriptorSetParent PFileDescriptorSetParent;
    typedef mozilla::ipc::TestShellParent TestShellParent;
    typedef mozilla::ipc::URIParams URIParams;
    typedef mozilla::dom::ClonedMessageData ClonedMessageData;

public:
#ifdef MOZ_NUWA_PROCESS
    static int32_t NuwaPid() {
        return sNuwaPid;
    }

    static bool IsNuwaReady() {
        return sNuwaReady;
    }
#endif
    virtual bool IsContentParent() override { return true; }
    



    static void StartUp();
    
    static void ShutDown();
    





    static void JoinAllSubprocesses();

    static bool PreallocatedProcessReady();

    





    static already_AddRefed<ContentParent>
    GetNewOrUsedBrowserProcess(bool aForBrowserElement = false,
                               hal::ProcessPriority aPriority =
                               hal::ProcessPriority::PROCESS_PRIORITY_FOREGROUND,
                               ContentParent* aOpener = nullptr);

    


    static already_AddRefed<ContentParent> PreallocateAppProcess();

    static already_AddRefed<ContentParent> RunNuwaProcess();

    




    static TabParent*
    CreateBrowserOrApp(const TabContext& aContext,
                       Element* aFrameElement,
                       ContentParent* aOpenerContentParent);

    static void GetAll(nsTArray<ContentParent*>& aArray);
    static void GetAllEvenIfDead(nsTArray<ContentParent*>& aArray);

    static bool IgnoreIPCPrincipal();

    static void NotifyUpdatedDictionaries();

#if defined(XP_WIN)
    






    static void SendAsyncUpdate(nsIWidget* aWidget);
#endif

    
    bool IsDestroyed() { return !mIPCOpen; }

    virtual bool RecvCreateChildProcess(const IPCTabContext& aContext,
                                        const hal::ProcessPriority& aPriority,
                                        const TabId& aOpenerTabId,
                                        ContentParentId* aCpId,
                                        bool* aIsForApp,
                                        bool* aIsForBrowser,
                                        TabId* aTabId) override;
    virtual bool RecvBridgeToChildProcess(const ContentParentId& aCpId) override;

    virtual bool RecvCreateGMPService() override;
    virtual bool RecvGetGMPPluginVersionForAPI(const nsCString& aAPI,
                                               nsTArray<nsCString>&& aTags,
                                               bool* aHasPlugin,
                                               nsCString* aVersion) override;

    virtual bool RecvLoadPlugin(const uint32_t& aPluginId, nsresult* aRv, uint32_t* aRunID) override;
    virtual bool RecvConnectPluginBridge(const uint32_t& aPluginId, nsresult* aRv) override;
    virtual bool RecvFindPlugins(const uint32_t& aPluginEpoch,
                                 nsTArray<PluginTag>* aPlugins,
                                 uint32_t* aNewPluginEpoch) override;

    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(ContentParent, nsIObserver)

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_NSIOBSERVER
    NS_DECL_NSIDOMGEOPOSITIONCALLBACK
    NS_DECL_NSIDOMGEOPOSITIONERRORCALLBACK

    


    virtual bool DoLoadMessageManagerScript(const nsAString& aURL,
                                            bool aRunInGlobalScope) override;
    virtual bool DoSendAsyncMessage(JSContext* aCx,
                                    const nsAString& aMessage,
                                    const mozilla::dom::StructuredCloneData& aData,
                                    JS::Handle<JSObject *> aCpows,
                                    nsIPrincipal* aPrincipal) override;
    virtual bool CheckPermission(const nsAString& aPermission) override;
    virtual bool CheckManifestURL(const nsAString& aManifestURL) override;
    virtual bool CheckAppHasPermission(const nsAString& aPermission) override;
    virtual bool CheckAppHasStatus(unsigned short aStatus) override;
    virtual bool KillChild() override;

    
    void NotifyTabDestroying(PBrowserParent* aTab);
    
    void NotifyTabDestroyed(PBrowserParent* aTab,
                            bool aNotifiedDestroying);

    TestShellParent* CreateTestShell();
    bool DestroyTestShell(TestShellParent* aTestShell);
    TestShellParent* GetTestShellSingleton();
    jsipc::CPOWManager* GetCPOWManager() override;

    static TabId
    AllocateTabId(const TabId& aOpenerTabId,
                  const IPCTabContext& aContext,
                  const ContentParentId& aCpId);
    static void
    DeallocateTabId(const TabId& aTabId, const ContentParentId& aCpId);

    void ReportChildAlreadyBlocked();
    bool RequestRunToCompletion();

    bool IsAlive();
    virtual bool IsForApp() override;
    virtual bool IsForBrowser() override
    {
      return mIsForBrowser;
    }
#ifdef MOZ_NUWA_PROCESS
    bool IsNuwaProcess() const;
#endif

    GeckoChildProcessHost* Process() {
        return mSubprocess;
    }

    int32_t Pid();

    ContentParent* Opener() {
        return mOpener;
    }

    bool NeedsPermissionsUpdate() const {
#ifdef MOZ_NUWA_PROCESS
        return !IsNuwaProcess() && mSendPermissionUpdates;
#else
        return mSendPermissionUpdates;
#endif
    }

    bool NeedsDataStoreInfos() const {
        return mSendDataStoreInfos;
    }

    




    void KillHard(const char* aWhy);

    



    bool IsKillHardAnnotationSet() { return mKillHardAnnotation.IsEmpty(); }
    const nsCString& GetKillHardAnnotation() { return mKillHardAnnotation; }
    void SetKillHardAnnotation(const nsACString& aReason) {
      mKillHardAnnotation = aReason;
    }

    ContentParentId ChildID() override { return mChildID; }
    const nsString& AppManifestURL() const { return mAppManifestURL; }

    bool IsPreallocated();

    





    void FriendlyName(nsAString& aName, bool aAnonymize = false);

    virtual void OnChannelError() override;

    virtual void OnBeginSyncTransaction() override;

    virtual PCrashReporterParent*
    AllocPCrashReporterParent(const NativeThreadId& tid,
                              const uint32_t& processType) override;
    virtual bool
    RecvPCrashReporterConstructor(PCrashReporterParent* actor,
                                  const NativeThreadId& tid,
                                  const uint32_t& processType) override;

    virtual PNeckoParent* AllocPNeckoParent() override;
    virtual bool RecvPNeckoConstructor(PNeckoParent* aActor) override {
        return PContentParent::RecvPNeckoConstructor(aActor);
    }

    virtual PPrintingParent* AllocPPrintingParent() override;
    virtual bool RecvPPrintingConstructor(PPrintingParent* aActor) override;
    virtual bool DeallocPPrintingParent(PPrintingParent* aActor) override;

    virtual PScreenManagerParent*
    AllocPScreenManagerParent(uint32_t* aNumberOfScreens,
                              float* aSystemDefaultScale,
                              bool* aSuccess) override;
    virtual bool DeallocPScreenManagerParent(PScreenManagerParent* aActor) override;

    virtual PHalParent* AllocPHalParent() override;
    virtual bool RecvPHalConstructor(PHalParent* aActor) override {
        return PContentParent::RecvPHalConstructor(aActor);
    }

    virtual PStorageParent* AllocPStorageParent() override;
    virtual bool RecvPStorageConstructor(PStorageParent* aActor) override {
        return PContentParent::RecvPStorageConstructor(aActor);
    }

    virtual PJavaScriptParent*
    AllocPJavaScriptParent() override;
    virtual bool
    RecvPJavaScriptConstructor(PJavaScriptParent* aActor) override {
        return PContentParent::RecvPJavaScriptConstructor(aActor);
    }
    virtual PRemoteSpellcheckEngineParent* AllocPRemoteSpellcheckEngineParent() override;

    virtual bool RecvRecordingDeviceEvents(const nsString& aRecordingStatus,
                                           const nsString& aPageURL,
                                           const bool& aIsAudio,
                                           const bool& aIsVideo) override;

    bool CycleCollectWithLogs(bool aDumpAllTraces,
                              nsICycleCollectorLogSink* aSink,
                              nsIDumpGCAndCCLogsCallback* aCallback);

    virtual PBlobParent* SendPBlobConstructor(
        PBlobParent* aActor,
        const BlobConstructorParams& aParams) override;

    virtual bool RecvAllocateTabId(const TabId& aOpenerTabId,
                                   const IPCTabContext& aContext,
                                   const ContentParentId& aCpId,
                                   TabId* aTabId) override;

    virtual bool RecvDeallocateTabId(const TabId& aTabId) override;

    nsTArray<TabContext> GetManagedTabContext();

    virtual POfflineCacheUpdateParent*
    AllocPOfflineCacheUpdateParent(const URIParams& aManifestURI,
                                   const URIParams& aDocumentURI,
                                   const bool& aStickDocument,
                                   const TabId& aTabId) override;
    virtual bool
    RecvPOfflineCacheUpdateConstructor(POfflineCacheUpdateParent* aActor,
                                       const URIParams& aManifestURI,
                                       const URIParams& aDocumentURI,
                                       const bool& stickDocument,
                                       const TabId& aTabId) override;
    virtual bool
    DeallocPOfflineCacheUpdateParent(POfflineCacheUpdateParent* aActor) override;

    virtual bool RecvSetOfflinePermission(const IPC::Principal& principal) override;

    virtual bool RecvFinishShutdown() override;

    void MaybeInvokeDragSession(TabParent* aParent);
protected:
    void OnChannelConnected(int32_t pid) override;
    virtual void ActorDestroy(ActorDestroyReason why) override;
    void OnNuwaForkTimeout();

    bool ShouldContinueFromReplyTimeout() override;

private:
    static nsDataHashtable<nsStringHashKey, ContentParent*> *sAppContentParents;
    static nsTArray<ContentParent*>* sNonAppContentParents;
    static nsTArray<ContentParent*>* sPrivateContent;
    static StaticAutoPtr<LinkedList<ContentParent> > sContentParents;

    static void JoinProcessesIOThread(const nsTArray<ContentParent*>* aProcesses,
                                      Monitor* aMonitor, bool* aDone);

    
    
    
    static already_AddRefed<ContentParent>
    GetNewOrPreallocatedAppProcess(mozIApplication* aApp,
                                   hal::ProcessPriority aInitialPriority,
                                   ContentParent* aOpener,
                                    bool* aTookPreAllocated = nullptr);

    static hal::ProcessPriority GetInitialProcessPriority(Element* aFrameElement);

    static ContentBridgeParent* CreateContentBridgeParent(const TabContext& aContext,
                                                          const hal::ProcessPriority& aPriority,
                                                          const TabId& aOpenerTabId,
                                                           TabId* aTabId);

    
    
    virtual PBrowserParent* SendPBrowserConstructor(
        PBrowserParent* actor,
        const TabId& aTabId,
        const IPCTabContext& context,
        const uint32_t& chromeFlags,
        const ContentParentId& aCpId,
        const bool& aIsForApp,
        const bool& aIsForBrowser) override;
    using PContentParent::SendPTestShellConstructor;

    
    
    ContentParent(mozIApplication* aApp,
                  ContentParent* aOpener,
                  bool aIsForBrowser,
                  bool aIsForPreallocated,
                  hal::ProcessPriority aInitialPriority = hal::PROCESS_PRIORITY_FOREGROUND,
                  bool aIsNuwaProcess = false);

#ifdef MOZ_NUWA_PROCESS
    ContentParent(ContentParent* aTemplate,
                  const nsAString& aAppManifestURL,
                  base::ProcessHandle aPid,
                  InfallibleTArray<ProtocolFdMapping>&& aFds);
#endif

    
    void InitializeMembers();

    
    void InitInternal(ProcessPriority aPriority,
                      bool aSetupOffMainThreadCompositing,
                      bool aSendRegisteredChrome);

    virtual ~ContentParent();

    void Init();

    
    
    
    void ForwardKnownInfo();

    
    
    
    
    void MaybeTakeCPUWakeLock(Element* aFrameElement);

    
    
    
    
    bool SetPriorityAndCheckIsAlive(hal::ProcessPriority aPriority);

    
    
    void TransformPreallocatedIntoApp(ContentParent* aOpener,
                                      const nsAString& aAppManifestURL);

    
    
    void TransformPreallocatedIntoBrowser(ContentParent* aOpener);

    



    void MarkAsDead();

    


    enum ShutDownMethod {
        
        SEND_SHUTDOWN_MESSAGE,
        
        CLOSE_CHANNEL,
        
        CLOSE_CHANNEL_WITH_ERROR,
    };

    










    void ShutDownProcess(ShutDownMethod aMethod);

    
    
    void ShutDownMessageManager();

    
    void StartForceKillTimer();

    static void ForceKillTimerCallback(nsITimer* aTimer, void* aClosure);

    PGMPServiceParent*
    AllocPGMPServiceParent(mozilla::ipc::Transport* aTransport,
                           base::ProcessId aOtherProcess) override;
    PCompositorParent*
    AllocPCompositorParent(mozilla::ipc::Transport* aTransport,
                           base::ProcessId aOtherProcess) override;
    PImageBridgeParent*
    AllocPImageBridgeParent(mozilla::ipc::Transport* aTransport,
                            base::ProcessId aOtherProcess) override;

    PSharedBufferManagerParent*
    AllocPSharedBufferManagerParent(mozilla::ipc::Transport* aTranport,
                                     base::ProcessId aOtherProcess) override;
    PBackgroundParent*
    AllocPBackgroundParent(Transport* aTransport, ProcessId aOtherProcess)
                           override;

    PProcessHangMonitorParent*
    AllocPProcessHangMonitorParent(Transport* aTransport,
                                   ProcessId aOtherProcess) override;

    virtual bool RecvGetProcessAttributes(ContentParentId* aCpId,
                                          bool* aIsForApp,
                                          bool* aIsForBrowser) override;
    virtual bool RecvGetXPCOMProcessAttributes(bool* aIsOffline,
                                               bool* aIsConnected,
                                               InfallibleTArray<nsString>* dictionaries,
                                               ClipboardCapabilities* clipboardCaps,
                                               DomainPolicyClone* domainPolicy)
        override;

    virtual bool DeallocPJavaScriptParent(mozilla::jsipc::PJavaScriptParent*) override;

    virtual bool DeallocPRemoteSpellcheckEngineParent(PRemoteSpellcheckEngineParent*) override;
    virtual PBrowserParent* AllocPBrowserParent(const TabId& aTabId,
                                                const IPCTabContext& aContext,
                                                const uint32_t& aChromeFlags,
                                                const ContentParentId& aCpId,
                                                const bool& aIsForApp,
                                                const bool& aIsForBrowser) override;
    virtual bool DeallocPBrowserParent(PBrowserParent* frame) override;

    virtual PDeviceStorageRequestParent*
    AllocPDeviceStorageRequestParent(const DeviceStorageParams&) override;
    virtual bool DeallocPDeviceStorageRequestParent(PDeviceStorageRequestParent*) override;

    virtual PFileSystemRequestParent*
    AllocPFileSystemRequestParent(const FileSystemParams&) override;

    virtual bool
    DeallocPFileSystemRequestParent(PFileSystemRequestParent*) override;

    virtual PBlobParent* AllocPBlobParent(const BlobConstructorParams& aParams)
                                          override;
    virtual bool DeallocPBlobParent(PBlobParent* aActor) override;

    virtual bool DeallocPCrashReporterParent(PCrashReporterParent* crashreporter) override;

    virtual bool RecvGetRandomValues(const uint32_t& length,
                                     InfallibleTArray<uint8_t>* randomValues) override;

    virtual bool RecvIsSecureURI(const uint32_t& aType, const URIParams& aURI,
                                 const uint32_t& aFlags, bool* aIsSecureURI) override;

    virtual bool DeallocPHalParent(PHalParent*) override;

    virtual PIccParent* AllocPIccParent(const uint32_t& aServiceId) override;
    virtual bool DeallocPIccParent(PIccParent* aActor) override;

    virtual PMemoryReportRequestParent*
    AllocPMemoryReportRequestParent(const uint32_t& aGeneration,
                                    const bool &aAnonymize,
                                    const bool &aMinimizeMemoryUsage,
                                    const MaybeFileDesc &aDMDFile) override;
    virtual bool DeallocPMemoryReportRequestParent(PMemoryReportRequestParent* actor) override;

    virtual PCycleCollectWithLogsParent*
    AllocPCycleCollectWithLogsParent(const bool& aDumpAllTraces,
                                     const FileDescriptor& aGCLog,
                                     const FileDescriptor& aCCLog) override;
    virtual bool
    DeallocPCycleCollectWithLogsParent(PCycleCollectWithLogsParent* aActor) override;

    virtual PTestShellParent* AllocPTestShellParent() override;
    virtual bool DeallocPTestShellParent(PTestShellParent* shell) override;

    virtual PMobileConnectionParent* AllocPMobileConnectionParent(const uint32_t& aClientId) override;
    virtual bool DeallocPMobileConnectionParent(PMobileConnectionParent* aActor) override;

    virtual bool DeallocPNeckoParent(PNeckoParent* necko) override;

    virtual PExternalHelperAppParent* AllocPExternalHelperAppParent(
            const OptionalURIParams& aUri,
            const nsCString& aMimeContentType,
            const nsCString& aContentDisposition,
            const uint32_t& aContentDispositionHint,
            const nsString& aContentDispositionFilename,
            const bool& aForceSave,
            const int64_t& aContentLength,
            const OptionalURIParams& aReferrer,
            PBrowserParent* aBrowser) override;
    virtual bool DeallocPExternalHelperAppParent(PExternalHelperAppParent* aService) override;

    virtual PCellBroadcastParent* AllocPCellBroadcastParent() override;
    virtual bool DeallocPCellBroadcastParent(PCellBroadcastParent*) override;
    virtual bool RecvPCellBroadcastConstructor(PCellBroadcastParent* aActor) override;

    virtual PSmsParent* AllocPSmsParent() override;
    virtual bool DeallocPSmsParent(PSmsParent*) override;

    virtual PTelephonyParent* AllocPTelephonyParent() override;
    virtual bool DeallocPTelephonyParent(PTelephonyParent*) override;

    virtual PVoicemailParent* AllocPVoicemailParent() override;
    virtual bool RecvPVoicemailConstructor(PVoicemailParent* aActor) override;
    virtual bool DeallocPVoicemailParent(PVoicemailParent* aActor) override;

    virtual bool DeallocPStorageParent(PStorageParent* aActor) override;

    virtual PBluetoothParent* AllocPBluetoothParent() override;
    virtual bool DeallocPBluetoothParent(PBluetoothParent* aActor) override;
    virtual bool RecvPBluetoothConstructor(PBluetoothParent* aActor) override;

    virtual PFMRadioParent* AllocPFMRadioParent() override;
    virtual bool DeallocPFMRadioParent(PFMRadioParent* aActor) override;

    virtual PAsmJSCacheEntryParent* AllocPAsmJSCacheEntryParent(
                                 const asmjscache::OpenMode& aOpenMode,
                                 const asmjscache::WriteParams& aWriteParams,
                                 const IPC::Principal& aPrincipal) override;
    virtual bool DeallocPAsmJSCacheEntryParent(
                                   PAsmJSCacheEntryParent* aActor) override;

    virtual PSpeechSynthesisParent* AllocPSpeechSynthesisParent() override;
    virtual bool DeallocPSpeechSynthesisParent(PSpeechSynthesisParent* aActor) override;
    virtual bool RecvPSpeechSynthesisConstructor(PSpeechSynthesisParent* aActor) override;

    virtual bool RecvReadPrefsArray(InfallibleTArray<PrefSetting>* aPrefs) override;
    virtual bool RecvReadFontList(InfallibleTArray<FontListEntry>* retValue) override;

    virtual bool RecvReadPermissions(InfallibleTArray<IPC::Permission>* aPermissions) override;

    virtual bool RecvSetClipboardText(const nsString& text,
                                      const bool& isPrivateData,
                                      const int32_t& whichClipboard) override;
    virtual bool RecvGetClipboardText(const int32_t& whichClipboard, nsString* text) override;
    virtual bool RecvEmptyClipboard(const int32_t& whichClipboard) override;
    virtual bool RecvClipboardHasText(const int32_t& whichClipboard, bool* hasText) override;

    virtual bool RecvGetSystemColors(const uint32_t& colorsCount,
                                     InfallibleTArray<uint32_t>* colors) override;
    virtual bool RecvGetIconForExtension(const nsCString& aFileExt,
                                         const uint32_t& aIconSize,
                                         InfallibleTArray<uint8_t>* bits) override;
    virtual bool RecvGetShowPasswordSetting(bool* showPassword) override;

    virtual bool RecvStartVisitedQuery(const URIParams& uri) override;

    virtual bool RecvVisitURI(const URIParams& uri,
                              const OptionalURIParams& referrer,
                              const uint32_t& flags) override;

    virtual bool RecvSetURITitle(const URIParams& uri,
                                 const nsString& title) override;

    virtual bool RecvShowAlertNotification(const nsString& aImageUrl, const nsString& aTitle,
                                           const nsString& aText, const bool& aTextClickable,
                                           const nsString& aCookie, const nsString& aName,
                                           const nsString& aBidi, const nsString& aLang,
                                           const nsString& aData,
                                           const IPC::Principal& aPrincipal,
                                           const bool& aInPrivateBrowsing) override;

    virtual bool RecvCloseAlert(const nsString& aName,
                                const IPC::Principal& aPrincipal) override;

    virtual bool RecvLoadURIExternal(const URIParams& uri) override;

    virtual bool RecvSyncMessage(const nsString& aMsg,
                                 const ClonedMessageData& aData,
                                 InfallibleTArray<CpowEntry>&& aCpows,
                                 const IPC::Principal& aPrincipal,
                                 InfallibleTArray<nsString>* aRetvals) override;
    virtual bool RecvRpcMessage(const nsString& aMsg,
                                const ClonedMessageData& aData,
                                InfallibleTArray<CpowEntry>&& aCpows,
                                const IPC::Principal& aPrincipal,
                                InfallibleTArray<nsString>* aRetvals) override;
    virtual bool RecvAsyncMessage(const nsString& aMsg,
                                  const ClonedMessageData& aData,
                                  InfallibleTArray<CpowEntry>&& aCpows,
                                  const IPC::Principal& aPrincipal) override;

    virtual bool RecvFilePathUpdateNotify(const nsString& aType,
                                          const nsString& aStorageName,
                                          const nsString& aFilePath,
                                          const nsCString& aReason) override;

    virtual bool RecvAddGeolocationListener(const IPC::Principal& aPrincipal,
                                            const bool& aHighAccuracy) override;
    virtual bool RecvRemoveGeolocationListener() override;
    virtual bool RecvSetGeolocationHigherAccuracy(const bool& aEnable) override;

    virtual bool RecvConsoleMessage(const nsString& aMessage) override;
    virtual bool RecvScriptError(const nsString& aMessage,
                                 const nsString& aSourceName,
                                 const nsString& aSourceLine,
                                 const uint32_t& aLineNumber,
                                 const uint32_t& aColNumber,
                                 const uint32_t& aFlags,
                                 const nsCString& aCategory) override;

    virtual bool RecvPrivateDocShellsExist(const bool& aExist) override;

    virtual bool RecvFirstIdle() override;

    virtual bool RecvAudioChannelGetState(const AudioChannel& aChannel,
                                          const bool& aElementHidden,
                                          const bool& aElementWasHidden,
                                          AudioChannelState* aValue) override;

    virtual bool RecvAudioChannelRegisterType(const AudioChannel& aChannel,
                                              const bool& aWithVideo) override;
    virtual bool RecvAudioChannelUnregisterType(const AudioChannel& aChannel,
                                                const bool& aElementHidden,
                                                const bool& aWithVideo) override;

    virtual bool RecvAudioChannelChangedNotification() override;

    virtual bool RecvAudioChannelChangeDefVolChannel(const int32_t& aChannel,
                                                     const bool& aHidden) override;
    virtual bool RecvGetSystemMemory(const uint64_t& getterId) override;

    virtual bool RecvDataStoreGetStores(
                       const nsString& aName,
                       const nsString& aOwner,
                       const IPC::Principal& aPrincipal,
                       InfallibleTArray<DataStoreSetting>* aValue) override;

    virtual bool RecvSpeakerManagerGetSpeakerStatus(bool* aValue) override;

    virtual bool RecvSpeakerManagerForceSpeaker(const bool& aEnable) override;

    virtual bool RecvSystemMessageHandled() override;

    virtual bool RecvNuwaReady() override;

    virtual bool RecvNuwaWaitForFreeze() override;

    virtual bool RecvAddNewProcess(const uint32_t& aPid,
                                   InfallibleTArray<ProtocolFdMapping>&& aFds) override;

    virtual bool RecvCreateFakeVolume(const nsString& fsName, const nsString& mountPoint) override;

    virtual bool RecvSetFakeVolumeState(const nsString& fsName, const int32_t& fsState) override;

    virtual bool RecvKeywordToURI(const nsCString& aKeyword,
                                  nsString* aProviderName,
                                  OptionalInputStreamParams* aPostData,
                                  OptionalURIParams* aURI) override;

    virtual bool RecvNotifyKeywordSearchLoading(const nsString &aProvider,
                                                const nsString &aKeyword) override;

    virtual void ProcessingError(Result aCode, const char* aMsgName) override;

    virtual bool RecvAllocateLayerTreeId(uint64_t* aId) override;
    virtual bool RecvDeallocateLayerTreeId(const uint64_t& aId) override;

    virtual bool RecvGetGraphicsFeatureStatus(const int32_t& aFeature,
                                              int32_t* aStatus,
                                              bool* aSuccess) override;

    virtual bool RecvAddIdleObserver(const uint64_t& observerId,
                                     const uint32_t& aIdleTimeInS) override;
    virtual bool RecvRemoveIdleObserver(const uint64_t& observerId,
                                        const uint32_t& aIdleTimeInS) override;

    virtual bool
    RecvBackUpXResources(const FileDescriptor& aXSocketFd) override;

    virtual bool
    RecvOpenAnonymousTemporaryFile(FileDescOrError* aFD) override;

    virtual bool
    RecvKeygenProcessValue(const nsString& oldValue, const nsString& challenge,
                           const nsString& keytype, const nsString& keyparams,
                           nsString* newValue) override;

    virtual bool
    RecvKeygenProvideContent(nsString* aAttribute,
                             nsTArray<nsString>* aContent) override;

    virtual PFileDescriptorSetParent*
    AllocPFileDescriptorSetParent(const mozilla::ipc::FileDescriptor&) override;

    virtual bool
    DeallocPFileDescriptorSetParent(PFileDescriptorSetParent*) override;

    virtual bool
    RecvGetFileReferences(const PersistenceType& aPersistenceType,
                          const nsCString& aOrigin,
                          const nsString& aDatabaseName,
                          const int64_t& aFileId,
                          int32_t* aRefCnt,
                          int32_t* aDBRefCnt,
                          int32_t* aSliceRefCnt,
                          bool* aResult) override;

    virtual PDocAccessibleParent* AllocPDocAccessibleParent(PDocAccessibleParent*, const uint64_t&) override;
    virtual bool DeallocPDocAccessibleParent(PDocAccessibleParent*) override;
    virtual bool RecvPDocAccessibleConstructor(PDocAccessibleParent* aDoc,
                                               PDocAccessibleParent* aParentDoc, const uint64_t& aParentID) override;

    virtual bool RecvUpdateDropEffect(const uint32_t& aDragAction,
                                      const uint32_t& aDropEffect) override;
    
    
    

    GeckoChildProcessHost* mSubprocess;
    ContentParent* mOpener;

    ContentParentId mChildID;
    int32_t mGeolocationWatchID;

    nsString mAppManifestURL;

    nsCString mKillHardAnnotation;

    




    nsString mAppName;

    
    
    
    
    nsCOMPtr<nsITimer> mForceKillTimer;
    
    
    
    int32_t mNumDestroyingTabs;
    
    
    
    
    bool mIsAlive;

    
    
    bool mMetamorphosed;

    bool mSendPermissionUpdates;
    bool mSendDataStoreInfos;
    bool mIsForBrowser;
    bool mIsNuwaProcess;

    
    
    bool mCalledClose;
    bool mCalledCloseWithError;
    bool mCalledKillHard;
    bool mCreatedPairedMinidumps;
    bool mShutdownPending;
    bool mIPCOpen;

    friend class CrashReporterParent;

    nsRefPtr<nsConsoleService>  mConsoleService;
    nsConsoleService* GetConsoleService();

    nsDataHashtable<nsUint64HashKey, nsRefPtr<ParentIdleListener> > mIdleListeners;

#ifdef MOZ_X11
    
    
    ScopedClose mChildXSocketFdDup;
#endif

#ifdef MOZ_NUWA_PROCESS
    static int32_t sNuwaPid;
    static bool sNuwaReady;
#endif

    PProcessHangMonitorParent* mHangMonitorActor;
};

} 
} 

class ParentIdleListener : public nsIObserver {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  ParentIdleListener(mozilla::dom::ContentParent* aParent, uint64_t aObserver)
    : mParent(aParent), mObserver(aObserver)
  {}
private:
  virtual ~ParentIdleListener() {}
  nsRefPtr<mozilla::dom::ContentParent> mParent;
  uint64_t mObserver;
};

#endif
