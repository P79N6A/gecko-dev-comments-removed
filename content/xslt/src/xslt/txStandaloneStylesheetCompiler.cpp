





































#include "txStandaloneStylesheetCompiler.h"
#include "txLog.h"
#include "txStylesheetCompiler.h"
#include "txURIUtils.h"
#include "expat_config.h"
#include "expat.h"
#include "txXMLParser.h"





class txDriver : public txACompileObserver
{
  public:
    nsresult parse(istream& aInputStream, const nsAString& aUri);
    const nsAString& getErrorString();

    


    void StartElement(const XML_Char *aName, const XML_Char **aAtts);
    void EndElement(const XML_Char* aName);
    void CharacterData(const XML_Char* aChars, int aLength);
    int ExternalEntityRef(const XML_Char *aContext, const XML_Char *aBase,
                          const XML_Char *aSystemId,
                          const XML_Char *aPublicId);

    TX_DECL_ACOMPILEOBSERVER;

    nsRefPtr<txStylesheetCompiler> mCompiler;
  protected:
    void createErrorString();
    nsString  mErrorString;
    
    nsresult mRV;
    XML_Parser mExpatParser;
    nsAutoRefCnt mRefCnt;
};

nsresult
TX_CompileStylesheetPath(const txParsedURL& aURL, txStylesheet** aResult)
{
    *aResult = nsnull;
    nsAutoString errMsg, filePath;

    aURL.getFile(filePath);
    PR_LOG(txLog::xslt, PR_LOG_ALWAYS,
           ("TX_CompileStylesheetPath: %s\n",
            NS_LossyConvertUTF16toASCII(filePath).get()));
    istream* xslInput = URIUtils::getInputStream(filePath, errMsg);
    if (!xslInput) {
        return NS_ERROR_FAILURE;
    }
    nsRefPtr<txDriver> driver = new txDriver();
    if (!driver) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    nsAutoString spec = filePath;
    if (!aURL.mRef.IsEmpty()) {
        spec.Append(PRUnichar('#'));
        spec.Append(aURL.mRef);
    }
    driver->mCompiler =  new txStylesheetCompiler(spec, driver);
    if (!driver->mCompiler) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    nsresult rv = driver->parse(*xslInput, filePath);
    if (NS_FAILED(rv)) {
        return rv;
    };
    *aResult = driver->mCompiler->getStylesheet();
    NS_ADDREF(*aResult);
    return NS_OK;
}






#define TX_DRIVER(_userData) static_cast<txDriver*>(_userData)

PR_STATIC_CALLBACK(void)
startElement(void *aUserData, const XML_Char *aName, const XML_Char **aAtts)
{
    if (!aUserData) {
        NS_WARNING("no userData in startElement handler");
        return;
    }
    TX_DRIVER(aUserData)->StartElement(aName, aAtts);
}

PR_STATIC_CALLBACK(void)
endElement(void *aUserData, const XML_Char* aName)
{
    if (!aUserData) {
        NS_WARNING("no userData in endElement handler");
        return;
    }
    TX_DRIVER(aUserData)->EndElement(aName);
}

PR_STATIC_CALLBACK(void)
charData(void* aUserData, const XML_Char* aChars, int aLength)
{
    if (!aUserData) {
        NS_WARNING("no userData in charData handler");
        return;
    }
    TX_DRIVER(aUserData)->CharacterData(aChars, aLength);
}

PR_STATIC_CALLBACK(int)
externalEntityRefHandler(XML_Parser aParser,
                         const XML_Char *aContext,
                         const XML_Char *aBase,
                         const XML_Char *aSystemId,
                         const XML_Char *aPublicId)
{
    
    
    NS_ENSURE_TRUE(aParser, XML_ERROR_NONE);
    return ((txDriver*)aParser)->ExternalEntityRef(aContext, aBase,
                                                   aSystemId, aPublicId);
}






