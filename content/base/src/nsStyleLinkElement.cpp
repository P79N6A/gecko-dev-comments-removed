












































#include "nsStyleLinkElement.h"

#include "nsIContent.h"
#include "nsICSSLoader.h"
#include "nsICSSStyleSheet.h"
#include "nsIDocument.h"
#include "nsIDOMComment.h"
#include "nsIDOMNode.h"
#include "nsIDOMStyleSheet.h"
#include "nsIDOMText.h"
#include "nsIUnicharInputStream.h"
#include "nsISimpleUnicharStreamFactory.h"
#include "nsNetUtil.h"
#include "nsUnicharUtils.h"
#include "nsVoidArray.h"
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
  nsCOMPtr<nsICSSStyleSheet> cssSheet = do_QueryInterface(mStyleSheet);
  if (cssSheet) {
    cssSheet->SetOwningNode(nsnull);
  }
}

NS_IMETHODIMP 
nsStyleLinkElement::SetStyleSheet(nsIStyleSheet* aStyleSheet)
{
  nsCOMPtr<nsICSSStyleSheet> cssSheet = do_QueryInterface(mStyleSheet);
  if (cssSheet) {
    cssSheet->SetOwningNode(nsnull);
  }

  mStyleSheet = aStyleSheet;
  cssSheet = do_QueryInterface(mStyleSheet);
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
                                        nsStringArray& aResult)
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
        aResult.AppendString(subString);
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
    aResult.AppendString(subString);
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
    mStyleSheet = nsnull;
  }

  if (mDontLoadStyle || !mUpdatesEnabled) {
    return NS_OK;
  }

  nsCOMPtr<nsIContent> thisContent;
  QueryInterface(NS_GET_IID(nsIContent), getter_AddRefs(thisContent));

  NS_ENSURE_TRUE(thisContent, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocument> doc = thisContent->GetDocument();

  if (!doc) {
    return NS_OK;
  }

  nsCOMPtr<nsIURI> uri;
  PRBool isInline;
  GetStyleSheetURL(&isInline, getter_AddRefs(uri));

  if (!aForceUpdate && mStyleSheet && !isInline && uri) {
    nsCOMPtr<nsIURI> oldURI;

    mStyleSheet->GetSheetURI(getter_AddRefs(oldURI));
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
    mStyleSheet = nsnull;
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
    nsAutoString content;
    nsContentUtils::GetNodeTextContent(thisContent, PR_FALSE, content);

    nsCOMPtr<nsIUnicharInputStream> uin;
    rv = nsSimpleUnicharStreamFactory::GetInstance()->
      CreateInstanceFromString(content, getter_AddRefs(uin));
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    
    rv = doc->CSSLoader()->
      LoadInlineStyle(thisContent, uin, mLineNumber, title, media,
                      aObserver, &doneLoading, &isAlternate);
  }
  else {
    rv = doc->CSSLoader()->
      LoadStyleLink(thisContent, uri, title, media, isAlternate, aObserver,
                    &isAlternate);
  }

  NS_ENSURE_SUCCESS(rv, rv);

  *aWillNotify = !doneLoading;
  *aIsAlternate = isAlternate;

  return NS_OK;
}

