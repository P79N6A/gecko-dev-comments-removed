




#ifndef AggregatePlaceholderTxn_h__
#define AggregatePlaceholderTxn_h__

#include "EditAggregateTxn.h"
#include "nsEditorUtils.h"
#include "nsIAbsorbingTransaction.h"
#include "nsIDOMNode.h"
#include "nsCOMPtr.h"
#include "nsWeakPtr.h"
#include "nsWeakReference.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace dom {
class IMETextTxn;
}
}







 
class PlaceholderTxn : public EditAggregateTxn, 
                       public nsIAbsorbingTransaction, 
                       public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS_INHERITED  
  
  PlaceholderTxn();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(PlaceholderTxn, EditAggregateTxn)


  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction() override;
  NS_IMETHOD Merge(nsITransaction *aTransaction, bool *aDidMerge) override;



  NS_IMETHOD Init(nsIAtom* aName, nsSelectionState* aSelState,
                  nsEditor* aEditor) override;
  
  NS_IMETHOD GetTxnName(nsIAtom **aName) override;
  
  NS_IMETHOD StartSelectionEquals(nsSelectionState *aSelState, bool *aResult) override;

  NS_IMETHOD EndPlaceHolderBatch() override;

  NS_IMETHOD ForwardEndBatchTo(nsIAbsorbingTransaction *aForwardingAddress) override;

  NS_IMETHOD Commit() override;

  nsresult RememberEndingSelection();

protected:
  virtual ~PlaceholderTxn();

  
  bool        mAbsorb;          
  nsWeakPtr   mForwarding;
  mozilla::dom::IMETextTxn *mIMETextTxn;      
                                
  bool        mCommitted;       
  
  
  
  
  nsAutoPtr<nsSelectionState> mStartSel; 
  nsSelectionState  mEndSel;
  nsEditor*         mEditor;   
};


#endif
