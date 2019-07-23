




































#ifndef __inCSSValueSearch_h__
#define __inCSSValueSearch_h__

#include "inICSSValueSearch.h"

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIDOMDocument.h"
#include "inISearchObserver.h"
#include "nsTArray.h"
#include "nsCSSProps.h"

class nsIDOMCSSStyleSheet;
class nsIDOMCSSRuleList;
class nsIDOMCSSStyleRule;
class nsIURI;

class inCSSValueSearch : public inICSSValueSearch
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_INISEARCHPROCESS
  NS_DECL_INICSSVALUESEARCH

  inCSSValueSearch();
  virtual ~inCSSValueSearch();

protected:
  nsCOMPtr<inISearchObserver> mObserver;
  nsCOMPtr<nsIDOMDocument> mDocument;
  nsTArray<nsAutoString *>* mResults;
  nsCSSProperty* mProperties;
  nsString mLastResult;
  nsString mBaseURL;
  nsString mTextCriteria;
  PRInt32 mResultCount;
  PRUint32 mPropertyCount;
  PRBool mIsActive;
  PRBool mHoldResults;
  PRBool mReturnRelativeURLs;
  PRBool mNormalizeChromeURLs;

  nsresult InitSearch();
  nsresult KillSearch(PRInt16 aResult);
  nsresult SearchStyleSheet(nsIDOMCSSStyleSheet* aStyleSheet, nsIURI* aBaseURI);
  nsresult SearchRuleList(nsIDOMCSSRuleList* aRuleList, nsIURI* aBaseURI);
  nsresult SearchStyleRule(nsIDOMCSSStyleRule* aStyleRule, nsIURI* aBaseURI);
  nsresult SearchStyleValue(const nsAFlatString& aValue, nsIURI* aBaseURI);
  nsresult EqualizeURL(nsAutoString* aURL);
};


#define IN_CSSVALUESEARCH_CID \
{ 0x4d977f60, 0xfbe7, 0x4583, { 0x8c, 0xb7, 0xf5, 0xed, 0x88, 0x22, 0x93, 0xef } }

#endif 
