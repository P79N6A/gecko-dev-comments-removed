












































#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIRDFContainer.h"
#include "nsIRDFContainerUtils.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "plstr.h"
#include "prprf.h"
#include "rdf.h"
#include "rdfutil.h"

static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);
static const char kRDFNameSpaceURI[] = RDF_NAMESPACE_URI;

class RDFContainerUtilsImpl : public nsIRDFContainerUtils
{
public:
    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIRDFCONTAINERUTILS

private:
    friend nsresult NS_NewRDFContainerUtils(nsIRDFContainerUtils** aResult);

    RDFContainerUtilsImpl();
    virtual ~RDFContainerUtilsImpl();

    nsresult MakeContainer(nsIRDFDataSource* aDataSource,
                           nsIRDFResource* aResource,
                           nsIRDFResource* aType,
                           nsIRDFContainer** aResult);

    PRBool IsA(nsIRDFDataSource* aDataSource, nsIRDFResource* aResource, nsIRDFResource* aType);

    
    static PRInt32 gRefCnt;
    static nsIRDFService* gRDFService;
    static nsIRDFResource* kRDF_instanceOf;
    static nsIRDFResource* kRDF_nextVal;
    static nsIRDFResource* kRDF_Bag;
    static nsIRDFResource* kRDF_Seq;
    static nsIRDFResource* kRDF_Alt;
    static nsIRDFLiteral* kOne;
};


PRInt32         RDFContainerUtilsImpl::gRefCnt = 0;
nsIRDFService*  RDFContainerUtilsImpl::gRDFService;
nsIRDFResource* RDFContainerUtilsImpl::kRDF_instanceOf;
nsIRDFResource* RDFContainerUtilsImpl::kRDF_nextVal;
nsIRDFResource* RDFContainerUtilsImpl::kRDF_Bag;
nsIRDFResource* RDFContainerUtilsImpl::kRDF_Seq;
nsIRDFResource* RDFContainerUtilsImpl::kRDF_Alt;
nsIRDFLiteral*  RDFContainerUtilsImpl::kOne;




NS_IMPL_THREADSAFE_ISUPPORTS1(RDFContainerUtilsImpl, nsIRDFContainerUtils)




NS_IMETHODIMP
RDFContainerUtilsImpl::IsOrdinalProperty(nsIRDFResource *aProperty, PRBool *_retval)
{
    NS_PRECONDITION(aProperty != nsnull, "null ptr");
    if (! aProperty)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    const char	*propertyStr;
    rv = aProperty->GetValueConst( &propertyStr );
    if (NS_FAILED(rv)) return rv;

    if (PL_strncmp(propertyStr, kRDFNameSpaceURI, sizeof(kRDFNameSpaceURI) - 1) != 0) {
        *_retval = PR_FALSE;
        return NS_OK;
    }

    const char* s = propertyStr;
    s += sizeof(kRDFNameSpaceURI) - 1;
    if (*s != '_') {
        *_retval = PR_FALSE;
        return NS_OK;
    }

    ++s;
    while (*s) {
        if (*s < '0' || *s > '9') {
            *_retval = PR_FALSE;
            return NS_OK;
        }

        ++s;
    }

    *_retval = PR_TRUE;
    return NS_OK;
}


NS_IMETHODIMP
RDFContainerUtilsImpl::IndexToOrdinalResource(PRInt32 aIndex, nsIRDFResource **aOrdinal)
{
    NS_PRECONDITION(aIndex > 0, "illegal value");
    if (aIndex <= 0)
        return NS_ERROR_ILLEGAL_VALUE;

    nsCAutoString uri(kRDFNameSpaceURI);
    uri.Append('_');
    uri.AppendInt(aIndex);
    
    nsresult rv = gRDFService->GetResource(uri, aOrdinal);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get ordinal resource");
    if (NS_FAILED(rv)) return rv;
    
    return NS_OK;
}


NS_IMETHODIMP
RDFContainerUtilsImpl::OrdinalResourceToIndex(nsIRDFResource *aOrdinal, PRInt32 *aIndex)
{
    NS_PRECONDITION(aOrdinal != nsnull, "null ptr");
    if (! aOrdinal)
        return NS_ERROR_NULL_POINTER;

    const char	*ordinalStr;
    if (NS_FAILED(aOrdinal->GetValueConst( &ordinalStr )))
        return NS_ERROR_FAILURE;

    const char* s = ordinalStr;
    if (PL_strncmp(s, kRDFNameSpaceURI, sizeof(kRDFNameSpaceURI) - 1) != 0) {
        NS_ERROR("not an ordinal");
        return NS_ERROR_UNEXPECTED;
    }

    s += sizeof(kRDFNameSpaceURI) - 1;
    if (*s != '_') {
        NS_ERROR("not an ordinal");
        return NS_ERROR_UNEXPECTED;
    }

    PRInt32 idx = 0;

    ++s;
    while (*s) {
        if (*s < '0' || *s > '9') {
            NS_ERROR("not an ordinal");
            return NS_ERROR_UNEXPECTED;
        }

        idx *= 10;
        idx += (*s - '0');

        ++s;
    }

    *aIndex = idx;
    return NS_OK;
}

