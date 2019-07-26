




#include "nsError.h"
#include "nsIDOMHTMLAudioElement.h"
#include "mozilla/dom/HTMLAudioElement.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsIDocument.h"
#include "jsfriendapi.h"
#include "nsContentUtils.h"
#include "nsJSUtils.h"
#include "AudioSampleFormat.h"
#include "AudioChannelCommon.h"
#include <algorithm>
#include "mozilla/Preferences.h"

DOMCI_NODE_DATA(HTMLAudioElement, mozilla::dom::HTMLAudioElement)

static bool
IsAudioAPIEnabled()
{
  return mozilla::Preferences::GetBool("media.audio_data.enabled", true);
}

nsGenericHTMLElement*
NS_NewHTMLAudioElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                       mozilla::dom::FromParser aFromParser)
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

  return new mozilla::dom::HTMLAudioElement(nodeInfo.forget());
}

namespace mozilla {
namespace dom {

NS_IMPL_ADDREF_INHERITED(HTMLAudioElement, HTMLMediaElement)
NS_IMPL_RELEASE_INHERITED(HTMLAudioElement, HTMLMediaElement)

NS_INTERFACE_TABLE_HEAD(HTMLAudioElement)
NS_HTML_CONTENT_INTERFACE_TABLE3(HTMLAudioElement, nsIDOMHTMLMediaElement,
                                 nsIDOMHTMLAudioElement, nsIJSNativeInitializer)
NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLAudioElement,
                                             HTMLMediaElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLAudioElement)

NS_IMPL_ELEMENT_CLONE(HTMLAudioElement)


HTMLAudioElement::HTMLAudioElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : HTMLMediaElement(aNodeInfo)
{
}

HTMLAudioElement::~HTMLAudioElement()
{
}

NS_IMETHODIMP
HTMLAudioElement::Initialize(nsISupports* aOwner, JSContext* aContext,
                             JSObject *aObj, uint32_t argc, jsval *argv)
{
  
  
  
  nsresult rv = SetAttr(kNameSpaceID_None, nsGkAtoms::preload,
                        NS_LITERAL_STRING("auto"), true);
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

  
  
  return SetSrc(str);
}

NS_IMETHODIMP
HTMLAudioElement::MozSetup(uint32_t aChannels, uint32_t aRate)
{
  if (!IsAudioAPIEnabled()) {
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  
  if (mDecoder) {
    return NS_ERROR_FAILURE;
  }

  
  if (0 == aChannels) {
    return NS_ERROR_FAILURE;
  }

  if (mAudioStream) {
    mAudioStream->Shutdown();
  }

  mAudioStream = AudioStream::AllocateStream();
  nsresult rv = mAudioStream->Init(aChannels, aRate, mAudioChannelType);
  if (NS_FAILED(rv)) {
    mAudioStream->Shutdown();
    mAudioStream = nullptr;
    return rv;
  }

  MetadataLoaded(aChannels, aRate, true, false, nullptr);
  mAudioStream->SetVolume(mVolume);

  return NS_OK;
}

NS_IMETHODIMP
HTMLAudioElement::MozWriteAudio(const JS::Value& aData, JSContext* aCx, uint32_t* aRetVal)
{
  if (!IsAudioAPIEnabled()) {
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  if (!mAudioStream) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  if (!aData.isObject()) {
    return NS_ERROR_DOM_TYPE_MISMATCH_ERR;
  }

  JSObject* darray = &aData.toObject();
  JS::AutoObjectRooter tvr(aCx);
  JSObject* tsrc = NULL;

  
  if (JS_IsFloat32Array(darray)) {
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

  uint32_t dataLength = JS_GetTypedArrayLength(tsrc);

  
  
  if (dataLength % mChannels != 0) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  
  uint32_t writeLen = std::min(mAudioStream->Available(), dataLength / mChannels);

  float* frames = JS_GetFloat32ArrayData(tsrc);
  
  
  
  
  nsAutoArrayPtr<AudioDataValue> audioData(new AudioDataValue[writeLen * mChannels]);
  ConvertAudioSamples(frames, audioData.get(), writeLen * mChannels);
  nsresult rv = mAudioStream->Write(audioData.get(), writeLen);
  if (NS_FAILED(rv)) {
    return rv;
  }
  mAudioStream->Start();

  
  *aRetVal = writeLen * mChannels;
  return rv;
}

NS_IMETHODIMP
HTMLAudioElement::MozCurrentSampleOffset(uint64_t *aRetVal)
{
  if (!IsAudioAPIEnabled()) {
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

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

nsresult HTMLAudioElement::SetAcceptHeader(nsIHttpChannel* aChannel)
{
    nsAutoCString value(
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

} 
} 
