






































#ifndef nsDOMStorage_h___
#define nsDOMStorage_h___

#include "nscore.h"
#include "nsAutoPtr.h"
#include "nsIDOMStorage.h"
#include "nsIDOMStorageList.h"
#include "nsIDOMStorageItem.h"
#include "nsInterfaceHashtable.h"
#include "nsVoidArray.h"
#include "nsPIDOMStorage.h"
#include "nsIDOMToString.h"
#include "nsDOMEvent.h"
#include "nsIDOMStorageEvent.h"

#ifdef MOZ_STORAGE
#include "nsDOMStorageDB.h"
#endif

class nsDOMStorage;
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

class nsDOMStorageManager : public nsIObserver
{
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIOBSERVER

  void AddToStoragesHash(nsDOMStorage* aStorage);
  void RemoveFromStoragesHash(nsDOMStorage* aStorage);

  nsresult ClearAllStorages();

  static nsresult Initialize();
  static void Shutdown();

  static nsDOMStorageManager* gStorageManager;

protected:

  nsTHashtable<nsDOMStorageEntry> mStorages;
};

class nsDOMStorage : public nsIDOMStorage,
                     public nsPIDOMStorage
{
public:
  nsDOMStorage();
  nsDOMStorage(nsIURI* aURI, const nsAString& aDomain, PRBool aUseDB);
  virtual ~nsDOMStorage();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSTORAGE

  
  virtual void Init(nsIURI* aURI, const nsAString& aDomain, PRBool aUseDB);
  virtual already_AddRefed<nsIDOMStorage> Clone(nsIURI* aURI);
  virtual nsTArray<nsString> *GetKeys();

  PRBool UseDB() { return mUseDB && !mSessionOnly; }

  
  
  
  static PRBool
  CanUseStorage(nsIURI* aURI, PRPackedBool* aSessionOnly);

  PRBool
  CacheStoragePermissions()
  {
    return CanUseStorage(mURI, &mSessionOnly);
  }

  
  nsresult
  GetDBValue(const nsAString& aKey,
             nsAString& aValue,
             PRBool* aSecure,
             nsAString& aOwner);

  
  
  
  nsresult
  SetDBValue(const nsAString& aKey,
             const nsAString& aValue,
             PRBool aSecure);

  
  nsresult
  SetSecure(const nsAString& aKey, PRBool aSecure);

  
  void ClearAll();

protected:

  friend class nsDOMStorageManager;

  static nsresult InitDB();

  
  nsresult CacheKeysFromDB();

  void BroadcastChangeNotification();

  
  PRPackedBool mUseDB;

  
  PRPackedBool mSessionOnly;

  
  PRPackedBool mItemsCached;

  
  nsCOMPtr<nsIURI> mURI;

  
  nsAutoString mDomain;

  
  nsTHashtable<nsSessionStorageEntry> mItems;

#ifdef MOZ_STORAGE
  static nsDOMStorageDB* gStorageDB;
#endif
};

class nsDOMStorageList : public nsIDOMStorageList
{
public:
  nsDOMStorageList()
  {
    mStorages.Init();
  };

  virtual ~nsDOMStorageList() {};

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSTORAGELIST

  


  static PRBool
  CanAccessDomain(const nsAString& aRequestedDomain,
                  const nsAString& aCurrentDomain);

protected:

  









  nsresult
  GetStorageForDomain(nsIURI* aURI,
                      const nsAString& aRequestedDomain,
                      const nsAString& aCurrentDomain,
                      PRBool aNoCurrentDomainCheck,
                      nsIDOMStorage** aStorage);

  


  static PRBool
  ConvertDomainToArray(const nsAString& aDomain,
                       nsStringArray* aArray);

  nsInterfaceHashtable<nsStringHashKey, nsIDOMStorage> mStorages;
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

  
  NS_DECL_ISUPPORTS

  
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
