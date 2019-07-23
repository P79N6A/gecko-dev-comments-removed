




































#ifndef TransactionFactory_h__
#define TransactionFactory_h__

#include "nsISupports.h"

class EditTxn;





class TransactionFactory
{
protected:
  TransactionFactory();
  virtual ~TransactionFactory();

public:
  



  static nsresult GetNewTransaction(REFNSIID aTxnType, EditTxn **aResult);

};

#endif
