





































#include "nsUICommandCollector.h"
#include "nsMetricsService.h"

#include "nsServiceManagerUtils.h"
#include "nsIObserverService.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMXULCommandEvent.h"
#include "nsIDOMElement.h"
#include "nsIDOMWindow.h"
#include "nsDataHashtable.h"
#ifndef MOZ_PLACES_BOOKMARKS
#include "nsIRDFService.h"
#include "nsIRDFResource.h"
#include "nsIRDFContainer.h"
#include "nsIBookmarksService.h"
#include "nsIArray.h"
#include "nsComponentManagerUtils.h"
#endif

NS_IMPL_ISUPPORTS3(nsUICommandCollector, nsIObserver, nsIDOMEventListener,
                   nsIMetricsCollector)


PLDHashOperator PR_CALLBACK nsUICommandCollector::AddCommandEventListener(
const nsIDOMWindow* key, PRUint32 windowID, void* userArg)
{
  nsCOMPtr<nsIDOMEventTarget> windowTarget =
    do_QueryInterface(NS_CONST_CAST(nsIDOMWindow *, key));
  if (!windowTarget) {
    MS_LOG(("Error casting domeventtarget"));
    return PL_DHASH_NEXT;
  }

  nsIDOMEventListener* listener = NS_STATIC_CAST(nsIDOMEventListener*,
                                                 userArg);
  if (!listener) {
    MS_LOG(("no event listener in userArg"));
    return PL_DHASH_NEXT;
  }

  nsresult rv = windowTarget->AddEventListener(NS_LITERAL_STRING("command"),
                                               listener, PR_TRUE);
  if (NS_FAILED(rv)) {
    MS_LOG(("Warning: Adding event listener failed, window %p (id %d)",
            key, windowID));
  }
  return PL_DHASH_NEXT;
}


PLDHashOperator PR_CALLBACK nsUICommandCollector::RemoveCommandEventListener(
const nsIDOMWindow* key, PRUint32 windowID, void* userArg)
{
  nsCOMPtr<nsIDOMEventTarget> windowTarget =
    do_QueryInterface(NS_CONST_CAST(nsIDOMWindow *, key));
  if (!windowTarget) {
    MS_LOG(("Error casting domeventtarget"));
    return PL_DHASH_NEXT;
  }

  nsIDOMEventListener* listener = NS_STATIC_CAST(nsIDOMEventListener*,
                                                 userArg);
  if (!listener) {
    MS_LOG(("no event listener in userArg"));
    return PL_DHASH_NEXT;
  }

  nsresult rv = windowTarget->RemoveEventListener(NS_LITERAL_STRING("command"),
                                                  listener, PR_TRUE);
  if (NS_FAILED(rv)) {
    MS_LOG(("Warning: Removing event listener failed, window %p (id %d)",
            key, windowID));
  }
  return PL_DHASH_NEXT;
}

nsUICommandCollector::nsUICommandCollector()
{
}

nsUICommandCollector::~nsUICommandCollector()
{
}


