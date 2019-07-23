




































#ifndef CreateElementTxn_h__
#define CreateElementTxn_h__

#include "EditTxn.h"
#include "nsEditor.h"
#include "nsIDOMNode.h"
#include "nsCOMPtr.h"

#define CREATE_ELEMENT_TXN_CID \
{/* 7a6393c0-ac48-11d2-86d8-000064657374 */ \
0x7a6393c0, 0xac48, 0x11d2, \
{0x86, 0xd8, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74} }




class CreateElementTxn : public EditTxn
{
public:

  static const nsIID& GetCID() { static const nsIID iid = CREATE_ELEMENT_TXN_CID; return iid; }

  enum { eAppend=-1 };

  






  NS_IMETHOD Init(nsEditor *aEditor,
                  const nsAString& aTag,
                  nsIDOMNode *aParent,
                  PRUint32 aOffsetInParent);

private:
  CreateElementTxn();

public:
  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction();

  NS_IMETHOD GetNewNode(nsIDOMNode **aNewNode);

protected:
  
  
  nsEditor* mEditor;
  
  
  nsString mTag;

  
  nsCOMPtr<nsIDOMNode> mParent;

  
  PRUint32 mOffsetInParent;

  
  nsCOMPtr<nsIDOMNode> mNewNode;  

  
  nsCOMPtr<nsIDOMNode> mRefNode;

  friend class TransactionFactory;

};

#endif
