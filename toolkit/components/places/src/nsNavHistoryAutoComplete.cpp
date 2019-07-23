

































































































#include "nsNavHistory.h"
#include "nsNetUtil.h"

#include "mozIStorageService.h"
#include "mozIStorageConnection.h"
#include "mozIStorageValueArray.h"
#include "mozIStorageStatement.h"
#include "mozIStorageFunction.h"
#include "mozStorageCID.h"
#include "mozStorageHelper.h"

#define NS_AUTOCOMPLETESIMPLERESULT_CONTRACTID \
  "@mozilla.org/autocomplete/simple-result;1"


#define AUTOCOMPLETE_NONPAGE_VISIT_COUNT_BOOST 5


#define AUTOCOMPLETE_TYPED_BOOST 5


#define AUTOCOMPLETE_BOOKMARKED_BOOST 5







#define AUTOCOMPLETE_MATCHES_PREFIX_PENALTY (-50)



#define AUTOCOMPLETE_MATCHES_SCHEME_PENALTY (-20)






#define AUTOCOMPLETE_MAX_PER_PREFIX 50



#define AUTOCOMPLETE_MAX_PER_TYPED 100

PRInt32 ComputeAutoCompletePriority(const nsAString& aUrl, PRInt32 aVisitCount,
                                    PRBool aWasTyped, PRBool aIsBookmarked);
nsresult NormalizeAutocompleteInput(const nsAString& aInput,
                                    nsString& aOutput);












struct AutoCompleteIntermediateResult
{
  AutoCompleteIntermediateResult(const nsString& aUrl, const nsString& aTitle,
                                 const nsString& aImage,
                                 PRInt32 aVisitCount, PRInt32 aPriority) :
    url(aUrl), title(aTitle), image(aImage), 
    visitCount(aVisitCount), priority(aPriority) {}
  nsString url;
  nsString title;
  nsString image;
  PRInt32 visitCount;
  PRInt32 priority;
};




class AutoCompleteResultComparator
{
public:
  AutoCompleteResultComparator(nsNavHistory* history) : mHistory(history) {}

  PRBool Equals(const AutoCompleteIntermediateResult& a,
                const AutoCompleteIntermediateResult& b) const {
    
    
    return PR_FALSE;
  }
  PRBool LessThan(const AutoCompleteIntermediateResult& match1,
                  const AutoCompleteIntermediateResult& match2) const {
    
    

    
    if (match1.priority != match2.priority)
    {
      return match1.priority > match2.priority;
    }
    else
    {
      
      PRBool isPath1 = PR_FALSE, isPath2 = PR_FALSE;
      if (!match1.url.IsEmpty())
        isPath1 = (match1.url.Last() == PRUnichar('/'));
      if (!match2.url.IsEmpty())
        isPath2 = (match2.url.Last() == PRUnichar('/'));

      if (isPath1 && !isPath2) return PR_FALSE; 
      if (!isPath1 && isPath2) return PR_TRUE;  

      
      PRInt32 prefix1 = mHistory->AutoCompleteGetPrefixLength(match1.url);
      PRInt32 prefix2 = mHistory->AutoCompleteGetPrefixLength(match2.url);

      
      
      
      PRInt32 ret = 0;
      mHistory->mCollation->CompareString(
          nsICollation::kCollationCaseInSensitive,
          Substring(match1.url, prefix1), Substring(match2.url, prefix2),
          &ret);
      if (ret != 0)
        return ret > 0;

      
      return prefix1 > prefix2;
    }
    return PR_FALSE;
  }
protected:
  nsNavHistory* mHistory;
};




nsresult
nsNavHistory::InitAutoComplete()
{
  nsresult rv = CreateAutoCompleteQuery();
  NS_ENSURE_SUCCESS(rv, rv);

  AutoCompletePrefix* ok;

  
  
  ok = mAutoCompletePrefixes.AppendElement(AutoCompletePrefix(NS_LITERAL_STRING("http://"), PR_FALSE));
  NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);
  ok = mAutoCompletePrefixes.AppendElement(AutoCompletePrefix(NS_LITERAL_STRING("http://www."), PR_TRUE));
  NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);
  ok = mAutoCompletePrefixes.AppendElement(AutoCompletePrefix(NS_LITERAL_STRING("ftp://"), PR_FALSE));
  NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);
  ok = mAutoCompletePrefixes.AppendElement(AutoCompletePrefix(NS_LITERAL_STRING("ftp://ftp."), PR_TRUE));
  NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);
  ok = mAutoCompletePrefixes.AppendElement(AutoCompletePrefix(NS_LITERAL_STRING("https://"), PR_FALSE));
  NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);
  ok = mAutoCompletePrefixes.AppendElement(AutoCompletePrefix(NS_LITERAL_STRING("https://www."), PR_TRUE));
  NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}







