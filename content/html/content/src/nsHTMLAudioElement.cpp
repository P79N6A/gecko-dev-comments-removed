




































#include "nsIDOMHTMLAudioElement.h"
#include "nsIDOMHTMLSourceElement.h"
#include "nsHTMLAudioElement.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsSize.h"
#include "nsIFrame.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsDOMError.h"
#include "nsNodeInfoManager.h"
#include "plbase64.h"
#include "nsNetUtil.h"
#include "prmem.h"
#include "nsNetUtil.h"
#include "nsXPCOMStrings.h"
#include "prlock.h"
#include "nsThreadUtils.h"

#include "nsIScriptSecurityManager.h"
#include "nsIXPConnect.h"
#include "jsapi.h"
#include "nsJSUtils.h"

#include "nsIRenderingContext.h"
#include "nsITimer.h"

#include "nsEventDispatcher.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMProgressEvent.h"
#include "nsHTMLMediaError.h"

nsGenericHTMLElement*
NS_NewHTMLAudioElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                       PRUint32 aFromParser)
{
  




  nsCOMPtr<nsINodeInfo> nodeInfo(aNodeInfo);
  if (!nodeInfo) {
    nsCOMPtr<nsIDocument> doc =
      do_QueryInterface(nsContentUtils::GetDocumentFromCaller());
    NS_ENSURE_TRUE(doc, nsnull);

    nodeInfo = doc->NodeInfoManager()->GetNodeInfo(nsGkAtoms::audio, nsnull,
                                                   kNameSpaceID_XHTML);
    NS_ENSURE_TRUE(nodeInfo, nsnull);
  }

  return new nsHTMLAudioElement(nodeInfo.forget(), aFromParser);
}

NS_IMPL_ADDREF_INHERITED(nsHTMLAudioElement, nsHTMLMediaElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLAudioElement, nsHTMLMediaElement)

DOMCI_NODE_DATA(HTMLAudioElement, nsHTMLAudioElement)

NS_INTERFACE_TABLE_HEAD(nsHTMLAudioElement)
NS_HTML_CONTENT_INTERFACE_TABLE3(nsHTMLAudioElement, nsIDOMHTMLMediaElement,
                                 nsIDOMHTMLAudioElement, nsIJSNativeInitializer)
NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLAudioElement,
                                               nsHTMLMediaElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLAudioElement)

NS_IMPL_ELEMENT_CLONE(nsHTMLAudioElement)


nsHTMLAudioElement::nsHTMLAudioElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                                       PRUint32 aFromParser)
  : nsHTMLMediaElement(aNodeInfo, aFromParser)
{
}

nsHTMLAudioElement::~nsHTMLAudioElement()
{
}

NS_IMETHODIMP
nsHTMLAudioElement::Initialize(nsISupports* aOwner, JSContext* aContext,
                               JSObject *aObj, PRUint32 argc, jsval *argv)
{
  
  
  nsresult rv = SetAttr(kNameSpaceID_None, nsGkAtoms::autobuffer,
                        NS_LITERAL_STRING("autobuffer"), PR_TRUE);
  if (NS_FAILED(rv))
    return rv;

  if (argc <= 0) {
    
    return NS_OK;
  }

  
  JSString* jsstr = JS_ValueToString(aContext, argv[0]);
  if (!jsstr)
    return NS_ERROR_FAILURE;

  nsDependentJSString str(jsstr);
  rv = SetAttr(kNameSpaceID_None, nsGkAtoms::src, str, PR_TRUE);
  if (NS_FAILED(rv))
    return rv;

  
  QueueSelectResourceTask();

  return NS_OK;
}

  
nsresult nsHTMLAudioElement::SetAcceptHeader(nsIHttpChannel* aChannel)
{
    nsCAutoString value(
#ifdef MOZ_WEBM
      "audio/webm,"
#endif
#ifdef MOZ_OGG
      "audio/ogg,"
#endif
#ifdef MOZ_WAVE
      "audio/wav,"
#endif
      "audio/*;q=0.9,"
#ifdef MOZ_OGG
      "application/ogg;q=0.7,"
#endif
      "video/*;q=0.6,*/*;q=0.5");

    return aChannel->SetRequestHeader(NS_LITERAL_CSTRING("Accept"),
                                      value,
                                      PR_FALSE);
}
