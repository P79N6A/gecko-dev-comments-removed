




#include "nsWindowDataSource.h"
#include "nsIXULWindow.h"
#include "rdf.h"
#include "nsIRDFContainerUtils.h"
#include "nsIServiceManager.h"
#include "nsReadableUtils.h"
#include "nsIObserverService.h"
#include "nsIWindowMediator.h"
#include "nsXPCOMCID.h"
#include "mozilla/ModuleUtils.h"
#include "nsString.h"


#include "nsIInterfaceRequestorUtils.h"
#include "nsIDocShell.h"

uint32_t nsWindowDataSource::windowCount = 0;

nsIRDFResource* nsWindowDataSource::kNC_Name = nullptr;
nsIRDFResource* nsWindowDataSource::kNC_WindowRoot = nullptr;
nsIRDFResource* nsWindowDataSource::kNC_KeyIndex = nullptr;

nsIRDFService*  nsWindowDataSource::gRDFService = nullptr;

uint32_t nsWindowDataSource::gRefCnt = 0;

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
                                          false);
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
nsWindowDataSource::Observe(nsISupports *aSubject, const char* aTopic, const char16_t *aData)
{
    if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
        
        
        mContainer = nullptr;
        mInner = nullptr;
    }

    return NS_OK;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsWindowDataSource)

NS_IMPL_CYCLE_COLLECTION_UNLINK_0(nsWindowDataSource)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsWindowDataSource)
    
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mInner)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsWindowDataSource)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsWindowDataSource)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsWindowDataSource)
    NS_INTERFACE_MAP_ENTRY(nsIObserver)
    NS_INTERFACE_MAP_ENTRY(nsIWindowMediatorListener)
    NS_INTERFACE_MAP_ENTRY(nsIWindowDataSource)
    NS_INTERFACE_MAP_ENTRY(nsIRDFDataSource)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIObserver)
NS_INTERFACE_MAP_END






