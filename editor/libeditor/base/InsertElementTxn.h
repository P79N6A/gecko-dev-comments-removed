




































#ifndef InsertElementTxn_h__
#define InsertElementTxn_h__

#include "EditTxn.h"
#include "nsIEditor.h"
#include "nsIDOMNode.h"
#include "nsCOMPtr.h"




class InsertElementTxn : public EditTxn
{
public:
  




  NS_IMETHOD Init(nsIDOMNode *aNode,
                  nsIDOMNode *aParent,
                  PRInt32     aOffset,
                  nsIEditor  *aEditor);

  InsertElementTxn();

  NS_DECL_EDITTXN

protected:
  
  
  nsCOMPtr<nsIDOMNode> mNode;

  
  nsCOMPtr<nsIDOMNode> mParent;

  
  nsIEditor*           mEditor;

  
  PRInt32 mOffset;
};

#endif
