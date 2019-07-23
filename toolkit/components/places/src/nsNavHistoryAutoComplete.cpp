



























































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
#include "nsILivemarkService.h"

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
    "SELECT h.url, h.title, f.url, b.id, b.parent, b.title "
    "FROM moz_places h "
    "LEFT OUTER JOIN moz_bookmarks b ON b.fk = h.id "
    "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
    "WHERE h.frecency <> 0 AND ");

  if (mAutoCompleteOnlyTyped)
    sql += NS_LITERAL_CSTRING("h.typed = 1 AND ");

  
  
  
  
  
  
  
  sql += NS_LITERAL_CSTRING(
    "(b.title LIKE ?1 ESCAPE '/' OR " 
     "h.title LIKE ?1 ESCAPE '/' OR "
     "h.url LIKE ?1 ESCAPE '/') "
    "ORDER BY h.frecency DESC LIMIT ?2 OFFSET ?3");

  nsresult rv = mDBConn->CreateStatement(sql, 
    getter_AddRefs(mDBAutoCompleteQuery));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  
  sql = NS_LITERAL_CSTRING(
    "SELECT h.url, h.title, f.url, b.id, b.parent "
    "FROM moz_places h "
    "JOIN moz_bookmarks b ON b.fk = h.id "
    "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
    "WHERE h.frecency <> 0 AND "
    "(b.parent in (SELECT t.id FROM moz_bookmarks t WHERE t.parent = ?1 AND LOWER(t.title) = LOWER(?2))) "
    "ORDER BY h.frecency DESC");

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






#define AUTOCOMPLETE_SEARCH_CHUNK_SIZE 100


#define AUTOCOMPLETE_SEARCH_TIMEOUT 100



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
    
    
    
    if (!mCurrentChunkOffset) {
      rv = AutoCompleteTagsSearch();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = AutoCompleteFullHistorySearch(&moreChunksToSearch);
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
    mCurrentChunkOffset += AUTOCOMPLETE_SEARCH_CHUNK_SIZE;
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

  nsresult rv = mDBAutoCompleteQuery->EscapeStringForLIKE(mCurrentSearchString, PRUnichar('/'), mCurrentSearchStringEscaped);
  NS_ENSURE_SUCCESS(rv, rv);

  mCurrentListener = aListener;

  mCurrentResult = do_CreateInstance(NS_AUTOCOMPLETESIMPLERESULT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mCurrentChunkOffset = 0;
  mCurrentResultURLs.Clear();
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
    "WHERE h.frecency <> 0 AND h.typed = 1 "
    "GROUP BY h.id ORDER BY h.frecency DESC LIMIT ");
  sql.AppendInt(AUTOCOMPLETE_MAX_PER_TYPED);
  
  nsFaviconService* faviconService = nsFaviconService::GetFaviconService();
  NS_ENSURE_TRUE(faviconService, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = mDBConn->CreateStatement(sql, 
    getter_AddRefs(dbSelectStatement));
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
    if (itemId)
      dbSelectStatement->GetInt64(kAutoCompleteIndex_ParentId, &parentId);

    PRBool dummy;
    
    
    PRBool isBookmark = (itemId && 
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
      "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE h.frecency <> 0 AND "
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
      "GROUP BY h.id ORDER BY h.frecency DESC");

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
    nsAutoString entryURL;
    rv = tagAutoCompleteQuery->GetString(kAutoCompleteIndex_URL, entryURL);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool dummy;
    
    
    
    
    
    
    
    
    if (!mCurrentResultURLs.Get(entryURL, &dummy)) {
      nsAutoString entryTitle, entryFavicon;
      rv = tagAutoCompleteQuery->GetString(kAutoCompleteIndex_Title, entryTitle);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = tagAutoCompleteQuery->GetString(kAutoCompleteIndex_FaviconURL, entryFavicon);
      NS_ENSURE_SUCCESS(rv, rv);
      PRInt64 itemId = 0;
      rv = tagAutoCompleteQuery->GetInt64(kAutoCompleteIndex_ItemId, &itemId);
      NS_ENSURE_SUCCESS(rv, rv);
      PRInt64 parentId = 0;
      if (itemId) {
        rv = tagAutoCompleteQuery->GetInt64(kAutoCompleteIndex_ParentId, &parentId);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      
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
nsNavHistory::AutoCompleteFullHistorySearch(PRBool* aHasMoreResults)
{
  mozStorageStatementScoper scope(mDBAutoCompleteQuery);

  
  nsresult rv = mDBAutoCompleteQuery->BindStringParameter(0, NS_LITERAL_STRING("%") + mCurrentSearchStringEscaped + NS_LITERAL_STRING("%"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBAutoCompleteQuery->BindInt32Parameter(1, AUTOCOMPLETE_SEARCH_CHUNK_SIZE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBAutoCompleteQuery->BindInt32Parameter(2, mCurrentChunkOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  nsFaviconService* faviconService = nsFaviconService::GetFaviconService();
  NS_ENSURE_TRUE(faviconService, NS_ERROR_OUT_OF_MEMORY);

  PRBool hasMore = PR_FALSE;

  
  while (NS_SUCCEEDED(mDBAutoCompleteQuery->ExecuteStep(&hasMore)) && hasMore) {
    *aHasMoreResults = PR_TRUE;
    nsAutoString entryURL;
    rv = mDBAutoCompleteQuery->GetString(kAutoCompleteIndex_URL, entryURL);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    PRBool dummy;
    if (!mCurrentResultURLs.Get(entryURL, &dummy)) {
      nsAutoString entryTitle, entryFavicon, entryBookmarkTitle;
      rv = mDBAutoCompleteQuery->GetString(kAutoCompleteIndex_Title, entryTitle);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mDBAutoCompleteQuery->GetString(kAutoCompleteIndex_FaviconURL, entryFavicon);
      NS_ENSURE_SUCCESS(rv, rv);
      PRInt64 itemId = 0;
      rv = mDBAutoCompleteQuery->GetInt64(kAutoCompleteIndex_ItemId, &itemId);
      NS_ENSURE_SUCCESS(rv, rv);

      PRInt64 parentId = 0;
      
      
      if (itemId) {
        rv = mDBAutoCompleteQuery->GetInt64(kAutoCompleteIndex_ParentId, &parentId);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = mDBAutoCompleteQuery->GetString(kAutoCompleteIndex_BookmarkTitle, entryBookmarkTitle);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      
      
      
      
      
      
      
      
      
      PRBool isBookmark = (itemId && 
                           !mLivemarkFeedItemIds.Get(parentId, &dummy)) ||
                           mLivemarkFeedURIs.Get(entryURL, &dummy);  

      
      nsCAutoString faviconSpec;
      faviconService->GetFaviconSpecForIconString(
        NS_ConvertUTF16toUTF8(entryFavicon), faviconSpec);

      
      
      PRBool matchInBookmarkTitle = itemId && 
        CaseInsensitiveFindInReadable(mCurrentSearchString, entryBookmarkTitle);

      rv = mCurrentResult->AppendMatch(entryURL, 
        matchInBookmarkTitle ? entryBookmarkTitle : entryTitle, 
        NS_ConvertUTF8toUTF16(faviconSpec), 
        isBookmark ? NS_LITERAL_STRING("bookmark") : NS_LITERAL_STRING("favicon"));
      NS_ENSURE_SUCCESS(rv, rv);

      mCurrentResultURLs.Put(entryURL, PR_TRUE);

      if (mCurrentResultURLs.Count() >= mAutoCompleteMaxResults) {
        *aHasMoreResults = PR_FALSE;
        break;
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
