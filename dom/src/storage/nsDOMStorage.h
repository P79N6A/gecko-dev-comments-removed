







































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

#include "nsDOMStorageDBWrapper.h"

#define IS_PERMISSION_ALLOWED(perm) \
      ((perm) != nsIPermissionManager::UNKNOWN_ACTION && \
      (perm) != nsIPermissionManager::DENY_ACTION)

class nsDOMStorage;
class nsIDOMStorage;
class nsDOMStorageItem;
class nsDOMStoragePersistentDB;

namespace mozilla {
namespace dom {
class StorageParent;
}
}
using mozilla::dom::StorageParent;

class DOMStorageImpl;

class nsDOMStorageEntry : public nsVoidPtrHashKey
{
public:
  nsDOMStorageEntry(KeyTypePointer aStr);
  nsDOMStorageEntry(const nsDOMStorageEntry& aToCopy);
  ~nsDOMStorageEntry();

  
  DOMStorageImpl* mStorage;
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
                          , public nsSupportsWeakReference
{
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSTORAGEMANAGER

  
  NS_DECL_NSIOBSERVER

  nsDOMStorageManager();

  void AddToStoragesHash(DOMStorageImpl* aStorage);
  void RemoveFromStoragesHash(DOMStorageImpl* aStorage);

  nsresult ClearAllStorages();

  bool InPrivateBrowsingMode() { return mInPrivateBrowsing; }

  static nsresult Initialize();
  static nsDOMStorageManager* GetInstance();
  static void Shutdown();

  


  bool UnflushedDataExists();

  static nsDOMStorageManager* gStorageManager;

protected:

  nsTHashtable<nsDOMStorageEntry> mStorages;
  bool mInPrivateBrowsing;
};

class DOMStorageBase : public nsISupports
{
public:
  DOMStorageBase();
  DOMStorageBase(DOMStorageBase&);

  virtual void InitAsSessionStorage(nsIURI* aDomainURI);
  virtual void InitAsLocalStorage(nsIURI* aDomainURI, bool aCanUseChromePersist);
  virtual void InitAsGlobalStorage(const nsACString& aDomainDemanded);

  virtual nsTArray<nsString>* GetKeys(bool aCallerSecure) = 0;
  virtual nsresult GetLength(bool aCallerSecure, PRUint32* aLength) = 0;
  virtual nsresult GetKey(bool aCallerSecure, PRUint32 aIndex, nsAString& aKey) = 0;
  virtual nsIDOMStorageItem* GetValue(bool aCallerSecure, const nsAString& aKey,
                                      nsresult* rv) = 0;
  virtual nsresult SetValue(bool aCallerSecure, const nsAString& aKey,
                            const nsAString& aData, nsAString& aOldValue) = 0;
  virtual nsresult RemoveValue(bool aCallerSecure, const nsAString& aKey,
                               nsAString& aOldValue) = 0;
  virtual nsresult Clear(bool aCallerSecure, PRInt32* aOldCount) = 0;
  
  
  
  
  
  
  
  bool UseDB() {
    return mUseDB;
  }

  
  virtual nsresult
  GetDBValue(const nsAString& aKey,
             nsAString& aValue,
             bool* aSecure) = 0;

  
  
  
  virtual nsresult
  SetDBValue(const nsAString& aKey,
             const nsAString& aValue,
             bool aSecure) = 0;

  
  virtual nsresult
  SetSecure(const nsAString& aKey, bool aSecure) = 0;

  virtual nsresult
  CloneFrom(bool aCallerSecure, DOMStorageBase* aThat) = 0;

  
  
  
  nsCString& GetScopeDBKey() {return mScopeDBKey;}

  
  
  nsCString& GetQuotaDomainDBKey(bool aOfflineAllowed)
  {
    return aOfflineAllowed ? mQuotaDomainDBKey : mQuotaETLDplus1DomainDBKey;
  }

  virtual bool CacheStoragePermissions() = 0;

protected:
  friend class nsDOMStorageManager;
  friend class nsDOMStorage;

