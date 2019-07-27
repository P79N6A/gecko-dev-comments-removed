




#ifndef TRANSFRMX_TEXT_HANDLER_H
#define TRANSFRMX_TEXT_HANDLER_H

#include "txXMLEventHandler.h"
#include "nsString.h"

class txTextHandler : public txAXMLEventHandler
{
public:
    explicit txTextHandler(bool aOnlyText);

    TX_DECL_TXAXMLEVENTHANDLER

    nsString mValue;

private:
    uint32_t mLevel;
    bool mOnlyText;
};

#endif
