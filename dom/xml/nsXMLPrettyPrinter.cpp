



 
#include "nsXMLPrettyPrinter.h"
#include "nsContentUtils.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMDocumentXBL.h"
#include "nsIObserver.h"
#include "nsIXSLTProcessor.h"
#include "nsSyncLoadService.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsIServiceManager.h"
#include "nsNetUtil.h"
#include "mozilla/dom/Element.h"
#include "nsIDOMDocumentFragment.h"
#include "nsBindingManager.h"
#include "nsXBLService.h"
#include "nsIScriptSecurityManager.h"
#include "mozilla/Preferences.h"
#include "nsIDocument.h"
#include "nsVariant.h"
#include "nsIDOMCustomEvent.h"

using namespace mozilla;
using namespace mozilla::dom;

NS_IMPL_ISUPPORTS(nsXMLPrettyPrinter,
                  nsIDocumentObserver,
                  nsIMutationObserver)

nsXMLPrettyPrinter::nsXMLPrettyPrinter() : mDocument(nullptr),
                                           mUnhookPending(false)
{
}

nsXMLPrettyPrinter::~nsXMLPrettyPrinter()
{
    NS_ASSERTION(!mDocument, "we shouldn't be referencing the document still");
}

nsresult
nsXMLPrettyPrinter::PrettyPrint(nsIDocument* aDocument,
                                bool* aDidPrettyPrint)
{
    *aDidPrettyPrint = false;

    
    if (!aDocument->GetShell()) {
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
        if (frameOwnerDoc) {
            nsCOMPtr<nsIDOMWindow> window;
            frameOwnerDoc->GetDefaultView(getter_AddRefs(window));
            if (window) {
                window->GetComputedStyle(frameElem,
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

    
    if (!Preferences::GetBool("layout.xml.prettyprint", true)) {
        return NS_OK;
    }

    
    *aDidPrettyPrint = true;
    nsresult rv = NS_OK;

    
    nsCOMPtr<nsIURI> xslUri;
    rv = NS_NewURI(getter_AddRefs(xslUri),
                   NS_LITERAL_CSTRING("chrome://global/content/xml/XMLPrettyPrint.xsl"));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMDocument> xslDocument;
    rv = nsSyncLoadService::LoadDocument(xslUri, nsContentUtils::GetSystemPrincipal(),
                                         nullptr, true,
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

    
    
    
    
    
    
    
    

    
    nsXBLService* xblService = nsXBLService::GetInstance();
    NS_ENSURE_TRUE(xblService, NS_ERROR_NOT_AVAILABLE);

    
    nsCOMPtr<nsIURI> bindingUri;
    rv = NS_NewURI(getter_AddRefs(bindingUri),
        NS_LITERAL_STRING("chrome://global/content/xml/XMLPrettyPrint.xml#prettyprint"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIContent> rootCont = aDocument->GetRootElement();
    NS_ENSURE_TRUE(rootCont, NS_ERROR_UNEXPECTED);

    
    nsCOMPtr<nsIPrincipal> sysPrincipal;
    nsContentUtils::GetSecurityManager()->
        GetSystemPrincipal(getter_AddRefs(sysPrincipal));

    
    nsRefPtr<nsXBLBinding> unused;
    bool ignored;
    rv = xblService->LoadBindings(rootCont, bindingUri, sysPrincipal,
                                  getter_AddRefs(unused), &ignored);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIDOMEvent> domEvent;
    rv = NS_NewDOMCustomEvent(getter_AddRefs(domEvent), rootCont,
                              nullptr, nullptr);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIDOMCustomEvent> customEvent = do_QueryInterface(domEvent);
    MOZ_ASSERT(customEvent);
    nsCOMPtr<nsIWritableVariant> resultFragmentVariant = new nsVariant();
    rv = resultFragmentVariant->SetAsISupports(resultFragment);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    rv = customEvent->InitCustomEvent(NS_LITERAL_STRING("prettyprint-dom-created"),
                                       false,  false,
                                       resultFragmentVariant);
    NS_ENSURE_SUCCESS(rv, rv);
    customEvent->SetTrusted(true);
    bool dummy;
    rv = rootCont->DispatchEvent(domEvent, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);

    
    aDocument->AddObserver(this);
    mDocument = aDocument;

    NS_ADDREF_THIS();

    return NS_OK;
}

void
nsXMLPrettyPrinter::MaybeUnhook(nsIContent* aContent)
{
    
    
    if ((!aContent || !aContent->GetBindingParent()) && !mUnhookPending) {
        
        
        
        mUnhookPending = true;
        nsContentUtils::AddScriptRunner(
          NS_NewRunnableMethod(this, &nsXMLPrettyPrinter::Unhook));
    }
}

void
nsXMLPrettyPrinter::Unhook()
{
    mDocument->RemoveObserver(this);
    nsCOMPtr<Element> element = mDocument->GetDocumentElement();

    if (element) {
        mDocument->BindingManager()->ClearBinding(element);
    }

    mDocument = nullptr;

    NS_RELEASE_THIS();
}

void
nsXMLPrettyPrinter::AttributeChanged(nsIDocument* aDocument,
                                     Element* aElement,
                                     int32_t aNameSpaceID,
                                     nsIAtom* aAttribute,
                                     int32_t aModType)
{
    MaybeUnhook(aElement);
}

void
nsXMLPrettyPrinter::ContentAppended(nsIDocument* aDocument,
                                    nsIContent* aContainer,
                                    nsIContent* aFirstNewContent,
                                    int32_t aNewIndexInContainer)
{
    MaybeUnhook(aContainer);
}

void
nsXMLPrettyPrinter::ContentInserted(nsIDocument* aDocument,
                                    nsIContent* aContainer,
                                    nsIContent* aChild,
                                    int32_t aIndexInContainer)
{
    MaybeUnhook(aContainer);
}

void
nsXMLPrettyPrinter::ContentRemoved(nsIDocument* aDocument,
                                   nsIContent* aContainer,
                                   nsIContent* aChild,
                                   int32_t aIndexInContainer,
                                   nsIContent* aPreviousSibling)
{
    MaybeUnhook(aContainer);
}

void
nsXMLPrettyPrinter::NodeWillBeDestroyed(const nsINode* aNode)
{
    mDocument = nullptr;
    NS_RELEASE_THIS();
}


nsresult NS_NewXMLPrettyPrinter(nsXMLPrettyPrinter** aPrinter)
{
    *aPrinter = new nsXMLPrettyPrinter;
    NS_ADDREF(*aPrinter);
    return NS_OK;
}
