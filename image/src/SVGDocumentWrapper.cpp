





































#include "SVGDocumentWrapper.h"

#include "mozilla/dom/Element.h"
#include "nsIAtom.h"
#include "nsICategoryManager.h"
#include "nsIChannel.h"
#include "nsIContentViewer.h"
#include "nsIDocument.h"
#include "nsIDocumentLoaderFactory.h"
#include "nsIDOMSVGAnimatedLength.h"
#include "nsIDOMSVGLength.h"
#include "nsIHttpChannel.h"
#include "nsIObserverService.h"
#include "nsIParser.h"
#include "nsIPresShell.h"
#include "nsIRequest.h"
#include "nsIStreamListener.h"
#include "nsIXMLContentSink.h"
#include "nsNetCID.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsSize.h"
#include "gfxRect.h"
#include "nsSVGSVGElement.h"
#include "nsSVGLength2.h"
#include "nsSVGEffects.h"

using namespace mozilla::dom;

namespace mozilla {
namespace imagelib {

NS_IMPL_ISUPPORTS4(SVGDocumentWrapper,
                   nsIStreamListener,
                   nsIRequestObserver,
                   nsIObserver,
                   nsISupportsWeakReference)

SVGDocumentWrapper::SVGDocumentWrapper()
  : mIgnoreInvalidation(false),
    mRegisteredForXPCOMShutdown(false)
{
}

SVGDocumentWrapper::~SVGDocumentWrapper()
{
  DestroyViewer();
  if (mRegisteredForXPCOMShutdown) {
    UnregisterForXPCOMShutdown();
  }
}

void
SVGDocumentWrapper::DestroyViewer()
{
  if (mViewer) {
    mViewer->GetDocument()->OnPageHide(false, nsnull);
    mViewer->Close(nsnull);
    mViewer->Destroy();
    mViewer = nsnull;
  }
}

bool
SVGDocumentWrapper::GetWidthOrHeight(Dimension aDimension,
                                     PRInt32& aResult)
{
  nsSVGSVGElement* rootElem = GetRootSVGElem();
  NS_ABORT_IF_FALSE(rootElem, "root elem missing or of wrong type");
  nsresult rv;

  
  nsRefPtr<nsIDOMSVGAnimatedLength> domAnimLength;
  if (aDimension == eWidth) {
    rv = rootElem->GetWidth(getter_AddRefs(domAnimLength));
  } else {
    NS_ABORT_IF_FALSE(aDimension == eHeight, "invalid dimension");
    rv = rootElem->GetHeight(getter_AddRefs(domAnimLength));
  }
  NS_ENSURE_SUCCESS(rv, false);
  NS_ENSURE_TRUE(domAnimLength, false);

  
  nsRefPtr<nsIDOMSVGLength> domLength;
  rv = domAnimLength->GetAnimVal(getter_AddRefs(domLength));
  NS_ENSURE_SUCCESS(rv, false);
  NS_ENSURE_TRUE(domLength, false);

  
  PRUint16 unitType;
  rv = domLength->GetUnitType(&unitType);
  NS_ENSURE_SUCCESS(rv, false);
  if (unitType == nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE) {
    return false;
  }

  
  float floatLength;
  rv = domLength->GetValue(&floatLength);
  NS_ENSURE_SUCCESS(rv, false);

  aResult = nsSVGUtils::ClampToInt(floatLength);

  return true;
}

nsIFrame*
SVGDocumentWrapper::GetRootLayoutFrame()
{
  Element* rootElem = GetRootSVGElem();
  return rootElem ? rootElem->GetPrimaryFrame() : nsnull;
}

void
SVGDocumentWrapper::UpdateViewportBounds(const nsIntSize& aViewportSize)
{
  NS_ABORT_IF_FALSE(!mIgnoreInvalidation, "shouldn't be reentrant");
  mIgnoreInvalidation = true;
  mViewer->SetBounds(nsIntRect(nsIntPoint(0, 0), aViewportSize));
  FlushLayout();
  mIgnoreInvalidation = false;
}

void
SVGDocumentWrapper::FlushImageTransformInvalidation()
{
  NS_ABORT_IF_FALSE(!mIgnoreInvalidation, "shouldn't be reentrant");

  nsSVGSVGElement* svgElem = GetRootSVGElem();
  if (!svgElem)
    return;

  mIgnoreInvalidation = true;
  svgElem->FlushImageTransformInvalidation();
  FlushLayout();
  mIgnoreInvalidation = false;
}

bool
SVGDocumentWrapper::IsAnimated()
{
  nsIDocument* doc = mViewer->GetDocument();
  return doc && doc->HasAnimationController() &&
    doc->GetAnimationController()->HasRegisteredAnimations();
}

void
SVGDocumentWrapper::StartAnimation()
{
  
  
  if (!mViewer)
    return;

  nsIDocument* doc = mViewer->GetDocument();
  if (doc) {
    nsSMILAnimationController* controller = doc->GetAnimationController();
    if (controller) {
      controller->Resume(nsSMILTimeContainer::PAUSE_IMAGE);
    }
    doc->SetImagesNeedAnimating(true);
  }
}

void
SVGDocumentWrapper::StopAnimation()
{
  
  
  if (!mViewer)
    return;

  nsIDocument* doc = mViewer->GetDocument();
  if (doc) {
    nsSMILAnimationController* controller = doc->GetAnimationController();
    if (controller) {
      controller->Pause(nsSMILTimeContainer::PAUSE_IMAGE);
    }
    doc->SetImagesNeedAnimating(false);
  }
}

void
SVGDocumentWrapper::ResetAnimation()
{
  nsSVGSVGElement* svgElem = GetRootSVGElem();
  if (!svgElem)
    return;

#ifdef DEBUG
  nsresult rv =
#endif
    svgElem->SetCurrentTime(0.0f);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "SetCurrentTime failed");
}







NS_IMETHODIMP
SVGDocumentWrapper::OnDataAvailable(nsIRequest* aRequest, nsISupports* ctxt,
                                    nsIInputStream* inStr,
                                    PRUint32 sourceOffset,
                                    PRUint32 count)
{
  return mListener->OnDataAvailable(aRequest, ctxt, inStr,
                                    sourceOffset, count);
}




NS_IMETHODIMP
SVGDocumentWrapper::OnStartRequest(nsIRequest* aRequest, nsISupports* ctxt)
{
  nsresult rv = SetupViewer(aRequest,
                            getter_AddRefs(mViewer),
                            getter_AddRefs(mLoadGroup));

  if (NS_SUCCEEDED(rv) &&
      NS_SUCCEEDED(mListener->OnStartRequest(aRequest, nsnull))) {
    mViewer->GetDocument()->SetIsBeingUsedAsImage();
    StopAnimation(); 

    rv = mViewer->Init(nsnull, nsIntRect(0, 0, 0, 0));
    if (NS_SUCCEEDED(rv)) {
      rv = mViewer->Open(nsnull, nsnull);
    }
  }
  return rv;
}




NS_IMETHODIMP
SVGDocumentWrapper::OnStopRequest(nsIRequest* aRequest, nsISupports* ctxt,
                                  nsresult status)
{
  if (mListener) {
    mListener->OnStopRequest(aRequest, ctxt, status);
    
    
    
    
    
    nsCOMPtr<nsIParser> parser = do_QueryInterface(mListener);
    while (!parser->IsComplete()) {
      parser->CancelParsingEvents();
      parser->ContinueInterruptedParsing();
    }
    FlushLayout();
    mListener = nsnull;

    
    
    
    mViewer->LoadComplete(NS_OK);
  }

  return NS_OK;
}


NS_IMETHODIMP
SVGDocumentWrapper::Observe(nsISupports* aSubject,
                            const char* aTopic,
                            const PRUnichar *aData)
{
  if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    
    nsSVGSVGElement* svgElem = GetRootSVGElem();
    if (svgElem) {
      nsSVGEffects::RemoveAllRenderingObservers(svgElem);
    }

    
    DestroyViewer();
    if (mListener)
      mListener = nsnull;
    if (mLoadGroup)
      mLoadGroup = nsnull;

    
    
    mRegisteredForXPCOMShutdown = false;
  } else {
    NS_ERROR("Unexpected observer topic.");
  }
  return NS_OK;
}





