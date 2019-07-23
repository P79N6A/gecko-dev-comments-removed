




































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

class nsHTMLEditor;
class IMETextTxn;







 
class PlaceholderTxn : public EditAggregateTxn, 
                       public nsIAbsorbingTransaction, 
                       public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS_INHERITED  
  
  PlaceholderTxn();



  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction();
  NS_IMETHOD Merge(nsITransaction *aTransaction, PRBool *aDidMerge);



  NS_IMETHOD Init(nsIAtom *aName, nsSelectionState *aSelState, nsIEditor *aEditor);
  
  NS_IMETHOD GetTxnName(nsIAtom **aName);
  
  NS_IMETHOD StartSelectionEquals(nsSelectionState *aSelState, PRBool *aResult);

  NS_IMETHOD EndPlaceHolderBatch();

  NS_IMETHOD ForwardEndBatchTo(nsIAbsorbingTransaction *aForwardingAddress);

  NS_IMETHOD Commit();

  NS_IMETHOD RememberEndingSelection();

protected:

  
  PRBool      mAbsorb;          
  nsWeakPtr   mForwarding;
  IMETextTxn *mIMETextTxn;      
                                
  PRBool      mCommitted;       
  
  
  
  
  nsAutoPtr<nsSelectionState> mStartSel; 
  nsSelectionState  mEndSel;
  nsIEditor*        mEditor;   
};


#endif
