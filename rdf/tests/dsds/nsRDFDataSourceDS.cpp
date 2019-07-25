






































#include "nsCRT.h"
#include "nsCOMPtr.h"

#include "nsIRDFService.h"
#include "nsRDFCID.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFRemoteDataSource.h"
#include "rdf.h"

#include "nsIServiceManager.h"

#include "nsEnumeratorUtils.h"

#include "nsXPIDLString.h"

#ifdef NS_DEBUG
#include <stdio.h>
#endif
























#define NS_RDF_DSDS_NAMESPACE_URI "http://www.mozilla.org/rdf/vocab/dsds#"
#define NS_RDF_ARCSOUT NS_RDF_DSDS_NAMESPACE_URI "arcsout"
#define NS_RDF_ARCSIN NS_RDF_DSDS_NAMESPACE_URI "arcsin"
#define NS_RDF_SUBJECT NS_RDF_DSDS_NAMESPACE_URI "subject"
#define NS_RDF_PREDICATE NS_RDF_DSDS_NAMESPACE_URI "predicate"
#define NS_RDF_OBJECT NS_RDF_DSDS_NAMESPACE_URI "object"

#define NC_RDF_Name  NC_NAMESPACE_URI "Name"
#define NC_RDF_Value NC_NAMESPACE_URI "Value"
#define NC_RDF_Child NC_NAMESPACE_URI "child"

static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);

class nsRDFDataSourceDataSource :
    public nsIRDFDataSource,
    public nsIRDFRemoteDataSource {
public: 
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRDFDATASOURCE
  NS_DECL_NSIRDFREMOTEDATASOURCE

  nsRDFDataSourceDataSource();
  virtual ~nsRDFDataSourceDataSource();
  
private:
  nsCString mURI;
  nsCOMPtr<nsIRDFDataSource> mDataSource;

  static nsIRDFResource* kNC_Name;
  static nsIRDFResource* kNC_Value;
  static nsIRDFResource* kNC_Child;
  
};

nsIRDFResource* nsRDFDataSourceDataSource::kNC_Name=nsnull;
nsIRDFResource* nsRDFDataSourceDataSource::kNC_Value=nsnull;
nsIRDFResource* nsRDFDataSourceDataSource::kNC_Child=nsnull;


nsRDFDataSourceDataSource::nsRDFDataSourceDataSource()
{
}

nsRDFDataSourceDataSource::~nsRDFDataSourceDataSource()
{
}


NS_IMPL_ISUPPORTS2(nsRDFDataSourceDataSource,
                   nsIRDFDataSource,
                   nsIRDFRemoteDataSource)






NS_IMETHODIMP nsRDFDataSourceDataSource::GetLoaded(PRBool *aLoaded)
{
    nsCOMPtr<nsIRDFRemoteDataSource> remote =
        do_QueryInterface(mDataSource);
    if (remote) {
        return remote->GetLoaded(aLoaded);
    }
    *aLoaded = PR_TRUE;
    return NS_OK;
}


NS_IMETHODIMP
nsRDFDataSourceDataSource::Init(const char *uri)
{
  nsresult rv;

  mURI = uri;

  
  NS_NAMED_LITERAL_CSTRING(prefix, "rdf:datasource");
  nsCAutoString mInnerURI;
  mInnerURI = Substring(mURI, prefix.Length() + 1);
  
  if (mInnerURI.IsEmpty() || mInnerURI == prefix) {
      mURI.Truncate();
      return NS_ERROR_INVALID_ARG;
  }
  nsCOMPtr<nsIRDFService> rdf(do_GetService(kRDFServiceCID, &rv));
  rv = rdf->GetDataSource(mInnerURI.get(), getter_AddRefs(mDataSource));
  if (NS_FAILED(rv)) {
      mURI.Truncate();
      NS_WARNING("Could not get inner datasource");
      return rv;
  }

  
  
  if (!kNC_Name) {
    rdf->GetResource(NS_LITERAL_CSTRING(NC_RDF_Name), &kNC_Name);
    rdf->GetResource(NS_LITERAL_CSTRING(NC_RDF_Child), &kNC_Child);
    rdf->GetResource(NS_LITERAL_CSTRING(NC_RDF_Value), &kNC_Value);
  }

#ifdef DEBUG_alecf
  printf("nsRDFDataSourceDataSource::Init(%s)\n", uri);
#endif
  
  return NS_OK;
}


NS_IMETHODIMP nsRDFDataSourceDataSource::Refresh(PRBool aBlocking)
{
    nsCOMPtr<nsIRDFRemoteDataSource> remote =
        do_QueryInterface(mDataSource);
    if (remote) {
        return remote->Refresh(aBlocking);
    }
    return NS_OK;
}


