











































#include "nsContentCreatorFunctions.h"
#include "nsXMLElement.h"
#include "nsImageLoadingContent.h"
#include "imgIRequest.h"
#include "nsIEventStateManager.h"

class nsGenConImageContent : public nsXMLElement,
                             public nsImageLoadingContent
{
public:
  nsGenConImageContent(nsINodeInfo* aNodeInfo)
    : nsXMLElement(aNodeInfo)
  {
  }

  nsresult Init(imgIRequest* aImageRequest)
  {
    
    return UseAsPrimaryRequest(aImageRequest, PR_FALSE);
  }

  
  virtual PRInt32 IntrinsicState() const;
  
private:
  virtual ~nsGenConImageContent();

public:
  NS_DECL_ISUPPORTS_INHERITED
};

NS_IMPL_ISUPPORTS_INHERITED2(nsGenConImageContent, nsXMLElement,
                             nsIImageLoadingContent, imgIDecoderObserver)

nsresult
NS_NewGenConImageContent(nsIContent** aResult, nsINodeInfo* aNodeInfo,
                         imgIRequest* aImageRequest)
{
  NS_PRECONDITION(aImageRequest, "Must have request!");
  nsGenConImageContent *it = new nsGenConImageContent(aNodeInfo);
  if (!it)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult = it);
  nsresult rv = it->Init(aImageRequest);
  if (NS_FAILED(rv))
    NS_RELEASE(*aResult);
  return rv;
}

nsGenConImageContent::~nsGenConImageContent()
{
  DestroyImageLoadingContent();
}

PRInt32
nsGenConImageContent::IntrinsicState() const
{
  PRInt32 state = nsXMLElement::IntrinsicState();

  PRInt32 imageState = nsImageLoadingContent::ImageState();
  if (imageState & NS_EVENT_STATE_BROKEN) {
    
    
    imageState |= NS_EVENT_STATE_SUPPRESSED;
    imageState &= ~NS_EVENT_STATE_BROKEN;
  }
  imageState &= ~NS_EVENT_STATE_LOADING;
  return state | imageState;
}
