







































#ifndef nsDOMStorage_h___
#define nsDOMStorage_h___

#include "nscore.h"
#include "nsAutoPtr.h"
#include "nsIDOMStorageObsolete.h"
#include "nsIDOMStorage.h"
#include "nsIDOMStorageList.h"
#include "nsIDOMStorageItem.h"
#include "nsIPermissionManager.h"
#include "nsInterfaceHashtable.h"
#include "nsVoidArray.h"
#include "nsTArray.h"
#include "nsPIDOMStorage.h"
#include "nsIDOMToString.h"
#include "nsDOMEvent.h"
#include "nsIDOMStorageEvent.h"
#include "nsIDOMStorageEventObsolete.h"
#include "nsIDOMStorageManager.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIObserver.h"
#include "nsITimer.h"
#include "nsWeakReference.h"
#include "mozilla/TimeStamp.h"

#define NS_DOMSTORAGE_FLUSH_TIMER_OBSERVER "domstorage-flush-timer"
#define NS_DOMSTORAGE_CUTOFF_OBSERVER "domstorage-cutoff"

#ifdef MOZ_STORAGE
#include "nsDOMStorageDBWrapper.h"
#endif

#define IS_PERMISSION_ALLOWED(perm) \
      ((perm) != nsIPermissionManager::UNKNOWN_ACTION && \
      (perm) != nsIPermissionManager::DENY_ACTION)

class nsDOMStorage;
class nsIDOMStorage;
class nsDOMStorageItem;

using mozilla::TimeStamp;
using mozilla::TimeDuration;

class nsDOMStorageEntry : public nsVoidPtrHashKey
{
public:
  nsDOMStorageEntry(KeyTypePointer aStr);
  nsDOMStorageEntry(const nsDOMStorageEntry& aToCopy);
  ~nsDOMStorageEntry();

  
  nsDOMStorage* mStorage;
};

class nsSessionStorageEntry : public nsStringHashKey
{
public:
  nsSessionStorageEntry(KeyTypePointer aStr);
  nsSessionStorageEntry(const nsSessionStorageEntry& aToCopy);
  ~nsSessionStorageEntry();

  nsRefPtr<nsDOMStorageItem> mItem;
};

class nsDOMStorageManager : public nsIDOMStorageManager
                          , public nsIObserver
{
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSTORAGEMANAGER

  
  NS_DECL_NSIOBSERVER

  nsDOMStorageManager();

  void AddToStoragesHash(nsDOMStorage* aStorage);
  void RemoveFromStoragesHash(nsDOMStorage* aStorage);

  nsresult ClearAllStorages();

  PRBool InPrivateBrowsingMode() { return mInPrivateBrowsing; }

  static nsresult Initialize();
  static nsDOMStorageManager* GetInstance();
  static void Shutdown();

  static nsDOMStorageManager* gStorageManager;

protected:

  nsTHashtable<nsDOMStorageEntry> mStorages;
  PRBool mInPrivateBrowsing;
};

