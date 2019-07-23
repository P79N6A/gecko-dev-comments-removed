





































#ifndef TRANSFRMX_HTML_OUTPUT_H
#define TRANSFRMX_HTML_OUTPUT_H

#include "txXMLOutput.h"

class txHTMLOutput : public txXMLOutput
{
public:
    txHTMLOutput(txOutputFormat* aFormat, ostream* aOut);
    ~txHTMLOutput();

    


    static nsresult init();
    static void shutdown();

    nsresult attribute(const nsAString& aName, const PRInt32 aNsID,
                       const nsAString& aValue);
    nsresult characters(const nsAString& aData, PRBool aDOE);
    nsresult endElement(const nsAString& aName, const PRInt32 aNsID);
    nsresult processingInstruction(const nsAString& aTarget,
                                   const nsAString& aData);
    nsresult startDocument();
    nsresult startElement(const nsAString& aName, const PRInt32 aNsID);
    TX_DECL_TXAOUTPUTXMLEVENTHANDLER

private:
    void closeStartTag(MBool aUseEmptyElementShorthand);
    MBool isShorthandElement(const nsAString& aName);
    MBool isShorthandAttribute(const nsAString& aLocalName);

    txStack mCurrentElements;
};

#endif
