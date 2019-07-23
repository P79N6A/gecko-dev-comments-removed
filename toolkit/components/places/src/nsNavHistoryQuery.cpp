













































#include "nsNavHistory.h"
#include "nsNavBookmarks.h"
#include "nsEscape.h"
#include "nsCOMArray.h"
#include "nsNetUtil.h"
#include "nsTArray.h"
#include "prprf.h"

class QueryKeyValuePair
{
public:

  
  
  
  
  
  
  
  
  
  

  QueryKeyValuePair(const nsCSubstring& aSource, PRInt32 aKeyBegin,
                    PRInt32 aEquals, PRInt32 aPastEnd)
  {
    if (aEquals == aKeyBegin)
      aEquals = aPastEnd;
    key = Substring(aSource, aKeyBegin, aEquals - aKeyBegin);
    if (aPastEnd - aEquals > 0)
      value = Substring(aSource, aEquals + 1, aPastEnd - aEquals - 1);
  }
  nsCString key;
  nsCString value;
};

static nsresult TokenizeQueryString(const nsACString& aQuery,
                                    nsTArray<QueryKeyValuePair>* aTokens);
static nsresult ParseQueryBooleanString(const nsCString& aString,
                                        PRBool* aValue);


typedef NS_STDCALL_FUNCPROTO(nsresult, BoolQueryGetter, nsINavHistoryQuery,
                             GetOnlyBookmarked, (PRBool*));
typedef NS_STDCALL_FUNCPROTO(nsresult, Uint32QueryGetter, nsINavHistoryQuery,
                             GetBeginTimeReference, (PRUint32*));
typedef NS_STDCALL_FUNCPROTO(nsresult, Int64QueryGetter, nsINavHistoryQuery,
                             GetBeginTime, (PRInt64*));
static void AppendBoolKeyValueIfTrue(nsACString& aString,
                                     const nsCString& aName,
                                     nsINavHistoryQuery* aQuery,
                                     BoolQueryGetter getter);
static void AppendUint32KeyValueIfNonzero(nsACString& aString,
                                          const nsCString& aName,
                                          nsINavHistoryQuery* aQuery,
                                          Uint32QueryGetter getter);
static void AppendInt64KeyValueIfNonzero(nsACString& aString,
                                         const nsCString& aName,
                                         nsINavHistoryQuery* aQuery,
                                         Int64QueryGetter getter);


typedef NS_STDCALL_FUNCPROTO(nsresult, BoolQuerySetter, nsINavHistoryQuery,
                             SetOnlyBookmarked, (PRBool));
typedef NS_STDCALL_FUNCPROTO(nsresult, Uint32QuerySetter, nsINavHistoryQuery,
                             SetBeginTimeReference, (PRUint32));
typedef NS_STDCALL_FUNCPROTO(nsresult, Int64QuerySetter, nsINavHistoryQuery,
                             SetBeginTime, (PRInt64));
static void SetQueryKeyBool(const nsCString& aValue, nsINavHistoryQuery* aQuery,
                            BoolQuerySetter setter);
static void SetQueryKeyUint32(const nsCString& aValue, nsINavHistoryQuery* aQuery,
                              Uint32QuerySetter setter);
static void SetQueryKeyInt64(const nsCString& aValue, nsINavHistoryQuery* aQuery,
                             Int64QuerySetter setter);


typedef NS_STDCALL_FUNCPROTO(nsresult, BoolOptionsSetter,
                             nsINavHistoryQueryOptions,
                             SetExpandQueries, (PRBool));
typedef NS_STDCALL_FUNCPROTO(nsresult, Uint32OptionsSetter,
                             nsINavHistoryQueryOptions,
                             SetMaxResults, (PRUint32));
typedef NS_STDCALL_FUNCPROTO(nsresult, Uint16OptionsSetter,
                             nsINavHistoryQueryOptions,
                             SetResultType, (PRUint16));
static void SetOptionsKeyBool(const nsCString& aValue,
                              nsINavHistoryQueryOptions* aOptions,
                              BoolOptionsSetter setter);
static void SetOptionsKeyUint16(const nsCString& aValue,
                                nsINavHistoryQueryOptions* aOptions,
                                Uint16OptionsSetter setter);
static void SetOptionsKeyUint32(const nsCString& aValue,
                                nsINavHistoryQueryOptions* aOptions,
                                Uint32OptionsSetter setter);




#define QUERYKEY_BEGIN_TIME "beginTime"
#define QUERYKEY_BEGIN_TIME_REFERENCE "beginTimeRef"
#define QUERYKEY_END_TIME "endTime"
#define QUERYKEY_END_TIME_REFERENCE "endTimeRef"
#define QUERYKEY_SEARCH_TERMS "terms"
#define QUERYKEY_MIN_VISITS "minVisits"
#define QUERYKEY_MAX_VISITS "maxVisits"
#define QUERYKEY_ONLY_BOOKMARKED "onlyBookmarked"
#define QUERYKEY_DOMAIN_IS_HOST "domainIsHost"
#define QUERYKEY_DOMAIN "domain"
#define QUERYKEY_FOLDER "folder"
#define QUERYKEY_NOTANNOTATION "!annotation"
#define QUERYKEY_ANNOTATION "annotation"
#define QUERYKEY_URI "uri"
#define QUERYKEY_URIISPREFIX "uriIsPrefix"
#define QUERYKEY_SEPARATOR "OR"
#define QUERYKEY_GROUP "group"
#define QUERYKEY_SORT "sort"
#define QUERYKEY_SORTING_ANNOTATION "sortingAnnotation"
#define QUERYKEY_RESULT_TYPE "type"
#define QUERYKEY_EXCLUDE_ITEMS "excludeItems"
#define QUERYKEY_EXCLUDE_QUERIES "excludeQueries"
#define QUERYKEY_EXCLUDE_READ_ONLY_FOLDERS "excludeReadOnlyFolders"
#define QUERYKEY_EXCLUDE_ITEM_IF_PARENT_HAS_ANNOTATION "excludeItemIfParentHasAnnotation"
#define QUERYKEY_EXPAND_QUERIES "expandQueries"
#define QUERYKEY_FORCE_ORIGINAL_TITLE "originalTitle"
#define QUERYKEY_INCLUDE_HIDDEN "includeHidden"
#define QUERYKEY_REDIRECTS_MODE "redirectsMode"
#define QUERYKEY_MAX_RESULTS "maxResults"
#define QUERYKEY_QUERY_TYPE "queryType"
#define QUERYKEY_TAG "tag"
#define QUERYKEY_NOTTAGS "!tags"

