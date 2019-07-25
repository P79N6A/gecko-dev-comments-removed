






































#ifndef LookupCache_h__
#define LookupCache_h__

#include "Entries.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIFile.h"
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

  
  Prefix mCodedPrefix;

  
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

  





  static nsresult KeyedHash(PRUint32 aPref, PRUint32 aDomain,
                            PRUint32 aKey, PRUint32* aOut,
                            bool aPassthrough);

  LookupCache(const nsACString& aTableName, nsIFile* aStoreFile,
              bool aPerClientRandomize);
  ~LookupCache();

  const nsCString &TableName() const { return mTableName; }

  nsresult Init();
  nsresult Open();
  
  nsresult Build(AddPrefixArray& aAddPrefixes,
                 AddCompleteArray& aAddCompletes);
  nsresult GetPrefixes(nsTArray<PRUint32>* aAddPrefixes);

#if DEBUG && defined(PR_LOGGING)
  void Dump();
#endif
  nsresult WriteFile();
  nsresult Has(const Completion& aCompletion,
               const Completion& aHostkey,
               PRUint32 aHashKey,
               bool* aHas, bool* aComplete,
               Prefix* aOrigPrefix);
  bool IsPrimed();

private:

  void Clear();
  nsresult Reset();
  void UpdateHeader();
  nsresult ReadHeader();
  nsresult EnsureSizeConsistent();
  nsresult ReadCompletions();
  nsresult LoadPrefixSet();
  
  
  nsresult ConstructPrefixSet(AddPrefixArray& aAddPrefixes);

  struct Header {
    uint32 magic;
    uint32 version;
    uint32 numCompletions;
  };
  Header mHeader;

  bool mPrimed;
  bool mPerClientRandomize;
  nsCString mTableName;
  nsCOMPtr<nsIFile> mStoreDirectory;
  nsCOMPtr<nsIInputStream> mInputStream;
  CompletionArray mCompletions;
  
  nsRefPtr<nsUrlClassifierPrefixSet> mPrefixSet;
};

}
}

#endif
