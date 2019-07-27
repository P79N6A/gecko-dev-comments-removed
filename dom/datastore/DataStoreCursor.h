



#ifndef mozilla_dom_DataStoreCursor_h
#define mozilla_dom_DataStoreCursor_h

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"

namespace mozilla {

class ErrorResult;

namespace dom {

class Promise;
class DataStore;
class GlobalObject;
class DataStoreCursorImpl;

class DataStoreCursor final : public nsISupports
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DataStoreCursor)

  

  static already_AddRefed<DataStoreCursor> Constructor(GlobalObject& aGlobal,
                                                       ErrorResult& aRv);

  bool WrapObject(JSContext *aCx, JS::Handle<JSObject*> aGivenProto, JS::MutableHandle<JSObject*> aReflector);

  

  already_AddRefed<DataStore> GetStore(ErrorResult& aRv);

  already_AddRefed<Promise> Next(ErrorResult& aRv);

  void Close(ErrorResult& aRv);

  
  
  
  void SetDataStoreCursorImpl(DataStoreCursorImpl& aCursor);

private:
  ~DataStoreCursor() {}
  nsRefPtr<DataStoreCursorImpl> mCursor;
};

} 
} 

#endif
