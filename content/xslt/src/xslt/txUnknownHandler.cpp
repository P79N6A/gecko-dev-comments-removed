





































#include "txUnknownHandler.h"
#include "txExecutionState.h"
#include "txStringUtils.h"
#include "txStylesheet.h"
#include "txAtoms.h"

txUnknownHandler::txUnknownHandler(txExecutionState* aEs)
    : mEs(aEs)
{
}

txUnknownHandler::~txUnknownHandler()
{
}

nsresult
txUnknownHandler::endDocument(nsresult aResult)
{
    if (NS_FAILED(aResult)) {
        return NS_OK;
    }

    
    
    

    
    
    
    NS_ASSERTION(mEs->mResultHandler == this,
                 "We're leaking mEs->mResultHandler and are going to crash.");

    nsresult rv = createHandlerAndFlush(PR_FALSE, EmptyString(),
                                        kNameSpaceID_None);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mEs->mResultHandler->endDocument(aResult);

    delete this;

    return rv;
}

nsresult
txUnknownHandler::startElement(nsIAtom* aPrefix, nsIAtom* aLocalName,
                               nsIAtom* aLowercaseLocalName, PRInt32 aNsID)
{
    
    
    
    NS_ASSERTION(mEs->mResultHandler == this,
                 "We're leaking mEs->mResultHandler.");

    nsCOMPtr<nsIAtom> owner;
    if (!aLowercaseLocalName) {
        owner = TX_ToLowerCaseAtom(aLocalName);
        NS_ENSURE_TRUE(owner, NS_ERROR_OUT_OF_MEMORY);
        
        aLowercaseLocalName = owner;
    }

    PRBool htmlRoot = aNsID == kNameSpaceID_None && !aPrefix &&
                      aLowercaseLocalName == txHTMLAtoms::html;

    
    
    
    nsAutoString name;
    aLocalName->ToString(name);
    nsresult rv = createHandlerAndFlush(htmlRoot, name, aNsID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mEs->mResultHandler->startElement(aPrefix, aLocalName,
                                           aLowercaseLocalName, aNsID);

    delete this;

    return rv;
}

nsresult
txUnknownHandler::startElement(nsIAtom* aPrefix, const nsSubstring& aLocalName,
                               const PRInt32 aNsID)
{
    
    
    
    NS_ASSERTION(mEs->mResultHandler == this,
                 "We're leaking mEs->mResultHandler.");

    PRBool htmlRoot = aNsID == kNameSpaceID_None && !aPrefix &&
                      aLocalName.Equals(NS_LITERAL_STRING("html"),
                                        txCaseInsensitiveStringComparator());
    nsresult rv = createHandlerAndFlush(htmlRoot, aLocalName, aNsID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mEs->mResultHandler->startElement(aPrefix, aLocalName, aNsID);

    delete this;

    return rv;
}

nsresult txUnknownHandler::createHandlerAndFlush(PRBool aHTMLRoot,
                                                 const nsSubstring& aName,
                                                 const PRInt32 aNsID)
{
    NS_ENSURE_TRUE(mBuffer, NS_ERROR_NOT_INITIALIZED);

    txOutputFormat format;
    format.merge(*(mEs->mStylesheet->getOutputFormat()));
    if (format.mMethod == eMethodNotSet) {
        format.mMethod = aHTMLRoot ? eHTMLOutput : eXMLOutput;
    }

    txAXMLEventHandler *handler = nsnull;
    nsresult rv = mEs->mOutputHandlerFactory->createHandlerWith(&format, aName,
                                                                aNsID,
                                                                &handler);
    NS_ENSURE_SUCCESS(rv, rv);

    mEs->mOutputHandler = handler;
    mEs->mResultHandler = handler;

    return mBuffer->flushToHandler(&handler);
}
