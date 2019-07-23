





































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
  GetAllKeys(const nsAString& aDomain,
             nsDOMStorage* aStorage,
             nsTHashtable<nsSessionStorageEntry>* aKeys);

  




  nsresult
  GetKeyValue(const nsAString& aDomain,
              const nsAString& aKey,
              nsAString& aValue,
              PRBool* aSecure,
              nsAString& aOwner);

  


  nsresult
  SetKey(const nsAString& aDomain,
         const nsAString& aKey,
         const nsAString& aValue,
         PRBool aSecure,
         const nsAString& aOwner,
         PRInt32 aQuota,
         PRInt32* aNewUsage);

  



  nsresult
  SetSecure(const nsAString& aDomain,
            const nsAString& aKey,
            const PRBool aSecure);

  


  nsresult
  RemoveKey(const nsAString& aDomain,
            const nsAString& aKey,
            const nsAString& aOwner,
            PRInt32 aKeyUsage);

  


  nsresult
  RemoveOwner(const nsAString& aOwner);

  



  nsresult
  RemoveOwners(const nsTArray<nsString>& aOwners, PRBool aMatch);

  


  nsresult
  RemoveAll();

  nsresult GetUsage(const nsAString &aOwner, PRInt32 *aUsage);

protected:

  nsCOMPtr<mozIStorageConnection> mConnection;

  nsCOMPtr<mozIStorageStatement> mGetAllKeysStatement;
  nsCOMPtr<mozIStorageStatement> mGetKeyValueStatement;
  nsCOMPtr<mozIStorageStatement> mInsertKeyStatement;
  nsCOMPtr<mozIStorageStatement> mUpdateKeyStatement;
  nsCOMPtr<mozIStorageStatement> mSetSecureStatement;
  nsCOMPtr<mozIStorageStatement> mRemoveKeyStatement;
  nsCOMPtr<mozIStorageStatement> mRemoveOwnerStatement;
  nsCOMPtr<mozIStorageStatement> mRemoveAllStatement;
  nsCOMPtr<mozIStorageStatement> mGetUsageStatement;

  nsAutoString mCachedOwner;
  PRInt32 mCachedUsage;
};

#endif 
