




































#ifndef SplitElementTxn_h__
#define SplitElementTxn_h__

#include "EditTxn.h"
#include "nsIDOMNode.h"
#include "nsCOMPtr.h"
#include "nsIEditor.h"

class nsEditor;





class SplitElementTxn : public EditTxn
{
public:
  






  NS_IMETHOD Init (nsEditor   *aEditor,
                   nsIDOMNode *aNode,
                   PRInt32     aOffset);

  SplitElementTxn();

  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction(void);

  NS_IMETHOD GetNewNode(nsIDOMNode **aNewNode);

protected:
  
  
  nsCOMPtr<nsIDOMNode> mExistingRightNode;

  



  PRInt32  mOffset;

  
  nsCOMPtr<nsIDOMNode> mNewLeftNode;

  
  nsCOMPtr<nsIDOMNode> mParent;
  nsEditor*  mEditor;
};

#endif
