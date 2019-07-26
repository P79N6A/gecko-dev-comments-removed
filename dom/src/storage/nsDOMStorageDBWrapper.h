




#ifndef nsDOMStorageDB_h___
#define nsDOMStorageDB_h___

#include "nscore.h"
#include "nsTHashtable.h"

#include "nsDOMStoragePersistentDB.h"
#include "nsDOMStorageMemoryDB.h"

extern void ReverseString(const nsCSubstring& source, nsCSubstring& result);

class nsDOMStorage;
class nsSessionStorageEntry;


































class nsDOMStorageDBWrapper
{
public:
  nsDOMStorageDBWrapper();
  ~nsDOMStorageDBWrapper();

  


  void Close();

  nsresult
  Init();

  


  nsresult
  GetAllKeys(DOMStorageImpl* aStorage,
             nsTHashtable<nsSessionStorageEntry>* aKeys);

  




  nsresult
  GetKeyValue(DOMStorageImpl* aStorage,
              const nsAString& aKey,
              nsAString& aValue,
              bool* aSecure);

  


  nsresult
  SetKey(DOMStorageImpl* aStorage,
         const nsAString& aKey,
         const nsAString& aValue,
         bool aSecure);

  



  nsresult
  SetSecure(DOMStorageImpl* aStorage,
            const nsAString& aKey,
            const bool aSecure);

  


  nsresult
  RemoveKey(DOMStorageImpl* aStorage,
            const nsAString& aKey);

  


  nsresult
  ClearStorage(DOMStorageImpl* aStorage);

  


  nsresult
  DropSessionOnlyStoragesForHost(const nsACString& aHostName);

  


  nsresult
  DropPrivateBrowsingStorages();

  


  nsresult
  RemoveOwner(const nsACString& aOwner);

  


  nsresult
  RemoveAll();

  


  nsresult
  GetUsage(DOMStorageImpl* aStorage, int32_t *aUsage);

  


  nsresult
  GetUsage(const nsACString& aDomain, int32_t *aUsage, bool aPrivate);

  








  void
  MarkScopeCached(DOMStorageImpl* aStorage);

  



  bool
  IsScopeDirty(DOMStorageImpl* aStorage);

  




  static nsresult CreateScopeDBKey(nsIURI* aUri, nsACString& aKey);

  



  static nsresult CreateReversedDomain(nsIURI* aUri, nsACString& aKey);
  static nsresult CreateReversedDomain(const nsACString& aAsciiDomain, nsACString& aKey);

  




  static nsresult CreateQuotaDBKey(const nsACString& aAsciiDomain,
                                   nsACString& aKey);

  static nsresult GetDomainFromScopeKey(const nsACString& aScope,
                                        nsACString& aDomain);

  



  void EnsureTempTableFlushTimer();

  





  nsresult FlushAndDeleteTemporaryTables(bool force);

  


  void StopTempTableFlushTimer();

protected:
  nsDOMStoragePersistentDB mPersistentDB;
  nsDOMStorageMemoryDB mSessionOnlyDB;
  nsDOMStorageMemoryDB mPrivateBrowsingDB;

  nsCOMPtr<nsITimer> mTempTableFlushTimer;
};

#endif 
