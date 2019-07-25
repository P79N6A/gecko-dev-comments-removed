





































#ifndef mozilla_dom_ContentParent_h
#define mozilla_dom_ContentParent_h

#include "base/waitable_event_watcher.h"

#include "mozilla/dom/PContentParent.h"
#include "mozilla/ipc/GeckoChildProcessHost.h"

#include "nsIObserver.h"
#include "nsIThreadInternal.h"
#include "mozilla/Monitor.h"
#include "nsNetUtil.h"
#include "nsIPrefService.h"
#include "nsIPermissionManager.h"
#include "nsIDOMGeoPositionCallback.h"
#include "nsIAccelerometer.h"

namespace mozilla {

namespace ipc {
class TestShellParent;
}

namespace dom {

class TabParent;

class ContentParent : public PContentParent
                    , public nsIObserver
                    , public nsIThreadObserver
                    , public nsIDOMGeoPositionCallback
                    , public nsIAccelerationListener
{
private:
    typedef mozilla::ipc::GeckoChildProcessHost GeckoChildProcessHost;
    typedef mozilla::ipc::TestShellParent TestShellParent;

public:
    static ContentParent* GetSingleton(PRBool aForceNew = PR_TRUE);

#if 0
    
    static ContentParent* FreeSingleton();
#endif

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
    NS_DECL_NSITHREADOBSERVER
    NS_DECL_NSIDOMGEOPOSITIONCALLBACK
    NS_DECL_NSIACCELERATIONLISTENER

    TabParent* CreateTab(PRUint32 aChromeFlags);

    TestShellParent* CreateTestShell();
    bool DestroyTestShell(TestShellParent* aTestShell);

    void ReportChildAlreadyBlocked();
    bool RequestRunToCompletion();

    bool IsAlive();

protected:
    virtual void ActorDestroy(ActorDestroyReason why);

private:
    static ContentParent* gSingleton;

    
    
    using PContentParent::SendPBrowserConstructor;
    using PContentParent::SendPTestShellConstructor;

    ContentParent();
    virtual ~ContentParent();

    virtual PBrowserParent* AllocPBrowser(const PRUint32& aChromeFlags);
    virtual bool DeallocPBrowser(PBrowserParent* frame);

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

    virtual bool RecvReadPrefsArray(InfallibleTArray<PrefTuple> *retValue);

    void EnsurePrefService();

    virtual bool RecvReadPermissions(InfallibleTArray<IPC::Permission>* aPermissions);

    virtual bool RecvStartVisitedQuery(const IPC::URI& uri);

    virtual bool RecvVisitURI(const IPC::URI& uri,
                              const IPC::URI& referrer,
                              const PRUint32& flags);

    virtual bool RecvSetURITitle(const IPC::URI& uri,
                                 const nsString& title);
    
    virtual bool RecvShowFilePicker(const PRInt16& mode,
                                    const PRInt16& selectedType,
                                    const nsString& title,
                                    const nsString& defaultFile,
                                    const nsString& defaultExtension,
                                    const InfallibleTArray<nsString>& filters,
                                    const InfallibleTArray<nsString>& filterNames,
                                    InfallibleTArray<nsString>* files,
                                    PRInt16* retValue,
                                    nsresult* result);
 
    virtual bool RecvShowAlertNotification(const nsString& aImageUrl, const nsString& aTitle,
                                           const nsString& aText, const PRBool& aTextClickable,
                                           const nsString& aCookie, const nsString& aName);

    virtual bool RecvLoadURIExternal(const IPC::URI& uri);

    virtual bool RecvSyncMessage(const nsString& aMsg, const nsString& aJSON,
                                 InfallibleTArray<nsString>* aRetvals);
    virtual bool RecvAsyncMessage(const nsString& aMsg, const nsString& aJSON);

    virtual bool RecvAddGeolocationListener();
    virtual bool RecvRemoveGeolocationListener();
    virtual bool RecvAddAccelerometerListener();
    virtual bool RecvRemoveAccelerometerListener();

    virtual bool RecvConsoleMessage(const nsString& aMessage);
    virtual bool RecvScriptError(const nsString& aMessage,
                                 const nsString& aSourceName,
                                 const nsString& aSourceLine,
                                 const PRUint32& aLineNumber,
                                 const PRUint32& aColNumber,
                                 const PRUint32& aFlags,
                                 const nsCString& aCategory);

    mozilla::Monitor mMonitor;

    GeckoChildProcessHost* mSubprocess;

    PRInt32 mGeolocationWatchID;
    int mRunToCompletionDepth;
    bool mShouldCallUnblockChild;
    nsCOMPtr<nsIThreadObserver> mOldObserver;

    bool mIsAlive;
    nsCOMPtr<nsIPrefServiceInternal> mPrefService; 
};

} 
} 

#endif
