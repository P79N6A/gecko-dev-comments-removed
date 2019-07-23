



































#include "nsIDOMHTMLLinkElement.h"
#include "nsIDOMLinkStyle.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsILink.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIDOMStyleSheet.h"
#include "nsIStyleSheet.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsStyleLinkElement.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsIDocument.h"
#include "nsIDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsParserUtils.h"
#include "nsContentUtils.h"
#include "nsPIDOMWindow.h"
#include "nsPLDOMEvent.h"

#include "Link.h"
using namespace mozilla::dom;

class nsHTMLLinkElement : public nsGenericHTMLElement,
                          public nsIDOMHTMLLinkElement,
                          public nsILink,
                          public nsStyleLinkElement,
                          public Link
{
public:
  nsHTMLLinkElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLLinkElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLLINKELEMENT

  
  NS_IMETHOD    LinkAdded();
  NS_IMETHOD    LinkRemoved();

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  void CreateAndDispatchEvent(nsIDocument* aDoc, const nsAString& aEventName);
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

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);
  virtual PRBool IsLink(nsIURI** aURI) const;
  virtual void GetLinkTarget(nsAString& aTarget);
  virtual nsLinkState GetLinkState() const;
  virtual void SetLinkState(nsLinkState aState);
  virtual already_AddRefed<nsIURI> GetHrefURI() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:
  virtual already_AddRefed<nsIURI> GetStyleSheetURL(PRBool* aIsInline);
  virtual void GetStyleSheetInfo(nsAString& aTitle,
                                 nsAString& aType,
                                 nsAString& aMedia,
                                 PRBool* aIsAlternate);
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Link)


nsHTMLLinkElement::nsHTMLLinkElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLLinkElement::~nsHTMLLinkElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLLinkElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLLinkElement, nsGenericElement) 



NS_INTERFACE_TABLE_HEAD(nsHTMLLinkElement)
  NS_HTML_CONTENT_INTERFACE_TABLE4(nsHTMLLinkElement,
                                   nsIDOMHTMLLinkElement,
                                   nsIDOMLinkStyle,
                                   nsILink,
                                   nsIStyleSheetLinkingElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLLinkElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLLinkElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLLinkElement)


NS_IMETHODIMP
nsHTMLLinkElement::GetDisabled(PRBool* aDisabled)
{
  nsCOMPtr<nsIDOMStyleSheet> ss(do_QueryInterface(GetStyleSheet()));
  nsresult result = NS_OK;

  if (ss) {
    result = ss->GetDisabled(aDisabled);
  } else {
    *aDisabled = PR_FALSE;
  }

  return result;
}

NS_IMETHODIMP 
nsHTMLLinkElement::SetDisabled(PRBool aDisabled)
{
  nsCOMPtr<nsIDOMStyleSheet> ss(do_QueryInterface(GetStyleSheet()));
  nsresult result = NS_OK;

  if (ss) {
    result = ss->SetDisabled(aDisabled);
  }

  return result;
}


NS_IMPL_STRING_ATTR(nsHTMLLinkElement, Charset, charset)
NS_IMPL_URI_ATTR(nsHTMLLinkElement, Href, href)
NS_IMPL_STRING_ATTR(nsHTMLLinkElement, Hreflang, hreflang)
NS_IMPL_STRING_ATTR(nsHTMLLinkElement, Media, media)
NS_IMPL_STRING_ATTR(nsHTMLLinkElement, Rel, rel)
NS_IMPL_STRING_ATTR(nsHTMLLinkElement, Rev, rev)
NS_IMPL_STRING_ATTR(nsHTMLLinkElement, Target, target)
NS_IMPL_STRING_ATTR(nsHTMLLinkElement, Type, type)

nsresult
nsHTMLLinkElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  nsContentUtils::AddScriptRunner(
    new nsRunnableMethod<nsHTMLLinkElement>(this,
                                            &nsHTMLLinkElement::UpdateStyleSheetInternal));

  CreateAndDispatchEvent(aDocument, NS_LITERAL_STRING("DOMLinkAdded"));

  return rv;  
}

NS_IMETHODIMP
nsHTMLLinkElement::LinkAdded()
{
  CreateAndDispatchEvent(GetOwnerDoc(), NS_LITERAL_STRING("DOMLinkAdded"));
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLLinkElement::LinkRemoved()
{
  CreateAndDispatchEvent(GetOwnerDoc(), NS_LITERAL_STRING("DOMLinkRemoved"));
  return NS_OK;
}

void
nsHTMLLinkElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  Link::ResetLinkState();

  
  
  nsCOMPtr<nsIDocument> oldDoc = GetCurrentDoc();
  CreateAndDispatchEvent(oldDoc, NS_LITERAL_STRING("DOMLinkRemoved"));
  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
  UpdateStyleSheetInternal(oldDoc);
}

