




#ifndef mozilla_dom_HTMLTrackElement_h
#define mozilla_dom_HTMLTrackElement_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/TextTrack.h"
#include "nsCycleCollectionParticipant.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMEventTarget.h"
#include "nsIHttpChannel.h"

class nsIContent;
class nsIDocument;

namespace mozilla {
namespace dom {

class WebVTTListener;

class HTMLTrackElement MOZ_FINAL : public nsGenericHTMLElement
{
public:
  explicit HTMLTrackElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLTrackElement,
                                           nsGenericHTMLElement)

  
  void GetKind(DOMString& aKind) const;
  void SetKind(const nsAString& aKind, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::kind, aKind, aError);
  }

  void GetSrc(DOMString& aSrc) const
  {
    GetHTMLURIAttr(nsGkAtoms::src, aSrc);
  }
  void SetSrc(const nsAString& aSrc, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::src, aSrc, aError);
  }

  void GetSrclang(DOMString& aSrclang) const
  {
    GetHTMLAttr(nsGkAtoms::srclang, aSrclang);
  }
  void GetSrclang(nsString& aSrclang) const
  {
    GetHTMLAttr(nsGkAtoms::srclang, aSrclang);
  }
  void SetSrclang(const nsAString& aSrclang, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::srclang, aSrclang, aError);
  }

  void GetLabel(DOMString& aLabel) const
  {
    GetHTMLAttr(nsGkAtoms::label, aLabel);
  }
  void GetLabel(nsString& aLabel) const
  {
    GetHTMLAttr(nsGkAtoms::label, aLabel);
  }
  void SetLabel(const nsAString& aLabel, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::label, aLabel, aError);
  }

  bool Default() const
  {
    return GetBoolAttr(nsGkAtoms::_default);
  }
  void SetDefault(bool aDefault, ErrorResult& aError)
  {
    SetHTMLBoolAttr(nsGkAtoms::_default, aDefault, aError);
  }

  uint16_t ReadyState() const;
  void SetReadyState(uint16_t aReadyState);

  TextTrack* GetTrack();

  virtual nsresult Clone(mozilla::dom::NodeInfo* aNodeInfo, nsINode** aResult) const MOZ_OVERRIDE;

  
  virtual void GetItemValueText(nsAString& aText) MOZ_OVERRIDE
  {
    DOMString value;
    GetSrc(value);
    aText = value;
  }
  virtual void SetItemValueText(const nsAString& aText) MOZ_OVERRIDE
  {
    ErrorResult rv;
    SetSrc(aText, rv);
  }

  
  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult) MOZ_OVERRIDE;

  
  
  virtual nsresult BindToTree(nsIDocument* aDocument,
                              nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) MOZ_OVERRIDE;
  virtual void UnbindFromTree(bool aDeep, bool aNullParent) MOZ_OVERRIDE;

  
  static bool IsWebVTTEnabled();

  void DispatchTrackRunnable(const nsString& aEventName);
  void DispatchTrustedEvent(const nsAString& aName);

  void DropChannel();

protected:
  virtual ~HTMLTrackElement();

  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;
  void OnChannelRedirect(nsIChannel* aChannel, nsIChannel* aNewChannel,
                         uint32_t aFlags);
  
  
  void LoadResource();

  friend class TextTrackCue;
  friend class WebVTTListener;

  nsRefPtr<TextTrack> mTrack;
  nsCOMPtr<nsIChannel> mChannel;
  nsRefPtr<HTMLMediaElement> mMediaParent;
  nsRefPtr<WebVTTListener> mListener;

  void CreateTextTrack();
};

} 
} 

#endif 
