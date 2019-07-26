





#include "IDBWrapperCache.h"
#include "nsCycleCollector.h"

USING_INDEXEDDB_NAMESPACE

NS_IMPL_CYCLE_COLLECTION_CLASS(IDBWrapperCache)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(IDBWrapperCache,
                                                  DOMEventTargetHelper)
  
  
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(IDBWrapperCache,
                                                DOMEventTargetHelper)
  if (tmp->mScriptOwner) {
    tmp->mScriptOwner = nullptr;
    mozilla::DropJSObjects(tmp);
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(IDBWrapperCache,
                                               DOMEventTargetHelper)
  
  
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mScriptOwner)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(IDBWrapperCache)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(IDBWrapperCache, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(IDBWrapperCache, DOMEventTargetHelper)

IDBWrapperCache::~IDBWrapperCache()
{
  mScriptOwner = nullptr;
  ReleaseWrapper(this);
  mozilla::DropJSObjects(this);
}

void
IDBWrapperCache::SetScriptOwner(JSObject* aScriptOwner)
{
  NS_ASSERTION(aScriptOwner, "This should never be null!");

  mScriptOwner = aScriptOwner;
  mozilla::HoldJSObjects(this);
}

#ifdef DEBUG
void
IDBWrapperCache::AssertIsRooted() const
{
  MOZ_ASSERT(cyclecollector::IsJSHolder(const_cast<IDBWrapperCache*>(this)),
             "Why aren't we rooted?!");
}
#endif
