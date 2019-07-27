




#ifndef mozilla_dom_HTMLAudioElement_h
#define mozilla_dom_HTMLAudioElement_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/TypedArray.h"

typedef uint16_t nsMediaNetworkState;
typedef uint16_t nsMediaReadyState;

namespace mozilla {
namespace dom {

class HTMLAudioElement final : public HTMLMediaElement
{
public:
  typedef mozilla::dom::NodeInfo NodeInfo;

  explicit HTMLAudioElement(already_AddRefed<NodeInfo>& aNodeInfo);

  
  virtual bool IsInteractiveHTMLContent(bool aIgnoreTabindex) const override;

  
  using HTMLMediaElement::GetPaused;

  virtual nsresult Clone(NodeInfo *aNodeInfo, nsINode **aResult) const override;
  virtual nsresult SetAcceptHeader(nsIHttpChannel* aChannel) override;

  virtual nsIDOMNode* AsDOMNode() override { return this; }

  

  static already_AddRefed<HTMLAudioElement>
  Audio(const GlobalObject& aGlobal,
        const Optional<nsAString>& aSrc, ErrorResult& aRv);

protected:
  virtual ~HTMLAudioElement();

  virtual JSObject* WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;
};

} 
} 

#endif 
