




#ifndef mozilla_dom_HTMLAudioElement_h
#define mozilla_dom_HTMLAudioElement_h

#include "nsIDOMHTMLAudioElement.h"
#include "nsIJSNativeInitializer.h"
#include "mozilla/dom/HTMLMediaElement.h"

typedef uint16_t nsMediaNetworkState;
typedef uint16_t nsMediaReadyState;

namespace mozilla {
namespace dom {

class HTMLAudioElement : public HTMLMediaElement,
                         public nsIDOMHTMLAudioElement,
                         public nsIJSNativeInitializer
{
public:
  HTMLAudioElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLAudioElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  using HTMLMediaElement::GetPaused;
  NS_FORWARD_NSIDOMHTMLMEDIAELEMENT(HTMLMediaElement::)

  
  NS_DECL_NSIDOMHTMLAUDIOELEMENT

  
  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext* aContext,
                        JSObject* aObj, uint32_t argc, jsval* argv);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsresult SetAcceptHeader(nsIHttpChannel* aChannel);

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
};

} 
} 

#endif 
