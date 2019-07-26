




#ifndef LookupCache_h__
#define LookupCache_h__

#include "Entries.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIFile.h"
#include "nsIFileStreams.h"
#include "nsUrlClassifierPrefixSet.h"
#include "prlog.h"

namespace mozilla {
namespace safebrowsing {

#define MAX_HOST_COMPONENTS 5
#define MAX_PATH_COMPONENTS 4

class LookupResult {
public:
  LookupResult() : mComplete(false), mNoise(false), mFresh(false), mProtocolConfirmed(false) {}

  
  union {
    Prefix prefix;
    Completion complete;
  } hash;

  const Prefix &PrefixHash() { return hash.prefix; }
  const Completion &CompleteHash() { return hash.complete; }

  bool Confirmed() const { return (mComplete && mFresh) || mProtocolConfirmed; }
  bool Complete() const { return mComplete; }

  
  bool mComplete;

  
  
  bool mNoise;

  
  bool mFresh;

  bool mProtocolConfirmed;

  nsCString mTableName;
};

typedef nsTArray<LookupResult> LookupResultArray;

struct CacheResult {
  AddComplete entry;
  nsCString table;
};
typedef nsTArray<CacheResult> CacheResultArray;

class LookupCache {
public:
  
  static bool IsCanonicalizedIP(const nsACString& aHost);

  
  
  
  static nsresult GetLookupFragments(const nsACString& aSpec,
                                     nsTArray<nsCString>* aFragments);
  
  
  
  
  
  static nsresult GetHostKeys(const nsACString& aSpec,
                              nsTArray<nsCString>* aHostKeys);
  
  
  
  
  
  static nsresult GetKey(const nsACString& aSpec, Completion* aHash,
                         nsCOMPtr<nsICryptoHash>& aCryptoHash);

  LookupCache(const nsACString& aTableName, nsIFile* aStoreFile);
  ~LookupCache();

  const nsCString &TableName() const { return mTableName; }

  nsresult Init();
  nsresult Open();
  
  
  nsresult UpdateDirHandle(nsIFile* aStoreDirectory);
  
  nsresult Build(AddPrefixArray& aAddPrefixes,
                 AddCompleteArray& aAddCompletes);
  nsresult GetPrefixes(nsTArray<uint32_t>* aAddPrefixes);
  void ClearCompleteCache();

#if DEBUG && defined(PR_LOGGING)
  void Dump();
#endif
  nsresult WriteFile();
  nsresult Has(const Completion& aCompletion,
               bool* aHas, bool* aComplete);
  bool IsPrimed();

private:
  void ClearAll();
  nsresult Reset();
  void UpdateHeader();
  nsresult ReadHeader(nsIInputStream* aInputStream);
  nsresult ReadCompletions(nsIInputStream* aInputStream);
  nsresult EnsureSizeConsistent();
  nsresult LoadPrefixSet();
  
  
  nsresult ConstructPrefixSet(AddPrefixArray& aAddPrefixes);

  struct Header {
    uint32_t magic;
    uint32_t version;
    uint32_t numCompletions;
  };
  Header mHeader;

  bool mPrimed;
  nsCString mTableName;
  nsCOMPtr<nsIFile> mStoreDirectory;
  CompletionArray mCompletions;
  
  nsRefPtr<nsUrlClassifierPrefixSet> mPrefixSet;
};

}
}

#endif
