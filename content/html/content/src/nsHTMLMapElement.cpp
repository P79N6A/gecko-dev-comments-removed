



































#include "nsIDOMHTMLMapElement.h"
#include "nsIDOMEventReceiver.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsContentList.h"
#include "nsIDocument.h"
#include "nsIHTMLDocument.h"
#include "nsCOMPtr.h"


class nsHTMLMapElement : public nsGenericHTMLElement,
                         public nsIDOMHTMLMapElement
{
public:
  nsHTMLMapElement(nsINodeInfo *aNodeInfo);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLMAPELEMENT

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsHTMLMapElement,
                                                     nsGenericHTMLElement)

protected:
  nsRefPtr<nsContentList> mAreas;
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Map)


nsHTMLMapElement::nsHTMLMapElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsHTMLMapElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHTMLMapElement,
                                                  nsGenericHTMLElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mAreas,
                                                       nsBaseContentList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsHTMLMapElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLMapElement, nsGenericElement) 



NS_HTML_CONTENT_CC_INTERFACE_MAP_BEGIN(nsHTMLMapElement, nsGenericHTMLElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLMapElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLMapElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


nsresult
nsHTMLMapElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                             nsIContent* aBindingParent,
                             PRBool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(aDocument);

  if (htmlDoc) {
    htmlDoc->AddImageMap(this);
  }

  return rv;
}

void
nsHTMLMapElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(GetCurrentDoc());

  if (htmlDoc) {
    htmlDoc->RemoveImageMap(this);
  }

  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
}

NS_IMPL_ELEMENT_CLONE(nsHTMLMapElement)


NS_IMETHODIMP
nsHTMLMapElement::GetAreas(nsIDOMHTMLCollection** aAreas)
{
  NS_ENSURE_ARG_POINTER(aAreas);

  if (!mAreas) {
    
    mAreas = new nsContentList(this,
                               nsGkAtoms::area,
                               mNodeInfo->NamespaceID(),
                               PR_FALSE);

    if (!mAreas) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  NS_ADDREF(*aAreas = mAreas);
  return NS_OK;
}


NS_IMPL_STRING_ATTR(nsHTMLMapElement, Name, name)