NS_IMETHODIMP
nsUICommandCollector::OnAttach()
{
  nsresult rv;
  nsCOMPtr<nsIObserverService> obsSvc =
    do_GetService("@mozilla.org/observer-service;1");
  NS_ENSURE_STATE(obsSvc);

  
  
  rv = obsSvc->AddObserver(this, "domwindowopened", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsMetricsService *ms = nsMetricsService::get();
  NS_ENSURE_STATE(ms);

  ms->WindowMap().EnumerateRead(AddCommandEventListener,
                                NS_STATIC_CAST(nsIDOMEventListener*, this));

  return NS_OK;
}

NS_IMETHODIMP
nsUICommandCollector::OnDetach()
{
  nsresult rv;
  nsCOMPtr<nsIObserverService> obsSvc =
    do_GetService("@mozilla.org/observer-service;1");
  NS_ENSURE_STATE(obsSvc);

  
  rv = obsSvc->RemoveObserver(this, "domwindowopened");
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  nsMetricsService *ms = nsMetricsService::get();
  NS_ENSURE_STATE(ms);

  ms->WindowMap().EnumerateRead(RemoveCommandEventListener,
    NS_STATIC_CAST(nsIDOMEventListener*, this));

  return NS_OK;
}

NS_IMETHODIMP
nsUICommandCollector::OnNewLog()
{
  return NS_OK;
}


NS_IMETHODIMP
nsUICommandCollector::Observe(nsISupports *subject,
                              const char *topic,
                              const PRUnichar *data)
{
  if (strcmp(topic, "domwindowopened") == 0) {
    
    
    
    nsCOMPtr<nsIDOMEventTarget> window = do_QueryInterface(subject);
    NS_ENSURE_STATE(window);

    nsresult rv = window->AddEventListener(NS_LITERAL_STRING("command"),
                                           this,
                                           PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsUICommandCollector::HandleEvent(nsIDOMEvent* event)
{
  
  nsString type;
  nsresult rv = event->GetType(type);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!type.Equals(NS_LITERAL_STRING("command"))) {
    MS_LOG(("UICommandCollector: Unexpected event type %s received",
            NS_ConvertUTF16toUTF8(type).get()));
    return NS_ERROR_UNEXPECTED;
  }

  PRUint32 window;
  if (NS_FAILED(GetEventWindow(event, &window))) {
    return NS_OK;
  }

  nsString targetId, targetAnonId;
  if (NS_FAILED(GetEventTargets(event, targetId, targetAnonId))) {
    return NS_OK;
  }
  NS_ASSERTION(!targetId.IsEmpty(), "can't have an empty target id");

  nsString keyId;
  GetEventKeyId(event, keyId);

  
  nsCOMPtr<nsIWritablePropertyBag2> properties;
  nsMetricsUtils::NewPropertyBag(getter_AddRefs(properties));
  NS_ENSURE_STATE(properties);

  rv = properties->SetPropertyAsUint32(NS_LITERAL_STRING("window"), window);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = properties->SetPropertyAsAString(NS_LITERAL_STRING("action"),
                                        type);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetHashedValue(properties, NS_LITERAL_STRING("targetidhash"), targetId);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!targetAnonId.IsEmpty()) {
    rv = SetHashedValue(properties, NS_LITERAL_STRING("targetanonidhash"),
                        targetAnonId);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (!keyId.IsEmpty()) {
    rv = SetHashedValue(properties, NS_LITERAL_STRING("keyidhash"), keyId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsMetricsService *ms = nsMetricsService::get();
  NS_ENSURE_STATE(ms);

  nsCOMPtr<nsIMetricsEventItem> item;
  ms->CreateEventItem(NS_LITERAL_STRING("uielement"), getter_AddRefs(item));
  NS_ENSURE_STATE(item);
  item->SetProperties(properties);

  
  rv = LogBookmarkInfo(targetId, item);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = ms->LogEvent(item);
  NS_ENSURE_SUCCESS(rv, rv);

  MS_LOG(("Successfully logged UI Event"));
  return NS_OK;
}

nsresult
nsUICommandCollector::GetEventTargets(nsIDOMEvent *event,
                                      nsString &targetId,
                                      nsString &targetAnonId) const
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsCOMPtr<nsIDOMNSEvent> nsEvent = do_QueryInterface(event);
  NS_ENSURE_STATE(nsEvent);

  nsCOMPtr<nsIDOMEventTarget> originalTarget;
  nsEvent->GetOriginalTarget(getter_AddRefs(originalTarget));
  NS_ENSURE_STATE(originalTarget);

  nsString origElementId;
  nsCOMPtr<nsIDOMElement> origElement(do_QueryInterface(originalTarget));
  if (origElement) {
    origElement->GetAttribute(NS_LITERAL_STRING("id"), origElementId);
    origElement->GetAttribute(NS_LITERAL_STRING("anonid"), targetAnonId);
  }

  nsCOMPtr<nsIDOMEventTarget> target;
  event->GetTarget(getter_AddRefs(target));
  NS_ENSURE_STATE(target);

  nsCOMPtr<nsIDOMElement> targetElement(do_QueryInterface(target));
  if (targetElement) {
    targetElement->GetAttribute(NS_LITERAL_STRING("id"), targetId);
  }

  MS_LOG(("Original Target Id: %s, Original Target Anonid: %s, Target Id: %s",
          NS_ConvertUTF16toUTF8(origElementId).get(),
          NS_ConvertUTF16toUTF8(targetAnonId).get(),
          NS_ConvertUTF16toUTF8(targetId).get()));

  if (targetId.IsEmpty()) {
    
    
    MS_LOG(("Warning: skipping logging because of empty target ID"));
    return NS_ERROR_FAILURE;
  }

  if (origElementId.IsEmpty()) {
    
    
    if (targetAnonId.IsEmpty()) {
      MS_LOG(("Warning: skipping logging because of empty anonid"));
      return NS_ERROR_FAILURE;
    }
  } else {
    
    targetAnonId.SetLength(0);
  }

  return NS_OK;
}

void
nsUICommandCollector::GetEventKeyId(nsIDOMEvent *event, nsString &keyId) const
{
  
  
  nsCOMPtr<nsIDOMXULCommandEvent> commandEvent = do_QueryInterface(event);
  if (!commandEvent) {
    
    return;
  }

  nsCOMPtr<nsIDOMEvent> sourceEvent;
  commandEvent->GetSourceEvent(getter_AddRefs(sourceEvent));
  if (!sourceEvent) {
    return;
  }

  nsCOMPtr<nsIDOMEventTarget> sourceEventTarget;
  sourceEvent->GetTarget(getter_AddRefs(sourceEventTarget));
  nsCOMPtr<nsIDOMElement> sourceElement = do_QueryInterface(sourceEventTarget);
  if (!sourceElement) {
    return;
  }

  nsString namespaceURI, localName;
  sourceElement->GetNamespaceURI(namespaceURI);
  sourceElement->GetLocalName(localName);
  if (namespaceURI.Equals(NS_LITERAL_STRING("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul")) &&
      localName.Equals(NS_LITERAL_STRING("key"))) {
    sourceElement->GetAttribute(NS_LITERAL_STRING("id"), keyId);
    MS_LOG(("Key Id: %s", NS_ConvertUTF16toUTF8(keyId).get()));
  }
}

nsresult
nsUICommandCollector::GetEventWindow(nsIDOMEvent *event,
                                     PRUint32 *window) const
{
  nsCOMPtr<nsIDOMEventTarget> target;
  event->GetTarget(getter_AddRefs(target));
  nsCOMPtr<nsIDOMNode> targetNode = do_QueryInterface(target);
  if (!targetNode) {
    MS_LOG(("Warning: couldn't get window id because target is not a node"));
    return NS_ERROR_FAILURE;
  }

  *window = nsMetricsUtils::FindWindowForNode(targetNode);
  return NS_OK;
}

 nsresult
nsUICommandCollector::SetHashedValue(nsIWritablePropertyBag2 *properties,
                                     const nsString &propertyName,
                                     const nsString &propertyValue) const
{
  nsMetricsService *ms = nsMetricsService::get();
  NS_ENSURE_STATE(ms);

  nsCString hashedValue;
  nsresult rv = ms->HashUTF16(propertyValue, hashedValue);
  NS_ENSURE_SUCCESS(rv, rv);

  return properties->SetPropertyAsACString(propertyName, hashedValue);
}

nsresult
nsUICommandCollector::LogBookmarkInfo(const nsString& id,
                                      nsIMetricsEventItem* parentItem)
{
#ifdef MOZ_PLACES_BOOKMARKS
  
  
  return NS_OK;

#else

  
  
  if (!StringHead(id, strlen("rdf:")).Equals(NS_LITERAL_STRING("rdf:"))) {
    return NS_OK;
  }

  nsCOMPtr<nsIRDFService> rdfSvc =
    do_GetService("@mozilla.org/rdf/rdf-service;1");
  NS_ENSURE_STATE(rdfSvc);

  nsCOMPtr<nsIRDFResource> targetResource;
  rdfSvc->GetUnicodeResource(id, getter_AddRefs(targetResource));
  NS_ENSURE_STATE(targetResource);

  nsCOMPtr<nsIWritablePropertyBag2> bmProps;
  nsMetricsUtils::NewPropertyBag(getter_AddRefs(bmProps));
  NS_ENSURE_STATE(bmProps);

  nsCOMPtr<nsIBookmarksService> bmSvc =
    do_GetService(NS_BOOKMARKS_SERVICE_CONTRACTID);
  NS_ENSURE_STATE(bmSvc);

  nsCOMPtr<nsIArray> parentChain;
  bmSvc->GetParentChain(targetResource, getter_AddRefs(parentChain));
  NS_ENSURE_STATE(parentChain);

  PRUint32 depth = 0;
  parentChain->GetLength(&depth);
  bmProps->SetPropertyAsInt32(NS_LITERAL_STRING("depth"), depth);
  if (depth == 0) {
    
    return NS_OK;
  }

  nsCOMPtr<nsIRDFDataSource> bmDS =
    do_GetService(NS_BOOKMARKS_DATASOURCE_CONTRACTID);
  NS_ENSURE_STATE(bmDS);

  
  
  nsCOMPtr<nsIRDFResource> parent;
  parentChain->QueryElementAt(depth - 1, NS_GET_IID(nsIRDFResource),
                              getter_AddRefs(parent));
  NS_ENSURE_STATE(parent);

  nsCOMPtr<nsIRDFContainer> container =
    do_CreateInstance("@mozilla.org/rdf/container;1");
  NS_ENSURE_STATE(container);

  nsresult rv = container->Init(bmDS, parent);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 pos;
  rv = container->IndexOf(targetResource, &pos);
  NS_ENSURE_SUCCESS(rv, rv);
  if (pos == -1) {
    MS_LOG(("Bookmark not contained in its parent?"));
  } else {
    bmProps->SetPropertyAsInt32(NS_LITERAL_STRING("pos"), pos);
  }

  
  PRBool isToolbarBM = PR_FALSE;
  nsCOMPtr<nsIRDFResource> toolbarFolder;
  bmSvc->GetBookmarksToolbarFolder(getter_AddRefs(toolbarFolder));
  if (toolbarFolder) {
    
    
    for (PRUint32 i = 0; i < depth; ++i) {
      nsCOMPtr<nsIRDFResource> item;
      parentChain->QueryElementAt(i, NS_GET_IID(nsIRDFResource),
                                  getter_AddRefs(item));
      if (toolbarFolder == item) {
        isToolbarBM = PR_TRUE;
        break;
      }
    }
  }
  bmProps->SetPropertyAsBool(NS_LITERAL_STRING("toolbar"), isToolbarBM);

  return nsMetricsUtils::AddChildItem(parentItem,
                                      NS_LITERAL_STRING("bookmark"), bmProps);
#endif  
}
