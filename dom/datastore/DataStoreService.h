





#ifndef mozilla_dom_DataStoreService_h
#define mozilla_dom_DataStoreService_h

#include "mozilla/dom/PContent.h"
#include "nsClassHashtable.h"
#include "nsIDataStoreService.h"
#include "nsIObserver.h"
#include "nsRefPtrHashtable.h"

class nsIPrincipal;
class nsIUUIDGenerator;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class DataStoreInfo;
class FirstRevisionIdCallback;
class PendingRequest;
class Promise;
class RetrieveRevisionsCounter;
class RevisionAddedEnableStoreCallback;

class DataStoreService MOZ_FINAL : public nsIDataStoreService
                                 , public nsIObserver
{
  friend class ContentChild;
  friend class FirstRevisionIdCallback;
  friend class RetrieveRevisionsCounter;
  friend class RevisionAddedEnableStoreCallback;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIDATASTORESERVICE

  
  
  static already_AddRefed<DataStoreService> GetOrCreate();

  static already_AddRefed<DataStoreService> Get();

  static void Shutdown();

  static bool CheckPermission(nsIPrincipal* principal);

  nsresult GenerateUUID(nsAString& aID);

  nsresult GetDataStoresFromIPC(const nsAString& aName,
                                const nsAString& aOwner,
                                nsIPrincipal* aPrincipal,
                                nsTArray<DataStoreSetting>* aValue);

private:
  DataStoreService();
  ~DataStoreService();

  nsresult Init();

  typedef nsClassHashtable<nsUint32HashKey, DataStoreInfo> HashApp;

  nsresult AddPermissions(uint32_t aAppId, const nsAString& aName,
                          const nsAString& aOriginURL,
                          const nsAString& aManifestURL,
                          bool aReadOnly);

  nsresult AddAccessPermissions(uint32_t aAppId, const nsAString& aName,
                                const nsAString& aOriginURL,
                                const nsAString& aManifestURL,
                                bool aReadOnly);

  nsresult CreateFirstRevisionId(uint32_t aAppId, const nsAString& aName,
                                 const nsAString& aManifestURL);

  void GetDataStoresCreate(nsPIDOMWindow* aWindow, Promise* aPromise,
                           const nsTArray<DataStoreInfo>& aStores);

  void GetDataStoresResolve(nsPIDOMWindow* aWindow, Promise* aPromise,
                            const nsTArray<DataStoreInfo>& aStores);

  nsresult GetDataStoreInfos(const nsAString& aName, const nsAString& aOwner,
                             uint32_t aAppId, nsIPrincipal* aPrincipal,
                             nsTArray<DataStoreInfo>& aStores);

  void DeleteDataStores(uint32_t aAppId);

  nsresult EnableDataStore(uint32_t aAppId, const nsAString& aName,
                           const nsAString& aManifestURL);

  already_AddRefed<RetrieveRevisionsCounter> GetCounter(uint32_t aId) const;

  void RemoveCounter(uint32_t aId);

  nsClassHashtable<nsStringHashKey, HashApp> mStores;
  nsClassHashtable<nsStringHashKey, HashApp> mAccessStores;

  typedef nsTArray<PendingRequest> PendingRequests;
  nsClassHashtable<nsStringHashKey, PendingRequests> mPendingRequests;

  nsRefPtrHashtable<nsUint32HashKey, RetrieveRevisionsCounter> mPendingCounters;

  nsCOMPtr<nsIUUIDGenerator> mUUIDGenerator;
};

} 
} 

#endif 
