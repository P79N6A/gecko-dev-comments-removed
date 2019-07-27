





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
#include "PermissionMessageUtils.h"

#define CHILD_PROCESS_SHUTDOWN_MESSAGE NS_LITERAL_STRING("child-process-shutdown")

class mozIApplication;
class nsConsoleService;
class nsICycleCollectorLogSink;
class nsIDOMBlob;
class nsIDumpGCAndCCLogsCallback;
class nsIMemoryReporter;
class ParentIdleListener;

namespace mozilla {
class PRemoteSpellcheckEngineParent;

namespace ipc {
class OptionalURIParams;
class PFileDescriptorSetParent;
class URIParams;
class TestShellParent;
} 

namespace jsipc {
class JavaScriptParent;
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

class ContentParent MOZ_FINAL : public PContentParent
                              , public nsIContentParent
                              , public nsIObserver
                              , public nsIDOMGeoPositionCallback
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
    static bool IsNuwaReady() {
        return sNuwaReady;
    }
#endif
    virtual bool IsContentParent() MOZ_OVERRIDE { return true; }
    



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

    virtual bool RecvCreateChildProcess(const IPCTabContext& aContext,
                                        const hal::ProcessPriority& aPriority,
                                        uint64_t* aId,
                                        bool* aIsForApp,
                                        bool* aIsForBrowser) MOZ_OVERRIDE;
    virtual bool AnswerBridgeToChildProcess(const uint64_t& id) MOZ_OVERRIDE;

    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(ContentParent, nsIObserver)

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_NSIOBSERVER
    NS_DECL_NSIDOMGEOPOSITIONCALLBACK

    


    virtual bool DoSendAsyncMessage(JSContext* aCx,
                                    const nsAString& aMessage,
                                    const mozilla::dom::StructuredCloneData& aData,
                                    JS::Handle<JSObject *> aCpows,
                                    nsIPrincipal* aPrincipal) MOZ_OVERRIDE;
    virtual bool CheckPermission(const nsAString& aPermission) MOZ_OVERRIDE;
    virtual bool CheckManifestURL(const nsAString& aManifestURL) MOZ_OVERRIDE;
    virtual bool CheckAppHasPermission(const nsAString& aPermission) MOZ_OVERRIDE;
    virtual bool CheckAppHasStatus(unsigned short aStatus) MOZ_OVERRIDE;
    virtual bool KillChild() MOZ_OVERRIDE;

    
    void NotifyTabDestroying(PBrowserParent* aTab);
    
    void NotifyTabDestroyed(PBrowserParent* aTab,
                            bool aNotifiedDestroying);

    TestShellParent* CreateTestShell();
    bool DestroyTestShell(TestShellParent* aTestShell);
    TestShellParent* GetTestShellSingleton();
    jsipc::JavaScriptParent *GetCPOWManager();

    void ReportChildAlreadyBlocked();
    bool RequestRunToCompletion();

    bool IsAlive();
    virtual bool IsForApp() MOZ_OVERRIDE;
    virtual bool IsForBrowser() MOZ_OVERRIDE
    {
      return mIsForBrowser;
    }
#ifdef MOZ_NUWA_PROCESS
    bool IsNuwaProcess();
#endif

    GeckoChildProcessHost* Process() {
        return mSubprocess;
    }

    int32_t Pid();

    ContentParent* Opener() {
        return mOpener;
    }

    bool NeedsPermissionsUpdate() const {
        return mSendPermissionUpdates;
    }

    bool NeedsDataStoreInfos() const {
        return mSendDataStoreInfos;
    }

    




    void KillHard();

    uint64_t ChildID() MOZ_OVERRIDE { return mChildID; }
    const nsString& AppManifestURL() const { return mAppManifestURL; }

    bool IsPreallocated();

    





    void FriendlyName(nsAString& aName, bool aAnonymize = false);

    virtual void OnChannelError() MOZ_OVERRIDE;

    virtual PCrashReporterParent*
    AllocPCrashReporterParent(const NativeThreadId& tid,
                              const uint32_t& processType) MOZ_OVERRIDE;
    virtual bool
    RecvPCrashReporterConstructor(PCrashReporterParent* actor,
                                  const NativeThreadId& tid,
                                  const uint32_t& processType) MOZ_OVERRIDE;

