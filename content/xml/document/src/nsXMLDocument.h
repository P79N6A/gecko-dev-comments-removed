




































#ifndef nsXMLDocument_h___
#define nsXMLDocument_h___

#include "nsDocument.h"
#include "nsIDOMXMLDocument.h"
#include "nsIScriptContext.h"
#include "nsHTMLStyleSheet.h"
#include "nsIHTMLCSSStyleSheet.h"

class nsIParser;
class nsIDOMNode;
class nsIURI;
class nsIChannel;

class nsXMLDocument : public nsDocument
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

  
  NS_DECL_NSIDOMXMLDOCUMENT

  virtual nsresult Init();

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:
  
  
  
  
  
  
  PRPackedBool mChannelIsPending;
  PRPackedBool mLoadedAsInteractiveData;
  PRPackedBool mAsync;
  PRPackedBool mLoopingForSyncLoad;
};


#endif 
