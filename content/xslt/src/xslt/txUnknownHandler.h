




#ifndef txUnknownHandler_h___
#define txUnknownHandler_h___

#include "txBufferingHandler.h"
#include "txOutputFormat.h"

class txExecutionState;

class txUnknownHandler : public txBufferingHandler
{
public:
    txUnknownHandler(txExecutionState* aEs);
    virtual ~txUnknownHandler();

    TX_DECL_TXAXMLEVENTHANDLER

private:
    nsresult createHandlerAndFlush(bool aHTMLRoot,
                                   const nsSubstring& aName,
                                   const int32_t aNsID);

    




    txExecutionState* mEs;

    
    
    bool mFlushed;
};

#endif 
