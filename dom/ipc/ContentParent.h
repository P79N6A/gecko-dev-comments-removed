





#ifndef mozilla_dom_ContentParent_h
#define mozilla_dom_ContentParent_h

#include "base/waitable_event_watcher.h"

#include "mozilla/dom/PContentParent.h"
#include "mozilla/dom/PMemoryReportRequestParent.h"
#include "mozilla/dom/TabContext.h"
#include "mozilla/ipc/GeckoChildProcessHost.h"
#include "mozilla/dom/ipc/Blob.h"
#include "mozilla/Attributes.h"
#include "mozilla/HalTypes.h"
#include "mozilla/LinkedList.h"

#include "nsFrameMessageManager.h"
#include "nsIObserver.h"
#include "nsIThreadInternal.h"
#include "nsNetUtil.h"
#include "nsIPermissionManager.h"
#include "nsIDOMGeoPositionCallback.h"
#include "nsIMemoryReporter.h"
#include "nsCOMArray.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "PermissionMessageUtils.h"

#define CHILD_PROCESS_SHUTDOWN_MESSAGE NS_LITERAL_STRING("child-process-shutdown")

class mozIApplication;
class nsConsoleService;
class nsIDOMBlob;

namespace mozilla {

namespace ipc {
class OptionalURIParams;
class URIParams;
class TestShellParent;
} 

namespace jsipc {
class JavaScriptParent;
}

namespace layers {
class PCompositorParent;
} 

namespace dom {

class TabParent;
class PStorageParent;
class ClonedMessageData;

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
                       nsIDOMElement* aFrameElement);

    static void GetAll(nsTArray<ContentParent*>& aArray);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
    NS_DECL_NSITHREADOBSERVER
    NS_DECL_NSIDOMGEOPOSITIONCALLBACK

    


    virtual bool DoSendAsyncMessage(const nsAString& aMessage,
                                    const mozilla::dom::StructuredCloneData& aData) MOZ_OVERRIDE;
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

    void SetChildMemoryReporters(const InfallibleTArray<MemoryReport>& report);

    GeckoChildProcessHost* Process() {
        return mSubprocess;
    }

    int32_t Pid() {
        return base::GetProcId(mSubprocess->GetChildProcessHandle());
    }

    bool NeedsPermissionsUpdate() {
        return mSendPermissionUpdates;
    }

    BlobParent* GetOrCreateActorForBlob(nsIDOMBlob* aBlob);

    




    void KillHard();

    uint64_t ChildID() { return mChildID; }
    bool IsPreallocated();

    





    void FriendlyName(nsAString& aName);

protected:
    void OnChannelConnected(int32_t pid) MOZ_OVERRIDE;
    virtual void ActorDestroy(ActorDestroyReason why);

private:
    static nsDataHashtable<nsStringHashKey, ContentParent*> *sAppContentParents;
    static nsTArray<ContentParent*>* sNonAppContentParents;
    static nsTArray<ContentParent*>* sPrivateContent;
    static LinkedList<ContentParent> sContentParents;

    static void JoinProcessesIOThread(const nsTArray<ContentParent*>* aProcesses,
                                      Monitor* aMonitor, bool* aDone);

    
    
    
    static already_AddRefed<ContentParent>
    MaybeTakePreallocatedAppProcess(const nsAString& aAppManifestURL,
                                    ChildPrivileges aPrivs,
                                    hal::ProcessPriority aInitialPriority);

    static hal::ProcessPriority GetInitialProcessPriority(nsIDOMElement* aFrameElement);

    
    
    using PContentParent::SendPBrowserConstructor;
    using PContentParent::SendPTestShellConstructor;
    using PContentParent::SendPJavaScriptConstructor;

    
    
    ContentParent(mozIApplication* aApp,
                  bool aIsForBrowser,
                  bool aIsForPreallocated,
                  ChildPrivileges aOSPrivileges = base::PRIVILEGES_DEFAULT,
                  hal::ProcessPriority aInitialPriority = hal::PROCESS_PRIORITY_FOREGROUND);

    virtual ~ContentParent();

    void Init();

    
    
    
    
    void MaybeTakeCPUWakeLock(nsIDOMElement* aFrameElement);

    
    
    
    
    bool SetPriorityAndCheckIsAlive(hal::ProcessPriority aPriority);

    
    
    
    bool TransformPreallocatedIntoApp(const nsAString& aAppManifestURL,
                                      ChildPrivileges aPrivs);

    



    void MarkAsDead();

    





    void ShutDownProcess();

    PCompositorParent*
    AllocPCompositor(mozilla::ipc::Transport* aTransport,
                     base::ProcessId aOtherProcess) MOZ_OVERRIDE;
    PImageBridgeParent*
    AllocPImageBridge(mozilla::ipc::Transport* aTransport,
                      base::ProcessId aOtherProcess) MOZ_OVERRIDE;

    virtual bool RecvGetProcessAttributes(uint64_t* aId,
                                          bool* aIsForApp,
                                          bool* aIsForBrowser) MOZ_OVERRIDE;
    virtual bool RecvGetXPCOMProcessAttributes(bool* aIsOffline) MOZ_OVERRIDE;

