





































#include "nsSVGElement.h"
#include "nsGkAtoms.h"
#include "nsIDOMSVGStyleElement.h"
#include "nsUnicharUtils.h"
#include "nsIDocument.h"
#include "nsStyleLinkElement.h"

typedef nsSVGElement nsSVGStyleElementBase;

class nsSVGStyleElement : public nsSVGStyleElementBase,
                          public nsIDOMSVGStyleElement,
                          public nsStyleLinkElement
{
protected:
  friend nsresult NS_NewSVGStyleElement(nsIContent **aResult,
                                        nsINodeInfo *aNodeInfo);
  nsSVGStyleElement(nsINodeInfo *aNodeInfo);
  
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGSTYLEELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGStyleElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGStyleElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGStyleElementBase::)

  
  virtual nsresult InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                 PRBool aNotify);
  virtual nsresult RemoveChildAt(PRUint32 aIndex, PRBool aNotify);
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify);
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:
  
  
  
  inline nsresult Init()
  {
    return NS_OK;
  }

  
  void GetStyleSheetURL(PRBool* aIsInline,
                        nsIURI** aURI);
  void GetStyleSheetInfo(nsAString& aTitle,
                         nsAString& aType,
                         nsAString& aMedia,
                         PRBool* aIsAlternate);
};


NS_IMPL_NS_NEW_SVG_ELEMENT(Style)





NS_IMPL_ADDREF_INHERITED(nsSVGStyleElement,nsSVGStyleElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGStyleElement,nsSVGStyleElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGStyleElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGStyleElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMLinkStyle)
  NS_INTERFACE_MAP_ENTRY(nsIStyleSheetLinkingElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGStyleElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGStyleElementBase)




nsSVGStyleElement::nsSVGStyleElement(nsINodeInfo *aNodeInfo)
  : nsSVGStyleElementBase(aNodeInfo)
{

}






NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGStyleElement)





nsresult
nsSVGStyleElement::InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                 PRBool aNotify)
{
  nsresult rv = nsSVGStyleElementBase::InsertChildAt(aKid, aIndex, aNotify);
  if (NS_SUCCEEDED(rv)) {
    UpdateStyleSheet();
  }

  return rv;
}

nsresult
nsSVGStyleElement::RemoveChildAt(PRUint32 aIndex, PRBool aNotify)
{
  nsresult rv = nsSVGStyleElementBase::RemoveChildAt(aIndex, aNotify);
  if (NS_SUCCEEDED(rv)) {
    UpdateStyleSheet();
  }

  return rv;
}

nsresult
nsSVGStyleElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers)
{
  nsresult rv = nsSVGStyleElementBase::BindToTree(aDocument, aParent,
                                                  aBindingParent,
                                                  aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  UpdateStyleSheet(nsnull);

  return rv;  
}

void
nsSVGStyleElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  nsCOMPtr<nsIDocument> oldDoc = GetCurrentDoc();

  nsSVGStyleElementBase::UnbindFromTree(aDeep, aNullParent);
  UpdateStyleSheet(oldDoc);
}

nsresult
nsSVGStyleElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify)
{
  nsresult rv = nsSVGStyleElementBase::SetAttr(aNameSpaceID, aName, aPrefix,
                                               aValue, aNotify);
  if (NS_SUCCEEDED(rv)) {
    UpdateStyleSheet(nsnull, nsnull,
                     aNameSpaceID == kNameSpaceID_None &&
                     (aName == nsGkAtoms::title ||
                      aName == nsGkAtoms::media ||
                      aName == nsGkAtoms::type));
  }

  return rv;
}

nsresult
nsSVGStyleElement::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                              PRBool aNotify)
{
  nsresult rv = nsSVGStyleElementBase::UnsetAttr(aNameSpaceID, aAttribute,
                                                 aNotify);
  if (NS_SUCCEEDED(rv)) {
    UpdateStyleSheet(nsnull, nsnull,
                     aNameSpaceID == kNameSpaceID_None &&
                     (aAttribute == nsGkAtoms::title ||
                      aAttribute == nsGkAtoms::media ||
                      aAttribute == nsGkAtoms::type));
  }

  return rv;
}





NS_IMETHODIMP nsSVGStyleElement::GetXmlspace(nsAString & aXmlspace)
{
  GetAttr(kNameSpaceID_XML, nsGkAtoms::space, aXmlspace);

  return NS_OK;
}
NS_IMETHODIMP nsSVGStyleElement::SetXmlspace(const nsAString & aXmlspace)
{
  return SetAttr(kNameSpaceID_XML, nsGkAtoms::space, aXmlspace, PR_TRUE);
}


NS_IMETHODIMP nsSVGStyleElement::GetType(nsAString & aType)
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::type, aType);

  return NS_OK;
}
NS_IMETHODIMP nsSVGStyleElement::SetType(const nsAString & aType)
{
  return SetAttr(kNameSpaceID_None, nsGkAtoms::type, aType, PR_TRUE);
}


NS_IMETHODIMP nsSVGStyleElement::GetMedia(nsAString & aMedia)
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::media, aMedia);

  return NS_OK;
}
NS_IMETHODIMP nsSVGStyleElement::SetMedia(const nsAString & aMedia)
{
  return SetAttr(kNameSpaceID_XML, nsGkAtoms::media, aMedia, PR_TRUE);
}


NS_IMETHODIMP nsSVGStyleElement::GetTitle(nsAString & aTitle)
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::title, aTitle);

  return NS_OK;
}
NS_IMETHODIMP nsSVGStyleElement::SetTitle(const nsAString & aTitle)
{
  return SetAttr(kNameSpaceID_XML, nsGkAtoms::title, aTitle, PR_TRUE);
}




void
nsSVGStyleElement::GetStyleSheetURL(PRBool* aIsInline,
                                    nsIURI** aURI)
{
  *aURI = nsnull;
  *aIsInline = PR_TRUE;

  return;
}

void
nsSVGStyleElement::GetStyleSheetInfo(nsAString& aTitle,
                                     nsAString& aType,
                                     nsAString& aMedia,
                                     PRBool* aIsAlternate)
{
  *aIsAlternate = PR_FALSE;

  nsAutoString title;
  GetAttr(kNameSpaceID_None, nsGkAtoms::title, title);
  title.CompressWhitespace();
  aTitle.Assign(title);

  GetAttr(kNameSpaceID_None, nsGkAtoms::media, aMedia);
  
  
  ToLowerCase(aMedia);

  GetAttr(kNameSpaceID_None, nsGkAtoms::type, aType);

  return;
}
