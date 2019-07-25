




































#include "MediaDocument.h"
#include "nsGkAtoms.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsHTMLMediaElement.h"
#include "nsIDocShellTreeItem.h"
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
                                     nsIContentSink*     aSink = nsnull);

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
  if (!mStreamListener)
    return NS_ERROR_OUT_OF_MEMORY;

  
  rv = CreateSyntheticVideoDocument(aChannel,
      getter_AddRefs(mStreamListener->mNextStream));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aDocListener = mStreamListener);
  return rv;
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
  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::video, nsnull,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_FAILURE);

  nsRefPtr<nsHTMLMediaElement> element =
    static_cast<nsHTMLMediaElement*>(NS_NewHTMLVideoElement(nodeInfo.forget(),
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
  } else {
    Element* head = GetHeadElement();
    if (!head) {
      NS_WARNING("no head on video document!");
      return NS_ERROR_FAILURE;
    }

    nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::style, nsnull,
                                             kNameSpaceID_XHTML,
                                             nsIDOMNode::ELEMENT_NODE);
    NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);
    nsRefPtr<nsGenericHTMLElement> styleContent = NS_NewHTMLStyleElement(nodeInfo.forget());
    NS_ENSURE_TRUE(styleContent, NS_ERROR_OUT_OF_MEMORY);

    styleContent->SetTextContent(
      NS_LITERAL_STRING("body { background: url(chrome://global/skin/icons/tabprompts-bgtexture.png) #333; height: 100%; width: 100%; margin: 0; padding: 0; } ") +
      NS_LITERAL_STRING("video { position: absolute; top: 0; right: 0; bottom: 0; left: 0; margin: auto; box-shadow: 0 0 15px #000; } ") +
      NS_LITERAL_STRING("video:focus { outline-width: 0; } "));
    head->AppendChildTo(styleContent, false);
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
  if (!doc) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(doc);
  nsresult rv = doc->Init();

  if (NS_FAILED(rv)) {
    NS_RELEASE(doc);
  }

  *aResult = doc;

  return rv;
}
