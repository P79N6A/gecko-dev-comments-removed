






































#include "txRtfHandler.h"

txResultTreeFragment::txResultTreeFragment(nsAutoPtr<txResultBuffer>& aBuffer)
    : txAExprResult(nsnull),
      mBuffer(aBuffer)
{
}

short txResultTreeFragment::getResultType()
{
    return RESULT_TREE_FRAGMENT;
}

void
txResultTreeFragment::stringValue(nsString& aResult)
{
    if (!mBuffer) {
        return;
    }

    aResult.Append(mBuffer->mStringValue);
}

const nsString*
txResultTreeFragment::stringValuePointer()
{
    return mBuffer ? &mBuffer->mStringValue : nsnull;
}

PRBool txResultTreeFragment::booleanValue()
{
    return PR_TRUE;
}

double txResultTreeFragment::numberValue()
{
    if (!mBuffer) {
        return 0;
    }

    return Double::toDouble(mBuffer->mStringValue);
}

nsresult txResultTreeFragment::flushToHandler(txAXMLEventHandler* aHandler)
{
    if (!mBuffer) {
        return NS_ERROR_FAILURE;
    }

    return mBuffer->flushToHandler(aHandler);
}

nsresult
txRtfHandler::getAsRTF(txAExprResult** aResult)
{
    *aResult = new txResultTreeFragment(mBuffer);
    NS_ENSURE_TRUE(*aResult, NS_ERROR_OUT_OF_MEMORY);

    NS_ADDREF(*aResult);

    return NS_OK;
}

nsresult
txRtfHandler::endDocument(nsresult aResult)
{
    return NS_OK;
}

nsresult
txRtfHandler::startDocument()
{
    return NS_OK;
}