class nsDOMStorage : public nsIDOMStorageObsolete,
                     public nsPIDOMStorage,
                     public nsIObserver,
                     public nsSupportsWeakReference
{
public:
  nsDOMStorage();
  nsDOMStorage(nsDOMStorage& aThat);
  virtual ~nsDOMStorage();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsDOMStorage, nsIDOMStorageObsolete)

  NS_DECL_NSIDOMSTORAGEOBSOLETE
  NS_DECL_NSIOBSERVER

  
  nsresult GetItem(const nsAString& key, nsAString& aData);
  nsresult Clear();

  
  virtual nsresult InitAsSessionStorage(nsIPrincipal *aPrincipal, const nsSubstring &aDocumentURI);
  virtual nsresult InitAsLocalStorage(nsIPrincipal *aPrincipal, const nsSubstring &aDocumentURI);
  virtual nsresult InitAsGlobalStorage(const nsACString &aDomainDemanded);
  virtual already_AddRefed<nsIDOMStorage> Clone();
  virtual already_AddRefed<nsIDOMStorage> Fork(const nsSubstring &aDocumentURI);
  virtual PRBool IsForkOf(nsIDOMStorage* aThat);
  virtual nsTArray<nsString> *GetKeys();
  virtual nsIPrincipal* Principal();
  virtual PRBool CanAccess(nsIPrincipal *aPrincipal);
  virtual nsDOMStorageType StorageType();
  virtual void BroadcastChangeNotification(const nsSubstring &aKey,
                                           const nsSubstring &aOldValue,
                                           const nsSubstring &aNewValue);

  
  
  
  
  
  
  PRBool UseDB() {
    return mUseDB;
  }

  PRBool SessionOnly() {
    return mSessionOnly;
  }

  
  
  bool CanUseChromePersist();

  
  
  static PRBool
  CanUseStorage(PRPackedBool* aSessionOnly);

  
  
  
  static PRBool
  URICanUseChromePersist(nsIURI* aURI);
  
  
  
  PRBool
  CacheStoragePermissions();

  
  
  nsresult
  GetCachedValue(const nsAString& aKey,
                 nsAString& aValue,
                 PRBool* aSecure);

  
  nsresult
  GetDBValue(const nsAString& aKey,
             nsAString& aValue,
             PRBool* aSecure);

  
  
  
  nsresult
  SetDBValue(const nsAString& aKey,
             const nsAString& aValue,
             PRBool aSecure);

  
  nsresult
  SetSecure(const nsAString& aKey, PRBool aSecure);

  
  void ClearAll();

  nsresult
  CloneFrom(nsDOMStorage* aThat);

  nsIDOMStorageItem* GetNamedItem(const nsAString& aKey, nsresult* aResult);

  static nsDOMStorage* FromSupports(nsISupports* aSupports)
  {
    return static_cast<nsDOMStorage*>(static_cast<nsIDOMStorageObsolete*>(aSupports));
  }

  nsresult RegisterObservers(bool persistent);
  nsresult MaybeCommitTemporaryTable(bool force);

  bool WasTemporaryTableLoaded();
  void SetTemporaryTableLoaded(bool loaded);

protected:

  friend class nsDOMStorageManager;
  friend class nsDOMStorage2;
  friend class nsDOMStoragePersistentDB;

  static nsresult InitDB();

  
  nsresult CacheKeysFromDB();

  PRBool CanAccessSystem(nsIPrincipal *aPrincipal);

  
  PRPackedBool mUseDB;

  
  nsCString mDomain;

  
  nsString mDocumentURI;

  
  
  
  
  
  PRPackedBool mSessionOnly;

  
  
  
  
  nsDOMStorageType mStorageType;

  
  PRPackedBool mItemsCached;

  
  nsTHashtable<nsSessionStorageEntry> mItems;

  
  
  nsCString mScopeDBKey;
  nsCString mQuotaETLDplus1DomainDBKey;
  nsCString mQuotaDomainDBKey;

  friend class nsIDOMStorage2;
  nsPIDOMStorage* mSecurityChecker;
  nsPIDOMStorage* mEventBroadcaster;

  bool mCanUseChromePersist;

  bool mLoadedTemporaryTable;
  TimeStamp mLastTemporaryTableAccessTime;
  TimeStamp mTemporaryTableAge;

public:
  
  
  
  nsCString& GetScopeDBKey() {return mScopeDBKey;}

  
  
  nsCString& GetQuotaDomainDBKey(PRBool aOfflineAllowed)
  {
    return aOfflineAllowed ? mQuotaDomainDBKey : mQuotaETLDplus1DomainDBKey;
  }

 #ifdef MOZ_STORAGE
   static nsDOMStorageDBWrapper* gStorageDB;
 #endif
};

class nsDOMStorage2 : public nsIDOMStorage,
                      public nsPIDOMStorage
{
public:
  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsDOMStorage2, nsIDOMStorage)

  nsDOMStorage2(nsDOMStorage2& aThat);
  nsDOMStorage2();

  NS_DECL_NSIDOMSTORAGE

  
  virtual nsresult InitAsSessionStorage(nsIPrincipal *aPrincipal, const nsSubstring &aDocumentURI);
  virtual nsresult InitAsLocalStorage(nsIPrincipal *aPrincipal, const nsSubstring &aDocumentURI);
  virtual nsresult InitAsGlobalStorage(const nsACString &aDomainDemanded);
  virtual already_AddRefed<nsIDOMStorage> Clone();
  virtual already_AddRefed<nsIDOMStorage> Fork(const nsSubstring &aDocumentURI);
  virtual PRBool IsForkOf(nsIDOMStorage* aThat);
  virtual nsTArray<nsString> *GetKeys();
  virtual nsIPrincipal* Principal();
  virtual PRBool CanAccess(nsIPrincipal *aPrincipal);
  virtual nsDOMStorageType StorageType();
  virtual void BroadcastChangeNotification(const nsSubstring &aKey,
                                           const nsSubstring &aOldValue,
                                           const nsSubstring &aNewValue);

  nsresult InitAsSessionStorageFork(nsIPrincipal *aPrincipal,
                                    const nsSubstring &aDocumentURI,
                                    nsIDOMStorageObsolete* aStorage);

