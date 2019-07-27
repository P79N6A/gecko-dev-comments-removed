




#include "nsExpatDriver.h"
#include "nsCOMPtr.h"
#include "nsParserCIID.h"
#include "CParserContext.h"
#include "nsIExpatSink.h"
#include "nsIExtendedExpatSink.h"
#include "nsIContentSink.h"
#include "nsParserMsgUtils.h"
#include "nsIURL.h"
#include "nsIUnicharInputStream.h"
#include "nsISimpleUnicharStreamFactory.h"
#include "nsNetUtil.h"
#include "nsNullPrincipal.h"
#include "prprf.h"
#include "prmem.h"
#include "nsTextFormatter.h"
#include "nsDirectoryServiceDefs.h"
#include "nsCRT.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsError.h"
#include "nsXPCOMCIDInternal.h"
#include "nsUnicharInputStream.h"

#define kExpatSeparatorChar 0xFFFF

static const char16_t kUTF16[] = { 'U', 'T', 'F', '-', '1', '6', '\0' };

#ifdef PR_LOGGING
static PRLogModuleInfo *
GetExpatDriverLog()
{
  static PRLogModuleInfo *sLog;
  if (!sLog)
    sLog = PR_NewLogModule("expatdriver");
  return sLog;
}
#endif




static void
Driver_HandleXMLDeclaration(void *aUserData,
                            const XML_Char *aVersion,
                            const XML_Char *aEncoding,
                            int aStandalone)
{
  NS_ASSERTION(aUserData, "expat driver should exist");
  if (aUserData) {
    nsExpatDriver* driver = static_cast<nsExpatDriver*>(aUserData);
    driver->HandleXMLDeclaration(aVersion, aEncoding, aStandalone);
  }
}

static void
Driver_HandleStartElement(void *aUserData,
                          const XML_Char *aName,
                          const XML_Char **aAtts)
{
  NS_ASSERTION(aUserData, "expat driver should exist");
  if (aUserData) {
    static_cast<nsExpatDriver*>(aUserData)->HandleStartElement(aName,
                                                                  aAtts);
  }
}

static void
Driver_HandleEndElement(void *aUserData,
                        const XML_Char *aName)
{
  NS_ASSERTION(aUserData, "expat driver should exist");
  if (aUserData) {
    static_cast<nsExpatDriver*>(aUserData)->HandleEndElement(aName);
  }
}

static void
Driver_HandleCharacterData(void *aUserData,
                           const XML_Char *aData,
                           int aLength)
{
  NS_ASSERTION(aUserData, "expat driver should exist");
  if (aUserData) {
    nsExpatDriver* driver = static_cast<nsExpatDriver*>(aUserData);
    driver->HandleCharacterData(aData, uint32_t(aLength));
  }
}

static void
Driver_HandleComment(void *aUserData,
                     const XML_Char *aName)
{
  NS_ASSERTION(aUserData, "expat driver should exist");
  if(aUserData) {
    static_cast<nsExpatDriver*>(aUserData)->HandleComment(aName);
  }
}

static void
Driver_HandleProcessingInstruction(void *aUserData,
                                   const XML_Char *aTarget,
                                   const XML_Char *aData)
{
  NS_ASSERTION(aUserData, "expat driver should exist");
  if (aUserData) {
    nsExpatDriver* driver = static_cast<nsExpatDriver*>(aUserData);
    driver->HandleProcessingInstruction(aTarget, aData);
  }
}

static void
Driver_HandleDefault(void *aUserData,
                     const XML_Char *aData,
                     int aLength)
{
  NS_ASSERTION(aUserData, "expat driver should exist");
  if (aUserData) {
    nsExpatDriver* driver = static_cast<nsExpatDriver*>(aUserData);
    driver->HandleDefault(aData, uint32_t(aLength));
  }
}

static void
Driver_HandleStartCdataSection(void *aUserData)
{
  NS_ASSERTION(aUserData, "expat driver should exist");
  if (aUserData) {
    static_cast<nsExpatDriver*>(aUserData)->HandleStartCdataSection();
  }
}

static void
Driver_HandleEndCdataSection(void *aUserData)
{
  NS_ASSERTION(aUserData, "expat driver should exist");
  if (aUserData) {
    static_cast<nsExpatDriver*>(aUserData)->HandleEndCdataSection();
  }
}

static void
Driver_HandleStartDoctypeDecl(void *aUserData,
                              const XML_Char *aDoctypeName,
                              const XML_Char *aSysid,
                              const XML_Char *aPubid,
                              int aHasInternalSubset)
{
  NS_ASSERTION(aUserData, "expat driver should exist");
  if (aUserData) {
    static_cast<nsExpatDriver*>(aUserData)->
      HandleStartDoctypeDecl(aDoctypeName, aSysid, aPubid, !!aHasInternalSubset);
  }
}

static void
Driver_HandleEndDoctypeDecl(void *aUserData)
{
  NS_ASSERTION(aUserData, "expat driver should exist");
  if (aUserData) {
    static_cast<nsExpatDriver*>(aUserData)->HandleEndDoctypeDecl();
  }
}

