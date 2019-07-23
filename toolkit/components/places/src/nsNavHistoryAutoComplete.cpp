




























































#include "nsNavHistory.h"
#include "nsNetUtil.h"
#include "nsEscape.h"

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
  
  
#define SQL_STR_FRAGMENT_GET_BOOK_TAG(column, comparison, getMostRecent) \
  NS_LITERAL_CSTRING( \
  "(SELECT " column " " \
  "FROM moz_bookmarks b " \
  "JOIN moz_bookmarks t ON t.id = b.parent AND t.parent " comparison " ?1 " \
  "WHERE b.type = ") + nsPrintfCString("%d", \
    nsINavBookmarksService::TYPE_BOOKMARK) + \
    NS_LITERAL_CSTRING(" AND b.fk = h.id") + \
  (getMostRecent ? NS_LITERAL_CSTRING(" " \
    "ORDER BY b.lastModified DESC LIMIT 1), ") : NS_LITERAL_CSTRING("), "))

  
  const nsCString &bookTag = 
    SQL_STR_FRAGMENT_GET_BOOK_TAG("b.parent", "!=", PR_TRUE) +
    SQL_STR_FRAGMENT_GET_BOOK_TAG("b.title", "!=", PR_TRUE) +
    SQL_STR_FRAGMENT_GET_BOOK_TAG("GROUP_CONCAT(t.title, ' ')", "=", PR_FALSE);

  nsCString sql = NS_LITERAL_CSTRING(
    "SELECT h.url, h.title, f.url, ") + bookTag +
      NS_LITERAL_CSTRING("NULL "
    "FROM moz_places h "
    "LEFT OUTER JOIN moz_favicons f ON f.id = h.favicon_id "
    "WHERE h.frecency <> 0 ");

  if (mAutoCompleteOnlyTyped)
    sql += NS_LITERAL_CSTRING("AND h.typed = 1 ");

  
  
  
  
  
  
  
  sql += NS_LITERAL_CSTRING(
    "ORDER BY h.frecency DESC LIMIT ?2 OFFSET ?3");

  nsresult rv = mDBConn->CreateStatement(sql, 
    getter_AddRefs(mDBAutoCompleteQuery));
  NS_ENSURE_SUCCESS(rv, rv);

  sql = NS_LITERAL_CSTRING(
    "SELECT h.url, h.title, f.url, ") + bookTag + NS_LITERAL_CSTRING(
      "ROUND(MAX(((i.input = ?2) + (SUBSTR(i.input, 1, LENGTH(?2)) = ?2)) * "
                "i.use_count), 1) rank "
    "FROM moz_inputhistory i "
    "JOIN moz_places h ON h.id = i.place_id "
    "LEFT OUTER JOIN moz_favicons f ON f.id = h.favicon_id "
    "GROUP BY i.place_id HAVING rank > 0 "
    "ORDER BY rank DESC, h.frecency DESC");
  rv = mDBConn->CreateStatement(sql, getter_AddRefs(mDBAdaptiveQuery));
  NS_ENSURE_SUCCESS(rv, rv);

  sql = NS_LITERAL_CSTRING(
    
    "INSERT OR REPLACE INTO moz_inputhistory "
    
    "SELECT h.id, IFNULL(i.input, ?1), IFNULL(i.use_count, 0) * .9 + 1 "
    "FROM moz_places h "
    "LEFT OUTER JOIN moz_inputhistory i ON i.place_id = h.id AND i.input = ?1 "
    "WHERE h.url = ?2");
  rv = mDBConn->CreateStatement(sql, getter_AddRefs(mDBFeedbackIncrease));
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

  mCurrentResult->SetSearchString(mCurrentSearchString);

  nsresult rv;
  
  if (!mCurrentChunkOffset) {
    
    rv = AutoCompleteAdaptiveSearch();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRBool moreChunksToSearch = PR_FALSE;
  rv = AutoCompleteFullHistorySearch(&moreChunksToSearch);
  NS_ENSURE_SUCCESS(rv, rv);

  
  moreChunksToSearch &= !AutoCompleteHasEnoughResults();
 
  
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

  
  ToLowerCase(aSearchString, mCurrentSearchString);
  
  mCurrentSearchString.Trim(" \r\n\t\b");

  mCurrentListener = aListener;

  nsresult rv;
  mCurrentResult = do_CreateInstance(NS_AUTOCOMPLETESIMPLERESULT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mCurrentChunkOffset = 0;
  mCurrentResultURLs.Clear();
  mCurrentSearchTokens.Clear();
  mLivemarkFeedItemIds.Clear();
  mLivemarkFeedURIs.Clear();

  
  GenerateSearchTokens();

  
  
  
  
  
  
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
    mCurrentSearchTokens.AppendString(aToken);
}

nsresult
nsNavHistory::AutoCompleteAdaptiveSearch()
{
  mozStorageStatementScoper scope(mDBAdaptiveQuery);

  nsresult rv = mDBAdaptiveQuery->BindInt32Parameter(0, GetTagsFolder());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBAdaptiveQuery->BindStringParameter(1, mCurrentSearchString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AutoCompleteProcessSearch(mDBAdaptiveQuery, QUERY_ADAPTIVE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}










nsresult
nsNavHistory::AutoCompleteFullHistorySearch(PRBool* aHasMoreResults)
{
  mozStorageStatementScoper scope(mDBAutoCompleteQuery);

  nsresult rv = mDBAutoCompleteQuery->BindInt32Parameter(0, GetTagsFolder());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBAutoCompleteQuery->BindInt32Parameter(1, mAutoCompleteSearchChunkSize);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBAutoCompleteQuery->BindInt32Parameter(2, mCurrentChunkOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AutoCompleteProcessSearch(mDBAutoCompleteQuery, QUERY_FULL, aHasMoreResults);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsNavHistory::AutoCompleteProcessSearch(mozIStorageStatement* aQuery,
                                        const QueryType aType,
                                        PRBool *aHasMoreResults)
{
  
  if (AutoCompleteHasEnoughResults())
    return NS_OK;

  nsFaviconService* faviconService = nsFaviconService::GetFaviconService();
  NS_ENSURE_TRUE(faviconService, NS_ERROR_OUT_OF_MEMORY);

  
  const nsString &javascriptColon = NS_LITERAL_STRING("javascript:");
  PRBool filterJavascript = mAutoCompleteFilterJavascript &&
    mCurrentSearchString.Find(javascriptColon) != 0;

  PRBool hasMore = PR_FALSE;
  
  while (NS_SUCCEEDED(aQuery->ExecuteStep(&hasMore)) && hasMore) {
    nsAutoString escapedEntryURL;
    nsresult rv = aQuery->GetString(kAutoCompleteIndex_URL, escapedEntryURL);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (filterJavascript && escapedEntryURL.Find(javascriptColon) == 0)
      continue;

    
    
    
    
    
    
    PRBool dummy;
    if (!mCurrentResultURLs.Get(escapedEntryURL, &dummy)) {
      
      NS_LossyConvertUTF16toASCII cEntryURL(escapedEntryURL);
      NS_UnescapeURL(cEntryURL);
      NS_ConvertUTF8toUTF16 entryURL(cEntryURL);

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

      PRBool useBookmark = PR_FALSE;
      PRBool showTags = PR_FALSE;
      nsString style;
      switch (aType) {
        case QUERY_FULL: {
          
          if (aHasMoreResults)
            *aHasMoreResults = PR_TRUE;

          
          
          PRBool matchAll = PR_TRUE;
          for (PRInt32 i = 0; i < mCurrentSearchTokens.Count() && matchAll; i++) {
            const nsString *token = mCurrentSearchTokens.StringAt(i);

            
            PRBool bookmarkMatch = parentId &&
              CaseInsensitiveFindInReadable(*token, entryBookmarkTitle);
            
            
            useBookmark |= bookmarkMatch;

            
            PRBool tagsMatch = CaseInsensitiveFindInReadable(*token, entryTags);
            showTags |= tagsMatch;

            
            matchAll = bookmarkMatch || tagsMatch ||
              CaseInsensitiveFindInReadable(*token, entryTitle) ||
              CaseInsensitiveFindInReadable(*token, entryURL);
          }

          
          if (!matchAll)
            continue;

          break;
        }
        default: {
          
          
          if (!entryTags.IsEmpty())
            showTags = PR_TRUE;
          else
            useBookmark = !entryBookmarkTitle.IsEmpty();

          break;
        }
      }

      
      if (showTags) {
        
        useBookmark = !entryBookmarkTitle.IsEmpty();
        



      }

      
      
      style = showTags ? NS_LITERAL_STRING("tag") : (parentId &&
        !mLivemarkFeedItemIds.Get(parentId, &dummy)) ||
        mLivemarkFeedURIs.Get(escapedEntryURL, &dummy) ?
        NS_LITERAL_STRING("bookmark") : NS_LITERAL_STRING("favicon");

      
      const nsAString &title = useBookmark ? entryBookmarkTitle : entryTitle;

      
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
  mozStorageStatementScoper scope(mDBFeedbackIncrease);

  nsAutoString input;
  nsresult rv = aController->GetSearchString(input);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBFeedbackIncrease->BindStringParameter(0, input);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString url;
  rv = aController->GetValueAt(aIndex, url);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBFeedbackIncrease->BindStringParameter(1, url);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBFeedbackIncrease->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
