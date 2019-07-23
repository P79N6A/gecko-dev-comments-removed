






























































#include "nsNavHistory.h"
#include "nsNetUtil.h"
#include "nsEscape.h"
#include "nsThreadUtils.h"

#include "mozIStorageService.h"
#include "mozIStorageConnection.h"
#include "mozIStorageValueArray.h"
#include "mozIStorageStatement.h"
#include "mozIStorageFunction.h"
#include "mozStorageCID.h"
#include "mozStorageHelper.h"
#include "nsFaviconService.h"
#include "nsUnicharUtils.h"
#include "nsNavBookmarks.h"
#include "nsPrintfCString.h"
#include "nsILivemarkService.h"
#include "mozIStoragePendingStatement.h"
#include "mozIStorageStatementCallback.h"
#include "mozIStorageError.h"
#include "nsPlacesTables.h"

#define NS_AUTOCOMPLETESIMPLERESULT_CONTRACTID \
  "@mozilla.org/autocomplete/simple-result;1"


#define GET_BEHAVIOR(aBitName) \
  (mAutoCompleteCurrentBehavior & kAutoCompleteBehavior##aBitName)
#define SET_BEHAVIOR(aBitName) \
  mAutoCompleteCurrentBehavior |= kAutoCompleteBehavior##aBitName




#define BOOK_TAG_FRAG(name, column, forTag) nsPrintfCString(200, ", (" \
  "SELECT %s " \
  "FROM moz_bookmarks b " \
  "JOIN moz_bookmarks t ON t.id = b.parent AND t.parent %s= ?1 " \
  "WHERE b.fk = h.id AND b.type = %d " \
  "%s) AS %s", \
  column, \
  forTag ? "" : "!", \
  nsINavBookmarksService::TYPE_BOOKMARK, \
  forTag ? "AND LENGTH(t.title) > 0" : "ORDER BY b.lastModified DESC LIMIT 1", \
  name)


#define BOOK_TAG_SQL (\
  BOOK_TAG_FRAG("parent", "b.parent", 0) + \
  BOOK_TAG_FRAG("bookmark", "b.title", 0) + \
  BOOK_TAG_FRAG("tags", "GROUP_CONCAT(t.title, ',')", 1))





const PRUnichar kTitleTagsSeparatorChars[] = { ' ', 0x2013, ' ', 0 };
#define TITLE_TAGS_SEPARATOR nsAutoString(kTitleTagsSeparatorChars)


#define BEST_FAVICON_FOR_REVHOST( __table_name ) \
  "(SELECT f.url FROM " __table_name " " \
   "JOIN moz_favicons f ON f.id = favicon_id " \
   "WHERE rev_host = IFNULL( " \
     "(SELECT rev_host FROM moz_places_temp WHERE id = b.fk), " \
     "(SELECT rev_host FROM moz_places WHERE id = b.fk)) " \
   "ORDER BY frecency DESC LIMIT 1) "

#define PLACES_AUTOCOMPLETE_FEEDBACK_UPDATED_TOPIC "places-autocomplete-feedback-updated"



class AutoCompleteStatementCallbackNotifier : public mozIStorageStatementCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGESTATEMENTCALLBACK
};


NS_IMPL_ISUPPORTS1(AutoCompleteStatementCallbackNotifier,
                   mozIStorageStatementCallback)

