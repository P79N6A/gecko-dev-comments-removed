




#if !defined(nsHTMLAudioElement_h__)
#define nsHTMLAudioElement_h__

#include "nsIDOMHTMLAudioElement.h"
#include "nsIJSNativeInitializer.h"
#include "mozilla/dom/HTMLMediaElement.h"

typedef uint16_t nsMediaNetworkState;
typedef uint16_t nsMediaReadyState;

class nsHTMLAudioElement : public mozilla::dom::HTMLMediaElement,
                           public nsIDOMHTMLAudioElement,
                           public nsIJSNativeInitializer
{
public:
  nsHTMLAudioElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLAudioElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  using mozilla::dom::HTMLMediaElement::GetPaused;
  NS_FORWARD_NSIDOMHTMLMEDIAELEMENT(mozilla::dom::HTMLMediaElement::)

  
  NS_DECL_NSIDOMHTMLAUDIOELEMENT

  
  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext* aContext,
                        JSObject* aObj, uint32_t argc, jsval* argv);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsresult SetAcceptHeader(nsIHttpChannel* aChannel);

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
};

#endif