inline void AppendAmpersandIfNonempty(nsACString& aString)
{
  if (! aString.IsEmpty())
    aString.Append('&');
}
inline void AppendInt16(nsACString& str, PRInt16 i)
{
  nsCAutoString tmp;
  tmp.AppendInt(i);
  str.Append(tmp);
}
inline void AppendInt32(nsACString& str, PRInt32 i)
{
  nsCAutoString tmp;
  tmp.AppendInt(i);
  str.Append(tmp);
}
inline void AppendInt64(nsACString& str, PRInt64 i)
{
  nsCString tmp;
  tmp.AppendInt(i);
  str.Append(tmp);
}

namespace PlacesFolderConversion {
  #define PLACES_ROOT_FOLDER "PLACES_ROOT"
  #define BOOKMARKS_MENU_FOLDER "BOOKMARKS_MENU"
  #define TAGS_FOLDER "TAGS"
  #define UNFILED_BOOKMARKS_FOLDER "UNFILED_BOOKMARKS"
  #define TOOLBAR_FOLDER "TOOLBAR"
  
  






  inline PRInt64 DecodeFolder(const nsCString &aName)
  {
    nsNavBookmarks *bs = nsNavBookmarks::GetBookmarksService();
    NS_ENSURE_TRUE(bs, PR_FALSE);
    PRInt64 folderID = -1;

    if (aName.EqualsLiteral(PLACES_ROOT_FOLDER))
      (void)bs->GetPlacesRoot(&folderID);
    else if (aName.EqualsLiteral(BOOKMARKS_MENU_FOLDER))
      (void)bs->GetBookmarksMenuFolder(&folderID);
    else if (aName.EqualsLiteral(TAGS_FOLDER))
      (void)bs->GetTagsFolder(&folderID);
    else if (aName.EqualsLiteral(UNFILED_BOOKMARKS_FOLDER))
      (void)bs->GetUnfiledBookmarksFolder(&folderID);
    else if (aName.EqualsLiteral(TOOLBAR_FOLDER))
      (void)bs->GetToolbarFolder(&folderID);

    return folderID;
  }

  










  inline void AppendFolder(nsCString &aQuery, PRInt64 aFolderID)
  {
    nsNavBookmarks *bs = nsNavBookmarks::GetBookmarksService();
    PRInt64 folderID;

    (void)bs->GetPlacesRoot(&folderID);
    if (aFolderID == folderID) {
      aQuery.AppendLiteral(PLACES_ROOT_FOLDER);
      return;
    }

    (void)bs->GetBookmarksMenuFolder(&folderID);
    if (aFolderID == folderID) {
      aQuery.AppendLiteral(BOOKMARKS_MENU_FOLDER);
      return;
    }

    (void)bs->GetTagsFolder(&folderID);
    if (aFolderID == folderID) {
      aQuery.AppendLiteral(TAGS_FOLDER);
      return;
    }

    (void)bs->GetUnfiledBookmarksFolder(&folderID);
    if (aFolderID == folderID) {
      aQuery.AppendLiteral(UNFILED_BOOKMARKS_FOLDER);
      return;
    }

    (void)bs->GetToolbarFolder(&folderID);
    if (aFolderID == folderID) {
      aQuery.AppendLiteral(TOOLBAR_FOLDER);
      return;
    }

    
    aQuery.AppendInt(aFolderID);
  }
}






NS_IMETHODIMP
nsNavHistory::QueryStringToQueries(const nsACString& aQueryString,
                                   nsINavHistoryQuery*** aQueries,
                                   PRUint32* aResultCount,
                                   nsINavHistoryQueryOptions** aOptions)
{
  NS_ENSURE_ARG_POINTER(aQueries);
  NS_ENSURE_ARG_POINTER(aResultCount);
  NS_ENSURE_ARG_POINTER(aOptions);

  *aQueries = nsnull;
  *aResultCount = 0;
  nsCOMPtr<nsNavHistoryQueryOptions> options;
  nsCOMArray<nsNavHistoryQuery> queries;
  nsresult rv = QueryStringToQueryArray(aQueryString, &queries,
                                        getter_AddRefs(options));
  NS_ENSURE_SUCCESS(rv, rv);

  *aResultCount = queries.Count();
  if (queries.Count() > 0) {
    
    *aQueries = static_cast<nsINavHistoryQuery**>
                           (nsMemory::Alloc(sizeof(nsINavHistoryQuery*) * queries.Count()));
    NS_ENSURE_TRUE(*aQueries, NS_ERROR_OUT_OF_MEMORY);
    for (PRInt32 i = 0; i < queries.Count(); i ++) {
      (*aQueries)[i] = queries[i];
      NS_ADDREF((*aQueries)[i]);
    }
  }
  NS_ADDREF(*aOptions = options);
  return NS_OK;
}







