





#ifndef nsDocShell_h__
#define nsDocShell_h__

#include "nsITimer.h"
#include "nsContentPolicyUtils.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIBaseWindow.h"
#include "nsINetworkInterceptController.h"
#include "nsIScrollable.h"
#include "nsITextScroll.h"
#include "nsIContentViewerContainer.h"
#include "nsIDOMStorageManager.h"
#include "nsDocLoader.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/WeakPtr.h"
#include "mozilla/TimeStamp.h"
#include "GeckoProfiler.h"
#include "mozilla/dom/ProfileTimelineMarkerBinding.h"
#include "jsapi.h"


#include "nsCOMPtr.h"
#include "nsPoint.h" 
#include "nsString.h"
#include "nsAutoPtr.h"
#include "nsThreadUtils.h"
#include "nsContentUtils.h"
#include "TimelineMarker.h"


#define REFRESH_REDIRECT_TIMER 15000


#include "nsIDocCharset.h"
#include "nsIInterfaceRequestor.h"
#include "nsIRefreshURI.h"
#include "nsIWebNavigation.h"
#include "nsIWebPageDescriptor.h"
#include "nsIWebProgressListener.h"
#include "nsIDocShellLoadInfo.h"
#include "nsIAuthPromptProvider.h"
#include "nsILoadContext.h"
#include "nsIWebShellServices.h"
#include "nsILinkHandler.h"
#include "nsIClipboardCommands.h"
#include "nsITabParent.h"
#include "nsCRT.h"
#include "prtime.h"
#include "nsRect.h"
#include "Units.h"

namespace mozilla {
namespace dom {
class EventTarget;
class URLSearchParams;
}
}

class nsDocShell;
class nsDOMNavigationTiming;
class nsGlobalWindow;
class nsIController;
class nsIScrollableFrame;
class OnLinkClickEvent;
class nsDSURIContentListener;
class nsDocShellEditorData;
class nsIClipboardDragDropHookList;
class nsICommandManager;
class nsIContentViewer;
class nsIDocument;
class nsIDOMNode;
class nsIDocShellTreeOwner;
class nsIGlobalHistory2;
class nsIHttpChannel;
class nsIPrompt;
class nsISHistory;
class nsISecureBrowserUI;
class nsIStringBundle;
class nsISupportsArray;
class nsIURIFixup;
class nsIURILoader;
class nsIWebBrowserFind;
class nsIWidget;





enum ViewMode
{
  viewNormal = 0x0,
  viewSource = 0x1
};





class nsRefreshTimer : public nsITimerCallback
{
public:
  nsRefreshTimer();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

  int32_t GetDelay() { return mDelay ;}

  nsRefPtr<nsDocShell> mDocShell;
  nsCOMPtr<nsIURI> mURI;
  int32_t mDelay;
  bool mRepeat;
  bool mMetaRefresh;

protected:
  virtual ~nsRefreshTimer();
};

enum eCharsetReloadState
{
  eCharsetReloadInit,
  eCharsetReloadRequested,
  eCharsetReloadStopOrigional
};





