





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

  JSObject* GetParentObject()
  {
    if (mScriptOwner) {
      return mScriptOwner;
    }

    
    nsCOMPtr<nsIScriptGlobalObject> parent;
    DOMEventTargetHelper::GetParentObject(getter_AddRefs(parent));

    return parent ? parent->GetGlobalJSObject() : nullptr;
  }

#ifdef DEBUG
  void AssertIsRooted() const;
#else
  inline void AssertIsRooted() const
  {
  }
#endif

protected:
  IDBWrapperCache(DOMEventTargetHelper* aOwner)
    : DOMEventTargetHelper(aOwner), mScriptOwner(nullptr)
  { }
  IDBWrapperCache(nsPIDOMWindow* aOwner)
    : DOMEventTargetHelper(aOwner), mScriptOwner(nullptr)
  { }

  virtual ~IDBWrapperCache();

private:
  JS::Heap<JSObject*> mScriptOwner;
};

END_INDEXEDDB_NAMESPACE

#endif 
