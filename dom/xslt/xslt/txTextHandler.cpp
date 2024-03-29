




#include "txTextHandler.h"
#include "nsAString.h"

txTextHandler::txTextHandler(bool aOnlyText) : mLevel(0),
                                                mOnlyText(aOnlyText)
{
}

nsresult
txTextHandler::attribute(nsIAtom* aPrefix, nsIAtom* aLocalName,
                         nsIAtom* aLowercaseLocalName, int32_t aNsID,
                         const nsString& aValue)
{
    return NS_OK;
}

nsresult
txTextHandler::attribute(nsIAtom* aPrefix, const nsSubstring& aLocalName,
                         const int32_t aNsID,
                         const nsString& aValue)
{
    return NS_OK;
}

nsresult
txTextHandler::characters(const nsSubstring& aData, bool aDOE)
{
    if (mLevel == 0)
        mValue.Append(aData);

    return NS_OK;
}

nsresult
txTextHandler::comment(const nsString& aData)
{
    return NS_OK;
}

nsresult
txTextHandler::endDocument(nsresult aResult)
{
    return NS_OK;
}

nsresult
txTextHandler::endElement()
{
    if (mOnlyText)
        --mLevel;

    return NS_OK;
}

nsresult
txTextHandler::processingInstruction(const nsString& aTarget, const nsString& aData)
{
    return NS_OK;
}

nsresult
txTextHandler::startDocument()
{
    return NS_OK;
}

nsresult
txTextHandler::startElement(nsIAtom* aPrefix, nsIAtom* aLocalName,
                            nsIAtom* aLowercaseLocalName, const int32_t aNsID)
{
    if (mOnlyText)
        ++mLevel;

    return NS_OK;
}

nsresult
txTextHandler::startElement(nsIAtom* aPrefix, const nsSubstring& aLocalName,
                            const int32_t aNsID)
{
    if (mOnlyText)
        ++mLevel;

    return NS_OK;
}