NS_IMETHODIMP
RDFContainerUtilsImpl::IsContainer(nsIRDFDataSource *aDataSource, nsIRDFResource *aResource, PRBool *_retval)
{
    NS_PRECONDITION(aDataSource != nsnull, "null ptr");
    if (! aDataSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aResource != nsnull, "null ptr");
    if (! aResource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (! _retval)
        return NS_ERROR_NULL_POINTER;

    if (IsA(aDataSource, aResource, kRDF_Seq) ||
        IsA(aDataSource, aResource, kRDF_Bag) ||
        IsA(aDataSource, aResource, kRDF_Alt)) {
        *_retval = PR_TRUE;
    }
    else {
        *_retval = PR_FALSE;
    }
    return NS_OK;
}


NS_IMETHODIMP
RDFContainerUtilsImpl::IsEmpty(nsIRDFDataSource* aDataSource, nsIRDFResource* aResource, PRBool* _retval)
{
    if (! aDataSource)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    
    
    *_retval = PR_TRUE;

    nsCOMPtr<nsIRDFNode> nextValNode;
    rv = aDataSource->GetTarget(aResource, kRDF_nextVal, PR_TRUE, getter_AddRefs(nextValNode));
    if (NS_FAILED(rv)) return rv;

    if (rv == NS_RDF_NO_VALUE)
        return NS_OK;

    nsCOMPtr<nsIRDFLiteral> nextValLiteral;
    rv = nextValNode->QueryInterface(NS_GET_IID(nsIRDFLiteral), getter_AddRefs(nextValLiteral));
    if (NS_FAILED(rv)) return rv;

    if (nextValLiteral.get() != kOne)
        *_retval = PR_FALSE;

    return NS_OK;
}


NS_IMETHODIMP
RDFContainerUtilsImpl::IsBag(nsIRDFDataSource *aDataSource, nsIRDFResource *aResource, PRBool *_retval)
{
    NS_PRECONDITION(aDataSource != nsnull, "null ptr");
    if (! aDataSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aResource != nsnull, "null ptr");
    if (! aResource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (! _retval)
        return NS_ERROR_NULL_POINTER;

    *_retval = IsA(aDataSource, aResource, kRDF_Bag);
    return NS_OK;
}


NS_IMETHODIMP
RDFContainerUtilsImpl::IsSeq(nsIRDFDataSource *aDataSource, nsIRDFResource *aResource, PRBool *_retval)
{
    NS_PRECONDITION(aDataSource != nsnull, "null ptr");
    if (! aDataSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aResource != nsnull, "null ptr");
    if (! aResource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (! _retval)
        return NS_ERROR_NULL_POINTER;

    *_retval = IsA(aDataSource, aResource, kRDF_Seq);
    return NS_OK;
}


NS_IMETHODIMP
RDFContainerUtilsImpl::IsAlt(nsIRDFDataSource *aDataSource, nsIRDFResource *aResource, PRBool *_retval)
{
    NS_PRECONDITION(aDataSource != nsnull, "null ptr");
    if (! aDataSource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aResource != nsnull, "null ptr");
    if (! aResource)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(_retval != nsnull, "null ptr");
    if (! _retval)
        return NS_ERROR_NULL_POINTER;

    *_retval = IsA(aDataSource, aResource, kRDF_Alt);
    return NS_OK;
}


NS_IMETHODIMP
RDFContainerUtilsImpl::MakeBag(nsIRDFDataSource *aDataSource, nsIRDFResource *aResource, nsIRDFContainer **_retval)
{
    return MakeContainer(aDataSource, aResource, kRDF_Bag, _retval);
}


NS_IMETHODIMP
RDFContainerUtilsImpl::MakeSeq(nsIRDFDataSource *aDataSource, nsIRDFResource *aResource, nsIRDFContainer **_retval)
{
    return MakeContainer(aDataSource, aResource, kRDF_Seq, _retval);
}


NS_IMETHODIMP
RDFContainerUtilsImpl::MakeAlt(nsIRDFDataSource *aDataSource, nsIRDFResource *aResource, nsIRDFContainer **_retval)
{
    return MakeContainer(aDataSource, aResource, kRDF_Alt, _retval);
}






RDFContainerUtilsImpl::RDFContainerUtilsImpl()
{
    if (gRefCnt++ == 0) {
        nsresult rv;

        rv = CallGetService(kRDFServiceCID, &gRDFService);

        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF service");
        if (NS_SUCCEEDED(rv)) {
            gRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "instanceOf"),
                                     &kRDF_instanceOf);
            gRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "nextVal"),
                                     &kRDF_nextVal);
            gRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "Bag"),
                                     &kRDF_Bag);
            gRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "Seq"),
                                     &kRDF_Seq);
            gRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "Alt"),
                                     &kRDF_Alt);
            gRDFService->GetLiteral(NS_LITERAL_STRING("1").get(), &kOne);
        }
    }
}


