





































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

    nsresult endDocument(nsresult aResult);
    nsresult startElement(nsIAtom* aPrefix, nsIAtom* aName,
                          nsIAtom* aLowercaseName, PRInt32 aNsID);
    nsresult startElement(nsIAtom* aPrefix, const nsSubstring& aLocalName,
                          const PRInt32 aNsID);

private:
    nsresult createHandlerAndFlush(PRBool aHTMLRoot,
                                   const nsSubstring& aName,
                                   const PRInt32 aNsID);

    




    txExecutionState* mEs;
};

#endif 