static int
Driver_HandleExternalEntityRef(void *aExternalEntityRefHandler,
                               const XML_Char *aOpenEntityNames,
                               const XML_Char *aBase,
                               const XML_Char *aSystemId,
                               const XML_Char *aPublicId)
{
  NS_ASSERTION(aExternalEntityRefHandler, "expat driver should exist");
  if (!aExternalEntityRefHandler) {
    return 1;
  }

  nsExpatDriver* driver = static_cast<nsExpatDriver*>
                                     (aExternalEntityRefHandler);

  return driver->HandleExternalEntityRef(aOpenEntityNames, aBase, aSystemId,
                                         aPublicId);
}

static void
Driver_HandleStartNamespaceDecl(void *aUserData,
                                const XML_Char *aPrefix,
                                const XML_Char *aUri)
{
  NS_ASSERTION(aUserData, "expat driver should exist");
  if (aUserData) {
    static_cast<nsExpatDriver*>(aUserData)->
      HandleStartNamespaceDecl(aPrefix, aUri);
  }
}

static void
Driver_HandleEndNamespaceDecl(void *aUserData,
                              const XML_Char *aPrefix)
{
  NS_ASSERTION(aUserData, "expat driver should exist");
  if (aUserData) {
    static_cast<nsExpatDriver*>(aUserData)->
      HandleEndNamespaceDecl(aPrefix);
  }
}

static void
Driver_HandleNotationDecl(void *aUserData,
                          const XML_Char *aNotationName,
                          const XML_Char *aBase,
                          const XML_Char *aSysid,
                          const XML_Char *aPubid)
{
  NS_ASSERTION(aUserData, "expat driver should exist");
  if (aUserData) {
    static_cast<nsExpatDriver*>(aUserData)->
      HandleNotationDecl(aNotationName, aBase, aSysid, aPubid);
  }
}

static void
Driver_HandleUnparsedEntityDecl(void *aUserData,
                                const XML_Char *aEntityName,
                                const XML_Char *aBase,
                                const XML_Char *aSysid,
                                const XML_Char *aPubid,
                                const XML_Char *aNotationName)
{
  NS_ASSERTION(aUserData, "expat driver should exist");
  if (aUserData) {
    static_cast<nsExpatDriver*>(aUserData)->
      HandleUnparsedEntityDecl(aEntityName, aBase, aSysid, aPubid,
                               aNotationName);
  }
}












struct nsCatalogData {
  const char* mPublicID;
  const char* mLocalDTD;
  const char* mAgentSheet;
};


static const nsCatalogData kCatalogTable[] = {
  { "-//W3C//DTD XHTML 1.0 Transitional//EN",    "htmlmathml-f.ent", nullptr },
  { "-//W3C//DTD XHTML 1.1//EN",                 "htmlmathml-f.ent", nullptr },
  { "-//W3C//DTD XHTML 1.0 Strict//EN",          "htmlmathml-f.ent", nullptr },
  { "-//W3C//DTD XHTML 1.0 Frameset//EN",        "htmlmathml-f.ent", nullptr },
  { "-//W3C//DTD XHTML Basic 1.0//EN",           "htmlmathml-f.ent", nullptr },
  { "-//W3C//DTD XHTML 1.1 plus MathML 2.0//EN", "htmlmathml-f.ent", nullptr },
  { "-//W3C//DTD XHTML 1.1 plus MathML 2.0 plus SVG 1.1//EN", "htmlmathml-f.ent", nullptr },
  { "-//W3C//DTD MathML 2.0//EN",                "htmlmathml-f.ent", nullptr },
  { "-//WAPFORUM//DTD XHTML Mobile 1.0//EN",     "htmlmathml-f.ent", nullptr },
  { nullptr, nullptr, nullptr }
};

static const nsCatalogData*
LookupCatalogData(const char16_t* aPublicID)
{
  nsDependentString publicID(aPublicID);

  
  
  
  const nsCatalogData* data = kCatalogTable;
  while (data->mPublicID) {
    if (publicID.EqualsASCII(data->mPublicID)) {
      return data;
    }
    ++data;
  }

  return nullptr;
}





static void
GetLocalDTDURI(const nsCatalogData* aCatalogData, nsIURI* aDTD,
              nsIURI** aResult)
{
  NS_ASSERTION(aDTD, "Null parameter.");

  nsAutoCString fileName;
  if (aCatalogData) {
    
    fileName.Assign(aCatalogData->mLocalDTD);
  }

  if (fileName.IsEmpty()) {
    
    
    
    
    nsCOMPtr<nsIURL> dtdURL = do_QueryInterface(aDTD);
    if (!dtdURL) {
      return;
    }

    dtdURL->GetFileName(fileName);
    if (fileName.IsEmpty()) {
      return;
    }
  }

  nsAutoCString respath("resource://gre/res/dtd/");
  respath += fileName;
  NS_NewURI(aResult, respath);
}



NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsExpatDriver)
  NS_INTERFACE_MAP_ENTRY(nsITokenizer)
  NS_INTERFACE_MAP_ENTRY(nsIDTD)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDTD)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsExpatDriver)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsExpatDriver)

NS_IMPL_CYCLE_COLLECTION(nsExpatDriver, mSink, mExtendedSink)

nsExpatDriver::nsExpatDriver()
  : mExpatParser(nullptr),
    mInCData(false),
    mInInternalSubset(false),
    mInExternalDTD(false),
    mMadeFinalCallToExpat(false),
    mIsFinalChunk(false),
    mInternalState(NS_OK),
    mExpatBuffered(0),
    mCatalogData(nullptr),
    mInnerWindowID(0)
{
}