RDFContainerUtilsImpl::~RDFContainerUtilsImpl()
{
#ifdef DEBUG_REFS
    --gInstanceCount;
    fprintf(stdout, "%d - RDF: RDFContainerUtilsImpl\n", gInstanceCount);
#endif

    if (--gRefCnt == 0) {
        NS_IF_RELEASE(gRDFService);
        NS_IF_RELEASE(kRDF_instanceOf);
        NS_IF_RELEASE(kRDF_nextVal);
        NS_IF_RELEASE(kRDF_Bag);
        NS_IF_RELEASE(kRDF_Seq);
        NS_IF_RELEASE(kRDF_Alt);
        NS_IF_RELEASE(kOne);
    }
}



nsresult
NS_NewRDFContainerUtils(nsIRDFContainerUtils** aResult)
{
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    RDFContainerUtilsImpl* result =
        new RDFContainerUtilsImpl();

    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(result);
    *aResult = result;
    return NS_OK;
}


nsresult
RDFContainerUtilsImpl::MakeContainer(nsIRDFDataSource* aDataSource, nsIRDFResource* aResource, nsIRDFResource* aType, nsIRDFContainer** aResult)
{
    NS_PRECONDITION(aDataSource != nsnull, "null ptr");
    if (! aDataSource)	return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aResource != nsnull, "null ptr");
    if (! aResource)	return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aType != nsnull, "null ptr");
    if (! aType)	return NS_ERROR_NULL_POINTER;

    if (aResult)	*aResult = nsnull;

    nsresult rv;

    
    
    PRBool isContainer;
    rv = IsContainer(aDataSource, aResource, &isContainer);
    if (NS_FAILED(rv)) return rv;

    if (isContainer == PR_FALSE)
    {
	rv = aDataSource->Assert(aResource, kRDF_instanceOf, aType, PR_TRUE);
	if (NS_FAILED(rv)) return rv;

	rv = aDataSource->Assert(aResource, kRDF_nextVal, kOne, PR_TRUE);
	if (NS_FAILED(rv)) return rv;
    }

    if (aResult) {
        rv = NS_NewRDFContainer(aResult);
        if (NS_FAILED(rv)) return rv;

        rv = (*aResult)->Init(aDataSource, aResource);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}

PRBool
RDFContainerUtilsImpl::IsA(nsIRDFDataSource* aDataSource, nsIRDFResource* aResource, nsIRDFResource* aType)
{
    if (!aDataSource || !aResource || !aType)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    PRBool result;
    rv = aDataSource->HasAssertion(aResource, kRDF_instanceOf, aType, PR_TRUE, &result);
    if (NS_FAILED(rv)) return PR_FALSE;

    return result;
}

NS_IMETHODIMP
RDFContainerUtilsImpl::IndexOf(nsIRDFDataSource* aDataSource, nsIRDFResource* aContainer, nsIRDFNode* aElement, PRInt32* aIndex)
{
    if (!aDataSource || !aContainer)
        return NS_ERROR_NULL_POINTER;

    
    *aIndex = -1;

    
    if (! aElement)
      return NS_OK;

    
    
    
    nsCOMPtr<nsISimpleEnumerator> arcsIn;
    aDataSource->ArcLabelsIn(aElement, getter_AddRefs(arcsIn));
    if (! arcsIn)
        return NS_OK;

    while (1) {
        PRBool hasMoreArcs = PR_FALSE;
        arcsIn->HasMoreElements(&hasMoreArcs);
        if (! hasMoreArcs)
            break;

        nsCOMPtr<nsISupports> isupports;
        arcsIn->GetNext(getter_AddRefs(isupports));
        if (! isupports)
            break;

        nsCOMPtr<nsIRDFResource> property =
            do_QueryInterface(isupports);

        if (! property)
            continue;

        PRBool isOrdinal;
        IsOrdinalProperty(property, &isOrdinal);
        if (! isOrdinal)
            continue;

        nsCOMPtr<nsISimpleEnumerator> sources;
        aDataSource->GetSources(property, aElement, PR_TRUE, getter_AddRefs(sources));
        if (! sources)
            continue;

        while (1) {
            PRBool hasMoreSources = PR_FALSE;
            sources->HasMoreElements(&hasMoreSources);
            if (! hasMoreSources)
                break;

            nsCOMPtr<nsISupports> isupports2;
            sources->GetNext(getter_AddRefs(isupports2));
            if (! isupports2)
                break;

            nsCOMPtr<nsIRDFResource> source =
                do_QueryInterface(isupports2);

            if (source == aContainer)
                
                return OrdinalResourceToIndex(property, aIndex);
        }
    }

    return NS_OK;
}
