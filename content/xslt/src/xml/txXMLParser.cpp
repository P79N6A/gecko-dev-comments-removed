





































#include "txXMLParser.h"
#include "txURIUtils.h"
#include "txXPathTreeWalker.h"

#ifndef TX_EXE
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsSyncLoadService.h"
#include "nsNetUtil.h"
#else
#include "expat_config.h"
#include "expat.h"
#include "txXMLUtils.h"
#endif

#ifdef TX_EXE




class txXMLParser
{
  public:
    nsresult parse(istream& aInputStream, const nsAString& aUri,
                   txXPathNode** aResultDoc);
    const nsAString& getErrorString();

    


    int StartElement(const XML_Char *aName, const XML_Char **aAtts);
    int EndElement(const XML_Char* aName);
    void CharacterData(const XML_Char* aChars, int aLength);
    void Comment(const XML_Char* aChars);
    int ProcessingInstruction(const XML_Char *aTarget, const XML_Char *aData);
    int ExternalEntityRef(const XML_Char *aContext, const XML_Char *aBase,
                          const XML_Char *aSystemId,
                          const XML_Char *aPublicId);

  protected:
    void createErrorString();
    nsString  mErrorString;
    Document* mDocument;
    Node*  mCurrentNode;
    XML_Parser mExpatParser;
};
#endif

