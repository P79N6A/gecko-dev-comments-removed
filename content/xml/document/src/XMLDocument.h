




#ifndef mozilla_dom_XMLDocument_h
#define mozilla_dom_XMLDocument_h

#include "nsDocument.h"
#include "nsIDOMXMLDocument.h"
#include "nsIScriptContext.h"

class nsIParser;
class nsIDOMNode;
class nsIURI;
class nsIChannel;

namespace mozilla {
namespace dom {

class XMLDocument : public nsDocument
{
public:
  XMLDocument(const char* aContentType = "application/xml");
  virtual ~XMLDocument();

  NS_DECL_ISUPPORTS_INHERITED

  virtual void Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup);
  virtual void ResetToURI(nsIURI *aURI, nsILoadGroup *aLoadGroup,
                          nsIPrincipal* aPrincipal);

  virtual nsresult StartDocumentLoad(const char* aCommand, nsIChannel* channel,
                                     nsILoadGroup* aLoadGroup,
                                     nsISupports* aContainer,
                                     nsIStreamListener **aDocListener,
                                     bool aReset = true,
                                     nsIContentSink* aSink = nullptr);

  virtual void EndLoad();

  
  NS_DECL_NSIDOMXMLDOCUMENT

  virtual nsresult Init();

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual void DocSizeOfExcludingThis(nsWindowSizes* aWindowSizes) const;
  

protected:
  
  
  
  
  
  
  bool mChannelIsPending;
  bool mAsync;
  bool mLoopingForSyncLoad;
};

} 
} 

#endif 