class nsDocShell final
  : public nsDocLoader
  , public nsIDocShell
  , public nsIWebNavigation
  , public nsIBaseWindow
  , public nsIScrollable
  , public nsITextScroll
  , public nsIDocCharset
  , public nsIContentViewerContainer
  , public nsIRefreshURI
  , public nsIWebProgressListener
  , public nsIWebPageDescriptor
  , public nsIAuthPromptProvider
  , public nsILoadContext
  , public nsIWebShellServices
  , public nsILinkHandler
  , public nsIClipboardCommands
  , public nsIDOMStorageManager
  , public nsINetworkInterceptController
  , public mozilla::SupportsWeakPtr<nsDocShell>
{
  friend class nsDSURIContentListener;

public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(nsDocShell)
  
  nsDocShell();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  virtual nsresult Init() override;

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_NSIDOCSHELL
  NS_DECL_NSIDOCSHELLTREEITEM
  NS_DECL_NSIWEBNAVIGATION
  NS_DECL_NSIBASEWINDOW
  NS_DECL_NSISCROLLABLE
  NS_DECL_NSITEXTSCROLL
  NS_DECL_NSIDOCCHARSET
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSIWEBPROGRESSLISTENER
  NS_DECL_NSIREFRESHURI
  NS_DECL_NSICONTENTVIEWERCONTAINER
  NS_DECL_NSIWEBPAGEDESCRIPTOR
  NS_DECL_NSIAUTHPROMPTPROVIDER
  NS_DECL_NSICLIPBOARDCOMMANDS
  NS_DECL_NSIWEBSHELLSERVICES
  NS_DECL_NSINETWORKINTERCEPTCONTROLLER
  NS_FORWARD_SAFE_NSIDOMSTORAGEMANAGER(TopSessionStorageManager())

  NS_IMETHOD Stop() override {
    
    
    return nsDocLoader::Stop();
  }

  
  
  NS_FORWARD_NSISECURITYEVENTSINK(nsDocLoader::)

  
  NS_IMETHOD OnLinkClick(nsIContent* aContent,
                         nsIURI* aURI,
                         const char16_t* aTargetSpec,
                         const nsAString& aFileName,
                         nsIInputStream* aPostDataStream,
                         nsIInputStream* aHeadersDataStream,
                         bool aIsTrusted) override;
  NS_IMETHOD OnLinkClickSync(nsIContent* aContent,
                             nsIURI* aURI,
                             const char16_t* aTargetSpec,
                             const nsAString& aFileName,
                             nsIInputStream* aPostDataStream = 0,
                             nsIInputStream* aHeadersDataStream = 0,
                             nsIDocShell** aDocShell = 0,
                             nsIRequest** aRequest = 0) override;
  NS_IMETHOD OnOverLink(nsIContent* aContent,
                        nsIURI* aURI,
                        const char16_t* aTargetSpec) override;
  NS_IMETHOD OnLeaveLink() override;

  nsDocShellInfoLoadType ConvertLoadTypeToDocShellLoadInfo(uint32_t aLoadType);
  uint32_t ConvertDocShellLoadInfoToLoadType(
      nsDocShellInfoLoadType aDocShellLoadType);

  
  
  NS_IMETHOD GetAssociatedWindow(nsIDOMWindow**) override;
  NS_IMETHOD GetTopWindow(nsIDOMWindow**) override;
  NS_IMETHOD GetTopFrameElement(nsIDOMElement**) override;
  NS_IMETHOD GetNestedFrameId(uint64_t*) override;
  NS_IMETHOD IsAppOfType(uint32_t, bool*) override;
  NS_IMETHOD GetIsContent(bool*) override;
  NS_IMETHOD GetUsePrivateBrowsing(bool*) override;
  NS_IMETHOD SetUsePrivateBrowsing(bool) override;
  NS_IMETHOD SetPrivateBrowsing(bool) override;
  NS_IMETHOD GetUseRemoteTabs(bool*) override;
  NS_IMETHOD SetRemoteTabs(bool) override;

  
  
  
  nsresult RestoreFromHistory();

  
  
  
  
  nsresult ForceRefreshURIFromTimer(nsIURI* aURI, int32_t aDelay,
                                    bool aMetaRefresh, nsITimer* aTimer);

  friend class OnLinkClickEvent;

  
  
  void FireDummyOnLocationChange()
  {
    FireOnLocationChange(this, nullptr, mCurrentURI,
                         LOCATION_CHANGE_SAME_DOCUMENT);
  }

  nsresult HistoryTransactionRemoved(int32_t aIndex);

  
  
  void NotifyAsyncPanZoomStarted();
  
  
  void NotifyAsyncPanZoomStopped();

  
  
  
  void AddProfileTimelineMarker(const char* aName,
                                TracingMetadata aMetaData);
  void AddProfileTimelineMarker(mozilla::UniquePtr<TimelineMarker>&& aMarker);

  
  
  static unsigned long gProfileTimelineRecordingsCount;

protected:
  
  virtual ~nsDocShell();
  virtual void DestroyChildren() override;

  
  nsresult EnsureContentViewer();
  
  
  nsresult CreateAboutBlankContentViewer(nsIPrincipal* aPrincipal,
                                         nsIURI* aBaseURI,
                                         bool aTryToSaveOldPresentation = true);
  nsresult CreateContentViewer(const nsACString& aContentType,
                               nsIRequest* aRequest,
                               nsIStreamListener** aContentHandler);
  nsresult NewContentViewerObj(const nsACString& aContentType,
                               nsIRequest* aRequest, nsILoadGroup* aLoadGroup,
                               nsIStreamListener** aContentHandler,
                               nsIContentViewer** aViewer);
  nsresult SetupNewViewer(nsIContentViewer* aNewViewer);

  void SetupReferrerFromChannel(nsIChannel* aChannel);

  nsresult GetEldestPresContext(nsPresContext** aPresContext);

  
  
  
  
  
  
  nsIPrincipal* GetInheritedPrincipal(bool aConsiderCurrentDocument);

  
  
  
  
  
  nsresult DoURILoad(nsIURI* aURI,
                     nsIURI* aReferrer,
                     bool aSendReferrer,
                     uint32_t aReferrerPolicy,
                     nsISupports* aOwner,
                     const char* aTypeHint,
                     const nsAString& aFileName,
                     nsIInputStream* aPostData,
                     nsIInputStream* aHeadersData,
                     bool aFirstParty,
                     nsIDocShell** aDocShell,
                     nsIRequest** aRequest,
                     bool aIsNewWindowTarget,
                     bool aBypassClassifier,
                     bool aForceAllowCookies,
                     const nsAString& aSrcdoc,
                     nsIURI* aBaseURI,
                     nsContentPolicyType aContentPolicyType);
  nsresult AddHeadersToChannel(nsIInputStream* aHeadersData,
                               nsIChannel* aChannel);
  nsresult DoChannelLoad(nsIChannel* aChannel,
                         nsIURILoader* aURILoader,
                         bool aBypassClassifier);

  nsresult ScrollToAnchor(nsACString& aCurHash, nsACString& aNewHash,
                          uint32_t aLoadType);

  
  
  
  
  
  bool OnLoadingSite(nsIChannel* aChannel,
                     bool aFireOnLocationChange,
                     bool aAddToGlobalHistory = true);

  
  
  
  
  
  
  
  
  
  bool OnNewURI(nsIURI* aURI, nsIChannel* aChannel, nsISupports* aOwner,
                uint32_t aLoadType,
                bool aFireOnLocationChange,
                bool aAddToGlobalHistory,
                bool aCloneSHChildren);

  void SetReferrerURI(nsIURI* aURI);
  void SetReferrerPolicy(uint32_t aReferrerPolicy);

  
  bool ShouldAddToSessionHistory(nsIURI* aURI);
  
  
  
  
  
  
  nsresult AddToSessionHistory(nsIURI* aURI, nsIChannel* aChannel,
                               nsISupports* aOwner,
                               bool aCloneChildren,
                               nsISHEntry** aNewEntry);
  nsresult AddChildSHEntryToParent(nsISHEntry* aNewEntry, int32_t aChildOffset,
                                   bool aCloneChildren);

  nsresult AddChildSHEntryInternal(nsISHEntry* aCloneRef, nsISHEntry* aNewEntry,
                                   int32_t aChildOffset, uint32_t aLoadType,
                                   bool aCloneChildren);

  nsresult LoadHistoryEntry(nsISHEntry* aEntry, uint32_t aLoadType);
  nsresult PersistLayoutHistoryState();

  
  
  
  
  
  
  
  
  static nsresult CloneAndReplace(nsISHEntry* aSrcEntry,
                                  nsDocShell* aSrcShell,
                                  uint32_t aCloneID,
                                  nsISHEntry* aReplaceEntry,
                                  bool aCloneChildren,
                                  nsISHEntry** aDestEntry);

  
  static nsresult CloneAndReplaceChild(nsISHEntry* aEntry, nsDocShell* aShell,
                                       int32_t aChildIndex, void* aData);

  nsresult GetRootSessionHistory(nsISHistory** aReturn);
  nsresult GetHttpChannel(nsIChannel* aChannel, nsIHttpChannel** aReturn);
  bool ShouldDiscardLayoutState(nsIHttpChannel* aChannel);

  
  
  bool HasHistoryEntry(nsISHEntry* aEntry) const
  {
    return aEntry && (aEntry == mOSHE || aEntry == mLSHE);
  }

  
  void SwapHistoryEntries(nsISHEntry* aOldEntry, nsISHEntry* aNewEntry);

  
  
  
  void SetHistoryEntry(nsCOMPtr<nsISHEntry>* aPtr, nsISHEntry* aEntry);

  
  static nsresult SetChildHistoryEntry(nsISHEntry* aEntry, nsDocShell* aShell,
                                       int32_t aEntryIndex, void* aData);

  
  
  
  
  typedef nsresult(*WalkHistoryEntriesFunc)(nsISHEntry* aEntry,
                                            nsDocShell* aShell,
                                            int32_t aChildIndex,
                                            void* aData);

  
  
  
  static nsresult WalkHistoryEntries(nsISHEntry* aRootEntry,
                                     nsDocShell* aRootShell,
                                     WalkHistoryEntriesFunc aCallback,
                                     void* aData);

  
  
  virtual void OnRedirectStateChange(nsIChannel* aOldChannel,
                                     nsIChannel* aNewChannel,
                                     uint32_t aRedirectFlags,
                                     uint32_t aStateFlags) override;

  







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
                   uint32_t aResponseStatus = 0);

  
  nsresult ConfirmRepost(bool* aRepost);
  NS_IMETHOD GetPromptAndStringBundle(nsIPrompt** aPrompt,
                                      nsIStringBundle** aStringBundle);
  NS_IMETHOD GetChildOffset(nsIDOMNode* aChild, nsIDOMNode* aParent,
                            int32_t* aOffset);
  nsIScrollableFrame* GetRootScrollFrame();
  NS_IMETHOD EnsureScriptEnvironment();
  NS_IMETHOD EnsureEditorData();
  nsresult EnsureTransferableHookData();
  NS_IMETHOD EnsureFind();
  nsresult RefreshURIFromQueue();
  NS_IMETHOD LoadErrorPage(nsIURI* aURI, const char16_t* aURL,
                           const char* aErrorPage,
                           const char16_t* aErrorType,
                           const char16_t* aDescription,
                           const char* aCSSClass,
                           nsIChannel* aFailedChannel);
  bool IsPrintingOrPP(bool aDisplayErrorDialog = true);
  bool IsNavigationAllowed(bool aDisplayPrintErrorDialog = true,
                           bool aCheckIfUnloadFired = true);

  nsresult SetBaseUrlForWyciwyg(nsIContentViewer* aContentViewer);

  static inline uint32_t PRTimeToSeconds(PRTime aTimeUsec)
  {
    PRTime usecPerSec = PR_USEC_PER_SEC;
    return uint32_t(aTimeUsec /= usecPerSec);
  }

  inline bool UseErrorPages()
  {
    return (mObserveErrorPages ? sUseErrorPages : mUseErrorPages);
  }

  bool IsFrame();

  
  
  
  
  virtual nsresult EndPageLoad(nsIWebProgress* aProgress,
                               nsIChannel* aChannel,
                               nsresult aResult);

  
  
  
  nsresult SetDocCurrentStateObj(nsISHEntry* aShEntry);

  nsresult CheckLoadingPermissions();

  
  
  static bool CanAccessItem(nsIDocShellTreeItem* aTargetItem,
                            nsIDocShellTreeItem* aAccessingItem,
                            bool aConsiderOpener = true);
  static bool ValidateOrigin(nsIDocShellTreeItem* aOriginTreeItem,
                             nsIDocShellTreeItem* aTargetTreeItem);

  
  
  
  
  
  bool SetCurrentURI(nsIURI* aURI, nsIRequest* aRequest,
                     bool aFireOnLocationChange,
                     uint32_t aLocationFlags);

  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  bool CanSavePresentation(uint32_t aLoadType,
                           nsIRequest* aNewRequest,
                           nsIDocument* aNewDocument);

  
  
  
  nsresult CaptureState();

  
  
  
  nsresult RestorePresentation(nsISHEntry* aSHEntry, bool* aRestoring);

  
  nsresult BeginRestoreChildren();

  
  void DoGetPositionAndSize(int32_t* aX, int32_t* aY, int32_t* aWidth,
                            int32_t* aHeight);

  
  
  
  bool IsOKToLoadURI(nsIURI* aURI);

  void ReattachEditorToWindow(nsISHEntry* aSHEntry);

  nsCOMPtr<nsIDOMStorageManager> mSessionStorageManager;
  nsIDOMStorageManager* TopSessionStorageManager();

  
  nsresult GetControllerForCommand(const char* aCommand,
                                   nsIController** aResult);
  nsresult EnsureCommandHandler();

  nsIChannel* GetCurrentDocChannel();

  bool ShouldBlockLoadingForBackButton();

  
  already_AddRefed<nsDocShell> GetParentDocshell();

  
  
  bool DoAppRedirectIfNeeded(nsIURI* aURI,
                             nsIDocShellLoadInfo* aLoadInfo,
                             bool aFirstParty);