nsresult
SVGDocumentWrapper::SetupViewer(nsIRequest* aRequest,
                                nsIContentViewer** aViewer,
                                nsILoadGroup** aLoadGroup)
{
  nsCOMPtr<nsIChannel> chan(do_QueryInterface(aRequest));
  NS_ENSURE_TRUE(chan, NS_ERROR_UNEXPECTED);

  
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aRequest));
  if (httpChannel) {
    bool requestSucceeded;
    if (NS_FAILED(httpChannel->GetRequestSucceeded(&requestSucceeded)) ||
        !requestSucceeded) {
      return NS_ERROR_FAILURE;
    }
  }

  
  nsCOMPtr<nsILoadGroup> loadGroup;
  chan->GetLoadGroup(getter_AddRefs(loadGroup));

  nsCOMPtr<nsILoadGroup> newLoadGroup =
        do_CreateInstance(NS_LOADGROUP_CONTRACTID);
  NS_ENSURE_TRUE(newLoadGroup, NS_ERROR_OUT_OF_MEMORY);
  newLoadGroup->SetLoadGroup(loadGroup);

  nsCOMPtr<nsICategoryManager> catMan =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID);
  NS_ENSURE_TRUE(catMan, NS_ERROR_NOT_AVAILABLE);
  nsXPIDLCString contractId;
  nsresult rv = catMan->GetCategoryEntry("Gecko-Content-Viewers", SVG_MIMETYPE,
                                         getter_Copies(contractId));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDocumentLoaderFactory> docLoaderFactory =
    do_GetService(contractId);
  NS_ENSURE_TRUE(docLoaderFactory, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsIContentViewer> viewer;
  nsCOMPtr<nsIStreamListener> listener;
  rv = docLoaderFactory->CreateInstance("external-resource", chan,
                                        newLoadGroup,
                                        SVG_MIMETYPE, nsnull, nsnull,
                                        getter_AddRefs(listener),
                                        getter_AddRefs(viewer));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(viewer, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIParser> parser = do_QueryInterface(listener);
  NS_ENSURE_TRUE(parser, NS_ERROR_UNEXPECTED);

  
  nsIContentSink* sink = parser->GetContentSink();
  nsCOMPtr<nsIXMLContentSink> xmlSink = do_QueryInterface(sink);
  NS_ENSURE_TRUE(sink, NS_ERROR_UNEXPECTED);

  listener.swap(mListener);
  viewer.forget(aViewer);
  newLoadGroup.forget(aLoadGroup);

  RegisterForXPCOMShutdown();
  return NS_OK;
}

void
SVGDocumentWrapper::RegisterForXPCOMShutdown()
{
  NS_ABORT_IF_FALSE(!mRegisteredForXPCOMShutdown,
                    "re-registering for XPCOM shutdown");
  
  
  
  
  nsresult rv;
  nsCOMPtr<nsIObserverService> obsSvc = do_GetService(OBSERVER_SVC_CID, &rv);
  if (NS_FAILED(rv) ||
      NS_FAILED(obsSvc->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                    true))) {
    NS_WARNING("Failed to register as observer of XPCOM shutdown");
  } else {
    mRegisteredForXPCOMShutdown = true;
  }
}

