





































#ifndef TRANSFRMX_TXMOZILLAXSLTPROCESSOR_H
#define TRANSFRMX_TXMOZILLAXSLTPROCESSOR_H

#include "nsAutoPtr.h"
#include "nsStubMutationObserver.h"
#include "nsIDocumentTransformer.h"
#include "nsIXSLTProcessor.h"
#include "nsIXSLTProcessorObsolete.h"
#include "nsIXSLTProcessorPrivate.h"
#include "txExpandedNameMap.h"
#include "txNamespaceMap.h"
#include "nsIJSNativeInitializer.h"

class nsIDOMNode;
class nsIPrincipal;
class nsIURI;
class nsIXMLContentSink;
class txStylesheet;
class txResultRecycler;
class txIGlobalParameter;


#define TRANSFORMIIX_XSLT_PROCESSOR_CID   \
{ 0xbacd8ad0, 0x552f, 0x11d3, {0xa9, 0xf7, 0x00, 0x00, 0x64, 0x65, 0x73, 0x74} }

#define TRANSFORMIIX_XSLT_PROCESSOR_CONTRACTID \
"@mozilla.org/document-transformer;1?type=xslt"

#define XSLT_MSGS_URL  "chrome://global/locale/xslt/xslt.properties"




class txMozillaXSLTProcessor : public nsIXSLTProcessor,
                               public nsIXSLTProcessorObsolete,
                               public nsIXSLTProcessorPrivate,
                               public nsIDocumentTransformer,
                               public nsStubMutationObserver,
                               public nsIJSNativeInitializer
{
public:
    


    txMozillaXSLTProcessor();

    


    virtual ~txMozillaXSLTProcessor();

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIXSLTPROCESSOR

    
    NS_DECL_NSIXSLTPROCESSOROBSOLETE

    
    NS_DECL_NSIXSLTPROCESSORPRIVATE

    
    NS_IMETHOD Init(nsIPrincipal* aPrincipal);
    NS_IMETHOD SetTransformObserver(nsITransformObserver* aObserver);
    NS_IMETHOD LoadStyleSheet(nsIURI* aUri, nsILoadGroup* aLoadGroup);
    NS_IMETHOD SetSourceContentModel(nsIDOMNode* aSource);
    NS_IMETHOD CancelLoads() {return NS_OK;}
    NS_IMETHOD AddXSLTParamNamespace(const nsString& aPrefix,
                                     const nsString& aNamespace);
    NS_IMETHOD AddXSLTParam(const nsString& aName,
                            const nsString& aNamespace,
                            const nsString& aSelect,
                            const nsString& aValue,
                            nsIDOMNode* aContext);

    
    NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
    NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED
    NS_DECL_NSIMUTATIONOBSERVER_NODEWILLBEDESTROYED

    nsresult setStylesheet(txStylesheet* aStylesheet);
    void reportError(nsresult aResult, const PRUnichar *aErrorText,
                     const PRUnichar *aSourceText);

    nsIDOMNode *GetSourceContentModel()
    {
        return mSource;
    }

    nsresult TransformToDoc(nsIDOMDocument *aOutputDoc,
                            nsIDOMDocument **aResult);

    PRBool IsLoadDisabled()
    {
        return (mFlags & DISABLE_ALL_LOADS) != 0;
    }

    
    NS_IMETHODIMP Initialize(JSContext *cx, JSObject *obj, 
                             PRUint32 argc, jsval *argv);

    static nsresult Startup();
    static void Shutdown();

private:
    nsresult DoTransform();
    void notifyError();
    nsresult ensureStylesheet();

    nsRefPtr<txStylesheet> mStylesheet;
    nsIDocument* mStylesheetDocument; 
    nsCOMPtr<nsIContent> mEmbeddedStylesheetRoot;

    nsCOMPtr<nsIDOMNode> mSource;
    nsresult mTransformResult;
    nsresult mCompileResult;
    nsString mErrorText, mSourceText;
    nsCOMPtr<nsIPrincipal> mPrincipal;
    nsCOMPtr<nsITransformObserver> mObserver;
    txOwningExpandedNameMap<txIGlobalParameter> mVariables;
    txNamespaceMap mParamNamespaceMap;
    nsRefPtr<txResultRecycler> mRecycler;

    PRUint32 mFlags;
};

extern nsresult TX_LoadSheet(nsIURI* aUri, txMozillaXSLTProcessor* aProcessor,
                             nsILoadGroup* aLoadGroup,
                             nsIPrincipal* aCallerPrincipal);

extern nsresult TX_CompileStylesheet(nsIDOMNode* aNode,
                                     txMozillaXSLTProcessor* aProcessor,
                                     nsIPrincipal* aCallerPrincipal,
                                     txStylesheet** aStylesheet);

#endif
