




#ifndef DeleteRangeTxn_h__
#define DeleteRangeTxn_h__

#include "EditAggregateTxn.h"
#include "nsRange.h"
#include "nsEditor.h"
#include "nsCOMPtr.h"

class nsRangeUpdater;




class DeleteRangeTxn : public EditAggregateTxn
{
public:
  



  nsresult Init(nsEditor* aEditor,
                nsRange* aRange,
                nsRangeUpdater* aRangeUpdater);

  DeleteRangeTxn();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DeleteRangeTxn, EditAggregateTxn)
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction();

protected:

  nsresult CreateTxnsToDeleteBetween(nsINode* aNode,
                                     PRInt32 aStartOffset,
                                     PRInt32 aEndOffset);

  nsresult CreateTxnsToDeleteNodesBetween();

  nsresult CreateTxnsToDeleteContent(nsINode* aParent,
                                     PRInt32 aOffset,
                                     nsIEditor::EDirection aAction);

protected:

  
  nsRefPtr<nsRange> mRange;

  
  nsEditor* mEditor;

  
  nsRangeUpdater* mRangeUpdater;
};

#endif
