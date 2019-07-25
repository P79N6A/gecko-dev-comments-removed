




#include "nsError.h"
#include "nsIDOMHTMLAudioElement.h"
#include "nsHTMLAudioElement.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsIDocument.h"
#include "jsfriendapi.h"
#include "nsContentUtils.h"

using namespace mozilla::dom;

nsGenericHTMLElement*
NS_NewHTMLAudioElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                       FromParser aFromParser)
{
  




  nsCOMPtr<nsINodeInfo> nodeInfo(aNodeInfo);
  if (!nodeInfo) {
    nsCOMPtr<nsIDocument> doc =
      do_QueryInterface(nsContentUtils::GetDocumentFromCaller());
    NS_ENSURE_TRUE(doc, nullptr);

    nodeInfo = doc->NodeInfoManager()->GetNodeInfo(nsGkAtoms::audio, nullptr,
                                                   kNameSpaceID_XHTML,
                                                   nsIDOMNode::ELEMENT_NODE);
    NS_ENSURE_TRUE(nodeInfo, nullptr);
  }

  return new nsHTMLAudioElement(nodeInfo.forget());
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


nsHTMLAudioElement::nsHTMLAudioElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsHTMLMediaElement(aNodeInfo)
{
}

nsHTMLAudioElement::~nsHTMLAudioElement()
{
}

void
nsHTMLAudioElement::GetItemValueText(nsAString& aValue)
{
  
  GetURIAttr(nsGkAtoms::src, nullptr, aValue);
}

void
nsHTMLAudioElement::SetItemValueText(const nsAString& aValue)
{
  
  SetAttr(kNameSpaceID_None, nsGkAtoms::src, aValue, true);
}

NS_IMETHODIMP
nsHTMLAudioElement::Initialize(nsISupports* aOwner, JSContext* aContext,
                               JSObject *aObj, uint32_t argc, jsval *argv)
{
  
  
  
  nsresult rv = SetAttr(kNameSpaceID_None, nsGkAtoms::preload,
                        NS_LITERAL_STRING("auto"), true);
  if (NS_FAILED(rv))
    return rv;

  if (argc <= 0) {
    
    return NS_OK;
  }

  
  
  return SetSrc(aContext, argv[0]);
}

NS_IMETHODIMP
nsHTMLAudioElement::MozSetup(uint32_t aChannels, uint32_t aRate)
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
  nsresult rv = mAudioStream->Init(aChannels, aRate);
  if (NS_FAILED(rv)) {
    mAudioStream->Shutdown();
    mAudioStream = nullptr;
    return rv;
  }

  MetadataLoaded(aChannels, aRate, true, nullptr);
  mAudioStream->SetVolume(mVolume);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAudioElement::MozWriteAudio(const JS::Value& aData, JSContext* aCx, uint32_t* aRetVal)
{
  if (!mAudioStream) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  if (!aData.isObject()) {
    return NS_ERROR_DOM_TYPE_MISMATCH_ERR;
  }

  JSObject* darray = &aData.toObject();
  JS::AutoObjectRooter tvr(aCx);
  JSObject* tsrc = NULL;

  
  if (JS_IsFloat32Array(darray, aCx)) {
    tsrc = darray;
  } else if (JS_IsArrayObject(aCx, darray)) {
    JSObject* nobj = JS_NewFloat32ArrayFromArray(aCx, darray);
    if (!nobj) {
      return NS_ERROR_DOM_TYPE_MISMATCH_ERR;
    }
    tsrc = nobj;
  } else {
    return NS_ERROR_DOM_TYPE_MISMATCH_ERR;
  }
  tvr.setObject(tsrc);

  uint32_t dataLength = JS_GetTypedArrayLength(tsrc, aCx);

  
  
  if (dataLength % mChannels != 0) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  
  uint32_t writeLen = NS_MIN(mAudioStream->Available(), dataLength / mChannels);

  float* frames = JS_GetFloat32ArrayData(tsrc, aCx);
#ifdef MOZ_SAMPLE_TYPE_S16LE
  
  
  nsAutoArrayPtr<short> shortsArray(new short[writeLen * mChannels]);
  
  for (uint32_t i = 0; i <  writeLen * mChannels; ++i) {
    float scaled_value = floorf(0.5 + 32768 * frames[i]);
    if (frames[i] < 0.0) {
      shortsArray[i] = (scaled_value < -32768.0) ?
        -32768 :
        short(scaled_value);
    } else {
      shortsArray[i] = (scaled_value > 32767.0) ?
        32767 :
        short(scaled_value);
    }
  }
  nsresult rv = mAudioStream->Write(shortsArray, writeLen);
#else
  nsresult rv = mAudioStream->Write(frames, writeLen);
#endif


  if (NS_FAILED(rv)) {
    return rv;
  }

  
  *aRetVal = writeLen * mChannels;
  return rv;
}

NS_IMETHODIMP
nsHTMLAudioElement::MozCurrentSampleOffset(uint64_t *aRetVal)
{
  if (!mAudioStream) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  int64_t position = mAudioStream->GetPositionInFrames();
  if (position < 0) {
    *aRetVal = 0;
  } else {
    *aRetVal = position * mChannels;
  }
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
                                      false);
}
