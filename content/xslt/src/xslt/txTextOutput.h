





































#ifndef TRANSFRMX_TEXT_OUTPUT_H
#define TRANSFRMX_TEXT_OUTPUT_H

#include "txXMLEventHandler.h"

class txTextOutput : public txAOutputXMLEventHandler
{
public:
    txTextOutput(ostream* aOut);
    ~txTextOutput();

    TX_DECL_TXAXMLEVENTHANDLER
    TX_DECL_TXAOUTPUTXMLEVENTHANDLER

private:
    ostream* mOut;
};

#endif
