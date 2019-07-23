




































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

#define PLACEHOLDER_TXN_CID \
{/* {0CE9FB00-D9D1-11d2-86DE-000064657374} */ \
0x0CE9FB00, 0xD9D1, 0x11d2, \
{0x86, 0xde, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74} }

class nsHTMLEditor;
class IMETextTxn;







 
class PlaceholderTxn : public EditAggregateTxn, 
                       public nsIAbsorbingTransaction, 
                       public nsSupportsWeakReference
{
public:

  static const nsIID& GetCID() { static const nsIID iid = PLACEHOLDER_TXN_CID; return iid; }

  NS_DECL_ISUPPORTS_INHERITED  
  
private:
  PlaceholderTxn();

public:


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
  
  friend class TransactionFactory;

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
