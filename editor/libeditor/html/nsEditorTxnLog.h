




































#ifndef nsEditorTxnLog_h__
#define nsEditorTxnLog_h__

#include "nsITransaction.h"
#include "nsITransactionManager.h"
#include "nsITransactionListener.h"

class nsHTMLEditorLog;




class nsEditorTxnLog : public nsITransactionListener
{
private:

  nsHTMLEditorLog *mEditorLog;
  PRInt32 mIndentLevel;
  PRInt32 mBatchCount;

public:

  

  nsEditorTxnLog(nsHTMLEditorLog *aEditorLog=0);

  

  virtual ~nsEditorTxnLog();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSITRANSACTIONLISTENER

private:

  
  nsresult PrintIndent(PRInt32 aIndentLevel);
  nsresult Write(const char *aBuffer);
  nsresult WriteInt(PRInt32 aInt);
  nsresult WriteTransaction(nsITransaction *aTransaction);
  nsresult Flush();
};

#endif 
