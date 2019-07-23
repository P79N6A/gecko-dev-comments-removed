





































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
  nsDOMStorageDBWrapper() {}
  ~nsDOMStorageDBWrapper() {}

  nsresult
  Init();

  


  nsresult
  GetAllKeys(nsDOMStorage* aStorage,
             nsTHashtable<nsSessionStorageEntry>* aKeys);

  




  nsresult
  GetKeyValue(nsDOMStorage* aStorage,
              const nsAString& aKey,
              nsAString& aValue,
              PRBool* aSecure);

  


  nsresult
  SetKey(nsDOMStorage* aStorage,
         const nsAString& aKey,
         const nsAString& aValue,
         PRBool aSecure,
         PRInt32 aQuota,
         PRInt32* aNewUsage);

  



  nsresult
  SetSecure(nsDOMStorage* aStorage,
            const nsAString& aKey,
            const PRBool aSecure);

  


  nsresult
  RemoveKey(nsDOMStorage* aStorage,
            const nsAString& aKey,
            PRInt32 aKeyUsage);

  


  nsresult
  ClearStorage(nsDOMStorage* aStorage);

  


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
  GetUsage(nsDOMStorage* aStorage, PRInt32 *aUsage);

  


  nsresult
  GetUsage(const nsACString& aDomain, PRBool aIncludeSubDomains, PRInt32 *aUsage);

  




  static nsresult CreateOriginScopeDBKey(nsIURI* aUri, nsACString& aKey);

  



  static nsresult CreateDomainScopeDBKey(nsIURI* aUri, nsACString& aKey);
  static nsresult CreateDomainScopeDBKey(const nsACString& aAsciiDomain, nsACString& aKey);

  




  static nsresult CreateQuotaDomainDBKey(const nsACString& aAsciiDomain,
                                         PRBool aIncludeSubDomains, nsACString& aKey);

protected:
  nsDOMStoragePersistentDB mPersistentDB;
  nsDOMStorageMemoryDB mSessionOnlyDB;
  nsDOMStorageMemoryDB mPrivateBrowsingDB;
};

#endif 
