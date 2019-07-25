





































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

  nsresult
  Init();

  


  nsresult
  GetAllKeys(DOMStorageImpl* aStorage,
             nsTHashtable<nsSessionStorageEntry>* aKeys);

  




  nsresult
  GetKeyValue(DOMStorageImpl* aStorage,
              const nsAString& aKey,
              nsAString& aValue,
              PRBool* aSecure);

  


  nsresult
  SetKey(DOMStorageImpl* aStorage,
         const nsAString& aKey,
         const nsAString& aValue,
         PRBool aSecure,
         PRInt32 aQuota,
         PRBool aExcludeOfflineFromUsage,
         PRInt32* aNewUsage);

  



  nsresult
  SetSecure(DOMStorageImpl* aStorage,
            const nsAString& aKey,
            const PRBool aSecure);

  


  nsresult
  RemoveKey(DOMStorageImpl* aStorage,
            const nsAString& aKey,
            PRBool aExcludeOfflineFromUsage,
            PRInt32 aKeyUsage);

  


  nsresult
  ClearStorage(DOMStorageImpl* aStorage);

  


  nsresult
  DropSessionOnlyStoragesForHost(const nsACString& aHostName);

  


  nsresult
  DropPrivateBrowsingStorages();

  


  nsresult
  RemoveOwner(const nsACString& aOwner, PRBool aIncludeSubDomains);

  



  nsresult
  RemoveOwners(const nsTArray<nsString>& aOwners,
               PRBool aIncludeSubDomains, PRBool aMatch);

  


  nsresult
  RemoveAll();

  


  nsresult
  GetUsage(DOMStorageImpl* aStorage, PRBool aExcludeOfflineFromUsage, PRInt32 *aUsage);

  


  nsresult
  GetUsage(const nsACString& aDomain, PRBool aIncludeSubDomains, PRInt32 *aUsage);

  








  void
  MarkScopeCached(DOMStorageImpl* aStorage);

  



  bool
  IsScopeDirty(DOMStorageImpl* aStorage);

  




  static nsresult CreateOriginScopeDBKey(nsIURI* aUri, nsACString& aKey);

  



  static nsresult CreateDomainScopeDBKey(nsIURI* aUri, nsACString& aKey);
  static nsresult CreateDomainScopeDBKey(const nsACString& aAsciiDomain, nsACString& aKey);

  




  static nsresult CreateQuotaDomainDBKey(const nsACString& aAsciiDomain,
                                         PRBool aIncludeSubDomains, PRBool aETLDplus1Only,
                                         nsACString& aKey);

  static nsresult GetDomainFromScopeKey(const nsACString& aScope,
                                         nsACString& aDomain);

  



  void EnsureTempTableFlushTimer();

  





  nsresult FlushAndDeleteTemporaryTables(bool force);

  


  void StopTempTableFlushTimer();

protected:
  nsDOMStoragePersistentDB mChromePersistentDB;
  nsDOMStoragePersistentDB mPersistentDB;
  nsDOMStorageMemoryDB mSessionOnlyDB;
  nsDOMStorageMemoryDB mPrivateBrowsingDB;

  nsCOMPtr<nsITimer> mTempTableFlushTimer;
};

#endif 
