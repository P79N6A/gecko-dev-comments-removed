








































































#include "nsCOMPtr.h"
#include "nsInterfaceHashtable.h"
#include "nsIContentSink.h"
#include "nsIRDFContainer.h"
#include "nsIRDFContainerUtils.h"
#include "nsIRDFContentSink.h"
#include "nsIRDFNode.h"
#include "nsIRDFService.h"
#include "nsIRDFXMLSink.h"
#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsIXMLContentSink.h"
#include "nsRDFCID.h"
#include "nsTArray.h"
#include "nsXPIDLString.h"
#include "prlog.h"
#include "prmem.h"
#include "rdf.h"
#include "rdfutil.h"
#include "nsReadableUtils.h"
#include "nsIExpatSink.h"
#include "nsCRT.h"
#include "nsIAtom.h"
#include "nsStaticAtom.h"
#include "nsIScriptError.h"
#include "nsIDTD.h"




static NS_DEFINE_IID(kIContentSinkIID,         NS_ICONTENT_SINK_IID); 
static NS_DEFINE_IID(kIExpatSinkIID,           NS_IEXPATSINK_IID);
static NS_DEFINE_IID(kIRDFServiceIID,          NS_IRDFSERVICE_IID);
static NS_DEFINE_IID(kISupportsIID,            NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIXMLContentSinkIID,      NS_IXMLCONTENT_SINK_IID);
static NS_DEFINE_IID(kIRDFContentSinkIID,      NS_IRDFCONTENTSINK_IID);

static NS_DEFINE_CID(kRDFServiceCID,            NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFContainerUtilsCID,     NS_RDFCONTAINERUTILS_CID);



#ifdef PR_LOGGING
static PRLogModuleInfo* gLog;
#endif



enum RDFContentSinkState {
    eRDFContentSinkState_InProlog,
    eRDFContentSinkState_InDocumentElement,
    eRDFContentSinkState_InDescriptionElement,
    eRDFContentSinkState_InContainerElement,
    eRDFContentSinkState_InPropertyElement,
    eRDFContentSinkState_InMemberElement,
    eRDFContentSinkState_InEpilog
};

enum RDFContentSinkParseMode {
    eRDFContentSinkParseMode_Resource,
    eRDFContentSinkParseMode_Literal,
    eRDFContentSinkParseMode_Int,
    eRDFContentSinkParseMode_Date
};

typedef
NS_STDCALL_FUNCPROTO(nsresult,
                     nsContainerTestFn,
                     nsIRDFContainerUtils, IsAlt,
                     (nsIRDFDataSource*, nsIRDFResource*, PRBool*));

typedef
NS_STDCALL_FUNCPROTO(nsresult,
                     nsMakeContainerFn,
                     nsIRDFContainerUtils, MakeAlt,
                     (nsIRDFDataSource*, nsIRDFResource*, nsIRDFContainer**));

class RDFContentSinkImpl : public nsIRDFContentSink,
                           public nsIExpatSink
{
public:
    RDFContentSinkImpl();
    virtual ~RDFContentSinkImpl();

    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIEXPATSINK

    
    NS_IMETHOD WillParse(void);
    NS_IMETHOD WillBuildModel(nsDTDMode aDTDMode);
    NS_IMETHOD DidBuildModel(PRBool aTerminated);
    NS_IMETHOD WillInterrupt(void);
    NS_IMETHOD WillResume(void);
    NS_IMETHOD SetParser(nsIParser* aParser);  
    virtual void FlushPendingNotifications(mozFlushType aType) { }
    NS_IMETHOD SetDocumentCharset(nsACString& aCharset) { return NS_OK; }
    virtual nsISupports *GetTarget() { return nsnull; }

    
    NS_IMETHOD Init(nsIURI* aURL);
    NS_IMETHOD SetDataSource(nsIRDFDataSource* aDataSource);
    NS_IMETHOD GetDataSource(nsIRDFDataSource*& aDataSource);

    
    static PRInt32 gRefCnt;
    static nsIRDFService* gRDFService;
    static nsIRDFContainerUtils* gRDFContainerUtils;
    static nsIRDFResource* kRDF_type;
    static nsIRDFResource* kRDF_instanceOf; 
    static nsIRDFResource* kRDF_Alt;
    static nsIRDFResource* kRDF_Bag;
    static nsIRDFResource* kRDF_Seq;
    static nsIRDFResource* kRDF_nextVal;

    static nsIAtom* kAboutAtom;
    static nsIAtom* kIdAtom;
    static nsIAtom* kNodeIdAtom;
    static nsIAtom* kAboutEachAtom;
    static nsIAtom* kResourceAtom;
    static nsIAtom* kRDFAtom;
    static nsIAtom* kDescriptionAtom;
    static nsIAtom* kBagAtom;
    static nsIAtom* kSeqAtom;
    static nsIAtom* kAltAtom;
    static nsIAtom* kLiAtom;
    static nsIAtom* kXMLNSAtom;
    static nsIAtom* kParseTypeAtom;


    typedef struct ContainerInfo {
        nsIRDFResource**  mType;
        nsContainerTestFn mTestFn;
        nsMakeContainerFn mMakeFn;
    } ContainerInfo;

protected:
    
    void ParseText(nsIRDFNode **aResult);

    nsresult FlushText();
    nsresult AddText(const PRUnichar* aText, PRInt32 aLength);

    
    nsresult OpenRDF(const PRUnichar* aName);
    nsresult OpenObject(const PRUnichar* aName ,const PRUnichar** aAttributes);
    nsresult OpenProperty(const PRUnichar* aName, const PRUnichar** aAttributes);
    nsresult OpenMember(const PRUnichar* aName, const PRUnichar** aAttributes);
    nsresult OpenValue(const PRUnichar* aName, const PRUnichar** aAttributes);
    
    nsresult GetIdAboutAttribute(const PRUnichar** aAttributes, nsIRDFResource** aResource, PRBool* aIsAnonymous = nsnull);
    nsresult GetResourceAttribute(const PRUnichar** aAttributes, nsIRDFResource** aResource);
    nsresult AddProperties(const PRUnichar** aAttributes, nsIRDFResource* aSubject, PRInt32* aCount = nsnull);
    void SetParseMode(const PRUnichar **aAttributes);

    PRUnichar* mText;
    PRInt32 mTextLength;
    PRInt32 mTextSize;

    






    void RegisterNamespaces(const PRUnichar **aAttributes);

    





    const nsDependentSubstring SplitExpatName(const PRUnichar *aExpatName,
                                              nsIAtom **aLocalName);

    enum eContainerType { eBag, eSeq, eAlt };
    nsresult InitContainer(nsIRDFResource* aContainerType, nsIRDFResource* aContainer);
    nsresult ReinitContainer(nsIRDFResource* aContainerType, nsIRDFResource* aContainer);

    
    nsCOMPtr<nsIRDFDataSource> mDataSource;

    
    nsInterfaceHashtable<nsStringHashKey, nsIRDFResource> mNodeIDMap;

    
    RDFContentSinkState mState;
    RDFContentSinkParseMode mParseMode;

    
    PRInt32         
    PushContext(nsIRDFResource *aContext,
                RDFContentSinkState aState,
                RDFContentSinkParseMode aParseMode);

    nsresult
    PopContext(nsIRDFResource         *&aContext,
               RDFContentSinkState     &aState,
               RDFContentSinkParseMode &aParseMode);

    nsIRDFResource* GetContextElement(PRInt32 ancestor = 0);


    struct RDFContextStackElement {
        nsCOMPtr<nsIRDFResource> mResource;
        RDFContentSinkState      mState;
        RDFContentSinkParseMode  mParseMode;
    };

    nsAutoTArray<RDFContextStackElement, 8>* mContextStack;

    nsIURI*      mDocumentURL;
};

