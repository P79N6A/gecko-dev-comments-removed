

























































































#include "nsIFileStreams.h"
#include "nsIOutputStream.h"
#include "nsIFile.h"
#include "nsIFileChannel.h"
#include "nsIDTD.h"
#include "nsIRDFPurgeableDataSource.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsIRDFContainerUtils.h"
#include "nsIRDFNode.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsIRDFService.h"
#include "nsIRDFXMLParser.h"
#include "nsIRDFXMLSerializer.h"
#include "nsIRDFXMLSink.h"
#include "nsIRDFXMLSource.h"
#include "nsIServiceManager.h"
#include "nsIStreamListener.h"
#include "nsIURL.h"
#include "nsIFileURL.h"
#include "nsNetUtil.h"
#include "nsIChannel.h"
#include "nsRDFCID.h"
#include "nsRDFBaseDataSources.h"
#include "nsCOMArray.h"
#include "nsXPIDLString.h"
#include "plstr.h"
#include "prio.h"
#include "prthread.h"
#include "rdf.h"
#include "rdfutil.h"
#include "prlog.h"
#include "nsNameSpaceMap.h"
#include "nsCRT.h"
#include "nsCycleCollectionParticipant.h"

#include "rdfIDataSource.h"



static NS_DEFINE_CID(kRDFInMemoryDataSourceCID, NS_RDFINMEMORYDATASOURCE_CID);
static NS_DEFINE_CID(kRDFServiceCID,            NS_RDFSERVICE_CID);

#ifdef PR_LOGGING
static PRLogModuleInfo* gLog;
#endif






