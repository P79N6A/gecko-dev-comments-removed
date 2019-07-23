




































#ifndef nsTransactionItem_h__
#define nsTransactionItem_h__

class nsITransaction;
class nsTransactionStack;
class nsTransactionRedoStack;
class nsTransactionManager;

class nsTransactionItem
{
  nsITransaction         *mTransaction;
  nsTransactionStack     *mUndoStack;
  nsTransactionRedoStack *mRedoStack;

public:

  nsTransactionItem(nsITransaction *aTransaction);
  virtual ~nsTransactionItem();

  virtual nsresult AddChild(nsTransactionItem *aTransactionItem);
  virtual nsresult GetTransaction(nsITransaction **aTransaction);
  virtual nsresult GetIsBatch(PRBool *aIsBatch);
  virtual nsresult GetNumberOfChildren(PRInt32 *aNumChildren);
  virtual nsresult GetChild(PRInt32 aIndex, nsTransactionItem **aChild);

  virtual nsresult DoTransaction(void);
  virtual nsresult UndoTransaction(nsTransactionManager *aTxMgr);
  virtual nsresult RedoTransaction(nsTransactionManager *aTxMgr);

private:

  virtual nsresult UndoChildren(nsTransactionManager *aTxMgr);
  virtual nsresult RedoChildren(nsTransactionManager *aTxMgr);

  virtual nsresult RecoverFromUndoError(nsTransactionManager *aTxMgr);
  virtual nsresult RecoverFromRedoError(nsTransactionManager *aTxMgr);

  virtual nsresult GetNumberOfUndoItems(PRInt32 *aNumItems);
  virtual nsresult GetNumberOfRedoItems(PRInt32 *aNumItems);
};

#endif 
