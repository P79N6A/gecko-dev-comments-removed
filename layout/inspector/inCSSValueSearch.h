



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

class inCSSValueSearch MOZ_FINAL : public inICSSValueSearch
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_INISEARCHPROCESS
  NS_DECL_INICSSVALUESEARCH

  inCSSValueSearch();

protected:
  virtual ~inCSSValueSearch();
  nsCOMPtr<inISearchObserver> mObserver;
  nsCOMPtr<nsIDOMDocument> mDocument;
  nsTArray<nsAutoString *>* mResults;
  nsCSSProperty* mProperties;
  nsString mLastResult;
  nsString mBaseURL;
  nsString mTextCriteria;
  int32_t mResultCount;
  uint32_t mPropertyCount;
  bool mIsActive;
  bool mHoldResults;
  bool mReturnRelativeURLs;
  bool mNormalizeChromeURLs;

  nsresult InitSearch();
  nsresult KillSearch(int16_t aResult);
  nsresult SearchStyleSheet(nsIDOMCSSStyleSheet* aStyleSheet, nsIURI* aBaseURI);
  nsresult SearchRuleList(nsIDOMCSSRuleList* aRuleList, nsIURI* aBaseURI);
  nsresult SearchStyleRule(nsIDOMCSSStyleRule* aStyleRule, nsIURI* aBaseURI);
  nsresult SearchStyleValue(const nsAFlatString& aValue, nsIURI* aBaseURI);
  nsresult EqualizeURL(nsAutoString* aURL);
};


#define IN_CSSVALUESEARCH_CID \
{ 0x4d977f60, 0xfbe7, 0x4583, { 0x8c, 0xb7, 0xf5, 0xed, 0x88, 0x22, 0x93, 0xef } }

#endif 
