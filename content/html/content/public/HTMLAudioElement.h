




#ifndef mozilla_dom_HTMLAudioElement_h
#define mozilla_dom_HTMLAudioElement_h

#include "mozilla/Attributes.h"
#include "nsIDOMHTMLAudioElement.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/TypedArray.h"

class nsITimer;

typedef uint16_t nsMediaNetworkState;
typedef uint16_t nsMediaReadyState;

namespace mozilla {
namespace dom {

class HTMLAudioElement MOZ_FINAL : public HTMLMediaElement,
                                   public nsITimerCallback,
                                   public nsIDOMHTMLAudioElement
{
public:
  HTMLAudioElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLAudioElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  using HTMLMediaElement::GetPaused;
  NS_FORWARD_NSIDOMHTMLMEDIAELEMENT(HTMLMediaElement::)

  
  NS_DECL_NSIAUDIOCHANNELAGENTCALLBACK

  
  NS_DECL_NSITIMERCALLBACK

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsresult SetAcceptHeader(nsIHttpChannel* aChannel);

  virtual nsIDOMNode* AsDOMNode() MOZ_OVERRIDE { return this; }

  

  static already_AddRefed<HTMLAudioElement>
  Audio(const GlobalObject& aGlobal,
        const Optional<nsAString>& aSrc, ErrorResult& aRv);

  void MozSetup(uint32_t aChannels, uint32_t aRate, ErrorResult& aRv);

  uint32_t MozWriteAudio(const Float32Array& aData, ErrorResult& aRv)
  {
    return MozWriteAudio(aData.Data(), aData.Length(), aRv);
  }
  uint32_t MozWriteAudio(const Sequence<float>& aData, ErrorResult& aRv)
  {
    return MozWriteAudio(aData.Elements(), aData.Length(), aRv);
  }
  uint32_t MozWriteAudio(const float* aData, uint32_t aLength,
                         ErrorResult& aRv);

  uint64_t MozCurrentSampleOffset(ErrorResult& aRv);

protected:
  virtual JSObject* WrapNode(JSContext* aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  
  virtual void UpdateAudioChannelPlayingState() MOZ_OVERRIDE;

  
  
  nsCOMPtr<nsITimer> mDeferStopPlayTimer;
  
  bool mTimerActivated;
};

} 
} 

#endif 
