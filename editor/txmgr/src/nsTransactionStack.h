




































#ifndef nsTransactionStack_h__
#define nsTransactionStack_h__

#include "nsDeque.h"
#include "nsCOMPtr.h"

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
  already_AddRefed<nsTransactionItem> GetItem(PRInt32 aIndex);
  void Clear();
  PRInt32 GetSize() { return mQue.GetSize(); }

  void DoUnlink() { Clear(); }
  void DoTraverse(nsCycleCollectionTraversalCallback &cb);

private:
  nsDeque mQue;
  const Type mType;
};

#endif