PRInt32         RDFContentSinkImpl::gRefCnt = 0;
nsIRDFService*  RDFContentSinkImpl::gRDFService;
nsIRDFContainerUtils* RDFContentSinkImpl::gRDFContainerUtils;
nsIRDFResource* RDFContentSinkImpl::kRDF_type;
nsIRDFResource* RDFContentSinkImpl::kRDF_instanceOf;
nsIRDFResource* RDFContentSinkImpl::kRDF_Alt;
nsIRDFResource* RDFContentSinkImpl::kRDF_Bag;
nsIRDFResource* RDFContentSinkImpl::kRDF_Seq;
nsIRDFResource* RDFContentSinkImpl::kRDF_nextVal;

nsIAtom* RDFContentSinkImpl::kAboutAtom;
nsIAtom* RDFContentSinkImpl::kIdAtom;
nsIAtom* RDFContentSinkImpl::kNodeIdAtom;
nsIAtom* RDFContentSinkImpl::kAboutEachAtom;
nsIAtom* RDFContentSinkImpl::kResourceAtom;
nsIAtom* RDFContentSinkImpl::kRDFAtom;
nsIAtom* RDFContentSinkImpl::kDescriptionAtom;
nsIAtom* RDFContentSinkImpl::kBagAtom;
nsIAtom* RDFContentSinkImpl::kSeqAtom;
nsIAtom* RDFContentSinkImpl::kAltAtom;
nsIAtom* RDFContentSinkImpl::kLiAtom;
nsIAtom* RDFContentSinkImpl::kXMLNSAtom;
nsIAtom* RDFContentSinkImpl::kParseTypeAtom;


static const nsStaticAtom rdf_atoms[] = {
    { "about", &RDFContentSinkImpl::kAboutAtom },
    { "ID", &RDFContentSinkImpl::kIdAtom },
    { "nodeID", &RDFContentSinkImpl::kNodeIdAtom },
    { "aboutEach", &RDFContentSinkImpl::kAboutEachAtom },
    { "resource", &RDFContentSinkImpl::kResourceAtom },
    { "RDF", &RDFContentSinkImpl::kRDFAtom },
    { "Description", &RDFContentSinkImpl::kDescriptionAtom },
    { "Bag", &RDFContentSinkImpl::kBagAtom },
    { "Seq", &RDFContentSinkImpl::kSeqAtom },
    { "Alt", &RDFContentSinkImpl::kAltAtom },
    { "li", &RDFContentSinkImpl::kLiAtom },
    { "xmlns", &RDFContentSinkImpl::kXMLNSAtom },
    { "parseType", &RDFContentSinkImpl::kParseTypeAtom },
};

RDFContentSinkImpl::RDFContentSinkImpl()
    : mText(nsnull),
      mTextLength(0),
      mTextSize(0),
      mState(eRDFContentSinkState_InProlog),
      mParseMode(eRDFContentSinkParseMode_Literal),
      mContextStack(nsnull),
      mDocumentURL(nsnull)
{
    if (gRefCnt++ == 0) {
        nsresult rv = CallGetService(kRDFServiceCID, &gRDFService);

        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF service");
        if (NS_SUCCEEDED(rv)) {
            rv = gRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "type"),
                                          &kRDF_type);
            rv = gRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "instanceOf"),
                                          &kRDF_instanceOf);
            rv = gRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "Alt"),
                                          &kRDF_Alt);
            rv = gRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "Bag"),
                                          &kRDF_Bag);
            rv = gRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "Seq"),
                                          &kRDF_Seq);
            rv = gRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "nextVal"),
                                          &kRDF_nextVal);
        }


        rv = CallGetService(kRDFContainerUtilsCID, &gRDFContainerUtils);

        NS_RegisterStaticAtoms(rdf_atoms, NS_ARRAY_LENGTH(rdf_atoms));
    }

    mNodeIDMap.Init();

