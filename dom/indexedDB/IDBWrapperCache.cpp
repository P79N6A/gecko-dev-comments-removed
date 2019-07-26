





#include "IDBWrapperCache.h"
#include "nsContentUtils.h"

USING_INDEXEDDB_NAMESPACE

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(IDBWrapperCache,
                                                  nsDOMEventTargetHelper)
  
  
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(IDBWrapperCache,
                                                nsDOMEventTargetHelper)
  if (tmp->mScriptOwner) {
    tmp->mScriptOwner = nullptr;
    NS_DROP_JS_OBJECTS(tmp, IDBWrapperCache);
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(IDBWrapperCache,
                                               nsDOMEventTargetHelper)
  
  
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mScriptOwner)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(IDBWrapperCache)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(IDBWrapperCache, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(IDBWrapperCache, nsDOMEventTargetHelper)

IDBWrapperCache::~IDBWrapperCache()
{
  mScriptOwner = nullptr;
  nsContentUtils::ReleaseWrapper(this, this);
  NS_DROP_JS_OBJECTS(this, IDBWrapperCache);
}

void
IDBWrapperCache::SetScriptOwner(JSObject* aScriptOwner)
{
  NS_ASSERTION(aScriptOwner, "This should never be null!");

  mScriptOwner = aScriptOwner;

  nsISupports* thisSupports = NS_CYCLE_COLLECTION_UPCAST(this, IDBWrapperCache);
  nsXPCOMCycleCollectionParticipant* participant;
  CallQueryInterface(this, &participant);
  nsContentUtils::HoldJSObjects(thisSupports, participant);
}

#ifdef DEBUG
void
IDBWrapperCache::AssertIsRooted() const
{
  NS_ASSERTION(nsContentUtils::AreJSObjectsHeld(const_cast<IDBWrapperCache*>(this)),
               "Why aren't we rooted?!");
}
#endif