    virtual PNeckoParent* AllocPNeckoParent() MOZ_OVERRIDE;
    virtual bool RecvPNeckoConstructor(PNeckoParent* aActor) MOZ_OVERRIDE {
        return PContentParent::RecvPNeckoConstructor(aActor);
    }

    virtual PScreenManagerParent*
    AllocPScreenManagerParent(uint32_t* aNumberOfScreens,
                              float* aSystemDefaultScale,
                              bool* aSuccess) MOZ_OVERRIDE;
    virtual bool DeallocPScreenManagerParent(PScreenManagerParent* aActor) MOZ_OVERRIDE;

    virtual PHalParent* AllocPHalParent() MOZ_OVERRIDE;
    virtual bool RecvPHalConstructor(PHalParent* aActor) MOZ_OVERRIDE {
        return PContentParent::RecvPHalConstructor(aActor);
    }

    virtual PStorageParent* AllocPStorageParent() MOZ_OVERRIDE;
    virtual bool RecvPStorageConstructor(PStorageParent* aActor) MOZ_OVERRIDE {
        return PContentParent::RecvPStorageConstructor(aActor);
    }

    virtual PJavaScriptParent*
    AllocPJavaScriptParent() MOZ_OVERRIDE;
    virtual bool
    RecvPJavaScriptConstructor(PJavaScriptParent* aActor) MOZ_OVERRIDE {
        return PContentParent::RecvPJavaScriptConstructor(aActor);
    }
    virtual PRemoteSpellcheckEngineParent* AllocPRemoteSpellcheckEngineParent() MOZ_OVERRIDE;

    virtual bool RecvRecordingDeviceEvents(const nsString& aRecordingStatus,
                                           const nsString& aPageURL,
                                           const bool& aIsAudio,
                                           const bool& aIsVideo) MOZ_OVERRIDE;

    bool CycleCollectWithLogs(bool aDumpAllTraces,
                              nsICycleCollectorLogSink* aSink,
                              nsIDumpGCAndCCLogsCallback* aCallback);

    virtual PBlobParent* SendPBlobConstructor(
        PBlobParent* aActor,
        const BlobConstructorParams& aParams) MOZ_OVERRIDE;

protected:
    void OnChannelConnected(int32_t pid) MOZ_OVERRIDE;
    virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;
    void OnNuwaForkTimeout();

    bool ShouldContinueFromReplyTimeout() MOZ_OVERRIDE;

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
                                                          const hal::ProcessPriority& aPriority);

    
    
    virtual PBrowserParent* SendPBrowserConstructor(
        PBrowserParent* actor,
        const IPCTabContext& context,
        const uint32_t& chromeFlags,
        const uint64_t& aId,
        const bool& aIsForApp,
        const bool& aIsForBrowser) MOZ_OVERRIDE;
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
                  const nsTArray<ProtocolFdMapping>& aFds);