#ifdef PR_LOGGING
    if (! gLog)
        gLog = PR_NewLogModule("nsRDFContentSink");
#endif
}


RDFContentSinkImpl::~RDFContentSinkImpl()
{
#ifdef DEBUG_REFS
    --gInstanceCount;
    fprintf(stdout, "%d - RDF: RDFContentSinkImpl\n", gInstanceCount);
#endif

    NS_IF_RELEASE(mDocumentURL);

    if (mContextStack) {
        PR_LOG(gLog, PR_LOG_WARNING,
               ("rdfxml: warning! unclosed tag"));

        
        
        
        PRInt32 i = mContextStack->Length();
        while (0 < i--) {
            nsIRDFResource* resource;
            RDFContentSinkState state;
            RDFContentSinkParseMode parseMode;
            PopContext(resource, state, parseMode);

#ifdef PR_LOGGING
            
            
            
            if (resource) {
                nsXPIDLCString uri;
                resource->GetValue(getter_Copies(uri));
                PR_LOG(gLog, PR_LOG_NOTICE,
                       ("rdfxml:   uri=%s", (const char*) uri));
            }
#endif

            NS_IF_RELEASE(resource);
        }

        delete mContextStack;
    }
    PR_FREEIF(mText);


    if (--gRefCnt == 0) {
        NS_IF_RELEASE(gRDFService);
        NS_IF_RELEASE(gRDFContainerUtils);
        NS_IF_RELEASE(kRDF_type);
        NS_IF_RELEASE(kRDF_instanceOf);
        NS_IF_RELEASE(kRDF_Alt);
        NS_IF_RELEASE(kRDF_Bag);
        NS_IF_RELEASE(kRDF_Seq);
        NS_IF_RELEASE(kRDF_nextVal);
    }
}




NS_IMPL_ADDREF(RDFContentSinkImpl)
NS_IMPL_RELEASE(RDFContentSinkImpl)

NS_IMETHODIMP
RDFContentSinkImpl::QueryInterface(REFNSIID iid, void** result)
{
    NS_PRECONDITION(result, "null ptr");
    if (! result)
        return NS_ERROR_NULL_POINTER;

    *result = nsnull;
    if (iid.Equals(kIRDFContentSinkIID) ||
        iid.Equals(kIXMLContentSinkIID) ||
        iid.Equals(kIContentSinkIID) ||
        iid.Equals(kISupportsIID)) {
        *result = static_cast<nsIXMLContentSink*>(this);
        AddRef();
        return NS_OK;
    }
    else if (iid.Equals(kIExpatSinkIID)) {
      *result = static_cast<nsIExpatSink*>(this);
       AddRef();
       return NS_OK;
    }
    return NS_NOINTERFACE;
}

NS_IMETHODIMP 
RDFContentSinkImpl::HandleStartElement(const PRUnichar *aName, 
                                       const PRUnichar **aAtts, 
                                       PRUint32 aAttsCount, 
                                       PRInt32 aIndex, 
                                       PRUint32 aLineNumber)
{
  FlushText();

  nsresult rv = NS_ERROR_UNEXPECTED; 

  RegisterNamespaces(aAtts);

  switch (mState) {
  case eRDFContentSinkState_InProlog:
      rv = OpenRDF(aName);
      break;

  case eRDFContentSinkState_InDocumentElement:
      rv = OpenObject(aName,aAtts);
      break;

  case eRDFContentSinkState_InDescriptionElement:
      rv = OpenProperty(aName,aAtts);
      break;

  case eRDFContentSinkState_InContainerElement:
      rv = OpenMember(aName,aAtts);
      break;

  case eRDFContentSinkState_InPropertyElement:
  case eRDFContentSinkState_InMemberElement:
      rv = OpenValue(aName,aAtts);
      break;

  case eRDFContentSinkState_InEpilog:
      PR_LOG(gLog, PR_LOG_WARNING,
             ("rdfxml: unexpected content in epilog at line %d",
              aLineNumber));
      break;
  }

  return rv;
}

NS_IMETHODIMP 
RDFContentSinkImpl::HandleEndElement(const PRUnichar *aName)
{
  FlushText();

  nsIRDFResource* resource;
  if (NS_FAILED(PopContext(resource, mState, mParseMode))) {
      
#ifdef PR_LOGGING
      if (PR_LOG_TEST(gLog, PR_LOG_WARNING)) {
          nsAutoString tagStr(aName);
          char* tagCStr = ToNewCString(tagStr);

          PR_LogPrint
                 ("rdfxml: extra close tag '%s' at line %d",
                  tagCStr, 0);

          NS_Free(tagCStr);
      }
#endif

      return NS_ERROR_UNEXPECTED; 
  }

  
  
  switch (mState) {
    case eRDFContentSinkState_InMemberElement: 
      {
        nsCOMPtr<nsIRDFContainer> container;
        NS_NewRDFContainer(getter_AddRefs(container));
        container->Init(mDataSource, GetContextElement(1));
        container->AppendElement(resource);
      } 
      break;

    case eRDFContentSinkState_InPropertyElement: 
      {
        mDataSource->Assert(GetContextElement(1), GetContextElement(0), resource, PR_TRUE);                                          
      } break;
    default:
      break;
  }
  
  if (mContextStack->IsEmpty())
      mState = eRDFContentSinkState_InEpilog;

  NS_IF_RELEASE(resource);
  return NS_OK;
}
 