void
nsHTMLLinkElement::CreateAndDispatchEvent(nsIDocument* aDoc,
                                          const nsAString& aEventName)
{
  if (!aDoc)
    return;

  
  
  
  
  
  
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::_empty, &nsGkAtoms::stylesheet, nsnull};

  if (!nsContentUtils::HasNonEmptyAttr(this, kNameSpaceID_None,
                                       nsGkAtoms::rev) &&
      FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::rel,
                      strings, eIgnoreCase) != ATTR_VALUE_NO_MATCH)
    return;

  nsRefPtr<nsPLDOMEvent> event = new nsPLDOMEvent(this, aEventName, PR_TRUE);
  if (event) {
    
    
    
    
    
    if (nsContentUtils::IsSafeToRunScript())
      event->PostDOMEvent();
    else
      event->RunDOMEventWhenSafe();
  }
}

nsresult
nsHTMLLinkElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify)
{
  if (aName == nsGkAtoms::href && kNameSpaceID_None == aNameSpaceID) {
    Link::ResetLinkState();
  }

  nsresult rv = nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix,
                                              aValue, aNotify);
  if (NS_SUCCEEDED(rv)) {
    PRBool dropSheet = PR_FALSE;
    if (aNameSpaceID == kNameSpaceID_None && aName == nsGkAtoms::rel &&
        GetStyleSheet()) {
      nsAutoTArray<nsString, 4> linkTypes;
      nsStyleLinkElement::ParseLinkTypes(aValue, linkTypes);
      dropSheet = !linkTypes.Contains(NS_LITERAL_STRING("stylesheet"));
    }
    
    UpdateStyleSheetInternal(nsnull,
                             dropSheet ||
                             (aNameSpaceID == kNameSpaceID_None &&
                              (aName == nsGkAtoms::title ||
                               aName == nsGkAtoms::media ||
                               aName == nsGkAtoms::type)));
  }

  return rv;
}

nsresult
nsHTMLLinkElement::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify)
{
  if (aAttribute == nsGkAtoms::href && kNameSpaceID_None == aNameSpaceID) {
    Link::ResetLinkState();
  }

  nsresult rv = nsGenericHTMLElement::UnsetAttr(aNameSpaceID, aAttribute,
                                                aNotify);
  if (NS_SUCCEEDED(rv)) {
    UpdateStyleSheetInternal(nsnull,
                             aNameSpaceID == kNameSpaceID_None &&
                             (aAttribute == nsGkAtoms::rel ||
                              aAttribute == nsGkAtoms::title ||
                              aAttribute == nsGkAtoms::media ||
                              aAttribute == nsGkAtoms::type));
  }

  return rv;
}

nsresult
nsHTMLLinkElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  return PreHandleEventForAnchors(aVisitor);
}

nsresult
nsHTMLLinkElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  return PostHandleEventForAnchors(aVisitor);
}

PRBool
nsHTMLLinkElement::IsLink(nsIURI** aURI) const
{
  return IsHTMLLink(aURI);
}

void
nsHTMLLinkElement::GetLinkTarget(nsAString& aTarget)
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::target, aTarget);
  if (aTarget.IsEmpty()) {
    GetBaseTarget(aTarget);
  }
}

nsLinkState
nsHTMLLinkElement::GetLinkState() const
{
  return Link::GetLinkState();
}

void
nsHTMLLinkElement::SetLinkState(nsLinkState aState)
{
  Link::SetLinkState(aState);
}

already_AddRefed<nsIURI>
nsHTMLLinkElement::GetHrefURI() const
{
  return GetHrefURIForAnchors();
}

already_AddRefed<nsIURI>
nsHTMLLinkElement::GetStyleSheetURL(PRBool* aIsInline)
{
  *aIsInline = PR_FALSE;
  return GetHrefURIForAnchors();
}

void
nsHTMLLinkElement::GetStyleSheetInfo(nsAString& aTitle,
                                     nsAString& aType,
                                     nsAString& aMedia,
                                     PRBool* aIsAlternate)
{
  aTitle.Truncate();
  aType.Truncate();
  aMedia.Truncate();
  *aIsAlternate = PR_FALSE;

  nsAutoString rel;
  nsAutoTArray<nsString, 4> linkTypes;
  GetAttr(kNameSpaceID_None, nsGkAtoms::rel, rel);
  nsStyleLinkElement::ParseLinkTypes(rel, linkTypes);
  
  if (!linkTypes.Contains(NS_LITERAL_STRING("stylesheet"))) {
    return;
  }

  nsAutoString title;
  GetAttr(kNameSpaceID_None, nsGkAtoms::title, title);
  title.CompressWhitespace();
  aTitle.Assign(title);

  
  if (linkTypes.Contains(NS_LITERAL_STRING("alternate"))) {
    if (aTitle.IsEmpty()) { 
      return;
    } else {
      *aIsAlternate = PR_TRUE;
    }
  }

  GetAttr(kNameSpaceID_None, nsGkAtoms::media, aMedia);
  ToLowerCase(aMedia); 

  nsAutoString mimeType;
  nsAutoString notUsed;
  GetAttr(kNameSpaceID_None, nsGkAtoms::type, aType);
  nsParserUtils::SplitMimeType(aType, mimeType, notUsed);
  if (!mimeType.IsEmpty() && !mimeType.LowerCaseEqualsLiteral("text/css")) {
    return;
  }

  
  
  aType.AssignLiteral("text/css");

  return;
}
