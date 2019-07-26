




#ifndef nsLocalStorageCache_h___
#define nsLocalStorageCache_h___

#include "nscore.h"
#include "nsClassHashtable.h"
#include "nsTHashtable.h"
#include "nsTArray.h"

class DOMStorageImpl;
class nsSessionStorageEntry;



































class nsScopeCache
{
public:
  nsScopeCache();
  nsresult AddEntry(const nsAString& aKey,
                    const nsAString& aValue,
                    bool aSecure);
  nsresult GetAllKeys(DOMStorageImpl* aStorage,
                      nsTHashtable<nsSessionStorageEntry>* aKeys) const;
  bool GetKey(const nsAString& aKey, nsAString& aValue, bool* aSecure) const;
  nsresult SetKey(const nsAString& aKey, const nsAString& aValue, bool aSecure);
  void SetSecure(const nsAString& aKey, bool aSecure);
  void RemoveKey(const nsAString& aKey);
  void DeleteScope();
  int32_t GetQuotaUsage() const;

  


  class KeyEntry
  {
  public:
    KeyEntry() : mIsSecure(false), mIsDirty(false) {}
    nsString mValue;
    bool mIsSecure;
    bool mIsDirty;
  };

private:
  friend class nsLocalStorageCache;

  
  
  bool mWasScopeDeleted;
  
  nsTArray<nsString> mDeletedKeys;
  
  nsClassHashtable<nsStringHashKey, KeyEntry> mTable;

  
  
  PRIntervalTime mAccessTime;
  
  
  bool mIsDirty;
  
  bool mIsFlushPending;
};




class nsLocalStorageCache
{
public:
  nsLocalStorageCache();

  nsScopeCache* GetScope(const nsACString& aScopeName);
  void AddScope(const nsACString& aScopeName, nsScopeCache* aScopeCache);
  bool IsScopeCached(const nsACString& aScopeName) const;
  uint32_t Count() const;

  int32_t GetQuotaUsage(const nsACString& aQuotaKey) const;
  void MarkMatchingScopesDeleted(const nsACString& aPattern);
  void ForgetAllScopes();

  


  struct FlushData
  {
  public:
    struct ChangeSet
    {
      bool mWasDeleted;
      nsTArray<nsString>* mDeletedKeys;
      nsTArray<nsString> mDirtyKeys;
      nsTArray<nsScopeCache::KeyEntry*> mDirtyValues;
    };
    
    
    nsTArray<nsCString> mScopeNames;
    nsTArray<ChangeSet> mChanged;
  };

  


  void GetFlushData(FlushData* aData) const;

  



  void MarkScopesPending();
  void MarkScopesFlushed();
  void MarkFlushFailed();

  



  void EvictScopes(nsTArray<nsCString>& aEvicted,
                   nsTArray<int32_t>& aEvictedSize);

private:

  static PLDHashOperator
  GetDirtyDataEnum(const nsACString& aScopeName,
                   nsScopeCache* aScopeCache,
                   void* aParams);

  enum FlushState {
    FLUSH_PENDING,
    FLUSHED,
    FLUSH_FAILED
  };
  static PLDHashOperator
  SetFlushStateEnum(const nsACString& aScopeName,
                    nsAutoPtr<nsScopeCache>& aScopeCache,
                    void* aParams);

  static PLDHashOperator
  EvictEnum(const nsACString& aScopeName,
            nsAutoPtr<nsScopeCache>& aScopeCache,
            void* aParams);

private:
  
  
  nsClassHashtable<nsCStringHashKey, nsScopeCache> mScopeCaches;
};

#endif
