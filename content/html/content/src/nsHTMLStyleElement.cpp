




































#include "nsIDOMHTMLStyleElement.h"
#include "nsIDOMLinkStyle.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIDOMStyleSheet.h"
#include "nsIStyleSheet.h"
#include "nsStyleLinkElement.h"
#include "nsNetUtil.h"
#include "nsIDocument.h"
#include "nsUnicharUtils.h"
#include "nsParserUtils.h"


class nsHTMLStyleElement : public nsGenericHTMLElement,
                           public nsIDOMHTMLStyleElement,
                           public nsStyleLinkElement,
                           public nsStubMutationObserver
{
public:
  nsHTMLStyleElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLStyleElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLSTYLEELEMENT

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

  virtual nsresult GetInnerHTML(nsAString& aInnerHTML);
  virtual nsresult SetInnerHTML(const nsAString& aInnerHTML);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  virtual void CharacterDataChanged(nsIDocument* aDocument,
                                    nsIContent* aContent,
                                    CharacterDataChangeInfo* aInfo);
  virtual void ContentAppended(nsIDocument* aDocument,
                                nsIContent* aContainer,
                               PRInt32 aNewIndexInContainer);
  virtual void ContentInserted(nsIDocument* aDocument,
                               nsIContent* aContainer,
                               nsIContent* aChild,
                               PRInt32 aIndexInContainer);
  virtual void ContentRemoved(nsIDocument* aDocument,
                              nsIContent* aContainer,
                              nsIContent* aChild,
                              PRInt32 aIndexInContainer);

protected:
  void GetStyleSheetURL(PRBool* aIsInline,
                        nsIURI** aURI);
  void GetStyleSheetInfo(nsAString& aTitle,
                         nsAString& aType,
                         nsAString& aMedia,
                         PRBool* aIsAlternate);
  




  void ContentChanged(nsIContent* aContent);
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Style)


nsHTMLStyleElement::nsHTMLStyleElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
  AddMutationObserver(this);
}

nsHTMLStyleElement::~nsHTMLStyleElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLStyleElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLStyleElement, nsGenericElement) 



NS_HTML_CONTENT_INTERFACE_TABLE_HEAD(nsHTMLStyleElement, nsGenericHTMLElement)
  NS_INTERFACE_TABLE_INHERITED4(nsHTMLStyleElement,
                                nsIDOMHTMLStyleElement,
                                nsIDOMLinkStyle,
                                nsIStyleSheetLinkingElement,
                                nsIMutationObserver)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLStyleElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLStyleElement)


NS_IMETHODIMP
nsHTMLStyleElement::GetDisabled(PRBool* aDisabled)
{
  nsresult result = NS_OK;
  
  if (mStyleSheet) {
    nsCOMPtr<nsIDOMStyleSheet> ss(do_QueryInterface(mStyleSheet));

    if (ss) {
      result = ss->GetDisabled(aDisabled);
    }
  }
  else {
    *aDisabled = PR_FALSE;
  }

  return result;
}

NS_IMETHODIMP 
nsHTMLStyleElement::SetDisabled(PRBool aDisabled)
{
  nsresult result = NS_OK;
  
  if (mStyleSheet) {
    nsCOMPtr<nsIDOMStyleSheet> ss(do_QueryInterface(mStyleSheet));

    if (ss) {
      result = ss->SetDisabled(aDisabled);
    }
  }

  return result;
}

NS_IMPL_STRING_ATTR(nsHTMLStyleElement, Media, media)
NS_IMPL_STRING_ATTR(nsHTMLStyleElement, Type, type)

void
nsHTMLStyleElement::CharacterDataChanged(nsIDocument* aDocument,
                                         nsIContent* aContent,
                                         CharacterDataChangeInfo* aInfo)
{
  ContentChanged(aContent);
}

void
nsHTMLStyleElement::ContentAppended(nsIDocument* aDocument,
                                    nsIContent* aContainer,
                                    PRInt32 aNewIndexInContainer)
{
  ContentChanged(aContainer);
}

void
nsHTMLStyleElement::ContentInserted(nsIDocument* aDocument,
                                    nsIContent* aContainer,
                                    nsIContent* aChild,
                                    PRInt32 aIndexInContainer)
{
  ContentChanged(aChild);
}

