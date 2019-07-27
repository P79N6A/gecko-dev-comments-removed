







































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
#include "rdf.h"
#include "rdfutil.h"
#include "nsReadableUtils.h"
#include "nsIExpatSink.h"
#include "nsCRT.h"
#include "nsIAtom.h"
#include "nsStaticAtom.h"
#include "nsIScriptError.h"
#include "nsIDTD.h"

using namespace mozilla;



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
                     (nsIRDFDataSource*, nsIRDFResource*, bool*));

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

    
    NS_DECL_ISUPPORTS
    NS_DECL_NSIEXPATSINK

    
    NS_IMETHOD WillParse(void) override;
    NS_IMETHOD WillBuildModel(nsDTDMode aDTDMode) override;
    NS_IMETHOD DidBuildModel(bool aTerminated) override;
    NS_IMETHOD WillInterrupt(void) override;
    NS_IMETHOD WillResume(void) override;
    NS_IMETHOD SetParser(nsParserBase* aParser) override;
    virtual void FlushPendingNotifications(mozFlushType aType) override { }
    NS_IMETHOD SetDocumentCharset(nsACString& aCharset) override { return NS_OK; }
    virtual nsISupports *GetTarget() override { return nullptr; }

    
    NS_IMETHOD Init(nsIURI* aURL) override;
    NS_IMETHOD SetDataSource(nsIRDFDataSource* aDataSource) override;
    NS_IMETHOD GetDataSource(nsIRDFDataSource*& aDataSource) override;

    
    static int32_t gRefCnt;
    static nsIRDFService* gRDFService;
    static nsIRDFContainerUtils* gRDFContainerUtils;
    static nsIRDFResource* kRDF_type;
    static nsIRDFResource* kRDF_instanceOf; 
    static nsIRDFResource* kRDF_Alt;
    static nsIRDFResource* kRDF_Bag;
    static nsIRDFResource* kRDF_Seq;
    static nsIRDFResource* kRDF_nextVal;

#define RDF_ATOM(name_, value_) static nsIAtom* name_;
#include "nsRDFContentSinkAtomList.h"
#undef RDF_ATOM

    typedef struct ContainerInfo {
        nsIRDFResource**  mType;
        nsContainerTestFn mTestFn;
        nsMakeContainerFn mMakeFn;
    } ContainerInfo;

protected:
    virtual ~RDFContentSinkImpl();

    
    void ParseText(nsIRDFNode **aResult);

    nsresult FlushText();
    nsresult AddText(const char16_t* aText, int32_t aLength);

    
    nsresult OpenRDF(const char16_t* aName);
    nsresult OpenObject(const char16_t* aName ,const char16_t** aAttributes);
    nsresult OpenProperty(const char16_t* aName, const char16_t** aAttributes);
    nsresult OpenMember(const char16_t* aName, const char16_t** aAttributes);
    nsresult OpenValue(const char16_t* aName, const char16_t** aAttributes);
    
    nsresult GetIdAboutAttribute(const char16_t** aAttributes, nsIRDFResource** aResource, bool* aIsAnonymous = nullptr);
    nsresult GetResourceAttribute(const char16_t** aAttributes, nsIRDFResource** aResource);
    nsresult AddProperties(const char16_t** aAttributes, nsIRDFResource* aSubject, int32_t* aCount = nullptr);
    void SetParseMode(const char16_t **aAttributes);

    char16_t* mText;
    int32_t mTextLength;
    int32_t mTextSize;

    






    void RegisterNamespaces(const char16_t **aAttributes);

    





    const nsDependentSubstring SplitExpatName(const char16_t *aExpatName,
                                              nsIAtom **aLocalName);

    enum eContainerType { eBag, eSeq, eAlt };
    nsresult InitContainer(nsIRDFResource* aContainerType, nsIRDFResource* aContainer);
    nsresult ReinitContainer(nsIRDFResource* aContainerType, nsIRDFResource* aContainer);

    
    nsCOMPtr<nsIRDFDataSource> mDataSource;

    
    nsInterfaceHashtable<nsStringHashKey, nsIRDFResource> mNodeIDMap;

    
    RDFContentSinkState mState;
    RDFContentSinkParseMode mParseMode;

    
    int32_t         
    PushContext(nsIRDFResource *aContext,
                RDFContentSinkState aState,
                RDFContentSinkParseMode aParseMode);

    nsresult
    PopContext(nsIRDFResource         *&aContext,
               RDFContentSinkState     &aState,
               RDFContentSinkParseMode &aParseMode);

    nsIRDFResource* GetContextElement(int32_t ancestor = 0);


    struct RDFContextStackElement {
        nsCOMPtr<nsIRDFResource> mResource;
        RDFContentSinkState      mState;
        RDFContentSinkParseMode  mParseMode;
    };

    nsAutoTArray<RDFContextStackElement, 8>* mContextStack;

    nsIURI*      mDocumentURL;

