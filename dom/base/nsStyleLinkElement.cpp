











#include "nsStyleLinkElement.h"

#include "mozilla/CSSStyleSheet.h"
#include "mozilla/css/Loader.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/FragmentOrElement.h"
#include "mozilla/dom/ShadowRoot.h"
#include "mozilla/Preferences.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMComment.h"
#include "nsIDOMNode.h"
#include "nsIDOMStyleSheet.h"
#include "nsNetUtil.h"
#include "nsUnicharUtils.h"
#include "nsCRT.h"
#include "nsXPCOMCIDInternal.h"
#include "nsUnicharInputStream.h"
#include "nsContentUtils.h"
#include "nsStyleUtil.h"
#include "nsQueryObject.h"

using namespace mozilla;
using namespace mozilla::dom;

nsStyleLinkElement::nsStyleLinkElement()
  : mDontLoadStyle(false)
  , mUpdatesEnabled(true)
  , mLineNumber(1)
{
}

nsStyleLinkElement::~nsStyleLinkElement()
{
  nsStyleLinkElement::SetStyleSheet(nullptr);
}

void
nsStyleLinkElement::Unlink()
{
  mStyleSheet = nullptr;
}

void
nsStyleLinkElement::Traverse(nsCycleCollectionTraversalCallback &cb)
{
  nsStyleLinkElement* tmp = this;
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mStyleSheet);
}

NS_IMETHODIMP 
nsStyleLinkElement::SetStyleSheet(CSSStyleSheet* aStyleSheet)
{
  if (mStyleSheet) {
    mStyleSheet->SetOwningNode(nullptr);
  }

  mStyleSheet = aStyleSheet;
  if (mStyleSheet) {
    nsCOMPtr<nsINode> node = do_QueryObject(this);
    if (node) {
      mStyleSheet->SetOwningNode(node);
    }
  }
    
  return NS_OK;
}

NS_IMETHODIMP_(CSSStyleSheet*)
nsStyleLinkElement::GetStyleSheet()
{
  return mStyleSheet;
}

NS_IMETHODIMP 
nsStyleLinkElement::InitStyleLinkElement(bool aDontLoadStyle)
{
  mDontLoadStyle = aDontLoadStyle;

  return NS_OK;
}

NS_IMETHODIMP
nsStyleLinkElement::SetEnableUpdates(bool aEnableUpdates)
{
  mUpdatesEnabled = aEnableUpdates;

  return NS_OK;
}

NS_IMETHODIMP
nsStyleLinkElement::GetCharset(nsAString& aCharset)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

 void
nsStyleLinkElement::OverrideBaseURI(nsIURI* aNewBaseURI)
{
  NS_NOTREACHED("Base URI can't be overriden in this implementation "
                "of nsIStyleSheetLinkingElement.");
}

 void
nsStyleLinkElement::SetLineNumber(uint32_t aLineNumber)
{
  mLineNumber = aLineNumber;
}

 bool
nsStyleLinkElement::IsImportEnabled()
{
  static bool sAdded = false;
  static bool sImportsEnabled;
  if (!sAdded) {
    
    Preferences::AddBoolVarCache(&sImportsEnabled,
                                 "dom.htmlimports.enabled",
                                 false);
    sAdded = true;
  }

  return sImportsEnabled;
}

static uint32_t ToLinkMask(const nsAString& aLink, nsIPrincipal* aPrincipal)
{ 
  if (aLink.EqualsLiteral("prefetch"))
    return nsStyleLinkElement::ePREFETCH;
  else if (aLink.EqualsLiteral("dns-prefetch"))
    return nsStyleLinkElement::eDNS_PREFETCH;
  else if (aLink.EqualsLiteral("stylesheet"))
    return nsStyleLinkElement::eSTYLESHEET;
  else if (aLink.EqualsLiteral("next"))
    return nsStyleLinkElement::eNEXT;
  else if (aLink.EqualsLiteral("alternate"))
    return nsStyleLinkElement::eALTERNATE;
  else if (aLink.EqualsLiteral("import") &&
           nsStyleLinkElement::IsImportEnabled())
    return nsStyleLinkElement::eHTMLIMPORT;
  else if (aLink.EqualsLiteral("preconnect"))
    return nsStyleLinkElement::ePRECONNECT;
  else 
    return 0;
}

uint32_t nsStyleLinkElement::ParseLinkTypes(const nsAString& aTypes, nsIPrincipal* aPrincipal)
{
  uint32_t linkMask = 0;
  nsAString::const_iterator start, done;
  aTypes.BeginReading(start);
  aTypes.EndReading(done);
  if (start == done)
    return linkMask;

  nsAString::const_iterator current(start);
  bool inString = !nsContentUtils::IsHTMLWhitespace(*current);
  nsAutoString subString;
  
  while (current != done) {
    if (nsContentUtils::IsHTMLWhitespace(*current)) {
      if (inString) {
        nsContentUtils::ASCIIToLower(Substring(start, current), subString);
        linkMask |= ToLinkMask(subString, aPrincipal);
        inString = false;
      }
    }
    else {
      if (!inString) {
        start = current;
        inString = true;
      }
    }
    ++current;
  }
  if (inString) {
    nsContentUtils::ASCIIToLower(Substring(start, current), subString);
    linkMask |= ToLinkMask(subString, aPrincipal);
  }
  return linkMask;
}

