









































#ifndef _nsContentSink_h_
#define _nsContentSink_h_



#include "nsICSSLoaderObserver.h"
#include "nsIScriptLoaderObserver.h"
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

class nsIDocument;
class nsIURI;
class nsIChannel;
class nsIDocShell;
class nsICSSLoader;
class nsIParser;
class nsIAtom;
class nsIChannel;
class nsIContent;
class nsIViewManager;
class nsNodeInfoManager;
class nsScriptLoader;
class nsIApplicationCache;

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
                      public nsIScriptLoaderObserver,
                      public nsSupportsWeakReference,
                      public nsStubDocumentObserver,
                      public nsITimerCallback
{
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsContentSink,
                                           nsIScriptLoaderObserver)
  NS_DECL_NSISCRIPTLOADEROBSERVER

    
  NS_DECL_NSITIMERCALLBACK

  
  NS_IMETHOD StyleSheetLoaded(nsICSSStyleSheet* aSheet, PRBool aWasAlternate,
                              nsresult aStatus);

  virtual nsresult ProcessMETATag(nsIContent* aContent);

  
  NS_HIDDEN_(nsresult) WillParseImpl(void);
  NS_HIDDEN_(nsresult) WillInterruptImpl(void);
  NS_HIDDEN_(nsresult) WillResumeImpl(void);
  NS_HIDDEN_(nsresult) DidProcessATokenImpl(void);
  NS_HIDDEN_(void) WillBuildModelImpl(void);
  NS_HIDDEN_(void) DidBuildModelImpl(PRBool aTerminated);
  NS_HIDDEN_(void) DropParserAndPerfHint(void);
  PRBool IsScriptExecutingImpl();

  void NotifyAppend(nsIContent* aContent, PRUint32 aStartIndex);

  
  virtual void BeginUpdate(nsIDocument *aDocument, nsUpdateType aUpdateType);
  virtual void EndUpdate(nsIDocument *aDocument, nsUpdateType aUpdateType);

  virtual void UpdateChildCounts() = 0;

  PRBool IsTimeToNotify();

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
  nsresult ProcessLink(nsIContent* aElement, const nsSubstring& aHref,
                       const nsSubstring& aRel, const nsSubstring& aTitle,
                       const nsSubstring& aType, const nsSubstring& aMedia);

  virtual nsresult ProcessStyleLink(nsIContent* aElement,
                                    const nsSubstring& aHref,
                                    PRBool aAlternate,
                                    const nsSubstring& aTitle,
                                    const nsSubstring& aType,
                                    const nsSubstring& aMedia);

  void PrefetchHref(const nsAString &aHref, nsIContent *aSource,
                    PRBool aExplicit);

  
  
  void PrefetchDNS(const nsAString &aHref);

  
  nsresult GetChannelCacheKey(nsIChannel* aChannel, nsACString& aCacheKey);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsresult SelectDocAppCache(nsIApplicationCache *aLoadApplicationCache,
                             nsIURI *aManifestURI,
                             PRBool aFetchedWithHTTPGetOrEquiv,
                             CacheSelectionAction *aAction);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsresult SelectDocAppCacheNoManifest(nsIApplicationCache *aLoadApplicationCache,
                                       nsIURI **aManifestURI,
                                       CacheSelectionAction *aAction);

public:
  
  
  
  
  void ProcessOfflineManifest(nsIContent *aElement);

protected:
  
  
  void ScrollToRef();
  nsresult RefreshIfEnabled(nsIViewManager* vm);

  
  
  
public:
  void StartLayout(PRBool aIgnorePendingSheets);
protected:
  void
  FavorPerformanceHint(PRBool perfOverStarvation, PRUint32 starvationDelay);

  inline PRInt32 GetNotificationInterval()
  {
    if (mDynamicLowerValue) {
      return 1000;
    }

    return mNotificationInterval;
  }

  
  virtual void PreEvaluateScript()                            {return;}
  virtual void PostEvaluateScript(nsIScriptElement *aElement) {return;}

  virtual nsresult FlushTags() = 0;

  
  
  PRBool WaitForPendingSheets() { return mPendingSheetCount > 0; }

  void DoProcessLinkHeader();

private:
  
  
  void* operator new(size_t sz) CPP_THROW_NEW;  

protected:

  void ContinueInterruptedParsingAsync();
  void ContinueInterruptedParsingIfEnabled();

  nsCOMPtr<nsIDocument>         mDocument;
  nsCOMPtr<nsIParser>           mParser;
  nsCOMPtr<nsIURI>              mDocumentURI;
  nsCOMPtr<nsIURI>              mDocumentBaseURI;
  nsCOMPtr<nsIDocShell>         mDocShell;
  nsCOMPtr<nsICSSLoader>        mCSSLoader;
  nsRefPtr<nsNodeInfoManager>   mNodeInfoManager;
  nsRefPtr<nsScriptLoader>      mScriptLoader;

  nsCOMArray<nsIScriptElement> mScriptElements;

  nsCString mRef; 

  
  PRInt32 mBackoffCount;

  
  PRInt32 mNotificationInterval;

  
  
  PRTime mLastNotificationTime;

  
  nsCOMPtr<nsITimer> mNotificationTimer;

  
  PRPackedBool mNotifyOnTimer;

  
  PRUint8 mBeganUpdate : 1;
  PRUint8 mLayoutStarted : 1;
  PRUint8 mScrolledToRefAlready : 1;
  PRUint8 mCanInterruptParser : 1;
  PRUint8 mDynamicLowerValue : 1;
  PRUint8 mParsing : 1;
  PRUint8 mDroppedTimer : 1;
  PRUint8 mChangeScrollPosWhenScrollingToRef : 1;
  
  PRUint8 mDeferredLayoutStart : 1;
  
  PRUint8 mDeferredFlushTags : 1;
  
  
  
  PRUint8 mIsDocumentObserver : 1;
  
  
  
  

  
  
  PRUint32 mDeflectedCount;

  
  PRInt32 mInteractiveDeflectCount;
  PRInt32 mPerfDeflectCount;

  
  
  
  PRInt32 mPendingEventMode;

  
  PRInt32 mEventProbeRate;

  
  PRBool mHasPendingEvent;

  
  PRInt32 mCurrentParseEndTime;

  
  PRInt32 mInteractiveParseTime;
  PRInt32 mPerfParseTime;

  
  PRInt32 mInteractiveTime;
  
  PRInt32 mInitialPerfTime;

  
  PRBool mEnablePerfMode;

  PRInt32 mBeginLoadTime;

  
  
  PRUint32 mLastSampledUserEventTime;

  PRInt32 mInMonolithicContainer;

  PRInt32 mInNotification;
  PRUint32 mUpdatesInNotification;

  PRUint32 mPendingSheetCount;

  nsRevocableEventPtr<nsNonOwningRunnableMethod<nsContentSink> >
    mProcessLinkHeaderEvent;
};


extern PRBool IsAttrURI(nsIAtom *aName);
extern nsIAtom** const kDefaultAllowedTags [];
extern nsIAtom** const kDefaultAllowedAttributes [];

#endif 
