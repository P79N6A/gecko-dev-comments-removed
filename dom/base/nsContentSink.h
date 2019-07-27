









#ifndef _nsContentSink_h_
#define _nsContentSink_h_



#include "mozilla/Attributes.h"
#include "nsICSSLoaderObserver.h"
#include "nsWeakReference.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsAutoPtr.h"
#include "nsGkAtoms.h"
#include "nsITimer.h"
#include "nsStubDocumentObserver.h"
#include "nsIContentSink.h"
#include "prlog.h"
#include "nsCycleCollectionParticipant.h"
#include "nsThreadUtils.h"

class nsIDocument;
class nsIURI;
class nsIChannel;
class nsIDocShell;
class nsIParser;
class nsIAtom;
class nsIChannel;
class nsIContent;
class nsViewManager;
class nsNodeInfoManager;
class nsScriptLoader;
class nsIApplicationCache;

namespace mozilla {
namespace css {
class Loader;
}
}

#ifdef DEBUG

extern PRLogModuleInfo* gContentSinkLogModuleInfo;

#define SINK_TRACE_CALLS              0x1
#define SINK_TRACE_REFLOW             0x2
#define SINK_ALWAYS_REFLOW            0x4

#define SINK_LOG_TEST(_lm, _bit) (int((_lm)->level) & (_bit))

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

  
  NS_IMETHOD StyleSheetLoaded(mozilla::CSSStyleSheet* aSheet,
                              bool aWasAlternate,
                              nsresult aStatus) override;

  virtual nsresult ProcessMETATag(nsIContent* aContent);

  
  nsresult WillParseImpl(void);
  nsresult WillInterruptImpl(void);
  nsresult WillResumeImpl(void);
  nsresult DidProcessATokenImpl(void);
  void WillBuildModelImpl(void);
  void DidBuildModelImpl(bool aTerminated);
  void DropParserAndPerfHint(void);
  bool IsScriptExecutingImpl();

  void NotifyAppend(nsIContent* aContent, uint32_t aStartIndex);

  
  NS_DECL_NSIDOCUMENTOBSERVER_BEGINUPDATE
  NS_DECL_NSIDOCUMENTOBSERVER_ENDUPDATE

  virtual void UpdateChildCounts() = 0;

  bool IsTimeToNotify();
  bool LinkContextIsOurDocument(const nsSubstring& aAnchor);
  bool Decode5987Format(nsAString& aEncoded);

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
                             nsIContent* aContent = nullptr);
  nsresult ProcessLinkHeader(const nsAString& aLinkData);
  nsresult ProcessLink(const nsSubstring& aAnchor,
                       const nsSubstring& aHref, const nsSubstring& aRel,
                       const nsSubstring& aTitle, const nsSubstring& aType,
                       const nsSubstring& aMedia);

  virtual nsresult ProcessStyleLink(nsIContent* aElement,
                                    const nsSubstring& aHref,
                                    bool aAlternate,
                                    const nsSubstring& aTitle,
                                    const nsSubstring& aType,
                                    const nsSubstring& aMedia);

  void PrefetchHref(const nsAString &aHref, nsINode *aSource,
                    bool aExplicit);

  
  
  void PrefetchDNS(const nsAString &aHref);
  void Preconnect(const nsAString &aHref);

  
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

  inline int32_t GetNotificationInterval()
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

  
  int32_t mBackoffCount;

  
  
  PRTime mLastNotificationTime;

  
  nsCOMPtr<nsITimer> mNotificationTimer;

  
  uint8_t mBeganUpdate : 1;
  uint8_t mLayoutStarted : 1;
  uint8_t mDynamicLowerValue : 1;
  uint8_t mParsing : 1;
  uint8_t mDroppedTimer : 1;
  
  uint8_t mDeferredLayoutStart : 1;
  
  uint8_t mDeferredFlushTags : 1;
  
  
  
  uint8_t mIsDocumentObserver : 1;
  
  
  uint8_t mRunsToCompletion : 1;
  
  
  
  

  
  
  uint32_t mDeflectedCount;

  
  bool mHasPendingEvent;

  
  uint32_t mCurrentParseEndTime;

  int32_t mBeginLoadTime;

  
  
  uint32_t mLastSampledUserEventTime;

  int32_t mInMonolithicContainer;

  int32_t mInNotification;
  uint32_t mUpdatesInNotification;

  uint32_t mPendingSheetCount;

  nsRevocableEventPtr<nsRunnableMethod<nsContentSink, void, false> >
    mProcessLinkHeaderEvent;

  
  static bool sNotifyOnTimer;
  
  static int32_t sBackoffCount;
  
  static int32_t sNotificationInterval;
  
  static int32_t sInteractiveDeflectCount;
  static int32_t sPerfDeflectCount;
  
  
  
  static int32_t sPendingEventMode;
  
  static int32_t sEventProbeRate;
  
  static int32_t sInteractiveParseTime;
  static int32_t sPerfParseTime;
  
  static int32_t sInteractiveTime;
  
  static int32_t sInitialPerfTime;
  
  static int32_t sEnablePerfMode;
};

#endif 
