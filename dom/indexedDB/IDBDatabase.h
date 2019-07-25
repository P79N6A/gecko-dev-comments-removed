






































#ifndef mozilla_dom_indexeddb_idbdatabase_h__
#define mozilla_dom_indexeddb_idbdatabase_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIDBDatabase.h"

#include "nsCycleCollectionParticipant.h"
#include "nsDOMEventTargetHelper.h"
#include "nsDOMLists.h"

class nsIScriptContext;
class nsPIDOMWindow;

BEGIN_INDEXEDDB_NAMESPACE

class AsyncConnectionHelper;
struct DatabaseInfo;
class IDBTransaction;
class IndexedDatabaseManager;

class IDBDatabase : public nsDOMEventTargetHelper,
                    public nsIIDBDatabase
{
  friend class AsyncConnectionHelper;
  friend class IndexedDatabaseManager;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIIDBDATABASE

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBDatabase,
                                           nsDOMEventTargetHelper)

  static already_AddRefed<IDBDatabase>
  Create(nsIScriptContext* aScriptContext,
         nsPIDOMWindow* aOwner,
         DatabaseInfo* aDatabaseInfo,
         const nsACString& aASCIIOrigin);

  PRUint32 Id()
  {
    return mDatabaseId;
  }

  const nsString& FilePath()
  {
    return mFilePath;
  }

  nsIScriptContext* ScriptContext()
  {
    NS_ASSERTION(mScriptContext, "This should never be null!");
    return mScriptContext;
  }

  nsPIDOMWindow* Owner()
  {
    NS_ASSERTION(mOwner, "This should never be null!");
    return mOwner;
  }

  bool IsQuotaDisabled();

  nsCString& Origin()
  {
    return mASCIIOrigin;
  }

  void Invalidate();
  bool IsInvalidated();

private:
  IDBDatabase();
  ~IDBDatabase();

  PRUint32 mDatabaseId;
  nsString mName;
  nsString mDescription;
  nsString mFilePath;
  nsCString mASCIIOrigin;
  PRInt32 mInvalidated;
  bool mRegistered;

  
  nsRefPtr<nsDOMEventListenerWrapper> mOnErrorListener;
};

END_INDEXEDDB_NAMESPACE

#endif 
