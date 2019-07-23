

































































































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






#define AUTOCOMPLETE_MAX_TRUNCATION_VISIT 6

PRInt32 ComputeAutoCompletePriority(const nsAString& aUrl, PRInt32 aVisitCount,
                                    PRBool aWasTyped, PRBool aIsBookmarked);
nsresult NormalizeAutocompleteInput(const nsAString& aInput,
                                    nsString& aOutput);












struct AutoCompleteIntermediateResult
{
  AutoCompleteIntermediateResult(const nsString& aUrl, const nsString& aTitle,
                                 PRInt32 aVisitCount, PRInt32 aPriority) :
    url(aUrl), title(aTitle), visitCount(aVisitCount), priority(aPriority) {}
  nsString url;
  nsString title;
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
          "(SELECT b.fk FROM moz_bookmarks b WHERE b.fk = p.id AND b.type = ?3) "
        "FROM moz_places p "
        "WHERE p.url >= ?1 AND p.url < ?2 "
        "AND p.typed = 1 "
        "ORDER BY p.visit_count DESC "
        "LIMIT ");
  } else {
    sql = NS_LITERAL_CSTRING(
        "SELECT p.url, p.title, p.visit_count, p.typed, "
          "(SELECT b.fk FROM moz_bookmarks b WHERE b.fk = p.id AND b.type = ?3) "
        "FROM moz_places p "
        "WHERE p.url >= ?1 AND p.url < ?2 "
        "AND (p.hidden <> 1 OR p.typed = 1) "
        "ORDER BY p.visit_count DESC "
        "LIMIT ");
  }
  sql.AppendInt(AUTOCOMPLETE_MAX_PER_PREFIX);
  nsresult rv = mDBConn->CreateStatement(sql,
      getter_AddRefs(mDBAutoCompleteQuery));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBAutoCompleteQuery->BindInt32Parameter(2, nsINavBookmarksService::TYPE_BOOKMARK);
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
      "SELECT url, title "
      "FROM moz_historyvisits v JOIN moz_places h ON v.place_id = h.id "
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
    nsAutoString entryURL, entryTitle;
    dbSelectStatement->GetString(0, entryURL);
    dbSelectStatement->GetString(1, entryTitle);

    if (! urls.Get(entryURL, &dummy)) {
      
      rv = result->AppendMatch(entryURL, entryTitle);
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

  
  AutoCompleteResultComparator comparator(this);
  matches.Sort(comparator);

  
  nsAutoString zerothEntry;
  if (matches.Length() > 0 &&
      matches[0].visitCount <= AUTOCOMPLETE_MAX_TRUNCATION_VISIT) {
    
    
    
    
    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), NS_ConvertUTF16toUTF8(matches[0].url));
    NS_ENSURE_SUCCESS(rv, rv);
    uri->SetPath(NS_LITERAL_CSTRING("/"));

    nsCAutoString spec;
    uri->GetSpec(spec);
    zerothEntry = NS_ConvertUTF8toUTF16(spec);

    if (! zerothEntry.Equals(matches[0].url))
      aResult->AppendMatch(zerothEntry, EmptyString());
    rv = aResult->AppendMatch(matches[0].url, matches[0].title);
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (matches.Length() > 0) {
    
    rv = aResult->AppendMatch(matches[0].url, matches[0].title);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  for (i = 1; i < matches.Length(); i ++) {
    
    
    if (! matches[i].url.Equals(matches[i-1].url) &&
        ! zerothEntry.Equals(matches[i].url)) {
      rv = aResult->AppendMatch(matches[i].url, matches[i].title);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
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
  endQuery.Append(NS_REINTERPRET_CAST(const char*, maxChar), 6);

  nsTArray<nsCString> ranges;
  if (aExcludePrefixes.Length() > 0) {
    
    ranges.AppendElement(beginQuery);
    for (PRUint32 i = 0; i < aExcludePrefixes.Length(); i ++) {
      nsCAutoString thisPrefix = NS_ConvertUTF16toUTF8(
          mAutoCompletePrefixes[aExcludePrefixes[i]].prefix);
      ranges.AppendElement(thisPrefix);
      thisPrefix.Append(NS_REINTERPRET_CAST(const char*, maxChar), 6);
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
    nsAutoString url, title;
    while (NS_SUCCEEDED(mDBAutoCompleteQuery->ExecuteStep(&hasMore)) && hasMore) {
      mDBAutoCompleteQuery->GetString(0, url);
      mDBAutoCompleteQuery->GetString(1, title);
      PRInt32 visitCount = mDBAutoCompleteQuery->AsInt32(2);
      PRInt32 priority = ComputeAutoCompletePriority(url, visitCount,
          mDBAutoCompleteQuery->AsInt32(3) > 0,
          mDBAutoCompleteQuery->AsInt32(4) > 0) + aPriorityDelta;
      aResult->AppendElement(AutoCompleteIntermediateResult(
          url, title, visitCount, priority));
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
