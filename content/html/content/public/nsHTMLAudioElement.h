




































#if !defined(nsHTMLAudioElement_h__)
#define nsHTMLAudioElement_h__

#include "nsIDOMHTMLAudioElement.h"
#include "nsIJSNativeInitializer.h"
#include "nsHTMLMediaElement.h"

typedef PRUint16 nsMediaNetworkState;
typedef PRUint16 nsMediaReadyState;

class nsHTMLAudioElement : public nsHTMLMediaElement,
                           public nsIDOMHTMLAudioElement,
                           public nsIJSNativeInitializer
{
public:
  nsHTMLAudioElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLAudioElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsHTMLMediaElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsHTMLMediaElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsHTMLMediaElement::)

  
  NS_FORWARD_NSIDOMHTMLMEDIAELEMENT(nsHTMLMediaElement::)

  
  NS_DECL_NSIDOMHTMLAUDIOELEMENT

  
  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext* aContext,
                        JSObject* aObj, PRUint32 argc, jsval* argv);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsresult SetAcceptHeader(nsIHttpChannel* aChannel);

  virtual nsXPCClassInfo* GetClassInfo();
};

#endif
