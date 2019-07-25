





#ifndef mozilla_dom_indexeddb_idbwrappercache_h__
#define mozilla_dom_indexeddb_idbwrappercache_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsDOMEventTargetHelper.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBWrapperCache : public nsDOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(
                                                   IDBWrapperCache,
                                                   nsDOMEventTargetHelper)

  JSObject* GetScriptOwner() const
  {
    return mScriptOwner;
  }
  bool SetScriptOwner(JSObject* aScriptOwner);

  JSObject* GetParentObject()
  {
    if (mScriptOwner) {
      return mScriptOwner;
    }

    
    nsCOMPtr<nsIScriptGlobalObject> parent;
    nsDOMEventTargetHelper::GetParentObject(getter_AddRefs(parent));

    return parent ? parent->GetGlobalJSObject() : nsnull;
  }

  static IDBWrapperCache* FromSupports(nsISupports* aSupports)
  {
    return static_cast<IDBWrapperCache*>(
      nsDOMEventTargetHelper::FromSupports(aSupports));
  }

#ifdef DEBUG
  void AssertIsRooted() const;
#else
  inline void AssertIsRooted() const
  {
  }
#endif

protected:
  IDBWrapperCache()
  : mScriptOwner(nsnull)
  { }

  virtual ~IDBWrapperCache();

private:
  JSObject* mScriptOwner;
};

END_INDEXEDDB_NAMESPACE

#endif 
