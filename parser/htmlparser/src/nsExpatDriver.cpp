





































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
#include "prprf.h"
#include "prmem.h"
#include "nsTextFormatter.h"
#include "nsDirectoryServiceDefs.h"
#include "nsCRT.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsContentErrors.h"
#include "nsXPCOMCIDInternal.h"
#include "nsUnicharInputStream.h"

#define kExpatSeparatorChar 0xFFFF

static const PRUnichar kUTF16[] = { 'U', 'T', 'F', '-', '1', '6', '\0' };

#ifdef PR_LOGGING
static PRLogModuleInfo *gExpatDriverLog = PR_NewLogModule("expatdriver");
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
    driver->HandleCharacterData(aData, PRUint32(aLength));
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
    driver->HandleDefault(aData, PRUint32(aLength));
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
  { "-//W3C//DTD XHTML 1.0 Transitional//EN",    "xhtml11.dtd", nsnull },
  { "-//W3C//DTD XHTML 1.1//EN",                 "xhtml11.dtd", nsnull },
  { "-//W3C//DTD XHTML 1.0 Strict//EN",          "xhtml11.dtd", nsnull },
  { "-//W3C//DTD XHTML 1.0 Frameset//EN",        "xhtml11.dtd", nsnull },
  { "-//W3C//DTD XHTML Basic 1.0//EN",           "xhtml11.dtd", nsnull },
  { "-//W3C//DTD XHTML 1.1 plus MathML 2.0//EN", "mathml.dtd",  "resource://gre-resources/res/mathml.css" },
  { "-//W3C//DTD XHTML 1.1 plus MathML 2.0 plus SVG 1.1//EN", "mathml.dtd", "resource://gre-resources/mathml.css" },
  { "-//W3C//DTD MathML 2.0//EN",                "mathml.dtd",  "resource://gre-resources/mathml.css" },
  { "-//WAPFORUM//DTD XHTML Mobile 1.0//EN",     "xhtml11.dtd", nsnull },
  { nsnull, nsnull, nsnull }
};

static const nsCatalogData*
LookupCatalogData(const PRUnichar* aPublicID)
{
  nsDependentString publicID(aPublicID);

  
  
  
  const nsCatalogData* data = kCatalogTable;
  while (data->mPublicID) {
    if (publicID.EqualsASCII(data->mPublicID)) {
      return data;
    }
    ++data;
  }

  return nsnull;
}







static PRBool
IsLoadableDTD(const nsCatalogData* aCatalogData, nsIURI* aDTD,
              nsIURI** aResult)
{
  NS_ASSERTION(aDTD, "Null parameter.");

  nsCAutoString fileName;
  if (aCatalogData) {
    
    fileName.Assign(aCatalogData->mLocalDTD);
  }

  if (fileName.IsEmpty()) {
    
    
    
    
    nsCOMPtr<nsIURL> dtdURL = do_QueryInterface(aDTD);
    if (!dtdURL) {
      return PR_FALSE;
    }

    dtdURL->GetFileName(fileName);
    if (fileName.IsEmpty()) {
      return PR_FALSE;
    }
  }

  nsCOMPtr<nsIFile> dtdPath;
  NS_GetSpecialDirectory(NS_GRE_DIR, getter_AddRefs(dtdPath));
  if (!dtdPath) {
    return PR_FALSE;
  }

  nsCOMPtr<nsILocalFile> lfile = do_QueryInterface(dtdPath);

  
  
  
  lfile->AppendNative(NS_LITERAL_CSTRING("res"));
  lfile->AppendNative(NS_LITERAL_CSTRING("dtd"));
  lfile->AppendNative(fileName);

  PRBool exists;
  dtdPath->Exists(&exists);
  if (!exists) {
    return PR_FALSE;
  }

  
  
  NS_NewFileURI(aResult, dtdPath);

  return *aResult != nsnull;
}



NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsExpatDriver)
  NS_INTERFACE_MAP_ENTRY(nsITokenizer)
  NS_INTERFACE_MAP_ENTRY(nsIDTD)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDTD)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsExpatDriver)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsExpatDriver)

NS_IMPL_CYCLE_COLLECTION_2(nsExpatDriver, mSink, mExtendedSink)

nsExpatDriver::nsExpatDriver()
  : mExpatParser(nsnull),
    mInCData(PR_FALSE),
    mInInternalSubset(PR_FALSE),
    mInExternalDTD(PR_FALSE),
    mMadeFinalCallToExpat(PR_FALSE),
    mIsFinalChunk(PR_FALSE),
    mInternalState(NS_OK),
    mExpatBuffered(0),
    mCatalogData(nsnull)
{
}