    virtual mozilla::jsipc::PJavaScriptParent* AllocPJavaScript();
    virtual bool DeallocPJavaScript(mozilla::jsipc::PJavaScriptParent*);

    virtual PBrowserParent* AllocPBrowser(const IPCTabContext& aContext,
                                          const uint32_t& aChromeFlags);
    virtual bool DeallocPBrowser(PBrowserParent* frame);

    virtual PDeviceStorageRequestParent* AllocPDeviceStorageRequest(const DeviceStorageParams&);
    virtual bool DeallocPDeviceStorageRequest(PDeviceStorageRequestParent*);

    virtual PBlobParent* AllocPBlob(const BlobConstructorParams& aParams);
    virtual bool DeallocPBlob(PBlobParent*);

    virtual PCrashReporterParent* AllocPCrashReporter(const NativeThreadId& tid,
                                                      const uint32_t& processType);
    virtual bool DeallocPCrashReporter(PCrashReporterParent* crashreporter);
    virtual bool RecvPCrashReporterConstructor(PCrashReporterParent* actor,
                                               const NativeThreadId& tid,
                                               const uint32_t& processType);

    virtual bool RecvGetRandomValues(const uint32_t& length,
                                     InfallibleTArray<uint8_t>* randomValues);

    virtual PHalParent* AllocPHal() MOZ_OVERRIDE;
    virtual bool DeallocPHal(PHalParent*) MOZ_OVERRIDE;

    virtual PIndexedDBParent* AllocPIndexedDB();

    virtual bool DeallocPIndexedDB(PIndexedDBParent* aActor);

    virtual bool
    RecvPIndexedDBConstructor(PIndexedDBParent* aActor);

    virtual PMemoryReportRequestParent* AllocPMemoryReportRequest();
    virtual bool DeallocPMemoryReportRequest(PMemoryReportRequestParent* actor);

    virtual PTestShellParent* AllocPTestShell();
    virtual bool DeallocPTestShell(PTestShellParent* shell);

    virtual PNeckoParent* AllocPNecko();
    virtual bool DeallocPNecko(PNeckoParent* necko);

    virtual PExternalHelperAppParent* AllocPExternalHelperApp(
            const OptionalURIParams& aUri,
            const nsCString& aMimeContentType,
            const nsCString& aContentDisposition,
            const bool& aForceSave,
            const int64_t& aContentLength,
            const OptionalURIParams& aReferrer);
    virtual bool DeallocPExternalHelperApp(PExternalHelperAppParent* aService);

    virtual PSmsParent* AllocPSms();
    virtual bool DeallocPSms(PSmsParent*);

    virtual PStorageParent* AllocPStorage();
    virtual bool DeallocPStorage(PStorageParent* aActor);

    virtual PBluetoothParent* AllocPBluetooth();
    virtual bool DeallocPBluetooth(PBluetoothParent* aActor);
    virtual bool RecvPBluetoothConstructor(PBluetoothParent* aActor);

    virtual PSpeechSynthesisParent* AllocPSpeechSynthesis();
    virtual bool DeallocPSpeechSynthesis(PSpeechSynthesisParent* aActor);
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

    virtual bool RecvTestPermissionFromPrincipal(const IPC::Principal& aPrincipal,
                                                 const nsCString& aType,
                                                 uint32_t* permission);

    virtual bool RecvLoadURIExternal(const URIParams& uri);

    virtual bool RecvSyncMessage(const nsString& aMsg,
                                 const ClonedMessageData& aData,
                                 InfallibleTArray<nsString>* aRetvals);
    virtual bool RecvAsyncMessage(const nsString& aMsg,
                                  const ClonedMessageData& aData);

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

    virtual bool RecvAudioChannelGetMuted(const AudioChannelType& aType,
                                          const bool& aElementHidden,
                                          const bool& aElementWasHidden,
                                          bool* aValue);

    virtual bool RecvAudioChannelRegisterType(const AudioChannelType& aType);
    virtual bool RecvAudioChannelUnregisterType(const AudioChannelType& aType,
                                                const bool& aElementHidden);

    virtual bool RecvAudioChannelChangedNotification();

    virtual bool RecvBroadcastVolume(const nsString& aVolumeName);

    virtual bool RecvRecordingDeviceEvents(const nsString& aRecordingStatus);

    virtual bool RecvSystemMessageHandled() MOZ_OVERRIDE;

    virtual void ProcessingError(Result what) MOZ_OVERRIDE;

    GeckoChildProcessHost* mSubprocess;
    base::ChildPrivileges mOSPrivileges;

    uint64_t mChildID;
    int32_t mGeolocationWatchID;

    
    
    
    
    nsCOMArray<nsIMemoryReporter> mMemoryReporters;

    nsString mAppManifestURL;

    




    nsString mAppName;

    nsRefPtr<nsFrameMessageManager> mMessageManager;

    
    
    
    
    CancelableTask* mForceKillTask;
    
    
    
    int32_t mNumDestroyingTabs;
    
    
    
    
    bool mIsAlive;
    
    
    
    bool mIsDestroyed;
    bool mSendPermissionUpdates;
    bool mIsForBrowser;

    friend class CrashReporterParent;

    nsRefPtr<nsConsoleService>  mConsoleService;
    nsConsoleService* GetConsoleService();
};

} 
} 

#endif
