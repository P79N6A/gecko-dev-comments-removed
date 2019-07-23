


















































#include "nsNavHistory.h"
#include "nsNetUtil.h"

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

#define NS_AUTOCOMPLETESIMPLERESULT_CONTRACTID \
  "@mozilla.org/autocomplete/simple-result;1"



#define AUTOCOMPLETE_MAX_PER_TYPED 100


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







nsresult
nsNavHistory::CreateAutoCompleteQueries()
{
  nsCString sql = NS_LITERAL_CSTRING(
    "SELECT h.url, h.title, f.url, b.id, b.parent "
    "FROM moz_places h "
    "JOIN moz_historyvisits v ON h.id = v.place_id "
    "LEFT OUTER JOIN moz_bookmarks b ON b.fk = h.id "
    "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
    "WHERE v.visit_date >= ?1 AND v.visit_date <= ?2 AND h.hidden <> 1 AND "
    " v.visit_type NOT IN(0,4) AND ");

  if (mAutoCompleteOnlyTyped)
    sql += NS_LITERAL_CSTRING("h.typed = 1 AND ");

  sql += NS_LITERAL_CSTRING(
    "(h.title LIKE ?3 ESCAPE '/' OR h.url LIKE ?3 ESCAPE '/') "
    "GROUP BY h.id ORDER BY h.typed DESC, h.visit_count DESC, MAX(v.visit_date) DESC;");

  nsresult rv = mDBConn->CreateStatement(sql, getter_AddRefs(mDBAutoCompleteQuery));
  NS_ENSURE_SUCCESS(rv, rv);

  sql = NS_LITERAL_CSTRING(
    "SELECT h.url, h.title, f.url, b.id, b.parent "
    "FROM moz_places h "
    "JOIN moz_bookmarks b ON b.fk = h.id "
    "LEFT OUTER JOIN moz_historyvisits v ON h.id = v.place_id "
    "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
    "WHERE "
    "(b.parent in (SELECT t.id FROM moz_bookmarks t WHERE t.parent = ?1 AND LOWER(t.title) = LOWER(?2))) "
    "GROUP BY h.id ORDER BY h.visit_count DESC, MAX(v.visit_date) DESC;");
  rv = mDBConn->CreateStatement(sql, getter_AddRefs(mDBTagAutoCompleteQuery));
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

static const PRInt64 USECS_PER_DAY = LL_INIT(20, 500654080);








#define AUTOCOMPLETE_SEARCH_CHUNK (USECS_PER_DAY * 4)





#define AUTOCOMPLETE_SEARCH_TIMEOUT 100

#define LMANNO_FEEDURI "livemark/feedURI"



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

  mCurrentResult->SetSearchString(mCurrentSearchString);
  PRBool moreChunksToSearch = PR_FALSE;

  nsresult rv;
  
  if (mCurrentSearchString.IsEmpty())
    rv = AutoCompleteTypedSearch();
  else {
    
    
    
    if (mFirstChunk) {
      rv = AutoCompleteTagsSearch();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = AutoCompleteFullHistorySearch();
    moreChunksToSearch = (mCurrentChunkEndTime >= mCurrentOldestVisit);
  }
  NS_ENSURE_SUCCESS(rv, rv);
 
  
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
    mFirstChunk = PR_FALSE;
    mCurrentChunkEndTime -= AUTOCOMPLETE_SEARCH_CHUNK;
    rv = StartAutoCompleteTimer(AUTOCOMPLETE_SEARCH_TIMEOUT);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    DoneSearching();
  }
  return NS_OK;
}

