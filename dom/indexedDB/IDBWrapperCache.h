





#ifndef mozilla_dom_indexeddb_idbwrappercache_h__
#define mozilla_dom_indexeddb_idbwrappercache_h__

#include "js/RootingAPI.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {
namespace indexedDB {

class IDBWrapperCache : public DOMEventTargetHelper
{
  JS::Heap<JSObject*> mScriptOwner;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(IDBWrapperCache,
                                                         DOMEventTargetHelper)

  JSObject*
  GetScriptOwner() const
  {
    return mScriptOwner;
  }

  void
  SetScriptOwner(JSObject* aScriptOwner);

  void AssertIsRooted() const
#ifdef DEBUG
  ;
#else
  { }
#endif

protected:
  explicit IDBWrapperCache(DOMEventTargetHelper* aOwner);
  explicit IDBWrapperCache(nsPIDOMWindow* aOwner);

  virtual ~IDBWrapperCache();
};

} 
} 
} 

#endif 
