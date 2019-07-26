





#ifndef mozilla_dom_ContentParent_h
#define mozilla_dom_ContentParent_h

#include "mozilla/dom/PContentParent.h"
#include "mozilla/ipc/GeckoChildProcessHost.h"
#include "mozilla/dom/ipc/Blob.h"
#include "mozilla/Attributes.h"
#include "mozilla/HalTypes.h"
#include "mozilla/LinkedList.h"
#include "mozilla/StaticPtr.h"

#include "nsFrameMessageManager.h"
#include "nsIObserver.h"
#include "nsIThreadInternal.h"
#include "nsIDOMGeoPositionCallback.h"
#include "PermissionMessageUtils.h"

#define CHILD_PROCESS_SHUTDOWN_MESSAGE NS_LITERAL_STRING("child-process-shutdown")

class mozIApplication;
class nsConsoleService;
class nsIDOMBlob;
class nsIMemoryReporter;
template<class KeyClass,class DataType> class nsDataHashtable;

namespace mozilla {

namespace ipc {
class OptionalURIParams;
class URIParams;
class TestShellParent;
} 

namespace jsipc {
class JavaScriptParent;
class PJavaScriptParent;
}

namespace layers {
class PCompositorParent;
} 

namespace dom {

class Element;
class TabParent;
class PStorageParent;
class ClonedMessageData;
class MemoryReport;
class TabContext;

class ContentParent : public PContentParent
                    , public nsIObserver
                    , public nsIThreadObserver
                    , public nsIDOMGeoPositionCallback
                    , public mozilla::dom::ipc::MessageManagerCallback
                    , public mozilla::LinkedListElement<ContentParent>
{
    typedef mozilla::ipc::GeckoChildProcessHost GeckoChildProcessHost;
    typedef mozilla::ipc::OptionalURIParams OptionalURIParams;
    typedef mozilla::ipc::TestShellParent TestShellParent;
    typedef mozilla::ipc::URIParams URIParams;
    typedef mozilla::dom::ClonedMessageData ClonedMessageData;

public:
    



    static void StartUp();
    
    static void ShutDown();
    





    static void JoinAllSubprocesses();

    static already_AddRefed<ContentParent>
    GetNewOrUsed(bool aForBrowserElement = false);

    


    static already_AddRefed<ContentParent> PreallocateAppProcess();

    




    static TabParent*
    CreateBrowserOrApp(const TabContext& aContext,
                       Element* aFrameElement);

    static void GetAll(nsTArray<ContentParent*>& aArray);
    static void GetAllEvenIfDead(nsTArray<ContentParent*>& aArray);

    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIOBSERVER
    NS_DECL_NSITHREADOBSERVER
    NS_DECL_NSIDOMGEOPOSITIONCALLBACK

    


    virtual bool DoSendAsyncMessage(JSContext* aCx,
                                    const nsAString& aMessage,
                                    const mozilla::dom::StructuredCloneData& aData,
                                    JS::Handle<JSObject *> aCpows) MOZ_OVERRIDE;
    virtual bool CheckPermission(const nsAString& aPermission) MOZ_OVERRIDE;
    virtual bool CheckManifestURL(const nsAString& aManifestURL) MOZ_OVERRIDE;
    virtual bool CheckAppHasPermission(const nsAString& aPermission) MOZ_OVERRIDE;
    virtual bool CheckAppHasStatus(unsigned short aStatus) MOZ_OVERRIDE;

    
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
    bool IsForApp();

    void SetChildMemoryReports(const InfallibleTArray<MemoryReport>&
                               childReports);
    void UnregisterChildMemoryReporter();

    GeckoChildProcessHost* Process() {
        return mSubprocess;
    }

    int32_t Pid();

    bool NeedsPermissionsUpdate() {
        return mSendPermissionUpdates;
    }

    BlobParent* GetOrCreateActorForBlob(nsIDOMBlob* aBlob);

    




    void KillHard();

    uint64_t ChildID() { return mChildID; }
    bool IsPreallocated();

    





    void FriendlyName(nsAString& aName);

    virtual PIndexedDBParent* AllocPIndexedDBParent() MOZ_OVERRIDE;
    virtual bool
    RecvPIndexedDBConstructor(PIndexedDBParent* aActor) MOZ_OVERRIDE;

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

protected:
    void OnChannelConnected(int32_t pid) MOZ_OVERRIDE;
    virtual void ActorDestroy(ActorDestroyReason why);

    bool ShouldContinueFromReplyTimeout() MOZ_OVERRIDE;

private:
    static nsDataHashtable<nsStringHashKey, ContentParent*> *sAppContentParents;
    static nsTArray<ContentParent*>* sNonAppContentParents;
    static nsTArray<ContentParent*>* sPrivateContent;
    static StaticAutoPtr<LinkedList<ContentParent> > sContentParents;

    static void JoinProcessesIOThread(const nsTArray<ContentParent*>* aProcesses,
                                      Monitor* aMonitor, bool* aDone);

    
    
    
    static already_AddRefed<ContentParent>
    MaybeTakePreallocatedAppProcess(const nsAString& aAppManifestURL,
                                    ChildPrivileges aPrivs,
                                    hal::ProcessPriority aInitialPriority);

    static hal::ProcessPriority GetInitialProcessPriority(Element* aFrameElement);

    
    
    using PContentParent::SendPBrowserConstructor;
    using PContentParent::SendPTestShellConstructor;

    
    
    ContentParent(mozIApplication* aApp,
                  bool aIsForBrowser,
                  bool aIsForPreallocated,
                  ChildPrivileges aOSPrivileges = base::PRIVILEGES_DEFAULT,
                  hal::ProcessPriority aInitialPriority = hal::PROCESS_PRIORITY_FOREGROUND);

    virtual ~ContentParent();

    void Init();

    
    
    
    
    void MaybeTakeCPUWakeLock(Element* aFrameElement);

    
    
    
    
    bool SetPriorityAndCheckIsAlive(hal::ProcessPriority aPriority);

    
    
    
    bool TransformPreallocatedIntoApp(const nsAString& aAppManifestURL,
                                      ChildPrivileges aPrivs);

    



    void MarkAsDead();

    










    void ShutDownProcess(bool aCloseWithError);

    bool
    AllocPCompositorParent(mozilla::ipc::Transport* aTransport,
                           base::ProcessId aOtherProcess) MOZ_OVERRIDE;
    bool
    AllocPImageBridgeParent(mozilla::ipc::Transport* aTransport,
                            base::ProcessId aOtherProcess) MOZ_OVERRIDE;

    virtual bool RecvGetProcessAttributes(uint64_t* aId,
                                          bool* aIsForApp,
                                          bool* aIsForBrowser) MOZ_OVERRIDE;
    virtual bool RecvGetXPCOMProcessAttributes(bool* aIsOffline) MOZ_OVERRIDE;

    virtual bool DeallocPJavaScriptParent(mozilla::jsipc::PJavaScriptParent*);

    virtual PBrowserParent* AllocPBrowserParent(const IPCTabContext& aContext,
                                                const uint32_t& aChromeFlags);
    virtual bool DeallocPBrowserParent(PBrowserParent* frame);

    virtual PDeviceStorageRequestParent* AllocPDeviceStorageRequestParent(const DeviceStorageParams&);
    virtual bool DeallocPDeviceStorageRequestParent(PDeviceStorageRequestParent*);

    virtual PBlobParent* AllocPBlobParent(const BlobConstructorParams& aParams);
    virtual bool DeallocPBlobParent(PBlobParent*);

    virtual bool DeallocPCrashReporterParent(PCrashReporterParent* crashreporter);

    virtual bool RecvGetRandomValues(const uint32_t& length,
                                     InfallibleTArray<uint8_t>* randomValues);

    virtual bool DeallocPHalParent(PHalParent*) MOZ_OVERRIDE;

    virtual bool DeallocPIndexedDBParent(PIndexedDBParent* aActor);

    virtual PMemoryReportRequestParent* AllocPMemoryReportRequestParent();
    virtual bool DeallocPMemoryReportRequestParent(PMemoryReportRequestParent* actor);

    virtual PTestShellParent* AllocPTestShellParent();
    virtual bool DeallocPTestShellParent(PTestShellParent* shell);

    virtual bool DeallocPNeckoParent(PNeckoParent* necko);

    virtual PExternalHelperAppParent* AllocPExternalHelperAppParent(
            const OptionalURIParams& aUri,
            const nsCString& aMimeContentType,
            const nsCString& aContentDisposition,
            const bool& aForceSave,
            const int64_t& aContentLength,
            const OptionalURIParams& aReferrer,
            PBrowserParent* aBrowser);
    virtual bool DeallocPExternalHelperAppParent(PExternalHelperAppParent* aService);

    virtual PSmsParent* AllocPSmsParent();
    virtual bool DeallocPSmsParent(PSmsParent*);

    virtual PTelephonyParent* AllocPTelephonyParent();
    virtual bool DeallocPTelephonyParent(PTelephonyParent*);

    virtual bool DeallocPStorageParent(PStorageParent* aActor);

    virtual PBluetoothParent* AllocPBluetoothParent();
    virtual bool DeallocPBluetoothParent(PBluetoothParent* aActor);
    virtual bool RecvPBluetoothConstructor(PBluetoothParent* aActor);

    virtual PFMRadioParent* AllocPFMRadioParent();
    virtual bool DeallocPFMRadioParent(PFMRadioParent* aActor);

    virtual PSpeechSynthesisParent* AllocPSpeechSynthesisParent();
    virtual bool DeallocPSpeechSynthesisParent(PSpeechSynthesisParent* aActor);
    virtual bool RecvPSpeechSynthesisConstructor(PSpeechSynthesisParent* aActor);

    virtual bool RecvReadPrefsArray(InfallibleTArray<PrefSetting>* aPrefs);
    virtual bool RecvReadFontList(InfallibleTArray<FontListEntry>* retValue);

    virtual bool RecvReadPermissions(InfallibleTArray<IPC::Permission>* aPermissions);

    virtual bool RecvSetClipboardText(const nsString& text, const bool& isPrivateData, const int32_t& whichClipboard);
    virtual bool RecvGetClipboardText(const int32_t& whichClipboard, nsString* text);
    virtual bool RecvEmptyClipboard();
    virtual bool RecvClipboardHasText(bool* hasText);

    virtual bool RecvGetSystemColors(const uint32_t& colorsCount, InfallibleTArray<uint32_t>* colors);
    virtual bool RecvGetIconForExtension(const nsCString& aFileExt, const uint32_t& aIconSize, InfallibleTArray<uint8_t>* bits);
    virtual bool RecvGetShowPasswordSetting(bool* showPassword);

    virtual bool RecvStartVisitedQuery(const URIParams& uri);

    virtual bool RecvVisitURI(const URIParams& uri,
                              const OptionalURIParams& referrer,
                              const uint32_t& flags);

    virtual bool RecvSetURITitle(const URIParams& uri,
                                 const nsString& title);

    virtual bool RecvShowFilePicker(const int16_t& mode,
                                    const int16_t& selectedType,
                                    const bool& addToRecentDocs,
                                    const nsString& title,
                                    const nsString& defaultFile,
                                    const nsString& defaultExtension,
                                    const InfallibleTArray<nsString>& filters,
                                    const InfallibleTArray<nsString>& filterNames,
                                    InfallibleTArray<nsString>* files,
                                    int16_t* retValue,
                                    nsresult* result);

    virtual bool RecvShowAlertNotification(const nsString& aImageUrl, const nsString& aTitle,
                                           const nsString& aText, const bool& aTextClickable,
                                           const nsString& aCookie, const nsString& aName,
                                           const nsString& aBidi, const nsString& aLang);

    virtual bool RecvCloseAlert(const nsString& aName);

    virtual bool RecvLoadURIExternal(const URIParams& uri);

    virtual bool RecvSyncMessage(const nsString& aMsg,
                                 const ClonedMessageData& aData,
                                 const InfallibleTArray<CpowEntry>& aCpows,
                                 InfallibleTArray<nsString>* aRetvals);
    virtual bool RecvAsyncMessage(const nsString& aMsg,
                                  const ClonedMessageData& aData,
                                  const InfallibleTArray<CpowEntry>& aCpows);

    virtual bool RecvFilePathUpdateNotify(const nsString& aType,
                                          const nsString& aStorageName,
                                          const nsString& aFilePath,
                                          const nsCString& aReason);

    virtual bool RecvAddGeolocationListener(const IPC::Principal& aPrincipal,
                                            const bool& aHighAccuracy);
    virtual bool RecvRemoveGeolocationListener();
    virtual bool RecvSetGeolocationHigherAccuracy(const bool& aEnable);

    virtual bool RecvConsoleMessage(const nsString& aMessage);
    virtual bool RecvScriptError(const nsString& aMessage,
                                 const nsString& aSourceName,
                                 const nsString& aSourceLine,
                                 const uint32_t& aLineNumber,
                                 const uint32_t& aColNumber,
                                 const uint32_t& aFlags,
                                 const nsCString& aCategory);

    virtual bool RecvPrivateDocShellsExist(const bool& aExist);

    virtual bool RecvFirstIdle();

    virtual bool RecvAudioChannelGetState(const AudioChannelType& aType,
                                          const bool& aElementHidden,
                                          const bool& aElementWasHidden,
                                          AudioChannelState* aValue);

    virtual bool RecvAudioChannelRegisterType(const AudioChannelType& aType,
                                              const bool& aWithVideo);
    virtual bool RecvAudioChannelUnregisterType(const AudioChannelType& aType,
                                                const bool& aElementHidden,
                                                const bool& aWithVideo);

    virtual bool RecvAudioChannelChangedNotification();

    virtual bool RecvAudioChannelChangeDefVolChannel(
      const AudioChannelType& aType, const bool& aHidden);

    virtual bool RecvBroadcastVolume(const nsString& aVolumeName);

    virtual bool RecvRecordingDeviceEvents(const nsString& aRecordingStatus);

    virtual bool RecvSystemMessageHandled() MOZ_OVERRIDE;

    virtual bool RecvCreateFakeVolume(const nsString& fsName, const nsString& mountPoint) MOZ_OVERRIDE;

    virtual bool RecvSetFakeVolumeState(const nsString& fsName, const int32_t& fsState) MOZ_OVERRIDE;

    virtual bool RecvKeywordToURI(const nsCString& aKeyword, OptionalInputStreamParams* aPostData,
                                  OptionalURIParams* aURI);

    virtual void ProcessingError(Result what) MOZ_OVERRIDE;

    
    
    

    GeckoChildProcessHost* mSubprocess;
    base::ChildPrivileges mOSPrivileges;

    uint64_t mChildID;
    int32_t mGeolocationWatchID;

    
    
    
    
    
    
    nsCOMPtr<nsIMemoryReporter> mChildReporter;

    nsString mAppManifestURL;

    




    nsString mAppName;

    nsRefPtr<nsFrameMessageManager> mMessageManager;

    
    
    
    
    CancelableTask* mForceKillTask;
    
    
    
    int32_t mNumDestroyingTabs;
    
    
    
    
    bool mIsAlive;

    bool mSendPermissionUpdates;
    bool mIsForBrowser;

    
    
    bool mCalledClose;
    bool mCalledCloseWithError;
    bool mCalledKillHard;

    friend class CrashReporterParent;

    nsRefPtr<nsConsoleService>  mConsoleService;
    nsConsoleService* GetConsoleService();
};

} 
} 

#endif
