




#ifndef nsDOMStorage_h___
#define nsDOMStorage_h___

#include "nsIDOMStorage.h"
#include "nsPIDOMStorage.h"
#include "nsWeakReference.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace dom {

class DOMStorageManager;
class DOMStorageCache;

class DOMStorage MOZ_FINAL : public nsIDOMStorage
                           , public nsPIDOMStorage
                           , public nsSupportsWeakReference
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMSTORAGE

  
  virtual StorageType GetType() const;
  virtual DOMStorageManager* GetManager() const { return mManager; }
  virtual const DOMStorageCache* GetCache() const { return mCache; }

  virtual nsTArray<nsString>* GetKeys();
  virtual nsIPrincipal* GetPrincipal();
  virtual bool PrincipalEquals(nsIPrincipal* aPrincipal);
  virtual bool CanAccess(nsIPrincipal* aPrincipal);
  virtual bool IsPrivate() { return mIsPrivate; }

  DOMStorage(DOMStorageManager* aManager,
             DOMStorageCache* aCache,
             const nsAString& aDocumentURI,
             nsIPrincipal* aPrincipal,
             bool aIsPrivate);
  ~DOMStorage();

  
  
  
  
  
  
  
  static bool CanUseStorage(DOMStorage* aStorage = nullptr);

  bool IsPrivate() const { return mIsPrivate; }
  bool IsSessionOnly() const { return mIsSessionOnly; }

private:
  friend class DOMStorageManager;
  friend class DOMStorageCache;

  nsRefPtr<DOMStorageManager> mManager;
  nsRefPtr<DOMStorageCache> mCache;
  nsString mDocumentURI;

  
  
  nsCOMPtr<nsIPrincipal> mPrincipal;

  
  bool mIsPrivate : 1;

  
  
  
  bool mIsSessionOnly : 1;

  void BroadcastChangeNotification(const nsSubstring& aKey,
                                   const nsSubstring& aOldValue,
                                   const nsSubstring& aNewValue);
};

} 
} 

#endif 
