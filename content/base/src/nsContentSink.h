









































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
#include "nsITimer.h"
#include "nsStubDocumentObserver.h"
#include "nsIParserService.h"
#include "nsIContentSink.h"
#include "prlog.h"
#include "nsIRequest.h"
#include "nsTimer.h"

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
class nsIOfflineCacheSession;

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



#define NS_MAX_TOKENS_DEFLECTED_IN_LOW_FREQ_MODE 200

class nsContentSink : public nsICSSLoaderObserver,
                      public nsIScriptLoaderObserver,
                      public nsSupportsWeakReference,
                      public nsStubDocumentObserver,
                      public nsITimerCallback
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSISCRIPTLOADEROBSERVER

    
  NS_DECL_NSITIMERCALLBACK

  
  NS_IMETHOD StyleSheetLoaded(nsICSSStyleSheet* aSheet, PRBool aWasAlternate,
                              nsresult aStatus);

  nsresult ProcessMETATag(nsIContent* aContent);

  
  NS_HIDDEN_(nsresult) WillInterruptImpl(void);
  NS_HIDDEN_(nsresult) WillResumeImpl(void);
  NS_HIDDEN_(nsresult) DidProcessATokenImpl(void);
  NS_HIDDEN_(void) WillBuildModelImpl(void);
  NS_HIDDEN_(void) DidBuildModelImpl(void);
  NS_HIDDEN_(void) DropParserAndPerfHint(void);
  NS_HIDDEN_(nsresult) WillProcessTokensImpl(void);

  void NotifyAppend(nsIContent* aContent, PRUint32 aStartIndex);

  
  virtual void BeginUpdate(nsIDocument *aDocument, nsUpdateType aUpdateType);
  virtual void EndUpdate(nsIDocument *aDocument, nsUpdateType aUpdateType);

  virtual void UpdateChildCounts() = 0;

protected:
  nsContentSink();
  virtual ~nsContentSink();

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
                    PRBool aExplicit, PRBool aOffline);
  nsresult GetOfflineCacheSession(nsIOfflineCacheSession **aSession);
  nsresult AddOfflineResource(const nsAString &aHref);

  void ScrollToRef();
  nsresult RefreshIfEnabled(nsIViewManager* vm);

  
  
  
  void StartLayout(PRBool aIgnorePendingSheets);

  PRBool IsTimeToNotify();

  void
  FavorPerformanceHint(PRBool perfOverStarvation, PRUint32 starvationDelay);

  inline PRInt32 GetNotificationInterval()
  {
    if (mDynamicLowerValue) {
      return 1000;
    }

    return mNotificationInterval;
  }

  inline PRInt32 GetMaxTokenProcessingTime()
  {
    if (mDynamicLowerValue) {
      return 3000;
    }

    return mMaxTokenProcessingTime;
  }

  
  virtual void PreEvaluateScript()                            {return;}
  virtual void PostEvaluateScript(nsIScriptElement *aElement) {return;}

  virtual nsresult FlushTags() = 0;

  void TryToScrollToRef();

private:
  
  
  void* operator new(size_t sz) CPP_THROW_NEW;  

protected:

  void ContinueInterruptedParsingAsync();
  void ContinueInterruptedParsingIfEnabled();
  void ContinueInterruptedParsing();

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

  
  
  
  
  PRUint8 mDeflectedCount;

  
  PRPackedBool mNotifyOnTimer;

  
  nsCOMPtr<nsIOfflineCacheSession> mOfflineCacheSession;

  
  PRUint8 mBeganUpdate : 1;
  PRUint8 mLayoutStarted : 1;
  PRUint8 mScrolledToRefAlready : 1;
  PRUint8 mCanInterruptParser : 1;
  PRUint8 mDynamicLowerValue : 1;
  PRUint8 mParsing : 1;
  PRUint8 mDroppedTimer : 1;
  PRUint8 mInTitle : 1;
  PRUint8 mChangeScrollPosWhenScrollingToRef : 1;
  
  PRUint8 mDeferredLayoutStart : 1;
  
  PRUint8 mHaveOfflineResources : 1;
  
  PRUint8 mSaveOfflineResources : 1;

  
  PRUint32 mDelayTimerStart;

  
  PRInt32 mMaxTokenProcessingTime;

  
  PRInt32 mDynamicIntervalSwitchThreshold;

  PRInt32 mBeginLoadTime;

  
  
  PRUint32 mLastSampledUserEventTime;

  PRInt32 mInMonolithicContainer;

  PRInt32 mInNotification;
  PRUint32 mUpdatesInNotification;

  PRUint32 mPendingSheetCount;

  
  MOZ_TIMER_DECLARE(mWatch)
};


extern PRBool IsAttrURI(nsIAtom *aName);
extern nsIAtom** const kDefaultAllowedTags [];
extern nsIAtom** const kDefaultAllowedAttributes [];

#endif
