







































#ifndef nsDocShell_h__
#define nsDocShell_h__

#include "nsIPresShell.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIContentViewer.h"
#include "nsIPrefBranch.h"
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
#include "nsIDeviceContext.h"

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


#include "nsIDocumentCharsetInfo.h"
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
#include "nsPIDOMEventTarget.h"
#include "nsIURIClassifier.h"
#include "nsIChannelClassifier.h"
#include "nsILoadContext.h"
#include "nsIWidget.h"
#include "nsIWebShellServices.h"
#include "nsILinkHandler.h"
#include "nsIClipboardCommands.h"
#include "nsICommandManager.h"
#include "nsCRT.h"

class nsIScrollableView;
class nsDocShell;
class nsIController;
class OnLinkClickEvent;





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

    PRInt32 GetDelay() { return mDelay ;}

    nsRefPtr<nsDocShell>  mDocShell;
    nsCOMPtr<nsIURI>      mURI;
    PRInt32               mDelay;
    PRPackedBool          mRepeat;
    PRPackedBool          mMetaRefresh;
    
protected:
    virtual ~nsRefreshTimer();
};

class nsClassifierCallback : public nsIChannelClassifier
                           , public nsIURIClassifierCallback
                           , public nsIRunnable
                           , public nsIChannelEventSink
                           , public nsIInterfaceRequestor
{
public:
    nsClassifierCallback() {}
    ~nsClassifierCallback() {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSICHANNELCLASSIFIER
    NS_DECL_NSIURICLASSIFIERCALLBACK
    NS_DECL_NSIRUNNABLE
    NS_DECL_NSICHANNELEVENTSINK
    NS_DECL_NSIINTERFACEREQUESTOR

private:
    nsCOMPtr<nsIChannel> mChannel;
    nsCOMPtr<nsIChannel> mSuspendedChannel;
    nsCOMPtr<nsIInterfaceRequestor> mNotificationCallbacks;

    void MarkEntryClassified(nsresult status);
    PRBool HasBeenClassified(nsIChannel *aChannel);
};

#define NS_ERROR_DOCSHELL_REQUEST_REJECTED  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GENERAL,1001)

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
    NS_DECL_NSILOADCONTEXT
    NS_DECL_NSICLIPBOARDCOMMANDS
    NS_DECL_NSIWEBSHELLSERVICES

    NS_IMETHOD Stop() {
        
        
        return nsDocLoader::Stop();
    }

    
    
    NS_FORWARD_NSISECURITYEVENTSINK(nsDocLoader::)

    
    NS_IMETHOD OnLinkClick(nsIContent* aContent,
        nsIURI* aURI,
        const PRUnichar* aTargetSpec,
        nsIInputStream* aPostDataStream = 0,
        nsIInputStream* aHeadersDataStream = 0);
    NS_IMETHOD OnLinkClickSync(nsIContent* aContent,
        nsIURI* aURI,
        const PRUnichar* aTargetSpec,
        nsIInputStream* aPostDataStream = 0,
        nsIInputStream* aHeadersDataStream = 0,
        nsIDocShell** aDocShell = 0,
        nsIRequest** aRequest = 0);
    NS_IMETHOD OnOverLink(nsIContent* aContent,
        nsIURI* aURI,
        const PRUnichar* aTargetSpec);
    NS_IMETHOD OnLeaveLink();
    NS_IMETHOD GetLinkState(nsIURI* aLinkURI, nsLinkState& aState);

    nsDocShellInfoLoadType ConvertLoadTypeToDocShellLoadInfo(PRUint32 aLoadType);
    PRUint32 ConvertDocShellLoadInfoToLoadType(nsDocShellInfoLoadType aDocShellLoadType);

    
    virtual nsIScriptGlobalObject* GetScriptGlobalObject();

    
    
    
    nsresult RestoreFromHistory();

    
    
    
    
    nsresult ForceRefreshURIFromTimer(nsIURI * aURI, PRInt32 aDelay,
                                      PRBool aMetaRefresh, nsITimer* aTimer);

    friend class OnLinkClickEvent;

protected:
    
    virtual ~nsDocShell();
    virtual void DestroyChildren();

    
    NS_IMETHOD EnsureContentViewer();
    
    
    nsresult CreateAboutBlankContentViewer(nsIPrincipal* aPrincipal,
                                           nsIURI* aBaseURI);
    NS_IMETHOD CreateContentViewer(const char * aContentType, 
        nsIRequest * request, nsIStreamListener ** aContentHandler);
    NS_IMETHOD NewContentViewerObj(const char * aContentType, 
        nsIRequest * request, nsILoadGroup * aLoadGroup, 
        nsIStreamListener ** aContentHandler, nsIContentViewer ** aViewer);
    NS_IMETHOD SetupNewViewer(nsIContentViewer * aNewViewer);

    void SetupReferrerFromChannel(nsIChannel * aChannel);
    
    NS_IMETHOD GetEldestPresContext(nsPresContext** aPresContext);

    
    
    
    
    
    
    nsIPrincipal* GetInheritedPrincipal(PRBool aConsiderCurrentDocument);

    
    
    PRBool ShouldCheckAppCache(nsIURI * aURI);

    
    
    
    virtual nsresult DoURILoad(nsIURI * aURI,
                               nsIURI * aReferrer,
                               PRBool aSendReferrer,
                               nsISupports * aOwner,
                               const char * aTypeHint,
                               nsIInputStream * aPostData,
                               nsIInputStream * aHeadersData,
                               PRBool firstParty,
                               nsIDocShell ** aDocShell,
                               nsIRequest ** aRequest,
                               PRBool aIsNewWindowTarget,
                               PRBool aBypassClassifier,
                               PRBool aForceAllowCookies);
    NS_IMETHOD AddHeadersToChannel(nsIInputStream * aHeadersData, 
                                  nsIChannel * aChannel);
    virtual nsresult DoChannelLoad(nsIChannel * aChannel,
                                   nsIURILoader * aURILoader,
                                   PRBool aBypassClassifier);

    
    
    
    nsresult CheckClassifier(nsIChannel *aChannel);

    nsresult ScrollIfAnchor(nsIURI * aURI, PRBool * aWasAnchor,
                            PRUint32 aLoadType, nscoord *cx, nscoord *cy,
                            PRBool * aDoHashchange);

    
    
    nsresult DispatchAsyncHashchange();

    nsresult FireHashchange();

    
    
    
    
    
    PRBool OnLoadingSite(nsIChannel * aChannel,
                         PRBool aFireOnLocationChange,
                         PRBool aAddToGlobalHistory = PR_TRUE);

    
    
    
    
    
    
    
    PRBool OnNewURI(nsIURI * aURI, nsIChannel * aChannel, nsISupports* aOwner,
                    PRUint32 aLoadType,
                    PRBool aFireOnLocationChange,
                    PRBool aAddToGlobalHistory = PR_TRUE);

    virtual void SetReferrerURI(nsIURI * aURI);

    
    virtual PRBool ShouldAddToSessionHistory(nsIURI * aURI);
    
    
    virtual nsresult AddToSessionHistory(nsIURI * aURI, nsIChannel * aChannel,
                                         nsISupports* aOwner,
                                         nsISHEntry ** aNewEntry);
    nsresult DoAddChildSHEntry(nsISHEntry* aNewEntry, PRInt32 aChildOffset);

    NS_IMETHOD LoadHistoryEntry(nsISHEntry * aEntry, PRUint32 aLoadType);
    NS_IMETHOD PersistLayoutHistoryState();

    
    
    
    
    
    
    static nsresult CloneAndReplace(nsISHEntry *aSrcEntry,
                                    nsDocShell *aSrcShell,
                                    PRUint32 aCloneID,
                                    nsISHEntry *aReplaceEntry,
                                    nsISHEntry **aDestEntry);

    
    static nsresult CloneAndReplaceChild(nsISHEntry *aEntry,
                                         nsDocShell *aShell,
                                         PRInt32 aChildIndex, void *aData);

    nsresult GetRootSessionHistory(nsISHistory ** aReturn);
    nsresult GetHttpChannel(nsIChannel * aChannel, nsIHttpChannel ** aReturn);
    PRBool ShouldDiscardLayoutState(nsIHttpChannel * aChannel);

    
    
    PRBool HasHistoryEntry(nsISHEntry *aEntry) const
    {
        return aEntry && (aEntry == mOSHE || aEntry == mLSHE);
    }

    
    void SwapHistoryEntries(nsISHEntry *aOldEntry, nsISHEntry *aNewEntry);

    
    
    
    void SetHistoryEntry(nsCOMPtr<nsISHEntry> *aPtr, nsISHEntry *aEntry);

    
    static nsresult SetChildHistoryEntry(nsISHEntry *aEntry,
                                         nsDocShell *aShell,
                                         PRInt32 aEntryIndex, void *aData);

    
    
    
    
    typedef nsresult (*WalkHistoryEntriesFunc)(nsISHEntry *aEntry,
                                               nsDocShell *aShell,
                                               PRInt32 aChildIndex,
                                               void *aData);

    
    
    
    static nsresult WalkHistoryEntries(nsISHEntry *aRootEntry,
                                       nsDocShell *aRootShell,
                                       WalkHistoryEntriesFunc aCallback,
                                       void *aData);

    
    
    virtual void OnRedirectStateChange(nsIChannel* aOldChannel,
                                       nsIChannel* aNewChannel,
                                       PRUint32 aRedirectFlags,
                                       PRUint32 aStateFlags);

    
    nsresult AddToGlobalHistory(nsIURI * aURI, PRBool aRedirect,
                                nsIChannel * aChannel);

    
    nsresult   ConfirmRepost(PRBool * aRepost);
    NS_IMETHOD GetPromptAndStringBundle(nsIPrompt ** aPrompt,
        nsIStringBundle ** aStringBundle);
    NS_IMETHOD GetChildOffset(nsIDOMNode * aChild, nsIDOMNode * aParent,
        PRInt32 * aOffset);
    NS_IMETHOD GetRootScrollableView(nsIScrollableView ** aOutScrollView);
    NS_IMETHOD EnsureScriptEnvironment();
    NS_IMETHOD EnsureEditorData();
    nsresult   EnsureTransferableHookData();
    NS_IMETHOD EnsureFind();
    nsresult   RefreshURIFromQueue();
    NS_IMETHOD DisplayLoadError(nsresult aError, nsIURI *aURI,
                                const PRUnichar *aURL,
                                nsIChannel* aFailedChannel = nsnull);
    NS_IMETHOD LoadErrorPage(nsIURI *aURI, const PRUnichar *aURL,
                             const char *aErrorPage,
                             const PRUnichar *aErrorType,
                             const PRUnichar *aDescription,
                             const char *aCSSClass,
                             nsIChannel* aFailedChannel);
    PRBool IsNavigationAllowed(PRBool aDisplayPrintErrorDialog = PR_TRUE);
    PRBool IsPrintingOrPP(PRBool aDisplayErrorDialog = PR_TRUE);

    nsresult SetBaseUrlForWyciwyg(nsIContentViewer * aContentViewer);

    static  inline  PRUint32
    PRTimeToSeconds(PRTime t_usec)
    {
      PRTime usec_per_sec;
      PRUint32 t_sec;
      LL_I2L(usec_per_sec, PR_USEC_PER_SEC);
      LL_DIV(t_usec, t_usec, usec_per_sec);
      LL_L2I(t_sec, t_usec);
      return t_sec;
    }

    PRBool IsFrame();

    
    
    
    
    virtual nsresult EndPageLoad(nsIWebProgress * aProgress,
                                 nsIChannel * aChannel,
                                 nsresult aResult);

    nsresult CheckLoadingPermissions();

    
    
    static PRBool CanAccessItem(nsIDocShellTreeItem* aTargetItem,
                                nsIDocShellTreeItem* aAccessingItem,
                                PRBool aConsiderOpener = PR_TRUE);
    static PRBool ValidateOrigin(nsIDocShellTreeItem* aOriginTreeItem,
                                 nsIDocShellTreeItem* aTargetTreeItem);

    
    
    
    
    
    PRBool SetCurrentURI(nsIURI *aURI, nsIRequest *aRequest,
                         PRBool aFireOnLocationChange);

    
    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    
    
    
    
    
    
    PRBool CanSavePresentation(PRUint32 aLoadType,
                               nsIRequest *aNewRequest,
                               nsIDocument *aNewDocument);

    
    
    
    nsresult CaptureState();

    
    
    
    nsresult RestorePresentation(nsISHEntry *aSHEntry, PRBool *aRestoring);

    
    nsresult BeginRestoreChildren();

    
    void DoGetPositionAndSize(PRInt32 * x, PRInt32 * y, PRInt32 * cx,
                              PRInt32 * cy);
    
    
    static nsresult URIInheritsSecurityContext(nsIURI* aURI, PRBool* aResult);

    
    static PRBool URIIsLocalFile(nsIURI *aURI);

    
    static PRBool IsAboutBlank(nsIURI* aURI);

    
    
    
    PRBool IsOKToLoadURI(nsIURI* aURI);
    
    void ReattachEditorToWindow(nsISHEntry *aSHEntry);

    nsresult GetSessionStorageForURI(nsIURI* aURI,
                                     PRBool create,
                                     nsIDOMStorage** aStorage);

    
    nsresult GetControllerForCommand(const char *inCommand,
                                     nsIController** outController);
    nsresult IsCommandEnabled(const char * inCommand, PRBool* outEnabled);
    nsresult DoCommand(const char * inCommand);
    nsresult EnsureCommandHandler();
    
protected:
    
    virtual nsresult SetDocLoaderParent(nsDocLoader * aLoader);

    
    class RestorePresentationEvent : public nsRunnable {
    public:
        NS_DECL_NSIRUNNABLE
        RestorePresentationEvent(nsDocShell *ds) : mDocShell(ds) {}
        void Revoke() { mDocShell = nsnull; }
    private:
        nsRefPtr<nsDocShell> mDocShell;
    };

    
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
    nsCOMPtr<nsIDocumentCharsetInfo> mDocumentCharsetInfo;
    nsCOMPtr<nsIWidget>        mParentWidget;
    nsCOMPtr<nsIPrefBranch>    mPrefs;

    
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

    
    nsRefPtr<nsClassifierCallback> mClassifier;

    
    
    
    
    
    nsCOMPtr<nsIURI>           mLoadingURI;

    
    
    
    nsCOMPtr<nsIURI>           mFailedURI;
    nsCOMPtr<nsIChannel>       mFailedChannel;
    PRUint32                   mFailedLoadType;

    
    
    

    nsIDocShellTreeOwner *     mTreeOwner; 
    nsPIDOMEventTarget *       mChromeEventHandler; 

    eCharsetReloadState        mCharsetReloadState;

    
    
    
    
    
    
    
    
    
    
    PRUint32                   mChildOffset;
    PRUint32                   mBusyFlags;
    PRUint32                   mAppType;
    PRUint32                   mLoadType;

    PRInt32                    mMarginWidth;
    PRInt32                    mMarginHeight;
    PRInt32                    mItemType;

    
    
    PRInt32                    mPreviousTransIndex;
    PRInt32                    mLoadedTransIndex;

    PRPackedBool               mAllowSubframes;
    PRPackedBool               mAllowPlugins;
    PRPackedBool               mAllowJavascript;
    PRPackedBool               mAllowMetaRedirects;
    PRPackedBool               mAllowImages;
    PRPackedBool               mAllowDNSPrefetch;
    PRPackedBool               mCreatingDocument; 
    PRPackedBool               mUseErrorPages;
    PRPackedBool               mObserveErrorPages;
    PRPackedBool               mAllowAuth;
    PRPackedBool               mAllowKeywordFixup;
    PRPackedBool               mIsOffScreenBrowser;

    
    
    
    PRPackedBool               mFiredUnloadEvent;

    
    
    
    
    PRPackedBool               mEODForCurrentDocument;
    PRPackedBool               mURIResultedInDocument;

    PRPackedBool               mIsBeingDestroyed;

    PRPackedBool               mIsExecutingOnLoadHandler;

    
    PRPackedBool               mIsPrintingOrPP;

    
    
    
    PRPackedBool               mSavingOldViewer;
#ifdef DEBUG
    PRPackedBool               mInEnsureScriptEnv;
#endif

    static nsIURIFixup *sURIFixup;

#ifdef DEBUG
private:
    
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