protected:
  nsresult GetCurScrollPos(int32_t aScrollOrientation, int32_t* aCurPos);
  nsresult SetCurScrollPosEx(int32_t aCurHorizontalPos,
                             int32_t aCurVerticalPos);

  
  virtual nsresult SetDocLoaderParent(nsDocLoader* aLoader) override;

  void ClearFrameHistory(nsISHEntry* aEntry);

  



  void MaybeInitTiming();

public:
  
  class RestorePresentationEvent : public nsRunnable
  {
  public:
    NS_DECL_NSIRUNNABLE
    explicit RestorePresentationEvent(nsDocShell* aDs) : mDocShell(aDs) {}
    void Revoke() { mDocShell = nullptr; }
  private:
    nsRefPtr<nsDocShell> mDocShell;
  };

protected:
  bool JustStartedNetworkLoad();

  nsresult CreatePrincipalFromReferrer(nsIURI* aReferrer,
                                       nsIPrincipal** aResult);

  enum FrameType
  {
    eFrameTypeRegular,
    eFrameTypeBrowser,
    eFrameTypeApp
  };

  static const nsCString FrameTypeToString(FrameType aFrameType)
  {
    switch (aFrameType) {
      case FrameType::eFrameTypeApp:
        return NS_LITERAL_CSTRING("app");
      case FrameType::eFrameTypeBrowser:
        return NS_LITERAL_CSTRING("browser");
      case FrameType::eFrameTypeRegular:
        return NS_LITERAL_CSTRING("regular");
      default:
        NS_ERROR("Unknown frame type");
        return EmptyCString();
    }
  }

  FrameType GetInheritedFrameType();

  bool HasUnloadedParent();

  
  nsIntRect mBounds;
  nsString mName;
  nsString mTitle;

  



  nsCString mContentTypeHint;
  nsIntPoint mDefaultScrollbarPref; 

  nsCOMPtr<nsISupportsArray> mRefreshURIList;
  nsCOMPtr<nsISupportsArray> mSavedRefreshURIList;
  nsRefPtr<nsDSURIContentListener> mContentListener;
  nsCOMPtr<nsIContentViewer> mContentViewer;
  nsCOMPtr<nsIWidget> mParentWidget;

  
  nsCOMPtr<nsIURI> mCurrentURI;
  nsCOMPtr<nsIURI> mReferrerURI;
  uint32_t mReferrerPolicy;
  nsRefPtr<nsGlobalWindow> mScriptGlobal;
  nsCOMPtr<nsISHistory> mSessionHistory;
  nsCOMPtr<nsIGlobalHistory2> mGlobalHistory;
  nsCOMPtr<nsIWebBrowserFind> mFind;
  nsCOMPtr<nsICommandManager> mCommandManager;
  
  
  nsCOMPtr<nsISHEntry> mOSHE;
  
  
  
  
  
  
  nsCOMPtr<nsISHEntry> mLSHE;

  
  
  
  nsRevocableEventPtr<RestorePresentationEvent> mRestorePresentationEvent;

  
  nsAutoPtr<nsDocShellEditorData> mEditorData;

  
  nsCOMPtr<nsIClipboardDragDropHookList> mTransferableHookData;

  
  nsCOMPtr<nsISecureBrowserUI> mSecurityUI;

  
  
  
  
  
  nsCOMPtr<nsIURI> mLoadingURI;

  
  
  
  nsCOMPtr<nsIURI> mFailedURI;
  nsCOMPtr<nsIChannel> mFailedChannel;
  uint32_t mFailedLoadType;

  
  nsRefPtr<mozilla::dom::URLSearchParams> mURLSearchParams;

  
  
  
  nsCOMPtr<nsIChannel> mMixedContentChannel;

  
  
  

  nsIDocShellTreeOwner* mTreeOwner; 
  mozilla::dom::EventTarget* mChromeEventHandler; 

  eCharsetReloadState mCharsetReloadState;

  
  
  uint32_t mChildOffset;
  uint32_t mBusyFlags;
  uint32_t mAppType;
  uint32_t mLoadType;

  int32_t mMarginWidth;
  int32_t mMarginHeight;

  
  
  int32_t mItemType;

  
  
  int32_t mPreviousTransIndex;
  int32_t mLoadedTransIndex;

  uint32_t mSandboxFlags;
  nsWeakPtr mOnePermittedSandboxedNavigator;

  
  
  
  
  
  
  
  
  
  
  
  
  enum FullscreenAllowedState
  {
    CHECK_ATTRIBUTES,
    PARENT_ALLOWS,
    PARENT_PROHIBITS
  };
  FullscreenAllowedState mFullscreenAllowed;

  
  static bool sUseErrorPages;

  bool mCreated;
  bool mAllowSubframes;
  bool mAllowPlugins;
  bool mAllowJavascript;
  bool mAllowMetaRedirects;
  bool mAllowImages;
  bool mAllowMedia;
  bool mAllowDNSPrefetch;
  bool mAllowWindowControl;
  bool mAllowContentRetargeting;
  bool mAllowContentRetargetingOnChildren;
  bool mCreatingDocument; 
  bool mUseErrorPages;
  bool mObserveErrorPages;
  bool mAllowAuth;
  bool mAllowKeywordFixup;
  bool mIsOffScreenBrowser;
  bool mIsActive;
  bool mIsPrerendered;
  bool mIsAppTab;
  bool mUseGlobalHistory;
  bool mInPrivateBrowsing;
  bool mUseRemoteTabs;
  bool mDeviceSizeIsPageSize;
  bool mWindowDraggingAllowed;

  
  
  
  bool mCanExecuteScripts;
  void RecomputeCanExecuteScripts();

  
  
  
  bool mFiredUnloadEvent;

  
  
  
  
  bool mEODForCurrentDocument;
  bool mURIResultedInDocument;

  bool mIsBeingDestroyed;

  bool mIsExecutingOnLoadHandler;

  
  bool mIsPrintingOrPP;

  
  
  
  bool mSavingOldViewer;

  
  bool mDynamicallyCreated;