private:
#ifdef PR_LOGGING
    static PRLogModuleInfo* gLog;
#endif
};

int32_t         RDFContentSinkImpl::gRefCnt = 0;
nsIRDFService*  RDFContentSinkImpl::gRDFService;
nsIRDFContainerUtils* RDFContentSinkImpl::gRDFContainerUtils;
nsIRDFResource* RDFContentSinkImpl::kRDF_type;
nsIRDFResource* RDFContentSinkImpl::kRDF_instanceOf;
nsIRDFResource* RDFContentSinkImpl::kRDF_Alt;
nsIRDFResource* RDFContentSinkImpl::kRDF_Bag;
nsIRDFResource* RDFContentSinkImpl::kRDF_Seq;
nsIRDFResource* RDFContentSinkImpl::kRDF_nextVal;

#ifdef PR_LOGGING
PRLogModuleInfo* RDFContentSinkImpl::gLog;
#endif



#define RDF_ATOM(name_, value_) nsIAtom* RDFContentSinkImpl::name_;
#include "nsRDFContentSinkAtomList.h"
#undef RDF_ATOM

#define RDF_ATOM(name_, value_) NS_STATIC_ATOM_BUFFER(name_##_buffer, value_)
#include "nsRDFContentSinkAtomList.h"
#undef RDF_ATOM

static const nsStaticAtom rdf_atoms[] = {
#define RDF_ATOM(name_, value_) NS_STATIC_ATOM(name_##_buffer, &RDFContentSinkImpl::name_),
#include "nsRDFContentSinkAtomList.h"
#undef RDF_ATOM
};