  nsPIDOMStorage::nsDOMStorageType mStorageType;
  
  
  bool mUseDB;

  
  
  
  
  
  bool mSessionOnly;

  
  nsCString mDomain;

  
  
  nsCString mScopeDBKey;
  nsCString mQuotaETLDplus1DomainDBKey;
  nsCString mQuotaDomainDBKey;

  bool mCanUseChromePersist;
};

class DOMStorageImpl : public DOMStorageBase

{
public:
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMStorageImpl)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  DOMStorageImpl(nsDOMStorage*);
  DOMStorageImpl(nsDOMStorage*, DOMStorageImpl&);
  ~DOMStorageImpl();

  virtual void InitAsSessionStorage(nsIURI* aDomainURI);
  virtual void InitAsLocalStorage(nsIURI* aDomainURI, bool aCanUseChromePersist);
  virtual void InitAsGlobalStorage(const nsACString& aDomainDemanded);

  bool SessionOnly() {
    return mSessionOnly;
  }

  virtual nsTArray<nsString>* GetKeys(bool aCallerSecure);
  virtual nsresult GetLength(bool aCallerSecure, PRUint32* aLength);
  virtual nsresult GetKey(bool aCallerSecure, PRUint32 aIndex, nsAString& aKey);
  virtual nsIDOMStorageItem* GetValue(bool aCallerSecure, const nsAString& aKey,
                                      nsresult* rv);
  virtual nsresult SetValue(bool aCallerSecure, const nsAString& aKey,
                            const nsAString& aData, nsAString& aOldValue);
  virtual nsresult RemoveValue(bool aCallerSecure, const nsAString& aKey,
                               nsAString& aOldValue);
  virtual nsresult Clear(bool aCallerSecure, PRInt32* aOldCount);

  
  nsresult CacheKeysFromDB();

  PRUint64 CachedVersion() { return mItemsCachedVersion; }
  void SetCachedVersion(PRUint64 version) { mItemsCachedVersion = version; }
  
  
  
  bool CanUseChromePersist();

  
  
  nsresult
  GetCachedValue(const nsAString& aKey,
                 nsAString& aValue,
                 bool* aSecure);

  
  virtual nsresult
  GetDBValue(const nsAString& aKey,
             nsAString& aValue,
             bool* aSecure);

  
  
  
  virtual nsresult
  SetDBValue(const nsAString& aKey,
             const nsAString& aValue,
             bool aSecure);

  
  virtual nsresult
  SetSecure(const nsAString& aKey, bool aSecure);

  
  void ClearAll();

  virtual nsresult
  CloneFrom(bool aCallerSecure, DOMStorageBase* aThat);

  virtual bool CacheStoragePermissions();

private:
  static nsDOMStorageDBWrapper* gStorageDB;
  friend class nsDOMStorageManager;
  friend class nsDOMStoragePersistentDB;
  friend class StorageParent;

  void Init(nsDOMStorage*);

  
  
  void InitFromChild(bool aUseDB, bool aCanUseChromePersist, bool aSessionOnly,
                     const nsACString& aDomain,
                     const nsACString& aScopeDBKey,
                     const nsACString& aQuotaDomainDBKey,
                     const nsACString& aQuotaETLDplus1DomainDBKey,
                     PRUint32 aStorageType);
  void SetSessionOnly(bool aSessionOnly);

  static nsresult InitDB();

  
  
  PRUint64 mItemsCachedVersion;

  
  nsTHashtable<nsSessionStorageEntry> mItems;

  
  nsDOMStorage* mOwner;
};

