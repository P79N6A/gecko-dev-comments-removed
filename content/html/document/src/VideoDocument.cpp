




#include "MediaDocument.h"
#include "nsGkAtoms.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "mozilla/dom/HTMLMediaElement.h"
#include "nsIDocumentInlines.h"
#include "nsContentUtils.h"
#include "mozilla/dom/Element.h"

namespace mozilla {
namespace dom {

class VideoDocument : public MediaDocument
{
public:
  virtual nsresult StartDocumentLoad(const char*         aCommand,
                                     nsIChannel*         aChannel,
                                     nsILoadGroup*       aLoadGroup,
                                     nsISupports*        aContainer,
                                     nsIStreamListener** aDocListener,
                                     bool                aReset = true,
                                     nsIContentSink*     aSink = nullptr);
  virtual void SetScriptGlobalObject(nsIScriptGlobalObject* aScriptGlobalObject);

protected:

  
  void UpdateTitle(nsIChannel* aChannel);

  nsresult CreateSyntheticVideoDocument(nsIChannel* aChannel,
                                        nsIStreamListener** aListener);

  nsRefPtr<MediaDocumentStreamListener> mStreamListener;
};

nsresult
VideoDocument::StartDocumentLoad(const char*         aCommand,
                                 nsIChannel*         aChannel,
                                 nsILoadGroup*       aLoadGroup,
                                 nsISupports*        aContainer,
                                 nsIStreamListener** aDocListener,
                                 bool                aReset,
                                 nsIContentSink*     aSink)
{
  nsresult rv =
    MediaDocument::StartDocumentLoad(aCommand, aChannel, aLoadGroup, aContainer,
                                     aDocListener, aReset, aSink);
  NS_ENSURE_SUCCESS(rv, rv);

  mStreamListener = new MediaDocumentStreamListener(this);

  
  rv = CreateSyntheticVideoDocument(aChannel,
      getter_AddRefs(mStreamListener->mNextStream));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aDocListener = mStreamListener);
  return rv;
}

void
VideoDocument::SetScriptGlobalObject(nsIScriptGlobalObject* aScriptGlobalObject)
{
  
  
  MediaDocument::SetScriptGlobalObject(aScriptGlobalObject);

  if (aScriptGlobalObject) {
    if (!nsContentUtils::IsChildOfSameType(this) &&
        GetReadyStateEnum() != nsIDocument::READYSTATE_COMPLETE) {
      LinkStylesheet(NS_LITERAL_STRING("resource://gre/res/TopLevelVideoDocument.css"));
      LinkStylesheet(NS_LITERAL_STRING("chrome://global/skin/media/TopLevelVideoDocument.css"));
    }
    BecomeInteractive();
  }
}

nsresult
VideoDocument::CreateSyntheticVideoDocument(nsIChannel* aChannel,
                                            nsIStreamListener** aListener)
{
  
  nsresult rv = MediaDocument::CreateSyntheticDocument();
  NS_ENSURE_SUCCESS(rv, rv);

  Element* body = GetBodyElement();
  if (!body) {
    NS_WARNING("no body on video document!");
    return NS_ERROR_FAILURE;
  }

  
  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::video, nullptr,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_FAILURE);

  nsRefPtr<HTMLMediaElement> element =
    static_cast<HTMLMediaElement*>(NS_NewHTMLVideoElement(nodeInfo.forget(),
                                                          NOT_FROM_PARSER));
  if (!element)
    return NS_ERROR_OUT_OF_MEMORY;
  element->SetAutoplay(true);
  element->SetControls(true);
  element->LoadWithChannel(aChannel, aListener);
  UpdateTitle(aChannel);

  if (nsContentUtils::IsChildOfSameType(this)) {
    
    
    element->SetAttr(kNameSpaceID_None, nsGkAtoms::style,
        NS_LITERAL_STRING("position:absolute; top:0; left:0; width:100%; height:100%"),
        true);
  }

  return body->AppendChildTo(element, false);
}

void
VideoDocument::UpdateTitle(nsIChannel* aChannel)
{
  if (!aChannel)
    return;

  nsAutoString fileName;
  GetFileName(fileName);
  SetTitle(fileName);
}

} 
} 

nsresult
NS_NewVideoDocument(nsIDocument** aResult)
{
  mozilla::dom::VideoDocument* doc = new mozilla::dom::VideoDocument();

  NS_ADDREF(doc);
  nsresult rv = doc->Init();

  if (NS_FAILED(rv)) {
    NS_RELEASE(doc);
  }

  *aResult = doc;

  return rv;
}
