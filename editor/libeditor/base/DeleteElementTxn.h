




































#ifndef DeleteElementTxn_h__
#define DeleteElementTxn_h__

#include "EditTxn.h"

#include "nsIDOMNode.h"
#include "nsCOMPtr.h"

#define DELETE_ELEMENT_TXN_CID \
{/* 6fd77770-ac49-11d2-86d8-000064657374 */ \
0x6fd77770, 0xac49, 0x11d2, \
{0x86, 0xd8, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74} }

class nsRangeUpdater;




class DeleteElementTxn : public EditTxn
{
public:

  static const nsIID& GetCID() { static const nsIID iid = DELETE_ELEMENT_TXN_CID; return iid; }
 
  


  NS_IMETHOD Init(nsIDOMNode *aElement, nsRangeUpdater *aRangeUpdater);

private:
  DeleteElementTxn();

public:
  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction();

protected:
  
  
  nsCOMPtr<nsIDOMNode> mElement;

  
  nsCOMPtr<nsIDOMNode> mParent;

  
  nsCOMPtr<nsIDOMNode> mRefNode;

  
  nsRangeUpdater *mRangeUpdater;
  
  friend class TransactionFactory;

};

#endif
