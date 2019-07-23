






































#ifndef nsPermissionManager_h__
#define nsPermissionManager_h__

#include "nsIPermissionManager.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsWeakReference.h"
#include "nsCOMPtr.h"
#include "nsIFile.h"
#include "nsTHashtable.h"
#include "nsTArray.h"
#include "nsString.h"

class nsIPermission;
class nsIIDNService;
class mozIStorageConnection;
class mozIStorageStatement;



class nsPermissionEntry
{
public:
  nsPermissionEntry(PRUint32 aType, PRUint32 aPermission, PRInt64 aID, 
                    PRUint32 aExpireType, PRInt64 aExpireTime)
   : mType(aType)
   , mPermission(aPermission)
   , mID(aID)
   , mExpireType(aExpireType)
   , mExpireTime(aExpireTime) {}

  PRUint32 mType;
  PRUint32 mPermission;
  PRInt64  mID;
  PRUint32 mExpireType;
  PRInt64  mExpireTime;
};

class nsHostEntry : public PLDHashEntryHdr
{
public:
  
  typedef const char* KeyType;
  typedef const char* KeyTypePointer;

  nsHostEntry(const char* aHost);
  nsHostEntry(const nsHostEntry& toCopy);

  ~nsHostEntry()
  {
  }

  KeyType GetKey() const
  {
    return mHost;
  }

  PRBool KeyEquals(KeyTypePointer aKey) const
  {
    return !strcmp(mHost, aKey);
  }

  static KeyTypePointer KeyToPointer(KeyType aKey)
  {
    return aKey;
  }

  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    
    
    return PL_DHashStringKey(nsnull, aKey);
  }

  
  
  enum { ALLOW_MEMMOVE = PR_FALSE };

  
  inline const nsDependentCString GetHost() const
  {
    return nsDependentCString(mHost);
  }

  inline nsTArray<nsPermissionEntry> & GetPermissions()
  {
    return mPermissions;
  }

  inline PRInt32 GetPermissionIndex(PRUint32 aType) const
  {
    for (PRUint32 i = 0; i < mPermissions.Length(); ++i)
      if (mPermissions[i].mType == aType)
        return i;

    return -1;
  }

  inline nsPermissionEntry GetPermission(PRUint32 aType) const
  {
    for (PRUint32 i = 0; i < mPermissions.Length(); ++i)
      if (mPermissions[i].mType == aType)
        return mPermissions[i];

    
    nsPermissionEntry unk = nsPermissionEntry(aType, nsIPermissionManager::UNKNOWN_ACTION,
                                              -1, nsIPermissionManager::EXPIRE_NEVER, 0);
    return unk;
  }

private:
  const char *mHost;
  nsAutoTArray<nsPermissionEntry, 1> mPermissions;
};


class nsPermissionManager : public nsIPermissionManager,
                            public nsIObserver,
                            public nsSupportsWeakReference
{
public:

  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPERMISSIONMANAGER
  NS_DECL_NSIOBSERVER

  nsPermissionManager();
  virtual ~nsPermissionManager();
  nsresult Init();

private:

  
  enum OperationType {
    eOperationNone,
    eOperationAdding,
    eOperationRemoving,
    eOperationChanging
  };

  enum DBOperationType {
    eNoDBOperation,
    eWriteToDB
  };

  enum NotifyOperationType {
    eDontNotify,
    eNotify
  };

  nsresult AddInternal(const nsAFlatCString &aHost,
                       const nsAFlatCString &aType,
                       PRUint32 aPermission,
                       PRInt64 aID,
                       PRUint32 aExpireType,
                       PRInt64  aExpireTime,
                       NotifyOperationType aNotifyOperation,
                       DBOperationType aDBOperation);

  PRInt32 GetTypeIndex(const char *aTypeString,
                       PRBool      aAdd);

  nsHostEntry *GetHostEntry(const nsAFlatCString &aHost,
                            PRUint32              aType,
                            PRBool                aExactHostMatch);

  nsresult CommonTestPermission(nsIURI     *aURI,
                                const char *aType,
                                PRUint32   *aPermission,
                                PRBool      aExactHostMatch);

  nsresult InitDB(PRBool aRemoveFile);
  nsresult CreateTable();
  nsresult Import();
  nsresult Read();
  void     NotifyObserversWithPermission(const nsACString &aHost,
                                         const nsCString  &aType,
                                         PRUint32          aPermission,
                                         PRUint32          aExpireType,
                                         PRInt64           aExpireTime,
                                         const PRUnichar  *aData);
  void     NotifyObservers(nsIPermission *aPermission, const PRUnichar *aData);
  nsresult RemoveAllInternal();
  nsresult RemoveAllFromMemory();
  nsresult NormalizeToACE(nsCString &aHost);
  nsresult GetHost(nsIURI *aURI, nsACString &aResult);
  static void UpdateDB(OperationType         aOp,
                       mozIStorageStatement* aStmt,
                       PRInt64               aID,
                       const nsACString     &aHost,
                       const nsACString     &aType,
                       PRUint32              aPermission,
                       PRUint32              aExpireType,
                       PRInt64               aExpireTime);

  nsCOMPtr<nsIObserverService> mObserverService;
  nsCOMPtr<nsIIDNService>      mIDNService;

  nsCOMPtr<mozIStorageConnection> mDBConn;
  nsCOMPtr<mozIStorageStatement> mStmtInsert;
  nsCOMPtr<mozIStorageStatement> mStmtDelete;
  nsCOMPtr<mozIStorageStatement> mStmtUpdate;

  nsTHashtable<nsHostEntry>    mHostTable;
  
  PRInt64                      mLargestID;

  
  nsTArray<nsCString>          mTypeArray;
};


#define NS_PERMISSIONMANAGER_CID \
{ 0x4f6b5e00, 0xc36, 0x11d5, { 0xa5, 0x35, 0x0, 0x10, 0xa4, 0x1, 0xeb, 0x10 } }

#endif
