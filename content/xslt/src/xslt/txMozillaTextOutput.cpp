





































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
#include "nsIParser.h"
#include "nsICharsetAlias.h"
#include "nsIPrincipal.h"
#include "txURIUtils.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentUtils.h"
#include "nsGkAtoms.h"

using namespace mozilla::dom;

txMozillaTextOutput::txMozillaTextOutput(nsITransformObserver* aObserver)
{
    MOZ_COUNT_CTOR(txMozillaTextOutput);
    mObserver = do_GetWeakReference(aObserver);
}

txMozillaTextOutput::txMozillaTextOutput(nsIDOMDocumentFragment* aDest)
{
    MOZ_COUNT_CTOR(txMozillaTextOutput);
    mTextParent = do_QueryInterface(aDest);
    mDocument = mTextParent->GetOwnerDoc();
}

txMozillaTextOutput::~txMozillaTextOutput()
{
    MOZ_COUNT_DTOR(txMozillaTextOutput);
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
txMozillaTextOutput::createResultDocument(nsIDOMDocument* aSourceDocument)
{
    















    
    nsresult rv = NS_NewXMLDocument(getter_AddRefs(mDocument));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIDocument> source = do_QueryInterface(aSourceDocument);
    NS_ENSURE_STATE(source);
    PRBool hasHadScriptObject = PR_FALSE;
    nsIScriptGlobalObject* sgo =
      source->GetScriptHandlingObject(hasHadScriptObject);
    NS_ENSURE_STATE(sgo || !hasHadScriptObject);
    mDocument->SetScriptHandlingObject(sgo);

    NS_ASSERTION(mDocument, "Need document");

    
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

    

    
    
    if (!observer) {
        PRInt32 namespaceID;
        rv = nsContentUtils::NameSpaceManager()->
            RegisterNameSpace(NS_LITERAL_STRING(kTXNameSpaceURI), namespaceID);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mDocument->CreateElem(nsDependentAtomString(nsGkAtoms::result),
                                   nsGkAtoms::transformiix, namespaceID,
                                   PR_FALSE, getter_AddRefs(mTextParent));
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
    ni = mDocument->NodeInfoManager()->
        GetNodeInfo(aName, nsnull, kNameSpaceID_XHTML,
                    nsIDOMNode::ELEMENT_NODE);
    NS_ENSURE_TRUE(ni, NS_ERROR_OUT_OF_MEMORY);

    return NS_NewHTMLElement(aResult, ni.forget(), NOT_FROM_PARSER);
}
