





































#include "nsIAtom.h"
#include "nsString.h"
#include "nspr.h"
#include "nsCOMPtr.h"
#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsParserCIID.h"
#include "nsParser.h"
#include "CNavDTD.h"
#include "nsHTMLEntities.h"
#include "nsHTMLTokenizer.h"

#include "nsElementTable.h"
#include "nsParserService.h"
#include "nsSAXAttributes.h"
#include "nsSAXLocator.h"
#include "nsSAXXMLReader.h"

#ifdef MOZ_VIEW_SOURCE
#include "nsViewSourceHTML.h"
#endif

#if defined(NS_DEBUG)
#include "nsLoggingSink.h"
#include "nsExpatDriver.h"
#endif



#if defined(NS_DEBUG)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLoggingSink)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsExpatDriver)
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR(nsParser)
NS_GENERIC_FACTORY_CONSTRUCTOR(CNavDTD)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsParserService)

#ifdef MOZ_VIEW_SOURCE
NS_GENERIC_FACTORY_CONSTRUCTOR(CViewSourceHTML)
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR(nsSAXAttributes)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSAXXMLReader)

static const nsModuleComponentInfo gComponents[] = {

#if defined(NS_DEBUG)
  { "Logging sink", NS_LOGGING_SINK_CID, NULL, nsLoggingSinkConstructor },
  { "Expat Driver", NS_EXPAT_DRIVER_CID, NULL, nsExpatDriverConstructor },
#endif

  { "Parser", NS_PARSER_CID, NULL, nsParserConstructor },
  { "Navigator HTML DTD", NS_CNAVDTD_CID, NULL, CNavDTDConstructor },
#ifdef MOZ_VIEW_SOURCE
  { "ViewSource DTD", NS_VIEWSOURCE_DTD_CID, NULL, CViewSourceHTMLConstructor },
#endif
  { "ParserService",
    NS_PARSERSERVICE_CID,
    NS_PARSERSERVICE_CONTRACTID,
    nsParserServiceConstructor
  },

  {
    NS_SAXATTRIBUTES_CLASSNAME,
    NS_SAXATTRIBUTES_CID,
    NS_SAXATTRIBUTES_CONTRACTID,
    nsSAXAttributesConstructor
  },
  
  {
    NS_SAXXMLREADER_CLASSNAME,
    NS_SAXXMLREADER_CID,
    NS_SAXXMLREADER_CONTRACTID,
    nsSAXXMLReaderConstructor
  }
};

static PRBool gInitialized = PR_FALSE;

static nsresult
Initialize(nsIModule* aSelf)
{
  if (!gInitialized) {
    nsresult rv = nsHTMLTags::AddRefTable();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = nsHTMLEntities::AddRefTable();
    if (NS_FAILED(rv)) {
      nsHTMLTags::ReleaseTable();
      return rv;
    }
#ifdef NS_DEBUG
    CheckElementTable();
#endif
    CNewlineToken::AllocNewline();
    gInitialized = PR_TRUE;
  }

#ifdef DEBUG
  nsHTMLTags::TestTagTable();
#endif

  return nsParser::Init();
}

static void
Shutdown(nsIModule* aSelf)
{
  if (gInitialized) {
    nsHTMLTags::ReleaseTable();
    nsHTMLEntities::ReleaseTable();
    nsDTDContext::ReleaseGlobalObjects();
    nsParser::Shutdown();
    CNewlineToken::FreeNewline();
    gInitialized = PR_FALSE;
  }
}

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(nsParserModule, gComponents, Initialize, Shutdown)
