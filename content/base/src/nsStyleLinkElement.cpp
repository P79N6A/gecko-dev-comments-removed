












































#include "nsStyleLinkElement.h"

#include "nsIContent.h"
#include "mozilla/css/Loader.h"
#include "nsCSSStyleSheet.h"
#include "nsIDocument.h"
#include "nsIDOMComment.h"
#include "nsIDOMNode.h"
#include "nsIDOMStyleSheet.h"
#include "nsIDOMText.h"
#include "nsNetUtil.h"
#include "nsUnicharUtils.h"
#include "nsCRT.h"
#include "nsXPCOMCIDInternal.h"
#include "nsUnicharInputStream.h"
#include "nsContentUtils.h"

nsStyleLinkElement::nsStyleLinkElement()
  : mDontLoadStyle(PR_FALSE)
  , mUpdatesEnabled(PR_TRUE)
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
nsStyleLinkElement::InitStyleLinkElement(PRBool aDontLoadStyle)
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
nsStyleLinkElement::SetEnableUpdates(PRBool aEnableUpdates)
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

void nsStyleLinkElement::ParseLinkTypes(const nsAString& aTypes,
                                        nsTArray<nsString>& aResult)
{
  nsAString::const_iterator start, done;
  aTypes.BeginReading(start);
  aTypes.EndReading(done);
  if (start == done)
    return;

  nsAString::const_iterator current(start);
  PRBool inString = !nsCRT::IsAsciiSpace(*current);
  nsAutoString subString;

  while (current != done) {
    if (nsCRT::IsAsciiSpace(*current)) {
      if (inString) {
        ToLowerCase(Substring(start, current), subString);
        aResult.AppendElement(subString);
        inString = PR_FALSE;
      }
    }
    else {
      if (!inString) {
        start = current;
        inString = PR_TRUE;
      }
    }
    ++current;
  }
  if (inString) {
    ToLowerCase(Substring(start, current), subString);
    aResult.AppendElement(subString);
  }
}

NS_IMETHODIMP
nsStyleLinkElement::UpdateStyleSheet(nsICSSLoaderObserver* aObserver,
                                     PRBool* aWillNotify,
                                     PRBool* aIsAlternate)
{
  return DoUpdateStyleSheet(nsnull, aObserver, aWillNotify, aIsAlternate,
                            PR_FALSE);
}

nsresult
nsStyleLinkElement::UpdateStyleSheetInternal(nsIDocument *aOldDocument,
                                             PRBool aForceUpdate)
{
  PRBool notify, alternate;
  return DoUpdateStyleSheet(aOldDocument, nsnull, &notify, &alternate,
                            aForceUpdate);
}

nsresult
nsStyleLinkElement::DoUpdateStyleSheet(nsIDocument *aOldDocument,
                                       nsICSSLoaderObserver* aObserver,
                                       PRBool* aWillNotify,
                                       PRBool* aIsAlternate,
                                       PRBool aForceUpdate)
{
  *aWillNotify = PR_FALSE;

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

  PRBool isInline;
  nsCOMPtr<nsIURI> uri = GetStyleSheetURL(&isInline);

  if (!aForceUpdate && mStyleSheet && !isInline && uri) {
    nsIURI* oldURI = mStyleSheet->GetSheetURI();
    if (oldURI) {
      PRBool equal;
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
  PRBool isAlternate;

  GetStyleSheetInfo(title, type, media, &isAlternate);

  if (!type.LowerCaseEqualsLiteral("text/css")) {
    return NS_OK;
  }

  PRBool doneLoading = PR_FALSE;
  nsresult rv = NS_OK;
  if (isInline) {
    nsAutoString text;
    nsContentUtils::GetNodeTextContent(thisContent, PR_FALSE, text);

    
    rv = doc->CSSLoader()->
      LoadInlineStyle(thisContent, text, mLineNumber, title, media,
                      aObserver, &doneLoading, &isAlternate);
  }
  else {
    rv = doc->CSSLoader()->
      LoadStyleLink(thisContent, uri, title, media, isAlternate, aObserver,
                    &isAlternate);
    if (NS_FAILED(rv)) {
      
      
      
      doneLoading = PR_TRUE;
      isAlternate = PR_FALSE;
      rv = NS_OK;
    }
  }

  NS_ENSURE_SUCCESS(rv, rv);

  *aWillNotify = !doneLoading;
  *aIsAlternate = isAlternate;

  return NS_OK;
}
