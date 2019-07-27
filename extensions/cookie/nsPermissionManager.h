




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
#include "nsHashKeys.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsDataHashtable.h"

class nsIPermission;
class nsIIDNService;
class mozIStorageConnection;
class mozIStorageAsyncStatement;



class nsPermissionManager MOZ_FINAL : public nsIPermissionManager,
                                      public nsIObserver,
                                      public nsSupportsWeakReference
{
public:
  class PermissionEntry
  {
  public:
    PermissionEntry(int64_t aID, uint32_t aType, uint32_t aPermission,
                    uint32_t aExpireType, int64_t aExpireTime)
     : mID(aID)
     , mType(aType)
     , mPermission(aPermission)
     , mExpireType(aExpireType)
     , mExpireTime(aExpireTime)
     , mNonSessionPermission(aPermission)
     , mNonSessionExpireType(aExpireType)
     , mNonSessionExpireTime(aExpireTime)
    {}

    int64_t  mID;
    uint32_t mType;
    uint32_t mPermission;
    uint32_t mExpireType;
    int64_t  mExpireTime;
    uint32_t mNonSessionPermission;
    uint32_t mNonSessionExpireType;
    uint32_t mNonSessionExpireTime;
  };

  





  class PermissionKey
  {
  public:
    PermissionKey(nsIPrincipal* aPrincipal);
    PermissionKey(const nsACString& aHost,
                  uint32_t aAppId,
                  bool aIsInBrowserElement)
      : mHost(aHost)
      , mAppId(aAppId)
      , mIsInBrowserElement(aIsInBrowserElement)
    {
    }

    bool operator==(const PermissionKey& aKey) const {
      return mHost.Equals(aKey.mHost) &&
             mAppId == aKey.mAppId &&
             mIsInBrowserElement == aKey.mIsInBrowserElement;
    }

    PLDHashNumber GetHashCode() const {
      nsAutoCString str;
      str.Assign(mHost);
      str.AppendInt(mAppId);
      str.AppendInt(static_cast<int32_t>(mIsInBrowserElement));

      return mozilla::HashString(str);
    }

    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(PermissionKey)

    nsCString mHost;
    uint32_t  mAppId;
    bool      mIsInBrowserElement;

  private:
    
    PermissionKey() MOZ_DELETE;

    
    ~PermissionKey() {};
  };

  class PermissionHashKey : public nsRefPtrHashKey<PermissionKey>
  {
  public:
    PermissionHashKey(const PermissionKey* aPermissionKey)
      : nsRefPtrHashKey<PermissionKey>(aPermissionKey)
    {}

    PermissionHashKey(const PermissionHashKey& toCopy)
      : nsRefPtrHashKey<PermissionKey>(toCopy)
      , mPermissions(toCopy.mPermissions)
    {}

    bool KeyEquals(const PermissionKey* aKey) const
    {
      return *aKey == *GetKey();
    }

    static PLDHashNumber HashKey(const PermissionKey* aKey)
    {
      return aKey->GetHashCode();
    }

    
    
    enum { ALLOW_MEMMOVE = false };

    inline nsTArray<PermissionEntry> & GetPermissions()
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

    inline PermissionEntry GetPermission(uint32_t aType) const
    {
      for (uint32_t i = 0; i < mPermissions.Length(); ++i)
        if (mPermissions[i].mType == aType)
          return mPermissions[i];

      
      return PermissionEntry(-1, aType, nsIPermissionManager::UNKNOWN_ACTION,
                             nsIPermissionManager::EXPIRE_NEVER, 0);
    }

  private:
    nsAutoTArray<PermissionEntry, 1> mPermissions;
  };

  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPERMISSIONMANAGER
  NS_DECL_NSIOBSERVER

  nsPermissionManager();
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

  





  static void AppClearDataObserverInit();

private:
  virtual ~nsPermissionManager();

  int32_t GetTypeIndex(const char *aTypeString,
                       bool        aAdd);

  PermissionHashKey* GetPermissionHashKey(const nsACString& aHost,
                                          uint32_t aAppId,
                                          bool aIsInBrowserElement,
                                          uint32_t          aType,
                                          bool              aExactHostMatch);

  nsresult CommonTestPermission(nsIPrincipal* aPrincipal,
                                const char *aType,
                                uint32_t   *aPermission,
                                bool        aExactHostMatch,
                                bool        aIncludingSession);

  nsresult InitDB(bool aRemoveFile);
  nsresult CreateTable();
  nsresult Import();
  nsresult Read();
  void     NotifyObserversWithPermission(const nsACString &aHost,
                                         uint32_t          aAppId,
                                         bool              aIsInBrowserElement,
                                         const nsCString  &aType,
                                         uint32_t          aPermission,
                                         uint32_t          aExpireType,
                                         int64_t           aExpireTime,
                                         const char16_t  *aData);
  void     NotifyObservers(nsIPermission *aPermission, const char16_t *aData);

  
  
  void     CloseDB(bool aRebuildOnSuccess = false);

  nsresult RemoveAllInternal(bool aNotifyObservers);
  nsresult RemoveAllFromMemory();
  nsresult NormalizeToACE(nsCString &aHost);
  static void UpdateDB(OperationType aOp,
                       mozIStorageAsyncStatement* aStmt,
                       int64_t aID,
                       const nsACString& aHost,
                       const nsACString& aType,
                       uint32_t aPermission,
                       uint32_t aExpireType,
                       int64_t aExpireTime,
                       uint32_t aAppId,
                       bool aIsInBrowserElement);

  nsresult RemoveExpiredPermissionsForApp(uint32_t aAppId);

  






  struct GetPermissionsForAppStruct {
    uint32_t                  appId;
    bool                      browserOnly;
    nsCOMArray<nsIPermission> permissions;

    GetPermissionsForAppStruct() MOZ_DELETE;
    GetPermissionsForAppStruct(uint32_t aAppId, bool aBrowserOnly)
      : appId(aAppId)
      , browserOnly(aBrowserOnly)
    {}
  };

  




  static PLDHashOperator
  GetPermissionsForApp(PermissionHashKey* entry, void* arg);

  


  static PLDHashOperator
  RemoveExpiredPermissionsForAppEnumerator(PermissionHashKey* entry,
                                           void* nonused);

  nsCOMPtr<nsIObserverService> mObserverService;
  nsCOMPtr<nsIIDNService>      mIDNService;

  nsCOMPtr<mozIStorageConnection> mDBConn;
  nsCOMPtr<mozIStorageAsyncStatement> mStmtInsert;
  nsCOMPtr<mozIStorageAsyncStatement> mStmtDelete;
  nsCOMPtr<mozIStorageAsyncStatement> mStmtUpdate;

  nsTHashtable<PermissionHashKey> mPermissionTable;
  
  int64_t                      mLargestID;

  
  nsTArray<nsCString>          mTypeArray;

  
  struct ApplicationCounter {
    uint32_t mAppId;
    uint32_t mCounter;
  };
  nsTArray<ApplicationCounter> mAppIdRefcounts;

  
  
  bool mIsShuttingDown;

  friend class DeleteFromMozHostListener;
  friend class CloseDatabaseListener;
};


#define NS_PERMISSIONMANAGER_CID \
{ 0x4f6b5e00, 0xc36, 0x11d5, { 0xa5, 0x35, 0x0, 0x10, 0xa4, 0x1, 0xeb, 0x10 } }

#endif
