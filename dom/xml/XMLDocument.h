




#ifndef mozilla_dom_XMLDocument_h
#define mozilla_dom_XMLDocument_h

#include "mozilla/Attributes.h"
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
  explicit XMLDocument(const char* aContentType = "application/xml");

  NS_DECL_ISUPPORTS_INHERITED

  virtual void Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup) MOZ_OVERRIDE;
  virtual void ResetToURI(nsIURI *aURI, nsILoadGroup *aLoadGroup,
                          nsIPrincipal* aPrincipal) MOZ_OVERRIDE;

  virtual nsresult StartDocumentLoad(const char* aCommand, nsIChannel* channel,
                                     nsILoadGroup* aLoadGroup,
                                     nsISupports* aContainer,
                                     nsIStreamListener **aDocListener,
                                     bool aReset = true,
                                     nsIContentSink* aSink = nullptr) MOZ_OVERRIDE;

  virtual void EndLoad() MOZ_OVERRIDE;

  
  NS_DECL_NSIDOMXMLDOCUMENT

  virtual nsresult Init() MOZ_OVERRIDE;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  virtual void DocAddSizeOfExcludingThis(nsWindowSizes* aWindowSizes) const MOZ_OVERRIDE;
  


  
  bool Load(const nsAString& aUrl, mozilla::ErrorResult& aRv);
  bool Async() const
  {
    return mAsync;
  }
  

  
  
  
  using nsIDocument::GetLocation;
  
  
  using nsDocument::GetLocation;

protected:
  virtual ~XMLDocument();

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

  friend nsresult (::NS_NewXMLDocument)(nsIDocument**, bool, bool);


  
  
  
  
  
  
  bool mChannelIsPending;
  bool mAsync;
  bool mLoopingForSyncLoad;

  
  bool mIsPlainDocument;
};

} 
} 

#endif 