nsresult
nsNavHistory::QueryStringToQueryArray(const nsACString& aQueryString,
                                      nsCOMArray<nsNavHistoryQuery>* aQueries,
                                      nsNavHistoryQueryOptions** aOptions)
{
  nsresult rv;
  aQueries->Clear();
  *aOptions = nsnull;

  nsRefPtr<nsNavHistoryQueryOptions> options(new nsNavHistoryQueryOptions());
  if (! options)
    return NS_ERROR_OUT_OF_MEMORY;

  nsTArray<QueryKeyValuePair> tokens;
  rv = TokenizeQueryString(aQueryString, &tokens);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = TokensToQueries(tokens, aQueries, options);
  if (NS_FAILED(rv)) {
    NS_WARNING("Unable to parse the query string: ");
    NS_WARNING(PromiseFlatCString(aQueryString).get());
    return rv;
  }

  NS_ADDREF(*aOptions = options);
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistory::QueriesToQueryString(nsINavHistoryQuery **aQueries,
                                   PRUint32 aQueryCount,
                                   nsINavHistoryQueryOptions* aOptions,
                                   nsACString& aQueryString)
{
  NS_ENSURE_ARG(aQueries);
  NS_ENSURE_ARG(aOptions);

  nsCOMPtr<nsNavHistoryQueryOptions> options = do_QueryInterface(aOptions);
  NS_ENSURE_TRUE(options, NS_ERROR_INVALID_ARG);

  nsCAutoString queryString;
  for (PRUint32 queryIndex = 0; queryIndex < aQueryCount;  queryIndex ++) {
    nsCOMPtr<nsNavHistoryQuery> query = do_QueryInterface(aQueries[queryIndex]);
    if (queryIndex > 0) {
      AppendAmpersandIfNonempty(queryString);
      queryString += NS_LITERAL_CSTRING(QUERYKEY_SEPARATOR);
    }

    PRBool hasIt;

    
    query->GetHasBeginTime(&hasIt);
    if (hasIt) {
      AppendInt64KeyValueIfNonzero(queryString,
                                   NS_LITERAL_CSTRING(QUERYKEY_BEGIN_TIME),
                                   query, &nsINavHistoryQuery::GetBeginTime);
      AppendUint32KeyValueIfNonzero(queryString,
                                    NS_LITERAL_CSTRING(QUERYKEY_BEGIN_TIME_REFERENCE),
                                    query, &nsINavHistoryQuery::GetBeginTimeReference);
    }

    
    query->GetHasEndTime(&hasIt);
    if (hasIt) {
      AppendInt64KeyValueIfNonzero(queryString,
                                   NS_LITERAL_CSTRING(QUERYKEY_END_TIME),
                                   query, &nsINavHistoryQuery::GetEndTime);
      AppendUint32KeyValueIfNonzero(queryString,
                                    NS_LITERAL_CSTRING(QUERYKEY_END_TIME_REFERENCE),
                                    query, &nsINavHistoryQuery::GetEndTimeReference);
    }

    
    query->GetHasSearchTerms(&hasIt);
    if (hasIt) {
      nsAutoString searchTerms;
      query->GetSearchTerms(searchTerms);
      nsCString escapedTerms;
      if (! NS_Escape(NS_ConvertUTF16toUTF8(searchTerms), escapedTerms,
                      url_XAlphas))
        return NS_ERROR_OUT_OF_MEMORY;

      AppendAmpersandIfNonempty(queryString);
      queryString += NS_LITERAL_CSTRING(QUERYKEY_SEARCH_TERMS "=");
      queryString += escapedTerms;
    }

    
    PRInt32 minVisits;
    if (NS_SUCCEEDED(query->GetMinVisits(&minVisits)) && minVisits >= 0) {
      AppendAmpersandIfNonempty(queryString);
      queryString.Append(NS_LITERAL_CSTRING(QUERYKEY_MIN_VISITS "="));
      AppendInt32(queryString, minVisits);
    }

    PRInt32 maxVisits;
    if (NS_SUCCEEDED(query->GetMaxVisits(&maxVisits)) && maxVisits >= 0) {
      AppendAmpersandIfNonempty(queryString);
      queryString.Append(NS_LITERAL_CSTRING(QUERYKEY_MAX_VISITS "="));
      AppendInt32(queryString, maxVisits);
    }

    
    AppendBoolKeyValueIfTrue(queryString,
                             NS_LITERAL_CSTRING(QUERYKEY_ONLY_BOOKMARKED),
                             query, &nsINavHistoryQuery::GetOnlyBookmarked);

    
    
    
    query->GetHasDomain(&hasIt);
    if (hasIt) {
      AppendBoolKeyValueIfTrue(queryString,
                               NS_LITERAL_CSTRING(QUERYKEY_DOMAIN_IS_HOST),
                               query, &nsINavHistoryQuery::GetDomainIsHost);
      nsCAutoString domain;
      nsresult rv = query->GetDomain(domain);
      NS_ASSERTION(NS_SUCCEEDED(rv), "Failure getting value");
      nsCString escapedDomain;
      PRBool success = NS_Escape(domain, escapedDomain, url_XAlphas);
      NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

      AppendAmpersandIfNonempty(queryString);
      queryString.Append(NS_LITERAL_CSTRING(QUERYKEY_DOMAIN "="));
      queryString.Append(escapedDomain);
    }

    
    query->GetHasUri(&hasIt);
    if (hasIt) {
      AppendBoolKeyValueIfTrue(aQueryString,
                               NS_LITERAL_CSTRING(QUERYKEY_URIISPREFIX),
                               query, &nsINavHistoryQuery::GetUriIsPrefix);
      nsCOMPtr<nsIURI> uri;
      query->GetUri(getter_AddRefs(uri));
      NS_ENSURE_TRUE(uri, NS_ERROR_FAILURE); 
      nsCAutoString uriSpec;
      nsresult rv = uri->GetSpec(uriSpec);
      NS_ENSURE_SUCCESS(rv, rv);
      nsCAutoString escaped;
      PRBool success = NS_Escape(uriSpec, escaped, url_XAlphas);
      NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

      AppendAmpersandIfNonempty(queryString);
      queryString.Append(NS_LITERAL_CSTRING(QUERYKEY_URI "="));
      queryString.Append(escaped);
    }

    
    query->GetHasAnnotation(&hasIt);
    if (hasIt) {
      AppendAmpersandIfNonempty(queryString);
      PRBool annotationIsNot;
      query->GetAnnotationIsNot(&annotationIsNot);
      if (annotationIsNot)
        queryString.AppendLiteral(QUERYKEY_NOTANNOTATION "=");
      else
        queryString.AppendLiteral(QUERYKEY_ANNOTATION "=");
      nsCAutoString annot;
      query->GetAnnotation(annot);
      nsCAutoString escaped;
      PRBool success = NS_Escape(annot, escaped, url_XAlphas);
      NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
      queryString.Append(escaped);
    }

    
    PRInt64 *folders = nsnull;
    PRUint32 folderCount = 0;
    query->GetFolders(&folderCount, &folders);
    for (PRUint32 i = 0; i < folderCount; ++i) {
      AppendAmpersandIfNonempty(queryString);
      queryString += NS_LITERAL_CSTRING(QUERYKEY_FOLDER "=");
      PlacesFolderConversion::AppendFolder(queryString, folders[i]);
    }
    nsMemory::Free(folders);

    
    const nsTArray<nsString> &tags = query->Tags();
    for (PRUint32 i = 0; i < tags.Length(); ++i) {
      nsCAutoString escapedTag;
      if (!NS_Escape(NS_ConvertUTF16toUTF8(tags[i]), escapedTag, url_XAlphas))
        return NS_ERROR_OUT_OF_MEMORY;

      AppendAmpersandIfNonempty(queryString);
      queryString += NS_LITERAL_CSTRING(QUERYKEY_TAG "=");
      queryString += escapedTag;
    }
    AppendBoolKeyValueIfTrue(queryString,
                             NS_LITERAL_CSTRING(QUERYKEY_NOTTAGS),
                             query,
                             &nsINavHistoryQuery::GetTagsAreNot);
  }

  
  if (options->SortingMode() != nsINavHistoryQueryOptions::SORT_BY_NONE) {
    AppendAmpersandIfNonempty(queryString);
    queryString += NS_LITERAL_CSTRING(QUERYKEY_SORT "=");
    AppendInt16(queryString, options->SortingMode());
    if (options->SortingMode() == nsINavHistoryQueryOptions::SORT_BY_ANNOTATION_DESCENDING ||
        options->SortingMode() == nsINavHistoryQueryOptions::SORT_BY_ANNOTATION_ASCENDING) {
      
      nsCAutoString sortingAnnotation;
      if (NS_SUCCEEDED(options->GetSortingAnnotation(sortingAnnotation))) {
        nsCString escaped;
        if (!NS_Escape(sortingAnnotation, escaped, url_XAlphas))
          return NS_ERROR_OUT_OF_MEMORY;
        AppendAmpersandIfNonempty(queryString);
        queryString += NS_LITERAL_CSTRING(QUERYKEY_SORTING_ANNOTATION "=");
        queryString.Append(escaped);
      }
    } 
  }

  
  if (options->ResultType() != nsINavHistoryQueryOptions::RESULTS_AS_URI) {
    AppendAmpersandIfNonempty(queryString);
    queryString += NS_LITERAL_CSTRING(QUERYKEY_RESULT_TYPE "=");
    AppendInt16(queryString, options->ResultType());
  }

  
  if (options->ExcludeItems()) {
    AppendAmpersandIfNonempty(queryString);
    queryString += NS_LITERAL_CSTRING(QUERYKEY_EXCLUDE_ITEMS "=1");
  }

  
  if (options->ExcludeQueries()) {
    AppendAmpersandIfNonempty(queryString);
    queryString += NS_LITERAL_CSTRING(QUERYKEY_EXCLUDE_QUERIES "=1");
  }

  
  if (options->ExcludeReadOnlyFolders()) {
    AppendAmpersandIfNonempty(queryString);
    queryString += NS_LITERAL_CSTRING(QUERYKEY_EXCLUDE_READ_ONLY_FOLDERS "=1");
  }

  
  nsCAutoString parentAnnotationToExclude;
  if (NS_SUCCEEDED(options->GetExcludeItemIfParentHasAnnotation(parentAnnotationToExclude)) &&
      !parentAnnotationToExclude.IsEmpty()) {
    nsCString escaped;
    if (!NS_Escape(parentAnnotationToExclude, escaped, url_XAlphas))
      return NS_ERROR_OUT_OF_MEMORY;
    AppendAmpersandIfNonempty(queryString);
    queryString += NS_LITERAL_CSTRING(QUERYKEY_EXCLUDE_ITEM_IF_PARENT_HAS_ANNOTATION "=");
    queryString.Append(escaped);
  }

  
  if (!options->ExpandQueries()) {
    AppendAmpersandIfNonempty(queryString);
    queryString += NS_LITERAL_CSTRING(QUERYKEY_EXPAND_QUERIES "=0");
  }

  
  if (options->IncludeHidden()) {
    AppendAmpersandIfNonempty(queryString);
    queryString += NS_LITERAL_CSTRING(QUERYKEY_INCLUDE_HIDDEN "=1");
  }

  
  if (options->RedirectsMode() !=  nsINavHistoryQueryOptions::REDIRECTS_MODE_ALL) {
    AppendAmpersandIfNonempty(queryString);
    queryString += NS_LITERAL_CSTRING(QUERYKEY_REDIRECTS_MODE "=");
    AppendInt16(queryString, options->RedirectsMode());
  }

  
  if (options->MaxResults()) {
    AppendAmpersandIfNonempty(queryString);
    queryString += NS_LITERAL_CSTRING(QUERYKEY_MAX_RESULTS "=");
    AppendInt32(queryString, options->MaxResults());
  }

  
  if (options->QueryType() !=  nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY) {
    AppendAmpersandIfNonempty(queryString);
    queryString += NS_LITERAL_CSTRING(QUERYKEY_QUERY_TYPE "=");
    AppendInt16(queryString, options->QueryType());
  }

  aQueryString.Assign(NS_LITERAL_CSTRING("place:") + queryString);
  return NS_OK;
}




nsresult
TokenizeQueryString(const nsACString& aQuery,
                    nsTArray<QueryKeyValuePair>* aTokens)
{
  
  const PRUint32 prefixlen = 6; 
  nsCString query;
  if (aQuery.Length() >= prefixlen &&
      Substring(aQuery, 0, prefixlen).EqualsLiteral("place:"))
    query = Substring(aQuery, prefixlen);
  else
    query = aQuery;

  PRInt32 keyFirstIndex = 0;
  PRInt32 equalsIndex = 0;
  for (PRUint32 i = 0; i < query.Length(); i ++) {
    if (query[i] == '&') {
      
      if (i - keyFirstIndex > 1) {
        if (! aTokens->AppendElement(QueryKeyValuePair(query, keyFirstIndex,
                                                       equalsIndex, i)))
          return NS_ERROR_OUT_OF_MEMORY;
      }
      keyFirstIndex = equalsIndex = i + 1;
    } else if (query[i] == '=') {
      equalsIndex = i;
    }
  }

  
  if (query.Length() - keyFirstIndex > 1) {
    if (! aTokens->AppendElement(QueryKeyValuePair(query, keyFirstIndex,
                                                   equalsIndex, query.Length())))
      return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}



nsresult
nsNavHistory::TokensToQueries(const nsTArray<QueryKeyValuePair>& aTokens,
                              nsCOMArray<nsNavHistoryQuery>* aQueries,
                              nsNavHistoryQueryOptions* aOptions)
{
  nsresult rv;

  nsCOMPtr<nsNavHistoryQuery> query(new nsNavHistoryQuery());
  if (! query)
    return NS_ERROR_OUT_OF_MEMORY;
  if (! aQueries->AppendObject(query))
    return NS_ERROR_OUT_OF_MEMORY;

  if (aTokens.Length() == 0)
    return NS_OK; 

  nsTArray<PRInt64> folders;
  nsTArray<nsString> tags;
  for (PRUint32 i = 0; i < aTokens.Length(); i ++) {
    const QueryKeyValuePair& kvp = aTokens[i];

    
    if (kvp.key.EqualsLiteral(QUERYKEY_BEGIN_TIME)) {
      SetQueryKeyInt64(kvp.value, query, &nsINavHistoryQuery::SetBeginTime);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_BEGIN_TIME_REFERENCE)) {
      SetQueryKeyUint32(kvp.value, query, &nsINavHistoryQuery::SetBeginTimeReference);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_END_TIME)) {
      SetQueryKeyInt64(kvp.value, query, &nsINavHistoryQuery::SetEndTime);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_END_TIME_REFERENCE)) {
      SetQueryKeyUint32(kvp.value, query, &nsINavHistoryQuery::SetEndTimeReference);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_SEARCH_TERMS)) {
      nsCString unescapedTerms = kvp.value;
      NS_UnescapeURL(unescapedTerms); 
      rv = query->SetSearchTerms(NS_ConvertUTF8toUTF16(unescapedTerms));
      NS_ENSURE_SUCCESS(rv, rv);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_MIN_VISITS)) {
      PRInt32 visits = kvp.value.ToInteger((PRInt32*)&rv);
      if (NS_SUCCEEDED(rv))
        query->SetMinVisits(visits);
      else
        NS_WARNING("Bad number for minVisits in query");

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_MAX_VISITS)) {
      PRInt32 visits = kvp.value.ToInteger((PRInt32*)&rv);
      if (NS_SUCCEEDED(rv))
        query->SetMaxVisits(visits);
      else
        NS_WARNING("Bad number for maxVisits in query");

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_ONLY_BOOKMARKED)) {
      SetQueryKeyBool(kvp.value, query, &nsINavHistoryQuery::SetOnlyBookmarked);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_DOMAIN_IS_HOST)) {
      SetQueryKeyBool(kvp.value, query, &nsINavHistoryQuery::SetDomainIsHost);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_DOMAIN)) {
      nsCAutoString unescapedDomain(kvp.value);
      NS_UnescapeURL(unescapedDomain); 
      rv = query->SetDomain(unescapedDomain);
      NS_ENSURE_SUCCESS(rv, rv);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_FOLDER)) {
      PRInt64 folder;
      if (PR_sscanf(kvp.value.get(), "%lld", &folder) == 1) {
        NS_ENSURE_TRUE(folders.AppendElement(folder), NS_ERROR_OUT_OF_MEMORY);
      } else {
        folder = PlacesFolderConversion::DecodeFolder(kvp.value);
        if (folder != -1)
          NS_ENSURE_TRUE(folders.AppendElement(folder), NS_ERROR_OUT_OF_MEMORY);
        else
          NS_WARNING("folders value in query is invalid, ignoring");
      }

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_URI)) {
      nsCAutoString unescapedUri(kvp.value);
      NS_UnescapeURL(unescapedUri); 
      nsCOMPtr<nsIURI> uri;
      nsresult rv = NS_NewURI(getter_AddRefs(uri), unescapedUri);
      if (NS_FAILED(rv)) {
        NS_WARNING("Unable to parse URI");
      }
      rv = query->SetUri(uri);
      NS_ENSURE_SUCCESS(rv, rv);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_URIISPREFIX)) {
      SetQueryKeyBool(kvp.value, query, &nsINavHistoryQuery::SetUriIsPrefix);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_NOTANNOTATION)) {
      nsCAutoString unescaped(kvp.value);
      NS_UnescapeURL(unescaped); 
      query->SetAnnotationIsNot(PR_TRUE);
      query->SetAnnotation(unescaped);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_ANNOTATION)) {
      nsCAutoString unescaped(kvp.value);
      NS_UnescapeURL(unescaped); 
      query->SetAnnotationIsNot(PR_FALSE);
      query->SetAnnotation(unescaped);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_TAG)) {
      nsCAutoString unescaped(kvp.value);
      NS_UnescapeURL(unescaped); 
      nsString tag = NS_ConvertUTF8toUTF16(unescaped);
      if (!tags.Contains(tag)) {
        NS_ENSURE_TRUE(tags.AppendElement(tag), NS_ERROR_OUT_OF_MEMORY);
      }

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_NOTTAGS)) {
      SetQueryKeyBool(kvp.value, query, &nsINavHistoryQuery::SetTagsAreNot);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_SEPARATOR)) {

      if (folders.Length() != 0) {
        query->SetFolders(folders.Elements(), folders.Length());
        folders.Clear();
      }

      if (tags.Length() > 0) {
        rv = query->SetTags(tags);
        NS_ENSURE_SUCCESS(rv, rv);
        tags.Clear();
      }

      query = new nsNavHistoryQuery();
      if (! query)
        return NS_ERROR_OUT_OF_MEMORY;
      if (! aQueries->AppendObject(query))
        return NS_ERROR_OUT_OF_MEMORY;

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_SORT)) {
      SetOptionsKeyUint16(kvp.value, aOptions,
                          &nsINavHistoryQueryOptions::SetSortingMode);
    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_SORTING_ANNOTATION)) {
      nsCString sortingAnnotation = kvp.value;
      NS_UnescapeURL(sortingAnnotation);
      rv = aOptions->SetSortingAnnotation(sortingAnnotation);
      NS_ENSURE_SUCCESS(rv, rv);
    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_RESULT_TYPE)) {
      SetOptionsKeyUint16(kvp.value, aOptions,
                          &nsINavHistoryQueryOptions::SetResultType);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_EXCLUDE_ITEMS)) {
      SetOptionsKeyBool(kvp.value, aOptions,
                        &nsINavHistoryQueryOptions::SetExcludeItems);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_EXCLUDE_QUERIES)) {
      SetOptionsKeyBool(kvp.value, aOptions,
                        &nsINavHistoryQueryOptions::SetExcludeQueries);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_EXCLUDE_READ_ONLY_FOLDERS)) {
      SetOptionsKeyBool(kvp.value, aOptions,
                        &nsINavHistoryQueryOptions::SetExcludeReadOnlyFolders);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_EXCLUDE_ITEM_IF_PARENT_HAS_ANNOTATION)) {
      nsCString parentAnnotationToExclude = kvp.value;
      NS_UnescapeURL(parentAnnotationToExclude);
      rv = aOptions->SetExcludeItemIfParentHasAnnotation(parentAnnotationToExclude);
      NS_ENSURE_SUCCESS(rv, rv);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_EXPAND_QUERIES)) {
      SetOptionsKeyBool(kvp.value, aOptions,
                        &nsINavHistoryQueryOptions::SetExpandQueries);
    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_INCLUDE_HIDDEN)) {
      SetOptionsKeyBool(kvp.value, aOptions,
                        &nsINavHistoryQueryOptions::SetIncludeHidden);
    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_REDIRECTS_MODE)) {
      SetOptionsKeyUint16(kvp.value, aOptions,
                          &nsINavHistoryQueryOptions::SetRedirectsMode);
    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_MAX_RESULTS)) {
      SetOptionsKeyUint32(kvp.value, aOptions,
                          &nsINavHistoryQueryOptions::SetMaxResults);
    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_QUERY_TYPE)) {
      SetOptionsKeyUint16(kvp.value, aOptions,
                          &nsINavHistoryQueryOptions::SetQueryType);
    
    } else {
      NS_WARNING("TokensToQueries(), ignoring unknown key: ");
      NS_WARNING(kvp.key.get());
    }
  }

  if (folders.Length() != 0)
    query->SetFolders(folders.Elements(), folders.Length());

  if (tags.Length() > 0) {
    rv = query->SetTags(tags);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}






