




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
#include "nsSMILAnimationController.h"
#include "nsServiceManagerUtils.h"
#include "nsSize.h"
#include "gfxRect.h"
#include "mozilla/dom/SVGSVGElement.h"
#include "nsSVGLength2.h"
#include "nsSVGEffects.h"
#include "mozilla/dom/SVGAnimatedLength.h"

using namespace mozilla::dom;

namespace mozilla {
namespace image {

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
    mViewer->GetDocument()->OnPageHide(false, nullptr);
    mViewer->Close(nullptr);
    mViewer->Destroy();
    mViewer = nullptr;
  }
}

bool
SVGDocumentWrapper::GetWidthOrHeight(Dimension aDimension,
                                     int32_t& aResult)
{
  SVGSVGElement* rootElem = GetRootSVGElem();
  NS_ABORT_IF_FALSE(rootElem, "root elem missing or of wrong type");

  
  nsRefPtr<SVGAnimatedLength> domAnimLength;
  if (aDimension == eWidth) {
    domAnimLength = rootElem->Width();
  } else {
    NS_ABORT_IF_FALSE(aDimension == eHeight, "invalid dimension");
    domAnimLength = rootElem->Height();
  }
  NS_ENSURE_TRUE(domAnimLength, false);

  
  nsRefPtr<nsIDOMSVGLength> domLength;
  nsresult rv = domAnimLength->GetAnimVal(getter_AddRefs(domLength));
  NS_ENSURE_SUCCESS(rv, false);
  NS_ENSURE_TRUE(domLength, false);

  
  uint16_t unitType;
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
  return rootElem ? rootElem->GetPrimaryFrame() : nullptr;
}

void
SVGDocumentWrapper::UpdateViewportBounds(const nsIntSize& aViewportSize)
{
  NS_ABORT_IF_FALSE(!mIgnoreInvalidation, "shouldn't be reentrant");
  mIgnoreInvalidation = true;

  nsIntRect currentBounds;
  mViewer->GetBounds(currentBounds);
  
  
  if (currentBounds.Size() != aViewportSize) {
    mViewer->SetBounds(nsIntRect(nsIntPoint(0, 0), aViewportSize));
    FlushLayout();
  }

  mIgnoreInvalidation = false;
}

void
SVGDocumentWrapper::FlushImageTransformInvalidation()
{
  NS_ABORT_IF_FALSE(!mIgnoreInvalidation, "shouldn't be reentrant");

  SVGSVGElement* svgElem = GetRootSVGElem();
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
  SVGSVGElement* svgElem = GetRootSVGElem();
  if (!svgElem)
    return;

  svgElem->SetCurrentTime(0.0f);
}

float
SVGDocumentWrapper::GetCurrentTime()
{
  SVGSVGElement* svgElem = GetRootSVGElem();
  return svgElem ? svgElem->GetCurrentTime()
                 : 0.0f;
}

void
SVGDocumentWrapper::SetCurrentTime(float aTime)
{
  SVGSVGElement* svgElem = GetRootSVGElem();
  if (svgElem && svgElem->GetCurrentTime() != aTime) {
    svgElem->SetCurrentTime(aTime);
  }
}






NS_IMETHODIMP
SVGDocumentWrapper::OnDataAvailable(nsIRequest* aRequest, nsISupports* ctxt,
                                    nsIInputStream* inStr,
                                    uint64_t sourceOffset,
                                    uint32_t count)
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
      NS_SUCCEEDED(mListener->OnStartRequest(aRequest, nullptr))) {
    mViewer->GetDocument()->SetIsBeingUsedAsImage();
    StopAnimation(); 

    rv = mViewer->Init(nullptr, nsIntRect(0, 0, 0, 0));
    if (NS_SUCCEEDED(rv)) {
      rv = mViewer->Open(nullptr, nullptr);
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
    mListener = nullptr;
  }

  return NS_OK;
}


NS_IMETHODIMP
SVGDocumentWrapper::Observe(nsISupports* aSubject,
                            const char* aTopic,
                            const PRUnichar *aData)
{
  if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    
    SVGSVGElement* svgElem = GetRootSVGElem();
    if (svgElem) {
      nsSVGEffects::RemoveAllRenderingObservers(svgElem);
    }

    
    DestroyViewer();
    if (mListener)
      mListener = nullptr;
    if (mLoadGroup)
      mLoadGroup = nullptr;

    
    
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
  nsresult rv = catMan->GetCategoryEntry("Gecko-Content-Viewers", IMAGE_SVG_XML,
                                         getter_Copies(contractId));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDocumentLoaderFactory> docLoaderFactory =
    do_GetService(contractId);
  NS_ENSURE_TRUE(docLoaderFactory, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsIContentViewer> viewer;
  nsCOMPtr<nsIStreamListener> listener;
  rv = docLoaderFactory->CreateInstance("external-resource", chan,
                                        newLoadGroup,
                                        IMAGE_SVG_XML, nullptr, nullptr,
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

nsIDocument*
SVGDocumentWrapper::GetDocument()
{
  if (!mViewer)
    return nullptr;

  return mViewer->GetDocument(); 
}

SVGSVGElement*
SVGDocumentWrapper::GetRootSVGElem()
{
  if (!mViewer)
    return nullptr; 

  nsIDocument* doc = mViewer->GetDocument();
  if (!doc)
    return nullptr; 

  Element* rootElem = mViewer->GetDocument()->GetRootElement();
  if (!rootElem || !rootElem->IsSVG(nsGkAtoms::svg)) {
    return nullptr;
  }

  return static_cast<SVGSVGElement*>(rootElem);
}

} 
} 