class RDFXMLDataSourceImpl : public nsIRDFDataSource,
                             public nsIRDFRemoteDataSource,
                             public nsIRDFXMLSink,
                             public nsIRDFXMLSource,
                             public nsIStreamListener,
                             public rdfIDataSource
{
protected:
    enum LoadState {
        eLoadState_Unloaded,
        eLoadState_Pending,
        eLoadState_Loading,
        eLoadState_Loaded
    };

    nsCOMPtr<nsIRDFDataSource> mInner;
    PRPackedBool        mIsWritable;    
    PRPackedBool        mIsDirty;       
    LoadState           mLoadState;     
    nsCOMArray<nsIRDFXMLSinkObserver> mObservers;
    nsCOMPtr<nsIURI>    mURL;
    nsCOMPtr<nsIStreamListener> mListener;
    nsNameSpaceMap      mNameSpaces;

    
    static PRInt32 gRefCnt;
    static nsIRDFService* gRDFService;

    nsresult Init();
    RDFXMLDataSourceImpl(void);
    virtual ~RDFXMLDataSourceImpl(void);
    nsresult rdfXMLFlush(nsIURI *aURI);

    friend nsresult
    NS_NewRDFXMLDataSource(nsIRDFDataSource** aResult);

    inline PRBool IsLoading() {
        return (mLoadState == eLoadState_Pending) || 
               (mLoadState == eLoadState_Loading);
    }

public:
    
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(RDFXMLDataSourceImpl,
                                             nsIRDFDataSource)

    
    NS_IMETHOD GetURI(char* *uri);

    NS_IMETHOD GetSource(nsIRDFResource* property,
                         nsIRDFNode* target,
                         PRBool tv,
                         nsIRDFResource** source) {
        return mInner->GetSource(property, target, tv, source);
    }

    NS_IMETHOD GetSources(nsIRDFResource* property,
                          nsIRDFNode* target,
                          PRBool tv,
                          nsISimpleEnumerator** sources) {
        return mInner->GetSources(property, target, tv, sources);
    }

    NS_IMETHOD GetTarget(nsIRDFResource* source,
                         nsIRDFResource* property,
                         PRBool tv,
                         nsIRDFNode** target) {
        return mInner->GetTarget(source, property, tv, target);
    }

    NS_IMETHOD GetTargets(nsIRDFResource* source,
                          nsIRDFResource* property,
                          PRBool tv,
                          nsISimpleEnumerator** targets) {
        return mInner->GetTargets(source, property, tv, targets);
    }

    NS_IMETHOD Assert(nsIRDFResource* aSource,
                      nsIRDFResource* aProperty,
                      nsIRDFNode* aTarget,
                      PRBool tv);

    NS_IMETHOD Unassert(nsIRDFResource* source,
                        nsIRDFResource* property,
                        nsIRDFNode* target);

    NS_IMETHOD Change(nsIRDFResource* aSource,
                      nsIRDFResource* aProperty,
                      nsIRDFNode* aOldTarget,
                      nsIRDFNode* aNewTarget);

    NS_IMETHOD Move(nsIRDFResource* aOldSource,
                    nsIRDFResource* aNewSource,
                    nsIRDFResource* aProperty,
                    nsIRDFNode* aTarget);

    NS_IMETHOD HasAssertion(nsIRDFResource* source,
                            nsIRDFResource* property,
                            nsIRDFNode* target,
                            PRBool tv,
                            PRBool* hasAssertion) {
        return mInner->HasAssertion(source, property, target, tv, hasAssertion);
    }

    NS_IMETHOD AddObserver(nsIRDFObserver* aObserver) {
        return mInner->AddObserver(aObserver);
    }

    NS_IMETHOD RemoveObserver(nsIRDFObserver* aObserver) {
        return mInner->RemoveObserver(aObserver);
    }

    NS_IMETHOD HasArcIn(nsIRDFNode *aNode, nsIRDFResource *aArc, PRBool *_retval) {
        return mInner->HasArcIn(aNode, aArc, _retval);
    }

    NS_IMETHOD HasArcOut(nsIRDFResource *aSource, nsIRDFResource *aArc, PRBool *_retval) {
        return mInner->HasArcOut(aSource, aArc, _retval);
    }

    NS_IMETHOD ArcLabelsIn(nsIRDFNode* node,
                           nsISimpleEnumerator** labels) {
        return mInner->ArcLabelsIn(node, labels);
    }

    NS_IMETHOD ArcLabelsOut(nsIRDFResource* source,
                            nsISimpleEnumerator** labels) {
        return mInner->ArcLabelsOut(source, labels);
    }

    NS_IMETHOD GetAllResources(nsISimpleEnumerator** aResult) {
        return mInner->GetAllResources(aResult);
    }

    NS_IMETHOD GetAllCmds(nsIRDFResource* source,
                              nsISimpleEnumerator** commands) {
        return mInner->GetAllCmds(source, commands);
    }

    NS_IMETHOD IsCommandEnabled(nsISupportsArray* aSources,
                                nsIRDFResource*   aCommand,
                                nsISupportsArray* aArguments,
                                PRBool* aResult) {
        return mInner->IsCommandEnabled(aSources, aCommand, aArguments, aResult);
    }

    NS_IMETHOD DoCommand(nsISupportsArray* aSources,
                         nsIRDFResource*   aCommand,
                         nsISupportsArray* aArguments) {
        
        
        return mInner->DoCommand(aSources, aCommand, aArguments);
    }

    NS_IMETHOD BeginUpdateBatch() {
        return mInner->BeginUpdateBatch();
    }

    NS_IMETHOD EndUpdateBatch() {
        return mInner->EndUpdateBatch();
    }

    
    NS_DECL_NSIRDFREMOTEDATASOURCE

    
    NS_DECL_NSIRDFXMLSINK

    
    NS_DECL_NSIRDFXMLSOURCE

    
    NS_DECL_NSIREQUESTOBSERVER

    
    NS_DECL_NSISTREAMLISTENER

    
    NS_IMETHOD VisitAllSubjects(rdfITripleVisitor *aVisitor) {
        nsresult rv;
        nsCOMPtr<rdfIDataSource> rdfds = do_QueryInterface(mInner, &rv);
        if (NS_FAILED(rv)) return rv;
        return rdfds->VisitAllSubjects(aVisitor);
    } 

    NS_IMETHOD VisitAllTriples(rdfITripleVisitor *aVisitor) {
        nsresult rv;
        nsCOMPtr<rdfIDataSource> rdfds = do_QueryInterface(mInner, &rv);
        if (NS_FAILED(rv)) return rv;
        return rdfds->VisitAllTriples(aVisitor);
    } 

    
    PRBool
    MakeQName(nsIRDFResource* aResource,
              nsString& property,
              nsString& nameSpacePrefix,
              nsString& nameSpaceURI);

    nsresult
    SerializeAssertion(nsIOutputStream* aStream,
                       nsIRDFResource* aResource,
                       nsIRDFResource* aProperty,
                       nsIRDFNode* aValue);

    nsresult
    SerializeProperty(nsIOutputStream* aStream,
                      nsIRDFResource* aResource,
                      nsIRDFResource* aProperty);

    PRBool
    IsContainerProperty(nsIRDFResource* aProperty);

    nsresult
    SerializeDescription(nsIOutputStream* aStream,
                         nsIRDFResource* aResource);

    nsresult
    SerializeMember(nsIOutputStream* aStream,
                    nsIRDFResource* aContainer,
                    nsIRDFNode* aMember);

    nsresult
    SerializeContainer(nsIOutputStream* aStream,
                       nsIRDFResource* aContainer);

    nsresult
    SerializePrologue(nsIOutputStream* aStream);

    nsresult
    SerializeEpilogue(nsIOutputStream* aStream);

    PRBool
    IsA(nsIRDFDataSource* aDataSource, nsIRDFResource* aResource, nsIRDFResource* aType);

protected:
    nsresult
    BlockingParse(nsIURI* aURL, nsIStreamListener* aConsumer);
};

