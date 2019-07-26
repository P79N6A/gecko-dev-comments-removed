




#ifndef nsTransactionStack_h__
#define nsTransactionStack_h__

#include <deque>
#include "nsAutoPtr.h"

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
  int32_t GetSize() { return mDeque.size(); }

  void DoUnlink() { Clear(); }
  void DoTraverse(nsCycleCollectionTraversalCallback &cb);

private:
  std::deque<nsRefPtr<nsTransactionItem> > mDeque;
  const Type mType;
};

#endif
