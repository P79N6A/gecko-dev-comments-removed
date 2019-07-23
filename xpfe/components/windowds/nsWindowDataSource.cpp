




































#include "nsWindowDataSource.h"
#include "nsIXULWindow.h"
#include "rdf.h"
#include "nsIRDFContainerUtils.h"
#include "nsIServiceManager.h"
#include "nsReadableUtils.h"
#include "nsIObserverService.h"
#include "nsIWindowMediator.h"
#include "nsXPCOMCID.h"
#include "nsICategoryManager.h"
#include "nsIGenericFactory.h"


#include "nsIInterfaceRequestorUtils.h"
#include "nsIDocShell.h"

PRUint32 nsWindowDataSource::windowCount = 0;

nsIRDFResource* nsWindowDataSource::kNC_Name = nsnull;
nsIRDFResource* nsWindowDataSource::kNC_WindowRoot = nsnull;
nsIRDFResource* nsWindowDataSource::kNC_KeyIndex = nsnull;

nsIRDFService*  nsWindowDataSource::gRDFService = nsnull;

PRUint32 nsWindowDataSource::gRefCnt = 0;

static const char kURINC_WindowRoot[] = "NC:WindowMediatorRoot";

DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Name);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, KeyIndex);

nsresult
nsWindowDataSource::Init()
{
    nsresult rv;

    if (gRefCnt++ == 0) {
        rv = CallGetService("@mozilla.org/rdf/rdf-service;1", &gRDFService);
        if (NS_FAILED(rv)) return rv;

        gRDFService->GetResource(NS_LITERAL_CSTRING(kURINC_WindowRoot), &kNC_WindowRoot);
        gRDFService->GetResource(NS_LITERAL_CSTRING(kURINC_Name),       &kNC_Name);
        gRDFService->GetResource(NS_LITERAL_CSTRING(kURINC_KeyIndex),   &kNC_KeyIndex);
    }

    mInner = do_CreateInstance("@mozilla.org/rdf/datasource;1?name=in-memory-datasource", &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIRDFContainerUtils> rdfc =
        do_GetService("@mozilla.org/rdf/container-utils;1", &rv);
    if (NS_FAILED(rv)) return rv;

    rv = rdfc->MakeSeq(this, kNC_WindowRoot, getter_AddRefs(mContainer));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIWindowMediator> windowMediator =
        do_GetService(NS_WINDOWMEDIATOR_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = windowMediator->AddListener(this);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIObserverService> observerService =
        do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                          PR_FALSE);
    }
    return NS_OK;
}

nsWindowDataSource::~nsWindowDataSource()
{
    if (--gRefCnt == 0) {
        NS_IF_RELEASE(kNC_Name);
        NS_IF_RELEASE(kNC_KeyIndex);
        NS_IF_RELEASE(kNC_WindowRoot);
        NS_IF_RELEASE(gRDFService);
    }
}

NS_IMETHODIMP
nsWindowDataSource::Observe(nsISupports *aSubject, const char* aTopic, const PRUnichar *aData)
{
    if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
        
        
        mContainer = nsnull;
        mInner = nsnull;
    }

    return NS_OK;
}

#if 0
NS_IMETHODIMP_(nsrefcnt)
nsWindowMediator::Release()
{
	
	

	NS_PRECONDITION(PRInt32(mRefCnt) > 0, "duplicate release");
	--mRefCnt;
	NS_LOG_RELEASE(this, mRefCnt, "nsWindowMediator");

	if (mInner && mRefCnt == 2)
	{
		NS_IF_RELEASE(mContainer);
		mContainer = nsnull;

		nsIRDFDataSource* tmp = mInner;
		mInner = nsnull;
		NS_IF_RELEASE(tmp);
		return(0);
	}
	else if (mRefCnt == 0)
	{
		mRefCnt = 1;
		delete this;
		return(0);
	}
	return(mRefCnt);
}

#endif


NS_IMPL_ISUPPORTS4(nsWindowDataSource,
                   nsIObserver,
                   nsIWindowMediatorListener,
                   nsIWindowDataSource,
                   nsIRDFDataSource)