nsresult
txParseDocumentFromURI(const nsAString& aHref, const txXPathNode& aLoader,
                       nsAString& aErrMsg, txXPathNode** aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = nsnull;
#ifndef TX_EXE
    nsCOMPtr<nsIURI> documentURI;
    nsresult rv = NS_NewURI(getter_AddRefs(documentURI), aHref);
    NS_ENSURE_SUCCESS(rv, rv);

    nsIDocument* loaderDocument = txXPathNativeNode::getDocument(aLoader);

    nsCOMPtr<nsILoadGroup> loadGroup = loaderDocument->GetDocumentLoadGroup();
    nsIURI *loaderUri = loaderDocument->GetDocumentURI();
    NS_ENSURE_TRUE(loaderUri, NS_ERROR_FAILURE);

    
    
    nsIDOMDocument* theDocument = nsnull;
    rv = nsSyncLoadService::LoadDocument(documentURI, loaderUri, loadGroup,
                                         PR_TRUE, &theDocument);

    if (NS_FAILED(rv)) {
        aErrMsg.Append(NS_LITERAL_STRING("Document load of ") + 
                       aHref + NS_LITERAL_STRING(" failed."));
        return NS_FAILED(rv) ? rv : NS_ERROR_FAILURE;
    }

    *aResult = txXPathNativeNode::createXPathNode(theDocument);
    if (!*aResult) {
        NS_RELEASE(theDocument);
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
#else
    istream* xslInput = URIUtils::getInputStream(aHref, aErrMsg);
    if (!xslInput) {
        return NS_ERROR_FAILURE;
    }
    return txParseFromStream(*xslInput, aHref, aErrMsg, aResult);
#endif
}

#ifdef TX_EXE
nsresult
txParseFromStream(istream& aInputStream, const nsAString& aUri,
                  nsAString& aErrorString, txXPathNode** aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    txXMLParser parser;
    nsresult rv = parser.parse(aInputStream, aUri, aResult);
    aErrorString = parser.getErrorString();
    return rv;
}






#define TX_XMLPARSER(_userData) static_cast<txXMLParser*>(_userData)
#define TX_ENSURE_DATA(_userData)                       \
  PR_BEGIN_MACRO                                        \
    if (!aUserData) {                                   \
        NS_WARNING("no userData in comment handler");   \
        return;                                         \
    }                                                   \
  PR_END_MACRO

PR_STATIC_CALLBACK(void)
startElement(void *aUserData, const XML_Char *aName, const XML_Char **aAtts)
{
    TX_ENSURE_DATA(aUserData);
    TX_XMLPARSER(aUserData)->StartElement(aName, aAtts);
}

PR_STATIC_CALLBACK(void)
endElement(void *aUserData, const XML_Char* aName)
{
    TX_ENSURE_DATA(aUserData);
    TX_XMLPARSER(aUserData)->EndElement(aName);
}

PR_STATIC_CALLBACK(void)
charData(void* aUserData, const XML_Char* aChars, int aLength)
{
    TX_ENSURE_DATA(aUserData);
    TX_XMLPARSER(aUserData)->CharacterData(aChars, aLength);
}

PR_STATIC_CALLBACK(void)
commentHandler(void* aUserData, const XML_Char* aChars)
{
    TX_ENSURE_DATA(aUserData);
    TX_XMLPARSER(aUserData)->Comment(aChars);
}

PR_STATIC_CALLBACK(void)
piHandler(void *aUserData, const XML_Char *aTarget, const XML_Char *aData)
{
    TX_ENSURE_DATA(aUserData);
    TX_XMLPARSER(aUserData)->ProcessingInstruction(aTarget, aData);
}

PR_STATIC_CALLBACK(int)
externalEntityRefHandler(XML_Parser aParser,
                         const XML_Char *aContext,
                         const XML_Char *aBase,
                         const XML_Char *aSystemId,
                         const XML_Char *aPublicId)
{
    
    
    NS_ENSURE_TRUE(aParser, XML_ERROR_NONE);
    return ((txXMLParser*)aParser)->ExternalEntityRef(aContext, aBase,
                                                      aSystemId, aPublicId);
}






nsresult
txXMLParser::parse(istream& aInputStream, const nsAString& aUri,
                   txXPathNode** aResultDoc)
{
    mErrorString.Truncate();
    *aResultDoc = nsnull;
    if (!aInputStream) {
        mErrorString.AppendLiteral("unable to parse xml: invalid or unopen stream encountered.");
        return NS_ERROR_FAILURE;
    }

    static const XML_Memory_Handling_Suite memsuite = {
        (void *(*)(size_t))PR_Malloc,
        (void *(*)(void *, size_t))PR_Realloc,
        PR_Free
    };
    static const PRUnichar expatSeparator = kExpatSeparatorChar;
    mExpatParser = XML_ParserCreate_MM(nsnull, &memsuite, &expatSeparator);
    if (!mExpatParser) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    mDocument = new Document();
    if (!mDocument) {
        XML_ParserFree(mExpatParser);
        return NS_ERROR_OUT_OF_MEMORY;
    }
    mDocument->documentBaseURI = aUri;
    mCurrentNode = mDocument;

    XML_SetReturnNSTriplet(mExpatParser, XML_TRUE);
    XML_SetUserData(mExpatParser, this);
    XML_SetElementHandler(mExpatParser, startElement, endElement);
    XML_SetCharacterDataHandler(mExpatParser, charData);
    XML_SetProcessingInstructionHandler(mExpatParser, piHandler);
    XML_SetCommentHandler(mExpatParser, commentHandler);
#ifdef XML_DTD
    XML_SetParamEntityParsing(mExpatParser, XML_PARAM_ENTITY_PARSING_ALWAYS);
#endif
    XML_SetExternalEntityRefHandler(mExpatParser, externalEntityRefHandler);
    XML_SetExternalEntityRefHandlerArg(mExpatParser, this);
    XML_SetBase(mExpatParser,
                (const XML_Char*)(PromiseFlatString(aUri).get()));

    const int bufferSize = 1024;
    char buf[bufferSize];
    PRBool done;
    do {
        aInputStream.read(buf, bufferSize);
        done = aInputStream.eof();

        if (!XML_Parse(mExpatParser, buf, aInputStream.gcount(), done)) {
            createErrorString();
            done = MB_TRUE;
            delete mDocument;
            mDocument = nsnull;
        }
    } while (!done);
    aInputStream.clear();

    
    XML_ParserFree(mExpatParser);
    
    *aResultDoc = txXPathNativeNode::createXPathNode(mDocument);
    mDocument = nsnull;
    return *aResultDoc ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

const nsAString&
txXMLParser::getErrorString()
{
    return mErrorString;
}


int
txXMLParser::StartElement(const XML_Char *aName, const XML_Char **aAtts)
{
    nsCOMPtr<nsIAtom> prefix, localName;
    PRInt32 nsID;
    XMLUtils::splitExpatName(aName, getter_AddRefs(prefix),
                             getter_AddRefs(localName), &nsID);
    Element* newElement = mDocument->createElementNS(prefix, localName, nsID);
    if (!newElement) {
        return XML_ERROR_NO_MEMORY;
    }

    const XML_Char** theAtts = aAtts;
    while (*theAtts) {
        XMLUtils::splitExpatName(*theAtts++, getter_AddRefs(prefix),
                                 getter_AddRefs(localName), &nsID);
        nsDependentString attValue(*theAtts++);
        nsresult rv = newElement->appendAttributeNS(prefix, localName, nsID,
                                                    attValue);
        if (NS_FAILED(rv)) {
            return XML_ERROR_NO_MEMORY;
        }
    }

    int idx;
    if ((idx = XML_GetIdAttributeIndex(mExpatParser)) > -1) {
        nsDependentString idName((const PRUnichar*)*(aAtts + idx));
        nsDependentString idValue((const PRUnichar*)*(aAtts + idx + 1));
        
        if (!idValue.IsEmpty()) {
            mDocument->setElementID(idValue, newElement);
        }
    }
    mCurrentNode->appendChild(newElement);
    mCurrentNode = newElement;

    return XML_ERROR_NONE;
}

int
txXMLParser::EndElement(const XML_Char* aName)
{
    if (mCurrentNode->getParentNode()) {
        mCurrentNode = mCurrentNode->getParentNode();
    }
    return XML_ERROR_NONE;
}

void
txXMLParser::CharacterData(const XML_Char* aChars, int aLength)
{
    Node* prevSib = mCurrentNode->getLastChild();
    const PRUnichar* pChars = static_cast<const PRUnichar*>(aChars);
    if (prevSib && prevSib->getNodeType() == Node::TEXT_NODE) {
        static_cast<NodeDefinition*>(prevSib)->appendData(pChars, aLength);
    }
    else {
        
        Node* node = mDocument->createTextNode(Substring(pChars,
                                                         pChars + aLength));
        mCurrentNode->appendChild(node);
    }
}

void
txXMLParser::Comment(const XML_Char* aChars)
{
    Node* node = mDocument->createComment(
        nsDependentString(static_cast<const PRUnichar*>(aChars)));
    mCurrentNode->appendChild(node);
}

int
txXMLParser::ProcessingInstruction(const XML_Char *aTarget,
                                   const XML_Char *aData)
{
    nsCOMPtr<nsIAtom> target = do_GetAtom(aTarget);
    nsDependentString data((const PRUnichar*)aData);
    Node* node = mDocument->createProcessingInstruction(target, data);
    mCurrentNode->appendChild(node);

    return XML_ERROR_NONE;
}

int
txXMLParser::ExternalEntityRef(const XML_Char *aContext,
                               const XML_Char *aBase,
                               const XML_Char *aSystemId,
                               const XML_Char *aPublicId)
{
    if (aPublicId) {
        
        return XML_ERROR_EXTERNAL_ENTITY_HANDLING;
    }
    nsAutoString absUrl;
    URIUtils::resolveHref(nsDependentString((PRUnichar*)aSystemId),
                          nsDependentString((PRUnichar*)aBase), absUrl);
    istream* extInput = URIUtils::getInputStream(absUrl, mErrorString);
    if (!extInput) {
        return XML_ERROR_EXTERNAL_ENTITY_HANDLING;
    }
    XML_Parser parent = mExpatParser;
    mExpatParser = 
        XML_ExternalEntityParserCreate(mExpatParser, aContext, nsnull);
    if (!mExpatParser) {
        mExpatParser = parent;
        delete extInput;
        return XML_ERROR_EXTERNAL_ENTITY_HANDLING;
    }
    XML_SetBase(mExpatParser, absUrl.get());

    const int bufSize = 1024;
    char buffer[bufSize];
    int result;
    PRBool done;
    do {
        extInput->read(buffer, bufSize);
        done = extInput->eof();
        if (!(result =
              XML_Parse(mExpatParser, buffer,  extInput->gcount(), done))) {
            createErrorString();
            mErrorString.Append(PRUnichar('\n'));
            done = MB_TRUE;
        }
    } while (!done);

    delete extInput;
    XML_ParserFree(mExpatParser);

    mExpatParser = parent;

    return result;
}

void
txXMLParser::createErrorString()
{
    XML_Error errCode = XML_GetErrorCode(mExpatParser);
    mErrorString.AppendWithConversion(XML_ErrorString(errCode));
    mErrorString.AppendLiteral(" at line ");
    mErrorString.AppendInt(XML_GetCurrentLineNumber(mExpatParser));
    mErrorString.AppendLiteral(" in ");
    mErrorString.Append((const PRUnichar*)XML_GetBase(mExpatParser));
}
#endif