NS_IMETHODIMP nsRDFDataSourceDataSource::Flush()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsRDFDataSourceDataSource::FlushTo(const char *aURI)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}






NS_IMETHODIMP
nsRDFDataSourceDataSource::GetURI(char * *aURI)
{
#ifdef DEBUG_alecf
  printf("nsRDFDataSourceDataSource::GetURI()\n");
#endif
  *aURI = ToNewCString(mURI);
  
  return NS_OK;
}


NS_IMETHODIMP
nsRDFDataSourceDataSource::GetSource(nsIRDFResource *aProperty,
                                     nsIRDFNode *aTarget,
                                     PRBool aTruthValue,
                                     nsIRDFResource **_retval)
{
  return NS_RDF_NO_VALUE;
}


NS_IMETHODIMP
nsRDFDataSourceDataSource::GetSources(nsIRDFResource *aProperty,
                                      nsIRDFNode *aTarget,
                                      PRBool aTruthValue,
                                      nsISimpleEnumerator **_retval)
{
  return NS_RDF_NO_VALUE;
}


NS_IMETHODIMP
nsRDFDataSourceDataSource::GetTarget(nsIRDFResource *aSource,
                                     nsIRDFResource *aProperty,
                                     PRBool aTruthValue,
                                     nsIRDFNode **_retval)
{
#ifdef DEBUG_alecf
  nsXPIDLCString sourceval;
  nsXPIDLCString propval;
  aSource->GetValue(getter_Copies(sourceval));
  aProperty->GetValue(getter_Copies(propval));
  printf("GetTarget(%s, %s,..)\n", (const char*)sourceval,
         (const char*)propval);
#endif

  return NS_OK;
}