PRInt32         RDFXMLDataSourceImpl::gRefCnt = 0;
nsIRDFService*  RDFXMLDataSourceImpl::gRDFService;

static const char kFileURIPrefix[] = "file:";
static const char kResourceURIPrefix[] = "resource:";




nsresult
NS_NewRDFXMLDataSource(nsIRDFDataSource** aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    RDFXMLDataSourceImpl* datasource = new RDFXMLDataSourceImpl();
    if (! datasource)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv;
    rv = datasource->Init();

    if (NS_FAILED(rv)) {
        delete datasource;
        return rv;
    }

    NS_ADDREF(datasource);
    *aResult = datasource;
    return NS_OK;
}


RDFXMLDataSourceImpl::RDFXMLDataSourceImpl(void)
    : mIsWritable(PR_TRUE),
      mIsDirty(PR_FALSE),
      mLoadState(eLoadState_Unloaded)
{
#ifdef PR_LOGGING
    if (! gLog)
        gLog = PR_NewLogModule("nsRDFXMLDataSource");
#endif
}


nsresult
RDFXMLDataSourceImpl::Init()
{
    nsresult rv;
    mInner = do_CreateInstance(kRDFInMemoryDataSourceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    if (gRefCnt++ == 0) {
        rv = CallGetService(kRDFServiceCID, &gRDFService);

        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF service");
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}


RDFXMLDataSourceImpl::~RDFXMLDataSourceImpl(void)
{
    nsresult rv;

    
    rv = gRDFService->UnregisterDataSource(this);

    
    rv = Flush();

    
    mObservers.Clear();

    if (--gRefCnt == 0)
        NS_IF_RELEASE(gRDFService);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(RDFXMLDataSourceImpl)
NS_IMPL_CYCLE_COLLECTION_UNLINK_0(RDFXMLDataSourceImpl)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(RDFXMLDataSourceImpl)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mInner)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(RDFXMLDataSourceImpl,
                                          nsIRDFDataSource)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(RDFXMLDataSourceImpl,
                                           nsIRDFDataSource)

NS_INTERFACE_MAP_BEGIN(RDFXMLDataSourceImpl)
    NS_INTERFACE_MAP_ENTRY(nsIRDFDataSource)
    NS_INTERFACE_MAP_ENTRY(nsIRDFRemoteDataSource)
    NS_INTERFACE_MAP_ENTRY(nsIRDFXMLSink)
    NS_INTERFACE_MAP_ENTRY(nsIRDFXMLSource)
    NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
    NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
    NS_INTERFACE_MAP_ENTRY(rdfIDataSource)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIRDFDataSource)
    NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(RDFXMLDataSourceImpl)
NS_INTERFACE_MAP_END


nsresult
RDFXMLDataSourceImpl::BlockingParse(nsIURI* aURL, nsIStreamListener* aConsumer)
{
    nsresult rv;

    
    
    
    
    nsCOMPtr<nsIChannel> channel;
    nsCOMPtr<nsIRequest> request;

    
    rv = NS_NewChannel(getter_AddRefs(channel), aURL, nsnull);
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIInputStream> in;
    PRUint32 sourceOffset = 0;
    rv = channel->Open(getter_AddRefs(in));

    
    if (rv == NS_ERROR_FILE_NOT_FOUND) return NS_OK;
    if (NS_FAILED(rv)) return rv;

    if (! in) {
        NS_ERROR("no input stream");
        return NS_ERROR_FAILURE;
    }

    
    
    nsCOMPtr<nsIInputStream> bufStream;
    rv = NS_NewBufferedInputStream(getter_AddRefs(bufStream), in,
                                   4096 );
    if (NS_FAILED(rv)) return rv;

    
    PRInt32 i;
    for (i = mObservers.Count() - 1; i >= 0; --i) {
        
        
        
        nsCOMPtr<nsIRDFXMLSinkObserver> obs = mObservers[i];

        if (obs) {
            obs->OnBeginLoad(this);
        }
    }

    rv = aConsumer->OnStartRequest(channel, nsnull);

    PRUint32 offset = 0;
    while (NS_SUCCEEDED(rv)) {
        
        channel->GetStatus(&rv);
        if (NS_FAILED(rv))
            break;

        PRUint32 avail;
        if (NS_FAILED(rv = bufStream->Available(&avail)))
            break; 

        if (avail == 0)
            break; 

        rv = aConsumer->OnDataAvailable(channel, nsnull, bufStream, offset, avail);
        if (NS_SUCCEEDED(rv))
            offset += avail;
    }

    if (NS_FAILED(rv))
        channel->Cancel(rv);

    channel->GetStatus(&rv);
    aConsumer->OnStopRequest(channel, nsnull, rv);

    
    for (i = mObservers.Count() - 1; i >= 0; --i) {
        
        
        
        nsCOMPtr<nsIRDFXMLSinkObserver> obs = mObservers[i];

        if (obs) {
            if (NS_FAILED(rv))
                obs->OnError(this, rv, nsnull);

            obs->OnEndLoad(this);
        }
    }

    return rv;
}

NS_IMETHODIMP
RDFXMLDataSourceImpl::GetLoaded(PRBool* _result)
{
    *_result = (mLoadState == eLoadState_Loaded);
    return NS_OK;
}

NS_IMETHODIMP
RDFXMLDataSourceImpl::Init(const char* uri)
{
    NS_PRECONDITION(mInner != nsnull, "not initialized");
    if (! mInner)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv;

    rv = NS_NewURI(getter_AddRefs(mURL), nsDependentCString(uri));
    if (NS_FAILED(rv)) return rv;

    
    
    if ((PL_strncmp(uri, kFileURIPrefix, sizeof(kFileURIPrefix) - 1) != 0) &&
        (PL_strncmp(uri, kResourceURIPrefix, sizeof(kResourceURIPrefix) - 1) != 0)) {
        mIsWritable = PR_FALSE;
    }

    rv = gRDFService->RegisterDataSource(this, PR_FALSE);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}


NS_IMETHODIMP
RDFXMLDataSourceImpl::GetURI(char* *aURI)
{
    *aURI = nsnull;
    if (!mURL) {
        return NS_OK;
    }
    
    nsCAutoString spec;
    mURL->GetSpec(spec);
    *aURI = ToNewCString(spec);
    if (!*aURI) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    
    return NS_OK;
}

NS_IMETHODIMP
RDFXMLDataSourceImpl::Assert(nsIRDFResource* aSource,
                             nsIRDFResource* aProperty,
                             nsIRDFNode* aTarget,
                             PRBool aTruthValue)
{
    
    
    nsresult rv;

    if (IsLoading()) {
        PRBool hasAssertion = PR_FALSE;

        nsCOMPtr<nsIRDFPurgeableDataSource> gcable = do_QueryInterface(mInner);
        if (gcable) {
            rv = gcable->Mark(aSource, aProperty, aTarget, aTruthValue, &hasAssertion);
            if (NS_FAILED(rv)) return rv;
        }

        rv = NS_RDF_ASSERTION_ACCEPTED;

        if (! hasAssertion) {
            rv = mInner->Assert(aSource, aProperty, aTarget, aTruthValue);

            if (NS_SUCCEEDED(rv) && gcable) {
                
                
                
                PRBool didMark;
                (void) gcable->Mark(aSource, aProperty, aTarget, aTruthValue, &didMark);
            }

            if (NS_FAILED(rv)) return rv;
        }

        return rv;
    }
    else if (mIsWritable) {
        rv = mInner->Assert(aSource, aProperty, aTarget, aTruthValue);

        if (rv == NS_RDF_ASSERTION_ACCEPTED)
            mIsDirty = PR_TRUE;

        return rv;
    }
    else {
        return NS_RDF_ASSERTION_REJECTED;
    }
}


NS_IMETHODIMP
RDFXMLDataSourceImpl::Unassert(nsIRDFResource* source,
                               nsIRDFResource* property,
                               nsIRDFNode* target)
{
    
    
    nsresult rv;

    if (IsLoading() || mIsWritable) {
        rv = mInner->Unassert(source, property, target);
        if (!IsLoading() && rv == NS_RDF_ASSERTION_ACCEPTED)
            mIsDirty = PR_TRUE;
    }
    else {
        rv = NS_RDF_ASSERTION_REJECTED;
    }

    return rv;
}

NS_IMETHODIMP
RDFXMLDataSourceImpl::Change(nsIRDFResource* aSource,
                             nsIRDFResource* aProperty,
                             nsIRDFNode* aOldTarget,
                             nsIRDFNode* aNewTarget)
{
    nsresult rv;

    if (IsLoading() || mIsWritable) {
        rv = mInner->Change(aSource, aProperty, aOldTarget, aNewTarget);

        if (!IsLoading() && rv == NS_RDF_ASSERTION_ACCEPTED)
            mIsDirty = PR_TRUE;
    }
    else {
        rv = NS_RDF_ASSERTION_REJECTED;
    }

    return rv;
}

NS_IMETHODIMP
RDFXMLDataSourceImpl::Move(nsIRDFResource* aOldSource,
                           nsIRDFResource* aNewSource,
                           nsIRDFResource* aProperty,
                           nsIRDFNode* aTarget)
{
    nsresult rv;

    if (IsLoading() || mIsWritable) {
        rv = mInner->Move(aOldSource, aNewSource, aProperty, aTarget);
        if (!IsLoading() && rv == NS_RDF_ASSERTION_ACCEPTED)
            mIsDirty = PR_TRUE;
    }
    else {
        rv = NS_RDF_ASSERTION_REJECTED;
    }

    return rv;
}


nsresult
RDFXMLDataSourceImpl::rdfXMLFlush(nsIURI *aURI)
{

    nsresult rv;

    {
        
        
        
        
        nsCOMPtr<nsIRDFService> dummy = do_GetService(kRDFServiceCID, &rv);
        if (NS_FAILED(rv)) {
            NS_WARNING("unable to Flush() diry datasource during XPCOM shutdown");
            return rv;
        }
    }

    
    
    nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(aURI);
    
    if (fileURL) {
        nsCOMPtr<nsIFile> file;
        fileURL->GetFile(getter_AddRefs(file));
        if (file) {
            
            
            nsCOMPtr<nsIOutputStream> out;
            rv = NS_NewSafeLocalFileOutputStream(getter_AddRefs(out),
                                                 file,
                                                 PR_WRONLY | PR_CREATE_FILE,
                                                  0666,
                                                 0);
            if (NS_FAILED(rv)) return rv;

            nsCOMPtr<nsIOutputStream> bufferedOut;
            rv = NS_NewBufferedOutputStream(getter_AddRefs(bufferedOut), out, 4096);
            if (NS_FAILED(rv)) return rv;

            rv = Serialize(bufferedOut);
            if (NS_FAILED(rv)) return rv;
            
            
            
            nsCOMPtr<nsISafeOutputStream> safeStream = do_QueryInterface(bufferedOut, &rv);
            if (NS_FAILED(rv)) return rv;

            rv = safeStream->Finish();
            if (NS_FAILED(rv)) {
                NS_WARNING("failed to save datasource file! possible dataloss");
                return rv;
            }
        }
    }

    return NS_OK;
}


NS_IMETHODIMP
RDFXMLDataSourceImpl::FlushTo(const char *aURI)
{
    NS_PRECONDITION(aURI != nsnull, "not initialized");
    if (!aURI)
        return NS_ERROR_NULL_POINTER;

    
    
    if ((PL_strncmp(aURI, kFileURIPrefix, sizeof(kFileURIPrefix) - 1) != 0) &&
        (PL_strncmp(aURI, kResourceURIPrefix, sizeof(kResourceURIPrefix) - 1) != 0))
    {
        return NS_ERROR_ILLEGAL_VALUE;
    }

    nsCOMPtr<nsIURI>  url;
    nsresult rv = NS_NewURI(getter_AddRefs(url), aURI);
    if (NS_FAILED(rv))
      return rv;
    rv = rdfXMLFlush(url);
    return rv;
}


NS_IMETHODIMP
RDFXMLDataSourceImpl::Flush(void)
{
    if (!mIsWritable || !mIsDirty)
        return NS_OK;

    
    
    if (! mURL)
        return NS_ERROR_NOT_INITIALIZED;

#ifdef PR_LOGGING
    nsCAutoString spec;
    mURL->GetSpec(spec);
    PR_LOG(gLog, PR_LOG_NOTICE,
           ("rdfxml[%p] flush(%s)", this, spec.get()));
#endif

    nsresult rv;
    if (NS_SUCCEEDED(rv = rdfXMLFlush(mURL)))
    {
      mIsDirty = PR_FALSE;
    }
    return rv;
}







NS_IMETHODIMP
RDFXMLDataSourceImpl::GetReadOnly(PRBool* aIsReadOnly)
{
    *aIsReadOnly = !mIsWritable;
    return NS_OK;
}


NS_IMETHODIMP
RDFXMLDataSourceImpl::SetReadOnly(PRBool aIsReadOnly)
{
    if (mIsWritable && aIsReadOnly)
        mIsWritable = PR_FALSE;

    return NS_OK;
}

#include "nsITimelineService.h"

NS_IMETHODIMP
RDFXMLDataSourceImpl::Refresh(PRBool aBlocking)
{
#ifdef PR_LOGGING
    nsCAutoString spec;
    if (mURL) {
        mURL->GetSpec(spec);
    }
    PR_LOG(gLog, PR_LOG_NOTICE,
           ("rdfxml[%p] refresh(%s) %sblocking", this, spec.get(), (aBlocking ? "" : "non")));
#endif
    
    
    
    if (IsLoading()) {
        PR_LOG(gLog, PR_LOG_NOTICE,
               ("rdfxml[%p] refresh(%s) a load was pending", this, spec.get()));

        if (aBlocking) {
            NS_WARNING("blocking load requested when async load pending");
            return NS_ERROR_FAILURE;
        }
        else {
            return NS_OK;
        }
    }

    if (! mURL)
        return NS_ERROR_FAILURE;
    nsCOMPtr<nsIRDFXMLParser> parser = do_CreateInstance("@mozilla.org/rdf/xml-parser;1");
    if (! parser)
        return NS_ERROR_FAILURE;

    nsresult rv = parser->ParseAsync(this, mURL, getter_AddRefs(mListener));
    if (NS_FAILED(rv)) return rv;

    if (aBlocking) {
        NS_TIMELINE_START_TIMER("rdf blocking parse");
        rv = BlockingParse(mURL, this);
        NS_TIMELINE_STOP_TIMER("rdf blocking parse");
        NS_TIMELINE_MARK_TIMER("rdf blocking parse");

        mListener = nsnull; 

        if (NS_FAILED(rv)) return rv;
    }
    else {
        
        rv = NS_OpenURI(this, nsnull, mURL, nsnull);
        if (NS_FAILED(rv)) return rv;

        
        mLoadState = eLoadState_Pending;
    }

    return NS_OK;
}

NS_IMETHODIMP
RDFXMLDataSourceImpl::BeginLoad(void)
{
#ifdef PR_LOGGING
    nsCAutoString spec;
    if (mURL) {
        mURL->GetSpec(spec);
    }
    PR_LOG(gLog, PR_LOG_NOTICE,
           ("rdfxml[%p] begin-load(%s)", this, spec.get()));
#endif
    
    mLoadState = eLoadState_Loading;
    for (PRInt32 i = mObservers.Count() - 1; i >= 0; --i) {
        
        
        
        nsCOMPtr<nsIRDFXMLSinkObserver> obs = mObservers[i];

        if (obs) {
            obs->OnBeginLoad(this);
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
RDFXMLDataSourceImpl::Interrupt(void)
{
#ifdef PR_LOGGING
    nsCAutoString spec;
    if (mURL) {
        mURL->GetSpec(spec);
    }
    PR_LOG(gLog, PR_LOG_NOTICE,
           ("rdfxml[%p] interrupt(%s)", this, spec.get()));
#endif
    
    for (PRInt32 i = mObservers.Count() - 1; i >= 0; --i) {
        
        
        
        nsCOMPtr<nsIRDFXMLSinkObserver> obs = mObservers[i];

        if (obs) {
            obs->OnInterrupt(this);
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
RDFXMLDataSourceImpl::Resume(void)
{
#ifdef PR_LOGGING
    nsCAutoString spec;
    if (mURL) {
        mURL->GetSpec(spec);
    }
    PR_LOG(gLog, PR_LOG_NOTICE,
           ("rdfxml[%p] resume(%s)", this, spec.get()));
#endif
    
    for (PRInt32 i = mObservers.Count() - 1; i >= 0; --i) {
        
        
        
        nsCOMPtr<nsIRDFXMLSinkObserver> obs = mObservers[i];

        if (obs) {
            obs->OnResume(this);
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
RDFXMLDataSourceImpl::EndLoad(void)
{
#ifdef PR_LOGGING
    nsCAutoString spec;
    if (mURL) {
        mURL->GetSpec(spec);
    }
    PR_LOG(gLog, PR_LOG_NOTICE,
           ("rdfxml[%p] end-load(%s)", this, spec.get()));
#endif
    
    mLoadState = eLoadState_Loaded;

    
    nsCOMPtr<nsIRDFPurgeableDataSource> gcable = do_QueryInterface(mInner);
    if (gcable) {
        gcable->Sweep();
    }

    
    for (PRInt32 i = mObservers.Count() - 1; i >= 0; --i) {
        
        
        
        nsCOMPtr<nsIRDFXMLSinkObserver> obs = mObservers[i];

        if (obs) {
            obs->OnEndLoad(this);
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
RDFXMLDataSourceImpl::AddNameSpace(nsIAtom* aPrefix, const nsString& aURI)
{
    mNameSpaces.Put(aURI, aPrefix);
    return NS_OK;
}


NS_IMETHODIMP
RDFXMLDataSourceImpl::AddXMLSinkObserver(nsIRDFXMLSinkObserver* aObserver)
{
    if (! aObserver)
        return NS_ERROR_NULL_POINTER;

    mObservers.AppendObject(aObserver);
    return NS_OK;
}

NS_IMETHODIMP
RDFXMLDataSourceImpl::RemoveXMLSinkObserver(nsIRDFXMLSinkObserver* aObserver)
{
    if (! aObserver)
        return NS_ERROR_NULL_POINTER;

    mObservers.RemoveObject(aObserver);

    return NS_OK;
}







NS_IMETHODIMP
RDFXMLDataSourceImpl::OnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
    return mListener->OnStartRequest(request, ctxt);
}

NS_IMETHODIMP
RDFXMLDataSourceImpl::OnStopRequest(nsIRequest *request,
                                    nsISupports *ctxt,
                                    nsresult status)
{
    if (NS_FAILED(status)) {
        for (PRInt32 i = mObservers.Count() - 1; i >= 0; --i) {
            
            
            
            nsCOMPtr<nsIRDFXMLSinkObserver> obs = mObservers[i];

            if (obs) {
                obs->OnError(this, status, nsnull);
            }
        }
    }

    nsresult rv;
    rv = mListener->OnStopRequest(request, ctxt, status);

    mListener = nsnull; 

    return rv;
}






NS_IMETHODIMP
RDFXMLDataSourceImpl::OnDataAvailable(nsIRequest *request,
                                      nsISupports *ctxt,
                                      nsIInputStream *inStr,
                                      PRUint32 sourceOffset,
                                      PRUint32 count)
{
    return mListener->OnDataAvailable(request, ctxt, inStr, sourceOffset, count);
}






NS_IMETHODIMP
RDFXMLDataSourceImpl::Serialize(nsIOutputStream* aStream)
{
    nsresult rv;
    nsCOMPtr<nsIRDFXMLSerializer> serializer
        = do_CreateInstance("@mozilla.org/rdf/xml-serializer;1", &rv);

    if (! serializer)
        return rv;

    rv = serializer->Init(this);
    if (NS_FAILED(rv)) return rv;

    
    
    nsNameSpaceMap::const_iterator last = mNameSpaces.last();
    for (nsNameSpaceMap::const_iterator iter = mNameSpaces.first();
         iter != last; ++iter) {
        
        
        NS_ConvertUTF8toUTF16 uri(iter->mURI);
        serializer->AddNameSpace(iter->mPrefix, uri);
    }

    
    nsCOMPtr<nsIRDFXMLSource> source = do_QueryInterface(serializer);
    if (! source)
        return NS_ERROR_FAILURE;

    return source->Serialize(aStream);
}