#ifdef DEBUG
  bool mInEnsureScriptEnv;
#endif
  bool mAffectPrivateSessionLifetime;
  bool mInvisible;
  bool mHasLoadedNonBlankURI;
  uint64_t mHistoryID;
  uint32_t mDefaultLoadFlags;

  static nsIURIFixup* sURIFixup;

  nsRefPtr<nsDOMNavigationTiming> mTiming;

  
  
  
  bool mBlankTiming;

  
  FrameType mFrameType;

  
  
  
  
  
  
  
  
  uint32_t mOwnOrContainingAppId;

  nsString mPaymentRequestId;

  nsString GetInheritedPaymentRequestId();

private:
  nsCString mForcedCharset;
  nsCString mParentCharset;
  int32_t mParentCharsetSource;
  nsCOMPtr<nsIPrincipal> mParentCharsetPrincipal;
  nsTObserverArray<nsWeakPtr> mPrivacyObservers;
  nsTObserverArray<nsWeakPtr> mReflowObservers;
  nsTObserverArray<nsWeakPtr> mScrollObservers;
  nsCString mOriginalUriString;
  nsWeakPtr mOpener;

  
  
  uint32_t mJSRunToCompletionDepth;

  
  bool mProfileTimelineRecording;

  nsTArray<mozilla::UniquePtr<TimelineMarker>> mProfileTimelineMarkers;

  
  void ClearProfileTimelineMarkers();

  
  
  nsresult DoFindItemWithName(const char16_t* aName,
                              nsISupports* aRequestor,
                              nsIDocShellTreeItem* aOriginalRequestor,
                              nsIDocShellTreeItem** aResult);

  
  void MaybeNotifyKeywordSearchLoading(const nsString& aProvider,
                                       const nsString& aKeyword);

#ifdef DEBUG
  
  static unsigned long gNumberOfDocShells;
#endif 

public:
  class InterfaceRequestorProxy : public nsIInterfaceRequestor
  {
  public:
    explicit InterfaceRequestorProxy(nsIInterfaceRequestor* aRequestor);
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIINTERFACEREQUESTOR

  protected:
    virtual ~InterfaceRequestorProxy();
    InterfaceRequestorProxy() {}
    nsWeakPtr mWeakPtr;
  };
};

#endif 