nsExpatDriver::~nsExpatDriver()
{
  if (mExpatParser) {
    XML_ParserFree(mExpatParser);
  }
}

nsresult
nsExpatDriver::HandleStartElement(const char16_t *aValue,
                                  const char16_t **aAtts)
{
  NS_ASSERTION(mSink, "content sink not found!");

  
  
  
  
  uint32_t attrArrayLength;
  for (attrArrayLength = XML_GetSpecifiedAttributeCount(mExpatParser);
       aAtts[attrArrayLength];
       attrArrayLength += 2) {
    
  }

  if (mSink) {
    nsresult rv = mSink->
      HandleStartElement(aValue, aAtts, attrArrayLength,
                         XML_GetCurrentLineNumber(mExpatParser));
    MaybeStopParser(rv);
  }

  return NS_OK;
}

nsresult
nsExpatDriver::HandleEndElement(const char16_t *aValue)
{
  NS_ASSERTION(mSink, "content sink not found!");
  NS_ASSERTION(mInternalState != NS_ERROR_HTMLPARSER_BLOCK,
               "Shouldn't block from HandleStartElement.");

  if (mSink && mInternalState != NS_ERROR_HTMLPARSER_STOPPARSING) {
    nsresult rv = mSink->HandleEndElement(aValue);
    MaybeStopParser(rv);
  }

  return NS_OK;
}

nsresult
nsExpatDriver::HandleCharacterData(const char16_t *aValue,
                                   const uint32_t aLength)
{
  NS_ASSERTION(mSink, "content sink not found!");

  if (mInCData) {
    mCDataText.Append(aValue, aLength);
  }
  else if (mSink) {
    nsresult rv = mSink->HandleCharacterData(aValue, aLength);
    MaybeStopParser(rv);
  }

  return NS_OK;
}

nsresult
nsExpatDriver::HandleComment(const char16_t *aValue)
{
  NS_ASSERTION(mSink, "content sink not found!");

  if (mInExternalDTD) {
    
    return NS_OK;
  }

  if (mInInternalSubset) {
    mInternalSubset.AppendLiteral("<!--");
    mInternalSubset.Append(aValue);
    mInternalSubset.AppendLiteral("-->");
  }
  else if (mSink) {
    nsresult rv = mSink->HandleComment(aValue);
    MaybeStopParser(rv);
  }

  return NS_OK;
}

nsresult
nsExpatDriver::HandleProcessingInstruction(const char16_t *aTarget,
                                           const char16_t *aData)
{
  NS_ASSERTION(mSink, "content sink not found!");

  if (mInExternalDTD) {
    
    
    return NS_OK;
  }

  if (mInInternalSubset) {
    mInternalSubset.AppendLiteral("<?");
    mInternalSubset.Append(aTarget);
    mInternalSubset.Append(' ');
    mInternalSubset.Append(aData);
    mInternalSubset.AppendLiteral("?>");
  }
  else if (mSink) {
    nsresult rv = mSink->HandleProcessingInstruction(aTarget, aData);
    MaybeStopParser(rv);
  }

  return NS_OK;
}

nsresult
nsExpatDriver::HandleXMLDeclaration(const char16_t *aVersion,
                                    const char16_t *aEncoding,
                                    int32_t aStandalone)
{
  if (mSink) {
    nsresult rv = mSink->HandleXMLDeclaration(aVersion, aEncoding, aStandalone);
    MaybeStopParser(rv);
  }

  return NS_OK;
}

nsresult
nsExpatDriver::HandleDefault(const char16_t *aValue,
                             const uint32_t aLength)
{
  NS_ASSERTION(mSink, "content sink not found!");

  if (mInExternalDTD) {
    
    return NS_OK;
  }

  if (mInInternalSubset) {
    mInternalSubset.Append(aValue, aLength);
  }
  else if (mSink) {
    uint32_t i;
    nsresult rv = mInternalState;
    for (i = 0; i < aLength && NS_SUCCEEDED(rv); ++i) {
      if (aValue[i] == '\n' || aValue[i] == '\r') {
        rv = mSink->HandleCharacterData(&aValue[i], 1);
      }
    }
    MaybeStopParser(rv);
  }

  return NS_OK;
}

nsresult
nsExpatDriver::HandleStartCdataSection()
{
  mInCData = true;

  return NS_OK;
}

nsresult
nsExpatDriver::HandleEndCdataSection()
{
  NS_ASSERTION(mSink, "content sink not found!");

  mInCData = false;
  if (mSink) {
    nsresult rv = mSink->HandleCDataSection(mCDataText.get(),
                                            mCDataText.Length());
    MaybeStopParser(rv);
  }
  mCDataText.Truncate();

  return NS_OK;
}

nsresult
nsExpatDriver::HandleStartNamespaceDecl(const char16_t* aPrefix,
                                        const char16_t* aUri)
{
  if (mExtendedSink) {
    nsresult rv = mExtendedSink->HandleStartNamespaceDecl(aPrefix, aUri);
    MaybeStopParser(rv);
  }
  return NS_OK;
}

nsresult
nsExpatDriver::HandleEndNamespaceDecl(const char16_t* aPrefix)
{
  if (mExtendedSink && mInternalState != NS_ERROR_HTMLPARSER_STOPPARSING) {
    nsresult rv = mExtendedSink->HandleEndNamespaceDecl(aPrefix);
    MaybeStopParser(rv);
  }
  return NS_OK;
}