void
SVGDocumentWrapper::UnregisterForXPCOMShutdown()
{
  NS_ABORT_IF_FALSE(mRegisteredForXPCOMShutdown,
                    "unregistering for XPCOM shutdown w/out being registered");

  nsresult rv;
  nsCOMPtr<nsIObserverService> obsSvc = do_GetService(OBSERVER_SVC_CID, &rv);
  if (NS_FAILED(rv) ||
      NS_FAILED(obsSvc->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID))) {
    NS_WARNING("Failed to unregister as observer of XPCOM shutdown");
  } else {
    mRegisteredForXPCOMShutdown = false;
  }
}

void
SVGDocumentWrapper::FlushLayout()
{
  nsCOMPtr<nsIPresShell> presShell;
  mViewer->GetPresShell(getter_AddRefs(presShell));
  if (presShell) {
    presShell->FlushPendingNotifications(Flush_Layout);
  }
}

nsSVGSVGElement*
SVGDocumentWrapper::GetRootSVGElem()
{
  if (!mViewer)
    return nsnull; 

  nsIDocument* doc = mViewer->GetDocument();
  if (!doc)
    return nsnull; 

  Element* rootElem = mViewer->GetDocument()->GetRootElement();
  if (!rootElem || !rootElem->IsSVG(nsGkAtoms::svg)) {
    return nsnull;
  }

  return static_cast<nsSVGSVGElement*>(rootElem);
}

} 
} 