NS_IMETHODIMP 
RDFContentSinkImpl::HandleComment(const PRUnichar *aName)
{
    return NS_OK;
}

NS_IMETHODIMP 
RDFContentSinkImpl::HandleCDataSection(const PRUnichar *aData, 
                                       PRUint32 aLength)
{
  return aData ?  AddText(aData, aLength) : NS_OK;
}

NS_IMETHODIMP 
RDFContentSinkImpl::HandleDoctypeDecl(const nsAString & aSubset, 
                                      const nsAString & aName, 
                                      const nsAString & aSystemId, 
                                      const nsAString & aPublicId,
                                      nsISupports* aCatalogData)
{
    return NS_OK;
}

NS_IMETHODIMP 
RDFContentSinkImpl::HandleCharacterData(const PRUnichar *aData, 
                                        PRUint32 aLength)
{
  return aData ?  AddText(aData, aLength) : NS_OK;
}

NS_IMETHODIMP 
RDFContentSinkImpl::HandleProcessingInstruction(const PRUnichar *aTarget, 
                                                const PRUnichar *aData)
{
    return NS_OK;
}

NS_IMETHODIMP 
RDFContentSinkImpl::HandleXMLDeclaration(const PRUnichar *aVersion,
                                         const PRUnichar *aEncoding,
                                         PRInt32 aStandalone)
{
    return NS_OK;
}

NS_IMETHODIMP
RDFContentSinkImpl::ReportError(const PRUnichar* aErrorText, 
                                const PRUnichar* aSourceText,
                                nsIScriptError *aError,
                                PRBool *_retval)
{
  NS_PRECONDITION(aError && aSourceText && aErrorText, "Check arguments!!!");

  
  *_retval = PR_TRUE;
  return NS_OK;
}




NS_IMETHODIMP 
RDFContentSinkImpl::WillParse(void)
{
    return NS_OK;
}


NS_IMETHODIMP 
RDFContentSinkImpl::WillBuildModel(nsDTDMode)
{
    if (mDataSource) {
        nsCOMPtr<nsIRDFXMLSink> sink = do_QueryInterface(mDataSource);
        if (sink) 
            return sink->BeginLoad();
    }
    return NS_OK;
}

NS_IMETHODIMP 
RDFContentSinkImpl::DidBuildModel(PRBool aTerminated)
{
    if (mDataSource) {
        nsCOMPtr<nsIRDFXMLSink> sink = do_QueryInterface(mDataSource);
        if (sink)
            return sink->EndLoad();
    }
    return NS_OK;
}

NS_IMETHODIMP 
RDFContentSinkImpl::WillInterrupt(void)
{
    if (mDataSource) {
        nsCOMPtr<nsIRDFXMLSink> sink = do_QueryInterface(mDataSource);
        if (sink)
            return sink->Interrupt();
    }
    return NS_OK;
}

NS_IMETHODIMP 
RDFContentSinkImpl::WillResume(void)
{
    if (mDataSource) {
        nsCOMPtr<nsIRDFXMLSink> sink = do_QueryInterface(mDataSource);
        if (sink)
            return sink->Resume();
    }
    return NS_OK;
}

NS_IMETHODIMP 
RDFContentSinkImpl::SetParser(nsIParser* aParser)
{
    return NS_OK;
}




NS_IMETHODIMP
RDFContentSinkImpl::Init(nsIURI* aURL)
{
    NS_PRECONDITION(aURL != nsnull, "null ptr");
    if (! aURL)
        return NS_ERROR_NULL_POINTER;

    mDocumentURL = aURL;
    NS_ADDREF(aURL);

    mState = eRDFContentSinkState_InProlog;
    return NS_OK;
}

NS_IMETHODIMP
RDFContentSinkImpl::SetDataSource(nsIRDFDataSource* aDataSource)
{
    NS_PRECONDITION(aDataSource != nsnull, "SetDataSource null ptr");
    mDataSource = aDataSource;
    NS_ASSERTION(mDataSource != nsnull,"Couldn't QI RDF DataSource");
    return NS_OK;
}


NS_IMETHODIMP
RDFContentSinkImpl::GetDataSource(nsIRDFDataSource*& aDataSource)
{
    aDataSource = mDataSource;
    NS_IF_ADDREF(aDataSource);
    return NS_OK;
}




static PRBool
rdf_IsDataInBuffer(PRUnichar* buffer, PRInt32 length)
{
    for (PRInt32 i = 0; i < length; ++i) {
        if (buffer[i] == ' ' ||
            buffer[i] == '\t' ||
            buffer[i] == '\n' ||
            buffer[i] == '\r')
            continue;

        return PR_TRUE;
    }
    return PR_FALSE;
}