RDFContentSinkImpl::RDFContentSinkImpl()
    : mText(nullptr),
      mTextLength(0),
      mTextSize(0),
      mState(eRDFContentSinkState_InProlog),
      mParseMode(eRDFContentSinkParseMode_Literal),
      mContextStack(nullptr),
      mDocumentURL(nullptr)
{
    if (gRefCnt++ == 0) {
        NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);
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

        NS_DEFINE_CID(kRDFContainerUtilsCID, NS_RDFCONTAINERUTILS_CID);
        rv = CallGetService(kRDFContainerUtilsCID, &gRDFContainerUtils);

        NS_RegisterStaticAtoms(rdf_atoms);
    }

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

        
        
        
        int32_t i = mContextStack->Length();
        while (0 < i--) {
            nsIRDFResource* resource = nullptr;
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
    free(mText);


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

    NS_DEFINE_IID(kIContentSinkIID,    NS_ICONTENT_SINK_IID);
    NS_DEFINE_IID(kIExpatSinkIID,      NS_IEXPATSINK_IID);
    NS_DEFINE_IID(kISupportsIID,       NS_ISUPPORTS_IID);
    NS_DEFINE_IID(kIXMLContentSinkIID, NS_IXMLCONTENT_SINK_IID);
    NS_DEFINE_IID(kIRDFContentSinkIID, NS_IRDFCONTENTSINK_IID);

    *result = nullptr;
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
RDFContentSinkImpl::HandleStartElement(const char16_t *aName, 
                                       const char16_t **aAtts, 
                                       uint32_t aAttsCount, 
                                       uint32_t aLineNumber)
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
RDFContentSinkImpl::HandleEndElement(const char16_t *aName)
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

          free(tagCStr);
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
        mDataSource->Assert(GetContextElement(1), GetContextElement(0), resource, true);                                          
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
RDFContentSinkImpl::HandleComment(const char16_t *aName)
{
    return NS_OK;
}

NS_IMETHODIMP 
RDFContentSinkImpl::HandleCDataSection(const char16_t *aData, 
                                       uint32_t aLength)
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
RDFContentSinkImpl::HandleCharacterData(const char16_t *aData, 
                                        uint32_t aLength)
{
  return aData ?  AddText(aData, aLength) : NS_OK;
}

NS_IMETHODIMP 
RDFContentSinkImpl::HandleProcessingInstruction(const char16_t *aTarget, 
                                                const char16_t *aData)
{
    return NS_OK;
}

NS_IMETHODIMP 
RDFContentSinkImpl::HandleXMLDeclaration(const char16_t *aVersion,
                                         const char16_t *aEncoding,
                                         int32_t aStandalone)
{
    return NS_OK;
}

NS_IMETHODIMP
RDFContentSinkImpl::ReportError(const char16_t* aErrorText, 
                                const char16_t* aSourceText,
                                nsIScriptError *aError,
                                bool *_retval)
{
  NS_PRECONDITION(aError && aSourceText && aErrorText, "Check arguments!!!");

  
  *_retval = true;
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
RDFContentSinkImpl::DidBuildModel(bool aTerminated)
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
RDFContentSinkImpl::SetParser(nsParserBase* aParser)
{
    return NS_OK;
}




NS_IMETHODIMP
RDFContentSinkImpl::Init(nsIURI* aURL)
{
    NS_PRECONDITION(aURL != nullptr, "null ptr");
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
    NS_PRECONDITION(aDataSource != nullptr, "SetDataSource null ptr");
    mDataSource = aDataSource;
    NS_ASSERTION(mDataSource != nullptr,"Couldn't QI RDF DataSource");
    return NS_OK;
}


NS_IMETHODIMP
RDFContentSinkImpl::GetDataSource(nsIRDFDataSource*& aDataSource)
{
    aDataSource = mDataSource;
    NS_IF_ADDREF(aDataSource);
    return NS_OK;
}




static bool
rdf_IsDataInBuffer(char16_t* buffer, int32_t length)
{
    for (int32_t i = 0; i < length; ++i) {
        if (buffer[i] == ' ' ||
            buffer[i] == '\t' ||
            buffer[i] == '\n' ||
            buffer[i] == '\r')
            continue;

        return true;
    }
    return false;
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
            nsresult err;
            int32_t i = value.ToInteger(&err);
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

                mDataSource->Assert(GetContextElement(1), GetContextElement(0), node, true);
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
RDFContentSinkImpl::AddText(const char16_t* aText, int32_t aLength)
{
    
    if (0 == mTextSize) {
        mText = (char16_t *) malloc(sizeof(char16_t) * 4096);
        if (!mText) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        mTextSize = 4096;
    }

    
    
    
    int32_t amount = mTextSize - mTextLength;
    if (amount < aLength) {
        
        
        
        int32_t newSize = (2 * mTextSize > (mTextSize + aLength)) ?
                          (2 * mTextSize) : (mTextSize + aLength);
        char16_t* newText = 
            (char16_t *) realloc(mText, sizeof(char16_t) * newSize);
        if (!newText)
            return NS_ERROR_OUT_OF_MEMORY;
        mTextSize = newSize;
        mText = newText;
    }
    memcpy(&mText[mTextLength], aText, sizeof(char16_t) * aLength);
    mTextLength += aLength;

    return NS_OK;
}

bool
rdf_RequiresAbsoluteURI(const nsString& uri)
{
    
    return !(StringBeginsWith(uri, NS_LITERAL_STRING("urn:")) ||
             StringBeginsWith(uri, NS_LITERAL_STRING("chrome:")));
}

nsresult
RDFContentSinkImpl::GetIdAboutAttribute(const char16_t** aAttributes,
                                        nsIRDFResource** aResource,
                                        bool* aIsAnonymous)
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
                *aIsAnonymous = false;

            nsAutoString relURI(aAttributes[1]);
            if (rdf_RequiresAbsoluteURI(relURI)) {
                nsAutoCString uri;
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
                *aIsAnonymous = false;
            
            
            
            

            
            
            
            nsAutoCString name;
            nsAutoCString ref('#');
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
        *aIsAnonymous = true;

    
    
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
RDFContentSinkImpl::GetResourceAttribute(const char16_t** aAttributes,
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
              nsAutoCString uri;

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
RDFContentSinkImpl::AddProperties(const char16_t** aAttributes,
                                  nsIRDFResource* aSubject,
                                  int32_t* aCount)
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

      NS_ConvertUTF16toUTF8 propertyStr(nameSpaceURI);    
      propertyStr.Append(nsAtomCString(localName));

      
      nsCOMPtr<nsIRDFResource> property;
      gRDFService->GetResource(propertyStr, getter_AddRefs(property));

      nsCOMPtr<nsIRDFLiteral> target;
      gRDFService->GetLiteral(aAttributes[1], 
                              getter_AddRefs(target));

      mDataSource->Assert(aSubject, property, target, true);
  }
  return NS_OK;
}

void
RDFContentSinkImpl::SetParseMode(const char16_t **aAttributes)
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
RDFContentSinkImpl::OpenRDF(const char16_t* aName)
{
    
    
    
    nsCOMPtr<nsIAtom> localName;
    const nsDependentSubstring& nameSpaceURI =
        SplitExpatName(aName, getter_AddRefs(localName));

    if (!nameSpaceURI.EqualsLiteral(RDF_NAMESPACE_URI) || localName != kRDFAtom) {
       
       
       

        return NS_ERROR_UNEXPECTED;
    }

    PushContext(nullptr, mState, mParseMode);
    mState = eRDFContentSinkState_InDocumentElement;
    return NS_OK;
}

nsresult
RDFContentSinkImpl::OpenObject(const char16_t* aName, 
                               const char16_t** aAttributes)
{
    
    
    
    nsCOMPtr<nsIAtom> localName;
    const nsDependentSubstring& nameSpaceURI =
        SplitExpatName(aName, getter_AddRefs(localName));

    
    nsCOMPtr<nsIRDFResource> source;
    GetIdAboutAttribute(aAttributes, getter_AddRefs(source));

    
    if (! source)
        return NS_ERROR_FAILURE;

    
    PushContext(source, mState, mParseMode);

    
    
    
    bool isaTypedNode = true;

    if (nameSpaceURI.EqualsLiteral(RDF_NAMESPACE_URI)) {
        isaTypedNode = false;

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
            
            
            isaTypedNode = true;
        }
    }

    if (isaTypedNode) {
        NS_ConvertUTF16toUTF8 typeStr(nameSpaceURI);
        typeStr.Append(nsAtomCString(localName));

        nsCOMPtr<nsIRDFResource> type;
        nsresult rv = gRDFService->GetResource(typeStr, getter_AddRefs(type));
        if (NS_FAILED(rv)) return rv;

        rv = mDataSource->Assert(source, kRDF_type, type, true);
        if (NS_FAILED(rv)) return rv;

        mState = eRDFContentSinkState_InDescriptionElement;
    }

    AddProperties(aAttributes, source);
    return NS_OK;
}

nsresult
RDFContentSinkImpl::OpenProperty(const char16_t* aName, const char16_t** aAttributes)
{
    nsresult rv;

    
    
    
    nsCOMPtr<nsIAtom> localName;
    const nsDependentSubstring& nameSpaceURI =
        SplitExpatName(aName, getter_AddRefs(localName));

    NS_ConvertUTF16toUTF8 propertyStr(nameSpaceURI);
    propertyStr.Append(nsAtomCString(localName));

    nsCOMPtr<nsIRDFResource> property;
    rv = gRDFService->GetResource(propertyStr, getter_AddRefs(property));
    if (NS_FAILED(rv)) return rv;

    
    
    nsCOMPtr<nsIRDFResource> target;
    GetResourceAttribute(aAttributes, getter_AddRefs(target));

    bool isAnonymous = false;

    if (! target) {
        
        

        
        
        

        
        
        GetIdAboutAttribute(aAttributes, getter_AddRefs(target), &isAnonymous);
    }

    if (target) {
        
        
        
        
        int32_t count;
        rv = AddProperties(aAttributes, target, &count);
        NS_ASSERTION(NS_SUCCEEDED(rv), "problem adding properties");
        if (NS_FAILED(rv)) return rv;

        if (count || !isAnonymous) {
            
            
            
            
            
            rv = mDataSource->Assert(GetContextElement(0), property, target, true);
            if (NS_FAILED(rv)) return rv;
        }

        
        
        
        
    }

    
    PushContext(property, mState, mParseMode);
    mState = eRDFContentSinkState_InPropertyElement;
    SetParseMode(aAttributes);

    return NS_OK;
}

nsresult
RDFContentSinkImpl::OpenMember(const char16_t* aName, 
                               const char16_t** aAttributes)
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

    
    
    
    
    
    PushContext(nullptr, mState, mParseMode);
    mState = eRDFContentSinkState_InMemberElement;
    SetParseMode(aAttributes);

    return NS_OK;
}


nsresult
RDFContentSinkImpl::OpenValue(const char16_t* aName, const char16_t** aAttributes)
{
    
    
    return OpenObject(aName,aAttributes);
}



void
RDFContentSinkImpl::RegisterNamespaces(const char16_t **aAttributes)
{
    nsCOMPtr<nsIRDFXMLSink> sink = do_QueryInterface(mDataSource);
    if (!sink) {
        return;
    }
    NS_NAMED_LITERAL_STRING(xmlns, "http://www.w3.org/2000/xmlns/");
    for (; *aAttributes; aAttributes += 2) {
        
        const char16_t* attr = aAttributes[0];
        const char16_t* xmlnsP = xmlns.BeginReading();
        while (*attr ==  *xmlnsP) {
            ++attr;
            ++xmlnsP;
        }
        if (*attr != 0xFFFF ||
            xmlnsP != xmlns.EndReading()) {
            continue;
        }
        
        const char16_t* endLocal = ++attr;
        while (*endLocal && *endLocal != 0xFFFF) {
            ++endLocal;
        }
        nsDependentSubstring lname(attr, endLocal);
        nsCOMPtr<nsIAtom> preferred = do_GetAtom(lname);
        if (preferred == kXMLNSAtom) {
            preferred = nullptr;
        }
        sink->AddNameSpace(preferred, nsDependentString(aAttributes[1]));
    }
}




const nsDependentSubstring
RDFContentSinkImpl::SplitExpatName(const char16_t *aExpatName,
                                   nsIAtom **aLocalName)
{
    









    const char16_t *uriEnd = aExpatName;
    const char16_t *nameStart = aExpatName;
    const char16_t *pos;
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
    *aLocalName = NS_NewAtom(Substring(nameStart, pos)).take();
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

        bool isContainer;
        rv = (gRDFContainerUtils->*(info->mTestFn))(mDataSource, aContainer, &isContainer);
        if (isContainer) {
            rv = ReinitContainer(aContainerType, aContainer);
        }
        else {
            rv = (gRDFContainerUtils->*(info->mMakeFn))(mDataSource, aContainer, nullptr);
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
    rv = gRDFService->GetLiteral(MOZ_UTF16("1"), getter_AddRefs(one));
    if (NS_FAILED(rv)) return rv;

    
    nsCOMPtr<nsIRDFNode> nextval;
    rv = mDataSource->GetTarget(aContainer, kRDF_nextVal, true, getter_AddRefs(nextval));
    if (NS_FAILED(rv)) return rv;

    rv = mDataSource->Change(aContainer, kRDF_nextVal, nextval, one);
    if (NS_FAILED(rv)) return rv;

    
    rv = mDataSource->Assert(aContainer, kRDF_instanceOf, aContainerType, true);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to mark container as such");
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}




nsIRDFResource* 
RDFContentSinkImpl::GetContextElement(int32_t ancestor )
{
    if ((nullptr == mContextStack) ||
        (uint32_t(ancestor) >= mContextStack->Length())) {
        return nullptr;
    }

    return mContextStack->ElementAt(
           mContextStack->Length()-ancestor-1).mResource;
}

int32_t 
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
    if ((nullptr == mContextStack) ||
        (mContextStack->IsEmpty())) {
        return NS_ERROR_NULL_POINTER;
    }

    uint32_t i = mContextStack->Length() - 1;
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
    NS_PRECONDITION(aResult != nullptr, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    RDFContentSinkImpl* sink = new RDFContentSinkImpl();
    if (! sink)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(sink);
    *aResult = sink;
    return NS_OK;
}
