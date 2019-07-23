





































#include "txTextOutput.h"
#include "nsString.h"

txTextOutput::txTextOutput(ostream* aOut)
    : mOut(aOut)
{
}

txTextOutput::~txTextOutput()
{
}

nsresult
txTextOutput::attribute(const nsAString& aName,
                        const PRInt32 aNsID,
                        const nsAString& aValue)
{
    return NS_OK;
}

nsresult
txTextOutput::characters(const nsAString& aData, PRBool aDOE)
{
    *mOut << NS_ConvertUTF16toUTF8(aData).get();
}

nsresult
txTextOutput::comment(const nsAString& aData)
{
    return NS_OK;
}

nsresult
txTextOutput::endDocument(nsresult aResult)
{
    return NS_OK;
}

nsresult
txTextOutput::endElement(const nsAString& aName, const PRInt32 aNsID)
{
    return NS_OK;
}

nsresult
txTextOutput::processingInstruction(const nsAString& aTarget,
                                    const nsAString& aData)
{
    return NS_OK;
}

nsresult
txTextOutput::startDocument()
{
    return NS_OK;
}

nsresult
txTextOutput::startElement(const nsAString& aName, const PRInt32 aNsID)
{
    return NS_OK;
}
