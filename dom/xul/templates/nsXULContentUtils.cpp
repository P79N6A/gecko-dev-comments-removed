


























#include "mozilla/ArrayUtils.h"

#include "nsCOMPtr.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMXULCommandDispatcher.h"
#include "nsIDOMXULDocument.h"
#include "nsIRDFNode.h"
#include "nsIRDFService.h"
#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsXULContentUtils.h"
#include "nsLayoutCID.h"
#include "nsNameSpaceManager.h"
#include "nsRDFCID.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsGkAtoms.h"
#include "mozilla/Logging.h"
#include "prtime.h"
#include "rdf.h"
#include "nsContentUtils.h"
#include "nsIDateTimeFormat.h"
#include "nsDateTimeFormatCID.h"
#include "nsIScriptableDateFormat.h"
#include "nsICollation.h"
#include "nsCollationCID.h"
#include "nsILocale.h"
#include "nsILocaleService.h"
#include "nsIConsoleService.h"
#include "nsEscape.h"

using namespace mozilla;



nsIRDFService* nsXULContentUtils::gRDF;
nsIDateTimeFormat* nsXULContentUtils::gFormat;
nsICollation *nsXULContentUtils::gCollation;

extern PRLogModuleInfo* gXULTemplateLog;

#define XUL_RESOURCE(ident, uri) nsIRDFResource* nsXULContentUtils::ident
#define XUL_LITERAL(ident, val) nsIRDFLiteral* nsXULContentUtils::ident
#include "nsXULResourceList.h"
#undef XUL_RESOURCE
#undef XUL_LITERAL





nsresult
nsXULContentUtils::Init()
{
    static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);
    nsresult rv = CallGetService(kRDFServiceCID, &gRDF);
    if (NS_FAILED(rv)) {
        return rv;
    }

#define XUL_RESOURCE(ident, uri)                              \
  PR_BEGIN_MACRO                                              \
   rv = gRDF->GetResource(NS_LITERAL_CSTRING(uri), &(ident)); \
   if (NS_FAILED(rv)) return rv;                              \
  PR_END_MACRO

#define XUL_LITERAL(ident, val)                                   \
  PR_BEGIN_MACRO                                                  \
   rv = gRDF->GetLiteral(NS_LITERAL_STRING(val).get(), &(ident)); \
   if (NS_FAILED(rv)) return rv;                                  \
  PR_END_MACRO

#include "nsXULResourceList.h"
#undef XUL_RESOURCE
#undef XUL_LITERAL

    rv = CallCreateInstance(NS_DATETIMEFORMAT_CONTRACTID, &gFormat);
    if (NS_FAILED(rv)) {
        return rv;
    }

    return NS_OK;
}


nsresult
nsXULContentUtils::Finish()
{
    NS_IF_RELEASE(gRDF);

#define XUL_RESOURCE(ident, uri) NS_IF_RELEASE(ident)
#define XUL_LITERAL(ident, val) NS_IF_RELEASE(ident)
#include "nsXULResourceList.h"
#undef XUL_RESOURCE
#undef XUL_LITERAL

    NS_IF_RELEASE(gFormat);
    NS_IF_RELEASE(gCollation);

    return NS_OK;
}

nsICollation*
nsXULContentUtils::GetCollation()
{
    if (!gCollation) {
        nsresult rv;

        
        nsCOMPtr<nsILocaleService> localeService =
            do_GetService(NS_LOCALESERVICE_CONTRACTID, &rv);
        if (NS_SUCCEEDED(rv)) {
            nsCOMPtr<nsILocale> locale;
            rv = localeService->GetApplicationLocale(getter_AddRefs(locale));
            if (NS_SUCCEEDED(rv) && locale) {
                nsCOMPtr<nsICollationFactory> colFactory =
                    do_CreateInstance(NS_COLLATIONFACTORY_CONTRACTID);
                if (colFactory) {
                    rv = colFactory->CreateCollation(locale, &gCollation);
                    NS_ASSERTION(NS_SUCCEEDED(rv),
                                 "couldn't create collation instance");
                } else
                    NS_ERROR("couldn't create instance of collation factory");
            } else
                NS_ERROR("unable to get application locale");
        } else
            NS_ERROR("couldn't get locale factory");
    }

    return gCollation;
}



nsresult
nsXULContentUtils::FindChildByTag(nsIContent* aElement,
                                  int32_t aNameSpaceID,
                                  nsIAtom* aTag,
                                  nsIContent** aResult)
{
    for (nsIContent* child = aElement->GetFirstChild();
         child;
         child = child->GetNextSibling()) {

        if (child->NodeInfo()->Equals(aTag, aNameSpaceID)) {
            NS_ADDREF(*aResult = child);

            return NS_OK;
        }
    }

    *aResult = nullptr;
    return NS_RDF_NO_VALUE; 
}