#endif

    
    void InitializeMembers();

    
    void InitInternal(ProcessPriority aPriority,
                      bool aSetupOffMainThreadCompositing,
                      bool aSendRegisteredChrome);

    virtual ~ContentParent();

    void Init();

    
    
    
    
    void MaybeTakeCPUWakeLock(Element* aFrameElement);

    
    
    
    
    bool SetPriorityAndCheckIsAlive(hal::ProcessPriority aPriority);

    
    
    void TransformPreallocatedIntoApp(ContentParent* aOpener,
                                      const nsAString& aAppManifestURL);

    
    
    void TransformPreallocatedIntoBrowser(ContentParent* aOpener);

    



    void MarkAsDead();

    










    void ShutDownProcess(bool aCloseWithError);

    
    
    void ShutDownMessageManager();

    PCompositorParent*
    AllocPCompositorParent(mozilla::ipc::Transport* aTransport,
                           base::ProcessId aOtherProcess) MOZ_OVERRIDE;
    PImageBridgeParent*
    AllocPImageBridgeParent(mozilla::ipc::Transport* aTransport,
                            base::ProcessId aOtherProcess) MOZ_OVERRIDE;

    PSharedBufferManagerParent*
    AllocPSharedBufferManagerParent(mozilla::ipc::Transport* aTranport,
                                     base::ProcessId aOtherProcess) MOZ_OVERRIDE;
    PBackgroundParent*
    AllocPBackgroundParent(Transport* aTransport, ProcessId aOtherProcess)
                           MOZ_OVERRIDE;

    virtual bool RecvGetProcessAttributes(uint64_t* aId,
                                          bool* aIsForApp,
                                          bool* aIsForBrowser) MOZ_OVERRIDE;
    virtual bool RecvGetXPCOMProcessAttributes(bool* aIsOffline) MOZ_OVERRIDE;

    virtual bool DeallocPJavaScriptParent(mozilla::jsipc::PJavaScriptParent*) MOZ_OVERRIDE;

    virtual bool DeallocPRemoteSpellcheckEngineParent(PRemoteSpellcheckEngineParent*) MOZ_OVERRIDE;
    virtual PBrowserParent* AllocPBrowserParent(const IPCTabContext& aContext,
                                                const uint32_t& aChromeFlags,
                                                const uint64_t& aId,
                                                const bool& aIsForApp,
                                                const bool& aIsForBrowser) MOZ_OVERRIDE;
    virtual bool DeallocPBrowserParent(PBrowserParent* frame) MOZ_OVERRIDE;

    virtual PDeviceStorageRequestParent*
    AllocPDeviceStorageRequestParent(const DeviceStorageParams&) MOZ_OVERRIDE;
    virtual bool DeallocPDeviceStorageRequestParent(PDeviceStorageRequestParent*) MOZ_OVERRIDE;

    virtual PFileSystemRequestParent*
    AllocPFileSystemRequestParent(const FileSystemParams&) MOZ_OVERRIDE;

    virtual bool
    DeallocPFileSystemRequestParent(PFileSystemRequestParent*) MOZ_OVERRIDE;

    virtual PBlobParent* AllocPBlobParent(const BlobConstructorParams& aParams)
                                          MOZ_OVERRIDE;
    virtual bool DeallocPBlobParent(PBlobParent* aActor) MOZ_OVERRIDE;

    virtual bool DeallocPCrashReporterParent(PCrashReporterParent* crashreporter) MOZ_OVERRIDE;

    virtual bool RecvGetRandomValues(const uint32_t& length,
                                     InfallibleTArray<uint8_t>* randomValues) MOZ_OVERRIDE;

    virtual bool RecvIsSecureURI(const uint32_t& type, const URIParams& uri,
                                 const uint32_t& flags, bool* isSecureURI);

    virtual bool DeallocPHalParent(PHalParent*) MOZ_OVERRIDE;

    virtual PMemoryReportRequestParent*
    AllocPMemoryReportRequestParent(const uint32_t& aGeneration,
                                    const bool &aAnonymize,
                                    const bool &aMinimizeMemoryUsage,
                                    const FileDescriptor &aDMDFile) MOZ_OVERRIDE;
    virtual bool DeallocPMemoryReportRequestParent(PMemoryReportRequestParent* actor) MOZ_OVERRIDE;

    virtual PCycleCollectWithLogsParent*
    AllocPCycleCollectWithLogsParent(const bool& aDumpAllTraces,
                                     const FileDescriptor& aGCLog,
                                     const FileDescriptor& aCCLog) MOZ_OVERRIDE;
    virtual bool
    DeallocPCycleCollectWithLogsParent(PCycleCollectWithLogsParent* aActor) MOZ_OVERRIDE;

    virtual PTestShellParent* AllocPTestShellParent() MOZ_OVERRIDE;
    virtual bool DeallocPTestShellParent(PTestShellParent* shell) MOZ_OVERRIDE;

    virtual PMobileConnectionParent* AllocPMobileConnectionParent(const uint32_t& aClientId) MOZ_OVERRIDE;
    virtual bool DeallocPMobileConnectionParent(PMobileConnectionParent* aActor) MOZ_OVERRIDE;

    virtual bool DeallocPNeckoParent(PNeckoParent* necko) MOZ_OVERRIDE;

    virtual PExternalHelperAppParent* AllocPExternalHelperAppParent(
            const OptionalURIParams& aUri,
            const nsCString& aMimeContentType,
            const nsCString& aContentDisposition,
            const uint32_t& aContentDispositionHint,
            const nsString& aContentDispositionFilename, 
            const bool& aForceSave,
            const int64_t& aContentLength,
            const OptionalURIParams& aReferrer,
            PBrowserParent* aBrowser) MOZ_OVERRIDE;
    virtual bool DeallocPExternalHelperAppParent(PExternalHelperAppParent* aService) MOZ_OVERRIDE;

    virtual PSmsParent* AllocPSmsParent() MOZ_OVERRIDE;
    virtual bool DeallocPSmsParent(PSmsParent*) MOZ_OVERRIDE;

    virtual PTelephonyParent* AllocPTelephonyParent() MOZ_OVERRIDE;
    virtual bool DeallocPTelephonyParent(PTelephonyParent*) MOZ_OVERRIDE;

    virtual bool DeallocPStorageParent(PStorageParent* aActor) MOZ_OVERRIDE;

    virtual PBluetoothParent* AllocPBluetoothParent() MOZ_OVERRIDE;
    virtual bool DeallocPBluetoothParent(PBluetoothParent* aActor) MOZ_OVERRIDE;
    virtual bool RecvPBluetoothConstructor(PBluetoothParent* aActor) MOZ_OVERRIDE;

    virtual PFMRadioParent* AllocPFMRadioParent() MOZ_OVERRIDE;
    virtual bool DeallocPFMRadioParent(PFMRadioParent* aActor) MOZ_OVERRIDE;

    virtual PAsmJSCacheEntryParent* AllocPAsmJSCacheEntryParent(
                                 const asmjscache::OpenMode& aOpenMode,
                                 const asmjscache::WriteParams& aWriteParams,
                                 const IPC::Principal& aPrincipal) MOZ_OVERRIDE;
    virtual bool DeallocPAsmJSCacheEntryParent(
                                   PAsmJSCacheEntryParent* aActor) MOZ_OVERRIDE;

    virtual PSpeechSynthesisParent* AllocPSpeechSynthesisParent() MOZ_OVERRIDE;
    virtual bool DeallocPSpeechSynthesisParent(PSpeechSynthesisParent* aActor) MOZ_OVERRIDE;
    virtual bool RecvPSpeechSynthesisConstructor(PSpeechSynthesisParent* aActor) MOZ_OVERRIDE;

    virtual bool RecvReadPrefsArray(InfallibleTArray<PrefSetting>* aPrefs) MOZ_OVERRIDE;
    virtual bool RecvReadFontList(InfallibleTArray<FontListEntry>* retValue) MOZ_OVERRIDE;

    virtual bool RecvReadPermissions(InfallibleTArray<IPC::Permission>* aPermissions) MOZ_OVERRIDE;

    virtual bool RecvSetClipboardText(const nsString& text,
                                      const bool& isPrivateData,
                                      const int32_t& whichClipboard) MOZ_OVERRIDE;
    virtual bool RecvGetClipboardText(const int32_t& whichClipboard, nsString* text) MOZ_OVERRIDE;
    virtual bool RecvEmptyClipboard(const int32_t& whichClipboard) MOZ_OVERRIDE;
    virtual bool RecvClipboardHasText(const int32_t& whichClipboard, bool* hasText) MOZ_OVERRIDE;

    virtual bool RecvGetSystemColors(const uint32_t& colorsCount,
                                     InfallibleTArray<uint32_t>* colors) MOZ_OVERRIDE;
    virtual bool RecvGetIconForExtension(const nsCString& aFileExt,
                                         const uint32_t& aIconSize,
                                         InfallibleTArray<uint8_t>* bits) MOZ_OVERRIDE;
    virtual bool RecvGetShowPasswordSetting(bool* showPassword) MOZ_OVERRIDE;

    virtual bool RecvStartVisitedQuery(const URIParams& uri) MOZ_OVERRIDE;

    virtual bool RecvVisitURI(const URIParams& uri,
                              const OptionalURIParams& referrer,
                              const uint32_t& flags) MOZ_OVERRIDE;

    virtual bool RecvSetURITitle(const URIParams& uri,
                                 const nsString& title) MOZ_OVERRIDE;

    virtual bool RecvShowAlertNotification(const nsString& aImageUrl, const nsString& aTitle,
                                           const nsString& aText, const bool& aTextClickable,
                                           const nsString& aCookie, const nsString& aName,
                                           const nsString& aBidi, const nsString& aLang,
                                           const nsString& aData,
                                           const IPC::Principal& aPrincipal) MOZ_OVERRIDE;

    virtual bool RecvCloseAlert(const nsString& aName,
                                const IPC::Principal& aPrincipal) MOZ_OVERRIDE;

    virtual bool RecvLoadURIExternal(const URIParams& uri) MOZ_OVERRIDE;

    virtual bool RecvSyncMessage(const nsString& aMsg,
                                 const ClonedMessageData& aData,
                                 const InfallibleTArray<CpowEntry>& aCpows,
                                 const IPC::Principal& aPrincipal,
                                 InfallibleTArray<nsString>* aRetvals) MOZ_OVERRIDE;
    virtual bool AnswerRpcMessage(const nsString& aMsg,
                                  const ClonedMessageData& aData,
                                  const InfallibleTArray<CpowEntry>& aCpows,
                                  const IPC::Principal& aPrincipal,
                                  InfallibleTArray<nsString>* aRetvals) MOZ_OVERRIDE;
    virtual bool RecvAsyncMessage(const nsString& aMsg,
                                  const ClonedMessageData& aData,
                                  const InfallibleTArray<CpowEntry>& aCpows,
                                  const IPC::Principal& aPrincipal) MOZ_OVERRIDE;

    virtual bool RecvFilePathUpdateNotify(const nsString& aType,
                                          const nsString& aStorageName,
                                          const nsString& aFilePath,
                                          const nsCString& aReason) MOZ_OVERRIDE;

    virtual bool RecvAddGeolocationListener(const IPC::Principal& aPrincipal,
                                            const bool& aHighAccuracy) MOZ_OVERRIDE;
    virtual bool RecvRemoveGeolocationListener() MOZ_OVERRIDE;
    virtual bool RecvSetGeolocationHigherAccuracy(const bool& aEnable) MOZ_OVERRIDE;

    virtual bool RecvConsoleMessage(const nsString& aMessage) MOZ_OVERRIDE;
    virtual bool RecvScriptError(const nsString& aMessage,
                                 const nsString& aSourceName,
                                 const nsString& aSourceLine,
                                 const uint32_t& aLineNumber,
                                 const uint32_t& aColNumber,
                                 const uint32_t& aFlags,
                                 const nsCString& aCategory) MOZ_OVERRIDE;

    virtual bool RecvPrivateDocShellsExist(const bool& aExist) MOZ_OVERRIDE;

    virtual bool RecvFirstIdle() MOZ_OVERRIDE;

    virtual bool RecvAudioChannelGetState(const AudioChannel& aChannel,
                                          const bool& aElementHidden,
                                          const bool& aElementWasHidden,
                                          AudioChannelState* aValue) MOZ_OVERRIDE;

    virtual bool RecvAudioChannelRegisterType(const AudioChannel& aChannel,
                                              const bool& aWithVideo) MOZ_OVERRIDE;
    virtual bool RecvAudioChannelUnregisterType(const AudioChannel& aChannel,
                                                const bool& aElementHidden,
                                                const bool& aWithVideo) MOZ_OVERRIDE;

    virtual bool RecvAudioChannelChangedNotification() MOZ_OVERRIDE;

    virtual bool RecvAudioChannelChangeDefVolChannel(const int32_t& aChannel,
                                                     const bool& aHidden) MOZ_OVERRIDE;
    virtual bool RecvGetSystemMemory(const uint64_t& getterId) MOZ_OVERRIDE;
    virtual bool RecvGetVolumes(InfallibleTArray<VolumeInfo>* aResult) MOZ_OVERRIDE;

    virtual bool RecvDataStoreGetStores(
                       const nsString& aName,
                       const nsString& aOwner,
                       const IPC::Principal& aPrincipal,
                       InfallibleTArray<DataStoreSetting>* aValue) MOZ_OVERRIDE;

    virtual bool RecvSpeakerManagerGetSpeakerStatus(bool* aValue) MOZ_OVERRIDE;

    virtual bool RecvSpeakerManagerForceSpeaker(const bool& aEnable) MOZ_OVERRIDE;

    virtual bool RecvSystemMessageHandled() MOZ_OVERRIDE;

    virtual bool RecvNuwaReady() MOZ_OVERRIDE;

    virtual bool RecvAddNewProcess(const uint32_t& aPid,
                                   const InfallibleTArray<ProtocolFdMapping>& aFds) MOZ_OVERRIDE;

    virtual bool RecvCreateFakeVolume(const nsString& fsName, const nsString& mountPoint) MOZ_OVERRIDE;

    virtual bool RecvSetFakeVolumeState(const nsString& fsName, const int32_t& fsState) MOZ_OVERRIDE;

    virtual bool RecvKeywordToURI(const nsCString& aKeyword, OptionalInputStreamParams* aPostData,
                                  OptionalURIParams* aURI) MOZ_OVERRIDE;

    virtual void ProcessingError(Result what) MOZ_OVERRIDE;

    virtual bool RecvAllocateLayerTreeId(uint64_t* aId) MOZ_OVERRIDE;
    virtual bool RecvDeallocateLayerTreeId(const uint64_t& aId) MOZ_OVERRIDE;

    virtual bool RecvGetGraphicsFeatureStatus(const int32_t& aFeature,
                                              int32_t* aStatus,
                                              bool* aSuccess) MOZ_OVERRIDE;

    virtual bool RecvAddIdleObserver(const uint64_t& observerId,
                                     const uint32_t& aIdleTimeInS) MOZ_OVERRIDE;
    virtual bool RecvRemoveIdleObserver(const uint64_t& observerId,
                                        const uint32_t& aIdleTimeInS) MOZ_OVERRIDE;

    virtual bool
    RecvBackUpXResources(const FileDescriptor& aXSocketFd) MOZ_OVERRIDE;

    virtual bool
    RecvOpenAnonymousTemporaryFile(FileDescriptor* aFD) MOZ_OVERRIDE;

    virtual PFileDescriptorSetParent*
    AllocPFileDescriptorSetParent(const mozilla::ipc::FileDescriptor&) MOZ_OVERRIDE;

    virtual bool
    DeallocPFileDescriptorSetParent(PFileDescriptorSetParent*) MOZ_OVERRIDE;

    virtual bool
    RecvGetFileReferences(const PersistenceType& aPersistenceType,
                          const nsCString& aOrigin,
                          const nsString& aDatabaseName,
                          const int64_t& aFileId,
                          int32_t* aRefCnt,
                          int32_t* aDBRefCnt,
                          int32_t* aSliceRefCnt,
                          bool* aResult) MOZ_OVERRIDE;

    
    
    

    GeckoChildProcessHost* mSubprocess;
    ContentParent* mOpener;

    uint64_t mChildID;
    int32_t mGeolocationWatchID;

    nsString mAppManifestURL;

    




    nsString mAppName;

    
    
    
    
    CancelableTask* mForceKillTask;
    
    
    
    int32_t mNumDestroyingTabs;
    
    
    
    
    bool mIsAlive;

    bool mSendPermissionUpdates;
    bool mSendDataStoreInfos;
    bool mIsForBrowser;
    bool mIsNuwaProcess;

    
    
    bool mCalledClose;
    bool mCalledCloseWithError;
    bool mCalledKillHard;

    friend class CrashReporterParent;

    nsRefPtr<nsConsoleService>  mConsoleService;
    nsConsoleService* GetConsoleService();

    nsDataHashtable<nsUint64HashKey, nsRefPtr<ParentIdleListener> > mIdleListeners;

#ifdef MOZ_X11
    
    
    ScopedClose mChildXSocketFdDup;
#endif

#ifdef MOZ_NUWA_PROCESS
    static bool sNuwaReady;
#endif
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
