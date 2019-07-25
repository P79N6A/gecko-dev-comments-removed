




#ifndef Classifier_h__
#define Classifier_h__

#include "Entries.h"
#include "HashStore.h"
#include "ProtocolParser.h"
#include "LookupCache.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIFile.h"
#include "nsICryptoHash.h"
#include "nsDataHashtable.h"

namespace mozilla {
namespace safebrowsing {




class Classifier {
public:
  Classifier();
  ~Classifier();

  nsresult Open(nsIFile& aCacheDirectory);
  nsresult Close();
  nsresult Reset();

  



  void TableRequest(nsACString& aResult);

  


  nsresult ActiveTables(nsTArray<nsCString>& aTables);

  


  nsresult Check(const nsACString& aSpec, LookupResultArray& aResults);

  



  nsresult ApplyUpdates(nsTArray<TableUpdate*>* aUpdates);
  



  nsresult MarkSpoiled(nsTArray<nsCString>& aTables);
  nsresult CacheCompletions(const CacheResultArray& aResults);
  PRUint32 GetHashKey(void) { return mHashKey; };
  void SetFreshTime(PRUint32 aTime) { mFreshTime = aTime; };
  void SetPerClientRandomize(bool aRandomize) { mPerClientRandomize = aRandomize; };
  



  nsresult ReadNoiseEntries(const Prefix& aPrefix,
                            const nsACString& aTableName,
                            PRInt32 aCount,
                            PrefixArray* aNoiseEntries);
private:
  void DropStores();
  nsresult RegenActiveTables();
  nsresult ScanStoreDir(nsTArray<nsCString>& aTables);

  nsresult ApplyTableUpdates(nsTArray<TableUpdate*>* aUpdates,
                             const nsACString& aTable);

  LookupCache *GetLookupCache(const nsACString& aTable);
  nsresult InitKey();

  nsCOMPtr<nsICryptoHash> mCryptoHash;
  nsCOMPtr<nsIFile> mStoreDirectory;
  nsTArray<HashStore*> mHashStores;
  nsTArray<LookupCache*> mLookupCaches;
  nsTArray<nsCString> mActiveTablesCache;
  PRUint32 mHashKey;
  
  nsDataHashtable<nsCStringHashKey, PRInt64> mTableFreshness;
  PRUint32 mFreshTime;
  bool mPerClientRandomize;
};

}
}

#endif