private:
  
  
  nsCOMPtr<nsIPrincipal> mPrincipal;

  
  
  nsString mDocumentURI;
  nsRefPtr<nsDOMStorage> mStorage;
};

class nsDOMStorageList : public nsIDOMStorageList
{
public:
  nsDOMStorageList()
  {
    mStorages.Init();
  }

  virtual ~nsDOMStorageList() {}

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSTORAGELIST

  nsIDOMStorageObsolete* GetNamedItem(const nsAString& aDomain, nsresult* aResult);

  


  static PRBool
  CanAccessDomain(const nsACString& aRequestedDomain,
                  const nsACString& aCurrentDomain);

protected:

  









  nsIDOMStorageObsolete*
  GetStorageForDomain(const nsACString& aRequestedDomain,
                      const nsACString& aCurrentDomain,
                      PRBool aNoCurrentDomainCheck,
                      nsresult* aResult);

  


  static PRBool
  ConvertDomainToArray(const nsACString& aDomain,
                       nsTArray<nsCString>* aArray);

  nsInterfaceHashtable<nsCStringHashKey, nsIDOMStorageObsolete> mStorages;
};

class nsDOMStorageItem : public nsIDOMStorageItem,
                         public nsIDOMToString
{
public:
  nsDOMStorageItem(nsDOMStorage* aStorage,
                   const nsAString& aKey,
                   const nsAString& aValue,
                   PRBool aSecure);
  virtual ~nsDOMStorageItem();

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsDOMStorageItem, nsIDOMStorageItem)

  
  NS_DECL_NSIDOMSTORAGEITEM

  
  NS_DECL_NSIDOMTOSTRING

  PRBool IsSecure()
  {
    return mSecure;
  }

  void SetSecureInternal(PRBool aSecure)
  {
    mSecure = aSecure;
  }

  const nsAString& GetValueInternal()
  {
    return mValue;
  }

  const void SetValueInternal(const nsAString& aValue)
  {
    mValue = aValue;
  }

  void ClearValue()
  {
    mValue.Truncate();
  }

  void SetInsertTimeToNow()
  {
    mInsertTime = PR_Now();
  }

  bool ShouldBeCutOff(PRInt64 since)
  {
    return mInsertTime > since;
  }

protected:

  
  PRBool mSecure;

  
  nsString mKey;

  
  nsString mValue;

  
  PRInt64 mInsertTime;

  
  
  nsRefPtr<nsDOMStorage> mStorage;
};

class nsDOMStorageEvent : public nsDOMEvent,
                          public nsIDOMStorageEvent
{
public:
  nsDOMStorageEvent()
    : nsDOMEvent(nsnull, nsnull)
  {
  }

  virtual ~nsDOMStorageEvent()
  {
  }

  nsresult Init();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMStorageEvent, nsDOMEvent)

  NS_DECL_NSIDOMSTORAGEEVENT
  NS_FORWARD_NSIDOMEVENT(nsDOMEvent::)

protected:
  nsString mKey;
  nsString mOldValue;
  nsString mNewValue;
  nsString mUrl;
  nsCOMPtr<nsIDOMStorage> mStorageArea;
};

class nsDOMStorageEventObsolete : public nsDOMEvent,
                          public nsIDOMStorageEventObsolete
{
public:
  nsDOMStorageEventObsolete()
    : nsDOMEvent(nsnull, nsnull)
  {
  }

  virtual ~nsDOMStorageEventObsolete()
  {
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMSTORAGEEVENTOBSOLETE
  NS_FORWARD_NSIDOMEVENT(nsDOMEvent::)

protected:
  nsString mDomain;
};

nsresult
NS_NewDOMStorage(nsISupports* aOuter, REFNSIID aIID, void** aResult);

nsresult
NS_NewDOMStorage2(nsISupports* aOuter, REFNSIID aIID, void** aResult);

nsresult
NS_NewDOMStorageList(nsIDOMStorageList** aResult);

PRUint32
GetOfflinePermission(const nsACString &aDomain);

PRBool
IsOfflineAllowed(const nsACString &aDomain);

#endif 
