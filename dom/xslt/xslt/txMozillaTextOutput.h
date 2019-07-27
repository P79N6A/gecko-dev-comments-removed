




#ifndef TRANSFRMX_MOZILLA_TEXT_OUTPUT_H
#define TRANSFRMX_MOZILLA_TEXT_OUTPUT_H

#include "txXMLEventHandler.h"
#include "nsCOMPtr.h"
#include "nsWeakPtr.h"
#include "txOutputFormat.h"

class nsIDOMDocument;
class nsIDOMDocumentFragment;
class nsITransformObserver;
class nsIDocument;
class nsIContent;

class txMozillaTextOutput : public txAOutputXMLEventHandler
{
public:
    explicit txMozillaTextOutput(nsITransformObserver* aObserver);
    explicit txMozillaTextOutput(nsIDOMDocumentFragment* aDest);
    virtual ~txMozillaTextOutput();

    TX_DECL_TXAXMLEVENTHANDLER
    TX_DECL_TXAOUTPUTXMLEVENTHANDLER

    nsresult createResultDocument(nsIDOMDocument* aSourceDocument,
                                  bool aLoadedAsData);

private:
    nsresult createXHTMLElement(nsIAtom* aName, nsIContent** aResult);

    nsCOMPtr<nsIContent> mTextParent;
    nsWeakPtr mObserver;
    nsCOMPtr<nsIDocument> mDocument;
    txOutputFormat mOutputFormat;
    nsString mText;
};

#endif
