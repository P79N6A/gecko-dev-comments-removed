





#ifndef mozilla_dom_indexeddb_idbwrappercache_h__
#define mozilla_dom_indexeddb_idbwrappercache_h__

#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/indexedDB/IndexedDatabase.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBWrapperCache : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(
                                                   IDBWrapperCache,
                                                   DOMEventTargetHelper)

  JSObject* GetScriptOwner() const
  {
    return mScriptOwner;
  }
  void SetScriptOwner(JSObject* aScriptOwner);

#ifdef DEBUG
  void AssertIsRooted() const;
#else
  inline void AssertIsRooted() const
  {
  }
#endif

protected:
  explicit IDBWrapperCache(DOMEventTargetHelper* aOwner)
    : DOMEventTargetHelper(aOwner), mScriptOwner(nullptr)
  { }
  explicit IDBWrapperCache(nsPIDOMWindow* aOwner)
    : DOMEventTargetHelper(aOwner), mScriptOwner(nullptr)
  { }

  virtual ~IDBWrapperCache();

private:
  JS::Heap<JSObject*> mScriptOwner;
};

END_INDEXEDDB_NAMESPACE

#endif 