NS_IMETHODIMP
nsWindowDataSource::OnWindowTitleChange(nsIXULWindow *window,
                                        const PRUnichar *newTitle)
{
    nsresult rv;
    
    nsVoidKey key(window);

    nsCOMPtr<nsISupports> sup =
        dont_AddRef(mWindowResources.Get(&key));

    
    if (!sup) {
        OnOpenWindow(window);
        sup = dont_AddRef(mWindowResources.Get(&key));
    }
    
    NS_ENSURE_TRUE(sup, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIRDFResource> windowResource =
        do_QueryInterface(sup);

    nsCOMPtr<nsIRDFLiteral> newTitleLiteral;
    rv = gRDFService->GetLiteral(newTitle, getter_AddRefs(newTitleLiteral));
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIRDFNode> oldTitleNode;
    rv = GetTarget(windowResource, kNC_Name, PR_TRUE,
                   getter_AddRefs(oldTitleNode));
    
    
    if (NS_SUCCEEDED(rv) && oldTitleNode)
        
        rv = Change(windowResource, kNC_Name, oldTitleNode, newTitleLiteral);
    else
        
        rv = Assert(windowResource, kNC_Name, newTitleLiteral, PR_TRUE);

    if (rv != NS_RDF_ASSERTION_ACCEPTED)
    {
      NS_ERROR("unable to set window name");
    }
    
    return NS_OK;
}


NS_IMETHODIMP
nsWindowDataSource::OnOpenWindow(nsIXULWindow *window)
{
    nsCAutoString windowId(NS_LITERAL_CSTRING("window-"));
    windowId.AppendInt(windowCount++, 10);

    nsCOMPtr<nsIRDFResource> windowResource;
    gRDFService->GetResource(windowId, getter_AddRefs(windowResource));

    nsVoidKey key(window);
    mWindowResources.Put(&key, windowResource);

    
    if (mContainer)
        mContainer->AppendElement(windowResource);

    return NS_OK;
}


NS_IMETHODIMP
nsWindowDataSource::OnCloseWindow(nsIXULWindow *window)
{
    nsVoidKey key(window);
    nsCOMPtr<nsIRDFResource> resource;

    nsresult rv;

    if (!mWindowResources.Remove(&key, getter_AddRefs(resource)))
        return NS_ERROR_UNEXPECTED;

    
    if (!mContainer) return NS_OK;
    
    nsCOMPtr<nsIRDFNode> oldKeyNode;
    nsCOMPtr<nsIRDFInt> oldKeyInt;
    
    
    rv = GetTarget(resource, kNC_KeyIndex, PR_TRUE,
                   getter_AddRefs(oldKeyNode));
    if (NS_SUCCEEDED(rv) && (rv != NS_RDF_NO_VALUE))
        oldKeyInt = do_QueryInterface(oldKeyNode);

    
    
    
    PRInt32 winIndex = -1;
    rv = mContainer->IndexOf(resource, &winIndex);
        
    if (NS_FAILED(rv))
        return NS_OK;
            
    
    mContainer->RemoveElement(resource, PR_TRUE);
    
    nsCOMPtr<nsISimpleEnumerator> children;
    rv = mContainer->GetElements(getter_AddRefs(children));
    if (NS_FAILED(rv))
        return NS_OK;

    PRBool more = PR_FALSE;

    while (NS_SUCCEEDED(rv = children->HasMoreElements(&more)) && more) {
        nsCOMPtr<nsISupports> sup;
        rv = children->GetNext(getter_AddRefs(sup));
        if (NS_FAILED(rv))
            break;

        nsCOMPtr<nsIRDFResource> windowResource = do_QueryInterface(sup, &rv);
        if (NS_FAILED(rv))
            continue;

        PRInt32 currentIndex = -1;
        mContainer->IndexOf(windowResource, &currentIndex);

        
        
        if (currentIndex < winIndex)
            continue;

        nsCOMPtr<nsIRDFNode> newKeyNode;
        nsCOMPtr<nsIRDFInt> newKeyInt;

        rv = GetTarget(windowResource, kNC_KeyIndex, PR_TRUE,
                       getter_AddRefs(newKeyNode));
        if (NS_SUCCEEDED(rv) && (rv != NS_RDF_NO_VALUE))
            newKeyInt = do_QueryInterface(newKeyNode);

        
        if (oldKeyInt && newKeyInt)
            Change(windowResource, kNC_KeyIndex, oldKeyInt, newKeyInt);
        
        
        else if (newKeyInt)
            Assert(windowResource, kNC_KeyIndex, newKeyInt, PR_TRUE);
        
        
        
        else if (oldKeyInt)
            Unassert(windowResource, kNC_KeyIndex, oldKeyInt);
        
    }
    return NS_OK;
}

struct findWindowClosure {
    nsIRDFResource* targetResource;
    nsIXULWindow *resultWindow;
};

PR_STATIC_CALLBACK(PRBool)
findWindow(nsHashKey* aKey, void *aData, void* aClosure)
{
    nsVoidKey *thisKey = NS_STATIC_CAST(nsVoidKey*, aKey);

    nsIRDFResource *resource =
        NS_STATIC_CAST(nsIRDFResource*, aData);
    
    findWindowClosure* closure =
        NS_STATIC_CAST(findWindowClosure*, aClosure);

    if (resource == closure->targetResource) {
        closure->resultWindow =
            NS_STATIC_CAST(nsIXULWindow*,
                           thisKey->GetValue());
        return PR_FALSE;         
    }
    return PR_TRUE;
}



NS_IMETHODIMP
nsWindowDataSource::GetWindowForResource(const char *aResourceString,
                                         nsIDOMWindowInternal** aResult)
{
    nsCOMPtr<nsIRDFResource> windowResource;
    gRDFService->GetResource(nsDependentCString(aResourceString),
                             getter_AddRefs(windowResource));

    
    findWindowClosure closure = { windowResource.get(), nsnull };
    mWindowResources.Enumerate(findWindow, (void*)&closure);
    if (closure.resultWindow) {

        
        
        nsCOMPtr<nsIDocShell> docShell;
        closure.resultWindow->GetDocShell(getter_AddRefs(docShell));

        if (docShell) {
            nsCOMPtr<nsIDOMWindowInternal> result =
                do_GetInterface(docShell);
        
            *aResult = result;
            NS_IF_ADDREF(*aResult);
        }
    }

    return NS_OK;
}









NS_IMETHODIMP nsWindowDataSource::GetURI(char * *aURI)
{
    NS_ENSURE_ARG_POINTER(aURI);
    
    *aURI = ToNewCString(NS_LITERAL_CSTRING("rdf:window-mediator"));

    if (!*aURI)
        return NS_ERROR_OUT_OF_MEMORY;
    
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::GetTarget(nsIRDFResource *aSource, nsIRDFResource *aProperty, PRBool aTruthValue, nsIRDFNode **_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);

    
    if (!gRDFService) return NS_RDF_NO_VALUE;
    if (!mInner) return NS_RDF_NO_VALUE;
    if (!mContainer) return NS_RDF_NO_VALUE;
    
    if (aProperty == kNC_KeyIndex) {

        PRInt32 theIndex = 0;
        nsresult rv = mContainer->IndexOf(aSource, &theIndex);
        if (NS_FAILED(rv)) return rv;

        
        if (theIndex < 1 || theIndex > 9) return(NS_RDF_NO_VALUE);

        nsCOMPtr<nsIRDFInt> indexInt;
        rv = gRDFService->GetIntLiteral(theIndex, getter_AddRefs(indexInt));
        if (NS_FAILED(rv)) return(rv);
        if (!indexInt) return(NS_ERROR_FAILURE);
        
        return CallQueryInterface(indexInt, _retval);
    }

    
    return mInner->GetTarget(aSource, aProperty, aTruthValue, _retval);
}


NS_IMETHODIMP nsWindowDataSource::GetSource(nsIRDFResource *aProperty, nsIRDFNode *aTarget, PRBool aTruthValue, nsIRDFResource **_retval)
{
    if (mInner)
        return mInner->GetSource(aProperty, aTarget, aTruthValue, _retval);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::GetSources(nsIRDFResource *aProperty, nsIRDFNode *aTarget, PRBool aTruthValue, nsISimpleEnumerator **_retval)
{
    if (mInner)
        return mInner->GetSources(aProperty, aTarget, aTruthValue, _retval);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::GetTargets(nsIRDFResource *aSource, nsIRDFResource *aProperty, PRBool aTruthValue, nsISimpleEnumerator **_retval)
{
    if (mInner)
        return mInner->GetTargets(aSource, aProperty, aTruthValue, _retval);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::Assert(nsIRDFResource *aSource, nsIRDFResource *aProperty, nsIRDFNode *aTarget, PRBool aTruthValue)
{
    if (mInner)
        return mInner->Assert(aSource, aProperty, aTarget, aTruthValue);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::Unassert(nsIRDFResource *aSource, nsIRDFResource *aProperty, nsIRDFNode *aTarget)
{
    if (mInner)
        return mInner->Unassert(aSource, aProperty, aTarget);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::Change(nsIRDFResource *aSource, nsIRDFResource *aProperty, nsIRDFNode *aOldTarget, nsIRDFNode *aNewTarget)
{
    if (mInner)
        return mInner->Change(aSource, aProperty, aOldTarget, aNewTarget);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::Move(nsIRDFResource *aOldSource, nsIRDFResource *aNewSource, nsIRDFResource *aProperty, nsIRDFNode *aTarget)
{
    if (mInner)
        return mInner->Move(aOldSource, aNewSource, aProperty, aTarget);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::HasAssertion(nsIRDFResource *aSource, nsIRDFResource *aProperty, nsIRDFNode *aTarget, PRBool aTruthValue, PRBool *_retval)
{
    if (mInner)
        return mInner->HasAssertion(aSource, aProperty, aTarget, aTruthValue, _retval);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::AddObserver(nsIRDFObserver *aObserver)
{
    if (mInner)
        return mInner->AddObserver(aObserver);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::RemoveObserver(nsIRDFObserver *aObserver)
{
    if (mInner)
        return mInner->RemoveObserver(aObserver);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::ArcLabelsIn(nsIRDFNode *aNode, nsISimpleEnumerator **_retval)
{
    if (mInner)
        return mInner->ArcLabelsIn(aNode, _retval);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::ArcLabelsOut(nsIRDFResource *aSource, nsISimpleEnumerator **_retval)
{
    if (mInner)
        return mInner->ArcLabelsOut(aSource, _retval);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::GetAllResources(nsISimpleEnumerator **_retval)
{
    if (mInner)
        return mInner->GetAllResources(_retval);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::IsCommandEnabled(nsISupportsArray *aSources, nsIRDFResource *aCommand, nsISupportsArray *aArguments, PRBool *_retval)
{
    if (mInner)
        return mInner->IsCommandEnabled(aSources, aCommand, aArguments, _retval);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::DoCommand(nsISupportsArray *aSources, nsIRDFResource *aCommand, nsISupportsArray *aArguments)
{
    if (mInner)
        return mInner->DoCommand(aSources, aCommand, aArguments);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::GetAllCmds(nsIRDFResource *aSource, nsISimpleEnumerator **_retval)
{
    if (mInner)
        return mInner->GetAllCmds(aSource, _retval);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::HasArcIn(nsIRDFNode *aNode, nsIRDFResource *aArc, PRBool *_retval)
{
    if (mInner)
        return mInner->HasArcIn(aNode, aArc, _retval);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::HasArcOut(nsIRDFResource *aSource, nsIRDFResource *aArc, PRBool *_retval)
{
    if (mInner)
        return mInner->HasArcOut(aSource, aArc, _retval);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::BeginUpdateBatch()
{
    if (mInner)
        return mInner->BeginUpdateBatch();
    return NS_OK;
}
                                                                                

NS_IMETHODIMP nsWindowDataSource::EndUpdateBatch()
{
    if (mInner)
        return mInner->EndUpdateBatch();
    return NS_OK;
}



NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsWindowDataSource, Init)

static NS_METHOD
RegisterWindowDS(nsIComponentManager *aCompMgr,
                 nsIFile *aPath,
                 const char *registryLocation,
                 const char *componentType,
                 const nsModuleComponentInfo *info)
{
    nsresult rv;
    nsCOMPtr<nsICategoryManager> catman = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    return catman->AddCategoryEntry("app-startup", "Window Data Source",
                                    "service," NS_RDF_DATASOURCE_CONTRACTID_PREFIX "window-mediator",
                                    PR_TRUE, PR_TRUE, nsnull);
    return NS_OK;
}

static const nsModuleComponentInfo components[] = {
    { "nsWindowDataSource",
      NS_WINDOWDATASOURCE_CID,
      NS_RDF_DATASOURCE_CONTRACTID_PREFIX "window-mediator",
      nsWindowDataSourceConstructor, RegisterWindowDS }
};

NS_IMPL_NSGETMODULE(nsWindowDataSourceModule, components)
