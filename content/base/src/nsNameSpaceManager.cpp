









































#include "nscore.h"
#include "nsINameSpaceManager.h"
#include "nsAutoPtr.h"
#include "nsINodeInfo.h"
#include "nsCOMArray.h"
#include "nsContentCreatorFunctions.h"
#include "nsString.h"

#ifdef MOZ_XTF
#include "nsIServiceManager.h"
#include "nsIXTFService.h"
#include "nsContentUtils.h"
static NS_DEFINE_CID(kXTFServiceCID, NS_XTFSERVICE_CID);
#endif

#ifdef MOZ_SVG
PRBool NS_SVGEnabled();
#endif

#define kXMLNSNameSpaceURI "http://www.w3.org/2000/xmlns/"
#define kXMLNameSpaceURI "http://www.w3.org/XML/1998/namespace"
#define kXHTMLNameSpaceURI "http://www.w3.org/1999/xhtml"
#define kXLinkNameSpaceURI "http://www.w3.org/1999/xlink"
#define kXSLTNameSpaceURI "http://www.w3.org/1999/XSL/Transform"
#define kXBLNameSpaceURI "http://www.mozilla.org/xbl"
#define kMathMLNameSpaceURI "http://www.w3.org/1998/Math/MathML"
#define kRDFNameSpaceURI "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define kXULNameSpaceURI "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul"
#define kSVGNameSpaceURI "http://www.w3.org/2000/svg"
#define kXMLEventsNameSpaceURI "http://www.w3.org/2001/xml-events"

static nsINameSpaceManager* sNameSpaceManager = nsnull;

NS_IMPL_ISUPPORTS1(nsINameSpaceManager, nsINameSpaceManager)

nsresult nsINameSpaceManager::Init()
{
  nsresult rv = mURIToIDTable.Init(32);
  NS_ENSURE_SUCCESS(rv, rv);

#define REGISTER_NAMESPACE(uri, id) \
  rv = AddNameSpace(NS_LITERAL_STRING(uri), id); \
  NS_ENSURE_SUCCESS(rv, rv)

  
  REGISTER_NAMESPACE(kXMLNSNameSpaceURI, kNameSpaceID_XMLNS);
  REGISTER_NAMESPACE(kXMLNameSpaceURI, kNameSpaceID_XML);
  REGISTER_NAMESPACE(kXHTMLNameSpaceURI, kNameSpaceID_XHTML);
  REGISTER_NAMESPACE(kXLinkNameSpaceURI, kNameSpaceID_XLink);
  REGISTER_NAMESPACE(kXSLTNameSpaceURI, kNameSpaceID_XSLT);
  REGISTER_NAMESPACE(kXBLNameSpaceURI, kNameSpaceID_XBL);
  REGISTER_NAMESPACE(kMathMLNameSpaceURI, kNameSpaceID_MathML);
  REGISTER_NAMESPACE(kRDFNameSpaceURI, kNameSpaceID_RDF);
  REGISTER_NAMESPACE(kXULNameSpaceURI, kNameSpaceID_XUL);
  REGISTER_NAMESPACE(kSVGNameSpaceURI, kNameSpaceID_SVG);
  REGISTER_NAMESPACE(kXMLEventsNameSpaceURI, kNameSpaceID_XMLEvents);

#undef REGISTER_NAMESPACE

  return NS_OK;
}

nsresult
nsINameSpaceManager::RegisterNameSpace(const nsAString& aURI, 
                                       PRInt32& aNameSpaceID)
{
  if (aURI.IsEmpty()) {
    aNameSpaceID = kNameSpaceID_None; 

    return NS_OK;
  }

  nsresult rv = NS_OK;
  if (!mURIToIDTable.Get(&aURI, &aNameSpaceID)) {
    aNameSpaceID = mURIArray.Count() + 1; 

    rv = AddNameSpace(aURI, aNameSpaceID);
    if (NS_FAILED(rv)) {
      aNameSpaceID = kNameSpaceID_Unknown;
    }
  }

  NS_POSTCONDITION(aNameSpaceID >= -1, "Bogus namespace ID");
  
  return rv;
}

