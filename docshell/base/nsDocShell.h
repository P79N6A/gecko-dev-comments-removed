






#ifndef nsDocShell_h__
#define nsDocShell_h__

#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIContentViewer.h"
#include "nsInterfaceHashtable.h"
#include "nsIScriptContext.h"
#include "nsITimer.h"

#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIBaseWindow.h"
#include "nsIScrollable.h"
#include "nsITextScroll.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIContentViewerContainer.h"

#include "nsDocLoader.h"
#include "nsIURILoader.h"
#include "nsIEditorDocShell.h"

#include "nsWeakReference.h"


#include "nsDSURIContentListener.h"
#include "nsDocShellEditorData.h"


#include "nsCOMPtr.h"
#include "nsPoint.h" 
#include "nsString.h"
#include "nsAutoPtr.h"
#include "nsThreadUtils.h"


#define REFRESH_REDIRECT_TIMER 15000


#include "nsIDocCharset.h"
#include "nsIGlobalHistory2.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIPrompt.h"
#include "nsIRefreshURI.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsISHistory.h"
#include "nsILayoutHistoryState.h"
#include "nsIStringBundle.h"
#include "nsISupportsArray.h"
#include "nsIWebNavigation.h"
#include "nsIWebPageDescriptor.h"
#include "nsIWebProgressListener.h"
#include "nsISHContainer.h"
#include "nsIDocShellLoadInfo.h"
#include "nsIDocShellHistory.h"
#include "nsIURIFixup.h"
#include "nsIWebBrowserFind.h"
#include "nsIHttpChannel.h"
#include "nsDocShellTransferableHooks.h"
#include "nsIAuthPromptProvider.h"
#include "nsISecureBrowserUI.h"
#include "nsIObserver.h"
#include "nsDocShellLoadTypes.h"
#include "nsIDOMEventTarget.h"
#include "nsILoadContext.h"
#include "nsIWidget.h"
#include "nsIWebShellServices.h"
#include "nsILinkHandler.h"
#include "nsIClipboardCommands.h"
#include "nsICommandManager.h"
#include "nsCRT.h"

class nsDocShell;
class nsIController;
class OnLinkClickEvent;
class nsIScrollableFrame;
class nsDOMNavigationTiming;





enum ViewMode {
    viewNormal = 0x0,
    viewSource = 0x1
};





class nsRefreshTimer : public nsITimerCallback
{
public:
    nsRefreshTimer();

    NS_DECL_ISUPPORTS
    NS_DECL_NSITIMERCALLBACK

    int32_t GetDelay() { return mDelay ;}

    nsRefPtr<nsDocShell>  mDocShell;
    nsCOMPtr<nsIURI>      mURI;
    int32_t               mDelay;
    bool                  mRepeat;
    bool                  mMetaRefresh;
    
protected:
    virtual ~nsRefreshTimer();
};

typedef enum {
    eCharsetReloadInit,
    eCharsetReloadRequested,
    eCharsetReloadStopOrigional
} eCharsetReloadState;





