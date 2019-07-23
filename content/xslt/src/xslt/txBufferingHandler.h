






































#ifndef txBufferingHandler_h__
#define txBufferingHandler_h__

#include "txXMLEventHandler.h"
#include "nsString.h"
#include "nsVoidArray.h"
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
    nsVoidArray mTransactions;
};

class txBufferingHandler : public txAXMLEventHandler
{
public:
    txBufferingHandler();
    ~txBufferingHandler();

    TX_DECL_TXAXMLEVENTHANDLER

protected:
    nsAutoPtr<txResultBuffer> mBuffer;
    PRPackedBool mCanAddAttribute;
};

#endif 
