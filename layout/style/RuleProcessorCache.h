









#ifndef mozilla_RuleProcessorCache_h
#define mozilla_RuleProcessorCache_h

#include "mozilla/MemoryReporting.h"
#include "mozilla/StaticPtr.h"
#include "nsCSSRuleProcessor.h"
#include "nsExpirationTracker.h"
#include "nsIMediaList.h"
#include "nsIMemoryReporter.h"
#include "nsTArray.h"

class nsCSSRuleProcessor;
namespace mozilla {
class CSSStyleSheet;
namespace css {
class DocumentRule;
}
}

namespace mozilla {











class RuleProcessorCache final : public nsIMemoryReporter
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMEMORYREPORTER

public:
  static nsCSSRuleProcessor* GetRuleProcessor(
      const nsTArray<CSSStyleSheet*>& aSheets,
      nsPresContext* aPresContext);
  static void PutRuleProcessor(
      const nsTArray<CSSStyleSheet*>& aSheets,
      nsTArray<css::DocumentRule*>&& aDocumentRulesInSheets,
      const nsDocumentRuleResultCacheKey& aCacheKey,
      nsCSSRuleProcessor* aRuleProcessor);
  static void StartTracking(nsCSSRuleProcessor* aRuleProcessor);
  static void StopTracking(nsCSSRuleProcessor* aRuleProcessor);

#ifdef DEBUG
  static bool HasRuleProcessor(nsCSSRuleProcessor* aRuleProcessor);
#endif
  static void RemoveRuleProcessor(nsCSSRuleProcessor* aRuleProcessor);
  static void RemoveSheet(CSSStyleSheet* aSheet);

  static void Shutdown() { gShutdown = true; gRuleProcessorCache = nullptr; }

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf);

private:
  class ExpirationTracker : public nsExpirationTracker<nsCSSRuleProcessor,3>
  {
  public:
    explicit ExpirationTracker(RuleProcessorCache* aCache)
      : nsExpirationTracker<nsCSSRuleProcessor,3>(10000)
      , mCache(aCache) {}

    void RemoveObjectIfTracked(nsCSSRuleProcessor* aRuleProcessor);

    virtual void NotifyExpired(nsCSSRuleProcessor* aRuleProcessor) override {
      mCache->RemoveRuleProcessor(aRuleProcessor);
    }

  private:
    RuleProcessorCache* mCache;
  };

  RuleProcessorCache() : mExpirationTracker(this) {}
  ~RuleProcessorCache();

  void InitMemoryReporter();

  static bool EnsureGlobal();
  static StaticRefPtr<RuleProcessorCache> gRuleProcessorCache;
  static bool gShutdown;

  void DoRemoveSheet(CSSStyleSheet* aSheet);
  nsCSSRuleProcessor* DoGetRuleProcessor(
      const nsTArray<CSSStyleSheet*>& aSheets,
      nsPresContext* aPresContext);
  void DoPutRuleProcessor(const nsTArray<CSSStyleSheet*>& aSheets,
                          nsTArray<css::DocumentRule*>&& aDocumentRulesInSheets,
                          const nsDocumentRuleResultCacheKey& aCacheKey,
                          nsCSSRuleProcessor* aRuleProcessor);
#ifdef DEBUG
  bool DoHasRuleProcessor(nsCSSRuleProcessor* aRuleProcessor);
#endif
  void DoRemoveRuleProcessor(nsCSSRuleProcessor* aRuleProcessor);
  void DoStartTracking(nsCSSRuleProcessor* aRuleProcessor);
  void DoStopTracking(nsCSSRuleProcessor* aRuleProcessor);

  struct DocumentEntry {
    nsDocumentRuleResultCacheKey mCacheKey;
    nsRefPtr<nsCSSRuleProcessor> mRuleProcessor;
  };

  struct Entry {
    nsTArray<CSSStyleSheet*>     mSheets;
    nsTArray<css::DocumentRule*> mDocumentRulesInSheets;
    nsTArray<DocumentEntry>      mDocumentEntries;
  };

  
  
  
  struct HasSheet_ThenRemoveRuleProcessors {
    HasSheet_ThenRemoveRuleProcessors(RuleProcessorCache* aCache,
                                      CSSStyleSheet* aSheet)
      : mCache(aCache), mSheet(aSheet) {}
    bool operator()(Entry& aEntry) {
      if (aEntry.mSheets.Contains(mSheet)) {
        for (DocumentEntry& de : aEntry.mDocumentEntries) {
          de.mRuleProcessor->SetInRuleProcessorCache(false);
          mCache->mExpirationTracker.RemoveObjectIfTracked(de.mRuleProcessor);
        }
        return true;
      }
      return false;
    }
    RuleProcessorCache* mCache;
    CSSStyleSheet* mSheet;
  };

  ExpirationTracker mExpirationTracker;
  nsTArray<Entry> mEntries;
};

} 

#endif 