nsresult
txDriver::parse(istream& aInputStream, const nsAString& aUri)
{
    mErrorString.Truncate();
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

    XML_SetReturnNSTriplet(mExpatParser, XML_TRUE);
    XML_SetUserData(mExpatParser, this);
    XML_SetElementHandler(mExpatParser, startElement, endElement);
    XML_SetCharacterDataHandler(mExpatParser, charData);
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
    int success;
    mRV = NS_OK;
    do {
        aInputStream.read(buf, bufferSize);
        done = aInputStream.eof();
        success = XML_Parse(mExpatParser, buf, aInputStream.gcount(), done);
        
        if (!success || NS_FAILED(mRV)) {
            createErrorString();
            done = MB_TRUE;
        }
    } while (!done);
    aInputStream.clear();

    
    XML_ParserFree(mExpatParser);
    mCompiler->doneLoading();
    if (!success) {
        return NS_ERROR_FAILURE;
    }
    return mRV;
}

const nsAString&
txDriver::getErrorString()
{
    return mErrorString;
}

void
txDriver::StartElement(const XML_Char *aName, const XML_Char **aAtts)
{
    PRInt32 attcount = 0;
    const XML_Char** atts = aAtts;
    while (*atts) {
        ++atts;
        ++attcount;
    }
    PRInt32 idOffset = XML_GetIdAttributeIndex(mExpatParser);
    nsresult rv =
        mCompiler->startElement(static_cast<const PRUnichar*>(aName), 
                                static_cast<const PRUnichar**>(aAtts),
                                attcount/2, idOffset);
    if (NS_FAILED(rv)) {
        PR_LOG(txLog::xslt, PR_LOG_ALWAYS, 
               ("compile failed at %i with %x\n",
                XML_GetCurrentLineNumber(mExpatParser), rv));
        mCompiler->cancel(rv);
    }
}

void
txDriver::EndElement(const XML_Char* aName)
{
    nsresult rv = mCompiler->endElement();
    if (NS_FAILED(rv)) {
        mCompiler->cancel(rv);
    }
}

void
txDriver::CharacterData(const XML_Char* aChars, int aLength)
{
    const PRUnichar* pChars = static_cast<const PRUnichar*>(aChars);
    
    nsresult rv = mCompiler->characters(Substring(pChars, pChars + aLength));
    if (NS_FAILED(rv)) {
        mCompiler->cancel(rv);
    }
}

int
txDriver::ExternalEntityRef(const XML_Char *aContext, const XML_Char *aBase,
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
txDriver::createErrorString()
{
    XML_Error errCode = XML_GetErrorCode(mExpatParser);
    mErrorString.AppendWithConversion(XML_ErrorString(errCode));
    mErrorString.AppendLiteral(" at line ");
    mErrorString.AppendInt(XML_GetCurrentLineNumber(mExpatParser));
    mErrorString.AppendLiteral(" in ");
    mErrorString.Append((const PRUnichar*)XML_GetBase(mExpatParser));
}





nsrefcnt
txDriver::AddRef()
{
    return ++mRefCnt;
}

nsrefcnt
txDriver::Release()
{
    if (--mRefCnt == 0) {
        mRefCnt = 1; 
        delete this;
        return 0;
    }
    return mRefCnt;
}

void
txDriver::onDoneCompiling(txStylesheetCompiler* aCompiler, nsresult aResult,
                          const PRUnichar *aErrorText, const PRUnichar *aParam)
{
    
    mRV = aResult;
}

nsresult
txDriver::loadURI(const nsAString& aUri, const nsAString& aReferrerUri,
                  txStylesheetCompiler* aCompiler)
{
    nsAutoString errMsg;
    istream* xslInput = URIUtils::getInputStream(aUri, errMsg);
    if (!xslInput) {
        return NS_ERROR_FAILURE;
    }
    nsRefPtr<txDriver> driver = new txDriver();
    if (!driver) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    driver->mCompiler = aCompiler;
    return driver->parse(*xslInput, aUri);
}
