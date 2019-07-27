




#ifndef mozilla_net_Predictor_h
#define mozilla_net_Predictor_h

#include "nsINetworkPredictor.h"

#include "nsCOMPtr.h"
#include "nsICacheEntry.h"
#include "nsICacheEntryOpenCallback.h"
#include "nsICacheStorageVisitor.h"
#include "nsIDNSListener.h"
#include "nsIInterfaceRequestor.h"
#include "nsIObserver.h"
#include "nsISpeculativeConnect.h"
#include "nsRefPtr.h"
#include "nsString.h"
#include "nsTArray.h"

#include "mozilla/TimeStamp.h"

class nsICacheStorage;
class nsIDNSService;
class nsIIOService;
class nsINetworkPredictorVerifier;
class nsITimer;

namespace mozilla {
namespace net {

class Predictor : public nsINetworkPredictor
                , public nsIObserver
                , public nsISpeculativeConnectionOverrider
                , public nsIInterfaceRequestor
                , public nsICacheEntryMetaDataVisitor
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSINETWORKPREDICTOR
  NS_DECL_NSIOBSERVER
  NS_DECL_NSISPECULATIVECONNECTIONOVERRIDER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICACHEENTRYMETADATAVISITOR

  Predictor();

  nsresult Init();
  void Shutdown();
  static nsresult Create(nsISupports *outer, const nsIID& iid, void **result);

private:
  virtual ~Predictor();

  union Reason {
    PredictorLearnReason mLearn;
    PredictorPredictReason mPredict;
  };

  class DNSListener : public nsIDNSListener
  {
  public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIDNSLISTENER

    DNSListener()
    { }

  private:
    virtual ~DNSListener()
    { }
  };

  class Action : public nsICacheEntryOpenCallback
  {
  public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSICACHEENTRYOPENCALLBACK

    Action(bool fullUri, bool predict, Reason reason,
           nsIURI *targetURI, nsIURI *sourceURI,
           nsINetworkPredictorVerifier *verifier, Predictor *predictor);
    Action(bool fullUri, bool predict, Reason reason,
           nsIURI *targetURI, nsIURI *sourceURI,
           nsINetworkPredictorVerifier *verifier, Predictor *predictor,
           uint8_t stackCount);

    static const bool IS_FULL_URI = true;
    static const bool IS_ORIGIN = false;

    static const bool DO_PREDICT = true;
    static const bool DO_LEARN = false;

  private:
    virtual ~Action();

    bool mFullUri : 1;
    bool mPredict : 1;
    union {
      PredictorPredictReason mPredictReason;
      PredictorLearnReason mLearnReason;
    };
    nsCOMPtr<nsIURI> mTargetURI;
    nsCOMPtr<nsIURI> mSourceURI;
    nsCOMPtr<nsINetworkPredictorVerifier> mVerifier;
    TimeStamp mStartTime;
    uint8_t mStackCount;
    nsRefPtr<Predictor> mPredictor;
  };

  class Resetter : public nsICacheEntryOpenCallback,
                   public nsICacheEntryMetaDataVisitor,
                   public nsICacheStorageVisitor
  {
  public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSICACHEENTRYOPENCALLBACK
    NS_DECL_NSICACHEENTRYMETADATAVISITOR
    NS_DECL_NSICACHESTORAGEVISITOR

    explicit Resetter(Predictor *predictor);

  private:
    virtual ~Resetter() { }

    void Complete();

    uint32_t mEntriesToVisit;
    nsTArray<nsCString> mKeysToDelete;
    nsRefPtr<Predictor> mPredictor;
    nsTArray<nsCOMPtr<nsIURI>> mURIsToVisit;
  };

  class SpaceCleaner : public nsICacheEntryMetaDataVisitor
  {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICACHEENTRYMETADATAVISITOR

    explicit SpaceCleaner(Predictor *predictor)
      :mLRUStamp(0)
      ,mKeyToDelete(nullptr)
      ,mPredictor(predictor)
    { }

    void Finalize(nsICacheEntry *entry);

