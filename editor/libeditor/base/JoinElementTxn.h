




































#ifndef JoinElementTxn_h__
#define JoinElementTxn_h__

#include "EditTxn.h"
#include "nsIDOMNode.h"
#include "nsCOMPtr.h"
#include "nsIEditor.h"

#define JOIN_ELEMENT_TXN_CID \
{/* 9bc5f9f0-ac48-11d2-86d8-000064657374 */ \
0x9bc5f9f0, 0xac48, 0x11d2, \
{0x86, 0xd8, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74} }

class nsEditor;








class JoinElementTxn : public EditTxn
{
public:

  static const nsIID& GetCID() { static const nsIID iid = JOIN_ELEMENT_TXN_CID; return iid; }

  




  NS_IMETHOD Init(nsEditor   *aEditor,
                  nsIDOMNode *aLeftNode,
                  nsIDOMNode *aRightNode);
protected:
  JoinElementTxn();

public:
  NS_DECL_EDITTXN

protected:
  
  


  nsCOMPtr<nsIDOMNode> mLeftNode;
  nsCOMPtr<nsIDOMNode> mRightNode;

  



  PRUint32  mOffset;

  
  nsCOMPtr<nsIDOMNode> mParent;
  nsEditor*  mEditor;

  friend class TransactionFactory;

};

#endif
