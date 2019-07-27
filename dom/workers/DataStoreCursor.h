



#ifndef mozilla_dom_workers_DataStoreCursor_h
#define mozilla_dom_workers_DataStoreCursor_h

#include "nsProxyRelease.h"

namespace mozilla {

class ErrorResult;

namespace dom {

class Promise;
class GlobalObject;
class DataStoreCursor;
class DataStoreCursorImpl;

namespace workers {

class WorkerDataStore;

class WorkerDataStoreCursor MOZ_FINAL
{
public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WorkerDataStoreCursor)
  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(WorkerDataStoreCursor)

  explicit WorkerDataStoreCursor(WorkerDataStore* aWorkerStore);

  

  static already_AddRefed<WorkerDataStoreCursor> Constructor(GlobalObject& aGlobal,
                                                             ErrorResult& aRv);

  JSObject* WrapObject(JSContext *aCx);

  

  already_AddRefed<WorkerDataStore> GetStore(JSContext *aCx, ErrorResult& aRv);

  already_AddRefed<Promise> Next(JSContext *aCx, ErrorResult& aRv);

  void Close(JSContext *aCx, ErrorResult& aRv);

  
  void SetDataStoreCursorImpl(DataStoreCursorImpl& aCursor);

  void SetBackingDataStoreCursor(
    const nsMainThreadPtrHandle<DataStoreCursor>& aBackingCursor);

protected:
  virtual ~WorkerDataStoreCursor() {}

private:
  nsMainThreadPtrHandle<DataStoreCursor> mBackingCursor;

  
  nsRefPtr<WorkerDataStore> mWorkerStore;
};

} 
} 
} 

#endif
