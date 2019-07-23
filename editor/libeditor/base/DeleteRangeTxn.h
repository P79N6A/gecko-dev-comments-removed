




































#ifndef DeleteRangeTxn_h__
#define DeleteRangeTxn_h__

#include "EditAggregateTxn.h"
#include "nsIDOMNode.h"
#include "nsIDOMRange.h"
#include "nsIEditor.h"
#include "nsCOMPtr.h"

#define DELETE_RANGE_TXN_CID \
{/* 5ec6b260-ac49-11d2-86d8-000064657374 */ \
0x5ec6b260, 0xac49, 0x11d2, \
{0x86, 0xd8, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74} }

class nsIDOMRange;
class nsIEditor;
class nsRangeUpdater;




class DeleteRangeTxn : public EditAggregateTxn
{
public:

  static const nsIID& GetCID() { static const nsIID iid = DELETE_RANGE_TXN_CID; return iid; }

  



  NS_IMETHOD Init(nsIEditor *aEditor, 
                  nsIDOMRange *aRange,
                  nsRangeUpdater *aRangeUpdater);

private:
  DeleteRangeTxn();

public:
  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction();

protected:

  NS_IMETHOD CreateTxnsToDeleteBetween(nsIDOMNode *aStartParent, 
                                             PRUint32    aStartOffset, 
                                             PRUint32    aEndOffset);

  NS_IMETHOD CreateTxnsToDeleteNodesBetween();

  NS_IMETHOD CreateTxnsToDeleteContent(nsIDOMNode *aParent, 
                                             PRUint32 aOffset, 
                                             nsIEditor::EDirection aAction);
  
protected:
  
  
  nsCOMPtr<nsIDOMRange> mRange;			

  
  nsCOMPtr<nsIDOMNode> mStartParent;

  
  PRInt32 mStartOffset;

  
  nsCOMPtr<nsIDOMNode> mEndParent;

  
  nsCOMPtr<nsIDOMNode> mCommonParent;

  
  PRInt32 mEndOffset;

  
  nsIEditor* mEditor;

  
  nsRangeUpdater *mRangeUpdater;
  
  friend class TransactionFactory;

};

#endif