  private:
    virtual ~SpaceCleaner() { }
    uint32_t mLRUStamp;
    const char *mKeyToDelete;
    nsRefPtr<Predictor> mPredictor;
  };

  
  nsresult InstallObserver();
  void RemoveObserver();

  
  void MaybeCleanupOldDBFiles();

  

  
  
  
  
  
  
  
  
  
  
  
  
  
  bool PredictInternal(PredictorPredictReason reason, nsICacheEntry *entry,
                       bool isNew, bool fullUri, nsIURI *targetURI,
                       nsINetworkPredictorVerifier *verifier,
                       uint8_t stackCount);

  
  
  
  
  void PredictForLink(nsIURI *targetURI,
                      nsIURI *sourceURI,
                      nsINetworkPredictorVerifier *verifier);

  
  
  
  bool PredictForPageload(nsICacheEntry *entry,
                          uint8_t stackCount,
                          nsINetworkPredictorVerifier *verifier);

  
  
  
  bool PredictForStartup(nsICacheEntry *entry,
                         nsINetworkPredictorVerifier *verifier);

  

  
  
  
  
  
  int32_t CalculateGlobalDegradation(uint32_t lastLoad);

  
  
  
  
  
  
  
  
  
  
  int32_t CalculateConfidence(uint32_t hitCount, uint32_t hitsPossible,
                              uint32_t lastHit, uint32_t lastPossible,
                              int32_t globalDegradation);

  
  
  
  
  
  
  
  void CalculatePredictions(nsICacheEntry *entry, uint32_t lastLoad,
                            uint32_t loadCount, int32_t globalDegradation);

  
  
  
  void SetupPrediction(int32_t confidence, nsIURI *uri);

  
  
  
  bool RunPredictions(nsINetworkPredictorVerifier *verifier);

  
  
  
  
  
  
  
  
  
  bool WouldRedirect(nsICacheEntry *entry, uint32_t loadCount,
                     uint32_t lastLoad, int32_t globalDegradation,
                     nsIURI **redirectURI);

  

  
  
  
  
  
  
  
  
  
  
  
  void LearnInternal(PredictorLearnReason reason, nsICacheEntry *entry,
                     bool isNew, bool fullUri, nsIURI *targetURI,
                     nsIURI *sourceURI);

  
  
  
  void LearnForSubresource(nsICacheEntry *entry, nsIURI *targetURI);

  
  
  
  void LearnForRedirect(nsICacheEntry *entry, nsIURI *targetURI);

  
  
  
  
  
  void MaybeLearnForStartup(nsIURI *uri, bool fullUri);

  
  
  
  
  void LearnForStartup(nsICacheEntry *entry, nsIURI *targetURI);

  
  
  
  
  
  
  
  bool ParseMetaDataEntry(const char *key, const char *value, nsIURI **uri,
                          uint32_t &hitCount, uint32_t &lastHit,
                          uint32_t &flags);

  
  bool mInitialized;

  bool mEnabled;
  bool mEnableHoverOnSSL;

  int32_t mPageDegradationDay;
  int32_t mPageDegradationWeek;
  int32_t mPageDegradationMonth;
  int32_t mPageDegradationYear;
  int32_t mPageDegradationMax;

  int32_t mSubresourceDegradationDay;
  int32_t mSubresourceDegradationWeek;
  int32_t mSubresourceDegradationMonth;
  int32_t mSubresourceDegradationYear;
  int32_t mSubresourceDegradationMax;

  int32_t mPreconnectMinConfidence;
  int32_t mPreresolveMinConfidence;
  int32_t mRedirectLikelyConfidence;

  int32_t mMaxResourcesPerEntry;

  bool mCleanedUp;
  nsCOMPtr<nsITimer> mCleanupTimer;

  nsTArray<nsCString> mKeysToOperateOn;
  nsTArray<nsCString> mValuesToOperateOn;

  nsCOMPtr<nsICacheStorage> mCacheDiskStorage;

  nsCOMPtr<nsIIOService> mIOService;
  nsCOMPtr<nsISpeculativeConnect> mSpeculativeService;

  nsCOMPtr<nsIURI> mStartupURI;
  uint32_t mStartupTime;
  uint32_t mLastStartupTime;
  int32_t mStartupCount;

  nsCOMPtr<nsIDNSService> mDnsService;

  nsRefPtr<DNSListener> mDNSListener;

  nsTArray<nsCOMPtr<nsIURI>> mPreconnects;
  nsTArray<nsCOMPtr<nsIURI>> mPreresolves;

  static Predictor *sSelf;
};

} 
} 

#endif 