nsresult
ParseQueryBooleanString(const nsCString& aString, PRBool* aValue)
{
  if (aString.EqualsLiteral("1") || aString.EqualsLiteral("true")) {
    *aValue = PR_TRUE;
    return NS_OK;
  } else if (aString.EqualsLiteral("0") || aString.EqualsLiteral("false")) {
    *aValue = PR_FALSE;
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}




NS_IMPL_ISUPPORTS2(nsNavHistoryQuery, nsNavHistoryQuery, nsINavHistoryQuery)







nsNavHistoryQuery::nsNavHistoryQuery()
  : mMinVisits(-1), mMaxVisits(-1), mBeginTime(0),
    mBeginTimeReference(TIME_RELATIVE_EPOCH),
    mEndTime(0), mEndTimeReference(TIME_RELATIVE_EPOCH),
    mOnlyBookmarked(PR_FALSE),
    mDomainIsHost(PR_FALSE), mUriIsPrefix(PR_FALSE),
    mAnnotationIsNot(PR_FALSE),
    mTagsAreNot(PR_FALSE)
{
  
  mDomain.SetIsVoid(PR_TRUE);
}


NS_IMETHODIMP nsNavHistoryQuery::GetBeginTime(PRTime *aBeginTime)
{
  *aBeginTime = mBeginTime;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetBeginTime(PRTime aBeginTime)
{
  mBeginTime = aBeginTime;
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetBeginTimeReference(PRUint32* _retval)
{
  *_retval = mBeginTimeReference;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetBeginTimeReference(PRUint32 aReference)
{
  if (aReference > TIME_RELATIVE_NOW)
    return NS_ERROR_INVALID_ARG;
  mBeginTimeReference = aReference;
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetHasBeginTime(PRBool* _retval)
{
  *_retval = ! (mBeginTimeReference == TIME_RELATIVE_EPOCH && mBeginTime == 0);
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetAbsoluteBeginTime(PRTime* _retval)
{
  *_retval = nsNavHistory::NormalizeTime(mBeginTimeReference, mBeginTime);
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetEndTime(PRTime *aEndTime)
{
  *aEndTime = mEndTime;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetEndTime(PRTime aEndTime)
{
  mEndTime = aEndTime;
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetEndTimeReference(PRUint32* _retval)
{
  *_retval = mEndTimeReference;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetEndTimeReference(PRUint32 aReference)
{
  if (aReference > TIME_RELATIVE_NOW)
    return NS_ERROR_INVALID_ARG;
  mEndTimeReference = aReference;
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetHasEndTime(PRBool* _retval)
{
  *_retval = ! (mEndTimeReference == TIME_RELATIVE_EPOCH && mEndTime == 0);
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetAbsoluteEndTime(PRTime* _retval)
{
  *_retval = nsNavHistory::NormalizeTime(mEndTimeReference, mEndTime);
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetSearchTerms(nsAString& aSearchTerms)
{
  aSearchTerms = mSearchTerms;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetSearchTerms(const nsAString& aSearchTerms)
{
  mSearchTerms = aSearchTerms;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::GetHasSearchTerms(PRBool* _retval)
{
  *_retval = (! mSearchTerms.IsEmpty());
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetMinVisits(PRInt32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = mMinVisits;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetMinVisits(PRInt32 aVisits)
{
  mMinVisits = aVisits;
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetMaxVisits(PRInt32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = mMaxVisits;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetMaxVisits(PRInt32 aVisits)
{
  mMaxVisits = aVisits;
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetOnlyBookmarked(PRBool *aOnlyBookmarked)
{
  *aOnlyBookmarked = mOnlyBookmarked;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetOnlyBookmarked(PRBool aOnlyBookmarked)
{
  mOnlyBookmarked = aOnlyBookmarked;
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetDomainIsHost(PRBool *aDomainIsHost)
{
  *aDomainIsHost = mDomainIsHost;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetDomainIsHost(PRBool aDomainIsHost)
{
  mDomainIsHost = aDomainIsHost;
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetDomain(nsACString& aDomain)
{
  aDomain = mDomain;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetDomain(const nsACString& aDomain)
{
  mDomain = aDomain;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::GetHasDomain(PRBool* _retval)
{
  
  *_retval = (! mDomain.IsVoid());
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetUriIsPrefix(PRBool* aIsPrefix)
{
  *aIsPrefix = mUriIsPrefix;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetUriIsPrefix(PRBool aIsPrefix)
{
  mUriIsPrefix = aIsPrefix;
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetUri(nsIURI** aUri)
{
  NS_IF_ADDREF(*aUri = mUri);
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetUri(nsIURI* aUri)
{
  mUri = aUri;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::GetHasUri(PRBool* aHasUri)
{
  *aHasUri = (mUri != nsnull);
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetAnnotationIsNot(PRBool* aIsNot)
{
  *aIsNot = mAnnotationIsNot;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetAnnotationIsNot(PRBool aIsNot)
{
  mAnnotationIsNot = aIsNot;
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetAnnotation(nsACString& aAnnotation)
{
  aAnnotation = mAnnotation;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetAnnotation(const nsACString& aAnnotation)
{
  mAnnotation = aAnnotation;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::GetHasAnnotation(PRBool* aHasIt)
{
  *aHasIt = ! mAnnotation.IsEmpty();
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetTags(nsIVariant **aTags)
{
  NS_ENSURE_ARG_POINTER(aTags);

  nsresult rv;
  nsCOMPtr<nsIWritableVariant> out = do_CreateInstance(NS_VARIANT_CONTRACTID,
                                                       &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 arrayLen = mTags.Length();

  if (arrayLen == 0)
    rv = out->SetAsEmptyArray();
  else {
    
    const PRUnichar **array = reinterpret_cast<const PRUnichar **>
                              (NS_Alloc(arrayLen * sizeof(PRUnichar *)));
    NS_ENSURE_TRUE(array, NS_ERROR_OUT_OF_MEMORY);

    for (PRUint32 i = 0; i < arrayLen; ++i) {
      array[i] = mTags[i].get();
    }

    rv = out->SetAsArray(nsIDataType::VTYPE_WCHAR_STR,
                         nsnull,
                         arrayLen,
                         reinterpret_cast<void *>(array));
    NS_Free(array);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aTags = out);
  return NS_OK;
}

NS_IMETHODIMP nsNavHistoryQuery::SetTags(nsIVariant *aTags)
{
  NS_ENSURE_ARG(aTags);

  PRUint16 dataType;
  aTags->GetDataType(&dataType);

  
  if (dataType == nsIDataType::VTYPE_EMPTY_ARRAY) {
    mTags.Clear();
    return NS_OK;
  }

  
  NS_ENSURE_TRUE(dataType == nsIDataType::VTYPE_ARRAY, NS_ERROR_ILLEGAL_VALUE);

  PRUint16 eltType;
  nsIID eltIID;
  PRUint32 arrayLen;
  void *array;

  
  
  nsresult rv = aTags->GetAsArray(&eltType, &eltIID, &arrayLen, &array);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (eltType != nsIDataType::VTYPE_WCHAR_STR) {
    switch (eltType) {
    case nsIDataType::VTYPE_ID:
    case nsIDataType::VTYPE_CHAR_STR:
      {
        char **charArray = reinterpret_cast<char **>(array);
        for (PRUint32 i = 0; i < arrayLen; ++i) {
          if (charArray[i])
            NS_Free(charArray[i]);
        }
      }
      break;
    case nsIDataType::VTYPE_INTERFACE:
    case nsIDataType::VTYPE_INTERFACE_IS:
      {
        nsISupports **supportsArray = reinterpret_cast<nsISupports **>(array);
        for (PRUint32 i = 0; i < arrayLen; ++i) {
          NS_IF_RELEASE(supportsArray[i]);
        }
      }
      break;
    
    }
    NS_Free(array);
    return NS_ERROR_ILLEGAL_VALUE;
  }

  PRUnichar **tags = reinterpret_cast<PRUnichar **>(array);
  mTags.Clear();

  
  for (PRUint32 i = 0; i < arrayLen; ++i) {

    
    if (!tags[i]) {
      NS_Free(tags);
      return NS_ERROR_ILLEGAL_VALUE;
    }

    nsDependentString tag(tags[i]);

    
    
    if (!mTags.Contains(tag)) {
      if (!mTags.AppendElement(tag)) {
        NS_Free(tags[i]);
        NS_Free(tags);
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
    NS_Free(tags[i]);
  }
  NS_Free(tags);

  mTags.Sort();

  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetTagsAreNot(PRBool *aTagsAreNot)
{
  NS_ENSURE_ARG_POINTER(aTagsAreNot);
  *aTagsAreNot = mTagsAreNot;
  return NS_OK;
}

NS_IMETHODIMP nsNavHistoryQuery::SetTagsAreNot(PRBool aTagsAreNot)
{
  mTagsAreNot = aTagsAreNot;
  return NS_OK;
}

NS_IMETHODIMP nsNavHistoryQuery::GetFolders(PRUint32 *aCount,
                                            PRInt64 **aFolders)
{
  PRUint32 count = mFolders.Length();
  PRInt64 *folders = nsnull;
  if (count > 0) {
    folders = static_cast<PRInt64*>
                         (nsMemory::Alloc(count * sizeof(PRInt64)));
    NS_ENSURE_TRUE(folders, NS_ERROR_OUT_OF_MEMORY);

    for (PRUint32 i = 0; i < count; ++i) {
      folders[i] = mFolders[i];
    }
  }
  *aCount = count;
  *aFolders = folders;
  return NS_OK;
}

NS_IMETHODIMP nsNavHistoryQuery::GetFolderCount(PRUint32 *aCount)
{
  *aCount = mFolders.Length();
  return NS_OK;
}

NS_IMETHODIMP nsNavHistoryQuery::SetFolders(const PRInt64 *aFolders,
                                            PRUint32 aFolderCount)
{
  if (!mFolders.ReplaceElementsAt(0, mFolders.Length(),
                                  aFolders, aFolderCount)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

NS_IMETHODIMP nsNavHistoryQuery::Clone(nsINavHistoryQuery** _retval)
{
  *_retval = nsnull;

  nsNavHistoryQuery *clone = new nsNavHistoryQuery(*this);
  NS_ENSURE_TRUE(clone, NS_ERROR_OUT_OF_MEMORY);

  clone->mRefCnt = 0; 
  NS_ADDREF(*_retval = clone);
  return NS_OK;
}



NS_IMPL_ISUPPORTS2(nsNavHistoryQueryOptions, nsNavHistoryQueryOptions, nsINavHistoryQueryOptions)


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetSortingMode(PRUint16* aMode)
{
  *aMode = mSort;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryOptions::SetSortingMode(PRUint16 aMode)
{
  if (aMode > SORT_BY_ANNOTATION_DESCENDING)
    return NS_ERROR_INVALID_ARG;
  mSort = aMode;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetSortingAnnotation(nsACString& _result) {
  _result.Assign(mSortingAnnotation);
  return NS_OK;
}

NS_IMETHODIMP
nsNavHistoryQueryOptions::SetSortingAnnotation(const nsACString& aSortingAnnotation) {
  mSortingAnnotation.Assign(aSortingAnnotation);
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetResultType(PRUint16* aType)
{
  *aType = mResultType;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryOptions::SetResultType(PRUint16 aType)
{
  if (aType > RESULTS_AS_TAG_CONTENTS)
    return NS_ERROR_INVALID_ARG;
  
  
  if (aType == RESULTS_AS_TAG_QUERY || aType == RESULTS_AS_TAG_CONTENTS)
    mQueryType = QUERY_TYPE_BOOKMARKS;
  mResultType = aType;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetExcludeItems(PRBool* aExclude)
{
  *aExclude = mExcludeItems;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryOptions::SetExcludeItems(PRBool aExclude)
{
  mExcludeItems = aExclude;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetExcludeQueries(PRBool* aExclude)
{
  *aExclude = mExcludeQueries;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryOptions::SetExcludeQueries(PRBool aExclude)
{
  mExcludeQueries = aExclude;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetExcludeReadOnlyFolders(PRBool* aExclude)
{
  *aExclude = mExcludeReadOnlyFolders;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryOptions::SetExcludeReadOnlyFolders(PRBool aExclude)
{
  mExcludeReadOnlyFolders = aExclude;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetExcludeItemIfParentHasAnnotation(nsACString& _result) {
  _result.Assign(mParentAnnotationToExclude);
  return NS_OK;
}

NS_IMETHODIMP
nsNavHistoryQueryOptions::SetExcludeItemIfParentHasAnnotation(const nsACString& aParentAnnotationToExclude) {
  mParentAnnotationToExclude.Assign(aParentAnnotationToExclude);
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetExpandQueries(PRBool* aExpand)
{
  *aExpand = mExpandQueries;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryOptions::SetExpandQueries(PRBool aExpand)
{
  mExpandQueries = aExpand;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetIncludeHidden(PRBool* aIncludeHidden)
{
  *aIncludeHidden = mIncludeHidden;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryOptions::SetIncludeHidden(PRBool aIncludeHidden)
{
  mIncludeHidden = aIncludeHidden;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetRedirectsMode(PRUint16* _retval)
{
  *_retval = mRedirectsMode;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryOptions::SetRedirectsMode(PRUint16 aRedirectsMode)
{
  mRedirectsMode = aRedirectsMode;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetMaxResults(PRUint32* aMaxResults)
{
  *aMaxResults = mMaxResults;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryOptions::SetMaxResults(PRUint32 aMaxResults)
{
  mMaxResults = aMaxResults;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetQueryType(PRUint16* _retval)
{
  *_retval = mQueryType;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryOptions::SetQueryType(PRUint16 aQueryType)
{
  
  
  if (mResultType == RESULTS_AS_TAG_CONTENTS ||
      mResultType == RESULTS_AS_TAG_QUERY)
   return NS_OK;
  mQueryType = aQueryType;
  return NS_OK;
}

NS_IMETHODIMP
nsNavHistoryQueryOptions::Clone(nsINavHistoryQueryOptions** aResult)
{
  nsNavHistoryQueryOptions *clone = nsnull;
  nsresult rv = Clone(&clone);
  *aResult = clone;
  return rv;
}

nsresult
nsNavHistoryQueryOptions::Clone(nsNavHistoryQueryOptions **aResult)
{
  *aResult = nsnull;
  nsNavHistoryQueryOptions *result = new nsNavHistoryQueryOptions();
  if (! result)
    return NS_ERROR_OUT_OF_MEMORY;

  nsRefPtr<nsNavHistoryQueryOptions> resultHolder(result);
  result->mSort = mSort;
  result->mResultType = mResultType;
  result->mExcludeItems = mExcludeItems;
  result->mExcludeQueries = mExcludeQueries;
  result->mExpandQueries = mExpandQueries;
  result->mMaxResults = mMaxResults;
  result->mQueryType = mQueryType;
  result->mParentAnnotationToExclude = mParentAnnotationToExclude;

  resultHolder.swap(*aResult);
  return NS_OK;
}




void 
AppendBoolKeyValueIfTrue(nsACString& aString, const nsCString& aName,
                         nsINavHistoryQuery* aQuery,
                         BoolQueryGetter getter)
{
  PRBool value;
  nsresult rv = (aQuery->*getter)(&value);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failure getting boolean value");
  if (value) {
    AppendAmpersandIfNonempty(aString);
    aString += aName;
    aString.AppendLiteral("=1");
  }
}




void 
AppendUint32KeyValueIfNonzero(nsACString& aString,
                              const nsCString& aName,
                              nsINavHistoryQuery* aQuery,
                              Uint32QueryGetter getter)
{
  PRUint32 value;
  nsresult rv = (aQuery->*getter)(&value);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failure getting value");
  if (value) {
    AppendAmpersandIfNonempty(aString);
    aString += aName;

    
    nsCAutoString appendMe("=");
    appendMe.AppendInt(value);
    aString.Append(appendMe);
  }
}




void 
AppendInt64KeyValueIfNonzero(nsACString& aString,
                             const nsCString& aName,
                             nsINavHistoryQuery* aQuery,
                             Int64QueryGetter getter)
{
  PRInt64 value;
  nsresult rv = (aQuery->*getter)(&value);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failure getting value");
  if (value) {
    AppendAmpersandIfNonempty(aString);
    aString += aName;
    nsCAutoString appendMe("=");
    appendMe.AppendInt(value);
    aString.Append(appendMe);
  }
}




void 
SetQueryKeyBool(const nsCString& aValue, nsINavHistoryQuery* aQuery,
                BoolQuerySetter setter)
{
  PRBool value;
  nsresult rv = ParseQueryBooleanString(aValue, &value);
  if (NS_SUCCEEDED(rv)) {
    rv = (aQuery->*setter)(value);
    if (NS_FAILED(rv)) {
      NS_WARNING("Error setting boolean key value");
    }
  } else {
    NS_WARNING("Invalid boolean key value in query string.");
  }
}
void 
SetOptionsKeyBool(const nsCString& aValue, nsINavHistoryQueryOptions* aOptions,
                 BoolOptionsSetter setter)
{
  PRBool value;
  nsresult rv = ParseQueryBooleanString(aValue, &value);
  if (NS_SUCCEEDED(rv)) {
    rv = (aOptions->*setter)(value);
    if (NS_FAILED(rv)) {
      NS_WARNING("Error setting boolean key value");
    }
  } else {
    NS_WARNING("Invalid boolean key value in query string.");
  }
}




void 
SetQueryKeyUint32(const nsCString& aValue, nsINavHistoryQuery* aQuery,
                  Uint32QuerySetter setter)
{
  nsresult rv;
  PRUint32 value = aValue.ToInteger(reinterpret_cast<PRInt32*>(&rv));
  if (NS_SUCCEEDED(rv)) {
    rv = (aQuery->*setter)(value);
    if (NS_FAILED(rv)) {
      NS_WARNING("Error setting Int32 key value");
    }
  } else {
    NS_WARNING("Invalid Int32 key value in query string.");
  }
}
void 
SetOptionsKeyUint32(const nsCString& aValue, nsINavHistoryQueryOptions* aOptions,
                  Uint32OptionsSetter setter)
{
  nsresult rv;
  PRUint32 value = aValue.ToInteger(reinterpret_cast<PRInt32*>(&rv));
  if (NS_SUCCEEDED(rv)) {
    rv = (aOptions->*setter)(value);
    if (NS_FAILED(rv)) {
      NS_WARNING("Error setting Int32 key value");
    }
  } else {
    NS_WARNING("Invalid Int32 key value in query string.");
  }
}

void 
SetOptionsKeyUint16(const nsCString& aValue, nsINavHistoryQueryOptions* aOptions,
                    Uint16OptionsSetter setter)
{
  nsresult rv;
  PRUint16 value = static_cast<PRUint16>
                              (aValue.ToInteger(reinterpret_cast<PRInt32*>(&rv)));
  if (NS_SUCCEEDED(rv)) {
    rv = (aOptions->*setter)(value);
    if (NS_FAILED(rv)) {
      NS_WARNING("Error setting Int16 key value");
    }
  } else {
    NS_WARNING("Invalid Int16 key value in query string.");
  }
}




void SetQueryKeyInt64(const nsCString& aValue, nsINavHistoryQuery* aQuery,
                      Int64QuerySetter setter)
{
  nsresult rv;
  PRInt64 value;
  if (PR_sscanf(aValue.get(), "%lld", &value) == 1) {
    rv = (aQuery->*setter)(value);
    if (NS_FAILED(rv)) {
      NS_WARNING("Error setting Int64 key value");
    }
  } else {
    NS_WARNING("Invalid Int64 value in query string.");
  }
}
