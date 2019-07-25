




































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
#include "jscntxt.h"
#include "jstypedarray.h"
#include "nsJSUtils.h"

#include "nsITimer.h"

#include "nsEventDispatcher.h"
#include "nsIDOMProgressEvent.h"

using namespace mozilla::dom;

nsGenericHTMLElement*
NS_NewHTMLAudioElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                       FromParser aFromParser)
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
                                       FromParser aFromParser)
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
  
  
  
  nsresult rv = SetAttr(kNameSpaceID_None, nsGkAtoms::preload,
                        NS_LITERAL_STRING("auto"), PR_TRUE);
  if (NS_FAILED(rv))
    return rv;

  if (argc <= 0) {
    
    return NS_OK;
  }

  
  JSString* jsstr = JS_ValueToString(aContext, argv[0]);
  if (!jsstr)
    return NS_ERROR_FAILURE;

  nsDependentJSString str;
  if (!str.init(aContext, jsstr))
    return NS_ERROR_FAILURE;

  rv = SetAttr(kNameSpaceID_None, nsGkAtoms::src, str, PR_TRUE);
  if (NS_FAILED(rv))
    return rv;

  
  QueueSelectResourceTask();

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAudioElement::MozSetup(PRUint32 aChannels, PRUint32 aRate)
{
  
  if (mDecoder) {
    return NS_ERROR_FAILURE;
  }

  
  if (0 == aChannels) {
    return NS_ERROR_FAILURE;
  }

  if (mAudioStream) {
    mAudioStream->Shutdown();
  }

  mAudioStream = nsAudioStream::AllocateStream();
  nsresult rv = mAudioStream->Init(aChannels, aRate,
                                   nsAudioStream::FORMAT_FLOAT32);
  if (NS_FAILED(rv)) {
    mAudioStream->Shutdown();
    mAudioStream = nsnull;
    return rv;
  }

  MetadataLoaded(aChannels, aRate);
  mAudioStream->SetVolume(mVolume);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAudioElement::MozWriteAudio(const jsval &aData, JSContext *aCx, PRUint32 *aRetVal)
{
  if (!mAudioStream) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  if (JSVAL_IS_PRIMITIVE(aData)) {
    return NS_ERROR_DOM_TYPE_MISMATCH_ERR;
  }

  JSObject *darray = JSVAL_TO_OBJECT(aData);
  js::AutoValueRooter tsrc_tvr(aCx);
  js::TypedArray *tsrc = NULL;

  
  if (darray->getClass() == &js::TypedArray::fastClasses[js::TypedArray::TYPE_FLOAT32])
  {
    tsrc = js::TypedArray::fromJSObject(darray);
  } else if (JS_IsArrayObject(aCx, darray)) {
    JSObject *nobj = js_CreateTypedArrayWithArray(aCx, js::TypedArray::TYPE_FLOAT32, darray);
    if (!nobj) {
      return NS_ERROR_DOM_TYPE_MISMATCH_ERR;
    }
    *tsrc_tvr.jsval_addr() = OBJECT_TO_JSVAL(nobj);
    tsrc = js::TypedArray::fromJSObject(nobj);
  } else {
    return NS_ERROR_DOM_TYPE_MISMATCH_ERR;
  }

  PRUint32 dataLength = tsrc->length;

  
  
  if (dataLength % mChannels != 0) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  
  PRUint32 writeLen = NS_MIN(mAudioStream->Available(), dataLength);

  nsresult rv = mAudioStream->Write(tsrc->data, writeLen, PR_TRUE);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  *aRetVal = writeLen;
  return rv;
}

NS_IMETHODIMP
nsHTMLAudioElement::MozCurrentSampleOffset(PRUint64 *aRetVal)
{
  if (!mAudioStream) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  *aRetVal = mAudioStream->GetSampleOffset();
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