NS_IMETHODIMP
AutoCompleteStatementCallbackNotifier::HandleCompletion(PRUint16 aReason)
{
  if (aReason != mozIStorageStatementCallback::REASON_FINISHED)
    return NS_ERROR_UNEXPECTED;

  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = observerService->NotifyObservers(nsnull,
                                        PLACES_AUTOCOMPLETE_FEEDBACK_UPDATED_TOPIC,
                                        nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
AutoCompleteStatementCallbackNotifier::HandleError(mozIStorageError *aError)
{
  PRInt32 result;
  nsresult rv = aError->GetResult(&result);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString message;
  rv = aError->GetMessage(message);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString warnMsg;
  warnMsg.Append("An error occured while executing an async statement: ");
  warnMsg.Append(result);
  warnMsg.Append(" ");
  warnMsg.Append(message);
  NS_WARNING(warnMsg.get());

  return NS_OK;
}

NS_IMETHODIMP
AutoCompleteStatementCallbackNotifier::HandleResult(mozIStorageResultSet *aResultSet)
{
  NS_ASSERTION(PR_FALSE, "You cannot use AutoCompleteStatementCallbackNotifier to get async statements resultset");
  return NS_OK;
}

void GetAutoCompleteBaseQuery(nsACString& aQuery) {












  aQuery = NS_LITERAL_CSTRING(
      "SELECT h.url, h.title, f.url") + BOOK_TAG_SQL + NS_LITERAL_CSTRING(", "
        "h.visit_count, h.typed "
      "FROM ("
        "SELECT " MOZ_PLACES_COLUMNS " FROM moz_places_temp "
        "UNION ALL "
        "SELECT " MOZ_PLACES_COLUMNS " FROM moz_places "
        "ORDER BY frecency DESC LIMIT ?2 OFFSET ?3) h "
      "LEFT OUTER JOIN moz_favicons f ON f.id = h.favicon_id "
      "WHERE h.frecency <> 0 "
      "{ADDITIONAL_CONDITIONS}");
}







inline PRBool
StartsWithJS(const nsAString &aString)
{
  return StringBeginsWith(aString, NS_LITERAL_STRING("javascript:"));
}












PLDHashOperator
HashedURLsToArray(const nsAString &aKey, PRBool aData, void *aArg)
{
  
  static_cast<nsTArray<nsString> *>(aArg)->AppendElement(aKey);
  return PL_DHASH_NEXT;
}








inline PRBool
IsWordBoundary(const PRUnichar &aChar)
{
  
  
  
  return !(PRUnichar('a') <= aChar && aChar <= PRUnichar('z'));
}









PRBool
FindOnBoundary(const nsAString &aToken, const nsAString &aTarget)
{
  
  const nsCaseInsensitiveStringComparator caseInsensitiveCompare;

  
  if (aTarget.IsEmpty())
    return PR_FALSE;

  nsAString::const_iterator tokenStart, tokenEnd;
  aToken.BeginReading(tokenStart);
  aToken.EndReading(tokenEnd);

  nsAString::const_iterator targetStart, targetEnd;
  aTarget.BeginReading(targetStart);
  aTarget.EndReading(targetEnd);

  
  
  do {
    
    nsAString::const_iterator testToken(tokenStart);
    nsAString::const_iterator testTarget(targetStart);

    
    while (!caseInsensitiveCompare(*testToken, *testTarget)) {
      
      testToken++;
      testTarget++;

      
      if (testToken == tokenEnd)
        return PR_TRUE;

      
      if (testTarget == targetEnd)
        return PR_FALSE;
    }

    
    
    
    
    if (!IsWordBoundary(ToLowerCase(*targetStart++)))
      while (targetStart != targetEnd && !IsWordBoundary(*targetStart))
        targetStart++;

    
  } while (targetStart != targetEnd);

  return PR_FALSE;
}




inline PRBool
FindAnywhere(const nsAString &aToken, const nsAString &aTarget)
{
  return CaseInsensitiveFindInReadable(aToken, aTarget);
}




inline PRBool
FindBeginning(const nsAString &aToken, const nsAString &aTarget)
{
  return StringBeginsWith(aTarget, aToken, nsCaseInsensitiveStringComparator());
}


nsresult
nsNavHistory::InitAutoComplete()
{
  nsresult rv = CreateAutoCompleteQueries();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mCurrentResultURLs.Init(128))
    return NS_ERROR_OUT_OF_MEMORY;

  if (!mLivemarkFeedItemIds.Init(128))
    return NS_ERROR_OUT_OF_MEMORY;

  if (!mLivemarkFeedURIs.Init(128))
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}





mozIStorageStatement*
nsNavHistory::GetDBAutoCompleteHistoryQuery()
{
  if (mDBAutoCompleteHistoryQuery)
    return mDBAutoCompleteHistoryQuery;

  nsCString AutoCompleteHistoryQuery;
  GetAutoCompleteBaseQuery(AutoCompleteHistoryQuery);
  AutoCompleteHistoryQuery.ReplaceSubstring("{ADDITIONAL_CONDITIONS}",
                                            "AND h.visit_count > 0");
  nsresult rv = mDBConn->CreateStatement(AutoCompleteHistoryQuery,
    getter_AddRefs(mDBAutoCompleteHistoryQuery));
  NS_ENSURE_SUCCESS(rv, nsnull);

  return mDBAutoCompleteHistoryQuery;
}





mozIStorageStatement*
nsNavHistory::GetDBAutoCompleteStarQuery()
{
  if (mDBAutoCompleteStarQuery)
    return mDBAutoCompleteStarQuery;

  nsCString AutoCompleteStarQuery;
  GetAutoCompleteBaseQuery(AutoCompleteStarQuery);
  AutoCompleteStarQuery.ReplaceSubstring("{ADDITIONAL_CONDITIONS}",
                                         "AND bookmark IS NOT NULL");
  nsresult rv = mDBConn->CreateStatement(AutoCompleteStarQuery,
    getter_AddRefs(mDBAutoCompleteStarQuery));
  NS_ENSURE_SUCCESS(rv, nsnull);

  return mDBAutoCompleteStarQuery;
}





mozIStorageStatement*
nsNavHistory::GetDBAutoCompleteTagsQuery()
{
  if (mDBAutoCompleteTagsQuery)
    return mDBAutoCompleteTagsQuery;

  nsCString AutoCompleteTagsQuery;
  GetAutoCompleteBaseQuery(AutoCompleteTagsQuery);
  AutoCompleteTagsQuery.ReplaceSubstring("{ADDITIONAL_CONDITIONS}",
                                         "AND tags IS NOT NULL");
  nsresult rv = mDBConn->CreateStatement(AutoCompleteTagsQuery,
    getter_AddRefs(mDBAutoCompleteTagsQuery));
  NS_ENSURE_SUCCESS(rv, nsnull);

  return mDBAutoCompleteTagsQuery;
}





mozIStorageStatement*
nsNavHistory::GetDBFeedbackIncrease()
{
  if (mDBFeedbackIncrease)
    return mDBFeedbackIncrease;

  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    
    "INSERT OR REPLACE INTO moz_inputhistory "
      
      "SELECT h.id, IFNULL(i.input, ?1), IFNULL(i.use_count, 0) * .9 + 1 "
      "FROM moz_places_temp h "
      "LEFT JOIN moz_inputhistory i ON i.place_id = h.id AND i.input = ?1 "
      "WHERE url = ?2 "
      "UNION ALL "
      "SELECT h.id, IFNULL(i.input, ?1), IFNULL(i.use_count, 0) * .9 + 1 "
      "FROM moz_places h "
      "LEFT JOIN moz_inputhistory i ON i.place_id = h.id AND i.input = ?1 "
      "WHERE url = ?2 "
        "AND h.id NOT IN (SELECT id FROM moz_places_temp)"),
    getter_AddRefs(mDBFeedbackIncrease));
  NS_ENSURE_SUCCESS(rv, nsnull);

  return mDBFeedbackIncrease;
}







