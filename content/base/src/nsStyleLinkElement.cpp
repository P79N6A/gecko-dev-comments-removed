












































#include "nsStyleLinkElement.h"

#include "nsIContent.h"
#include "mozilla/css/Loader.h"
#include "nsCSSStyleSheet.h"
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

nsStyleLinkElement::nsStyleLinkElement()
  : mDontLoadStyle(false)
  , mUpdatesEnabled(true)
  , mLineNumber(1)
{
}

nsStyleLinkElement::~nsStyleLinkElement()
{
  nsStyleLinkElement::SetStyleSheet(nsnull);
}

NS_IMETHODIMP 
nsStyleLinkElement::SetStyleSheet(nsIStyleSheet* aStyleSheet)
{
  nsRefPtr<nsCSSStyleSheet> cssSheet = do_QueryObject(mStyleSheet);
  if (cssSheet) {
    cssSheet->SetOwningNode(nsnull);
  }

  mStyleSheet = aStyleSheet;
  cssSheet = do_QueryObject(mStyleSheet);
  if (cssSheet) {
    nsCOMPtr<nsIDOMNode> node;
    CallQueryInterface(this,
                       static_cast<nsIDOMNode**>(getter_AddRefs(node)));
    if (node) {
      cssSheet->SetOwningNode(node);
    }
  }
    
  return NS_OK;
}

NS_IMETHODIMP 
nsStyleLinkElement::GetStyleSheet(nsIStyleSheet*& aStyleSheet)
{
  aStyleSheet = mStyleSheet;
  NS_IF_ADDREF(aStyleSheet);

  return NS_OK;
}

NS_IMETHODIMP 
nsStyleLinkElement::InitStyleLinkElement(bool aDontLoadStyle)
{
  mDontLoadStyle = aDontLoadStyle;

  return NS_OK;
}

NS_IMETHODIMP
nsStyleLinkElement::GetSheet(nsIDOMStyleSheet** aSheet)
{
  NS_ENSURE_ARG_POINTER(aSheet);
  *aSheet = nsnull;

  if (mStyleSheet) {
    CallQueryInterface(mStyleSheet, aSheet);
  }

  
  
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
nsStyleLinkElement::SetLineNumber(PRUint32 aLineNumber)
{
  mLineNumber = aLineNumber;
}

PRUint32 ToLinkMask(const nsAString& aLink)
{ 
  if (aLink.EqualsLiteral("prefetch"))
     return PREFETCH;
  else if (aLink.EqualsLiteral("dns-prefetch"))
     return DNS_PREFETCH;
  else if (aLink.EqualsLiteral("stylesheet"))
    return STYLESHEET;
  else if (aLink.EqualsLiteral("next"))
    return NEXT;
  else if (aLink.EqualsLiteral("alternate"))
    return ALTERNATE;
  else 
    return 0;
}

PRUint32 nsStyleLinkElement::ParseLinkTypes(const nsAString& aTypes)
{
  PRUint32 linkMask = 0;
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
        ToLowerCase(Substring(start, current), subString);
        linkMask |= ToLinkMask(subString);
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
    ToLowerCase(Substring(start, current), subString);
     linkMask |= ToLinkMask(subString);
  }
  return linkMask;
}

NS_IMETHODIMP
nsStyleLinkElement::UpdateStyleSheet(nsICSSLoaderObserver* aObserver,
                                     bool* aWillNotify,
                                     bool* aIsAlternate)
{
  return DoUpdateStyleSheet(nsnull, aObserver, aWillNotify, aIsAlternate,
                            false);
}

nsresult
nsStyleLinkElement::UpdateStyleSheetInternal(nsIDocument *aOldDocument,
                                             bool aForceUpdate)
{
  bool notify, alternate;
  return DoUpdateStyleSheet(aOldDocument, nsnull, &notify, &alternate,
                            aForceUpdate);
}

nsresult
nsStyleLinkElement::DoUpdateStyleSheet(nsIDocument *aOldDocument,
                                       nsICSSLoaderObserver* aObserver,
                                       bool* aWillNotify,
                                       bool* aIsAlternate,
                                       bool aForceUpdate)
{
  *aWillNotify = false;

  if (mStyleSheet && aOldDocument) {
    
    
    
    
    aOldDocument->BeginUpdate(UPDATE_STYLE);
    aOldDocument->RemoveStyleSheet(mStyleSheet);
    aOldDocument->EndUpdate(UPDATE_STYLE);
    nsStyleLinkElement::SetStyleSheet(nsnull);
  }

  if (mDontLoadStyle || !mUpdatesEnabled) {
    return NS_OK;
  }

  nsCOMPtr<nsIContent> thisContent;
  QueryInterface(NS_GET_IID(nsIContent), getter_AddRefs(thisContent));

  NS_ENSURE_TRUE(thisContent, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocument> doc = thisContent->GetDocument();

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
    doc->BeginUpdate(UPDATE_STYLE);
    doc->RemoveStyleSheet(mStyleSheet);
    doc->EndUpdate(UPDATE_STYLE);
    nsStyleLinkElement::SetStyleSheet(nsnull);
  }

  if (!uri && !isInline) {
    return NS_OK; 
  }

  nsAutoString title, type, media;
  bool isAlternate;

  GetStyleSheetInfo(title, type, media, &isAlternate);

  if (!type.LowerCaseEqualsLiteral("text/css")) {
    return NS_OK;
  }

  bool doneLoading = false;
  nsresult rv = NS_OK;
  if (isInline) {
    nsAutoString text;
    nsContentUtils::GetNodeTextContent(thisContent, false, text);

    
    rv = doc->CSSLoader()->
      LoadInlineStyle(thisContent, text, mLineNumber, title, media,
                      aObserver, &doneLoading, &isAlternate);
  }
  else {
    
    nsCOMPtr<nsIURI> clonedURI;
    uri->Clone(getter_AddRefs(clonedURI));
    NS_ENSURE_TRUE(clonedURI, NS_ERROR_OUT_OF_MEMORY);
    rv = doc->CSSLoader()->
      LoadStyleLink(thisContent, clonedURI, title, media, isAlternate, aObserver,
                    &isAlternate);
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
