





































#ifndef TRANSFRMX_MOZILLA_XML_OUTPUT_H
#define TRANSFRMX_MOZILLA_XML_OUTPUT_H

#include "txXMLEventHandler.h"
#include "nsAutoPtr.h"
#include "nsIScriptLoaderObserver.h"
#include "txOutputFormat.h"
#include "nsCOMArray.h"
#include "nsICSSLoaderObserver.h"
#include "txStack.h"

class nsIContent;
class nsIDOMDocument;
class nsIAtom;
class nsIDOMDocumentFragment;
class nsIDOMElement;
class nsIStyleSheet;
class nsIDOMNode;
class nsITransformObserver;
class nsNodeInfoManager;
class nsIDocument;
class nsINode;

class txTransformNotifier : public nsIScriptLoaderObserver,
                            public nsICSSLoaderObserver
{
public:
    txTransformNotifier();

    NS_DECL_ISUPPORTS
    NS_DECL_NSISCRIPTLOADEROBSERVER
    
    
    NS_IMETHOD StyleSheetLoaded(nsCSSStyleSheet* aSheet,
                                PRBool aWasAlternate,
                                nsresult aStatus);

    void Init(nsITransformObserver* aObserver);
    nsresult AddScriptElement(nsIScriptElement* aElement);
    void AddPendingStylesheet();
    void OnTransformEnd(nsresult aResult = NS_OK);
    void OnTransformStart();
    nsresult SetOutputDocument(nsIDocument* aDocument);

private:
    void SignalTransformEnd(nsresult aResult = NS_OK);

    nsCOMPtr<nsIDocument> mDocument;
    nsCOMPtr<nsITransformObserver> mObserver;
    nsCOMArray<nsIScriptElement> mScriptElements;
    PRUint32 mPendingStylesheetCount;
    PRPackedBool mInTransform;
};

class txMozillaXMLOutput : public txAOutputXMLEventHandler
{
public:
    txMozillaXMLOutput(txOutputFormat* aFormat,
                       nsITransformObserver* aObserver);
    txMozillaXMLOutput(txOutputFormat* aFormat,
                       nsIDOMDocumentFragment* aFragment,
                       PRBool aNoFixup);
    ~txMozillaXMLOutput();

    TX_DECL_TXAXMLEVENTHANDLER
    TX_DECL_TXAOUTPUTXMLEVENTHANDLER

    nsresult closePrevious(PRBool aFlushText);

    nsresult createResultDocument(const nsSubstring& aName, PRInt32 aNsID,
                                  nsIDOMDocument* aSourceDocument);

private:
    nsresult createTxWrapper();
    nsresult startHTMLElement(nsIContent* aElement, PRBool aXHTML);
    nsresult endHTMLElement(nsIContent* aElement);
    void processHTTPEquiv(nsIAtom* aHeader, const nsString& aValue);
    nsresult createHTMLElement(nsIAtom* aName,
                               nsIContent** aResult);

    nsresult attributeInternal(nsIAtom* aPrefix, nsIAtom* aLocalName,
                               PRInt32 aNsID, const nsString& aValue);
    nsresult startElementInternal(nsIAtom* aPrefix, nsIAtom* aLocalName,
                                  PRInt32 aNsID);

    nsCOMPtr<nsIDocument> mDocument;
    nsCOMPtr<nsINode> mCurrentNode;     
                                        
                                        
                                        
                                        
    nsCOMPtr<nsIContent> mOpenedElement;
    nsRefPtr<nsNodeInfoManager> mNodeInfoManager;

    nsCOMArray<nsINode> mCurrentNodeStack;

    nsCOMPtr<nsIContent> mNonAddedNode;

    nsRefPtr<txTransformNotifier> mNotifier;

    PRUint32 mTreeDepth, mBadChildLevel;
    nsCString mRefreshString;

    txStack mTableStateStack;
    enum TableState {
        NORMAL,      
        TABLE,       
        ADDED_TBODY  
    };
    TableState mTableState;

    nsAutoString mText;

    txOutputFormat mOutputFormat;

    PRPackedBool mCreatingNewDocument;

    PRPackedBool mOpenedElementIsHTML;

    
    PRPackedBool mRootContentCreated;

    PRPackedBool mNoFixup;

    enum txAction { eCloseElement = 1, eFlushText = 2 };
};

#endif