nsresult
nsExpatDriver::HandleNotationDecl(const char16_t* aNotationName,
                                  const char16_t* aBase,
                                  const char16_t* aSysid,
                                  const char16_t* aPubid)
{
  if (mExtendedSink) {
    nsresult rv = mExtendedSink->HandleNotationDecl(aNotationName, aSysid,
                                                    aPubid);
    MaybeStopParser(rv);
  }
  return NS_OK;
}

nsresult
nsExpatDriver::HandleUnparsedEntityDecl(const char16_t* aEntityName,
                                        const char16_t* aBase,
                                        const char16_t* aSysid,
                                        const char16_t* aPubid,
                                        const char16_t* aNotationName)
{
  if (mExtendedSink) {
    nsresult rv = mExtendedSink->HandleUnparsedEntityDecl(aEntityName,
                                                          aSysid,
                                                          aPubid,
                                                          aNotationName);
    MaybeStopParser(rv);
  }
  return NS_OK;
}

nsresult
nsExpatDriver::HandleStartDoctypeDecl(const char16_t* aDoctypeName,
                                      const char16_t* aSysid,
                                      const char16_t* aPubid,
                                      bool aHasInternalSubset)
{
  mDoctypeName = aDoctypeName;
  mSystemID = aSysid;
  mPublicID = aPubid;

  if (mExtendedSink) {
    nsresult rv = mExtendedSink->HandleStartDTD(aDoctypeName, aSysid, aPubid);
    MaybeStopParser(rv);
  }

  if (aHasInternalSubset) {
    
    
    
    mInInternalSubset = true;
    mInternalSubset.SetCapacity(1024);
  } else {
    
    mInternalSubset.SetIsVoid(true);
  }

  return NS_OK;
}

nsresult
nsExpatDriver::HandleEndDoctypeDecl()
{
  NS_ASSERTION(mSink, "content sink not found!");

  mInInternalSubset = false;

  if (mSink) {
    
    
    
    nsCOMPtr<nsIURI> data;
#if 0
    if (mCatalogData && mCatalogData->mAgentSheet) {
      NS_NewURI(getter_AddRefs(data), mCatalogData->mAgentSheet);
    }
#endif

    
    
    MOZ_ASSERT(!mCatalogData || !mCatalogData->mAgentSheet,
               "Need to add back support for catalog style sheets");

    
    nsresult rv = mSink->HandleDoctypeDecl(mInternalSubset, mDoctypeName,
                                           mSystemID, mPublicID, data);
    MaybeStopParser(rv);
  }
  
  mInternalSubset.SetCapacity(0);

  return NS_OK;
}

static NS_METHOD
ExternalDTDStreamReaderFunc(nsIUnicharInputStream* aIn,
                            void* aClosure,
                            const char16_t* aFromSegment,
                            uint32_t aToOffset,
                            uint32_t aCount,
                            uint32_t *aWriteCount)
{
  
  if (XML_Parse((XML_Parser)aClosure, (const char *)aFromSegment,
                aCount * sizeof(char16_t), 0) == XML_STATUS_OK) {
    *aWriteCount = aCount;

    return NS_OK;
  }

  *aWriteCount = 0;

  return NS_ERROR_FAILURE;
}

int
nsExpatDriver::HandleExternalEntityRef(const char16_t *openEntityNames,
                                       const char16_t *base,
                                       const char16_t *systemId,
                                       const char16_t *publicId)
{
  if (mInInternalSubset && !mInExternalDTD && openEntityNames) {
    mInternalSubset.Append(char16_t('%'));
    mInternalSubset.Append(nsDependentString(openEntityNames));
    mInternalSubset.Append(char16_t(';'));
  }

  
  nsCOMPtr<nsIInputStream> in;
  nsAutoString absURL;
  nsresult rv = OpenInputStreamFromExternalDTD(publicId, systemId, base,
                                               getter_AddRefs(in), absURL);
  if (NS_FAILED(rv)) {
#ifdef DEBUG
    nsCString message("Failed to open external DTD: publicId \"");
    AppendUTF16toUTF8(publicId, message);
    message += "\" systemId \"";
    AppendUTF16toUTF8(systemId, message);
    message += "\" base \"";
    AppendUTF16toUTF8(base, message);
    message += "\" URL \"";
    AppendUTF16toUTF8(absURL, message);
    message += "\"";
    NS_WARNING(message.get());
#endif
    return 1;
  }

  nsCOMPtr<nsIUnicharInputStream> uniIn;
  rv = nsSimpleUnicharStreamFactory::GetInstance()->
    CreateInstanceFromUTF8Stream(in, getter_AddRefs(uniIn));
  NS_ENSURE_SUCCESS(rv, 1);

  int result = 1;
  if (uniIn) {
    XML_Parser entParser = XML_ExternalEntityParserCreate(mExpatParser, 0,
                                                          kUTF16);
    if (entParser) {
      XML_SetBase(entParser, absURL.get());

      mInExternalDTD = true;

      uint32_t totalRead;
      do {
        rv = uniIn->ReadSegments(ExternalDTDStreamReaderFunc, entParser,
                                 uint32_t(-1), &totalRead);
      } while (NS_SUCCEEDED(rv) && totalRead > 0);

      result = XML_Parse(entParser, nullptr, 0, 1);

      mInExternalDTD = false;

      XML_ParserFree(entParser);
    }
  }

  return result;
}

