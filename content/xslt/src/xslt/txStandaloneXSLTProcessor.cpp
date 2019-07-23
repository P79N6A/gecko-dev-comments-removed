






































#include "txStandaloneXSLTProcessor.h"
#include "txStandaloneStylesheetCompiler.h"
#include "nsCRT.h"
#include "nsReadableUtils.h"
#include "txHTMLOutput.h"
#include "txTextOutput.h"
#include "txUnknownHandler.h"
#include "txURIUtils.h"
#include "txXMLParser.h"

TX_IMPL_DOM_STATICS;




class txStandaloneHandlerFactory : public txAOutputHandlerFactory
{
public:
    txStandaloneHandlerFactory(txExecutionState* aEs,
                               ostream* aStream)
        : mEs(aEs), mStream(aStream)
    {
    }

    virtual ~txStandaloneHandlerFactory()
    {
    }

    TX_DECL_TXAOUTPUTHANDLERFACTORY

private:
    txExecutionState* mEs;
    ostream* mStream;
};

nsresult
txStandaloneHandlerFactory::createHandlerWith(txOutputFormat* aFormat,
                                              txAOutputXMLEventHandler** aHandler)
{
    *aHandler = 0;
    switch (aFormat->mMethod) {
        case eXMLOutput:
            *aHandler = new txXMLOutput(aFormat, mStream);
            break;

        case eHTMLOutput:
            *aHandler = new txHTMLOutput(aFormat, mStream);
            break;

        case eTextOutput:
            *aHandler = new txTextOutput(mStream);
            break;

        case eMethodNotSet:
            *aHandler = new txUnknownHandler(mEs);
            break;
    }
    NS_ENSURE_TRUE(*aHandler, NS_ERROR_OUT_OF_MEMORY);
    return NS_OK;
}

nsresult
txStandaloneHandlerFactory::createHandlerWith(txOutputFormat* aFormat,
                                              const nsAString& aName,
                                              PRInt32 aNsID,
                                              txAOutputXMLEventHandler** aHandler)
{
    *aHandler = 0;
    NS_ASSERTION(aFormat->mMethod != eMethodNotSet,
                 "How can method not be known when root element is?");
    NS_ENSURE_TRUE(aFormat->mMethod != eMethodNotSet, NS_ERROR_UNEXPECTED);
    return createHandlerWith(aFormat, aHandler);
}











nsresult
txStandaloneXSLTProcessor::transform(nsACString& aXMLPath, ostream& aOut,
                                     ErrorObserver& aErr)
{
    txXPathNode* xmlDoc = parsePath(aXMLPath, aErr);
    if (!xmlDoc) {
        return NS_ERROR_FAILURE;
    }

    
    nsresult rv = transform(*xmlDoc, aOut, aErr);

    delete xmlDoc;

    return rv;
}





nsresult
txStandaloneXSLTProcessor::transform(nsACString& aXMLPath,
                                     nsACString& aXSLPath, ostream& aOut,
                                     ErrorObserver& aErr)
{
    txXPathNode* xmlDoc = parsePath(aXMLPath, aErr);
    if (!xmlDoc) {
        return NS_ERROR_FAILURE;
    }
    txParsedURL path;
    path.init(NS_ConvertASCIItoUTF16(aXSLPath));
    nsRefPtr<txStylesheet> style;
    nsresult rv = TX_CompileStylesheetPath(path, getter_AddRefs(style));
    if (NS_FAILED(rv)) {
        delete xmlDoc;
        return rv;
    }
    
    rv = transform(*xmlDoc, style, aOut, aErr);

    delete xmlDoc;

    return rv;
}






nsresult
txStandaloneXSLTProcessor::transform(txXPathNode& aXMLDoc, ostream& aOut,
                                     ErrorObserver& aErr)
{
    Document* xmlDoc;
    nsresult rv = txXPathNativeNode::getDocument(aXMLDoc, &xmlDoc);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsAutoString stylePath, basePath;
    xmlDoc->getBaseURI(basePath);
    getHrefFromStylesheetPI(*xmlDoc, stylePath);
    txParsedURL base, ref, resolved;
    base.init(basePath);
    ref.init(stylePath);
    base.resolve(ref, resolved);

    nsRefPtr<txStylesheet> style;
    rv = TX_CompileStylesheetPath(resolved, getter_AddRefs(style));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = transform(aXMLDoc, style, aOut, aErr);

    return rv;
}





