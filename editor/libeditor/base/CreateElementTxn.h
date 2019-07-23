




































#ifndef CreateElementTxn_h__
#define CreateElementTxn_h__

#include "EditTxn.h"
#include "nsEditor.h"
#include "nsIDOMNode.h"
#include "nsCOMPtr.h"




class CreateElementTxn : public EditTxn
{
public:
  enum { eAppend=-1 };

  






  NS_IMETHOD Init(nsEditor *aEditor,
                  const nsAString& aTag,
                  nsIDOMNode *aParent,
                  PRUint32 aOffsetInParent);

  CreateElementTxn();

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
};

#endif