NS_IMETHODIMP
nsStyleLinkElement::UpdateStyleSheet(nsICSSLoaderObserver* aObserver,
                                     bool* aWillNotify,
                                     bool* aIsAlternate,
                                     bool aForceReload)
{
  if (aForceReload) {
    
    nsCOMPtr<nsIContent> thisContent;
    CallQueryInterface(this, getter_AddRefs(thisContent));
    nsCOMPtr<nsIDocument> doc = thisContent->IsInShadowTree() ?
      thisContent->OwnerDoc() : thisContent->GetUncomposedDoc();
    if (doc && doc->CSSLoader()->GetEnabled() &&
        mStyleSheet && mStyleSheet->GetOriginalURI()) {
      doc->CSSLoader()->ObsoleteSheet(mStyleSheet->GetOriginalURI());
    }
  }
  return DoUpdateStyleSheet(nullptr, nullptr, aObserver, aWillNotify,
                            aIsAlternate, aForceReload);
}

nsresult
nsStyleLinkElement::UpdateStyleSheetInternal(nsIDocument *aOldDocument,
                                             ShadowRoot *aOldShadowRoot,
                                             bool aForceUpdate)
{
  bool notify, alternate;
  return DoUpdateStyleSheet(aOldDocument, aOldShadowRoot, nullptr, &notify,
                            &alternate, aForceUpdate);
}

static bool
IsScopedStyleElement(nsIContent* aContent)
{
  
  
  
  return (aContent->IsHTMLElement(nsGkAtoms::style) ||
          aContent->IsSVGElement(nsGkAtoms::style)) &&
         aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::scoped);
}

static bool
HasScopedStyleSheetChild(nsIContent* aContent)
{
  for (nsIContent* n = aContent->GetFirstChild(); n; n = n->GetNextSibling()) {
    if (IsScopedStyleElement(n)) {
      return true;
    }
  }
  return false;
}


static void
UpdateIsElementInStyleScopeFlagOnSubtree(Element* aElement)
{
  NS_ASSERTION(aElement->IsElementInStyleScope(),
               "only call UpdateIsElementInStyleScopeFlagOnSubtree on a "
               "subtree that has IsElementInStyleScope boolean flag set");

  if (HasScopedStyleSheetChild(aElement)) {
    return;
  }

  aElement->ClearIsElementInStyleScope();

  nsIContent* n = aElement->GetNextNode(aElement);
  while (n) {
    if (HasScopedStyleSheetChild(n)) {
      n = n->GetNextNonChildNode(aElement);
    } else {
      if (n->IsElement()) {
        n->ClearIsElementInStyleScope();
      }
      n = n->GetNextNode(aElement);
    }
  }
}

static Element*
GetScopeElement(nsIStyleSheet* aSheet)
{
  nsRefPtr<CSSStyleSheet> cssStyleSheet = do_QueryObject(aSheet);
  if (!cssStyleSheet) {
    return nullptr;
  }

  return cssStyleSheet->GetScopeElement();
}

