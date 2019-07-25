




































#ifndef nsDOMCaretPosition_h
#define nsDOMCaretPosition_h

#include "nsIDOMCaretPosition.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"

class nsDOMCaretPosition : public nsIDOMCaretPosition
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsDOMCaretPosition)
  NS_DECL_NSIDOMCARETPOSITION

  nsDOMCaretPosition(nsIDOMNode* aNode, PRUint32 aOffset);

protected:
  virtual ~nsDOMCaretPosition();
  PRUint32 mOffset;
  nsCOMPtr<nsIDOMNode> mNode;
};
#endif