nsresult
nsNavHistory::CreateAutoCompleteQuery()
{
  nsCString sql;
  if (mAutoCompleteOnlyTyped) {
    sql = NS_LITERAL_CSTRING(
        "SELECT p.url, p.title, p.visit_count, p.typed, "
          "(SELECT b.fk FROM moz_bookmarks b WHERE b.fk = p.id), f.url "
        "FROM moz_places p "
        "LEFT OUTER JOIN moz_favicons f ON p.favicon_id = f.id "
        "WHERE p.url >= ?1 AND p.url < ?2 "
        "AND p.typed = 1 "
        "ORDER BY p.visit_count DESC "
        "LIMIT ");
  } else {
    sql = NS_LITERAL_CSTRING(
        "SELECT p.url, p.title, p.visit_count, p.typed, "
          "(SELECT b.fk FROM moz_bookmarks b WHERE b.fk = p.id), f.url "
        "FROM moz_places p "
        "LEFT OUTER JOIN moz_favicons f ON p.favicon_id = f.id "
        "WHERE p.url >= ?1 AND p.url < ?2 "
        "AND (p.hidden <> 1 OR p.typed = 1) "
        "ORDER BY p.visit_count DESC "
        "LIMIT ");
  }
  sql.AppendInt(AUTOCOMPLETE_MAX_PER_PREFIX);
  nsresult rv = mDBConn->CreateStatement(sql,
      getter_AddRefs(mDBAutoCompleteQuery));
  return rv;
}