nsresult
nsExpatDriver::OpenInputStreamFromExternalDTD(const char16_t* aFPIStr,
                                              const char16_t* aURLStr,
                                              const char16_t* aBaseURL,
                                              nsIInputStream** aStream,
                                              nsAString& aAbsURL)
{
  nsCOMPtr<nsIURI> baseURI;
  nsresult rv = NS_NewURI(getter_AddRefs(baseURI),
                          NS_ConvertUTF16toUTF8(aBaseURL));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), NS_ConvertUTF16toUTF8(aURLStr), nullptr,
                 baseURI);
  NS_ENSURE_SUCCESS(rv, rv);

  
  bool isChrome = false;
  uri->SchemeIs("chrome", &isChrome);
  if (!isChrome) {
    
    
    
    if (aFPIStr) {
      
      mCatalogData = LookupCatalogData(aFPIStr);
    }

    nsCOMPtr<nsIURI> localURI;
    GetLocalDTDURI(mCatalogData, uri, getter_AddRefs(localURI));
    if (!localURI) {
      return NS_ERROR_NOT_IMPLEMENTED;
    }

    localURI.swap(uri);
  }

  nsCOMPtr<nsIDocument> doc;
  NS_ASSERTION(mSink == nsCOMPtr<nsIExpatSink>(do_QueryInterface(mOriginalSink)),
               "In nsExpatDriver::OpenInputStreamFromExternalDTD: "
               "mOriginalSink not the same object as mSink?");
  if (mOriginalSink)
    doc = do_QueryInterface(mOriginalSink->GetTarget());
  int16_t shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_DTD,
                                uri,
                                (doc ? doc->NodePrincipal() : nullptr),
                                doc,
                                EmptyCString(), 
                                nullptr,         
                                &shouldLoad);
  if (NS_FAILED(rv)) return rv;
  if (NS_CP_REJECTED(shouldLoad)) {
    
    return NS_ERROR_CONTENT_BLOCKED;
  }

  nsAutoCString absURL;
  uri->GetSpec(absURL);

  CopyUTF8toUTF16(absURL, aAbsURL);

  nsCOMPtr<nsIChannel> channel;
  if (doc) {
    rv = NS_NewChannel(getter_AddRefs(channel),
                       uri,
                       doc,
                       nsILoadInfo::SEC_NORMAL,
                       nsIContentPolicy::TYPE_DTD);
  }
  else {
    nsCOMPtr<nsIPrincipal> nullPrincipal = nsNullPrincipal::Create();
    NS_ENSURE_TRUE(nullPrincipal, NS_ERROR_FAILURE);
    rv = NS_NewChannel(getter_AddRefs(channel),
                       uri,
                       nullPrincipal,
                       nsILoadInfo::SEC_NORMAL,
                       nsIContentPolicy::TYPE_DTD);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  channel->SetContentType(NS_LITERAL_CSTRING("application/xml"));
  return channel->Open(aStream);
}

static nsresult
CreateErrorText(const char16_t* aDescription,
                const char16_t* aSourceURL,
                const uint32_t aLineNumber,
                const uint32_t aColNumber,
                nsString& aErrorString)
{
  aErrorString.Truncate();

  nsAutoString msg;
  nsresult rv =
    nsParserMsgUtils::GetLocalizedStringByName(XMLPARSER_PROPERTIES,
                                               "XMLParsingError", msg);
  NS_ENSURE_SUCCESS(rv, rv);

  
  char16_t *message = nsTextFormatter::smprintf(msg.get(), aDescription,
                                                 aSourceURL, aLineNumber,
                                                 aColNumber);
  if (!message) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  aErrorString.Assign(message);
  nsTextFormatter::smprintf_free(message);

  return NS_OK;
}

static nsresult
AppendErrorPointer(const int32_t aColNumber,
                   const char16_t *aSourceLine,
                   nsString& aSourceString)
{
  aSourceString.Append(char16_t('\n'));

  
  int32_t last = aColNumber - 1;
  int32_t i;
  uint32_t minuses = 0;
  for (i = 0; i < last; ++i) {
    if (aSourceLine[i] == '\t') {
      
      uint32_t add = 8 - (minuses % 8);
      aSourceString.AppendASCII("--------", add);
      minuses += add;
    }
    else {
      aSourceString.Append(char16_t('-'));
      ++minuses;
    }
  }
  aSourceString.Append(char16_t('^'));

  return NS_OK;
}

