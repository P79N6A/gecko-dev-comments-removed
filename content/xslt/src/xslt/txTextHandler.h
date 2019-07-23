





































#ifndef TRANSFRMX_TEXT_HANDLER_H
#define TRANSFRMX_TEXT_HANDLER_H

#include "txXMLEventHandler.h"
#include "nsString.h"

class txTextHandler : public txAXMLEventHandler
{
public:
    txTextHandler(MBool aOnlyText);
    virtual ~txTextHandler();

    TX_DECL_TXAXMLEVENTHANDLER

    nsString mValue;

private:
    PRUint32 mLevel;
    MBool mOnlyText;
};

#endif
