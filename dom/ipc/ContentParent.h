





#ifndef mozilla_dom_ContentParent_h
#define mozilla_dom_ContentParent_h

#include "base/waitable_event_watcher.h"

#include "mozilla/dom/PContentParent.h"
#include "mozilla/dom/PMemoryReportRequestParent.h"
#include "mozilla/ipc/GeckoChildProcessHost.h"
#include "mozilla/dom/ipc/Blob.h"

#include "nsIObserver.h"
#include "nsIThreadInternal.h"
#include "nsNetUtil.h"
#include "nsIPermissionManager.h"
#include "nsIDOMGeoPositionCallback.h"
#include "nsIMemoryReporter.h"
#include "nsCOMArray.h"
#include "nsDataHashtable.h"
#include "nsInterfaceHashtable.h"
#include "nsHashKeys.h"

class nsFrameMessageManager;
class nsIDOMBlob;

namespace mozilla {

namespace ipc {
class TestShellParent;
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
{
private:
    typedef mozilla::ipc::GeckoChildProcessHost GeckoChildProcessHost;
    typedef mozilla::ipc::TestShellParent TestShellParent;
    typedef mozilla::layers::PCompositorParent PCompositorParent;
    typedef mozilla::dom::ClonedMessageData ClonedMessageData;

public:
    static ContentParent* GetNewOrUsed();

    






    static ContentParent* GetForApp(const nsAString& aManifestURL);
    static void GetAll(nsTArray<ContentParent*>& aArray);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
    NS_DECL_NSITHREADOBSERVER
    NS_DECL_NSIDOMGEOPOSITIONCALLBACK

    






    TabParent* CreateTab(PRUint32 aChromeFlags, bool aIsBrowserElement, PRUint32 aAppId);
    
    void NotifyTabDestroyed(PBrowserParent* aTab);

    TestShellParent* CreateTestShell();
    bool DestroyTestShell(TestShellParent* aTestShell);
    TestShellParent* GetTestShellSingleton();

    void ReportChildAlreadyBlocked();
    bool RequestRunToCompletion();

    bool IsAlive();
    bool IsForApp();

    void SetChildMemoryReporters(const InfallibleTArray<MemoryReport>& report);

    GeckoChildProcessHost* Process() {
        return mSubprocess;
    }

    bool NeedsPermissionsUpdate() {
        return mSendPermissionUpdates;
    }

    BlobParent* GetOrCreateActorForBlob(nsIDOMBlob* aBlob);

protected:
    void OnChannelConnected(int32 pid);
    virtual void ActorDestroy(ActorDestroyReason why);

private:
    static nsDataHashtable<nsStringHashKey, ContentParent*> *gAppContentParents;
    static nsTArray<ContentParent*>* gNonAppContentParents;
    static nsTArray<ContentParent*>* gPrivateContent;

    
    
    using PContentParent::SendPBrowserConstructor;
    using PContentParent::SendPTestShellConstructor;

    ContentParent(const nsAString& aAppManifestURL);
    virtual ~ContentParent();

    void Init();

    



    void MarkAsDead();

    





    void ShutDown();

    PCompositorParent* AllocPCompositor(mozilla::ipc::Transport* aTransport,
                                        base::ProcessId aOtherProcess) MOZ_OVERRIDE;

    virtual PBrowserParent* AllocPBrowser(const PRUint32& aChromeFlags, const bool& aIsBrowserElement, const PRUint32& aAppId);
    virtual bool DeallocPBrowser(PBrowserParent* frame);

    virtual PDeviceStorageRequestParent* AllocPDeviceStorageRequest(const DeviceStorageParams&);
    virtual bool DeallocPDeviceStorageRequest(PDeviceStorageRequestParent*);

    virtual PBlobParent* AllocPBlob(const BlobConstructorParams& aParams);
    virtual bool DeallocPBlob(PBlobParent*);

    virtual PCrashReporterParent* AllocPCrashReporter(const NativeThreadId& tid,
                                                      const PRUint32& processType);
    virtual bool DeallocPCrashReporter(PCrashReporterParent* crashreporter);
    virtual bool RecvPCrashReporterConstructor(PCrashReporterParent* actor,
                                               const NativeThreadId& tid,
                                               const PRUint32& processType);

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

    virtual PAudioParent* AllocPAudio(const PRInt32&,
                                     const PRInt32&,
                                     const PRInt32&);
    virtual bool DeallocPAudio(PAudioParent*);

    virtual PNeckoParent* AllocPNecko();
    virtual bool DeallocPNecko(PNeckoParent* necko);

    virtual PExternalHelperAppParent* AllocPExternalHelperApp(
            const IPC::URI& uri,
            const nsCString& aMimeContentType,
            const nsCString& aContentDisposition,
            const bool& aForceSave,
            const PRInt64& aContentLength,
            const IPC::URI& aReferrer);
    virtual bool DeallocPExternalHelperApp(PExternalHelperAppParent* aService);

    virtual PSmsParent* AllocPSms();
    virtual bool DeallocPSms(PSmsParent*);

    virtual PStorageParent* AllocPStorage(const StorageConstructData& aData);
    virtual bool DeallocPStorage(PStorageParent* aActor);

    virtual bool RecvReadPrefsArray(InfallibleTArray<PrefTuple> *retValue);
    virtual bool RecvReadFontList(InfallibleTArray<FontListEntry>* retValue);

    virtual bool RecvReadPermissions(InfallibleTArray<IPC::Permission>* aPermissions);

    virtual bool RecvSetClipboardText(const nsString& text, const PRInt32& whichClipboard);
    virtual bool RecvGetClipboardText(const PRInt32& whichClipboard, nsString* text);
    virtual bool RecvEmptyClipboard();
    virtual bool RecvClipboardHasText(bool* hasText);

    virtual bool RecvGetSystemColors(const PRUint32& colorsCount, InfallibleTArray<PRUint32>* colors);
    virtual bool RecvGetIconForExtension(const nsCString& aFileExt, const PRUint32& aIconSize, InfallibleTArray<PRUint8>* bits);
    virtual bool RecvGetShowPasswordSetting(bool* showPassword);

    virtual bool RecvStartVisitedQuery(const IPC::URI& uri);

    virtual bool RecvVisitURI(const IPC::URI& uri,
                              const IPC::URI& referrer,
                              const PRUint32& flags);

    virtual bool RecvSetURITitle(const IPC::URI& uri,
                                 const nsString& title);
    
    virtual bool RecvShowFilePicker(const PRInt16& mode,
                                    const PRInt16& selectedType,
                                    const bool& addToRecentDocs,
                                    const nsString& title,
                                    const nsString& defaultFile,
                                    const nsString& defaultExtension,
                                    const InfallibleTArray<nsString>& filters,
                                    const InfallibleTArray<nsString>& filterNames,
                                    InfallibleTArray<nsString>* files,
                                    PRInt16* retValue,
                                    nsresult* result);
 
    virtual bool RecvShowAlertNotification(const nsString& aImageUrl, const nsString& aTitle,
                                           const nsString& aText, const bool& aTextClickable,
                                           const nsString& aCookie, const nsString& aName);

    virtual bool RecvLoadURIExternal(const IPC::URI& uri);

    virtual bool RecvSyncMessage(const nsString& aMsg,
                                 const ClonedMessageData& aData,
                                 InfallibleTArray<nsString>* aRetvals);
    virtual bool RecvAsyncMessage(const nsString& aMsg,
                                  const ClonedMessageData& aData);

    virtual bool RecvAddGeolocationListener();
    virtual bool RecvRemoveGeolocationListener();

    virtual bool RecvConsoleMessage(const nsString& aMessage);
    virtual bool RecvScriptError(const nsString& aMessage,
                                 const nsString& aSourceName,
                                 const nsString& aSourceLine,
                                 const PRUint32& aLineNumber,
                                 const PRUint32& aColNumber,
                                 const PRUint32& aFlags,
                                 const nsCString& aCategory);

    virtual bool RecvPrivateDocShellsExist(const bool& aExist);

    virtual bool RecvAddFileWatch(const nsString& root);
    virtual bool RecvRemoveFileWatch(const nsString& root);

    GeckoChildProcessHost* mSubprocess;

    PRInt32 mGeolocationWatchID;
    int mRunToCompletionDepth;
    bool mShouldCallUnblockChild;

    
    
    
    
    nsCOMArray<nsIMemoryReporter> mMemoryReporters;

    bool mIsAlive;
    bool mSendPermissionUpdates;

    const nsString mAppManifestURL;
    nsRefPtr<nsFrameMessageManager> mMessageManager;

    class WatchedFile : public nsIFileUpdateListener {
      public:
        WatchedFile(ContentParent* aParent, const nsString& aPath)
          : mParent(aParent)
          , mUsageCount(1)
        {
          NS_NewLocalFile(aPath, false, getter_AddRefs(mFile));
        }

        NS_DECL_ISUPPORTS
        NS_DECL_NSIFILEUPDATELISTENER

        void Watch() {
          mFile->Watch(this);
        }

        void Unwatch() {
          mFile->Watch(this);
        }

        nsRefPtr<ContentParent> mParent;
        PRInt32 mUsageCount;
        nsCOMPtr<nsIFile> mFile;
    };

    
    nsInterfaceHashtable<nsStringHashKey, WatchedFile> mFileWatchers;

    friend class CrashReporterParent;
};

} 
} 

#endif