nsresult
nsXULContentUtils::GetTextForNode(nsIRDFNode* aNode, nsAString& aResult)
{
    if (! aNode) {
        aResult.Truncate();
        return NS_OK;
    }

    nsresult rv;

    
    nsCOMPtr<nsIRDFLiteral> literal = do_QueryInterface(aNode);
    if (literal) {
        const char16_t* p;
        rv = literal->GetValueConst(&p);
        if (NS_FAILED(rv)) return rv;

        aResult = p;
        return NS_OK;
    }

    nsCOMPtr<nsIRDFDate> dateLiteral = do_QueryInterface(aNode);
    if (dateLiteral) {
        PRTime value;
        rv = dateLiteral->GetValue(&value);
        if (NS_FAILED(rv)) return rv;

        nsAutoString str;
        rv = gFormat->FormatPRTime(nullptr ,
                                  kDateFormatShort,
                                  kTimeFormatSeconds,
                                  value,
                                  str);
        aResult.Assign(str);

        if (NS_FAILED(rv)) return rv;

        return NS_OK;
    }

    nsCOMPtr<nsIRDFInt> intLiteral = do_QueryInterface(aNode);
    if (intLiteral) {
        int32_t	value;
        rv = intLiteral->GetValue(&value);
        if (NS_FAILED(rv)) return rv;

        aResult.Truncate();
        nsAutoString intStr;
        intStr.AppendInt(value, 10);
        aResult.Append(intStr);
        return NS_OK;
    }


    nsCOMPtr<nsIRDFResource> resource = do_QueryInterface(aNode);
    if (resource) {
        const char* p;
        rv = resource->GetValueConst(&p);
        if (NS_FAILED(rv)) return rv;
        CopyUTF8toUTF16(p, aResult);
        return NS_OK;
    }

    NS_ERROR("not a resource or a literal");
    return NS_ERROR_UNEXPECTED;
}

nsresult
nsXULContentUtils::GetResource(int32_t aNameSpaceID, nsIAtom* aAttribute, nsIRDFResource** aResult)
{
    
    NS_PRECONDITION(aAttribute != nullptr, "null ptr");
    if (! aAttribute)
        return NS_ERROR_NULL_POINTER;

    return GetResource(aNameSpaceID, nsDependentAtomString(aAttribute),
                       aResult);
}


nsresult
nsXULContentUtils::GetResource(int32_t aNameSpaceID, const nsAString& aAttribute, nsIRDFResource** aResult)
{
    

    
    
    
    

    nsresult rv;

    char16_t buf[256];
    nsFixedString uri(buf, ArrayLength(buf), 0);
    if (aNameSpaceID != kNameSpaceID_Unknown && aNameSpaceID != kNameSpaceID_None) {
        rv = nsContentUtils::NameSpaceManager()->GetNameSpaceURI(aNameSpaceID, uri);
        
    }

    
    if (!uri.IsEmpty()  && uri.Last() != '#' && uri.Last() != '/' && aAttribute.First() != '#')
        uri.Append(char16_t('#'));

    uri.Append(aAttribute);

    rv = gRDF->GetUnicodeResource(uri, aResult);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get resource");
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}


nsresult
nsXULContentUtils::SetCommandUpdater(nsIDocument* aDocument, nsIContent* aElement)
{
    
    
    
    NS_PRECONDITION(aDocument != nullptr, "null ptr");
    if (! aDocument)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aElement != nullptr, "null ptr");
    if (! aElement)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    nsCOMPtr<nsIDOMXULDocument> xuldoc = do_QueryInterface(aDocument);
    NS_ASSERTION(xuldoc != nullptr, "not a xul document");
    if (! xuldoc)
        return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIDOMXULCommandDispatcher> dispatcher;
    rv = xuldoc->GetCommandDispatcher(getter_AddRefs(dispatcher));
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get dispatcher");
    if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(dispatcher != nullptr, "no dispatcher");
    if (! dispatcher)
        return NS_ERROR_UNEXPECTED;

    nsAutoString events;
    aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::events, events);
    if (events.IsEmpty())
        events.Assign('*');

    nsAutoString targets;
    aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::targets, targets);

    if (targets.IsEmpty())
        targets.Assign('*');

    nsCOMPtr<nsIDOMElement> domelement = do_QueryInterface(aElement);
    NS_ASSERTION(domelement != nullptr, "not a DOM element");
    if (! domelement)
        return NS_ERROR_UNEXPECTED;

    rv = dispatcher->AddCommandUpdater(domelement, events, targets);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

void
nsXULContentUtils::LogTemplateError(const char* aStr)
{
  nsAutoString message;
  message.AssignLiteral("Error parsing template: ");
  message.Append(NS_ConvertUTF8toUTF16(aStr).get());

  nsCOMPtr<nsIConsoleService> cs = do_GetService(NS_CONSOLESERVICE_CONTRACTID);
  if (cs) {
    cs->LogStringMessage(message.get());
    MOZ_LOG(gXULTemplateLog, LogLevel::Info, ("Error parsing template: %s", aStr));
  }
}
