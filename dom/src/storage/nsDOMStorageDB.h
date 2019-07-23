





































#ifndef nsDOMStorageDB_h___
#define nsDOMStorageDB_h___

#include "nscore.h"
#include "mozIStorageConnection.h"
#include "mozIStorageStatement.h"
#include "nsTHashtable.h"

class nsDOMStorage;
class nsSessionStorageEntry;


































class nsDOMStorageDB
{
public:
  nsDOMStorageDB() {}
  ~nsDOMStorageDB() {}

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

  


  nsresult ClearStorage(nsDOMStorage* aStorage);

  


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

  nsCOMPtr<mozIStorageConnection> mConnection;

  nsCOMPtr<mozIStorageStatement> mGetAllKeysStatement;
  nsCOMPtr<mozIStorageStatement> mGetKeyValueStatement;
  nsCOMPtr<mozIStorageStatement> mInsertKeyStatement;
  nsCOMPtr<mozIStorageStatement> mUpdateKeyStatement;
  nsCOMPtr<mozIStorageStatement> mSetSecureStatement;
  nsCOMPtr<mozIStorageStatement> mRemoveKeyStatement;
  nsCOMPtr<mozIStorageStatement> mRemoveOwnerStatement;
  nsCOMPtr<mozIStorageStatement> mRemoveStorageStatement;
  nsCOMPtr<mozIStorageStatement> mRemoveAllStatement;
  nsCOMPtr<mozIStorageStatement> mGetUsageStatement;

  nsCString mCachedOwner;
  PRInt32 mCachedUsage;

  nsresult
  GetUsageInternal(const nsACString& aQuotaDomainDBKey, PRInt32 *aUsage);
};

#endif 