nsresult
nsExpatDriver::HandleError()
{
  int32_t code = XML_GetErrorCode(mExpatParser);
  NS_ASSERTION(code > XML_ERROR_NONE, "unexpected XML error code");

  
  
  nsAutoString description;
  nsParserMsgUtils::GetLocalizedStringByID(XMLPARSER_PROPERTIES, code,
                                           description);

  if (code == XML_ERROR_TAG_MISMATCH) {
    








    const char16_t *mismatch = MOZ_XML_GetMismatchedTag(mExpatParser);
    const char16_t *uriEnd = nullptr;
    const char16_t *nameEnd = nullptr;
    const char16_t *pos;
    for (pos = mismatch; *pos; ++pos) {
      if (*pos == kExpatSeparatorChar) {
        if (uriEnd) {
          nameEnd = pos;
        }
        else {
          uriEnd = pos;
        }
      }
    }

    nsAutoString tagName;
    if (uriEnd && nameEnd) {
      
      tagName.Append(nameEnd + 1, pos - nameEnd - 1);
      tagName.Append(char16_t(':'));
    }
    const char16_t *nameStart = uriEnd ? uriEnd + 1 : mismatch;
    tagName.Append(nameStart, (nameEnd ? nameEnd : pos) - nameStart);
    
    nsAutoString msg;
    nsParserMsgUtils::GetLocalizedStringByName(XMLPARSER_PROPERTIES,
                                               "Expected", msg);

    
    char16_t *message = nsTextFormatter::smprintf(msg.get(), tagName.get());
    if (!message) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    description.Append(message);

    nsTextFormatter::smprintf_free(message);
  }

  
  uint32_t colNumber = XML_GetCurrentColumnNumber(mExpatParser) + 1;
  uint32_t lineNumber = XML_GetCurrentLineNumber(mExpatParser);

  nsAutoString errorText;
  CreateErrorText(description.get(), XML_GetBase(mExpatParser), lineNumber,
                  colNumber, errorText);

  NS_ASSERTION(mSink, "no sink?");

  nsAutoString sourceText(mLastLine);
  AppendErrorPointer(colNumber, mLastLine.get(), sourceText);

  
  nsCOMPtr<nsIScriptError> serr(do_CreateInstance(NS_SCRIPTERROR_CONTRACTID));
  nsresult rv = NS_ERROR_FAILURE;
  if (serr) {
    rv = serr->InitWithWindowID(description,
                                mURISpec,
                                mLastLine,
                                lineNumber, colNumber,
                                nsIScriptError::errorFlag, "malformed-xml",
                                mInnerWindowID);
  }

  
  bool shouldReportError = NS_SUCCEEDED(rv);

  if (mSink && shouldReportError) {
    rv = mSink->ReportError(errorText.get(), 
                            sourceText.get(), 
                            serr, 
                            &shouldReportError);
    if (NS_FAILED(rv)) {
      shouldReportError = true;
    }
  }

  if (shouldReportError) {
    nsCOMPtr<nsIConsoleService> cs
      (do_GetService(NS_CONSOLESERVICE_CONTRACTID));  
    if (cs) {
      cs->LogMessage(serr);
    }
  }

  return NS_ERROR_HTMLPARSER_STOPPARSING;
}

void
nsExpatDriver::ParseBuffer(const char16_t *aBuffer,
                           uint32_t aLength,
                           bool aIsFinal,
                           uint32_t *aConsumed)
{
  NS_ASSERTION((aBuffer && aLength != 0) || (!aBuffer && aLength == 0), "?");
  NS_ASSERTION(mInternalState != NS_OK || aIsFinal || aBuffer,
               "Useless call, we won't call Expat");
  NS_PRECONDITION(!BlockedOrInterrupted() || !aBuffer,
                  "Non-null buffer when resuming");
  NS_PRECONDITION(XML_GetCurrentByteIndex(mExpatParser) % sizeof(char16_t) == 0,
                  "Consumed part of a char16_t?");

  if (mExpatParser && (mInternalState == NS_OK || BlockedOrInterrupted())) {
    int32_t parserBytesBefore = XML_GetCurrentByteIndex(mExpatParser);
    NS_ASSERTION(parserBytesBefore >= 0, "Unexpected value");

    XML_Status status;
    if (BlockedOrInterrupted()) {
      mInternalState = NS_OK; 
      status = XML_ResumeParser(mExpatParser);
    }
    else {
      status = XML_Parse(mExpatParser,
                         reinterpret_cast<const char*>(aBuffer),
                         aLength * sizeof(char16_t), aIsFinal);
    }

    int32_t parserBytesConsumed = XML_GetCurrentByteIndex(mExpatParser);

    NS_ASSERTION(parserBytesConsumed >= 0, "Unexpected value");
    NS_ASSERTION(parserBytesConsumed >= parserBytesBefore,
                 "How'd this happen?");
    NS_ASSERTION(parserBytesConsumed % sizeof(char16_t) == 0,
                 "Consumed part of a char16_t?");

    
    *aConsumed = (parserBytesConsumed - parserBytesBefore) / sizeof(char16_t);
    NS_ASSERTION(*aConsumed <= aLength + mExpatBuffered,
                 "Too many bytes consumed?");

    NS_ASSERTION(status != XML_STATUS_SUSPENDED || BlockedOrInterrupted(), 
                 "Inconsistent expat suspension state.");

    if (status == XML_STATUS_ERROR) {
      mInternalState = NS_ERROR_HTMLPARSER_STOPPARSING;
    }
  }
  else {
    *aConsumed = 0;
  }
}