nsresult
nsNavHistory::CreateAutoCompleteQueries()
{
  nsCString AutoCompleteQuery;
  GetAutoCompleteBaseQuery(AutoCompleteQuery);
  AutoCompleteQuery.ReplaceSubstring("{ADDITIONAL_CONDITIONS}", "");
  nsresult rv = mDBConn->CreateStatement(AutoCompleteQuery,
    getter_AddRefs(mDBAutoCompleteQuery));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString AutoCompleteTypedQuery;
  GetAutoCompleteBaseQuery(AutoCompleteTypedQuery);
  AutoCompleteTypedQuery.ReplaceSubstring("{ADDITIONAL_CONDITIONS}",
                                          "AND h.typed = 1");
  rv = mDBConn->CreateStatement(AutoCompleteTypedQuery,
    getter_AddRefs(mDBAutoCompleteTypedQuery));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  nsCString sql = NS_LITERAL_CSTRING(
    "/* do not warn (bug 487789) */ "
    "SELECT IFNULL(h_t.url, h.url), IFNULL(h_t.title, h.title), f.url ") +
      BOOK_TAG_SQL + NS_LITERAL_CSTRING(", "
      "IFNULL(h_t.visit_count, h.visit_count), IFNULL(h_t.typed, h.typed), rank "
    "FROM ( "
      "SELECT ROUND(MAX(((i.input = ?2) + (SUBSTR(i.input, 1, LENGTH(?2)) = ?2)) * "
        "i.use_count), 1) AS rank, place_id "
      "FROM moz_inputhistory i "
      "GROUP BY i.place_id HAVING rank > 0 "
      ") AS i "
    "LEFT JOIN moz_places h ON h.id = i.place_id "
    "LEFT JOIN moz_places_temp h_t ON h_t.id = i.place_id "
    "LEFT JOIN moz_favicons f ON f.id = IFNULL(h_t.favicon_id, h.favicon_id) "
    "WHERE IFNULL(h_t.url, h.url) NOTNULL "
    "ORDER BY rank DESC, IFNULL(h_t.frecency, h.frecency) DESC");
  rv = mDBConn->CreateStatement(sql, getter_AddRefs(mDBAdaptiveQuery));
  NS_ENSURE_SUCCESS(rv, rv);

  sql = NS_LITERAL_CSTRING(
    "/* do not warn (bug 487787) */ "
    "SELECT IFNULL( "
        "(SELECT REPLACE(url, '%s', ?2) FROM moz_places_temp WHERE id = b.fk), "
        "(SELECT REPLACE(url, '%s', ?2) FROM moz_places WHERE id = b.fk) "
      ") AS search_url, IFNULL(h_t.title, h.title), "
      "COALESCE(f.url, "
        BEST_FAVICON_FOR_REVHOST("moz_places_temp") ", "
        BEST_FAVICON_FOR_REVHOST("moz_places")
      "), "
      "b.parent, b.title, NULL, IFNULL(h_t.visit_count, h.visit_count), "
      "IFNULL(h_t.typed, h.typed) "
    "FROM moz_keywords k "
    "JOIN moz_bookmarks b ON b.keyword_id = k.id "
    "LEFT JOIN moz_places AS h ON h.url = search_url "
    "LEFT JOIN moz_places_temp AS h_t ON h_t.url = search_url "
    "LEFT JOIN moz_favicons f ON f.id = IFNULL(h_t.favicon_id, h.favicon_id) "
    "WHERE LOWER(k.keyword) = LOWER(?1) "
    "ORDER BY IFNULL(h_t.frecency, h.frecency) DESC");
  rv = mDBConn->CreateStatement(sql, getter_AddRefs(mDBKeywordQuery));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}



