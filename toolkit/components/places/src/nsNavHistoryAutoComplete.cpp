



























































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
  nsCString sql = NS_LITERAL_CSTRING(
    "SELECT h.url, h.title, f.url, b.id, b.parent, b.title "
    "FROM moz_places h "
    "LEFT OUTER JOIN moz_bookmarks b ON b.fk = h.id "
    "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
    "WHERE h.frecency <> 0 ");

  if (mAutoCompleteOnlyTyped)
    sql += NS_LITERAL_CSTRING("AND h.typed = 1 ");

  
  
  
  
  
  
  
  sql += NS_LITERAL_CSTRING(
    "ORDER BY h.frecency DESC LIMIT ?1 OFFSET ?2");

  nsresult rv = mDBConn->CreateStatement(sql, 
    getter_AddRefs(mDBAutoCompleteQuery));
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
    
    if (!mCurrentSearchString.IsEmpty()) {
      rv = AutoCompleteTagsSearch();
      NS_ENSURE_SUCCESS(rv, rv);
    }
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
nsNavHistory::AutoCompleteTagsSearch()
{
  
  
  
  
  
  
  
  nsCString sql = NS_LITERAL_CSTRING(
    "SELECT h.url, h.title, f.url, b.id, b.parent, b.title "
    "FROM moz_places h "
    "JOIN moz_bookmarks b ON b.fk = h.id "
    "LEFT OUTER JOIN moz_favicons f ON f.id = h.favicon_id "
    "WHERE h.frecency <> 0 AND (b.parent IN "
      "(SELECT t.id FROM moz_bookmarks t WHERE t.parent = ?1 AND (");

  nsStringArray terms;
  CreateTermsFromTokens(mCurrentSearchTokens, terms);

  for (PRInt32 i = 0; i < terms.Count(); i++) {
    if (i)
      sql += NS_LITERAL_CSTRING(" OR");

    
    sql += NS_LITERAL_CSTRING(" LOWER(t.title) = ") +
           nsPrintfCString("LOWER(?%d)", i + 2);
  }

  sql += NS_LITERAL_CSTRING("))) "
    "ORDER BY h.frecency DESC");

  nsCOMPtr<mozIStorageStatement> tagAutoCompleteQuery;
  nsresult rv = mDBConn->CreateStatement(sql,
    getter_AddRefs(tagAutoCompleteQuery));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = tagAutoCompleteQuery->BindInt64Parameter(0, GetTagsFolder());
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRInt32 i = 0; i < terms.Count(); i++) {
    
    rv = tagAutoCompleteQuery->BindStringParameter(i + 1, *(terms.StringAt(i)));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = AutoCompleteProcessSearch(tagAutoCompleteQuery, QUERY_TAGS);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}










nsresult
nsNavHistory::AutoCompleteFullHistorySearch(PRBool* aHasMoreResults)
{
  mozStorageStatementScoper scope(mDBAutoCompleteQuery);

  nsresult rv = mDBAutoCompleteQuery->BindInt32Parameter(0, mAutoCompleteSearchChunkSize);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBAutoCompleteQuery->BindInt32Parameter(1, mCurrentChunkOffset);
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

      nsAutoString entryTitle, entryFavicon, entryBookmarkTitle;
      rv = aQuery->GetString(kAutoCompleteIndex_Title, entryTitle);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aQuery->GetString(kAutoCompleteIndex_FaviconURL, entryFavicon);
      NS_ENSURE_SUCCESS(rv, rv);
      PRInt64 itemId = 0;
      rv = aQuery->GetInt64(kAutoCompleteIndex_ItemId, &itemId);
      NS_ENSURE_SUCCESS(rv, rv);

      PRInt64 parentId = 0;
      
      if (itemId) {
        rv = aQuery->GetInt64(kAutoCompleteIndex_ParentId, &parentId);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = aQuery->GetString(kAutoCompleteIndex_BookmarkTitle, entryBookmarkTitle);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      PRBool useBookmark = PR_FALSE;
      nsString style;
      switch (aType) {
        case QUERY_TAGS: {
          
          useBookmark = !entryBookmarkTitle.IsEmpty();

          
          style = NS_LITERAL_STRING("tag");

          break;
        }
        case QUERY_FULL: {
          
          if (aHasMoreResults)
            *aHasMoreResults = PR_TRUE;

          
          PRBool matchAll = PR_TRUE;
          for (PRInt32 i = 0; i < mCurrentSearchTokens.Count() && matchAll; i++) {
            const nsString *token = mCurrentSearchTokens.StringAt(i);

            
            PRBool bookmarkMatch = itemId &&
              CaseInsensitiveFindInReadable(*token, entryBookmarkTitle);
            
            
            useBookmark |= bookmarkMatch;

            
            matchAll = bookmarkMatch ||
              CaseInsensitiveFindInReadable(*token, entryTitle) ||
              CaseInsensitiveFindInReadable(*token, entryURL);
          }

          
          if (!matchAll)
            continue;

          
          style = (itemId && !mLivemarkFeedItemIds.Get(parentId, &dummy)) ||
            mLivemarkFeedURIs.Get(escapedEntryURL, &dummy) ?
            NS_LITERAL_STRING("bookmark") : NS_LITERAL_STRING("favicon");

          break;
        }
        default: {
          
          useBookmark = !entryBookmarkTitle.IsEmpty();

          
          style = (itemId && !mLivemarkFeedItemIds.Get(parentId, &dummy)) ||
            mLivemarkFeedURIs.Get(escapedEntryURL, &dummy) ?
            NS_LITERAL_STRING("bookmark") : NS_LITERAL_STRING("favicon");

          break;
        }
      }

      
      const nsAString &title = useBookmark ? entryBookmarkTitle : entryTitle;

      
      nsCAutoString faviconSpec;
      faviconService->GetFaviconSpecForIconString(
        NS_ConvertUTF16toUTF8(entryFavicon), faviconSpec);
      NS_ConvertUTF8toUTF16 faviconURI(faviconSpec);

      
      rv = mCurrentResult->AppendMatch(entryURL, title, faviconURI, style);
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
