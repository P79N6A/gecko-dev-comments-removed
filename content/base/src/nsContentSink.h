









#ifndef _nsContentSink_h_
#define _nsContentSink_h_



#include "nsICSSLoaderObserver.h"
#include "nsWeakReference.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsString.h"
#include "nsAutoPtr.h"
#include "nsGkAtoms.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsTArray.h"
#include "nsITimer.h"
#include "nsStubDocumentObserver.h"
#include "nsIParserService.h"
#include "nsIContentSink.h"
#include "prlog.h"
#include "nsIRequest.h"
#include "nsCycleCollectionParticipant.h"
#include "nsThreadUtils.h"
#include "nsIScriptElement.h"

class nsIDocument;
class nsIURI;
class nsIChannel;
class nsIDocShell;
class nsIParser;
class nsIAtom;
class nsIChannel;
class nsIContent;
class nsIViewManager;
class nsNodeInfoManager;
class nsScriptLoader;
class nsIApplicationCache;

namespace mozilla {
namespace css {
class Loader;
}
}

#ifdef NS_DEBUG

extern PRLogModuleInfo* gContentSinkLogModuleInfo;

#define SINK_TRACE_CALLS              0x1
#define SINK_TRACE_REFLOW             0x2
#define SINK_ALWAYS_REFLOW            0x4

#define SINK_LOG_TEST(_lm, _bit) (PRIntn((_lm)->level) & (_bit))

#define SINK_TRACE(_lm, _bit, _args) \
  PR_BEGIN_MACRO                     \
    if (SINK_LOG_TEST(_lm, _bit)) {  \
      PR_LogPrint _args;             \
    }                                \
  PR_END_MACRO

#else
#define SINK_TRACE(_lm, _bit, _args)
#endif

#undef SINK_NO_INCREMENTAL




#define NS_DELAY_FOR_WINDOW_CREATION  500000

class nsContentSink : public nsICSSLoaderObserver,
                      public nsSupportsWeakReference,
                      public nsStubDocumentObserver,
                      public nsITimerCallback
{
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsContentSink,
                                           nsICSSLoaderObserver)
    
  NS_DECL_NSITIMERCALLBACK

  
  NS_IMETHOD StyleSheetLoaded(nsCSSStyleSheet* aSheet, bool aWasAlternate,
                              nsresult aStatus);

  virtual nsresult ProcessMETATag(nsIContent* aContent);

  
  NS_HIDDEN_(nsresult) WillParseImpl(void);
  NS_HIDDEN_(nsresult) WillInterruptImpl(void);
  NS_HIDDEN_(nsresult) WillResumeImpl(void);
  NS_HIDDEN_(nsresult) DidProcessATokenImpl(void);
  NS_HIDDEN_(void) WillBuildModelImpl(void);
  NS_HIDDEN_(void) DidBuildModelImpl(bool aTerminated);
  NS_HIDDEN_(void) DropParserAndPerfHint(void);
  bool IsScriptExecutingImpl();

  void NotifyAppend(nsIContent* aContent, PRUint32 aStartIndex);

  
  NS_DECL_NSIDOCUMENTOBSERVER_BEGINUPDATE
  NS_DECL_NSIDOCUMENTOBSERVER_ENDUPDATE

  virtual void UpdateChildCounts() = 0;

  bool IsTimeToNotify();
  bool LinkContextIsOurDocument(const nsSubstring& aAnchor);

  static void InitializeStatics();

protected:
  nsContentSink();
  virtual ~nsContentSink();

  enum CacheSelectionAction {
    
    
    
    
    
    CACHE_SELECTION_NONE = 0,

    
    CACHE_SELECTION_UPDATE = 1,

    
    
    
    
    
    CACHE_SELECTION_RELOAD = 2,

    
    CACHE_SELECTION_RESELECT_WITHOUT_MANIFEST = 3
  };

  nsresult Init(nsIDocument* aDoc, nsIURI* aURI,
                nsISupports* aContainer, nsIChannel* aChannel);

  nsresult ProcessHTTPHeaders(nsIChannel* aChannel);
  nsresult ProcessHeaderData(nsIAtom* aHeader, const nsAString& aValue,
                             nsIContent* aContent = nsnull);
  nsresult ProcessLinkHeader(nsIContent* aElement,
                             const nsAString& aLinkData);
  nsresult ProcessLink(nsIContent* aElement, const nsSubstring& aAnchor,
                       const nsSubstring& aHref, const nsSubstring& aRel,
                       const nsSubstring& aTitle, const nsSubstring& aType,
                       const nsSubstring& aMedia);

  virtual nsresult ProcessStyleLink(nsIContent* aElement,
                                    const nsSubstring& aHref,
                                    bool aAlternate,
                                    const nsSubstring& aTitle,
                                    const nsSubstring& aType,
                                    const nsSubstring& aMedia);

  void PrefetchHref(const nsAString &aHref, nsIContent *aSource,
                    bool aExplicit);

  
  
  void PrefetchDNS(const nsAString &aHref);

  
  nsresult GetChannelCacheKey(nsIChannel* aChannel, nsACString& aCacheKey);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsresult SelectDocAppCache(nsIApplicationCache *aLoadApplicationCache,
                             nsIURI *aManifestURI,
                             bool aFetchedWithHTTPGetOrEquiv,
                             CacheSelectionAction *aAction);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsresult SelectDocAppCacheNoManifest(nsIApplicationCache *aLoadApplicationCache,
                                       nsIURI **aManifestURI,
                                       CacheSelectionAction *aAction);

