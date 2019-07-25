





#include "IDBWrapperCache.h"

USING_INDEXEDDB_NAMESPACE

NS_IMPL_CYCLE_COLLECTION_CLASS(IDBWrapperCache)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(IDBWrapperCache,
                                                  nsDOMEventTargetHelper)
  
  
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(IDBWrapperCache,
                                                nsDOMEventTargetHelper)
  tmp->mScriptOwner = nsnull;
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(IDBWrapperCache,
                                               nsDOMEventTargetHelper)
  
  
  if (tmp->mScriptOwner) {
    NS_IMPL_CYCLE_COLLECTION_TRACE_JS_CALLBACK(tmp->mScriptOwner,
                                               "mScriptOwner")
  }
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(IDBWrapperCache)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(IDBWrapperCache, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(IDBWrapperCache, nsDOMEventTargetHelper)
