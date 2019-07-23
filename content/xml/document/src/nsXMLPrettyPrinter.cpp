




































 
#include "nsXMLPrettyPrinter.h"
#include "nsContentUtils.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMAbstractView.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMViewCSS.h"
#include "nsIDOMDocumentXBL.h"
#include "nsIObserver.h"
#include "nsIXSLTProcessor.h"
#include "nsSyncLoadService.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsIServiceManager.h"
#include "nsNetUtil.h"
#include "nsIContent.h"
#include "nsIDOMDocumentFragment.h"
#include "nsBindingManager.h"

NS_IMPL_ISUPPORTS2(nsXMLPrettyPrinter,
                   nsIDocumentObserver,
                   nsIMutationObserver)

nsXMLPrettyPrinter::nsXMLPrettyPrinter() : mDocument(nsnull),
                                           mUpdateDepth(0),
                                           mUnhookPending(PR_FALSE)
{
}

nsXMLPrettyPrinter::~nsXMLPrettyPrinter()
{
    NS_ASSERTION(!mDocument, "we shouldn't be referencing the document still");
}

nsresult
nsXMLPrettyPrinter::PrettyPrint(nsIDocument* aDocument,
                                PRBool* aDidPrettyPrint)
{
    *aDidPrettyPrint = PR_FALSE;
    
    
    if (!aDocument->GetPrimaryShell()) {
        return NS_OK;
    }

    
    nsPIDOMWindow *internalWin = aDocument->GetWindow();
    nsCOMPtr<nsIDOMElement> frameElem;
    if (internalWin) {
        internalWin->GetFrameElement(getter_AddRefs(frameElem));
    }

    if (frameElem) {
        nsCOMPtr<nsIDOMCSSStyleDeclaration> computedStyle;
        nsCOMPtr<nsIDOMDocument> frameOwnerDoc;
        frameElem->GetOwnerDocument(getter_AddRefs(frameOwnerDoc));
        nsCOMPtr<nsIDOMDocumentView> docView = do_QueryInterface(frameOwnerDoc);
        if (docView) {
            nsCOMPtr<nsIDOMAbstractView> defaultView;
            docView->GetDefaultView(getter_AddRefs(defaultView));
            nsCOMPtr<nsIDOMViewCSS> defaultCSSView =
                do_QueryInterface(defaultView);
            if (defaultCSSView) {
                defaultCSSView->GetComputedStyle(frameElem,
                                                 EmptyString(),
                                                 getter_AddRefs(computedStyle));
            }
        }

        if (computedStyle) {
            nsAutoString visibility;
            computedStyle->GetPropertyValue(NS_LITERAL_STRING("visibility"),
                                            visibility);
            if (!visibility.EqualsLiteral("visible")) {

                return NS_OK;
            }
        }
    }

    
    if (!nsContentUtils::GetBoolPref("layout.xml.prettyprint", PR_TRUE)) {
        return NS_OK;
    }

    
    *aDidPrettyPrint = PR_TRUE;
    nsresult rv = NS_OK;

    
    nsCOMPtr<nsIURI> xslUri;
    rv = NS_NewURI(getter_AddRefs(xslUri),
                   NS_LITERAL_CSTRING("chrome://global/content/xml/XMLPrettyPrint.xsl"));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMDocument> xslDocument;
    rv = nsSyncLoadService::LoadDocument(xslUri, nsnull, nsnull, PR_TRUE,
                                         getter_AddRefs(xslDocument));
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIXSLTProcessor> transformer =
        do_CreateInstance("@mozilla.org/document-transformer;1?type=xslt", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = transformer->ImportStylesheet(xslDocument);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMDocumentFragment> resultFragment;
    nsCOMPtr<nsIDOMDocument> sourceDocument = do_QueryInterface(aDocument);
    rv = transformer->TransformToFragment(sourceDocument, sourceDocument,
                                          getter_AddRefs(resultFragment));
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIDOMDocumentXBL> xblDoc = do_QueryInterface(aDocument);
    NS_ASSERTION(xblDoc, "xml document doesn't implement nsIDOMDocumentXBL");
    NS_ENSURE_TRUE(xblDoc, NS_ERROR_FAILURE);

    xblDoc->LoadBindingDocument(NS_LITERAL_STRING("chrome://global/content/xml/XMLPrettyPrint.xml"));

    nsCOMPtr<nsIDOMElement> rootElem;
    sourceDocument->GetDocumentElement(getter_AddRefs(rootElem));
    NS_ENSURE_TRUE(rootElem, NS_ERROR_UNEXPECTED);

    rv = xblDoc->AddBinding(rootElem,
                            NS_LITERAL_STRING("chrome://global/content/xml/XMLPrettyPrint.xml#prettyprint"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIObserver> binding;
    nsCOMPtr<nsIContent> rootCont = do_QueryInterface(rootElem);
    NS_ASSERTION(rootCont, "Element doesn't implement nsIContent");
    aDocument->BindingManager()->GetBindingImplementation(rootCont,
                                              NS_GET_IID(nsIObserver),
                                              (void**)getter_AddRefs(binding));
    NS_ASSERTION(binding, "Prettyprint binding doesn't implement nsIObserver");
    NS_ENSURE_TRUE(binding, NS_ERROR_UNEXPECTED);
    
    rv = binding->Observe(resultFragment, "prettyprint-dom-created",
                          EmptyString().get());
    NS_ENSURE_SUCCESS(rv, rv);

    
    aDocument->AddObserver(this);
    mDocument = aDocument;

    NS_ADDREF_THIS();

    return NS_OK;
}

void
nsXMLPrettyPrinter::MaybeUnhook(nsIContent* aContent)
{
    
    
    if (!aContent || !aContent->GetBindingParent()) {
        mUnhookPending = PR_TRUE;
    }
}



void
nsXMLPrettyPrinter::BeginUpdate(nsIDocument* aDocument,
                                nsUpdateType aUpdateType)
{
    mUpdateDepth++;
}

void
nsXMLPrettyPrinter::EndUpdate(nsIDocument* aDocument, nsUpdateType aUpdateType)
{
    mUpdateDepth--;

    
    
    
    if (mUnhookPending && mUpdateDepth == 0) {
        mDocument->RemoveObserver(this);
        nsCOMPtr<nsIDOMDocument> document = do_QueryInterface(mDocument);
        nsCOMPtr<nsIDOMElement> rootElem;
        document->GetDocumentElement(getter_AddRefs(rootElem));

        if (rootElem) {
            nsCOMPtr<nsIDOMDocumentXBL> xblDoc = do_QueryInterface(mDocument);
            xblDoc->RemoveBinding(rootElem,
                                  NS_LITERAL_STRING("chrome://global/content/xml/XMLPrettyPrint.xml#prettyprint"));
        }

        mDocument = nsnull;

        NS_RELEASE_THIS();
    }
}

void
nsXMLPrettyPrinter::AttributeChanged(nsIDocument* aDocument,
                                     nsIContent* aContent,
                                     PRInt32 aNameSpaceID,
                                     nsIAtom* aAttribute,
                                     PRInt32 aModType)
{
    MaybeUnhook(aContent);
}

void
nsXMLPrettyPrinter::ContentAppended(nsIDocument* aDocument,
                                    nsIContent* aContainer,
                                    PRInt32 aNewIndexInContainer)
{
    MaybeUnhook(aContainer);
}

void
nsXMLPrettyPrinter::ContentInserted(nsIDocument* aDocument,
                                    nsIContent* aContainer,
                                    nsIContent* aChild,
                                    PRInt32 aIndexInContainer)
{
    MaybeUnhook(aContainer);
}

void
nsXMLPrettyPrinter::ContentRemoved(nsIDocument* aDocument,
                                   nsIContent* aContainer,
                                   nsIContent* aChild,
                                   PRInt32 aIndexInContainer)
{
    MaybeUnhook(aContainer);
}

void
nsXMLPrettyPrinter::NodeWillBeDestroyed(const nsINode* aNode)
{
    mDocument = nsnull;
    NS_RELEASE_THIS();
}


nsresult NS_NewXMLPrettyPrinter(nsXMLPrettyPrinter** aPrinter)
{
    *aPrinter = new nsXMLPrettyPrinter;
    NS_ENSURE_TRUE(*aPrinter, NS_ERROR_OUT_OF_MEMORY);
    NS_ADDREF(*aPrinter);
    return NS_OK;
}
