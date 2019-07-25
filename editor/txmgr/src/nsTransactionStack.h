




#ifndef nsTransactionStack_h__
#define nsTransactionStack_h__

#include "nsCOMPtr.h"
#include "nsDeque.h"
#include "prtypes.h"

class nsCycleCollectionTraversalCallback;
class nsTransactionItem;

class nsTransactionStack
{
public:
  enum Type { FOR_UNDO, FOR_REDO };

  explicit nsTransactionStack(Type aType);
  ~nsTransactionStack();

  void Push(nsTransactionItem *aTransactionItem);
  already_AddRefed<nsTransactionItem> Pop();
  already_AddRefed<nsTransactionItem> PopBottom();
  already_AddRefed<nsTransactionItem> Peek();
  already_AddRefed<nsTransactionItem> GetItem(int32_t aIndex);
  void Clear();
  int32_t GetSize() { return mQue.GetSize(); }

  void DoUnlink() { Clear(); }
  void DoTraverse(nsCycleCollectionTraversalCallback &cb);

private:
  nsDeque mQue;
  const Type mType;
};

#endif
