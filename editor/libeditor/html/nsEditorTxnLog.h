




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
  int32_t mIndentLevel;
  int32_t mBatchCount;

public:

  

  nsEditorTxnLog(nsHTMLEditorLog *aEditorLog=0);

  

  virtual ~nsEditorTxnLog();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSITRANSACTIONLISTENER

private:

  
  nsresult PrintIndent(int32_t aIndentLevel);
  nsresult Write(const char *aBuffer);
  nsresult WriteInt(int32_t aInt);
  nsresult WriteTransaction(nsITransaction *aTransaction);
  nsresult Flush();
};

#endif 