nsExpatDriver::~nsExpatDriver()
{
  if (mExpatParser) {
    XML_ParserFree(mExpatParser);
  }
}

nsresult
nsExpatDriver::HandleStartElement(const PRUnichar *aValue,
                                  const PRUnichar **aAtts)
{
  NS_ASSERTION(mSink, "content sink not found!");

  
  
  
  
  PRUint32 attrArrayLength;
  for (attrArrayLength = XML_GetSpecifiedAttributeCount(mExpatParser);
       aAtts[attrArrayLength];
       attrArrayLength += 2) {
    
  }

  if (mSink) {
    nsresult rv = mSink->
      HandleStartElement(aValue, aAtts, attrArrayLength,
                         XML_GetIdAttributeIndex(mExpatParser),
                         XML_GetCurrentLineNumber(mExpatParser));
    MaybeStopParser(rv);
  }

  return NS_OK;
}

nsresult
nsExpatDriver::HandleEndElement(const PRUnichar *aValue)
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
nsExpatDriver::HandleCharacterData(const PRUnichar *aValue,
                                   const PRUint32 aLength)
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
nsExpatDriver::HandleComment(const PRUnichar *aValue)
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
nsExpatDriver::HandleProcessingInstruction(const PRUnichar *aTarget,
                                           const PRUnichar *aData)
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
nsExpatDriver::HandleXMLDeclaration(const PRUnichar *aVersion,
                                    const PRUnichar *aEncoding,
                                    PRInt32 aStandalone)
{
  if (mSink) {
    nsresult rv = mSink->HandleXMLDeclaration(aVersion, aEncoding, aStandalone);
    MaybeStopParser(rv);
  }

  return NS_OK;
}

