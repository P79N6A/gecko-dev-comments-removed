




































#ifndef DeleteRangeTxn_h__
#define DeleteRangeTxn_h__

#include "EditAggregateTxn.h"
#include "nsIDOMNode.h"
#include "nsIDOMRange.h"
#include "nsIEditor.h"
#include "nsCOMPtr.h"

class nsIDOMRange;
class nsIEditor;
class nsRangeUpdater;




class DeleteRangeTxn : public EditAggregateTxn
{
public:
  



  NS_IMETHOD Init(nsIEditor *aEditor, 
                  nsIDOMRange *aRange,
                  nsRangeUpdater *aRangeUpdater);

  DeleteRangeTxn();

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
};

#endif