NS_IMETHODIMP
nsRDFDataSourceDataSource::GetTargets(nsIRDFResource *aSource,
                                      nsIRDFResource *aProperty,
                                      PRBool aTruthValue,
                                      nsISimpleEnumerator **_retval)
{
  nsXPIDLCString sourceval;
  aSource->GetValue(getter_Copies(sourceval));
  nsXPIDLCString propval;
  aProperty->GetValue(getter_Copies(propval));
#ifdef DEBUG_alecf
  printf("GetTargets(%s, %s,..)\n", (const char*)sourceval,
         (const char*)propval);
#endif
  
  nsresult rv;
  PRBool isProp;
  nsCOMPtr<nsISupportsArray> arcs;
  nsISimpleEnumerator *enumerator;
  
  if (NS_SUCCEEDED(aProperty->EqualsNode(kNC_Child, &isProp)) &&
      isProp) {

    
    
    if (StringBeginsWith(sourceval, NS_LITERAL_CSTRING("dsresource:"))) {
      
      
      rv = mDataSource->ArcLabelsOut(aSource, &enumerator);      
    } else {
      rv = mDataSource->ArcLabelsOut(aSource, &enumerator);
    }
    
    PRBool hasMoreArcs=PR_FALSE;

    rv = enumerator->HasMoreElements(&hasMoreArcs);
    while (NS_SUCCEEDED(rv) && hasMoreArcs) {
      
      
      nsCOMPtr<nsISupports> arcSupports;
      rv = enumerator->GetNext(getter_AddRefs(arcSupports));
      nsCOMPtr<nsIRDFResource> arc = do_QueryInterface(arcSupports, &rv);

      
      nsCOMPtr<nsISimpleEnumerator> targetEnumerator;
      rv = mDataSource->GetTargets(aSource, arc, PR_TRUE,
                                   getter_AddRefs(targetEnumerator));

      PRBool hasMoreTargets;
      rv = targetEnumerator->HasMoreElements(&hasMoreTargets);
      while (NS_SUCCEEDED(rv) && hasMoreTargets) {
        
        nsCOMPtr<nsISupports> targetSupports;
        rv = enumerator->GetNext(getter_AddRefs(targetSupports));
        nsCOMPtr<nsIRDFResource> target=do_QueryInterface(targetSupports, &rv);

        
        
        
#ifdef DEBUG_alecf
        nsXPIDLString arcValue;
        nsXPIDLString targetValue;

        arc->GetValue(getter_Copies(arcValue));
        target->GetValue(getter_Copies(targetValue));
        printf("#child of %s:\n\t%s = %s\n",
               (const char*)sourceval
#endif

      }
      
      rv = enumerator->HasMoreElements(&hasMoreArcs);
    }
    
  } else if (NS_SUCCEEDED(aProperty->EqualsNode(kNC_Name, &isProp)) &&
             isProp) {
    if (StringBeginsWith(sourceval, NS_LITERAL_CSTRING("dsresource:"))) {
      

    }
    
  } else if (NS_SUCCEEDED(aProperty->EqualsNode(kNC_Value, &isProp)) &&
             isProp) {


  } else {
    rv = NS_NewISupportsArray(getter_AddRefs(arcs));
    if (NS_FAILED(rv)) return rv;
    
    nsArrayEnumerator* cursor =
      new nsArrayEnumerator(arcs);

    if (!cursor) return NS_ERROR_OUT_OF_MEMORY;
    
    *_retval = cursor;
    NS_ADDREF(*_retval);
  }
  
  return NS_OK;
}


NS_IMETHODIMP
nsRDFDataSourceDataSource::Assert(nsIRDFResource *aSource, nsIRDFResource *aProperty, nsIRDFNode *aTarget, PRBool aTruthValue)
{
  return NS_RDF_NO_VALUE;
}


NS_IMETHODIMP
nsRDFDataSourceDataSource::Unassert(nsIRDFResource *aSource, nsIRDFResource *aProperty, nsIRDFNode *aTarget)
{
  return NS_RDF_NO_VALUE;
}


NS_IMETHODIMP
nsRDFDataSourceDataSource::HasAssertion(nsIRDFResource *aSource, nsIRDFResource *aProperty, nsIRDFNode *aTarget, PRBool aTruthValue, PRBool *_retval)
{
  return NS_RDF_NO_VALUE;
}


NS_IMETHODIMP
nsRDFDataSourceDataSource::AddObserver(nsIRDFObserver *aObserver)
{
  return NS_RDF_NO_VALUE;
}


NS_IMETHODIMP
nsRDFDataSourceDataSource::RemoveObserver(nsIRDFObserver *aObserver)
{
  return NS_RDF_NO_VALUE;
}


NS_IMETHODIMP
nsRDFDataSourceDataSource::ArcLabelsIn(nsIRDFNode *aNode, nsISimpleEnumerator **_retval)
{
  return NS_RDF_NO_VALUE;
}


NS_IMETHODIMP
nsRDFDataSourceDataSource::ArcLabelsOut(nsIRDFResource *aSource,
                                        nsISimpleEnumerator **_retval)
{
  nsresult rv=NS_OK;
  
  nsCOMPtr<nsISupportsArray> arcs;
  rv = NS_NewISupportsArray(getter_AddRefs(arcs));
  
  if (NS_FAILED(rv)) return rv;
  nsXPIDLCString sourceval;
  aSource->GetValue(getter_Copies(sourceval));
  
#ifdef DEBUG_alecf
  printf("ArcLabelsOut(%s)\n", (const char*)sourceval);
#endif
  
  arcs->AppendElement(kNC_Name);
  arcs->AppendElement(kNC_Value);
  arcs->AppendElement(kNC_Child);

  nsArrayEnumerator* cursor =
    new nsArrayEnumerator(arcs);

  if (!cursor) return NS_ERROR_OUT_OF_MEMORY;
  
  NS_ADDREF(cursor);
  *_retval = cursor;

  return NS_OK;
}


NS_IMETHODIMP
nsRDFDataSourceDataSource::GetAllResources(nsISimpleEnumerator **_retval)
{
  return NS_RDF_NO_VALUE;
}


NS_IMETHODIMP
nsRDFDataSourceDataSource::IsCommandEnabled(nsISupportsArray * aSources, nsIRDFResource *aCommand, nsISupportsArray * aArguments, PRBool *_retval)
{
  return NS_RDF_NO_VALUE;
}


NS_IMETHODIMP
nsRDFDataSourceDataSource::DoCommand(nsISupportsArray * aSources, nsIRDFResource *aCommand, nsISupportsArray * aArguments)
{
  return NS_RDF_NO_VALUE;
}


NS_IMETHODIMP
nsRDFDataSourceDataSource::BeginUpdateBatch()
{
  return NS_OK;
}


NS_IMETHODIMP
nsRDFDataSourceDataSource::EndUpdateBatch()
{
  return NS_OK;
}

nsresult
NS_NewRDFDataSourceDataSource(nsISupports *, const nsIID& iid,
                              void ** result)

{
  nsRDFDataSourceDataSource * dsds = new nsRDFDataSourceDataSource();
  if (!dsds) return NS_ERROR_NOT_INITIALIZED;
  return dsds->QueryInterface(iid, result);
  
}
