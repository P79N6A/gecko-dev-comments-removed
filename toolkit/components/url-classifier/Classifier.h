




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
  void Close();
  void Reset();

  



  void TableRequest(nsACString& aResult);

  


  nsresult ActiveTables(nsTArray<nsCString>& aTables);

  


  nsresult Check(const nsACString& aSpec, LookupResultArray& aResults);

  



  nsresult ApplyUpdates(nsTArray<TableUpdate*>* aUpdates);
  



  nsresult MarkSpoiled(nsTArray<nsCString>& aTables);
  nsresult CacheCompletions(const CacheResultArray& aResults);
  uint32_t GetHashKey(void) { return mHashKey; }
  void SetFreshTime(uint32_t aTime) { mFreshTime = aTime; }
  



  nsresult ReadNoiseEntries(const Prefix& aPrefix,
                            const nsACString& aTableName,
                            uint32_t aCount,
                            PrefixArray* aNoiseEntries);
private:
  void DropStores();
  nsresult CreateStoreDirectory();
  nsresult SetupPathNames();
  nsresult RecoverBackups();
  nsresult CleanToDelete();
  nsresult BackupTables();
  nsresult RemoveBackupTables();
  nsresult RegenActiveTables();
  nsresult ScanStoreDir(nsTArray<nsCString>& aTables);

  nsresult ApplyTableUpdates(nsTArray<TableUpdate*>* aUpdates,
                             const nsACString& aTable);

  LookupCache *GetLookupCache(const nsACString& aTable);

  
  nsCOMPtr<nsIFile> mCacheDirectory;
  
  nsCOMPtr<nsIFile> mStoreDirectory;
  
  nsCOMPtr<nsIFile> mBackupDirectory;
  nsCOMPtr<nsIFile> mToDeleteDirectory;
  nsCOMPtr<nsICryptoHash> mCryptoHash;
  nsTArray<HashStore*> mHashStores;
  nsTArray<LookupCache*> mLookupCaches;
  nsTArray<nsCString> mActiveTablesCache;
  uint32_t mHashKey;
  
  nsDataHashtable<nsCStringHashKey, int64_t> mTableFreshness;
  uint32_t mFreshTime;
};

}
}

#endif
