




#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsISupportsUtils.h"
#include "nsTransactionItem.h"
#include "nsTransactionStack.h"
#include "nscore.h"

nsTransactionStack::nsTransactionStack(nsTransactionStack::Type aType)
  : mQue(0)
  , mType(aType)
{
}

nsTransactionStack::~nsTransactionStack()
{
  Clear();
}

void
nsTransactionStack::Push(nsTransactionItem *aTransaction)
{
  if (!aTransaction) {
    return;
  }

  


  NS_ADDREF(aTransaction);
  mQue.Push(aTransaction);
}

already_AddRefed<nsTransactionItem>
nsTransactionStack::Pop()
{
  


  return static_cast<nsTransactionItem*> (mQue.Pop());
}

already_AddRefed<nsTransactionItem>
nsTransactionStack::PopBottom()
{
  


  return static_cast<nsTransactionItem*> (mQue.PopFront());
}

already_AddRefed<nsTransactionItem>
nsTransactionStack::Peek()
{
  nsTransactionItem* transaction = nullptr;
  if (mQue.GetSize()) {
    NS_IF_ADDREF(transaction = static_cast<nsTransactionItem*>(mQue.Last()));
  }

  return transaction;
}

already_AddRefed<nsTransactionItem>
nsTransactionStack::GetItem(int32_t aIndex)
{
  nsTransactionItem* transaction = nullptr;
  if (aIndex >= 0 && aIndex < mQue.GetSize()) {
    NS_IF_ADDREF(transaction =
                 static_cast<nsTransactionItem*>(mQue.ObjectAt(aIndex)));
  }

  return transaction;
}

void
nsTransactionStack::Clear()
{
  nsRefPtr<nsTransactionItem> tx;

  do {
    tx = mType == FOR_UNDO ? Pop() : PopBottom();
  } while (tx);
}

void
nsTransactionStack::DoTraverse(nsCycleCollectionTraversalCallback &cb)
{
  for (int32_t i = 0, qcount = mQue.GetSize(); i < qcount; ++i) {
    nsTransactionItem *item =
      static_cast<nsTransactionItem*>(mQue.ObjectAt(i));
    if (item) {
      NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "transaction stack mQue[i]");
      cb.NoteNativeChild(item, NS_CYCLE_COLLECTION_PARTICIPANT(nsTransactionItem));
    }
  }
}
