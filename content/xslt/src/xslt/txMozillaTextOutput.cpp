





































#include "txMozillaTextOutput.h"
#include "nsContentCID.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMText.h"
#include "nsIDocumentTransformer.h"
#include "nsNetUtil.h"
#include "nsIDOMNSDocument.h"
#include "nsIParser.h"
#include "nsICharsetAlias.h"
#include "nsIPrincipal.h"
#include "txURIUtils.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentUtils.h"
#include "nsGkAtoms.h"

txMozillaTextOutput::txMozillaTextOutput(nsIDOMDocument* aSourceDocument,
                                         nsIDOMDocument* aResultDocument,
                                         nsITransformObserver* aObserver)
{
    mObserver = do_GetWeakReference(aObserver);
    createResultDocument(aSourceDocument, aResultDocument);
}

txMozillaTextOutput::txMozillaTextOutput(nsIDOMDocumentFragment* aDest)
{
    mTextParent = do_QueryInterface(aDest);
    mDocument = mTextParent->GetOwnerDoc();
}

txMozillaTextOutput::~txMozillaTextOutput()
{
}

nsresult
txMozillaTextOutput::attribute(nsIAtom* aPrefix, nsIAtom* aLocalName,
                               nsIAtom* aLowercaseLocalName,
                               PRInt32 aNsID, const nsString& aValue)
{
    return NS_OK;
}

nsresult
txMozillaTextOutput::attribute(nsIAtom* aPrefix, const nsSubstring& aName,
                               const PRInt32 aNsID,
                               const nsString& aValue)
{
    return NS_OK;
}

nsresult
txMozillaTextOutput::characters(const nsSubstring& aData, PRBool aDOE)
{
    mText.Append(aData);

    return NS_OK;
}

nsresult
txMozillaTextOutput::comment(const nsString& aData)
{
    return NS_OK;
}

nsresult
txMozillaTextOutput::endDocument(nsresult aResult)
{
    NS_ENSURE_TRUE(mDocument && mTextParent, NS_ERROR_FAILURE);

    nsCOMPtr<nsIContent> text;
    nsresult rv = NS_NewTextNode(getter_AddRefs(text),
                                 mDocument->NodeInfoManager());
    NS_ENSURE_SUCCESS(rv, rv);
    
    text->SetText(mText, PR_FALSE);
    rv = mTextParent->AppendChildTo(text, PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);

    if (NS_SUCCEEDED(aResult)) {
        nsCOMPtr<nsITransformObserver> observer = do_QueryReferent(mObserver);
        if (observer) {
            observer->OnTransformDone(aResult, mDocument);
        }
    }

    return NS_OK;
}

nsresult
txMozillaTextOutput::endElement()
{
    return NS_OK;
}

nsresult
txMozillaTextOutput::processingInstruction(const nsString& aTarget,
                                           const nsString& aData)
{
    return NS_OK;
}

nsresult
txMozillaTextOutput::startDocument()
{
    return NS_OK;
}

nsresult
txMozillaTextOutput::createResultDocument(nsIDOMDocument* aSourceDocument,
                                          nsIDOMDocument* aResultDocument)
{
    nsresult rv = NS_OK;

    















    if (!aResultDocument) {
        
        rv = NS_NewXMLDocument(getter_AddRefs(mDocument));
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
        mDocument = do_QueryInterface(aResultDocument);
    }

    NS_ASSERTION(mDocument, "Need document");

    nsCOMPtr<nsIDOMNSDocument> nsDoc = do_QueryInterface(mDocument);
    if (nsDoc) {
        nsDoc->SetTitle(EmptyString());
    }

    
    URIUtils::ResetWithSource(mDocument, aSourceDocument);

    
    if (!mOutputFormat.mEncoding.IsEmpty()) {
        NS_LossyConvertUTF16toASCII charset(mOutputFormat.mEncoding);
        nsCAutoString canonicalCharset;
        nsCOMPtr<nsICharsetAlias> calias =
            do_GetService("@mozilla.org/intl/charsetalias;1");

        if (calias &&
            NS_SUCCEEDED(calias->GetPreferred(charset, canonicalCharset))) {
            mDocument->SetDocumentCharacterSetSource(kCharsetFromOtherComponent);
            mDocument->SetDocumentCharacterSet(canonicalCharset);
        }
    }

    
    nsCOMPtr<nsITransformObserver> observer = do_QueryReferent(mObserver);
    if (observer) {
        rv = observer->OnDocumentCreated(mDocument);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    

    
    
    
    
    if (!aResultDocument && !observer) {
        PRInt32 namespaceID;
        rv = nsContentUtils::NameSpaceManager()->
            RegisterNameSpace(NS_LITERAL_STRING(kTXNameSpaceURI), namespaceID);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mDocument->CreateElem(nsGkAtoms::result, nsGkAtoms::transformiix,
                                   namespaceID, PR_FALSE, getter_AddRefs(mTextParent));
        NS_ENSURE_SUCCESS(rv, rv);


        rv = mDocument->AppendChildTo(mTextParent, PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
        nsCOMPtr<nsIContent> html, head, body;
        rv = createXHTMLElement(nsGkAtoms::html, getter_AddRefs(html));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = createXHTMLElement(nsGkAtoms::head, getter_AddRefs(head));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = html->AppendChildTo(head, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = createXHTMLElement(nsGkAtoms::body, getter_AddRefs(body));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = html->AppendChildTo(body, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = createXHTMLElement(nsGkAtoms::pre, getter_AddRefs(mTextParent));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mTextParent->SetAttr(kNameSpaceID_None, nsGkAtoms::id,
                                  NS_LITERAL_STRING("transformiixResult"),
                                  PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = body->AppendChildTo(mTextParent, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mDocument->AppendChildTo(html, PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}

nsresult
txMozillaTextOutput::startElement(nsIAtom* aPrefix, nsIAtom* aLocalName,
                                  nsIAtom* aLowercaseLocalName, PRInt32 aNsID)
{
    return NS_OK;
}

nsresult
txMozillaTextOutput::startElement(nsIAtom* aPrefix, const nsSubstring& aName,
                                  const PRInt32 aNsID)
{
    return NS_OK;
}

void txMozillaTextOutput::getOutputDocument(nsIDOMDocument** aDocument)
{
    CallQueryInterface(mDocument, aDocument);
}

nsresult
txMozillaTextOutput::createXHTMLElement(nsIAtom* aName,
                                        nsIContent** aResult)
{
    *aResult = nsnull;

    nsCOMPtr<nsINodeInfo> ni;
    nsresult rv = mDocument->NodeInfoManager()->
        GetNodeInfo(aName, nsnull, kNameSpaceID_XHTML, getter_AddRefs(ni));
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_NewHTMLElement(aResult, ni);
}