void
nsHTMLStyleElement::ContentRemoved(nsIDocument* aDocument,
                                   nsIContent* aContainer,
                                   nsIContent* aChild,
                                   PRInt32 aIndexInContainer)
{
  ContentChanged(aChild);
}

void
nsHTMLStyleElement::ContentChanged(nsIContent* aContent)
{
  if (nsContentUtils::IsInSameAnonymousTree(this, aContent)) {
    UpdateStyleSheetInternal(nsnull);
  }
}

nsresult
nsHTMLStyleElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                               nsIContent* aBindingParent,
                               PRBool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  UpdateStyleSheetInternal(nsnull);

  return rv;  
}

void
nsHTMLStyleElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  nsCOMPtr<nsIDocument> oldDoc = GetCurrentDoc();

  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
  UpdateStyleSheetInternal(oldDoc);
}

nsresult
nsHTMLStyleElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                            nsIAtom* aPrefix, const nsAString& aValue,
                            PRBool aNotify)
{
  nsresult rv = nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix,
                                              aValue, aNotify);
  if (NS_SUCCEEDED(rv)) {
    UpdateStyleSheetInternal(nsnull,
                             aNameSpaceID == kNameSpaceID_None &&
                             (aName == nsGkAtoms::title ||
                              aName == nsGkAtoms::media ||
                              aName == nsGkAtoms::type));
  }

  return rv;
}

nsresult
nsHTMLStyleElement::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                              PRBool aNotify)
{
  nsresult rv = nsGenericHTMLElement::UnsetAttr(aNameSpaceID, aAttribute,
                                                aNotify);
  if (NS_SUCCEEDED(rv)) {
    UpdateStyleSheetInternal(nsnull,
                             aNameSpaceID == kNameSpaceID_None &&
                             (aAttribute == nsGkAtoms::title ||
                              aAttribute == nsGkAtoms::media ||
                              aAttribute == nsGkAtoms::type));
  }

  return rv;
}

nsresult
nsHTMLStyleElement::GetInnerHTML(nsAString& aInnerHTML)
{
  nsContentUtils::GetNodeTextContent(this, PR_FALSE, aInnerHTML);
  return NS_OK;
}

nsresult
nsHTMLStyleElement::SetInnerHTML(const nsAString& aInnerHTML)
{
  SetEnableUpdates(PR_FALSE);

  nsresult rv = nsContentUtils::SetNodeTextContent(this, aInnerHTML, PR_TRUE);
  
  SetEnableUpdates(PR_TRUE);
  
  UpdateStyleSheetInternal(nsnull);
  return rv;
}

void
nsHTMLStyleElement::GetStyleSheetURL(PRBool* aIsInline,
                                     nsIURI** aURI)
{
  *aURI = nsnull;
  *aIsInline = !HasAttr(kNameSpaceID_None, nsGkAtoms::src);
  if (*aIsInline) {
    return;
  }
  if (mNodeInfo->NamespaceEquals(kNameSpaceID_XHTML)) {
    
    
    *aIsInline = PR_TRUE;
    return;
  }

  GetHrefURIForAnchors(aURI);
  return;
}

void
nsHTMLStyleElement::GetStyleSheetInfo(nsAString& aTitle,
                                      nsAString& aType,
                                      nsAString& aMedia,
                                      PRBool* aIsAlternate)
{
  aTitle.Truncate();
  aType.Truncate();
  aMedia.Truncate();
  *aIsAlternate = PR_FALSE;

  nsAutoString title;
  GetAttr(kNameSpaceID_None, nsGkAtoms::title, title);
  title.CompressWhitespace();
  aTitle.Assign(title);

  GetAttr(kNameSpaceID_None, nsGkAtoms::media, aMedia);
  ToLowerCase(aMedia); 

  GetAttr(kNameSpaceID_None, nsGkAtoms::type, aType);

  nsAutoString mimeType;
  nsAutoString notUsed;
  nsParserUtils::SplitMimeType(aType, mimeType, notUsed);
  if (!mimeType.IsEmpty() && !mimeType.LowerCaseEqualsLiteral("text/css")) {
    return;
  }

  
  
  aType.AssignLiteral("text/css");

  return;
}