NS_IMETHODIMP
nsNavHistory::StartSearch(const nsAString & aSearchString,
                          const nsAString & aSearchParam,
                          nsIAutoCompleteResult *aPreviousResult,
                          nsIAutoCompleteObserver *aListener)
{
  nsresult rv;

  NS_ENSURE_ARG_POINTER(aListener);

  nsCOMPtr<nsIAutoCompleteSimpleResult> result =
      do_CreateInstance(NS_AUTOCOMPLETESIMPLERESULT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  result->SetSearchString(aSearchString);

  
  
  
  
  
  
  
  
  
  
  if (aSearchString.IsEmpty()) {
    rv = AutoCompleteTypedSearch(result);
  } else {
    rv = AutoCompleteFullHistorySearch(aSearchString, result);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRUint32 count;
  result->GetMatchCount(&count);
  if (count > 0) {
    result->SetSearchResult(nsIAutoCompleteResult::RESULT_SUCCESS);
    result->SetDefaultIndex(0);
  } else {
    result->SetSearchResult(nsIAutoCompleteResult::RESULT_NOMATCH);
    result->SetDefaultIndex(-1);
  }

  rv = result->SetListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  aListener->OnSearchResult(this, result);
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistory::StopSearch()
{
  return NS_OK;
}











nsresult nsNavHistory::AutoCompleteTypedSearch(
                                            nsIAutoCompleteSimpleResult* result)
{
  
  nsCOMPtr<mozIStorageStatement> dbSelectStatement;
  nsCString sql = NS_LITERAL_CSTRING(
      "SELECT h.url, title, f.url "
      "FROM moz_historyvisits v JOIN moz_places h ON v.place_id = h.id "
      "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id " 
      "WHERE h.typed = 1 ORDER BY visit_date DESC LIMIT ");
  sql.AppendInt(AUTOCOMPLETE_MAX_PER_TYPED * 3);
  nsresult rv = mDBConn->CreateStatement(sql, getter_AddRefs(dbSelectStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsDataHashtable<nsStringHashKey, PRInt32> urls;
  if (! urls.Init(500))
    return NS_ERROR_OUT_OF_MEMORY;

  PRInt32 dummy;
  PRInt32 count = 0;
  PRBool hasMore = PR_FALSE;
  while (count < AUTOCOMPLETE_MAX_PER_TYPED &&
         NS_SUCCEEDED(dbSelectStatement->ExecuteStep(&hasMore)) && hasMore) {
    nsAutoString entryURL, entryTitle, entryImage;
    dbSelectStatement->GetString(0, entryURL);
    dbSelectStatement->GetString(1, entryTitle);
    dbSelectStatement->GetString(2, entryImage);

    if (! urls.Get(entryURL, &dummy)) {
      
      rv = result->AppendMatch(entryURL, entryTitle, entryImage, NS_LITERAL_STRING("favicon"));
      NS_ENSURE_SUCCESS(rv, rv);

      urls.Put(entryURL, 1);
      count ++;
    }
  }

  return NS_OK;
}









nsresult
nsNavHistory::AutoCompleteFullHistorySearch(const nsAString& aSearchString,
                                            nsIAutoCompleteSimpleResult* aResult)
{
  nsString searchString;
  nsresult rv = NormalizeAutocompleteInput(aSearchString, searchString);
  if (NS_FAILED(rv))
    return NS_OK; 

  nsTArray<AutoCompleteIntermediateResult> matches;

  
  
  PRUint32 i;
  const nsTArray<PRInt32> emptyArray;
  nsTArray<PRInt32> firstLevelMatches;
  nsTArray<PRInt32> secondLevelMatches;
  for (i = 0; i < mAutoCompletePrefixes.Length(); i ++) {
    if (StringBeginsWith(mAutoCompletePrefixes[i].prefix, searchString)) {
      if (mAutoCompletePrefixes[i].secondLevel)
        secondLevelMatches.AppendElement(i);
      else
        firstLevelMatches.AppendElement(i);
    }

    
    nsString cur = mAutoCompletePrefixes[i].prefix + searchString;

    
    nsTArray<PRInt32> curPrefixMatches;
    for (PRUint32 prefix = 0; prefix < mAutoCompletePrefixes.Length(); prefix ++) {
      if (StringBeginsWith(mAutoCompletePrefixes[prefix].prefix, cur))
        curPrefixMatches.AppendElement(prefix);
    }

    
    AutoCompleteQueryOnePrefix(cur, curPrefixMatches, 0, &matches);

    
    for (PRUint32 match = 0; match < curPrefixMatches.Length(); match ++) {
      AutoCompleteQueryOnePrefix(mAutoCompletePrefixes[curPrefixMatches[match]].prefix,
                                 emptyArray, AUTOCOMPLETE_MATCHES_PREFIX_PENALTY,
                                 &matches);
    }
  }

  
  if (firstLevelMatches.Length() > 0) {
    
    
    
    AutoCompleteQueryOnePrefix(searchString,
                               firstLevelMatches, 0, &matches);
  } else if (secondLevelMatches.Length() > 0) {
    
    
    
    
    AutoCompleteQueryOnePrefix(searchString,
                               secondLevelMatches, 0, &matches);

    
    
    
    for (PRUint32 match = 0; match < secondLevelMatches.Length(); match ++) {
      AutoCompleteQueryOnePrefix(mAutoCompletePrefixes[secondLevelMatches[match]].prefix,
                                 emptyArray, AUTOCOMPLETE_MATCHES_SCHEME_PENALTY,
                                 &matches);
    }
  } else {
    
    
    AutoCompleteQueryOnePrefix(searchString, emptyArray,
                               AUTOCOMPLETE_MATCHES_SCHEME_PENALTY, &matches);
  }

  
  if (matches.Length() > 0) {
    
    AutoCompleteResultComparator comparator(this);
    matches.Sort(comparator);

    rv = aResult->AppendMatch(matches[0].url, matches[0].title, matches[0].image, NS_LITERAL_STRING("favicon"));
    NS_ENSURE_SUCCESS(rv, rv);
    for (i = 1; i < matches.Length(); i ++) {
      
      
      if (!matches[i].url.Equals(matches[i-1].url)) {
        rv = aResult->AppendMatch(matches[i].url, matches[i].title, matches[i].image, NS_LITERAL_STRING("favicon"));
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }
  return NS_OK;
}



NS_IMETHODIMP
nsNavHistory::OnValueRemoved(nsIAutoCompleteSimpleResult* aResult,
                             const nsAString& aValue, PRBool aRemoveFromDb)
{
  if (!aRemoveFromDb)
    return NS_OK;

  nsresult rv;
  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), aValue);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RemovePage(uri);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}








nsresult
nsNavHistory::AutoCompleteQueryOnePrefix(const nsString& aSearchString,
    const nsTArray<PRInt32>& aExcludePrefixes,
    PRInt32 aPriorityDelta,
    nsTArray<AutoCompleteIntermediateResult>* aResult)
{
  
  
  
  nsCAutoString beginQuery = NS_ConvertUTF16toUTF8(aSearchString);
  if (beginQuery.IsEmpty())
    return NS_OK;
  nsCAutoString endQuery = beginQuery;
  unsigned char maxChar[6] = { 0xfd, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf };
  endQuery.Append(reinterpret_cast<const char*>(maxChar), 6);

  nsTArray<nsCString> ranges;
  if (aExcludePrefixes.Length() > 0) {
    
    ranges.AppendElement(beginQuery);
    for (PRUint32 i = 0; i < aExcludePrefixes.Length(); i ++) {
      nsCAutoString thisPrefix = NS_ConvertUTF16toUTF8(
          mAutoCompletePrefixes[aExcludePrefixes[i]].prefix);
      ranges.AppendElement(thisPrefix);
      thisPrefix.Append(reinterpret_cast<const char*>(maxChar), 6);
      ranges.AppendElement(thisPrefix);
    }
    ranges.AppendElement(endQuery);
    ranges.Sort();
  } else {
    
    ranges.AppendElement(beginQuery);
    ranges.AppendElement(endQuery);
  }

  NS_ASSERTION(ranges.Length() % 2 == 0, "Ranges should be pairs!");

  
  
  
  nsresult rv;
  for (PRUint32 range = 0; range < ranges.Length() - 1; range += 2) {
    mozStorageStatementScoper scoper(mDBAutoCompleteQuery);

    rv = mDBAutoCompleteQuery->BindUTF8StringParameter(0, ranges[range]);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBAutoCompleteQuery->BindUTF8StringParameter(1, ranges[range + 1]);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasMore;
    nsAutoString url, title, image;
    while (NS_SUCCEEDED(mDBAutoCompleteQuery->ExecuteStep(&hasMore)) && hasMore) {
      mDBAutoCompleteQuery->GetString(0, url);
      mDBAutoCompleteQuery->GetString(1, title);
      PRInt32 visitCount = mDBAutoCompleteQuery->AsInt32(2);
      PRInt32 priority = ComputeAutoCompletePriority(url, visitCount,
          mDBAutoCompleteQuery->AsInt32(3) > 0,
          mDBAutoCompleteQuery->AsInt32(4) > 0) + aPriorityDelta;
      mDBAutoCompleteQuery->GetString(5, image);
      aResult->AppendElement(AutoCompleteIntermediateResult(
          url, title, image, visitCount, priority));
    }
  }
  return NS_OK;
}




PRInt32
nsNavHistory::AutoCompleteGetPrefixLength(const nsString& aSpec)
{
  for (PRUint32 i = 0; i < mAutoCompletePrefixes.Length(); ++i) {
    if (StringBeginsWith(aSpec, mAutoCompletePrefixes[i].prefix))
      return mAutoCompletePrefixes[i].prefix.Length();
  }
  return 0; 
}

















PRInt32
ComputeAutoCompletePriority(const nsAString& aUrl, PRInt32 aVisitCount,
                            PRBool aWasTyped, PRBool aIsBookmarked)
{
  PRInt32 aPriority = aVisitCount;

  if (!aUrl.IsEmpty()) {
    
    if (aUrl.Last() == PRUnichar('/'))
      aPriority += AUTOCOMPLETE_NONPAGE_VISIT_COUNT_BOOST;
  }

  if (aWasTyped)
    aPriority += AUTOCOMPLETE_TYPED_BOOST;
  if (aIsBookmarked)
    aPriority += AUTOCOMPLETE_BOOKMARKED_BOOST;

  return aPriority;
}




nsresult NormalizeAutocompleteInput(const nsAString& aInput,
                                    nsString& aOutput)
{
  nsresult rv;

  if (aInput.IsEmpty()) {
    aOutput.Truncate();
    return NS_OK;
  }
  nsCAutoString input = NS_ConvertUTF16toUTF8(aInput);

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), input);
  PRBool isSchemeAdded = PR_FALSE;
  if (NS_FAILED(rv)) {
    
    isSchemeAdded = PR_TRUE;
    input = NS_LITERAL_CSTRING("http://") + input;

    rv = NS_NewURI(getter_AddRefs(uri), input);
    if (NS_FAILED(rv))
      return rv; 
  }

  nsCAutoString spec;
  rv = uri->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);
  if (spec.IsEmpty())
    return NS_OK; 

  aOutput = NS_ConvertUTF8toUTF16(spec);

  
  if (isSchemeAdded) {
    NS_ASSERTION(aOutput.Length() > 7, "String impossibly short");
    aOutput = Substring(aOutput, 7);
  }

  
  
  
  
  
  if (input[input.Length() - 1] != '/' && aOutput[aOutput.Length() - 1] == '/')
    aOutput.Truncate(aOutput.Length() - 1);

  return NS_OK;
}