NS_IMETHODIMP
nsExpatDriver::ConsumeToken(nsScanner& aScanner, bool& aFlushTokens)
{
  
  
  nsScannerIterator currentExpatPosition;
  aScanner.CurrentPosition(currentExpatPosition);

  
  nsScannerIterator start = currentExpatPosition;
  start.advance(mExpatBuffered);

  
  
  nsScannerIterator end;
  aScanner.EndReading(end);

  PR_LOG(GetExpatDriverLog(), PR_LOG_DEBUG,
         ("Remaining in expat's buffer: %i, remaining in scanner: %i.",
          mExpatBuffered, Distance(start, end)));

  
  
  
  while (start != end || (mIsFinalChunk && !mMadeFinalCallToExpat) ||
         (BlockedOrInterrupted() && mExpatBuffered > 0)) {
    bool noMoreBuffers = start == end && mIsFinalChunk;
    bool blocked = BlockedOrInterrupted();

    const char16_t *buffer;
    uint32_t length;
    if (blocked || noMoreBuffers) {
      
      
      buffer = nullptr;
      length = 0;

#if defined(PR_LOGGING) || defined (DEBUG)
      if (blocked) {
        PR_LOG(GetExpatDriverLog(), PR_LOG_DEBUG,
               ("Resuming Expat, will parse data remaining in Expat's "
                "buffer.\nContent of Expat's buffer:\n-----\n%s\n-----\n",
                NS_ConvertUTF16toUTF8(currentExpatPosition.get(),
                                      mExpatBuffered).get()));
      }
      else {
        NS_ASSERTION(mExpatBuffered == Distance(currentExpatPosition, end),
                     "Didn't pass all the data to Expat?");
        PR_LOG(GetExpatDriverLog(), PR_LOG_DEBUG,
               ("Last call to Expat, will parse data remaining in Expat's "
                "buffer.\nContent of Expat's buffer:\n-----\n%s\n-----\n",
                NS_ConvertUTF16toUTF8(currentExpatPosition.get(),
                                      mExpatBuffered).get()));
      }
#endif
    }
    else {
      buffer = start.get();
      length = uint32_t(start.size_forward());

      PR_LOG(GetExpatDriverLog(), PR_LOG_DEBUG,
             ("Calling Expat, will parse data remaining in Expat's buffer and "
              "new data.\nContent of Expat's buffer:\n-----\n%s\n-----\nNew "
              "data:\n-----\n%s\n-----\n",
              NS_ConvertUTF16toUTF8(currentExpatPosition.get(),
                                    mExpatBuffered).get(),
              NS_ConvertUTF16toUTF8(start.get(), length).get()));
    }

    uint32_t consumed;
    ParseBuffer(buffer, length, noMoreBuffers, &consumed);
    if (consumed > 0) {
      nsScannerIterator oldExpatPosition = currentExpatPosition;
      currentExpatPosition.advance(consumed);

      
      
      

      
      XML_Size lastLineLength = XML_GetCurrentColumnNumber(mExpatParser);

      if (lastLineLength <= consumed) {
        
        
        
        nsScannerIterator startLastLine = currentExpatPosition;
        startLastLine.advance(-((ptrdiff_t)lastLineLength));
        CopyUnicodeTo(startLastLine, currentExpatPosition, mLastLine);
      }
      else {
        
        
        AppendUnicodeTo(oldExpatPosition, currentExpatPosition, mLastLine);
      }
    }

    mExpatBuffered += length - consumed;

    if (BlockedOrInterrupted()) {
      PR_LOG(GetExpatDriverLog(), PR_LOG_DEBUG,
             ("Blocked or interrupted parser (probably for loading linked "
              "stylesheets or scripts)."));

      aScanner.SetPosition(currentExpatPosition, true);
      aScanner.Mark();

      return mInternalState;
    }

    if (noMoreBuffers && mExpatBuffered == 0) {
      mMadeFinalCallToExpat = true;
    }

    if (NS_FAILED(mInternalState)) {
      if (XML_GetErrorCode(mExpatParser) != XML_ERROR_NONE) {
        NS_ASSERTION(mInternalState == NS_ERROR_HTMLPARSER_STOPPARSING,
                     "Unexpected error");

        
        nsScannerIterator lastLine = currentExpatPosition;
        while (lastLine != end) {
          length = uint32_t(lastLine.size_forward());
          uint32_t endOffset = 0;
          const char16_t *buffer = lastLine.get();
          while (endOffset < length && buffer[endOffset] != '\n' &&
                 buffer[endOffset] != '\r') {
            ++endOffset;
          }
          mLastLine.Append(Substring(buffer, buffer + endOffset));
          if (endOffset < length) {
            
            break;
          }

          lastLine.advance(length);
        }

        HandleError();
      }

      return mInternalState;
    }

    
    
    NS_ASSERTION(!noMoreBuffers || blocked ||
                 (mExpatBuffered == 0 && currentExpatPosition == end),
                 "Unreachable data left in Expat's buffer");

    start.advance(length);

    
    
    
    aScanner.EndReading(end);
  }

  aScanner.SetPosition(currentExpatPosition, true);
  aScanner.Mark();

  PR_LOG(GetExpatDriverLog(), PR_LOG_DEBUG,
         ("Remaining in expat's buffer: %i, remaining in scanner: %i.",
          mExpatBuffered, Distance(currentExpatPosition, end)));

  return NS_SUCCEEDED(mInternalState) ? kEOF : NS_OK;
}