class nsDocShell : public nsDocLoader,
                   public nsIDocShell,
                   public nsIDocShellTreeItem, 
                   public nsIDocShellHistory,
                   public nsIWebNavigation,
                   public nsIBaseWindow, 
                   public nsIScrollable, 
                   public nsITextScroll, 
                   public nsIDocCharset, 
                   public nsIContentViewerContainer,
                   public nsIScriptGlobalObjectOwner,
                   public nsIRefreshURI,
                   public nsIWebProgressListener,
                   public nsIEditorDocShell,
                   public nsIWebPageDescriptor,
                   public nsIAuthPromptProvider,
                   public nsIObserver,
                   public nsILoadContext,
                   public nsIWebShellServices,
                   public nsILinkHandler,
                   public nsIClipboardCommands
{
    friend class nsDSURIContentListener;

public:
    
    nsDocShell();

    NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

    virtual nsresult Init();

    NS_DECL_ISUPPORTS_INHERITED

    NS_DECL_NSIDOCSHELL
    NS_DECL_NSIDOCSHELLTREEITEM
    NS_DECL_NSIDOCSHELLTREENODE
    NS_DECL_NSIDOCSHELLHISTORY
    NS_DECL_NSIWEBNAVIGATION
    NS_DECL_NSIBASEWINDOW
    NS_DECL_NSISCROLLABLE
    NS_DECL_NSITEXTSCROLL
    NS_DECL_NSIDOCCHARSET
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSIWEBPROGRESSLISTENER
    NS_DECL_NSIREFRESHURI
    NS_DECL_NSICONTENTVIEWERCONTAINER
    NS_DECL_NSIEDITORDOCSHELL
    NS_DECL_NSIWEBPAGEDESCRIPTOR
    NS_DECL_NSIAUTHPROMPTPROVIDER
    NS_DECL_NSIOBSERVER
    NS_DECL_NSICLIPBOARDCOMMANDS
    NS_DECL_NSIWEBSHELLSERVICES

    NS_IMETHOD Stop() {
        
        
        return nsDocLoader::Stop();
    }

    
    
    NS_FORWARD_NSISECURITYEVENTSINK(nsDocLoader::)

    
    NS_IMETHOD OnLinkClick(nsIContent* aContent,
        nsIURI* aURI,
        const PRUnichar* aTargetSpec,
        const nsAString& aFileName,
        nsIInputStream* aPostDataStream,
        nsIInputStream* aHeadersDataStream,
        bool aIsTrusted);
    NS_IMETHOD OnLinkClickSync(nsIContent* aContent,
        nsIURI* aURI,
        const PRUnichar* aTargetSpec,
        const nsAString& aFileName,
        nsIInputStream* aPostDataStream = 0,
        nsIInputStream* aHeadersDataStream = 0,
        nsIDocShell** aDocShell = 0,
        nsIRequest** aRequest = 0);
    NS_IMETHOD OnOverLink(nsIContent* aContent,
        nsIURI* aURI,
        const PRUnichar* aTargetSpec);
    NS_IMETHOD OnLeaveLink();

    nsDocShellInfoLoadType ConvertLoadTypeToDocShellLoadInfo(uint32_t aLoadType);
    uint32_t ConvertDocShellLoadInfoToLoadType(nsDocShellInfoLoadType aDocShellLoadType);

    
    virtual nsIScriptGlobalObject* GetScriptGlobalObject();

    
    
    NS_IMETHOD GetAssociatedWindow(nsIDOMWindow**);
    NS_IMETHOD GetTopWindow(nsIDOMWindow**);
    NS_IMETHOD GetTopFrameElement(nsIDOMElement**);
    NS_IMETHOD IsAppOfType(uint32_t, bool*);
    NS_IMETHOD GetIsContent(bool*);
    NS_IMETHOD GetUsePrivateBrowsing(bool*);
    NS_IMETHOD SetUsePrivateBrowsing(bool);
    NS_IMETHOD SetPrivateBrowsing(bool);

    
    
    
    nsresult RestoreFromHistory();

    
    
    
    
    nsresult ForceRefreshURIFromTimer(nsIURI * aURI, int32_t aDelay,
                                      bool aMetaRefresh, nsITimer* aTimer);

    friend class OnLinkClickEvent;

    
    
    void FireDummyOnLocationChange()
    {
        FireOnLocationChange(this, nullptr, mCurrentURI,
                             LOCATION_CHANGE_SAME_DOCUMENT);
    }

    nsresult HistoryTransactionRemoved(int32_t aIndex);
protected:
    
    virtual ~nsDocShell();
    virtual void DestroyChildren();

    
    NS_IMETHOD EnsureContentViewer();
    
    
    nsresult CreateAboutBlankContentViewer(nsIPrincipal* aPrincipal,
                                           nsIURI* aBaseURI,
                                           bool aTryToSaveOldPresentation = true);
    NS_IMETHOD CreateContentViewer(const char * aContentType, 
        nsIRequest * request, nsIStreamListener ** aContentHandler);
    NS_IMETHOD NewContentViewerObj(const char * aContentType, 
        nsIRequest * request, nsILoadGroup * aLoadGroup, 
        nsIStreamListener ** aContentHandler, nsIContentViewer ** aViewer);
    NS_IMETHOD SetupNewViewer(nsIContentViewer * aNewViewer);

    void SetupReferrerFromChannel(nsIChannel * aChannel);
    
    NS_IMETHOD GetEldestPresContext(nsPresContext** aPresContext);

    
    
    
    
    
    
    nsIPrincipal* GetInheritedPrincipal(bool aConsiderCurrentDocument);

    
    
    
    virtual nsresult DoURILoad(nsIURI * aURI,
                               nsIURI * aReferrer,
                               bool aSendReferrer,
                               nsISupports * aOwner,
                               const char * aTypeHint,
                               const nsAString & aFileName,
                               nsIInputStream * aPostData,
                               nsIInputStream * aHeadersData,
                               bool firstParty,
                               nsIDocShell ** aDocShell,
                               nsIRequest ** aRequest,
                               bool aIsNewWindowTarget,
                               bool aBypassClassifier,
                               bool aForceAllowCookies);
    NS_IMETHOD AddHeadersToChannel(nsIInputStream * aHeadersData, 
                                  nsIChannel * aChannel);
    virtual nsresult DoChannelLoad(nsIChannel * aChannel,
                                   nsIURILoader * aURILoader,
                                   bool aBypassClassifier);

    nsresult ScrollToAnchor(nsACString & curHash, nsACString & newHash,
                            uint32_t aLoadType);

    
    
    nsresult SerializeJSValVariant(JSContext *aCx, nsIVariant *aData,
                                   nsAString &aResult);

    
    
    
    
    
    bool OnLoadingSite(nsIChannel * aChannel,
                         bool aFireOnLocationChange,
                         bool aAddToGlobalHistory = true);

    
    
    
    
    
    
    
    
    
    bool OnNewURI(nsIURI * aURI, nsIChannel * aChannel, nsISupports* aOwner,
                    uint32_t aLoadType,
                    bool aFireOnLocationChange,
                    bool aAddToGlobalHistory,
                    bool aCloneSHChildren);

    virtual void SetReferrerURI(nsIURI * aURI);

    
    virtual bool ShouldAddToSessionHistory(nsIURI * aURI);
    
    
    
    
    
    
    virtual nsresult AddToSessionHistory(nsIURI * aURI, nsIChannel * aChannel,
                                         nsISupports* aOwner,
                                         bool aCloneChildren,
                                         nsISHEntry ** aNewEntry);
    nsresult DoAddChildSHEntry(nsISHEntry* aNewEntry, int32_t aChildOffset,
                               bool aCloneChildren);

    NS_IMETHOD LoadHistoryEntry(nsISHEntry * aEntry, uint32_t aLoadType);
    NS_IMETHOD PersistLayoutHistoryState();

    
    
    
    
    
    
    
    
    static nsresult CloneAndReplace(nsISHEntry *aSrcEntry,
                                    nsDocShell *aSrcShell,
                                    uint32_t aCloneID,
                                    nsISHEntry *aReplaceEntry,
                                    bool aCloneChildren,
                                    nsISHEntry **aDestEntry);

    
    static nsresult CloneAndReplaceChild(nsISHEntry *aEntry,
                                         nsDocShell *aShell,
                                         int32_t aChildIndex, void *aData);

    nsresult GetRootSessionHistory(nsISHistory ** aReturn);
    nsresult GetHttpChannel(nsIChannel * aChannel, nsIHttpChannel ** aReturn);
    bool ShouldDiscardLayoutState(nsIHttpChannel * aChannel);

    
    
    bool HasHistoryEntry(nsISHEntry *aEntry) const
    {
        return aEntry && (aEntry == mOSHE || aEntry == mLSHE);
    }

    
    void SwapHistoryEntries(nsISHEntry *aOldEntry, nsISHEntry *aNewEntry);

    
    
    
    void SetHistoryEntry(nsCOMPtr<nsISHEntry> *aPtr, nsISHEntry *aEntry);

    
    static nsresult SetChildHistoryEntry(nsISHEntry *aEntry,
                                         nsDocShell *aShell,
                                         int32_t aEntryIndex, void *aData);

    
    
    
    
    typedef nsresult (*WalkHistoryEntriesFunc)(nsISHEntry *aEntry,
                                               nsDocShell *aShell,
                                               int32_t aChildIndex,
                                               void *aData);

    
    
    
    static nsresult WalkHistoryEntries(nsISHEntry *aRootEntry,
                                       nsDocShell *aRootShell,
                                       WalkHistoryEntriesFunc aCallback,
                                       void *aData);

    
    
    virtual void OnRedirectStateChange(nsIChannel* aOldChannel,
                                       nsIChannel* aNewChannel,
                                       uint32_t aRedirectFlags,
                                       uint32_t aStateFlags);

    







    bool ChannelIsPost(nsIChannel* aChannel);

    














    void ExtractLastVisit(nsIChannel* aChannel,
                          nsIURI** aURI,
                          uint32_t* aChannelRedirectFlags);

    









    void SaveLastVisit(nsIChannel* aChannel,
                       nsIURI* aURI,
                       uint32_t aChannelRedirectFlags);

    

























    void AddURIVisit(nsIURI* aURI,
                     nsIURI* aReferrerURI,
                     nsIURI* aPreviousURI,
                     uint32_t aChannelRedirectFlags,
                     uint32_t aResponseStatus=0);

    
    nsresult   ConfirmRepost(bool * aRepost);
    NS_IMETHOD GetPromptAndStringBundle(nsIPrompt ** aPrompt,
        nsIStringBundle ** aStringBundle);
    NS_IMETHOD GetChildOffset(nsIDOMNode * aChild, nsIDOMNode * aParent,
        int32_t * aOffset);
    nsIScrollableFrame* GetRootScrollFrame();
    NS_IMETHOD EnsureScriptEnvironment();
    NS_IMETHOD EnsureEditorData();
    nsresult   EnsureTransferableHookData();
    NS_IMETHOD EnsureFind();
    nsresult   RefreshURIFromQueue();
    NS_IMETHOD DisplayLoadError(nsresult aError, nsIURI *aURI,
                                const PRUnichar *aURL,
                                nsIChannel* aFailedChannel = nullptr);
    NS_IMETHOD LoadErrorPage(nsIURI *aURI, const PRUnichar *aURL,
                             const char *aErrorPage,
                             const PRUnichar *aErrorType,
                             const PRUnichar *aDescription,
                             const char *aCSSClass,
                             nsIChannel* aFailedChannel);
    bool IsNavigationAllowed(bool aDisplayPrintErrorDialog = true);
    bool IsPrintingOrPP(bool aDisplayErrorDialog = true);

    nsresult SetBaseUrlForWyciwyg(nsIContentViewer * aContentViewer);

    static  inline  uint32_t
    PRTimeToSeconds(PRTime t_usec)
    {
      PRTime usec_per_sec = PR_USEC_PER_SEC;
      return  uint32_t(t_usec /= usec_per_sec);
    }

    bool IsFrame();

    
    
    
    
    virtual nsresult EndPageLoad(nsIWebProgress * aProgress,
                                 nsIChannel * aChannel,
                                 nsresult aResult);

    
    
    
    nsresult SetDocCurrentStateObj(nsISHEntry *shEntry);

    nsresult CheckLoadingPermissions();

    
    
    static bool CanAccessItem(nsIDocShellTreeItem* aTargetItem,
                                nsIDocShellTreeItem* aAccessingItem,
                                bool aConsiderOpener = true);
    static bool ValidateOrigin(nsIDocShellTreeItem* aOriginTreeItem,
                                 nsIDocShellTreeItem* aTargetTreeItem);

    
    
    
    
    
    bool SetCurrentURI(nsIURI *aURI, nsIRequest *aRequest,
                       bool aFireOnLocationChange,
                       uint32_t aLocationFlags);

    
    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    
    
    
    
    
    
    bool CanSavePresentation(uint32_t aLoadType,
                               nsIRequest *aNewRequest,
                               nsIDocument *aNewDocument);

    
    
    
    nsresult CaptureState();

    
    
    
    nsresult RestorePresentation(nsISHEntry *aSHEntry, bool *aRestoring);

    
    nsresult BeginRestoreChildren();

    
    void DoGetPositionAndSize(int32_t * x, int32_t * y, int32_t * cx,
                              int32_t * cy);
    
    
    
    
    bool IsOKToLoadURI(nsIURI* aURI);
    
    void ReattachEditorToWindow(nsISHEntry *aSHEntry);

    nsresult GetSessionStorageForURI(nsIURI* aURI,
                                     const nsSubstring& aDocumentURI,
                                     bool create,
                                     nsIDOMStorage** aStorage);

    
    nsresult GetControllerForCommand(const char *inCommand,
                                     nsIController** outController);
    nsresult IsCommandEnabled(const char * inCommand, bool* outEnabled);
    nsresult DoCommand(const char * inCommand);
    nsresult EnsureCommandHandler();

    nsIChannel* GetCurrentDocChannel();

    bool ShouldBlockLoadingForBackButton();
protected:
    
    virtual nsresult SetDocLoaderParent(nsDocLoader * aLoader);

    void ClearFrameHistory(nsISHEntry* aEntry);

    nsresult MaybeInitTiming();

    
    class RestorePresentationEvent : public nsRunnable {
    public:
        NS_DECL_NSIRUNNABLE
        RestorePresentationEvent(nsDocShell *ds) : mDocShell(ds) {}
        void Revoke() { mDocShell = nullptr; }
    private:
        nsRefPtr<nsDocShell> mDocShell;
    };

    bool JustStartedNetworkLoad();

    enum FrameType {
        eFrameTypeRegular,
        eFrameTypeBrowser,
        eFrameTypeApp
    };

    FrameType GetInheritedFrameType();

    bool HasUnloadedParent();

    
    nsInterfaceHashtable<nsCStringHashKey, nsIDOMStorage> mStorages;

    
    nsIntRect                  mBounds;
    nsString                   mName;
    nsString                   mTitle;

    



    nsCString                  mContentTypeHint;
    nsIntPoint                 mDefaultScrollbarPref; 

    nsCOMPtr<nsISupportsArray> mRefreshURIList;
    nsCOMPtr<nsISupportsArray> mSavedRefreshURIList;
    nsRefPtr<nsDSURIContentListener> mContentListener;
    nsCOMPtr<nsIContentViewer> mContentViewer;
    nsCOMPtr<nsIWidget>        mParentWidget;

    
    nsCOMPtr<nsIURI>           mCurrentURI;
    nsCOMPtr<nsIURI>           mReferrerURI;
    nsCOMPtr<nsIScriptGlobalObject> mScriptGlobal;
    nsCOMPtr<nsISHistory>      mSessionHistory;
    nsCOMPtr<nsIGlobalHistory2> mGlobalHistory;
    nsCOMPtr<nsIWebBrowserFind> mFind;
    nsCOMPtr<nsICommandManager> mCommandManager;
    
    
    nsCOMPtr<nsISHEntry>       mOSHE;
    
    
    
    
    
    
    nsCOMPtr<nsISHEntry>       mLSHE;

    
    
    
    nsRevocableEventPtr<RestorePresentationEvent> mRestorePresentationEvent;

    
    nsAutoPtr<nsDocShellEditorData> mEditorData;

    
    nsCOMPtr<nsIClipboardDragDropHookList> mTransferableHookData;

    
    nsCOMPtr<nsISecureBrowserUI> mSecurityUI;

    
    
    
    
    
    nsCOMPtr<nsIURI>           mLoadingURI;

    
    
    
    nsCOMPtr<nsIURI>           mFailedURI;
    nsCOMPtr<nsIChannel>       mFailedChannel;
    uint32_t                   mFailedLoadType;

    
    
    

    nsIDocShellTreeOwner *     mTreeOwner; 
    nsIDOMEventTarget *       mChromeEventHandler; 

    eCharsetReloadState        mCharsetReloadState;

    
    
    uint32_t                   mChildOffset;
    uint32_t                   mBusyFlags;
    uint32_t                   mAppType;
    uint32_t                   mLoadType;

    int32_t                    mMarginWidth;
    int32_t                    mMarginHeight;

    
    
    int32_t                    mItemType;

    
    
    int32_t                    mPreviousTransIndex;
    int32_t                    mLoadedTransIndex;

    uint32_t                   mSandboxFlags;

    
    
    
    
    
    
    
    
    
    
    
    
    enum FullscreenAllowedState {
        CHECK_ATTRIBUTES,
        PARENT_ALLOWS,
        PARENT_PROHIBITS
    };
    FullscreenAllowedState     mFullscreenAllowed;

    bool                       mCreated;
    bool                       mAllowSubframes;
    bool                       mAllowPlugins;
    bool                       mAllowJavascript;
    bool                       mAllowMetaRedirects;
    bool                       mAllowImages;
    bool                       mAllowDNSPrefetch;
    bool                       mAllowWindowControl;
    bool                       mCreatingDocument; 
    bool                       mUseErrorPages;
    bool                       mObserveErrorPages;
    bool                       mAllowAuth;
    bool                       mAllowKeywordFixup;
    bool                       mIsOffScreenBrowser;
    bool                       mIsActive;
    bool                       mIsAppTab;
    bool                       mUseGlobalHistory;
    bool                       mInPrivateBrowsing;

    
    
    
    bool                       mFiredUnloadEvent;

    
    
    
    
    bool                       mEODForCurrentDocument;
    bool                       mURIResultedInDocument;

    bool                       mIsBeingDestroyed;

    bool                       mIsExecutingOnLoadHandler;

    
    bool                       mIsPrintingOrPP;

    
    
    
    bool                       mSavingOldViewer;
    
    
    bool                       mDynamicallyCreated;
#ifdef DEBUG
    bool                       mInEnsureScriptEnv;
#endif
    uint64_t                   mHistoryID;

    static nsIURIFixup *sURIFixup;

    nsRefPtr<nsDOMNavigationTiming> mTiming;

    
    FrameType mFrameType;

    
    
    
    
    
    
    
    
    uint32_t mOwnOrContainingAppId;

private:
    nsCOMPtr<nsIAtom> mForcedCharset;
    nsCOMPtr<nsIAtom> mParentCharset;
    nsTObserverArray<nsWeakPtr> mPrivacyObservers;
    int32_t           mParentCharsetSource;
    nsCString         mOriginalUriString;

#ifdef DEBUG
    
    static unsigned long gNumberOfDocShells;
#endif 

public:
    class InterfaceRequestorProxy : public nsIInterfaceRequestor {
    public:
        InterfaceRequestorProxy(nsIInterfaceRequestor* p);
        virtual ~InterfaceRequestorProxy();
        NS_DECL_ISUPPORTS
        NS_DECL_NSIINTERFACEREQUESTOR
 
    protected:
        InterfaceRequestorProxy() {}
        nsWeakPtr mWeakPtr;
    };
};

#endif 
