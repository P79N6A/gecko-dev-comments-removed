




#ifndef mozilla_dom_HTMLAudioElement_h
#define mozilla_dom_HTMLAudioElement_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/TypedArray.h"

typedef uint16_t nsMediaNetworkState;
typedef uint16_t nsMediaReadyState;

namespace mozilla {
namespace dom {

class HTMLAudioElement MOZ_FINAL : public HTMLMediaElement
{
public:
  typedef mozilla::dom::NodeInfo NodeInfo;

  explicit HTMLAudioElement(already_AddRefed<NodeInfo>& aNodeInfo);

  
  using HTMLMediaElement::GetPaused;

  virtual nsresult Clone(NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;
  virtual nsresult SetAcceptHeader(nsIHttpChannel* aChannel) MOZ_OVERRIDE;

  virtual nsIDOMNode* AsDOMNode() MOZ_OVERRIDE { return this; }

  

  static already_AddRefed<HTMLAudioElement>
  Audio(const GlobalObject& aGlobal,
        const Optional<nsAString>& aSrc, ErrorResult& aRv);

protected:
  virtual ~HTMLAudioElement();

  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;
};

} 
} 

#endif 
