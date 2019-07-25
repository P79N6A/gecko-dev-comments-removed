






































#ifndef mozilla_dom_indexeddb_idbdatabase_h__
#define mozilla_dom_indexeddb_idbdatabase_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIDBDatabase.h"

#include "nsCycleCollectionParticipant.h"
#include "nsDOMEventTargetHelper.h"
#include "nsDOMLists.h"
#include "nsIDocument.h"

class nsIScriptContext;
class nsPIDOMWindow;

BEGIN_INDEXEDDB_NAMESPACE

class AsyncConnectionHelper;
struct DatabaseInfo;
class IDBIndex;
class IDBObjectStore;
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
         already_AddRefed<DatabaseInfo> aDatabaseInfo,
         const nsACString& aASCIIOrigin);

  
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

  nsIAtom* Id() const
  {
    return mDatabaseId;
  }

  DatabaseInfo* Info() const
  {
    return mDatabaseInfo;
  }

  const nsString& Name()
  {
    return mName;
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

  already_AddRefed<nsIDocument> GetOwnerDocument()
  {
    NS_ASSERTION(mOwner, "This should never be null!");
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(mOwner->GetExtantDocument());
    return doc.forget();
  }

  bool IsQuotaDisabled();

  nsCString& Origin()
  {
    return mASCIIOrigin;
  }

  void Invalidate();

  
  
  bool IsInvalidated();

  void CloseInternal(bool aIsDead);

  
  bool IsClosed();

  void EnterSetVersionTransaction();
  void ExitSetVersionTransaction();

private:
  IDBDatabase();
  ~IDBDatabase();

  void OnUnlink();

  nsRefPtr<DatabaseInfo> mDatabaseInfo;
  nsCOMPtr<nsIAtom> mDatabaseId;
  nsString mName;
  nsString mFilePath;
  nsCString mASCIIOrigin;

  PRInt32 mInvalidated;
  bool mRegistered;
  bool mClosed;
  bool mRunningVersionChange;

  
  nsRefPtr<nsDOMEventListenerWrapper> mOnErrorListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnVersionChangeListener;
};

END_INDEXEDDB_NAMESPACE

#endif 
