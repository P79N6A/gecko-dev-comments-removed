





#ifndef mozilla_dom_HTMLVideoElement_h
#define mozilla_dom_HTMLVideoElement_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/HTMLMediaElement.h"

namespace mozilla {
namespace dom {

class WakeLock;
class VideoPlaybackQuality;

class HTMLVideoElement final : public HTMLMediaElement
{
public:
  typedef mozilla::dom::NodeInfo NodeInfo;

  explicit HTMLVideoElement(already_AddRefed<NodeInfo>& aNodeInfo);

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLVideoElement, video)

  using HTMLMediaElement::GetPaused;

  NS_IMETHOD_(bool) IsVideo() override {
    return true;
  }

  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult) override;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const override;

  static void Init();

  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const override;

  virtual nsresult Clone(NodeInfo *aNodeInfo, nsINode **aResult) const override;

  
  
  nsresult GetVideoSize(nsIntSize* size);

  virtual nsresult SetAcceptHeader(nsIHttpChannel* aChannel) override;

  
  virtual bool IsInteractiveHTMLContent(bool aIgnoreTabindex) const override;

  

  uint32_t Width() const
  {
    return GetIntAttr(nsGkAtoms::width, 0);
  }

  void SetWidth(uint32_t aValue, ErrorResult& aRv)
  {
    SetHTMLIntAttr(nsGkAtoms::width, aValue, aRv);
  }

  uint32_t Height() const
  {
    return GetIntAttr(nsGkAtoms::height, 0);
  }

  void SetHeight(uint32_t aValue, ErrorResult& aRv)
  {
    SetHTMLIntAttr(nsGkAtoms::height, aValue, aRv);
  }

  uint32_t VideoWidth() const
  {
    return mMediaInfo.HasVideo() ? mMediaInfo.mVideo.mDisplay.width : 0;
  }

  uint32_t VideoHeight() const
  {
    return mMediaInfo.HasVideo() ? mMediaInfo.mVideo.mDisplay.height : 0;
  }

  void GetPoster(nsAString& aValue)
  {
    GetURIAttr(nsGkAtoms::poster, nullptr, aValue);
  }
  void SetPoster(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::poster, aValue, aRv);
  }

  uint32_t MozParsedFrames() const;

  uint32_t MozDecodedFrames() const;

  uint32_t MozPresentedFrames() const;

  uint32_t MozPaintedFrames();

  double MozFrameDelay();

  bool MozHasAudio() const;

  bool NotifyOwnerDocumentActivityChangedInternal() override;

  already_AddRefed<VideoPlaybackQuality> GetVideoPlaybackQuality();

protected:
  virtual ~HTMLVideoElement();

  virtual JSObject* WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  virtual void WakeLockCreate() override;
  virtual void WakeLockRelease() override;
  void UpdateScreenWakeLock();

  nsRefPtr<WakeLock> mScreenWakeLock;

private:
  static void MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                    nsRuleData* aData);
};

} 
} 

#endif 