nsresult
nsExpatDriver::HandleDefault(const PRUnichar *aValue,
                             const PRUint32 aLength)
{
  NS_ASSERTION(mSink, "content sink not found!");

  if (mInExternalDTD) {
    
    return NS_OK;
  }

  if (mInInternalSubset) {
    mInternalSubset.Append(aValue, aLength);
  }
  else if (mSink) {
    PRUint32 i;
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
  mInCData = PR_TRUE;

  return NS_OK;
}

nsresult
nsExpatDriver::HandleEndCdataSection()
{
  NS_ASSERTION(mSink, "content sink not found!");

  mInCData = PR_FALSE;
  if (mSink) {
    nsresult rv = mSink->HandleCDataSection(mCDataText.get(),
                                            mCDataText.Length());
    MaybeStopParser(rv);
  }
  mCDataText.Truncate();

  return NS_OK;
}

nsresult
nsExpatDriver::HandleStartNamespaceDecl(const PRUnichar* aPrefix,
                                        const PRUnichar* aUri)
{
  if (mExtendedSink) {
    nsresult rv = mExtendedSink->HandleStartNamespaceDecl(aPrefix, aUri);
    MaybeStopParser(rv);
  }
  return NS_OK;
}

nsresult
nsExpatDriver::HandleEndNamespaceDecl(const PRUnichar* aPrefix)
{
  if (mExtendedSink && mInternalState != NS_ERROR_HTMLPARSER_STOPPARSING) {
    nsresult rv = mExtendedSink->HandleEndNamespaceDecl(aPrefix);
    MaybeStopParser(rv);
  }
  return NS_OK;
}

nsresult
nsExpatDriver::HandleNotationDecl(const PRUnichar* aNotationName,
                                  const PRUnichar* aBase,
                                  const PRUnichar* aSysid,
                                  const PRUnichar* aPubid)
{
  if (mExtendedSink) {
    nsresult rv = mExtendedSink->HandleNotationDecl(aNotationName, aSysid,
                                                    aPubid);
    MaybeStopParser(rv);
  }
  return NS_OK;
}

nsresult
nsExpatDriver::HandleUnparsedEntityDecl(const PRUnichar* aEntityName,
                                        const PRUnichar* aBase,
                                        const PRUnichar* aSysid,
                                        const PRUnichar* aPubid,
                                        const PRUnichar* aNotationName)
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
nsExpatDriver::HandleStartDoctypeDecl(const PRUnichar* aDoctypeName,
                                      const PRUnichar* aSysid,
                                      const PRUnichar* aPubid,
                                      PRBool aHasInternalSubset)
{
  mDoctypeName = aDoctypeName;
  mSystemID = aSysid;
  mPublicID = aPubid;

  if (mExtendedSink) {
    nsresult rv = mExtendedSink->HandleStartDTD(aDoctypeName, aSysid, aPubid);
    MaybeStopParser(rv);
  }

  if (aHasInternalSubset) {
    
    
    
    mInInternalSubset = PR_TRUE;
    mInternalSubset.SetCapacity(1024);
  } else {
    
    mInternalSubset.SetIsVoid(PR_TRUE);
  }

  return NS_OK;
}

nsresult
nsExpatDriver::HandleEndDoctypeDecl()
{
  NS_ASSERTION(mSink, "content sink not found!");

  mInInternalSubset = PR_FALSE;

  if (mSink) {
    
    
    
    nsCOMPtr<nsIURI> data;
    if (mCatalogData && mCatalogData->mAgentSheet) {
      NS_NewURI(getter_AddRefs(data), mCatalogData->mAgentSheet);
    }

    
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
                            const PRUnichar* aFromSegment,
                            PRUint32 aToOffset,
                            PRUint32 aCount,
                            PRUint32 *aWriteCount)
{
  
  if (XML_Parse((XML_Parser)aClosure, (const char *)aFromSegment,
                aCount * sizeof(PRUnichar), 0) == XML_STATUS_OK) {
    *aWriteCount = aCount;

    return NS_OK;
  }

  *aWriteCount = 0;

  return NS_ERROR_FAILURE;
}

int
nsExpatDriver::HandleExternalEntityRef(const PRUnichar *openEntityNames,
                                       const PRUnichar *base,
                                       const PRUnichar *systemId,
                                       const PRUnichar *publicId)
{
  if (mInInternalSubset && !mInExternalDTD && openEntityNames) {
    mInternalSubset.Append(PRUnichar('%'));
    mInternalSubset.Append(nsDependentString(openEntityNames));
    mInternalSubset.Append(PRUnichar(';'));
  }

  
  nsCOMPtr<nsIInputStream> in;
  nsAutoString absURL;
  nsresult rv = OpenInputStreamFromExternalDTD(publicId, systemId, base,
                                               getter_AddRefs(in), absURL);
  NS_ENSURE_SUCCESS(rv, 1);

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

      mInExternalDTD = PR_TRUE;

      PRUint32 totalRead;
      do {
        rv = uniIn->ReadSegments(ExternalDTDStreamReaderFunc, entParser,
                                 PRUint32(-1), &totalRead);
      } while (NS_SUCCEEDED(rv) && totalRead > 0);

      result = XML_Parse(entParser, nsnull, 0, 1);

      mInExternalDTD = PR_FALSE;

      XML_ParserFree(entParser);
    }
  }

  return result;
}

nsresult
nsExpatDriver::OpenInputStreamFromExternalDTD(const PRUnichar* aFPIStr,
                                              const PRUnichar* aURLStr,
                                              const PRUnichar* aBaseURL,
                                              nsIInputStream** aStream,
                                              nsAString& aAbsURL)
{
  nsCOMPtr<nsIURI> baseURI;
  nsresult rv = NS_NewURI(getter_AddRefs(baseURI),
                          NS_ConvertUTF16toUTF8(aBaseURL));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), NS_ConvertUTF16toUTF8(aURLStr), nsnull,
                 baseURI);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRBool isChrome = PR_FALSE;
  uri->SchemeIs("chrome", &isChrome);
  if (!isChrome) {
    
    
    
    if (aFPIStr) {
      
      mCatalogData = LookupCatalogData(aFPIStr);
    }

    nsCOMPtr<nsIURI> localURI;
    if (!IsLoadableDTD(mCatalogData, uri, getter_AddRefs(localURI))) {
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
  PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_DTD,
                                uri,
                                (doc ? doc->NodePrincipal() : nsnull),
                                doc,
                                EmptyCString(), 
                                nsnull,         
                                &shouldLoad);
  if (NS_FAILED(rv)) return rv;
  if (NS_CP_REJECTED(shouldLoad)) {
    
    return NS_ERROR_CONTENT_BLOCKED;
  }

  rv = NS_OpenURI(aStream, uri);

  nsCAutoString absURL;
  uri->GetSpec(absURL);

  CopyUTF8toUTF16(absURL, aAbsURL);

  return rv;
}

