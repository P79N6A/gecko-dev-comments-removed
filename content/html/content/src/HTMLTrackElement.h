




#ifndef mozilla_dom_HTMLTrackElement_h
#define mozilla_dom_HTMLTrackElement_h

#define WEBVTT_NO_CONFIG_H 1
#define WEBVTT_STATIC 1

#include "mozilla/Attributes.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/TextTrack.h"
#include "nsCycleCollectionParticipant.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMEventTarget.h"
#include "nsIHttpChannel.h"

namespace mozilla {
namespace dom {

class WebVTTLoadListener;

class HTMLTrackElement MOZ_FINAL : public nsGenericHTMLElement
                                 , public nsIDOMHTMLElement
{
public:
  HTMLTrackElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLTrackElement();

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLTrackElement,
                                           nsGenericHTMLElement)

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  TextTrackKind Kind() const;
  void SetKind(TextTrackKind aKind, ErrorResult& aError);

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
  void SetSrclang(const nsAString& aSrclang, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::srclang, aSrclang, aError);
  }

  void GetLabel(DOMString& aLabel) const
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

  
  enum {
    NONE = 0U,
    LOADING = 1U,
    LOADED = 2U,
    ERROR = 3U
  };
  uint16_t ReadyState() const
  {
    return mReadyState;
  }

  TextTrack* Track();

  virtual nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const MOZ_OVERRIDE;
  virtual nsIDOMNode* AsDOMNode() MOZ_OVERRIDE { return this; }

  
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

protected:
  virtual JSObject* WrapNode(JSContext* aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;
  void OnChannelRedirect(nsIChannel* aChannel, nsIChannel* aNewChannel,
                         uint32_t aFlags);
  
  
  
  void LoadResource();

  friend class TextTrackCue;
  friend class WebVTTLoadListener;

  nsRefPtr<TextTrack> mTrack;
  nsCOMPtr<nsIChannel> mChannel;
  nsRefPtr<HTMLMediaElement> mMediaParent;
  nsRefPtr<WebVTTLoadListener> mLoadListener;
  uint16_t mReadyState;

  void CreateTextTrack();
};

} 
} 

#endif 