public:
  
  
  
  
  
  
  void ProcessOfflineManifest(const nsAString& aManifestSpec);

  
  
  void ProcessOfflineManifest(nsIContent *aElement);

protected:
  
  
  void ScrollToRef();

  
  
  
public:
  void StartLayout(bool aIgnorePendingSheets);

  static void NotifyDocElementCreated(nsIDocument* aDoc);

protected:
  void
  FavorPerformanceHint(bool perfOverStarvation, PRUint32 starvationDelay);

  inline PRInt32 GetNotificationInterval()
  {
    if (mDynamicLowerValue) {
      return 1000;
    }

    return sNotificationInterval;
  }

  virtual nsresult FlushTags() = 0;

  
  
  bool WaitForPendingSheets() { return mPendingSheetCount > 0; }

  void DoProcessLinkHeader();

  void StopDeflecting() {
    mDeflectedCount = sPerfDeflectCount;
  }

private:
  
  
  void* operator new(size_t sz) CPP_THROW_NEW;  

protected:

  nsCOMPtr<nsIDocument>         mDocument;
  nsRefPtr<nsParserBase>        mParser;
  nsCOMPtr<nsIURI>              mDocumentURI;
  nsCOMPtr<nsIDocShell>         mDocShell;
  nsRefPtr<mozilla::css::Loader> mCSSLoader;
  nsRefPtr<nsNodeInfoManager>   mNodeInfoManager;
  nsRefPtr<nsScriptLoader>      mScriptLoader;

  
  PRInt32 mBackoffCount;

  
  
  PRTime mLastNotificationTime;

  
  nsCOMPtr<nsITimer> mNotificationTimer;

  
  PRUint8 mBeganUpdate : 1;
  PRUint8 mLayoutStarted : 1;
  PRUint8 mDynamicLowerValue : 1;
  PRUint8 mParsing : 1;
  PRUint8 mDroppedTimer : 1;
  
  PRUint8 mDeferredLayoutStart : 1;
  
  PRUint8 mDeferredFlushTags : 1;
  
  
  
  PRUint8 mIsDocumentObserver : 1;
  
  
  PRUint8 mRunsToCompletion : 1;
  
  PRUint8 mPreventScriptExecution : 1;
  
  
  
  

  
  
  PRUint32 mDeflectedCount;

  
  bool mHasPendingEvent;

  
  PRUint32 mCurrentParseEndTime;

  PRInt32 mBeginLoadTime;

  
  
  PRUint32 mLastSampledUserEventTime;

  PRInt32 mInMonolithicContainer;

  PRInt32 mInNotification;
  PRUint32 mUpdatesInNotification;

  PRUint32 mPendingSheetCount;

  nsRevocableEventPtr<nsRunnableMethod<nsContentSink, void, false> >
    mProcessLinkHeaderEvent;

  
  static bool sNotifyOnTimer;
  
  static PRInt32 sBackoffCount;
  
  static PRInt32 sNotificationInterval;
  
  static PRInt32 sInteractiveDeflectCount;
  static PRInt32 sPerfDeflectCount;
  
  
  
  static PRInt32 sPendingEventMode;
  
  static PRInt32 sEventProbeRate;
  
  static PRInt32 sInteractiveParseTime;
  static PRInt32 sPerfParseTime;
  
  static PRInt32 sInteractiveTime;
  
  static PRInt32 sInitialPerfTime;
  
  static PRInt32 sEnablePerfMode;
};

#endif 
