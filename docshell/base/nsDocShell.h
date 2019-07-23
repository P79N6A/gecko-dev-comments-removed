







































#ifndef nsDocShell_h__
#define nsDocShell_h__

#include "nsIPresShell.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIContentViewer.h"
#include "nsIPrefBranch.h"
#include "nsVoidArray.h"
#include "nsInterfaceHashtable.h"
#include "nsIScriptContext.h"
#include "nsITimer.h"

#include "nsCDocShell.h"
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

class nsIScrollableView;





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

    nsCOMPtr<nsIDocShell> mDocShell;
    nsCOMPtr<nsIURI>      mURI;
    PRInt32               mDelay;
    PRPackedBool          mRepeat;
    PRPackedBool          mMetaRefresh;
    
protected:
    virtual ~nsRefreshTimer();
};





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
                   public nsIObserver
{
friend class nsDSURIContentListener;

public:
    
    nsDocShell();

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

    NS_IMETHOD Stop() {
        
        
        return nsDocLoader::Stop();
    }

    
    
    NS_FORWARD_NSISECURITYEVENTSINK(nsDocLoader::)

    nsDocShellInfoLoadType ConvertLoadTypeToDocShellLoadInfo(PRUint32 aLoadType);
    PRUint32 ConvertDocShellLoadInfoToLoadType(nsDocShellInfoLoadType aDocShellLoadType);

    
    virtual nsIScriptGlobalObject* GetScriptGlobalObject();

    
    
    
    nsresult RestoreFromHistory();

protected:
    
    virtual ~nsDocShell();
    virtual void DestroyChildren();

    
    NS_IMETHOD EnsureContentViewer();
    NS_IMETHOD EnsureDeviceContext();
    
    
    nsresult CreateAboutBlankContentViewer(nsIPrincipal* aPrincipal);
    NS_IMETHOD CreateContentViewer(const char * aContentType, 
        nsIRequest * request, nsIStreamListener ** aContentHandler);
    NS_IMETHOD NewContentViewerObj(const char * aContentType, 
        nsIRequest * request, nsILoadGroup * aLoadGroup, 
        nsIStreamListener ** aContentHandler, nsIContentViewer ** aViewer);
    NS_IMETHOD SetupNewViewer(nsIContentViewer * aNewViewer);

    void SetupReferrerFromChannel(nsIChannel * aChannel);
    
    NS_IMETHOD GetEldestPresContext(nsPresContext** aPresContext);

    
    
    
    
    
    
    nsIPrincipal* GetInheritedPrincipal(PRBool aConsiderCurrentDocument);

    
    
    
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
                               PRBool aIsNewWindowTarget);
    NS_IMETHOD AddHeadersToChannel(nsIInputStream * aHeadersData, 
                                  nsIChannel * aChannel);
    virtual nsresult DoChannelLoad(nsIChannel * aChannel,
                                   nsIURILoader * aURILoader);
    NS_IMETHOD ScrollIfAnchor(nsIURI * aURI, PRBool * aWasAnchor,
                              PRUint32 aLoadType, nscoord *cx, nscoord *cy);

    
    
    
    
    
    PRBool OnLoadingSite(nsIChannel * aChannel,
                         PRBool aFireOnLocationChange,
                         PRBool aAddToGlobalHistory = PR_TRUE);

    
    
    
    
    
    PRBool OnNewURI(nsIURI * aURI, nsIChannel * aChannel, PRUint32 aLoadType,
                    PRBool aFireOnLocationChange,
                    PRBool aAddToGlobalHistory = PR_TRUE);

    virtual void SetReferrerURI(nsIURI * aURI);

    
    virtual PRBool ShouldAddToSessionHistory(nsIURI * aURI);
    virtual nsresult AddToSessionHistory(nsIURI * aURI, nsIChannel * aChannel,
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
                             const PRUnichar *aPage,
                             const PRUnichar *aDescription,
                             nsIChannel* aFailedChannel);
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

    
    static PRBool IsAboutBlank(nsIURI* aURI);
    