nsresult
nsNavHistory::StartAutoCompleteTimer(PRUint32 aMilliseconds)
{
  nsresult rv;

  if (!mAutoCompleteTimer) {
    mAutoCompleteTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  rv = mAutoCompleteTimer->InitWithFuncCallback(AutoCompleteTimerCallback, this,
                                                aMilliseconds,
                                                nsITimer::TYPE_ONE_SHOT);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}



void 
nsNavHistory::AutoCompleteTimerCallback(nsITimer* aTimer, void* aClosure)
{
  nsNavHistory* history = static_cast<nsNavHistory*>(aClosure);
  (void)history->PerformAutoComplete();
}

nsresult 
nsNavHistory::PerformAutoComplete()
{
  
  if (!mCurrentListener)
    return NS_OK;

  nsresult rv;
  
  if (!mCurrentChunkOffset) {
    
    if (mCurrentSearchTokens.Length()) {
      rv = AutoCompleteKeywordSearch();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    rv = AutoCompleteAdaptiveSearch();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRBool moreChunksToSearch = PR_FALSE;
  
  if (mDBPreviousQuery) {
    rv = AutoCompletePreviousSearch();
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    if (moreChunksToSearch = mPreviousChunkOffset != -1)
      mCurrentChunkOffset = mPreviousChunkOffset - mAutoCompleteSearchChunkSize;
  } else {
    rv = AutoCompleteFullHistorySearch(&moreChunksToSearch);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  PRBool notEnoughResults = !AutoCompleteHasEnoughResults();
  if (!moreChunksToSearch) {
    
    
    if (notEnoughResults && mCurrentMatchType == MATCH_BOUNDARY_ANYWHERE) {
      mCurrentMatchType = MATCH_ANYWHERE;
      mCurrentChunkOffset = -mAutoCompleteSearchChunkSize;
      moreChunksToSearch = PR_TRUE;
    } else {
      mCurrentChunkOffset = -1;
    }
  } else {
    
    moreChunksToSearch = notEnoughResults;
  }

  
  PRUint32 count;
  mCurrentResult->GetMatchCount(&count); 

  if (count > 0) {
    mCurrentResult->SetSearchResult(moreChunksToSearch ?
      nsIAutoCompleteResult::RESULT_SUCCESS_ONGOING :
      nsIAutoCompleteResult::RESULT_SUCCESS);
    mCurrentResult->SetDefaultIndex(0);
  } else {
    mCurrentResult->SetSearchResult(moreChunksToSearch ?
      nsIAutoCompleteResult::RESULT_NOMATCH_ONGOING :
      nsIAutoCompleteResult::RESULT_NOMATCH);
    mCurrentResult->SetDefaultIndex(-1);
  }

  rv = mCurrentResult->SetListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  mCurrentListener->OnSearchResult(this, mCurrentResult);
 
  
  
  if (moreChunksToSearch) {
    mCurrentChunkOffset += mAutoCompleteSearchChunkSize;
    rv = StartAutoCompleteTimer(mAutoCompleteSearchTimeout);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    DoneSearching(PR_TRUE);
  }
  return NS_OK;
}

void
nsNavHistory::DoneSearching(PRBool aFinished)
{
  mPreviousMatchType = mCurrentMatchType;
  mPreviousChunkOffset = mCurrentChunkOffset;
  mAutoCompleteFinishedSearch = aFinished;
  mCurrentResult = nsnull;
  mCurrentListener = nsnull;
}




NS_IMETHODIMP
nsNavHistory::StartSearch(const nsAString & aSearchString,
                          const nsAString & aSearchParam,
                          nsIAutoCompleteResult *aPreviousResult,
                          nsIAutoCompleteObserver *aListener)
{
  
  
  

  NS_ENSURE_ARG_POINTER(aListener);
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  
  if (!mTextURIService)
    mTextURIService = do_GetService(NS_ITEXTTOSUBURI_CONTRACTID);

  
  PRUint32 prevMatchCount = mCurrentResultURLs.Count();
  nsAutoString prevSearchString(mCurrentSearchString);

  
  mOrigSearchString = aSearchString;
  
  mOrigSearchString.Trim(" \r\n\t\b");
  
  ToLowerCase(mOrigSearchString, mCurrentSearchString);

  
  mCurrentSearchString = FixupURIText(mCurrentSearchString);

  mCurrentListener = aListener;

  nsresult rv;
  mCurrentResult = do_CreateInstance(NS_AUTOCOMPLETESIMPLERESULT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mCurrentResult->SetSearchString(aSearchString);

  
  
  
  
  
  
  if (!mAutoCompleteEnabled ||
      (!prevSearchString.IsEmpty() &&
       StringBeginsWith(mCurrentSearchString, prevSearchString) &&
       (StartsWithJS(prevSearchString) == StartsWithJS(mCurrentSearchString)))) {

    
    if (!mAutoCompleteEnabled ||
        (mAutoCompleteFinishedSearch && prevMatchCount == 0)) {
      
      mCurrentResult->SetSearchResult(nsIAutoCompleteResult::RESULT_NOMATCH);
      mCurrentResult->SetDefaultIndex(-1);

      rv = mCurrentResult->SetListener(this);
      NS_ENSURE_SUCCESS(rv, rv);

      (void)mCurrentListener->OnSearchResult(this, mCurrentResult);
      DoneSearching(PR_TRUE);

      return NS_OK;
    } else {
      
      
      
      

      
      
      nsCString bindings;
      for (PRUint32 i = 0; i < prevMatchCount; i++) {
        if (i)
          bindings += NS_LITERAL_CSTRING(",");

        
        bindings += nsPrintfCString("?%d", i + 2);
      }

      nsCString sql = NS_LITERAL_CSTRING(
        "SELECT h.url, h.title, f.url") + BOOK_TAG_SQL + NS_LITERAL_CSTRING(", "
          "h.visit_count, h.typed "
        "FROM ( "
          "SELECT " MOZ_PLACES_COLUMNS " FROM moz_places_temp "
          "WHERE url IN (") + bindings + NS_LITERAL_CSTRING(") "
          "UNION ALL "
          "SELECT " MOZ_PLACES_COLUMNS " FROM moz_places "
          "WHERE id NOT IN (SELECT id FROM moz_places_temp) "
          "AND url IN (") + bindings + NS_LITERAL_CSTRING(") "
        ") AS h "
        "LEFT OUTER JOIN moz_favicons f ON f.id = h.favicon_id "
        "ORDER BY h.frecency DESC");

      rv = mDBConn->CreateStatement(sql, getter_AddRefs(mDBPreviousQuery));
      NS_ENSURE_SUCCESS(rv, rv);

      
      nsTArray<nsString> urls;
      (void)mCurrentResultURLs.EnumerateRead(HashedURLsToArray, &urls);

      
      for (PRUint32 i = 0; i < prevMatchCount; i++) {
        rv = mDBPreviousQuery->BindStringParameter(i + 1, urls[i]);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      
      mCurrentMatchType = mPreviousMatchType;
    }
  } else {
    
    mDBPreviousQuery = nsnull;
    
    mCurrentMatchType = mAutoCompleteMatchBehavior;
  }

  mAutoCompleteFinishedSearch = PR_FALSE;
  mCurrentChunkOffset = 0;
  mCurrentResultURLs.Clear();
  mCurrentSearchTokens.Clear();
  mLivemarkFeedItemIds.Clear();
  mLivemarkFeedURIs.Clear();

  
  GenerateSearchTokens();

  
  ProcessTokensForSpecialSearch();

  
  
  
  
  
  
  mozStorageStatementScoper scope(mFoldersWithAnnotationQuery);

  rv = mFoldersWithAnnotationQuery->BindUTF8StringParameter(0, NS_LITERAL_CSTRING(LMANNO_FEEDURI));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  while (NS_SUCCEEDED(mFoldersWithAnnotationQuery->ExecuteStep(&hasMore)) && hasMore) {
    PRInt64 itemId = 0;
    rv = mFoldersWithAnnotationQuery->GetInt64(0, &itemId);
    NS_ENSURE_SUCCESS(rv, rv);
    mLivemarkFeedItemIds.Put(itemId, PR_TRUE);
    nsAutoString feedURI;
    
    rv = mFoldersWithAnnotationQuery->GetString(1, feedURI);
    NS_ENSURE_SUCCESS(rv, rv);
    mLivemarkFeedURIs.Put(feedURI, PR_TRUE);
  }

  
  rv = StartAutoCompleteTimer(0);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}



NS_IMETHODIMP
nsNavHistory::StopSearch()
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  if (mAutoCompleteTimer)
    mAutoCompleteTimer->Cancel();

  DoneSearching(PR_FALSE);

  return NS_OK;
}

void
nsNavHistory::GenerateSearchTokens()
{
  
  nsString::const_iterator strStart, strEnd;
  mCurrentSearchString.BeginReading(strStart);
  mCurrentSearchString.EndReading(strEnd);
  nsString::const_iterator start = strStart, end = strEnd;
  while (FindInReadable(NS_LITERAL_STRING(" "), start, end)) {
    
    nsAutoString currentMatch(Substring(strStart, start));
    AddSearchToken(currentMatch);

    
    strStart = start = end;
    end = strEnd;
  }
  
  
  nsAutoString lastMatch(Substring(strStart, strEnd));
  AddSearchToken(lastMatch);
} 

inline void
nsNavHistory::AddSearchToken(nsAutoString &aToken)
{
  aToken.Trim("\r\n\t\b");
  if (!aToken.IsEmpty())
    mCurrentSearchTokens.AppendElement(aToken);
}

void
nsNavHistory::ProcessTokensForSpecialSearch()
{
  
  mAutoCompleteCurrentBehavior = mAutoCompleteDefaultBehavior;

  
  for (PRInt32 i = PRInt32(mCurrentSearchTokens.Length()); --i >= 0;) {
    PRBool needToRemove = PR_TRUE;
    const nsString& token = mCurrentSearchTokens[i];

    if (token.Equals(mAutoCompleteRestrictHistory))
      SET_BEHAVIOR(History);
    else if (token.Equals(mAutoCompleteRestrictBookmark))
      SET_BEHAVIOR(Bookmark);
    else if (token.Equals(mAutoCompleteRestrictTag))
      SET_BEHAVIOR(Tag);
    else if (token.Equals(mAutoCompleteMatchTitle))
      SET_BEHAVIOR(Title);
    else if (token.Equals(mAutoCompleteMatchUrl))
      SET_BEHAVIOR(Url);
    else if (token.Equals(mAutoCompleteRestrictTyped))
      SET_BEHAVIOR(Typed);
    else
      needToRemove = PR_FALSE;

    
    if (needToRemove)
      mCurrentSearchTokens.RemoveElementAt(i);
  }

  
  if (mOrigSearchString.IsEmpty()) {
    SET_BEHAVIOR(History);
    SET_BEHAVIOR(Typed);
  }

  
  
  mDBCurrentQuery = GET_BEHAVIOR(Tag) ? GetDBAutoCompleteTagsQuery() :
    GET_BEHAVIOR(Bookmark) ? GetDBAutoCompleteStarQuery() :
    GET_BEHAVIOR(Typed) ? static_cast<mozIStorageStatement *>(mDBAutoCompleteTypedQuery) :
    GET_BEHAVIOR(History) ? GetDBAutoCompleteHistoryQuery() :
    static_cast<mozIStorageStatement *>(mDBAutoCompleteQuery);
}

nsresult
nsNavHistory::AutoCompleteKeywordSearch()
{
  mozStorageStatementScoper scope(mDBKeywordQuery);

  
  nsCAutoString params;
  PRInt32 paramPos = mOrigSearchString.FindChar(' ') + 1;
  
  
  NS_Escape(NS_ConvertUTF16toUTF8(Substring(mOrigSearchString, paramPos)),
    params, url_XPAlphas);

  
  const nsAString &keyword = Substring(mOrigSearchString, 0,
    paramPos ? paramPos - 1 : mOrigSearchString.Length());
  nsresult rv = mDBKeywordQuery->BindStringParameter(0, keyword);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBKeywordQuery->BindUTF8StringParameter(1, params);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AutoCompleteProcessSearch(mDBKeywordQuery, QUERY_KEYWORD);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsNavHistory::AutoCompleteAdaptiveSearch()
{
  mozStorageStatementScoper scope(mDBAdaptiveQuery);

  nsresult rv = mDBAdaptiveQuery->BindInt64Parameter(0, GetTagsFolder());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBAdaptiveQuery->BindStringParameter(1, mCurrentSearchString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AutoCompleteProcessSearch(mDBAdaptiveQuery, QUERY_FILTERED);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsNavHistory::AutoCompletePreviousSearch()
{
  nsresult rv = mDBPreviousQuery->BindInt64Parameter(0, GetTagsFolder());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AutoCompleteProcessSearch(mDBPreviousQuery, QUERY_FILTERED);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mDBPreviousQuery = nsnull;

  return NS_OK;
}










nsresult
nsNavHistory::AutoCompleteFullHistorySearch(PRBool* aHasMoreResults)
{
  mozStorageStatementScoper scope(mDBCurrentQuery);

  nsresult rv = mDBCurrentQuery->BindInt64Parameter(0, GetTagsFolder());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBCurrentQuery->BindInt32Parameter(1, mAutoCompleteSearchChunkSize);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBCurrentQuery->BindInt32Parameter(2, mCurrentChunkOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AutoCompleteProcessSearch(mDBCurrentQuery, QUERY_FILTERED, aHasMoreResults);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsNavHistory::AutoCompleteProcessSearch(mozIStorageStatement* aQuery,
                                        const QueryType aType,
                                        PRBool *aHasMoreResults)
{
  
  
  if (!aHasMoreResults && AutoCompleteHasEnoughResults())
    return NS_OK;

  nsFaviconService* faviconService = nsFaviconService::GetFaviconService();
  NS_ENSURE_TRUE(faviconService, NS_ERROR_OUT_OF_MEMORY);

  
  PRBool filterJavascript = mAutoCompleteFilterJavascript &&
    !StartsWithJS(mCurrentSearchString);

  
  PRBool (*tokenMatchesTarget)(const nsAString &, const nsAString &);
  switch (mCurrentMatchType) {
    case MATCH_ANYWHERE:
      tokenMatchesTarget = FindAnywhere;
      break;
    case MATCH_BEGINNING:
      tokenMatchesTarget = FindBeginning;
      break;
    case MATCH_BOUNDARY_ANYWHERE:
    case MATCH_BOUNDARY:
    default:
      tokenMatchesTarget = FindOnBoundary;
      break;
  }

  PRBool hasMore = PR_FALSE;
  
  while (NS_SUCCEEDED(aQuery->ExecuteStep(&hasMore)) && hasMore) {
    nsAutoString escapedEntryURL;
    nsresult rv = aQuery->GetString(kAutoCompleteIndex_URL, escapedEntryURL);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (filterJavascript && StartsWithJS(escapedEntryURL))
      continue;

    
    
    
    
    
    
    PRBool dummy;
    if (!mCurrentResultURLs.Get(escapedEntryURL, &dummy)) {
      PRInt64 parentId = 0;
      nsAutoString entryTitle, entryFavicon, entryBookmarkTitle;
      rv = aQuery->GetString(kAutoCompleteIndex_Title, entryTitle);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aQuery->GetString(kAutoCompleteIndex_FaviconURL, entryFavicon);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aQuery->GetInt64(kAutoCompleteIndex_ParentId, &parentId);
      NS_ENSURE_SUCCESS(rv, rv);
      
      
      if (parentId) {
        rv = aQuery->GetString(kAutoCompleteIndex_BookmarkTitle, entryBookmarkTitle);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      nsAutoString entryTags;
      rv = aQuery->GetString(kAutoCompleteIndex_Tags, entryTags);
      NS_ENSURE_SUCCESS(rv, rv);
      PRInt32 visitCount = 0;
      rv = aQuery->GetInt32(kAutoCompleteIndex_VisitCount, &visitCount);
      NS_ENSURE_SUCCESS(rv, rv);
      PRInt32 typed = 0;
      rv = aQuery->GetInt32(kAutoCompleteIndex_Typed, &typed);

      
      nsAutoString title =
        entryBookmarkTitle.IsEmpty() ? entryTitle : entryBookmarkTitle;

      nsString style;
      switch (aType) {
        case QUERY_KEYWORD: {
          
          
          
          if (entryTitle.IsEmpty())
            style = NS_LITERAL_STRING("keyword");
          else
            title = entryTitle;

          break;
        }
        case QUERY_FILTERED: {
          
          if (aHasMoreResults)
            *aHasMoreResults = PR_TRUE;

          
          
          
          
          PRBool matchAll = !((GET_BEHAVIOR(History) && visitCount == 0) ||
                              (GET_BEHAVIOR(Typed) && typed == 0) ||
                              (GET_BEHAVIOR(Bookmark) && !parentId) ||
                              (GET_BEHAVIOR(Tag) && entryTags.IsEmpty()));

          
          nsString entryURL = FixupURIText(escapedEntryURL);

          
          
          for (PRUint32 i = 0; i < mCurrentSearchTokens.Length() && matchAll; i++) {
            const nsString& token = mCurrentSearchTokens[i];

            
            PRBool matchTags = (*tokenMatchesTarget)(token, entryTags);
            
            PRBool matchTitle = (*tokenMatchesTarget)(token, title);

            
            matchAll = matchTags || matchTitle;
            if (GET_BEHAVIOR(Title) && !matchAll)
              break;

            
            PRBool matchUrl = (*tokenMatchesTarget)(token, entryURL);
            
            
            if (GET_BEHAVIOR(Url) && !matchUrl)
              matchAll = PR_FALSE;
            else
              matchAll |= matchUrl;
          }

          
          if (!matchAll)
            continue;

          break;
        }
      }

      
      PRBool showTags = !entryTags.IsEmpty();

      
      if (showTags)
        title += TITLE_TAGS_SEPARATOR + entryTags;

      
      
      if (style.IsEmpty()) {
        if (showTags)
          style = NS_LITERAL_STRING("tag");
        else if ((parentId && !mLivemarkFeedItemIds.Get(parentId, &dummy)) ||
                 mLivemarkFeedURIs.Get(escapedEntryURL, &dummy))
          style = NS_LITERAL_STRING("bookmark");
        else
          style = NS_LITERAL_STRING("favicon");
      }

      
      nsCAutoString faviconSpec;
      faviconService->GetFaviconSpecForIconString(
        NS_ConvertUTF16toUTF8(entryFavicon), faviconSpec);
      NS_ConvertUTF8toUTF16 faviconURI(faviconSpec);

      
      rv = mCurrentResult->AppendMatch(escapedEntryURL, title, faviconURI, style);
      NS_ENSURE_SUCCESS(rv, rv);
      mCurrentResultURLs.Put(escapedEntryURL, PR_TRUE);

      
      if (AutoCompleteHasEnoughResults())
        break;
    }
  }

  return NS_OK;
}

inline PRBool
nsNavHistory::AutoCompleteHasEnoughResults()
{
  return mCurrentResultURLs.Count() >= (PRUint32)mAutoCompleteMaxResults;
}



NS_IMETHODIMP
nsNavHistory::OnValueRemoved(nsIAutoCompleteSimpleResult* aResult,
                             const nsAString& aValue, PRBool aRemoveFromDb)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

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
nsNavHistory::AutoCompleteFeedback(PRInt32 aIndex,
                                   nsIAutoCompleteController *aController)
{
  
  if (InPrivateBrowsingMode())
    return NS_OK;

  mozIStorageStatement *stmt = GetDBFeedbackIncrease();
  mozStorageStatementScoper scope(stmt);

  nsAutoString input;
  nsresult rv = aController->GetSearchString(input);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindStringParameter(0, input);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString url;
  rv = aController->GetValueAt(aIndex, url);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindStringParameter(1, url);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<AutoCompleteStatementCallbackNotifier> callback =
    new AutoCompleteStatementCallbackNotifier();
  nsCOMPtr<mozIStoragePendingStatement> canceler;
  rv = stmt->ExecuteAsync(callback, getter_AddRefs(canceler));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsString
nsNavHistory::FixupURIText(const nsAString &aURIText)
{
  
  NS_ConvertUTF16toUTF8 escaped(aURIText);

  
  
  if (StringBeginsWith(escaped, NS_LITERAL_CSTRING("https://")))
    escaped.Cut(0, 8);
  else if (StringBeginsWith(escaped, NS_LITERAL_CSTRING("http://")))
    escaped.Cut(0, 7);
  else if (StringBeginsWith(escaped, NS_LITERAL_CSTRING("ftp://")))
    escaped.Cut(0, 6);

  nsString fixedUp;
  
  if (mTextURIService) {
    mTextURIService->UnEscapeURIForUI(NS_LITERAL_CSTRING("UTF-8"),
      escaped, fixedUp);
    return fixedUp;
  }

  
  NS_UnescapeURL(escaped);
  CopyUTF8toUTF16(escaped, fixedUp);
  return fixedUp;
}
