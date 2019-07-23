







































#ifndef nsDOMStorage_h___
#define nsDOMStorage_h___

#include "nscore.h"
#include "nsAutoPtr.h"
#include "nsIDOMStorageObsolete.h"
#include "nsIDOMStorage.h"
#include "nsIDOMStorageList.h"
#include "nsIDOMStorageItem.h"
#include "nsInterfaceHashtable.h"
#include "nsVoidArray.h"
#include "nsTArray.h"
#include "nsPIDOMStorage.h"
#include "nsIDOMToString.h"
#include "nsDOMEvent.h"
#include "nsIDOMStorageEvent.h"
#include "nsIDOMStorageManager.h"
#include "nsCycleCollectionParticipant.h"

#ifdef MOZ_STORAGE
#include "nsDOMStorageDB.h"
#endif

class nsDOMStorage;
class nsIDOMStorage;
class nsDOMStorageItem;

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

  
  virtual nsresult InitAsLocalStorage(nsIPrincipal *aPrincipal);
  virtual nsresult InitAsGlobalStorage(const nsACString &aDomainDemanded);
  virtual nsresult InitAsSessionStorage(nsIURI* aURI);
  virtual already_AddRefed<nsIDOMStorageObsolete> Clone();
  virtual nsTArray<nsString> *GetKeys();
  virtual const nsCString &Domain();
  virtual PRBool CanAccess(nsIPrincipal *aPrincipal);

  
  
  
  
  
  
  PRBool UseDB() {
    return mUseDB && !mSessionOnly &&
           !nsDOMStorageManager::gStorageManager->InPrivateBrowsingMode();
  }

  
  
  static PRBool
  CanUseStorage(PRPackedBool* aSessionOnly);

  
  
  PRBool
  CacheStoragePermissions();

  
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

  nsIDOMStorageItem* GetNamedItem(const nsAString& aKey, nsresult* aResult);

  static nsDOMStorage* FromSupports(nsISupports* aSupports)
  {
    return static_cast<nsDOMStorage*>(static_cast<nsIDOMStorageObsolete*>(aSupports));
  }

protected:

  friend class nsDOMStorageManager;
  friend class nsDOMStorage2;

  static nsresult InitDB();

  
  nsresult CacheKeysFromDB();

  void BroadcastChangeNotification();

  PRBool CanAccessSystem(nsIPrincipal *aPrincipal);

  
  PRPackedBool mUseDB;

  
  
  
  
  
  PRPackedBool mSessionOnly;

  
  
  
  
  PRPackedBool mLocalStorage;

  
  PRPackedBool mItemsCached;

  
  nsCString mDomain;

  
  nsTHashtable<nsSessionStorageEntry> mItems;

  
  
  nsCString mScopeDBKey;
  nsCString mQuotaDomainDBKey;

public:
  
  
  
  nsCString& GetScopeDBKey() {return mScopeDBKey;}

  
  
  nsCString& GetQuotaDomainDBKey() {return mQuotaDomainDBKey;}

 #ifdef MOZ_STORAGE
   static nsDOMStorageDB* gStorageDB;
 #endif
};

class nsDOMStorage2 : public nsIDOMStorage,
                      public nsPIDOMStorage
{
public:
  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsDOMStorage2, nsIDOMStorage)

  NS_DECL_NSIDOMSTORAGE

  
  virtual nsresult InitAsLocalStorage(nsIPrincipal *aPrincipal);
  virtual nsresult InitAsGlobalStorage(const nsACString &aDomainDemanded);
  virtual nsresult InitAsSessionStorage(nsIURI* aURI);
  virtual already_AddRefed<nsIDOMStorageObsolete> Clone();
  virtual nsTArray<nsString> *GetKeys();
  virtual const nsCString &Domain();
  virtual PRBool CanAccess(nsIPrincipal *aPrincipal);

private:
  
  
  nsCOMPtr<nsIPrincipal> mPrincipal;

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

protected:

  
  PRBool mSecure;

  
  nsString mKey;

  
  nsString mValue;

  
  
  nsRefPtr<nsDOMStorage> mStorage;
};

class nsDOMStorageEvent : public nsDOMEvent,
                          public nsIDOMStorageEvent
{
public:
  nsDOMStorageEvent(const nsAString& aDomain)
    : nsDOMEvent(nsnull, nsnull), mDomain(aDomain)
  {
    if (aDomain.IsEmpty()) {
      
      

      mDomain = NS_LITERAL_STRING("#session");
    }
  }

  virtual ~nsDOMStorageEvent()
  {
  }

  nsresult Init();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMSTORAGEEVENT
  NS_FORWARD_NSIDOMEVENT(nsDOMEvent::)

protected:
  nsString mDomain;
};

NS_IMETHODIMP
NS_NewDOMStorage(nsISupports* aOuter, REFNSIID aIID, void** aResult);

nsresult
NS_NewDOMStorageList(nsIDOMStorageList** aResult);

#endif 
