

























#include "prlog.h"
#include "nsNetUtil.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMParser.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMNode.h"
#include "nsString.h"
#include "IMPDManager.h"
#include "nsDASHMPDParser.h"

#if defined(PR_LOGGING)
static PRLogModuleInfo* gDASHMPDParserLog = nullptr;
#define LOG(msg, ...) PR_LOG(gDASHMPDParserLog, PR_LOG_DEBUG, \
                             ("%p [nsDASHMPDParser] " msg, this, ##__VA_ARGS__))
#else
#define LOG(msg, ...)
#endif

namespace mozilla {
namespace net {

nsDASHMPDParser::nsDASHMPDParser(char*         aMPDData,
                                 uint32_t      aDataLength,
                                 nsIPrincipal* aPrincipal,
                                 nsIURI*       aURI) :
  mData(aMPDData),
  mDataLength(aDataLength),
  mPrincipal(aPrincipal),
  mURI(aURI)
{
  MOZ_COUNT_CTOR(nsDASHMPDParser);
#if defined(PR_LOGGING)
  if(!gDASHMPDParserLog)
    gDASHMPDParserLog = PR_NewLogModule("nsDASHMPDParser");
#endif
}

nsDASHMPDParser::~nsDASHMPDParser()
{
  MOZ_COUNT_DTOR(nsDASHMPDParser);
}


nsresult
nsDASHMPDParser::Parse(IMPDManager**    aMPDManager,
                       DASHMPDProfile*  aProfile)
{
  NS_ENSURE_ARG(aMPDManager);
  NS_ENSURE_ARG(aProfile);
  NS_ENSURE_TRUE(mData, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPrincipal, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mURI, NS_ERROR_NOT_INITIALIZED);

#ifdef PR_LOGGING
  {
    nsAutoCString spec;
    nsresult rv = mURI->GetSpec(spec);
    if (NS_FAILED(rv)) {
      LOG("Preparing to parse MPD: cannot get spec from URI");
    } else {
      LOG("Preparing to parse MPD: mURI:\"%s\"", spec.get());
    }
  }
#endif

  
  nsCOMPtr<nsIDOMParser> DOMParser;
  DOMParser = do_CreateInstance(NS_DOMPARSER_CONTRACTID);
  nsresult rv = DOMParser->Init(mPrincipal, mURI, nullptr, nullptr);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMDocument> doc;
  rv = DOMParser->ParseFromBuffer(reinterpret_cast<uint8_t const *>(mData.get()),
                                  mDataLength,
                                  "application/xml",
                                  getter_AddRefs(doc));
  NS_ENSURE_SUCCESS(rv, rv);
  if(!doc) {
    LOG("ERROR! Document not parsed as XML!");
    return NS_ERROR_NO_INTERFACE;
  }
  
  nsCOMPtr<nsIDOMElement> root;
  rv = doc->GetDocumentElement(getter_AddRefs(root));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(root, NS_ERROR_NULL_POINTER);
#ifdef PR_LOGGING
  PrintDOMElements(root);
#endif
  rv = GetProfile(root, *aProfile);
  NS_ENSURE_SUCCESS(rv, rv);
  *aMPDManager = IMPDManager::Create(*aProfile, root);
  NS_ENSURE_TRUE(*aMPDManager, NS_ERROR_NULL_POINTER);

  
  return rv;
}

void
nsDASHMPDParser::PrintDOMElement(nsIDOMElement* aElem, int32_t offset)
{
  
  nsAutoString ss;
  
  for(int32_t i = 0; i < offset; i++)
    ss.Append(NS_LITERAL_STRING(" "));
  
  nsAutoString tagName;
  NS_ENSURE_SUCCESS(aElem->GetTagName(tagName),);
  ss += NS_LITERAL_STRING("<");
  ss += tagName;

  
  nsCOMPtr<nsIDOMNamedNodeMap> attributes;
  NS_ENSURE_SUCCESS(aElem->GetAttributes(getter_AddRefs(attributes)),);

  uint32_t count;
  NS_ENSURE_SUCCESS(attributes->GetLength(&count),);

  for(uint32_t i = 0; i < count; i++)
  {
    ss += NS_LITERAL_STRING(" ");
    nsCOMPtr<nsIDOMNode> node;
    NS_ENSURE_SUCCESS(attributes->Item(i, getter_AddRefs(node)), );

    nsAutoString nodeName;
    NS_ENSURE_SUCCESS(node->GetNodeName(nodeName),);
    ss += nodeName;

    nsAutoString nodeValue;
    NS_ENSURE_SUCCESS(node->GetNodeValue(nodeValue),);
    if(!nodeValue.IsEmpty()) {
      ss += NS_LITERAL_STRING("=");
      ss += nodeValue;
    }
  }
  ss += NS_LITERAL_STRING(">");
  LOG("%s", NS_ConvertUTF16toUTF8(ss).get());

  offset++;

  
  nsCOMPtr<nsIDOMElement> child;
  NS_ENSURE_SUCCESS(aElem->GetFirstElementChild(getter_AddRefs(child)),);

  while(child)
  {
    PrintDOMElement(child, offset);
    NS_ENSURE_SUCCESS(child->GetNextElementSibling(getter_AddRefs(child)),);
  }
}


void
nsDASHMPDParser::PrintDOMElements(nsIDOMElement* aRoot)
{
  NS_ENSURE_TRUE(aRoot, );

  DASHMPDProfile profile;
  NS_ENSURE_SUCCESS(GetProfile(aRoot, profile), );
  LOG("Profile Is %d",(int32_t)profile);
  PrintDOMElement(aRoot, 0);
}


nsresult
nsDASHMPDParser::GetProfile(nsIDOMElement* aRoot,
                            DASHMPDProfile &aProfile)
{
  NS_ENSURE_ARG(aRoot);

  nsAutoString profileStr;
  nsresult rv = aRoot->GetAttribute(NS_LITERAL_STRING("profiles"), profileStr);
  LOG("profileStr: %s", NS_ConvertUTF16toUTF8(profileStr).get());
  NS_ENSURE_SUCCESS(rv, rv);

  if (profileStr
      == NS_LITERAL_STRING("urn:webm:dash:profile:webm-on-demand:2012")) {
    aProfile = WebMOnDemand;
  } else {
    aProfile = NotValid;
  }
  return NS_OK;
}


}
}
