




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
#include "nsPermission.h"

class nsIPermission;
class nsIIDNService;
class mozIStorageConnection;
class mozIStorageStatement;



class nsPermissionEntry
{
public:
  nsPermissionEntry(uint32_t aType, uint32_t aPermission, int64_t aID, 
                    uint32_t aExpireType, int64_t aExpireTime)
   : mType(aType)
   , mPermission(aPermission)
   , mID(aID)
   , mExpireType(aExpireType)
   , mExpireTime(aExpireTime) {}

  uint32_t mType;
  uint32_t mPermission;
  int64_t  mID;
  uint32_t mExpireType;
  int64_t  mExpireTime;
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

  bool KeyEquals(KeyTypePointer aKey) const
  {
    return !strcmp(mHost, aKey);
  }

  static KeyTypePointer KeyToPointer(KeyType aKey)
  {
    return aKey;
  }

  static PLDHashNumber HashKey(KeyTypePointer aKey)
  {
    
    
    return PL_DHashStringKey(nullptr, aKey);
  }

  
  
  enum { ALLOW_MEMMOVE = false };

  
  inline const nsDependentCString GetHost() const
  {
    return nsDependentCString(mHost);
  }

  inline nsTArray<nsPermissionEntry> & GetPermissions()
  {
    return mPermissions;
  }

  inline int32_t GetPermissionIndex(uint32_t aType) const
  {
    for (uint32_t i = 0; i < mPermissions.Length(); ++i)
      if (mPermissions[i].mType == aType)
        return i;

    return -1;
  }

  inline nsPermissionEntry GetPermission(uint32_t aType) const
  {
    for (uint32_t i = 0; i < mPermissions.Length(); ++i)
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
  static nsIPermissionManager* GetXPCOMSingleton();
  nsresult Init();

  
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

  nsresult AddInternal(nsIPrincipal* aPrincipal,
                       const nsAFlatCString &aType,
                       uint32_t aPermission,
                       int64_t aID,
                       uint32_t aExpireType,
                       int64_t  aExpireTime,
                       NotifyOperationType aNotifyOperation,
                       DBOperationType aDBOperation);

private:

  int32_t GetTypeIndex(const char *aTypeString,
                       bool        aAdd);

  nsHostEntry *GetHostEntry(const nsAFlatCString &aHost,
                            uint32_t              aType,
                            bool                  aExactHostMatch);

  nsresult CommonTestPermission(nsIPrincipal* aPrincipal,
                                const char *aType,
                                uint32_t   *aPermission,
                                bool        aExactHostMatch);

  nsresult InitDB(bool aRemoveFile);
  nsresult CreateTable();
  nsresult Import();
  nsresult Read();
  void     NotifyObserversWithPermission(const nsACString &aHost,
                                         const nsCString  &aType,
                                         uint32_t          aPermission,
                                         uint32_t          aExpireType,
                                         int64_t           aExpireTime,
                                         const PRUnichar  *aData);
  void     NotifyObservers(nsIPermission *aPermission, const PRUnichar *aData);

  
  
  void     CloseDB(bool aRebuildOnSuccess = false);

  nsresult RemoveAllInternal(bool aNotifyObservers);
  nsresult RemoveAllFromMemory();
  nsresult NormalizeToACE(nsCString &aHost);
  nsresult GetHost(nsIURI *aURI, nsACString &aResult);
  static void UpdateDB(OperationType         aOp,
                       mozIStorageStatement* aStmt,
                       int64_t               aID,
                       const nsACString     &aHost,
                       const nsACString     &aType,
                       uint32_t              aPermission,
                       uint32_t              aExpireType,
                       int64_t               aExpireTime);

  nsCOMPtr<nsIObserverService> mObserverService;
  nsCOMPtr<nsIIDNService>      mIDNService;

  nsCOMPtr<mozIStorageConnection> mDBConn;
  nsCOMPtr<mozIStorageStatement> mStmtInsert;
  nsCOMPtr<mozIStorageStatement> mStmtDelete;
  nsCOMPtr<mozIStorageStatement> mStmtUpdate;

  nsTHashtable<nsHostEntry>    mHostTable;
  
  int64_t                      mLargestID;

  
  nsTArray<nsCString>          mTypeArray;

  
  
  bool mIsShuttingDown;

  friend class DeleteFromMozHostListener;
  friend class CloseDatabaseListener;
};


#define NS_PERMISSIONMANAGER_CID \
{ 0x4f6b5e00, 0xc36, 0x11d5, { 0xa5, 0x35, 0x0, 0x10, 0xa4, 0x1, 0xeb, 0x10 } }

#endif 
