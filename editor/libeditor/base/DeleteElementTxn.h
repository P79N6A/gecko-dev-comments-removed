




































#ifndef DeleteElementTxn_h__
#define DeleteElementTxn_h__

#include "EditTxn.h"

#include "nsIDOMNode.h"
#include "nsIEditor.h"
#include "nsCOMPtr.h"

class nsRangeUpdater;




class DeleteElementTxn : public EditTxn
{
public:
  


  NS_IMETHOD Init(nsIEditor *aEditor, nsIDOMNode *aElement, nsRangeUpdater *aRangeUpdater);

  DeleteElementTxn();

  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction();

protected:
  
  
  nsCOMPtr<nsIDOMNode> mElement;

  
  nsCOMPtr<nsIDOMNode> mParent;

  
  nsCOMPtr<nsIDOMNode> mRefNode;

  
  nsIEditor* mEditor;

  
  nsRangeUpdater *mRangeUpdater;
};

#endif
