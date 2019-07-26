




#ifndef mozilla_dom_HTMLAudioElement_h
#define mozilla_dom_HTMLAudioElement_h

#include "mozilla/Attributes.h"
#include "nsIDOMHTMLAudioElement.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/TypedArray.h"

typedef uint16_t nsMediaNetworkState;
typedef uint16_t nsMediaReadyState;

namespace mozilla {
namespace dom {

class HTMLAudioElement MOZ_FINAL : public HTMLMediaElement,
                                   public nsIDOMHTMLAudioElement
{
public:
  HTMLAudioElement(already_AddRefed<nsINodeInfo>& aNodeInfo);
  virtual ~HTMLAudioElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  using HTMLMediaElement::GetPaused;
  NS_FORWARD_NSIDOMHTMLMEDIAELEMENT(HTMLMediaElement::)

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsresult SetAcceptHeader(nsIHttpChannel* aChannel);

  virtual nsIDOMNode* AsDOMNode() MOZ_OVERRIDE { return this; }

  

  static already_AddRefed<HTMLAudioElement>
  Audio(const GlobalObject& aGlobal,
        const Optional<nsAString>& aSrc, ErrorResult& aRv);

protected:
  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;
};

} 
} 

#endif 