void
RDFContentSinkImpl::ParseText(nsIRDFNode **aResult)
{
    
    
    nsAutoString value;
    value.Append(mText, mTextLength);
    value.Trim(" \t\n\r");

    switch (mParseMode) {
    case eRDFContentSinkParseMode_Literal:
        {
            nsIRDFLiteral *result;
            gRDFService->GetLiteral(value.get(), &result);
            *aResult = result;
        }
        break;

    case eRDFContentSinkParseMode_Resource:
        {
            nsIRDFResource *result;
            gRDFService->GetUnicodeResource(value, &result);
            *aResult = result;
        }
        break;

    case eRDFContentSinkParseMode_Int:
        {
            PRInt32 i, err;
            i = value.ToInteger(&err);
            nsIRDFInt *result;
            gRDFService->GetIntLiteral(i, &result);
            *aResult = result;
        }
        break;

    case eRDFContentSinkParseMode_Date:
        {
            PRTime t = rdf_ParseDate(nsDependentCString(NS_LossyConvertUTF16toASCII(value).get(), value.Length()));
            nsIRDFDate *result;
            gRDFService->GetDateLiteral(t, &result);
            *aResult = result;
        }
        break;

    default:
        NS_NOTREACHED("unknown parse type");
        break;
    }
}

nsresult
RDFContentSinkImpl::FlushText()
{
    nsresult rv = NS_OK;
    if (0 != mTextLength) {
        if (rdf_IsDataInBuffer(mText, mTextLength)) {
            
            

            switch (mState) {
            case eRDFContentSinkState_InMemberElement: {
                nsCOMPtr<nsIRDFNode> node;
                ParseText(getter_AddRefs(node));

                nsCOMPtr<nsIRDFContainer> container;
                NS_NewRDFContainer(getter_AddRefs(container));
                container->Init(mDataSource, GetContextElement(1));

                container->AppendElement(node);
            } break;

            case eRDFContentSinkState_InPropertyElement: {
                nsCOMPtr<nsIRDFNode> node;
                ParseText(getter_AddRefs(node));

                mDataSource->Assert(GetContextElement(1), GetContextElement(0), node, PR_TRUE);
            } break;

            default:
                
                break;
            }
        }
        mTextLength = 0;
    }
    return rv;
}


nsresult
RDFContentSinkImpl::AddText(const PRUnichar* aText, PRInt32 aLength)
{
    
    if (0 == mTextSize) {
        mText = (PRUnichar *) PR_MALLOC(sizeof(PRUnichar) * 4096);
        if (!mText) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        mTextSize = 4096;
    }

    
    
    
    PRInt32 amount = mTextSize - mTextLength;
    if (amount < aLength) {
        
        
        
        PRInt32 newSize = (2 * mTextSize > (mTextSize + aLength)) ?
                          (2 * mTextSize) : (mTextSize + aLength);
        PRUnichar* newText = 
            (PRUnichar *) PR_REALLOC(mText, sizeof(PRUnichar) * newSize);
        if (!newText)
            return NS_ERROR_OUT_OF_MEMORY;
        mTextSize = newSize;
        mText = newText;
    }
    memcpy(&mText[mTextLength], aText, sizeof(PRUnichar) * aLength);
    mTextLength += aLength;

    return NS_OK;
}

PRBool
rdf_RequiresAbsoluteURI(const nsString& uri)
{
    
    return !(StringBeginsWith(uri, NS_LITERAL_STRING("urn:")) ||
             StringBeginsWith(uri, NS_LITERAL_STRING("chrome:")));
}

nsresult
RDFContentSinkImpl::GetIdAboutAttribute(const PRUnichar** aAttributes,
                                        nsIRDFResource** aResource,
                                        PRBool* aIsAnonymous)
{
    
    nsresult rv = NS_OK;

    nsAutoString nodeID;

    nsCOMPtr<nsIAtom> localName;
    for (; *aAttributes; aAttributes += 2) {
        const nsDependentSubstring& nameSpaceURI =
            SplitExpatName(aAttributes[0], getter_AddRefs(localName));

        
        
        
        if (!nameSpaceURI.IsEmpty() &&
            !nameSpaceURI.EqualsLiteral(RDF_NAMESPACE_URI)) {
          continue;
        }

        
        
      
        if (localName == kAboutAtom) {
            if (aIsAnonymous)
                *aIsAnonymous = PR_FALSE;

            nsAutoString relURI(aAttributes[1]);
            if (rdf_RequiresAbsoluteURI(relURI)) {
                nsCAutoString uri;
                rv = mDocumentURL->Resolve(NS_ConvertUTF16toUTF8(aAttributes[1]), uri);
                if (NS_FAILED(rv)) return rv;
                
                return gRDFService->GetResource(uri, 
                                                aResource);
            } 
            return gRDFService->GetResource(NS_ConvertUTF16toUTF8(aAttributes[1]), 
                                            aResource);
        }
        else if (localName == kIdAtom) {
            if (aIsAnonymous)
                *aIsAnonymous = PR_FALSE;
            
            
            
            

            
            
            
            nsCAutoString name;
            nsCAutoString ref('#');
            AppendUTF16toUTF8(aAttributes[1], ref);

            rv = mDocumentURL->Resolve(ref, name);
            if (NS_FAILED(rv)) return rv;

            return gRDFService->GetResource(name, aResource);
        }
        else if (localName == kNodeIdAtom) {
            nodeID.Assign(aAttributes[1]);
        }
        else if (localName == kAboutEachAtom) {
            
            
            
            
        }
    }

    
    if (aIsAnonymous)
        *aIsAnonymous = PR_TRUE;

    
    
    if (!nodeID.IsEmpty()) {
        mNodeIDMap.Get(nodeID,aResource);

        if (!*aResource) {
            rv = gRDFService->GetAnonymousResource(aResource);
            mNodeIDMap.Put(nodeID,*aResource);
        }
    }
    else {
        rv = gRDFService->GetAnonymousResource(aResource);
    }

    return rv;
}

