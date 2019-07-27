




#ifndef nsDOMStorage_h___
#define nsDOMStorage_h___

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsIDOMStorage.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWeakReference.h"
#include "nsWrapperCache.h"
#include "nsISupports.h"

class nsIPrincipal;
class nsIDOMWindow;

namespace mozilla {
namespace dom {

class DOMStorageManager;
class DOMStorageCache;

class DOMStorage final
  : public nsIDOMStorage
  , public nsSupportsWeakReference
  , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(DOMStorage,
                                                         nsIDOMStorage)

  enum StorageType {
    LocalStorage = 1,
    SessionStorage = 2
  };

  StorageType GetType() const;

  DOMStorageManager* GetManager() const
  {
    return mManager;
  }

  DOMStorageCache const* GetCache() const
  {
    return mCache;
  }

  nsIPrincipal* GetPrincipal();
  bool PrincipalEquals(nsIPrincipal* aPrincipal);
  bool CanAccess(nsIPrincipal* aPrincipal);
  bool IsPrivate()
  {
    return mIsPrivate;
  }

  DOMStorage(nsIDOMWindow* aWindow,
             DOMStorageManager* aManager,
             DOMStorageCache* aCache,
             const nsAString& aDocumentURI,
             nsIPrincipal* aPrincipal,
             bool aIsPrivate);

  
  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  nsIDOMWindow* GetParentObject() const
  {
    return mWindow;
  }

  uint32_t GetLength(ErrorResult& aRv);

  void Key(uint32_t aIndex, nsAString& aResult, ErrorResult& aRv);

  void GetItem(const nsAString& aKey, nsAString& aResult, ErrorResult& aRv);

  bool NameIsEnumerable(const nsAString& aName) const
  {
    return true;
  }

  void GetSupportedNames(unsigned, nsTArray<nsString>& aKeys);

  void NamedGetter(const nsAString& aKey, bool& aFound, nsAString& aResult,
	           ErrorResult& aRv)
  {
    GetItem(aKey, aResult, aRv);
    aFound = !aResult.IsVoid();
  }

  void SetItem(const nsAString& aKey, const nsAString& aValue,
               ErrorResult& aRv);

  void NamedSetter(const nsAString& aKey, const nsAString& aValue,
                   ErrorResult& aRv)
  {
    SetItem(aKey, aValue, aRv);
  }

  void RemoveItem(const nsAString& aKey, ErrorResult& aRv);

  void NamedDeleter(const nsAString& aKey, bool& aFound, ErrorResult& aRv)
  {
    RemoveItem(aKey, aRv);

    aFound = !aRv.ErrorCodeIs(NS_SUCCESS_DOM_NO_OPERATION);
  }

  void Clear(ErrorResult& aRv);

  
  
  
  
  
  
  
  static bool CanUseStorage(DOMStorage* aStorage = nullptr);

  bool IsPrivate() const { return mIsPrivate; }
  bool IsSessionOnly() const { return mIsSessionOnly; }

  bool IsForkOf(const DOMStorage* aOther) const
  {
    MOZ_ASSERT(aOther);
    return mCache == aOther->mCache;
  }

private:
  ~DOMStorage();

  friend class DOMStorageManager;
  friend class DOMStorageCache;

  nsCOMPtr<nsIDOMWindow> mWindow;
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