nsresult
nsStyleLinkElement::DoUpdateStyleSheet(nsIDocument* aOldDocument,
                                       ShadowRoot* aOldShadowRoot,
                                       nsICSSLoaderObserver* aObserver,
                                       bool* aWillNotify,
                                       bool* aIsAlternate,
                                       bool aForceUpdate)
{
  *aWillNotify = false;

  nsCOMPtr<nsIContent> thisContent;
  CallQueryInterface(this, getter_AddRefs(thisContent));

  
  NS_ENSURE_TRUE(thisContent, NS_ERROR_FAILURE);

  if (thisContent->IsInAnonymousSubtree() &&
      thisContent->IsAnonymousContentInSVGUseSubtree()) {
    
    
    return NS_OK;
  }

  
  
  ShadowRoot* containingShadow = thisContent->GetContainingShadow();
  if (thisContent->IsHTMLElement(nsGkAtoms::link) &&
      (aOldShadowRoot || containingShadow)) {
    return NS_OK;
  }

  Element* oldScopeElement = GetScopeElement(mStyleSheet);

  if (mStyleSheet && (aOldDocument || aOldShadowRoot)) {
    MOZ_ASSERT(!(aOldDocument && aOldShadowRoot),
               "ShadowRoot content is never in document, thus "
               "there should not be a old document and old "
               "ShadowRoot simultaneously.");

    
    
    
    
    if (aOldShadowRoot) {
      aOldShadowRoot->RemoveSheet(mStyleSheet);
    } else {
      aOldDocument->BeginUpdate(UPDATE_STYLE);
      aOldDocument->RemoveStyleSheet(mStyleSheet);
      aOldDocument->EndUpdate(UPDATE_STYLE);
    }

    nsStyleLinkElement::SetStyleSheet(nullptr);
    if (oldScopeElement) {
      UpdateIsElementInStyleScopeFlagOnSubtree(oldScopeElement);
    }
  }

  
  if (mDontLoadStyle || !mUpdatesEnabled ||
      thisContent->OwnerDoc()->IsStaticDocument()) {
    return NS_OK;
  }

  nsCOMPtr<nsIDocument> doc = thisContent->IsInShadowTree() ?
    thisContent->OwnerDoc() : thisContent->GetUncomposedDoc();
  if (!doc || !doc->CSSLoader()->GetEnabled()) {
    return NS_OK;
  }

  bool isInline;
  nsCOMPtr<nsIURI> uri = GetStyleSheetURL(&isInline);

  if (!aForceUpdate && mStyleSheet && !isInline && uri) {
    nsIURI* oldURI = mStyleSheet->GetSheetURI();
    if (oldURI) {
      bool equal;
      nsresult rv = oldURI->Equals(uri, &equal);
      if (NS_SUCCEEDED(rv) && equal) {
        return NS_OK; 
      }
    }
  }

  if (mStyleSheet) {
    if (thisContent->IsInShadowTree()) {
      ShadowRoot* containingShadow = thisContent->GetContainingShadow();
      containingShadow->RemoveSheet(mStyleSheet);
    } else {
      doc->BeginUpdate(UPDATE_STYLE);
      doc->RemoveStyleSheet(mStyleSheet);
      doc->EndUpdate(UPDATE_STYLE);
    }

    nsStyleLinkElement::SetStyleSheet(nullptr);
  }

  if (!uri && !isInline) {
    return NS_OK; 
  }

  nsAutoString title, type, media;
  bool isScoped;
  bool isAlternate;

  GetStyleSheetInfo(title, type, media, &isScoped, &isAlternate);

  if (!type.LowerCaseEqualsLiteral("text/css")) {
    return NS_OK;
  }

  Element* scopeElement = isScoped ? thisContent->GetParentElement() : nullptr;
  if (scopeElement) {
    NS_ASSERTION(isInline, "non-inline style must not have scope element");
    scopeElement->SetIsElementInStyleScopeFlagOnSubtree(true);
  }

  bool doneLoading = false;
  nsresult rv = NS_OK;
  if (isInline) {
    nsAutoString text;
    if (!nsContentUtils::GetNodeTextContent(thisContent, false, text)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    MOZ_ASSERT(thisContent->NodeInfo()->NameAtom() != nsGkAtoms::link,
               "<link> is not 'inline', and needs different CSP checks");
    if (!nsStyleUtil::CSPAllowsInlineStyle(thisContent,
                                           thisContent->NodePrincipal(),
                                           doc->GetDocumentURI(),
                                           mLineNumber, text, &rv))
      return rv;

    
    rv = doc->CSSLoader()->
      LoadInlineStyle(thisContent, text, mLineNumber, title, media,
                      scopeElement, aObserver, &doneLoading, &isAlternate);
  }
  else {
    
    nsCOMPtr<nsIURI> clonedURI;
    uri->Clone(getter_AddRefs(clonedURI));
    NS_ENSURE_TRUE(clonedURI, NS_ERROR_OUT_OF_MEMORY);
    rv = doc->CSSLoader()->
      LoadStyleLink(thisContent, clonedURI, title, media, isAlternate,
                    GetCORSMode(), doc->GetReferrerPolicy(),
                    aObserver, &isAlternate);
    if (NS_FAILED(rv)) {
      
      
      
      doneLoading = true;
      isAlternate = false;
      rv = NS_OK;
    }
  }

  NS_ENSURE_SUCCESS(rv, rv);

  *aWillNotify = !doneLoading;
  *aIsAlternate = isAlternate;

  return NS_OK;
}

void
nsStyleLinkElement::UpdateStyleSheetScopedness(bool aIsNowScoped)
{
  if (!mStyleSheet) {
    return;
  }

  nsCOMPtr<nsIContent> thisContent;
  CallQueryInterface(this, getter_AddRefs(thisContent));

  Element* oldScopeElement = mStyleSheet->GetScopeElement();
  Element* newScopeElement = aIsNowScoped ?
                               thisContent->GetParentElement() :
                               nullptr;

  if (oldScopeElement == newScopeElement) {
    return;
  }

  nsIDocument* document = thisContent->GetOwnerDocument();

  if (thisContent->IsInShadowTree()) {
    ShadowRoot* containingShadow = thisContent->GetContainingShadow();
    containingShadow->RemoveSheet(mStyleSheet);

    mStyleSheet->SetScopeElement(newScopeElement);

    containingShadow->InsertSheet(mStyleSheet, thisContent);
  } else {
    document->BeginUpdate(UPDATE_STYLE);
    document->RemoveStyleSheet(mStyleSheet);

    mStyleSheet->SetScopeElement(newScopeElement);

    document->AddStyleSheet(mStyleSheet);
    document->EndUpdate(UPDATE_STYLE);
  }

  if (oldScopeElement) {
    UpdateIsElementInStyleScopeFlagOnSubtree(oldScopeElement);
  }
  if (newScopeElement) {
    newScopeElement->SetIsElementInStyleScopeFlagOnSubtree(true);
  }
}
