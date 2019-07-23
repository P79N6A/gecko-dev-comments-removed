




































#ifndef nsXMLDocument_h___
#define nsXMLDocument_h___

#include "nsDocument.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIChannelEventSink.h"
#include "nsIDOMXMLDocument.h"
#include "nsIScriptContext.h"
#include "nsHTMLStyleSheet.h"
#include "nsIHTMLCSSStyleSheet.h"

class nsIParser;
class nsIDOMNode;
class nsIURI;
class nsIChannel;

class nsXMLDocument : public nsDocument,
                      public nsIInterfaceRequestor,
                      public nsIChannelEventSink
{
public:
  nsXMLDocument(const char* aContentType = "application/xml");
  virtual ~nsXMLDocument();

  NS_DECL_ISUPPORTS_INHERITED

  virtual void Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup);
  virtual void ResetToURI(nsIURI *aURI, nsILoadGroup *aLoadGroup,
                          nsIPrincipal* aPrincipal);

  virtual nsresult StartDocumentLoad(const char* aCommand, nsIChannel* channel,
                                     nsILoadGroup* aLoadGroup,
                                     nsISupports* aContainer,
                                     nsIStreamListener **aDocListener,
                                     PRBool aReset = PR_TRUE,
                                     nsIContentSink* aSink = nsnull);

  virtual void EndLoad();

  
  NS_IMETHOD CloneNode(PRBool aDeep, nsIDOMNode** aReturn);

  
  NS_IMETHOD GetElementById(const nsAString& aElementId,
                            nsIDOMElement** aReturn);

  
  NS_DECL_NSIINTERFACEREQUESTOR

  
  NS_DECL_NSICHANNELEVENTSINK

  
  NS_DECL_NSIDOMXMLDOCUMENT

  virtual nsresult Init();

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsXMLDocument, nsDocument)

protected:
  virtual nsresult GetLoadGroup(nsILoadGroup **aLoadGroup);

  nsCOMPtr<nsIScriptContext> mScriptContext;

  
  
  
  
  
  
  PRPackedBool mChannelIsPending;
  PRPackedBool mCrossSiteAccessEnabled;
  PRPackedBool mLoadedAsInteractiveData;
  PRPackedBool mAsync;
  PRPackedBool mLoopingForSyncLoad;
};


#endif 
