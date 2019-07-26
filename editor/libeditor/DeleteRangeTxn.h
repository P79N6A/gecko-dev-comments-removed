




#ifndef DeleteRangeTxn_h__
#define DeleteRangeTxn_h__

#include "EditAggregateTxn.h"
#include "EditTxn.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsID.h"
#include "nsIEditor.h"
#include "nsISupportsImpl.h"
#include "nsRange.h"
#include "nscore.h"

class nsEditor;
class nsINode;
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

  virtual void LastRelease()
  {
    mRange = nullptr;
    EditAggregateTxn::LastRelease();
  }
protected:

  nsresult CreateTxnsToDeleteBetween(nsINode* aNode,
                                     int32_t aStartOffset,
                                     int32_t aEndOffset);

  nsresult CreateTxnsToDeleteNodesBetween();

  nsresult CreateTxnsToDeleteContent(nsINode* aParent,
                                     int32_t aOffset,
                                     nsIEditor::EDirection aAction);

protected:

  
  nsRefPtr<nsRange> mRange;

  
  nsEditor* mEditor;

  
  nsRangeUpdater* mRangeUpdater;
};

#endif
