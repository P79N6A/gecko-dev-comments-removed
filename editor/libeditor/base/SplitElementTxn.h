




































#ifndef SplitElementTxn_h__
#define SplitElementTxn_h__

#include "EditTxn.h"
#include "nsIDOMNode.h"
#include "nsCOMPtr.h"
#include "nsIEditor.h"

#define SPLIT_ELEMENT_TXN_CID \
{/* 690c6290-ac48-11d2-86d8-000064657374 */ \
0x690c6290, 0xac48, 0x11d2, \
{0x86, 0xd8, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74} }

class nsEditor;





class SplitElementTxn : public EditTxn
{
public:

  static const nsIID& GetCID() { static const nsIID iid = SPLIT_ELEMENT_TXN_CID; return iid; }

  






  NS_IMETHOD Init (nsEditor   *aEditor,
                   nsIDOMNode *aNode,
                   PRInt32     aOffset);
protected:
  SplitElementTxn();

public:
  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction(void);

  NS_IMETHOD GetNewNode(nsIDOMNode **aNewNode);

protected:
  
  
  nsCOMPtr<nsIDOMNode> mExistingRightNode;

  



  PRInt32  mOffset;

  
  nsCOMPtr<nsIDOMNode> mNewLeftNode;

  
  nsCOMPtr<nsIDOMNode> mParent;
  nsEditor*  mEditor;

  friend class TransactionFactory;

};

#endif
