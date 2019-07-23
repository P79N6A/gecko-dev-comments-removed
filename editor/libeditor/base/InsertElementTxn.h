




































#ifndef InsertElementTxn_h__
#define InsertElementTxn_h__

#include "EditTxn.h"
#include "nsIEditor.h"
#include "nsIDOMNode.h"
#include "nsCOMPtr.h"

#define INSERT_ELEMENT_TXN_CID \
{/* b5762440-cbb0-11d2-86db-000064657374 */ \
0xb5762440, 0xcbb0, 0x11d2, \
{0x86, 0xdb, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74} }




class InsertElementTxn : public EditTxn
{
public:

  static const nsIID& GetCID() { static const nsIID iid = INSERT_ELEMENT_TXN_CID; return iid; }

  




  NS_IMETHOD Init(nsIDOMNode *aNode,
                  nsIDOMNode *aParent,
                  PRInt32     aOffset,
                  nsIEditor  *aEditor);

private:
  InsertElementTxn();

public:
  NS_DECL_EDITTXN

protected:
  
  
  nsCOMPtr<nsIDOMNode> mNode;

  
  nsCOMPtr<nsIDOMNode> mParent;

  
  nsIEditor*           mEditor;

  
  PRInt32 mOffset;

  friend class TransactionFactory;

};

#endif
