




































#include "nsMediaDocument.h"
#include "nsGkAtoms.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsHTMLMediaElement.h"
#include "nsIDocShellTreeItem.h"

class nsVideoDocument : public nsMediaDocument
{
public:
  virtual nsresult StartDocumentLoad(const char*         aCommand,
                                     nsIChannel*         aChannel,
                                     nsILoadGroup*       aLoadGroup,
                                     nsISupports*        aContainer,
                                     nsIStreamListener** aDocListener,
                                     PRBool              aReset = PR_TRUE,
                                     nsIContentSink*     aSink = nsnull);

protected:

  
  void UpdateTitle(nsIChannel* aChannel);

  nsresult CreateSyntheticVideoDocument(nsIChannel* aChannel,
                                        nsIStreamListener** aListener);

  nsRefPtr<nsMediaDocumentStreamListener> mStreamListener;
};

nsresult
nsVideoDocument::StartDocumentLoad(const char*         aCommand,
                                   nsIChannel*         aChannel,
                                   nsILoadGroup*       aLoadGroup,
                                   nsISupports*        aContainer,
                                   nsIStreamListener** aDocListener,
                                   PRBool              aReset,
                                   nsIContentSink*     aSink)
{
  nsresult rv =
    nsMediaDocument::StartDocumentLoad(aCommand, aChannel, aLoadGroup,
                                       aContainer, aDocListener, aReset,
                                       aSink);
  NS_ENSURE_SUCCESS(rv, rv);

  mStreamListener = new nsMediaDocumentStreamListener(this);
  if (!mStreamListener)
    return NS_ERROR_OUT_OF_MEMORY;

  
  rv = CreateSyntheticVideoDocument(aChannel,
      getter_AddRefs(mStreamListener->mNextStream));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aDocListener = mStreamListener);
  return rv;
}

nsresult
nsVideoDocument::CreateSyntheticVideoDocument(nsIChannel* aChannel,
                                              nsIStreamListener** aListener)
{
  
  nsresult rv = nsMediaDocument::CreateSyntheticDocument();
  NS_ENSURE_SUCCESS(rv, rv);

  nsIContent* body = GetBodyContent();
  if (!body) {
    NS_WARNING("no body on video document!");
    return NS_ERROR_FAILURE;
  }

  
  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::video, nsnull,
                                           kNameSpaceID_XHTML);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_FAILURE);

  nsRefPtr<nsHTMLMediaElement> element =
    static_cast<nsHTMLMediaElement*>(NS_NewHTMLVideoElement(nodeInfo, PR_FALSE));
  if (!element)
    return NS_ERROR_OUT_OF_MEMORY;
  element->SetAutoplay(PR_TRUE);
  element->SetControls(PR_TRUE);
  element->LoadWithChannel(aChannel, aListener);
  UpdateTitle(aChannel);

  if (nsContentUtils::IsChildOfSameType(this)) {
    
    
    element->SetAttr(kNameSpaceID_None, nsGkAtoms::style,
        NS_LITERAL_STRING("position:absolute; top:0; left:0; width:100%; height:100%"),
        PR_TRUE);
  }

  return body->AppendChildTo(element, PR_FALSE);
}

void
nsVideoDocument::UpdateTitle(nsIChannel* aChannel)
{
  if (!aChannel)
    return;

  nsAutoString fileName;
  GetFileName(fileName);
  SetTitle(fileName);
}

nsresult
NS_NewVideoDocument(nsIDocument** aResult)
{
  nsVideoDocument* doc = new nsVideoDocument();
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