NS_IMETHODIMP
nsExpatDriver::WillBuildModel(const CParserContext& aParserContext,
                              nsITokenizer* aTokenizer,
                              nsIContentSink* aSink)
{
  mSink = do_QueryInterface(aSink);
  if (!mSink) {
    NS_ERROR("nsExpatDriver didn't get an nsIExpatSink");
    
    mInternalState = NS_ERROR_UNEXPECTED;
    return mInternalState;
  }

  mOriginalSink = aSink;

  static const XML_Memory_Handling_Suite memsuite =
    {
      (void *(*)(size_t))PR_Malloc,
      (void *(*)(void *, size_t))PR_Realloc,
      PR_Free
    };

  static const char16_t kExpatSeparator[] = { kExpatSeparatorChar, '\0' };

  mExpatParser = XML_ParserCreate_MM(kUTF16, &memsuite, kExpatSeparator);
  NS_ENSURE_TRUE(mExpatParser, NS_ERROR_FAILURE);

  XML_SetReturnNSTriplet(mExpatParser, XML_TRUE);

#ifdef XML_DTD
  XML_SetParamEntityParsing(mExpatParser, XML_PARAM_ENTITY_PARSING_ALWAYS);
#endif

  mURISpec = aParserContext.mScanner->GetFilename();

  XML_SetBase(mExpatParser, mURISpec.get());

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(mOriginalSink->GetTarget());
  if (doc) {
    nsCOMPtr<nsPIDOMWindow> win = doc->GetWindow();
    if (!win) {
      bool aHasHadScriptHandlingObject;
      nsIScriptGlobalObject *global =
        doc->GetScriptHandlingObject(aHasHadScriptHandlingObject);
      if (global) {
        win = do_QueryInterface(global);
      }
    }
    if (win && !win->IsInnerWindow()) {
      win = win->GetCurrentInnerWindow();
    }
    if (win) {
      mInnerWindowID = win->WindowID();
    }
  }

  
  XML_SetXmlDeclHandler(mExpatParser, Driver_HandleXMLDeclaration); 
  XML_SetElementHandler(mExpatParser, Driver_HandleStartElement,
                        Driver_HandleEndElement);
  XML_SetCharacterDataHandler(mExpatParser, Driver_HandleCharacterData);
  XML_SetProcessingInstructionHandler(mExpatParser,
                                      Driver_HandleProcessingInstruction);
  XML_SetDefaultHandlerExpand(mExpatParser, Driver_HandleDefault);
  XML_SetExternalEntityRefHandler(mExpatParser,
                                  (XML_ExternalEntityRefHandler)
                                          Driver_HandleExternalEntityRef);
  XML_SetExternalEntityRefHandlerArg(mExpatParser, this);
  XML_SetCommentHandler(mExpatParser, Driver_HandleComment);
  XML_SetCdataSectionHandler(mExpatParser, Driver_HandleStartCdataSection,
                             Driver_HandleEndCdataSection);

  XML_SetParamEntityParsing(mExpatParser,
                            XML_PARAM_ENTITY_PARSING_UNLESS_STANDALONE);
  XML_SetDoctypeDeclHandler(mExpatParser, Driver_HandleStartDoctypeDecl,
                            Driver_HandleEndDoctypeDecl);

  
  
  mExtendedSink = do_QueryInterface(mSink);
  if (mExtendedSink) {
    XML_SetNamespaceDeclHandler(mExpatParser,
                                Driver_HandleStartNamespaceDecl,
                                Driver_HandleEndNamespaceDecl);
    XML_SetUnparsedEntityDeclHandler(mExpatParser,
                                     Driver_HandleUnparsedEntityDecl);
    XML_SetNotationDeclHandler(mExpatParser,
                               Driver_HandleNotationDecl);
  }

  
  XML_SetUserData(mExpatParser, this);

  
  aParserContext.mScanner->OverrideReplacementCharacter(0xffff);

  return mInternalState;
}

NS_IMETHODIMP
nsExpatDriver::BuildModel(nsITokenizer* aTokenizer, nsIContentSink* aSink)
{
  return mInternalState;
}

NS_IMETHODIMP
nsExpatDriver::DidBuildModel(nsresult anErrorCode)
{
  mOriginalSink = nullptr;
  mSink = nullptr;
  mExtendedSink = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsExpatDriver::WillTokenize(bool aIsFinalChunk)
{
  mIsFinalChunk = aIsFinalChunk;
  return NS_OK;
}

NS_IMETHODIMP_(void)
nsExpatDriver::Terminate()
{
  
  if (mExpatParser) {
    XML_StopParser(mExpatParser, XML_FALSE);
  }
  mInternalState = NS_ERROR_HTMLPARSER_STOPPARSING;
}

NS_IMETHODIMP_(int32_t)
nsExpatDriver::GetType()
{
  return NS_IPARSER_FLAG_XML;
}

NS_IMETHODIMP_(nsDTDMode)
nsExpatDriver::GetMode() const
{
  return eDTDMode_full_standards;
}



NS_IMETHODIMP_(bool)
nsExpatDriver::IsContainer(int32_t aTag) const
{
  return true;
}

NS_IMETHODIMP_(bool)
nsExpatDriver::CanContain(int32_t aParent,int32_t aChild) const
{
  return true;
}

void
nsExpatDriver::MaybeStopParser(nsresult aState)
{
  if (NS_FAILED(aState)) {
    
    
    
    if (NS_SUCCEEDED(mInternalState) ||
        mInternalState == NS_ERROR_HTMLPARSER_INTERRUPTED ||
        (mInternalState == NS_ERROR_HTMLPARSER_BLOCK &&
         aState != NS_ERROR_HTMLPARSER_INTERRUPTED)) {
      mInternalState = (aState == NS_ERROR_HTMLPARSER_INTERRUPTED ||
                        aState == NS_ERROR_HTMLPARSER_BLOCK) ?
                       aState :
                       NS_ERROR_HTMLPARSER_STOPPARSING;
    }

    
    
    
    
    XML_StopParser(mExpatParser, BlockedOrInterrupted());
  }
  else if (NS_SUCCEEDED(mInternalState)) {
    
    
    mInternalState = aState;
  }
}
