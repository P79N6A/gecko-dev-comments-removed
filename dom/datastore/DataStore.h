



#ifndef mozilla_dom_DataStore_h
#define mozilla_dom_DataStore_h

#include "mozilla/DOMEventTargetHelper.h"

namespace mozilla {

class ErrorResult;

namespace dom {

class Promise;
class DataStoreCursor;
class DataStoreImpl;
class StringOrUnsignedLong;
class OwningStringOrUnsignedLong;

class DataStore MOZ_FINAL : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DataStore,
                                           DOMEventTargetHelper)

  explicit DataStore(nsPIDOMWindow* aWindow);

  

  static already_AddRefed<DataStore> Constructor(GlobalObject& aGlobal,
                                                 ErrorResult& aRv);

  virtual JSObject* WrapObject(JSContext *aCx) MOZ_OVERRIDE;

  static bool EnabledForScope(JSContext* aCx, JS::Handle<JSObject*> aObj);

  

  void GetName(nsAString& aName, ErrorResult& aRv);

  void GetOwner(nsAString& aOwner, ErrorResult& aRv);

  bool GetReadOnly(ErrorResult& aRv);

  already_AddRefed<Promise> Get(const Sequence<OwningStringOrUnsignedLong>& aId,
                                ErrorResult& aRv);

  already_AddRefed<Promise> Put(JSContext* aCx,
                                JS::Handle<JS::Value> aObj,
                                const StringOrUnsignedLong& aId,
                                const nsAString& aRevisionId,
                                ErrorResult& aRv);

  already_AddRefed<Promise> Add(JSContext* aCx,
                                JS::Handle<JS::Value> aObj,
                                const Optional<StringOrUnsignedLong>& aId,
                                const nsAString& aRevisionId,
                                ErrorResult& aRv);

  already_AddRefed<Promise> Remove(const StringOrUnsignedLong& aId,
                                   const nsAString& aRevisionId,
                                   ErrorResult& aRv);

  already_AddRefed<Promise> Clear(const nsAString& aRevisionId,
                                  ErrorResult& aRv);

  void GetRevisionId(nsAString& aRevisionId, ErrorResult& aRv);

  already_AddRefed<Promise> GetLength(ErrorResult& aRv);

  already_AddRefed<DataStoreCursor> Sync(const nsAString& aRevisionId,
                                         ErrorResult& aRv);

  IMPL_EVENT_HANDLER(change)

  
  
  
  
  void SetDataStoreImpl(DataStoreImpl& aStore, ErrorResult& aRv);

private:
  ~DataStore();

  nsRefPtr<DataStoreImpl> mStore;
};

} 
} 

#endif