NS_IMETHODIMP
nsWindowDataSource::OnWindowTitleChange(nsIXULWindow *window,
                                        const char16_t *newTitle)
{
    nsresult rv;

    nsCOMPtr<nsIRDFResource> windowResource;
    mWindowResources.Get(window, getter_AddRefs(windowResource));

    
    if (!windowResource) {
        OnOpenWindow(window);
        mWindowResources.Get(window, getter_AddRefs(windowResource));
    }

    NS_ENSURE_TRUE(windowResource, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIRDFLiteral> newTitleLiteral;
    rv = gRDFService->GetLiteral(newTitle, getter_AddRefs(newTitleLiteral));
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIRDFNode> oldTitleNode;
    rv = GetTarget(windowResource, kNC_Name, true,
                   getter_AddRefs(oldTitleNode));

    
    if (NS_SUCCEEDED(rv) && oldTitleNode)
        
        rv = Change(windowResource, kNC_Name, oldTitleNode, newTitleLiteral);
    else
        
        rv = Assert(windowResource, kNC_Name, newTitleLiteral, true);

    if (rv != NS_RDF_ASSERTION_ACCEPTED)
    {
      NS_ERROR("unable to set window name");
    }

    return NS_OK;
}


NS_IMETHODIMP
nsWindowDataSource::OnOpenWindow(nsIXULWindow *window)
{
    nsAutoCString windowId(NS_LITERAL_CSTRING("window-"));
    windowId.AppendInt(windowCount++, 10);

    nsCOMPtr<nsIRDFResource> windowResource;
    gRDFService->GetResource(windowId, getter_AddRefs(windowResource));

    mWindowResources.Put(window, windowResource);

    
    if (mContainer)
        mContainer->AppendElement(windowResource);

    return NS_OK;
}


NS_IMETHODIMP
nsWindowDataSource::OnCloseWindow(nsIXULWindow *window)
{
    nsresult rv;
    nsCOMPtr<nsIRDFResource> resource;
    mWindowResources.Get(window, getter_AddRefs(resource));
    if (!resource) {
        return NS_ERROR_UNEXPECTED;
    }

    mWindowResources.Remove(window);

    
    if (!mContainer) return NS_OK;

    nsCOMPtr<nsIRDFNode> oldKeyNode;
    nsCOMPtr<nsIRDFInt> oldKeyInt;

    
    rv = GetTarget(resource, kNC_KeyIndex, true,
                   getter_AddRefs(oldKeyNode));
    if (NS_SUCCEEDED(rv) && (rv != NS_RDF_NO_VALUE))
        oldKeyInt = do_QueryInterface(oldKeyNode);


    
    
    int32_t winIndex = -1;
    rv = mContainer->IndexOf(resource, &winIndex);

    if (NS_FAILED(rv))
        return NS_OK;

    
    mContainer->RemoveElement(resource, true);

    nsCOMPtr<nsISimpleEnumerator> children;
    rv = mContainer->GetElements(getter_AddRefs(children));
    if (NS_FAILED(rv))
        return NS_OK;

    bool more = false;

    while (NS_SUCCEEDED(rv = children->HasMoreElements(&more)) && more) {
        nsCOMPtr<nsISupports> sup;
        rv = children->GetNext(getter_AddRefs(sup));
        if (NS_FAILED(rv))
            break;

        nsCOMPtr<nsIRDFResource> windowResource = do_QueryInterface(sup, &rv);
        if (NS_FAILED(rv))
            continue;

        int32_t currentIndex = -1;
        mContainer->IndexOf(windowResource, &currentIndex);

        
        
        if (currentIndex < winIndex)
            continue;

        nsCOMPtr<nsIRDFNode> newKeyNode;
        nsCOMPtr<nsIRDFInt> newKeyInt;

        rv = GetTarget(windowResource, kNC_KeyIndex, true,
                       getter_AddRefs(newKeyNode));
        if (NS_SUCCEEDED(rv) && (rv != NS_RDF_NO_VALUE))
            newKeyInt = do_QueryInterface(newKeyNode);

        
        if (oldKeyInt && newKeyInt)
            Change(windowResource, kNC_KeyIndex, oldKeyInt, newKeyInt);
        
        
        else if (newKeyInt)
            Assert(windowResource, kNC_KeyIndex, newKeyInt, true);

        
        
        else if (oldKeyInt)
            Unassert(windowResource, kNC_KeyIndex, oldKeyInt);

    }
    return NS_OK;
}

struct findWindowClosure {
    nsIRDFResource* targetResource;
    nsIXULWindow *resultWindow;
};

static PLDHashOperator
findWindow(nsIXULWindow* aWindow, nsIRDFResource* aResource, void* aClosure)
{
    findWindowClosure* closure = static_cast<findWindowClosure*>(aClosure);

    if (aResource == closure->targetResource) {
        closure->resultWindow = aWindow;
        return PL_DHASH_STOP;
    }
    return PL_DHASH_NEXT;
}



NS_IMETHODIMP
nsWindowDataSource::GetWindowForResource(const char *aResourceString,
                                         nsIDOMWindow** aResult)
{
    if (NS_WARN_IF(!aResourceString)) {
        return NS_ERROR_INVALID_ARG;
    }

    nsCOMPtr<nsIRDFResource> windowResource;
    gRDFService->GetResource(nsDependentCString(aResourceString),
                             getter_AddRefs(windowResource));

    
    findWindowClosure closure = { windowResource.get(), nullptr };
    mWindowResources.EnumerateRead(findWindow, &closure);
    if (closure.resultWindow) {

        
        
        nsCOMPtr<nsIDocShell> docShell;
        closure.resultWindow->GetDocShell(getter_AddRefs(docShell));

        if (docShell) {
            nsCOMPtr<nsIDOMWindow> result = do_GetInterface(docShell);

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


NS_IMETHODIMP nsWindowDataSource::GetTarget(nsIRDFResource *aSource, nsIRDFResource *aProperty, bool aTruthValue, nsIRDFNode **_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);

    
    if (!gRDFService) return NS_RDF_NO_VALUE;
    if (!mInner) return NS_RDF_NO_VALUE;
    if (!mContainer) return NS_RDF_NO_VALUE;
    
    if (aProperty == kNC_KeyIndex) {

        int32_t theIndex = 0;
        nsresult rv = mContainer->IndexOf(aSource, &theIndex);
        if (NS_FAILED(rv)) return rv;

        
        if (theIndex < 1 || theIndex > 9) return(NS_RDF_NO_VALUE);

        nsCOMPtr<nsIRDFInt> indexInt;
        rv = gRDFService->GetIntLiteral(theIndex, getter_AddRefs(indexInt));
        if (NS_FAILED(rv)) return(rv);
        if (!indexInt) return(NS_ERROR_FAILURE);

        indexInt.forget(_retval);
        return NS_OK;
    }

    return mInner->GetTarget(aSource, aProperty, aTruthValue, _retval);
}


NS_IMETHODIMP nsWindowDataSource::GetSource(nsIRDFResource *aProperty, nsIRDFNode *aTarget, bool aTruthValue, nsIRDFResource **_retval)
{
    if (mInner)
        return mInner->GetSource(aProperty, aTarget, aTruthValue, _retval);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::GetSources(nsIRDFResource *aProperty, nsIRDFNode *aTarget, bool aTruthValue, nsISimpleEnumerator **_retval)
{
    if (mInner)
        return mInner->GetSources(aProperty, aTarget, aTruthValue, _retval);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::GetTargets(nsIRDFResource *aSource, nsIRDFResource *aProperty, bool aTruthValue, nsISimpleEnumerator **_retval)
{
    if (mInner)
        return mInner->GetTargets(aSource, aProperty, aTruthValue, _retval);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::Assert(nsIRDFResource *aSource, nsIRDFResource *aProperty, nsIRDFNode *aTarget, bool aTruthValue)
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


NS_IMETHODIMP nsWindowDataSource::HasAssertion(nsIRDFResource *aSource, nsIRDFResource *aProperty, nsIRDFNode *aTarget, bool aTruthValue, bool *_retval)
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


NS_IMETHODIMP nsWindowDataSource::IsCommandEnabled(nsISupportsArray *aSources, nsIRDFResource *aCommand, nsISupportsArray *aArguments, bool *_retval)
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


NS_IMETHODIMP nsWindowDataSource::HasArcIn(nsIRDFNode *aNode, nsIRDFResource *aArc, bool *_retval)
{
    if (mInner)
        return mInner->HasArcIn(aNode, aArc, _retval);
    return NS_OK;
}


NS_IMETHODIMP nsWindowDataSource::HasArcOut(nsIRDFResource *aSource, nsIRDFResource *aArc, bool *_retval)
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

NS_DEFINE_NAMED_CID(NS_WINDOWDATASOURCE_CID);

static const mozilla::Module::CIDEntry kWindowDSCIDs[] = {
    { &kNS_WINDOWDATASOURCE_CID, false, nullptr, nsWindowDataSourceConstructor },
    { nullptr }
};

static const mozilla::Module::ContractIDEntry kWindowDSContracts[] = {
    { NS_RDF_DATASOURCE_CONTRACTID_PREFIX "window-mediator", &kNS_WINDOWDATASOURCE_CID },
    { nullptr }
};

static const mozilla::Module::CategoryEntry kWindowDSCategories[] = {
    { "app-startup", "Window Data Source", "service," NS_RDF_DATASOURCE_CONTRACTID_PREFIX "window-mediator" },
    { nullptr }
};

static const mozilla::Module kWindowDSModule = {
    mozilla::Module::kVersion,
    kWindowDSCIDs,
    kWindowDSContracts,
    kWindowDSCategories
};

NSMODULE_DEFN(nsWindowDataSourceModule) = &kWindowDSModule;
