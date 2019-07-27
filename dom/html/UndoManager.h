



#ifndef mozilla_dom_UndoManager_h
#define mozilla_dom_UndoManager_h

#include "mozilla/dom/UndoManagerBinding.h"

#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIContent.h"
#include "nsTArray.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/Nullable.h"

class nsITransactionManager;

namespace mozilla {
class ErrorResult;
namespace dom {

class DOMTransaction;
class DOMTransactionCallback;

class UndoManager final : public nsISupports,
                          public nsWrapperCache
{
  friend class TxnScopeGuard;
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(UndoManager)

  explicit UndoManager(nsIContent* aNode);

  void Transact(JSContext* aCx, DOMTransaction& aTransaction,
                bool aMerge, ErrorResult& aRv);
  void Undo(JSContext* aCx, ErrorResult& aRv);
  void Redo(JSContext* acx, ErrorResult& aRv);
  void Item(uint32_t aIndex,
            Nullable<nsTArray<nsRefPtr<DOMTransaction> > >& aItems,
            ErrorResult& aRv);
  uint32_t GetLength(ErrorResult& aRv);
  uint32_t GetPosition(ErrorResult& aRv);
  void ClearUndo(ErrorResult& aRv);
  void ClearRedo(ErrorResult& aRv);
  void Disconnect();

  nsISupports* GetParentObject() const
  {
    return mHostNode;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override
  {
    return mozilla::dom::UndoManagerBinding::Wrap(aCx, this, aGivenProto);
  }

  nsITransactionManager* GetTransactionManager();

protected:
  virtual ~UndoManager();
  nsCOMPtr<nsITransactionManager> mTxnManager;
  nsCOMPtr<nsIContent> mHostNode;

  


  void ManualTransact(DOMTransaction* aTransaction,
                      ErrorResult& aRv);

  



  void AutomaticTransact(DOMTransaction* aTransaction,
                         DOMTransactionCallback* aCallback,
                         ErrorResult& aRv);

  



  void ItemInternal(uint32_t aIndex,
                    nsTArray<DOMTransaction*>& aItems,
                    ErrorResult& aRv);

  




  void DispatchTransactionEvent(JSContext* aCx, const nsAString& aType,
                                uint32_t aPreviousPosition,
                                ErrorResult& aRv);
  bool mInTransaction;
  bool mIsDisconnected;
};

} 
} 

#endif
