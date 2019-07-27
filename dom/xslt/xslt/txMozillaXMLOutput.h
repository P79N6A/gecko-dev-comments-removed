




#ifndef TRANSFRMX_MOZILLA_XML_OUTPUT_H
#define TRANSFRMX_MOZILLA_XML_OUTPUT_H

#include "txXMLEventHandler.h"
#include "nsAutoPtr.h"
#include "nsIScriptLoaderObserver.h"
#include "txOutputFormat.h"
#include "nsCOMArray.h"
#include "nsICSSLoaderObserver.h"
#include "txStack.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/Element.h"

class nsIContent;
class nsIDOMDocument;
class nsIAtom;
class nsIDOMDocumentFragment;
class nsITransformObserver;
class nsNodeInfoManager;
class nsIDocument;
class nsINode;

class txTransformNotifier final : public nsIScriptLoaderObserver,
                                  public nsICSSLoaderObserver
{
public:
    txTransformNotifier();

    NS_DECL_ISUPPORTS
    NS_DECL_NSISCRIPTLOADEROBSERVER
    
    
    NS_IMETHOD StyleSheetLoaded(mozilla::CSSStyleSheet* aSheet,
                                bool aWasAlternate,
                                nsresult aStatus) override;

    void Init(nsITransformObserver* aObserver);
    nsresult AddScriptElement(nsIScriptElement* aElement);
    void AddPendingStylesheet();
    void OnTransformEnd(nsresult aResult = NS_OK);
    void OnTransformStart();
    nsresult SetOutputDocument(nsIDocument* aDocument);

private:
    ~txTransformNotifier();
    void SignalTransformEnd(nsresult aResult = NS_OK);

    nsCOMPtr<nsIDocument> mDocument;
    nsCOMPtr<nsITransformObserver> mObserver;
    nsCOMArray<nsIScriptElement> mScriptElements;
    uint32_t mPendingStylesheetCount;
    bool mInTransform;
};

class txMozillaXMLOutput : public txAOutputXMLEventHandler
{
public:
    txMozillaXMLOutput(txOutputFormat* aFormat,
                       nsITransformObserver* aObserver);
    txMozillaXMLOutput(txOutputFormat* aFormat,
                       nsIDOMDocumentFragment* aFragment,
                       bool aNoFixup);
    ~txMozillaXMLOutput();

    TX_DECL_TXAXMLEVENTHANDLER
    TX_DECL_TXAOUTPUTXMLEVENTHANDLER

    nsresult closePrevious(bool aFlushText);

    nsresult createResultDocument(const nsSubstring& aName, int32_t aNsID,
                                  nsIDOMDocument* aSourceDocument,
                                  bool aLoadedAsData);

private:
    nsresult createTxWrapper();
    nsresult startHTMLElement(nsIContent* aElement, bool aXHTML);
    nsresult endHTMLElement(nsIContent* aElement);
    void processHTTPEquiv(nsIAtom* aHeader, const nsString& aValue);
    nsresult createHTMLElement(nsIAtom* aName,
                               nsIContent** aResult);

    nsresult attributeInternal(nsIAtom* aPrefix, nsIAtom* aLocalName,
                               int32_t aNsID, const nsString& aValue);
    nsresult startElementInternal(nsIAtom* aPrefix, nsIAtom* aLocalName,
                                  int32_t aNsID);

    nsCOMPtr<nsIDocument> mDocument;
    nsCOMPtr<nsINode> mCurrentNode;     
                                        
                                        
                                        
                                        
    nsCOMPtr<mozilla::dom::Element> mOpenedElement;
    nsRefPtr<nsNodeInfoManager> mNodeInfoManager;

    nsCOMArray<nsINode> mCurrentNodeStack;

    nsCOMPtr<nsIContent> mNonAddedNode;

    nsRefPtr<txTransformNotifier> mNotifier;

    uint32_t mTreeDepth, mBadChildLevel;
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

    bool mCreatingNewDocument;

    bool mOpenedElementIsHTML;

    
    bool mRootContentCreated;

    bool mNoFixup;

    enum txAction { eCloseElement = 1, eFlushText = 2 };
};

#endif