protected:
    
    virtual nsresult SetDocLoaderParent(nsDocLoader * aLoader);

    
    class RestorePresentationEvent : public nsRunnable {
    public:
        NS_DECL_NSIRUNNABLE
        RestorePresentationEvent(nsDocShell *ds) : mDocShell(ds) {}
        void Revoke() { mDocShell = nsnull; }
    private:
        nsDocShell *mDocShell;
    };

    PRPackedBool               mAllowSubframes;
    PRPackedBool               mAllowPlugins;
    PRPackedBool               mAllowJavascript;
    PRPackedBool               mAllowMetaRedirects;
    PRPackedBool               mAllowImages;
    PRPackedBool               mFocusDocFirst;
    PRPackedBool               mHasFocus;
    PRPackedBool               mCreatingDocument; 
    PRPackedBool               mUseErrorPages;
    PRPackedBool               mObserveErrorPages;
    PRPackedBool               mAllowAuth;
    PRPackedBool               mAllowKeywordFixup;

    PRPackedBool               mFiredUnloadEvent;

    
    
    
    
    PRPackedBool               mEODForCurrentDocument; 
    PRPackedBool               mURIResultedInDocument;

    PRPackedBool               mIsBeingDestroyed;

    PRPackedBool               mIsExecutingOnLoadHandler;

    
    PRPackedBool               mIsPrintingOrPP;

    
    
    
    PRPackedBool               mSavingOldViewer;

    PRUint32                   mAppType;

    
    
    
    
    
    
    
    
    
    
    PRUint32                   mChildOffset;

    PRUint32                   mBusyFlags;

    PRInt32                    mMarginWidth;
    PRInt32                    mMarginHeight;
    PRInt32                    mItemType;

    PRUint32                   mLoadType;

    nsString                   mName;
    nsString                   mTitle;
    



    nsCString                  mContentTypeHint;
    nsCOMPtr<nsISupportsArray> mRefreshURIList;
    nsCOMPtr<nsISupportsArray> mSavedRefreshURIList;
    nsRefPtr<nsDSURIContentListener> mContentListener;
    nsRect                     mBounds; 
    nsCOMPtr<nsIContentViewer> mContentViewer;
    nsCOMPtr<nsIDocumentCharsetInfo> mDocumentCharsetInfo;
    nsCOMPtr<nsIDeviceContext> mDeviceContext;
    nsCOMPtr<nsIWidget>        mParentWidget;
    nsCOMPtr<nsIPrefBranch>    mPrefs;

    
    nsCOMPtr<nsIURI>           mCurrentURI;
    nsCOMPtr<nsIURI>           mReferrerURI;
    nsCOMPtr<nsIScriptGlobalObject> mScriptGlobal;
    nsCOMPtr<nsISHistory>      mSessionHistory;
    nsCOMPtr<nsIGlobalHistory2> mGlobalHistory;
    nsCOMPtr<nsIWebBrowserFind> mFind;
    nsPoint                    mDefaultScrollbarPref; 
    
    
    nsCOMPtr<nsISHEntry>       mOSHE; 
    
    
    nsCOMPtr<nsISHEntry>       mLSHE;

    
    
    
    nsRevocableEventPtr<RestorePresentationEvent> mRestorePresentationEvent;

    
    nsInterfaceHashtable<nsCStringHashKey, nsIDOMStorage> mStorages;

    
    
    PRInt32                    mPreviousTransIndex;
    PRInt32                    mLoadedTransIndex;

    
    nsDocShellEditorData*      mEditorData;          

    
    nsCOMPtr<nsIClipboardDragDropHookList>  mTransferableHookData;

    
    nsCOMPtr<nsISecureBrowserUI> mSecurityUI;

    
    
    

    nsIDocShellTreeOwner *     mTreeOwner; 
    nsPIDOMEventTarget *       mChromeEventHandler; 

    static nsIURIFixup *sURIFixup;


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
