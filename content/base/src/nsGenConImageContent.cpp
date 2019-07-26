











#include "nsContentCreatorFunctions.h"
#include "nsXMLElement.h"
#include "nsImageLoadingContent.h"
#include "imgIRequest.h"
#include "nsEventStates.h"

class nsGenConImageContent : public nsXMLElement,
                             public nsImageLoadingContent
{
public:
  nsGenConImageContent(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsXMLElement(aNodeInfo)
  {
    
    
    AddStatesSilently(NS_EVENT_STATE_SUPPRESSED);
  }

  nsresult Init(imgIRequest* aImageRequest)
  {
    
    return UseAsPrimaryRequest(aImageRequest, false);
  }

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep, bool aNullParent);
  virtual nsEventStates IntrinsicState() const;
  
private:
  virtual ~nsGenConImageContent();

public:
  NS_DECL_ISUPPORTS_INHERITED
};

NS_IMPL_ISUPPORTS_INHERITED4(nsGenConImageContent,
                             nsXMLElement,
                             nsIImageLoadingContent,
                             imgIContainerObserver,
                             imgIDecoderObserver,
                             imgIOnloadBlocker)

nsresult
NS_NewGenConImageContent(nsIContent** aResult, already_AddRefed<nsINodeInfo> aNodeInfo,
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

nsresult
nsGenConImageContent::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                                 nsIContent* aBindingParent,
                                 bool aCompileEventHandlers)
{
  nsresult rv;
  rv = nsXMLElement::BindToTree(aDocument, aParent, aBindingParent,
                                aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  nsImageLoadingContent::BindToTree(aDocument, aParent, aBindingParent,
                                    aCompileEventHandlers);
  return NS_OK;
}

void
nsGenConImageContent::UnbindFromTree(bool aDeep, bool aNullParent)
{
  nsImageLoadingContent::UnbindFromTree(aDeep, aNullParent);
  nsXMLElement::UnbindFromTree(aDeep, aNullParent);
}

nsEventStates
nsGenConImageContent::IntrinsicState() const
{
  nsEventStates state = nsXMLElement::IntrinsicState();

  nsEventStates imageState = nsImageLoadingContent::ImageState();
  if (imageState.HasAtLeastOneOfStates(NS_EVENT_STATE_BROKEN | NS_EVENT_STATE_USERDISABLED)) {
    
    
    imageState |= NS_EVENT_STATE_SUPPRESSED;
    imageState &= ~NS_EVENT_STATE_BROKEN;
  }
  imageState &= ~NS_EVENT_STATE_LOADING;
  return state | imageState;
}
