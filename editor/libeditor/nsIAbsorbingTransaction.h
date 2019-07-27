




#ifndef nsIAbsorbingTransaction_h__
#define nsIAbsorbingTransaction_h__

#include "nsISupports.h"





#define NS_IABSORBINGTRANSACTION_IID \
{ /* a6cf9116-15b3-11d2-932e-00805f8add32 */ \
    0xa6cf9116, \
    0x15b3, \
    0x11d2, \
    {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32} }

class nsSelectionState;
class nsIAtom;





class nsIAbsorbingTransaction  : public nsISupports{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IABSORBINGTRANSACTION_IID)

  NS_IMETHOD Init(nsIAtom* aName, nsSelectionState* aSelState,
                  nsEditor* aEditor) = 0;
  
  NS_IMETHOD EndPlaceHolderBatch()=0;
  
  NS_IMETHOD GetTxnName(nsIAtom **aName)=0;

  NS_IMETHOD StartSelectionEquals(nsSelectionState *aSelState, bool *aResult)=0;

  NS_IMETHOD ForwardEndBatchTo(nsIAbsorbingTransaction *aForwardingAddress)=0;
  
  NS_IMETHOD Commit()=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIAbsorbingTransaction,
                              NS_IABSORBINGTRANSACTION_IID)

#endif 

