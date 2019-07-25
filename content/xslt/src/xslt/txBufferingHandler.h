






































#ifndef txBufferingHandler_h__
#define txBufferingHandler_h__

#include "txXMLEventHandler.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"

class txOutputTransaction;
class txCharacterTransaction;

class txResultBuffer
{
public:
    ~txResultBuffer();

    nsresult addTransaction(txOutputTransaction* aTransaction);

    




    nsresult flushToHandler(txAXMLEventHandler** aHandler);

    txOutputTransaction* getLastTransaction();

    nsString mStringValue;

private:
    nsTArray<txOutputTransaction*> mTransactions;
};

class txBufferingHandler : public txAXMLEventHandler
{
public:
    txBufferingHandler();

    TX_DECL_TXAXMLEVENTHANDLER

protected:
    nsAutoPtr<txResultBuffer> mBuffer;
    PRPackedBool mCanAddAttribute;
};

#endif 