class nsDOMStorage : public nsIDOMStorageObsolete,
                     public nsPIDOMStorage
{
public:
  nsDOMStorage();
  nsDOMStorage(nsDOMStorage& aThat);
  virtual ~nsDOMStorage();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsDOMStorage, nsIDOMStorageObsolete)

  NS_DECL_NSIDOMSTORAGEOBSOLETE

  
  nsresult GetItem(const nsAString& key, nsAString& aData);
  nsresult Clear();

  
  virtual nsresult InitAsSessionStorage(nsIPrincipal *aPrincipal, const nsSubstring &aDocumentURI);
  virtual nsresult InitAsLocalStorage(nsIPrincipal *aPrincipal, const nsSubstring &aDocumentURI);
  virtual nsresult InitAsGlobalStorage(const nsACString &aDomainDemanded);
  virtual already_AddRefed<nsIDOMStorage> Clone();
  virtual already_AddRefed<nsIDOMStorage> Fork(const nsSubstring &aDocumentURI);
  virtual bool IsForkOf(nsIDOMStorage* aThat);
  virtual nsTArray<nsString> *GetKeys();
  virtual nsIPrincipal* Principal();
  virtual bool CanAccess(nsIPrincipal *aPrincipal);
  virtual nsDOMStorageType StorageType();
  virtual void BroadcastChangeNotification(const nsSubstring &aKey,
                                           const nsSubstring &aOldValue,
                                           const nsSubstring &aNewValue);

  
  
  static bool
  CanUseStorage(bool* aSessionOnly);

  
  
  
  static bool
  URICanUseChromePersist(nsIURI* aURI);
  
  
  
  bool
  CacheStoragePermissions();

  nsIDOMStorageItem* GetNamedItem(const nsAString& aKey, nsresult* aResult);

  static nsDOMStorage* FromSupports(nsISupports* aSupports)
  {
    return static_cast<nsDOMStorage*>(static_cast<nsIDOMStorageObsolete*>(aSupports));
  }

  nsresult SetSecure(const nsAString& aKey, bool aSecure)
  {
    return mStorageImpl->SetSecure(aKey, aSecure);
  }

  nsresult CloneFrom(nsDOMStorage* aThat);

 protected:
  friend class nsDOMStorage2;
  friend class nsDOMStoragePersistentDB;

  nsRefPtr<DOMStorageBase> mStorageImpl;

  bool CanAccessSystem(nsIPrincipal *aPrincipal);

  
  nsString mDocumentURI;

  
  
  
  
  nsDOMStorageType mStorageType;

  friend class nsIDOMStorage2;
  nsPIDOMStorage* mSecurityChecker;
  nsPIDOMStorage* mEventBroadcaster;
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
  virtual bool IsForkOf(nsIDOMStorage* aThat);
  virtual nsTArray<nsString> *GetKeys();
  virtual nsIPrincipal* Principal();
  virtual bool CanAccess(nsIPrincipal *aPrincipal);
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

  


  static bool
  CanAccessDomain(const nsACString& aRequestedDomain,
                  const nsACString& aCurrentDomain);

protected:

  









  nsIDOMStorageObsolete*
  GetStorageForDomain(const nsACString& aRequestedDomain,
                      const nsACString& aCurrentDomain,
                      bool aNoCurrentDomainCheck,
                      nsresult* aResult);

  


  static bool
  ConvertDomainToArray(const nsACString& aDomain,
                       nsTArray<nsCString>* aArray);

  nsInterfaceHashtable<nsCStringHashKey, nsIDOMStorageObsolete> mStorages;
};

class nsDOMStorageItem : public nsIDOMStorageItem,
                         public nsIDOMToString
{
public:
  nsDOMStorageItem(DOMStorageBase* aStorage,
                   const nsAString& aKey,
                   const nsAString& aValue,
                   bool aSecure);
  virtual ~nsDOMStorageItem();

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsDOMStorageItem, nsIDOMStorageItem)

  
  NS_DECL_NSIDOMSTORAGEITEM

  
  NS_DECL_NSIDOMTOSTRING

  bool IsSecure()
  {
    return mSecure;
  }

  void SetSecureInternal(bool aSecure)
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

protected:

  
  bool mSecure;

  
  nsString mKey;

  
  nsString mValue;

  
  
  nsRefPtr<DOMStorageBase> mStorage;
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

bool
IsOfflineAllowed(const nsACString &aDomain);

#endif 