nsresult
txStandaloneXSLTProcessor::transform(txXPathNode& aSource,
                                     txStylesheet* aStylesheet,
                                     ostream& aOut, ErrorObserver& aErr)
{
    
    txExecutionState es(aStylesheet);

    

    txStandaloneHandlerFactory handlerFactory(&es, &aOut);

#ifndef XP_WIN
    bool sync = aOut.sync_with_stdio(false);
#endif
    es.mOutputHandlerFactory = &handlerFactory;

    es.init(aSource, nsnull);

    
    nsresult rv = txXSLTProcessor::execute(es);
    es.end(rv);

#ifndef XP_WIN
    aOut.sync_with_stdio(sync);
#endif

    return rv;
}








void txStandaloneXSLTProcessor::getHrefFromStylesheetPI(Document& xmlDocument,
                                                        nsAString& href)
{
    Node* node = xmlDocument.getFirstChild();
    nsAutoString type;
    nsAutoString tmpHref;
    while (node) {
        if (node->getNodeType() == Node::PROCESSING_INSTRUCTION_NODE) {
            nsAutoString target;
            node->getNodeName(target);
            if (target.EqualsLiteral("xml-stylesheet")) {
                nsAutoString data;
                node->getNodeValue(data);
                type.Truncate();
                tmpHref.Truncate();
                parseStylesheetPI(data, type, tmpHref);
                if (type.EqualsLiteral("text/xsl") ||
                    type.EqualsLiteral("text/xml") ||
                    type.EqualsLiteral("application/xml")) {
                    href = tmpHref;
                    return;
                }
            }
        }
        node = node->getNextSibling();
    }
}





#define SKIP_WHITESPACE(iter, end_iter)                          \
  while ((iter) != (end_iter) && nsCRT::IsAsciiSpace(*(iter))) { \
    ++(iter);                                                    \
  }                                                              \
  if ((iter) == (end_iter))                                      \
    break

#define SKIP_ATTR_NAME(iter, end_iter)                            \
  while ((iter) != (end_iter) && !nsCRT::IsAsciiSpace(*(iter)) && \
         *(iter) != '=') {                                        \
    ++(iter);                                                     \
  }                                                               \
  if ((iter) == (end_iter))                                       \
    break

void txStandaloneXSLTProcessor::parseStylesheetPI(const nsAFlatString& aData,
                                                  nsAString& aType,
                                                  nsAString& aHref)
{
  nsAFlatString::const_char_iterator start, end;
  aData.BeginReading(start);
  aData.EndReading(end);
  nsAFlatString::const_char_iterator iter;
  PRInt8 found = 0;

  while (start != end) {
    SKIP_WHITESPACE(start, end);
    iter = start;
    SKIP_ATTR_NAME(iter, end);

    
    const nsAString & attrName = Substring(start, iter);

    
    start = iter;
    SKIP_WHITESPACE(start, end);
    if (*start != '=') {
      
      
      break;
    }

    
    ++start;
    SKIP_WHITESPACE(start, end);
    PRUnichar q = *start;
    if (q != QUOTE && q != APOSTROPHE) {
      
      break;
    }

    ++start;  
    iter = start;
    while (iter != end && *iter != q) {
      ++iter;
    }
    if (iter == end) {
      
      break;
    }
    
    
    
    if (attrName.EqualsLiteral("type")) {
      aType = Substring(start, iter);
      ++found;
    }
    else if (attrName.EqualsLiteral("href")) {
      aHref = Substring(start, iter);
      ++found;
    }

    
    if (found == 2) {
      break;
    }

    
    start = iter;
    ++start;  
  }
}

txXPathNode*
txStandaloneXSLTProcessor::parsePath(const nsACString& aPath, ErrorObserver& aErr)
{
    NS_ConvertASCIItoUTF16 path(aPath);

    ifstream xmlInput(PromiseFlatCString(aPath).get(), ios::in);
    if (!xmlInput) {
        aErr.receiveError(NS_LITERAL_STRING("Couldn't open ") + path);
        return 0;
    }
    
    txXPathNode* xmlDoc;
    nsAutoString errors;
    nsresult rv = txParseFromStream(xmlInput, path, errors, &xmlDoc);
    xmlInput.close();
    if (NS_FAILED(rv) || !xmlDoc) {
        aErr.receiveError(NS_LITERAL_STRING("Parsing error \"") + errors +
                          NS_LITERAL_STRING("\""));
    }
    return xmlDoc;
}