void
nsNavHistory::DoneSearching()
{
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
  mCurrentSearchString = aSearchString;
  
  mCurrentSearchString.Trim(" \r\n\t\b");
  mCurrentListener = aListener;
  nsresult rv;

  
  
  
  
  PRBool searchPrevious = PR_FALSE;
  if (aPreviousResult) {
    nsAutoString prevSearchString;
    aPreviousResult->GetSearchString(prevSearchString);

    
    
    
    
    searchPrevious = !prevSearchString.IsEmpty() && Substring(mCurrentSearchString, 0,
                      prevSearchString.Length()).Equals(prevSearchString);
  }

  mCurrentResult = do_CreateInstance(NS_AUTOCOMPLETESIMPLERESULT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mCurrentResultURLs.Clear();

  
  
  if (!searchPrevious) {
    mLivemarkFeedItemIds.Clear();
    mLivemarkFeedURIs.Clear();

    
    
    
    
    
    
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
  }

  
  if (searchPrevious) {
    
    
    
    
    
    
    rv = AutoCompleteTagsSearch();
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 matchCount;
    aPreviousResult->GetMatchCount(&matchCount);
    for (PRUint32 i = 0; i < matchCount; i++) {
      
      
      
      
      
      
      nsAutoString style;
      aPreviousResult->GetStyleAt(i, style);
      if (!style.Equals(NS_LITERAL_STRING("tag"))) {
        nsAutoString url;
        aPreviousResult->GetValueAt(i, url);

        
        PRBool dummy;
        if (!mCurrentResultURLs.Get(url, &dummy)) {
          nsAutoString title;
          aPreviousResult->GetCommentAt(i, title);

          
          
          
          PRBool isMatch = CaseInsensitiveFindInReadable(mCurrentSearchString, title);
          if (!isMatch)
            isMatch = CaseInsensitiveFindInReadable(mCurrentSearchString, url);

          if (isMatch) {
            nsAutoString image;
            aPreviousResult->GetImageAt(i, image);

            mCurrentResultURLs.Put(url, PR_TRUE);
  
            rv = mCurrentResult->AppendMatch(url, title, image, style);
            NS_ENSURE_SUCCESS(rv, rv);
          }
        }
      }
    }
    
    
    PRUint32 count;
    mCurrentResult->GetMatchCount(&count); 

    if (count > 0) {
      
      
      
      mCurrentResult->SetSearchResult(nsIAutoCompleteResult::RESULT_SUCCESS_ONGOING);
      mCurrentResult->SetDefaultIndex(0);
      rv = mCurrentResult->SetListener(this);
      NS_ENSURE_SUCCESS(rv, rv);
      mCurrentListener->OnSearchResult(this, mCurrentResult);
    }
  }
  else if (!mCurrentSearchString.IsEmpty()) {
    
    mCurrentChunkEndTime = PR_Now();
    mCurrentOldestVisit = 0;
    mFirstChunk = PR_TRUE;

    
    nsCOMPtr<mozIStorageStatement> dbSelectStatement;
    rv = mDBConn->CreateStatement(
      NS_LITERAL_CSTRING("SELECT MIN(visit_date) id FROM moz_historyvisits WHERE visit_type NOT IN(0,4)"),
      getter_AddRefs(dbSelectStatement));
    NS_ENSURE_SUCCESS(rv, rv);
    PRBool hasMinVisit;
    rv = dbSelectStatement->ExecuteStep(&hasMinVisit);
    NS_ENSURE_SUCCESS(rv, rv);
  
    if (hasMinVisit) {
      rv = dbSelectStatement->GetInt64(0, &mCurrentOldestVisit);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (!mCurrentOldestVisit) {
      
      mCurrentOldestVisit = PR_Now() - USECS_PER_DAY;
    }
  }

  
  rv = StartAutoCompleteTimer(0);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}



NS_IMETHODIMP
nsNavHistory::StopSearch()
{
  if (mAutoCompleteTimer)
    mAutoCompleteTimer->Cancel();

  mCurrentSearchString.Truncate();
  DoneSearching();

  return NS_OK;
}










nsresult nsNavHistory::AutoCompleteTypedSearch()
{
  nsCOMPtr<mozIStorageStatement> dbSelectStatement;

  nsCString sql = NS_LITERAL_CSTRING(
    "SELECT h.url, h.title, f.url, b.id, b.parent "
    "FROM moz_places h "
    "LEFT OUTER JOIN moz_bookmarks b ON b.fk = h.id "
    "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
    "JOIN moz_historyvisits v ON h.id = v.place_id WHERE (h.id IN "
    "(SELECT DISTINCT h.id from moz_historyvisits v, moz_places h WHERE "
    "v.place_id = h.id AND h.typed = 1 AND v.visit_type NOT IN(0,4) "
    "ORDER BY v.visit_date DESC LIMIT ");
  sql.AppendInt(AUTOCOMPLETE_MAX_PER_TYPED);
  sql += NS_LITERAL_CSTRING(")) GROUP BY h.id ORDER BY MAX(v.visit_date) DESC");  
  
  nsFaviconService* faviconService = nsFaviconService::GetFaviconService();
  NS_ENSURE_TRUE(faviconService, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = mDBConn->CreateStatement(sql, getter_AddRefs(dbSelectStatement));
  NS_ENSURE_SUCCESS(rv, rv);
 
  PRBool hasMore = PR_FALSE;
  while (NS_SUCCEEDED(dbSelectStatement->ExecuteStep(&hasMore)) && hasMore) {
    nsAutoString entryURL, entryTitle, entryFavicon;
    dbSelectStatement->GetString(kAutoCompleteIndex_URL, entryURL);
    dbSelectStatement->GetString(kAutoCompleteIndex_Title, entryTitle);
    dbSelectStatement->GetString(kAutoCompleteIndex_FaviconURL, entryFavicon);
    PRInt64 itemId = 0;
    dbSelectStatement->GetInt64(kAutoCompleteIndex_ItemId, &itemId);
    PRInt64 parentId = 0;
    dbSelectStatement->GetInt64(kAutoCompleteIndex_ParentId, &parentId);

    PRBool dummy;
    
    
    PRBool isBookmark = (itemId != 0 && 
                         !mLivemarkFeedItemIds.Get(parentId, &dummy)) ||
                        mLivemarkFeedURIs.Get(entryURL, &dummy);   

    nsCAutoString imageSpec;
    faviconService->GetFaviconSpecForIconString(
      NS_ConvertUTF16toUTF8(entryFavicon), imageSpec);
    rv = mCurrentResult->AppendMatch(entryURL, entryTitle, 
      NS_ConvertUTF8toUTF16(imageSpec), isBookmark ? NS_LITERAL_STRING("bookmark") : NS_LITERAL_STRING("favicon"));
    NS_ENSURE_SUCCESS(rv, rv);
  } 
  return NS_OK;
}

nsresult
nsNavHistory::AutoCompleteTagsSearch()
{
  nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);
  nsresult rv;

  PRInt64 tagsFolder = GetTagsFolder();

  nsString::const_iterator strStart, strEnd;
  mCurrentSearchString.BeginReading(strStart);
  mCurrentSearchString.EndReading(strEnd);
  nsString::const_iterator start = strStart, end = strEnd;

  nsStringArray tagTokens;

  
  while (FindInReadable(NS_LITERAL_STRING(" "), start, end,
                        nsDefaultStringComparator())) {
    nsAutoString currentMatch(Substring(strStart, start));
    currentMatch.Trim("\r\n\t\b");
    if (!currentMatch.IsEmpty())
      tagTokens.AppendString(currentMatch);
    strStart = start = end;
    end = strEnd;
  }

  nsCOMPtr<mozIStorageStatement> tagAutoCompleteQuery;

  
  
  
  if (!tagTokens.Count()) {
    tagAutoCompleteQuery = mDBTagAutoCompleteQuery;

    rv = tagAutoCompleteQuery->BindInt64Parameter(0, tagsFolder);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = tagAutoCompleteQuery->BindStringParameter(1, mCurrentSearchString);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    
    nsAutoString lastMatch(Substring(strStart, strEnd));
    lastMatch.Trim("\r\n\t\b");
    if (!lastMatch.IsEmpty())
      tagTokens.AppendString(lastMatch);

    nsCString tagQuery = NS_LITERAL_CSTRING(
      "SELECT h.url, h.title, f.url, b.id, b.parent "
      "FROM moz_places h "
      "JOIN moz_bookmarks b ON b.fk = h.id "
      "LEFT OUTER JOIN moz_historyvisits v ON h.id = v.place_id "
      "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE "
      "(b.parent in "
      " (SELECT t.id FROM moz_bookmarks t WHERE t.parent = ?1 AND (");

    nsStringArray terms;
    CreateTermsFromTokens(tagTokens, terms);

    for (PRUint32 i=0; i<terms.Count(); i++) {
      if (i)
        tagQuery += NS_LITERAL_CSTRING(" OR");

      
      tagQuery += NS_LITERAL_CSTRING(" LOWER(t.title) = ") +
                  nsPrintfCString("LOWER(?%d)", i+2);
    }

    tagQuery += NS_LITERAL_CSTRING("))) "
      "GROUP BY h.id ORDER BY h.visit_count DESC, MAX(v.visit_date) DESC;");

    rv = mDBConn->CreateStatement(tagQuery, getter_AddRefs(tagAutoCompleteQuery));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = tagAutoCompleteQuery->BindInt64Parameter(0, tagsFolder);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 i=0; i<terms.Count(); i++) {
      
      rv = tagAutoCompleteQuery->BindStringParameter(i+1, *(terms.StringAt(i)));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  nsFaviconService* faviconService = nsFaviconService::GetFaviconService();
  NS_ENSURE_TRUE(faviconService, NS_ERROR_OUT_OF_MEMORY);

  mozStorageStatementScoper scope(tagAutoCompleteQuery);

  PRBool hasMore = PR_FALSE;

  
  while (NS_SUCCEEDED(tagAutoCompleteQuery->ExecuteStep(&hasMore)) && hasMore) {
    nsAutoString entryURL, entryTitle, entryFavicon;
    rv = tagAutoCompleteQuery->GetString(kAutoCompleteIndex_URL, entryURL);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = tagAutoCompleteQuery->GetString(kAutoCompleteIndex_Title, entryTitle);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = tagAutoCompleteQuery->GetString(kAutoCompleteIndex_FaviconURL, entryFavicon);
    NS_ENSURE_SUCCESS(rv, rv);
    PRInt64 itemId = 0;
    rv = tagAutoCompleteQuery->GetInt64(kAutoCompleteIndex_ItemId, &itemId);
    NS_ENSURE_SUCCESS(rv, rv);
    PRInt64 parentId = 0;
    rv = tagAutoCompleteQuery->GetInt64(kAutoCompleteIndex_ParentId, &parentId);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool dummy;
    
    
    if (!mCurrentResultURLs.Get(entryURL, &dummy)) {
      
      nsCAutoString faviconSpec;
      faviconService->GetFaviconSpecForIconString(
        NS_ConvertUTF16toUTF8(entryFavicon), faviconSpec);
      rv = mCurrentResult->AppendMatch(entryURL, entryTitle, 
        NS_ConvertUTF8toUTF16(faviconSpec), NS_LITERAL_STRING("tag"));
      NS_ENSURE_SUCCESS(rv, rv);

      mCurrentResultURLs.Put(entryURL, PR_TRUE);
    }
  }

  return NS_OK;
}








nsresult
nsNavHistory::AutoCompleteFullHistorySearch()
{
  mozStorageStatementScoper scope(mDBAutoCompleteQuery);

  nsresult rv = mDBAutoCompleteQuery->BindInt64Parameter(0, mCurrentChunkEndTime - AUTOCOMPLETE_SEARCH_CHUNK);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBAutoCompleteQuery->BindInt64Parameter(1, mCurrentChunkEndTime);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString escapedSearchString;
  rv = mDBAutoCompleteQuery->EscapeStringForLIKE(mCurrentSearchString, PRUnichar('/'), escapedSearchString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBAutoCompleteQuery->BindStringParameter(2, NS_LITERAL_STRING("%") + escapedSearchString + NS_LITERAL_STRING("%"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsFaviconService* faviconService = nsFaviconService::GetFaviconService();
  NS_ENSURE_TRUE(faviconService, NS_ERROR_OUT_OF_MEMORY);

  PRBool hasMore = PR_FALSE;

  
  while (NS_SUCCEEDED(mDBAutoCompleteQuery->ExecuteStep(&hasMore)) && hasMore) {
    nsAutoString entryURL, entryTitle, entryFavicon;
    rv = mDBAutoCompleteQuery->GetString(kAutoCompleteIndex_URL, entryURL);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBAutoCompleteQuery->GetString(kAutoCompleteIndex_Title, entryTitle);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBAutoCompleteQuery->GetString(kAutoCompleteIndex_FaviconURL, entryFavicon);
    NS_ENSURE_SUCCESS(rv, rv);
    PRInt64 itemId = 0;
    rv = mDBAutoCompleteQuery->GetInt64(kAutoCompleteIndex_ItemId, &itemId);
    NS_ENSURE_SUCCESS(rv, rv);
    PRInt64 parentId = 0;
    rv = mDBAutoCompleteQuery->GetInt64(kAutoCompleteIndex_ParentId, &parentId);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool dummy;
    
    
    PRBool isBookmark = (itemId != 0 && 
                         !mLivemarkFeedItemIds.Get(parentId, &dummy)) ||
                         mLivemarkFeedURIs.Get(entryURL, &dummy);  

    
    
    if (!mCurrentResultURLs.Get(entryURL, &dummy)) {
      
      nsCAutoString faviconSpec;
      faviconService->GetFaviconSpecForIconString(
        NS_ConvertUTF16toUTF8(entryFavicon), faviconSpec);
      rv = mCurrentResult->AppendMatch(entryURL, entryTitle, 
        NS_ConvertUTF8toUTF16(faviconSpec), isBookmark ? NS_LITERAL_STRING("bookmark") : NS_LITERAL_STRING("favicon"));
      NS_ENSURE_SUCCESS(rv, rv);

      mCurrentResultURLs.Put(entryURL, PR_TRUE);
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