static nsresult
CreateErrorText(const PRUnichar* aDescription,
                const PRUnichar* aSourceURL,
                const PRUint32 aLineNumber,
                const PRUint32 aColNumber,
                nsString& aErrorString)
{
  aErrorString.Truncate();

  nsAutoString msg;
  nsresult rv =
    nsParserMsgUtils::GetLocalizedStringByName(XMLPARSER_PROPERTIES,
                                               "XMLParsingError", msg);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRUnichar *message = nsTextFormatter::smprintf(msg.get(), aDescription,
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
AppendErrorPointer(const PRInt32 aColNumber,
                   const PRUnichar *aSourceLine,
                   nsString& aSourceString)
{
  aSourceString.Append(PRUnichar('\n'));

  
  PRInt32 last = aColNumber - 1;
  PRInt32 i;
  PRUint32 minuses = 0;
  for (i = 0; i < last; ++i) {
    if (aSourceLine[i] == '\t') {
      
      PRUint32 add = 8 - (minuses % 8);
      aSourceString.AppendASCII("--------", add);
      minuses += add;
    }
    else {
      aSourceString.Append(PRUnichar('-'));
      ++minuses;
    }
  }
  aSourceString.Append(PRUnichar('^'));

  return NS_OK;
}

nsresult
nsExpatDriver::HandleError()
{
  PRInt32 code = XML_GetErrorCode(mExpatParser);
  NS_ASSERTION(code > XML_ERROR_NONE, "unexpected XML error code");

  
  
  nsAutoString description;
  nsParserMsgUtils::GetLocalizedStringByID(XMLPARSER_PROPERTIES, code,
                                           description);

  if (code == XML_ERROR_TAG_MISMATCH) {
    








    const PRUnichar *mismatch = MOZ_XML_GetMismatchedTag(mExpatParser);
    const PRUnichar *uriEnd = nsnull;
    const PRUnichar *nameEnd = nsnull;
    const PRUnichar *pos;
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
      tagName.Append(PRUnichar(':'));
    }
    const PRUnichar *nameStart = uriEnd ? uriEnd + 1 : mismatch;
    tagName.Append(nameStart, (nameEnd ? nameEnd : pos) - nameStart);
    
    nsAutoString msg;
    nsParserMsgUtils::GetLocalizedStringByName(XMLPARSER_PROPERTIES,
                                               "Expected", msg);

    
    PRUnichar *message = nsTextFormatter::smprintf(msg.get(), tagName.get());
    if (!message) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    description.Append(message);

    nsTextFormatter::smprintf_free(message);
  }

  
  PRUint32 colNumber = XML_GetCurrentColumnNumber(mExpatParser) + 1;
  PRUint32 lineNumber = XML_GetCurrentLineNumber(mExpatParser);

  nsAutoString errorText;
  CreateErrorText(description.get(), XML_GetBase(mExpatParser), lineNumber,
                  colNumber, errorText);

  NS_ASSERTION(mSink, "no sink?");

  nsAutoString sourceText(mLastLine);
  AppendErrorPointer(colNumber, mLastLine.get(), sourceText);

  
  nsCOMPtr<nsIScriptError> serr(do_CreateInstance(NS_SCRIPTERROR_CONTRACTID));
  nsresult rv = NS_ERROR_FAILURE;
  if (serr) {
    rv = serr->Init(description.get(),
                    mURISpec.get(),
                    mLastLine.get(),
                    lineNumber, colNumber,
                    nsIScriptError::errorFlag, "malformed-xml");
  }

  
  PRBool shouldReportError = NS_SUCCEEDED(rv);

  if (mSink && shouldReportError) {
    rv = mSink->ReportError(errorText.get(), 
                            sourceText.get(), 
                            serr, 
                            &shouldReportError);
    if (NS_FAILED(rv)) {
      shouldReportError = PR_TRUE;
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
nsExpatDriver::ParseBuffer(const PRUnichar *aBuffer,
                           PRUint32 aLength,
                           PRBool aIsFinal,
                           PRUint32 *aConsumed)
{
  NS_ASSERTION((aBuffer && aLength != 0) || (!aBuffer && aLength == 0), "?");
  NS_ASSERTION(mInternalState != NS_OK || aIsFinal || aBuffer,
               "Useless call, we won't call Expat");
  NS_PRECONDITION(!BlockedOrInterrupted() || !aBuffer,
                  "Non-null buffer when resuming");
  NS_PRECONDITION(XML_GetCurrentByteIndex(mExpatParser) % sizeof(PRUnichar) == 0,
                  "Consumed part of a PRUnichar?");

  if (mExpatParser && (mInternalState == NS_OK || BlockedOrInterrupted())) {
    PRInt32 parserBytesBefore = XML_GetCurrentByteIndex(mExpatParser);
    NS_ASSERTION(parserBytesBefore >= 0, "Unexpected value");

    XML_Status status;
    if (BlockedOrInterrupted()) {
      mInternalState = NS_OK; 
      status = XML_ResumeParser(mExpatParser);
    }
    else {
      status = XML_Parse(mExpatParser,
                         reinterpret_cast<const char*>(aBuffer),
                         aLength * sizeof(PRUnichar), aIsFinal);
    }

    PRInt32 parserBytesConsumed = XML_GetCurrentByteIndex(mExpatParser);

    NS_ASSERTION(parserBytesConsumed >= 0, "Unexpected value");
    NS_ASSERTION(parserBytesConsumed >= parserBytesBefore,
                 "How'd this happen?");
    NS_ASSERTION(parserBytesConsumed % sizeof(PRUnichar) == 0,
                 "Consumed part of a PRUnichar?");

    
    *aConsumed = (parserBytesConsumed - parserBytesBefore) / sizeof(PRUnichar);
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
nsExpatDriver::ConsumeToken(nsScanner& aScanner, PRBool& aFlushTokens)
{
  
  
  nsScannerIterator currentExpatPosition;
  aScanner.CurrentPosition(currentExpatPosition);

  
  nsScannerIterator start = currentExpatPosition;
  start.advance(mExpatBuffered);

  
  
  nsScannerIterator end;
  aScanner.EndReading(end);

  PR_LOG(gExpatDriverLog, PR_LOG_DEBUG,
         ("Remaining in expat's buffer: %i, remaining in scanner: %i.",
          mExpatBuffered, Distance(start, end)));

  
  
  
  while (start != end || (mIsFinalChunk && !mMadeFinalCallToExpat) ||
         (BlockedOrInterrupted() && mExpatBuffered > 0)) {
    PRBool noMoreBuffers = start == end && mIsFinalChunk;
    PRBool blocked = BlockedOrInterrupted();

    const PRUnichar *buffer;
    PRUint32 length;
    if (blocked || noMoreBuffers) {
      
      
      buffer = nsnull;
      length = 0;

#if defined(PR_LOGGING) || defined (DEBUG)
      if (blocked) {
        PR_LOG(gExpatDriverLog, PR_LOG_DEBUG,
               ("Resuming Expat, will parse data remaining in Expat's "
                "buffer.\nContent of Expat's buffer:\n-----\n%s\n-----\n",
                NS_ConvertUTF16toUTF8(currentExpatPosition.get(),
                                      mExpatBuffered).get()));
      }
      else {
        NS_ASSERTION(mExpatBuffered == Distance(currentExpatPosition, end),
                     "Didn't pass all the data to Expat?");
        PR_LOG(gExpatDriverLog, PR_LOG_DEBUG,
               ("Last call to Expat, will parse data remaining in Expat's "
                "buffer.\nContent of Expat's buffer:\n-----\n%s\n-----\n",
                NS_ConvertUTF16toUTF8(currentExpatPosition.get(),
                                      mExpatBuffered).get()));
      }
#endif
    }
    else {
      buffer = start.get();
      length = PRUint32(start.size_forward());

      PR_LOG(gExpatDriverLog, PR_LOG_DEBUG,
             ("Calling Expat, will parse data remaining in Expat's buffer and "
              "new data.\nContent of Expat's buffer:\n-----\n%s\n-----\nNew "
              "data:\n-----\n%s\n-----\n",
              NS_ConvertUTF16toUTF8(currentExpatPosition.get(),
                                    mExpatBuffered).get(),
              NS_ConvertUTF16toUTF8(start.get(), length).get()));
    }

    PRUint32 consumed;
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
      PR_LOG(gExpatDriverLog, PR_LOG_DEBUG,
             ("Blocked or interrupted parser (probably for loading linked "
              "stylesheets or scripts)."));

      aScanner.SetPosition(currentExpatPosition, PR_TRUE);
      aScanner.Mark();

      return mInternalState;
    }

    if (noMoreBuffers && mExpatBuffered == 0) {
      mMadeFinalCallToExpat = PR_TRUE;
    }

    if (NS_FAILED(mInternalState)) {
      if (XML_GetErrorCode(mExpatParser) != XML_ERROR_NONE) {
        NS_ASSERTION(mInternalState == NS_ERROR_HTMLPARSER_STOPPARSING,
                     "Unexpected error");

        
        nsScannerIterator lastLine = currentExpatPosition;
        while (lastLine != end) {
          length = PRUint32(lastLine.size_forward());
          PRUint32 endOffset = 0;
          const PRUnichar *buffer = lastLine.get();
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

  aScanner.SetPosition(currentExpatPosition, PR_TRUE);
  aScanner.Mark();

  PR_LOG(gExpatDriverLog, PR_LOG_DEBUG,
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

  static const PRUnichar kExpatSeparator[] = { kExpatSeparatorChar, '\0' };

  mExpatParser = XML_ParserCreate_MM(kUTF16, &memsuite, kExpatSeparator);
  NS_ENSURE_TRUE(mExpatParser, NS_ERROR_FAILURE);

  XML_SetReturnNSTriplet(mExpatParser, XML_TRUE);

#ifdef XML_DTD
  XML_SetParamEntityParsing(mExpatParser, XML_PARAM_ENTITY_PARSING_ALWAYS);
#endif

  mURISpec = aParserContext.mScanner->GetFilename();

  XML_SetBase(mExpatParser, mURISpec.get());

  
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
nsExpatDriver::BuildModel(nsITokenizer* aTokenizer,
                          PRBool,
                          PRBool,
                          const nsCString*)
{
  return mInternalState;
}

NS_IMETHODIMP
nsExpatDriver::DidBuildModel(nsresult anErrorCode)
{
  mOriginalSink = nsnull;
  mSink = nsnull;
  mExtendedSink = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsExpatDriver::WillTokenize(PRBool aIsFinalChunk,
                            nsTokenAllocator* aTokenAllocator)
{
  mIsFinalChunk = aIsFinalChunk;
  return NS_OK;
}

NS_IMETHODIMP
nsExpatDriver::DidTokenize(PRBool aIsFinalChunk)
{
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

NS_IMETHODIMP_(PRInt32)
nsExpatDriver::GetType()
{
  return NS_IPARSER_FLAG_XML;
}

NS_IMETHODIMP_(nsDTDMode)
nsExpatDriver::GetMode() const
{
  return eDTDMode_full_standards;
}



NS_IMETHODIMP_(CToken*)
nsExpatDriver::PushTokenFront(CToken* aToken)
{
  return 0;
}

NS_IMETHODIMP_(CToken*)
nsExpatDriver::PushToken(CToken* aToken)
{
  return 0;
}

NS_IMETHODIMP_(CToken*)
nsExpatDriver::PopToken(void)
{
  return 0;
}

NS_IMETHODIMP_(CToken*)
nsExpatDriver::PeekToken(void)
{
  return 0;
}

NS_IMETHODIMP_(CToken*)
nsExpatDriver::GetTokenAt(PRInt32 anIndex)
{
  return 0;
}

NS_IMETHODIMP_(PRInt32)
nsExpatDriver::GetCount(void)
{
  return 0;
}

NS_IMETHODIMP_(nsTokenAllocator*)
nsExpatDriver::GetTokenAllocator(void)
{
  return 0;
}

NS_IMETHODIMP_(void)
nsExpatDriver::PrependTokens(nsDeque& aDeque)
{
}

NS_IMETHODIMP
nsExpatDriver::CopyState(nsITokenizer* aTokenizer)
{
  return NS_OK;
}

nsresult
nsExpatDriver::HandleToken(CToken* aToken)
{
  return NS_OK;
}

NS_IMETHODIMP_(PRBool)
nsExpatDriver::IsContainer(PRInt32 aTag) const
{
  return PR_TRUE;
}

NS_IMETHODIMP_(PRBool)
nsExpatDriver::CanContain(PRInt32 aParent,PRInt32 aChild) const
{
  return PR_TRUE;
}

void
nsExpatDriver::MaybeStopParser(nsresult aState)
{
  if (NS_FAILED(aState)) {
    
    
    
    if (NS_SUCCEEDED(mInternalState) ||
        mInternalState == NS_ERROR_HTMLPARSER_INTERRUPTED ||
        (mInternalState == NS_ERROR_HTMLPARSER_BLOCK &&
         aState != NS_ERROR_HTMLPARSER_INTERRUPTED)) {
      mInternalState = aState;
    }

    
    
    
    
    XML_StopParser(mExpatParser, BlockedOrInterrupted());
  }
  else if (NS_SUCCEEDED(mInternalState)) {
    
    
    mInternalState = aState;
  }
}