nsresult
nsINameSpaceManager::GetNameSpaceURI(PRInt32 aNameSpaceID, nsAString& aURI)
{
  NS_PRECONDITION(aNameSpaceID >= 0, "Bogus namespace ID");
  
  PRInt32 index = aNameSpaceID - 1; 
  if (index < 0 || index >= mURIArray.Count()) {
    aURI.Truncate();

    return NS_ERROR_ILLEGAL_VALUE;
  }

  mURIArray.StringAt(index, aURI);

  return NS_OK;
}

PRInt32
nsINameSpaceManager::GetNameSpaceID(const nsAString& aURI)
{
  if (aURI.IsEmpty()) {
    return kNameSpaceID_None; 
  }

  PRInt32 nameSpaceID;

  if (mURIToIDTable.Get(&aURI, &nameSpaceID)) {
    NS_POSTCONDITION(nameSpaceID >= 0, "Bogus namespace ID");
    return nameSpaceID;
  }

  return kNameSpaceID_Unknown;
}

nsresult
NS_NewElement(nsIContent** aResult, PRInt32 aElementType,
              nsINodeInfo* aNodeInfo, PRBool aFromParser)
{
  if (aElementType == kNameSpaceID_XHTML) {
    return NS_NewHTMLElement(aResult, aNodeInfo, aFromParser);
  }
#ifdef MOZ_XUL
  if (aElementType == kNameSpaceID_XUL) {
    return NS_NewXULElement(aResult, aNodeInfo);
  }
#endif
#ifdef MOZ_MATHML
  if (aElementType == kNameSpaceID_MathML) {
    return NS_NewMathMLElement(aResult, aNodeInfo);
  }
#endif
#ifdef MOZ_SVG
  if (aElementType == kNameSpaceID_SVG && NS_SVGEnabled()) {
    return NS_NewSVGElement(aResult, aNodeInfo);
  }
#endif
  if (aElementType == kNameSpaceID_XMLEvents) {
    return NS_NewXMLEventsElement(aResult, aNodeInfo);
  }
#ifdef MOZ_XTF
  if (aElementType > kNameSpaceID_LastBuiltin) {
    nsIXTFService* xtfService = nsContentUtils::GetXTFService();
    NS_ASSERTION(xtfService, "could not get xtf service");
    if (xtfService &&
        NS_SUCCEEDED(xtfService->CreateElement(aResult, aNodeInfo)))
      return NS_OK;
  }
#endif
  return NS_NewXMLElement(aResult, aNodeInfo);
}

PRBool
nsINameSpaceManager::HasElementCreator(PRInt32 aNameSpaceID)
{
  return aNameSpaceID == kNameSpaceID_XHTML ||
#ifdef MOZ_XUL
         aNameSpaceID == kNameSpaceID_XUL ||
#endif
#ifdef MOZ_MATHML
         aNameSpaceID == kNameSpaceID_MathML ||
#endif
#ifdef MOZ_SVG
         aNameSpaceID == kNameSpaceID_SVG ||
#endif
         aNameSpaceID == kNameSpaceID_XMLEvents ||
    PR_FALSE;
}

nsresult nsINameSpaceManager::AddNameSpace(const nsAString& aURI,
                                           const PRInt32 aNameSpaceID)
{
  if (aNameSpaceID < 0) {
    
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  NS_ASSERTION(aNameSpaceID - 1 == mURIArray.Count(),
               "BAD! AddNameSpace not called in right order!");

  if (!mURIArray.AppendString(aURI)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  const nsString* uri = mURIArray.StringAt(aNameSpaceID - 1);
  if (!mURIToIDTable.Put(uri, aNameSpaceID)) {
    mURIArray.RemoveStringAt(aNameSpaceID - 1);

    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

nsresult
NS_GetNameSpaceManager(nsINameSpaceManager** aInstancePtrResult)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);

  if (!sNameSpaceManager) {
    nsCOMPtr<nsINameSpaceManager> manager = new nsINameSpaceManager();
    if (manager) {
      nsresult rv = manager->Init();
      if (NS_SUCCEEDED(rv)) {
        manager.swap(sNameSpaceManager);
      }
    }
  }

  *aInstancePtrResult = sNameSpaceManager;
  NS_ENSURE_TRUE(sNameSpaceManager, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aInstancePtrResult);

  return NS_OK;
}

void
NS_NameSpaceManagerShutdown()
{
  NS_IF_RELEASE(sNameSpaceManager);
}