nsresult
RDFContentSinkImpl::GetResourceAttribute(const PRUnichar** aAttributes,
                                         nsIRDFResource** aResource)
{
  nsCOMPtr<nsIAtom> localName;

  nsAutoString nodeID;

  for (; *aAttributes; aAttributes += 2) {
      const nsDependentSubstring& nameSpaceURI =
          SplitExpatName(aAttributes[0], getter_AddRefs(localName));

      
      
      
      if (!nameSpaceURI.IsEmpty() &&
          !nameSpaceURI.EqualsLiteral(RDF_NAMESPACE_URI)) {
          continue;
      }

      
      

      if (localName == kResourceAtom) {
          
          
          
          nsAutoString relURI(aAttributes[1]);
          if (rdf_RequiresAbsoluteURI(relURI)) {
              nsresult rv;
              nsCAutoString uri;

              rv = mDocumentURL->Resolve(NS_ConvertUTF16toUTF8(aAttributes[1]), uri);
              if (NS_FAILED(rv)) return rv;

              return gRDFService->GetResource(uri, aResource);
          } 
          return gRDFService->GetResource(NS_ConvertUTF16toUTF8(aAttributes[1]), 
                                          aResource);
      }
      else if (localName == kNodeIdAtom) {
          nodeID.Assign(aAttributes[1]);
      }
  }
    
  
  
  if (!nodeID.IsEmpty()) {
      mNodeIDMap.Get(nodeID,aResource);

      if (!*aResource) {
          nsresult rv;
          rv = gRDFService->GetAnonymousResource(aResource);
          if (NS_FAILED(rv)) {
              return rv;
          }
          mNodeIDMap.Put(nodeID,*aResource);
      }
      return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

nsresult
RDFContentSinkImpl::AddProperties(const PRUnichar** aAttributes,
                                  nsIRDFResource* aSubject,
                                  PRInt32* aCount)
{
  if (aCount)
      *aCount = 0;

  nsCOMPtr<nsIAtom> localName;
  for (; *aAttributes; aAttributes += 2) {
      const nsDependentSubstring& nameSpaceURI =
          SplitExpatName(aAttributes[0], getter_AddRefs(localName));

      
      if (nameSpaceURI.EqualsLiteral("http://www.w3.org/2000/xmlns/")) {
        continue;
      }

      
      
      
      if (localName == kAboutAtom || localName == kIdAtom ||
          localName == kResourceAtom || localName == kNodeIdAtom) {
          if (nameSpaceURI.IsEmpty() ||
              nameSpaceURI.EqualsLiteral(RDF_NAMESPACE_URI))
              continue;
      }

      
      
      if (localName == kParseTypeAtom) {
          if (nameSpaceURI.IsEmpty() ||
              nameSpaceURI.EqualsLiteral(RDF_NAMESPACE_URI) ||
              nameSpaceURI.EqualsLiteral(NC_NAMESPACE_URI)) {
              continue;
          }
      }

      const char* attrName;
      localName->GetUTF8String(&attrName);

      NS_ConvertUTF16toUTF8 propertyStr(nameSpaceURI);    
      propertyStr.Append(attrName);

      
      nsCOMPtr<nsIRDFResource> property;
      gRDFService->GetResource(propertyStr, getter_AddRefs(property));

      nsCOMPtr<nsIRDFLiteral> target;
      gRDFService->GetLiteral(aAttributes[1], 
                              getter_AddRefs(target));

      mDataSource->Assert(aSubject, property, target, PR_TRUE);
  }
  return NS_OK;
}

void
RDFContentSinkImpl::SetParseMode(const PRUnichar **aAttributes)
{
    nsCOMPtr<nsIAtom> localName;
    for (; *aAttributes; aAttributes += 2) {
        const nsDependentSubstring& nameSpaceURI =
            SplitExpatName(aAttributes[0], getter_AddRefs(localName));

        if (localName == kParseTypeAtom) {
            nsDependentString v(aAttributes[1]);

            if (nameSpaceURI.IsEmpty() ||
                nameSpaceURI.EqualsLiteral(RDF_NAMESPACE_URI)) {
                if (v.EqualsLiteral("Resource"))
                    mParseMode = eRDFContentSinkParseMode_Resource;

                break;
            }
            else if (nameSpaceURI.EqualsLiteral(NC_NAMESPACE_URI)) {
                if (v.EqualsLiteral("Date"))
                    mParseMode = eRDFContentSinkParseMode_Date;
                else if (v.EqualsLiteral("Integer"))
                    mParseMode = eRDFContentSinkParseMode_Int;

                break;
            }
        }
    }
}




nsresult
RDFContentSinkImpl::OpenRDF(const PRUnichar* aName)
{
    
    
    
    nsCOMPtr<nsIAtom> localName;
    const nsDependentSubstring& nameSpaceURI =
        SplitExpatName(aName, getter_AddRefs(localName));

    if (!nameSpaceURI.EqualsLiteral(RDF_NAMESPACE_URI) || localName != kRDFAtom) {
       
       
       

        return NS_ERROR_UNEXPECTED;
    }

    PushContext(nsnull, mState, mParseMode);
    mState = eRDFContentSinkState_InDocumentElement;
    return NS_OK;
}

nsresult
RDFContentSinkImpl::OpenObject(const PRUnichar* aName, 
                               const PRUnichar** aAttributes)
{
    
    
    
    nsCOMPtr<nsIAtom> localName;
    const nsDependentSubstring& nameSpaceURI =
        SplitExpatName(aName, getter_AddRefs(localName));

    
    nsCOMPtr<nsIRDFResource> source;
    GetIdAboutAttribute(aAttributes, getter_AddRefs(source));

    
    if (! source)
        return NS_ERROR_FAILURE;

    
    PushContext(source, mState, mParseMode);

    
    
    
    PRBool isaTypedNode = PR_TRUE;

    if (nameSpaceURI.EqualsLiteral(RDF_NAMESPACE_URI)) {
        isaTypedNode = PR_FALSE;

        if (localName == kDescriptionAtom) {
            
            mState = eRDFContentSinkState_InDescriptionElement;
        }
        else if (localName == kBagAtom) {
            
            InitContainer(kRDF_Bag, source);
            mState = eRDFContentSinkState_InContainerElement;
        }
        else if (localName == kSeqAtom) {
            
            InitContainer(kRDF_Seq, source);
            mState = eRDFContentSinkState_InContainerElement;
        }
        else if (localName == kAltAtom) {
            
            InitContainer(kRDF_Alt, source);
            mState = eRDFContentSinkState_InContainerElement;
        }
        else {
            
            
            isaTypedNode = PR_TRUE;
        }
    }

    if (isaTypedNode) {
        const char* attrName;
        localName->GetUTF8String(&attrName);

        NS_ConvertUTF16toUTF8 typeStr(nameSpaceURI);
        typeStr.Append(attrName);

        nsCOMPtr<nsIRDFResource> type;
        nsresult rv = gRDFService->GetResource(typeStr, getter_AddRefs(type));
        if (NS_FAILED(rv)) return rv;

        rv = mDataSource->Assert(source, kRDF_type, type, PR_TRUE);
        if (NS_FAILED(rv)) return rv;

        mState = eRDFContentSinkState_InDescriptionElement;
    }

    AddProperties(aAttributes, source);
    return NS_OK;
}

nsresult
RDFContentSinkImpl::OpenProperty(const PRUnichar* aName, const PRUnichar** aAttributes)
{
    nsresult rv;

    
    
    
    nsCOMPtr<nsIAtom> localName;
    const nsDependentSubstring& nameSpaceURI =
        SplitExpatName(aName, getter_AddRefs(localName));

    const char* attrName;
    localName->GetUTF8String(&attrName);

    NS_ConvertUTF16toUTF8 propertyStr(nameSpaceURI);
    propertyStr.Append(attrName);

    nsCOMPtr<nsIRDFResource> property;
    rv = gRDFService->GetResource(propertyStr, getter_AddRefs(property));
    if (NS_FAILED(rv)) return rv;

    
    
    nsCOMPtr<nsIRDFResource> target;
    GetResourceAttribute(aAttributes, getter_AddRefs(target));

    PRBool isAnonymous = PR_FALSE;

    if (! target) {
        
        

        
        
        

        
        
        GetIdAboutAttribute(aAttributes, getter_AddRefs(target), &isAnonymous);
    }

    if (target) {
        
        
        
        
        PRInt32 count;
        rv = AddProperties(aAttributes, target, &count);
        NS_ASSERTION(NS_SUCCEEDED(rv), "problem adding properties");
        if (NS_FAILED(rv)) return rv;

        if (count || !isAnonymous) {
            
            
            
            
            
            rv = mDataSource->Assert(GetContextElement(0), property, target, PR_TRUE);
            if (NS_FAILED(rv)) return rv;
        }

        
        
        
        
    }

    
    PushContext(property, mState, mParseMode);
    mState = eRDFContentSinkState_InPropertyElement;
    SetParseMode(aAttributes);

    return NS_OK;
}

nsresult
RDFContentSinkImpl::OpenMember(const PRUnichar* aName, 
                               const PRUnichar** aAttributes)
{
    
    
    
    nsresult rv;

    nsCOMPtr<nsIAtom> localName;
    const nsDependentSubstring& nameSpaceURI =
        SplitExpatName(aName, getter_AddRefs(localName));

    if (!nameSpaceURI.EqualsLiteral(RDF_NAMESPACE_URI) ||
        localName != kLiAtom) {
        PR_LOG(gLog, PR_LOG_ALWAYS,
               ("rdfxml: expected RDF:li at line %d",
                -1)); 

        return NS_ERROR_UNEXPECTED;
    }

    
    nsIRDFResource* container = GetContextElement(0);
    if (! container)
        return NS_ERROR_NULL_POINTER;

    nsIRDFResource* resource;
    if (NS_SUCCEEDED(rv = GetResourceAttribute(aAttributes, &resource))) {
        
        
        nsCOMPtr<nsIRDFContainer> c;
        NS_NewRDFContainer(getter_AddRefs(c));
        c->Init(mDataSource, container);
        c->AppendElement(resource);

        
        
        
        
        NS_RELEASE(resource);
    }

    
    
    
    
    
    PushContext(nsnull, mState, mParseMode);
    mState = eRDFContentSinkState_InMemberElement;
    SetParseMode(aAttributes);

    return NS_OK;
}


nsresult
RDFContentSinkImpl::OpenValue(const PRUnichar* aName, const PRUnichar** aAttributes)
{
    
    
    return OpenObject(aName,aAttributes);
}



void
RDFContentSinkImpl::RegisterNamespaces(const PRUnichar **aAttributes)
{
    nsCOMPtr<nsIRDFXMLSink> sink = do_QueryInterface(mDataSource);
    if (!sink) {
        return;
    }
    NS_NAMED_LITERAL_STRING(xmlns, "http://www.w3.org/2000/xmlns/");
    for (; *aAttributes; aAttributes += 2) {
        
        const PRUnichar* attr = aAttributes[0];
        const PRUnichar* xmlnsP = xmlns.BeginReading();
        while (*attr ==  *xmlnsP) {
            ++attr;
            ++xmlnsP;
        }
        if (*attr != 0xFFFF ||
            xmlnsP != xmlns.EndReading()) {
            continue;
        }
        
        const PRUnichar* endLocal = ++attr;
        while (*endLocal && *endLocal != 0xFFFF) {
            ++endLocal;
        }
        nsDependentSubstring lname(attr, endLocal);
        nsCOMPtr<nsIAtom> preferred = do_GetAtom(lname);
        if (preferred == kXMLNSAtom) {
            preferred = nsnull;
        }
        sink->AddNameSpace(preferred, nsDependentString(aAttributes[1]));
    }
}




const nsDependentSubstring
RDFContentSinkImpl::SplitExpatName(const PRUnichar *aExpatName,
                                   nsIAtom **aLocalName)
{
    









    const PRUnichar *uriEnd = aExpatName;
    const PRUnichar *nameStart = aExpatName;
    const PRUnichar *pos;
    for (pos = aExpatName; *pos; ++pos) {
        if (*pos == 0xFFFF) {
            if (uriEnd != aExpatName) {
                break;
            }

            uriEnd = pos;
            nameStart = pos + 1;
        }
    }

    const nsDependentSubstring& nameSpaceURI = Substring(aExpatName, uriEnd);
    *aLocalName = NS_NewAtom(NS_ConvertUTF16toUTF8(nameStart,
                                                   pos - nameStart));
    return nameSpaceURI;
}

nsresult
RDFContentSinkImpl::InitContainer(nsIRDFResource* aContainerType, nsIRDFResource* aContainer)
{
    
    
    
    nsresult rv;

    static const ContainerInfo gContainerInfo[] = {
        { &RDFContentSinkImpl::kRDF_Alt, &nsIRDFContainerUtils::IsAlt, &nsIRDFContainerUtils::MakeAlt },
        { &RDFContentSinkImpl::kRDF_Bag, &nsIRDFContainerUtils::IsBag, &nsIRDFContainerUtils::MakeBag },
        { &RDFContentSinkImpl::kRDF_Seq, &nsIRDFContainerUtils::IsSeq, &nsIRDFContainerUtils::MakeSeq },
        { 0, 0, 0 },
    };

    for (const ContainerInfo* info = gContainerInfo; info->mType != 0; ++info) {
        if (*info->mType != aContainerType)
            continue;

        PRBool isContainer;
        rv = (gRDFContainerUtils->*(info->mTestFn))(mDataSource, aContainer, &isContainer);
        if (isContainer) {
            rv = ReinitContainer(aContainerType, aContainer);
        }
        else {
            rv = (gRDFContainerUtils->*(info->mMakeFn))(mDataSource, aContainer, nsnull);
        }
        return rv;
    }

    NS_NOTREACHED("not an RDF container type");
    return NS_ERROR_FAILURE;
}



nsresult
RDFContentSinkImpl::ReinitContainer(nsIRDFResource* aContainerType, nsIRDFResource* aContainer)
{
    
    
    
    
    
    nsresult rv;

    nsCOMPtr<nsIRDFLiteral> one;
    rv = gRDFService->GetLiteral(NS_LITERAL_STRING("1").get(), getter_AddRefs(one));
    if (NS_FAILED(rv)) return rv;

    
    nsCOMPtr<nsIRDFNode> nextval;
    rv = mDataSource->GetTarget(aContainer, kRDF_nextVal, PR_TRUE, getter_AddRefs(nextval));
    if (NS_FAILED(rv)) return rv;

    rv = mDataSource->Change(aContainer, kRDF_nextVal, nextval, one);
    if (NS_FAILED(rv)) return rv;

    
    rv = mDataSource->Assert(aContainer, kRDF_instanceOf, aContainerType, PR_TRUE);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to mark container as such");
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}




nsIRDFResource* 
RDFContentSinkImpl::GetContextElement(PRInt32 ancestor )
{
    if ((nsnull == mContextStack) ||
        (PRUint32(ancestor) >= mContextStack->Length())) {
        return nsnull;
    }

    return mContextStack->ElementAt(
           mContextStack->Length()-ancestor-1).mResource;
}

PRInt32 
RDFContentSinkImpl::PushContext(nsIRDFResource         *aResource,
                                RDFContentSinkState     aState,
                                RDFContentSinkParseMode aParseMode)
{
    if (! mContextStack) {
        mContextStack = new nsAutoTArray<RDFContextStackElement, 8>();
        if (! mContextStack)
            return 0;
    }

    RDFContextStackElement* e = mContextStack->AppendElement();
    if (! e)
        return mContextStack->Length();

    e->mResource  = aResource;
    e->mState     = aState;
    e->mParseMode = aParseMode;
  
    return mContextStack->Length();
}
 
nsresult
RDFContentSinkImpl::PopContext(nsIRDFResource         *&aResource,
                               RDFContentSinkState     &aState,
                               RDFContentSinkParseMode &aParseMode)
{
    if ((nsnull == mContextStack) ||
        (mContextStack->IsEmpty())) {
        return NS_ERROR_NULL_POINTER;
    }

    PRUint32 i = mContextStack->Length() - 1;
    RDFContextStackElement &e = mContextStack->ElementAt(i);

    aResource  = e.mResource;
    NS_IF_ADDREF(aResource);
    aState     = e.mState;
    aParseMode = e.mParseMode;

    mContextStack->RemoveElementAt(i);
    return NS_OK;
}
 



nsresult
NS_NewRDFContentSink(nsIRDFContentSink** aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    RDFContentSinkImpl* sink = new RDFContentSinkImpl();
    if (! sink)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(sink);
    *aResult = sink;
    return NS_OK;
}
