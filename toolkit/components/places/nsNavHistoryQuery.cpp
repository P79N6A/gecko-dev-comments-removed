










#include "mozilla/DebugOnly.h"

#include "nsNavHistory.h"
#include "nsNavBookmarks.h"
#include "nsEscape.h"
#include "nsCOMArray.h"
#include "nsNetUtil.h"
#include "nsTArray.h"
#include "prprf.h"

using namespace mozilla;

class QueryKeyValuePair
{
public:

  
  
  
  
  
  
  
  
  
  

  QueryKeyValuePair(const nsCSubstring& aSource, int32_t aKeyBegin,
                    int32_t aEquals, int32_t aPastEnd)
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
                                        bool* aValue);


typedef NS_STDCALL_FUNCPROTO(nsresult, BoolQueryGetter, nsINavHistoryQuery,
                             GetOnlyBookmarked, (bool*));
typedef NS_STDCALL_FUNCPROTO(nsresult, Uint32QueryGetter, nsINavHistoryQuery,
                             GetBeginTimeReference, (uint32_t*));
typedef NS_STDCALL_FUNCPROTO(nsresult, Int64QueryGetter, nsINavHistoryQuery,
                             GetBeginTime, (int64_t*));
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
                             SetOnlyBookmarked, (bool));
typedef NS_STDCALL_FUNCPROTO(nsresult, Uint32QuerySetter, nsINavHistoryQuery,
                             SetBeginTimeReference, (uint32_t));
typedef NS_STDCALL_FUNCPROTO(nsresult, Int64QuerySetter, nsINavHistoryQuery,
                             SetBeginTime, (int64_t));
static void SetQueryKeyBool(const nsCString& aValue, nsINavHistoryQuery* aQuery,
                            BoolQuerySetter setter);
static void SetQueryKeyUint32(const nsCString& aValue, nsINavHistoryQuery* aQuery,
                              Uint32QuerySetter setter);
static void SetQueryKeyInt64(const nsCString& aValue, nsINavHistoryQuery* aQuery,
                             Int64QuerySetter setter);


typedef NS_STDCALL_FUNCPROTO(nsresult, BoolOptionsSetter,
                             nsINavHistoryQueryOptions,
                             SetExpandQueries, (bool));
typedef NS_STDCALL_FUNCPROTO(nsresult, Uint32OptionsSetter,
                             nsINavHistoryQueryOptions,
                             SetMaxResults, (uint32_t));
typedef NS_STDCALL_FUNCPROTO(nsresult, Uint16OptionsSetter,
                             nsINavHistoryQueryOptions,
                             SetResultType, (uint16_t));
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
#define QUERYKEY_EXPAND_QUERIES "expandQueries"
#define QUERYKEY_FORCE_ORIGINAL_TITLE "originalTitle"
#define QUERYKEY_INCLUDE_HIDDEN "includeHidden"
#define QUERYKEY_MAX_RESULTS "maxResults"
#define QUERYKEY_QUERY_TYPE "queryType"
#define QUERYKEY_TAG "tag"
#define QUERYKEY_NOTTAGS "!tags"
#define QUERYKEY_ASYNC_ENABLED "asyncEnabled"
#define QUERYKEY_TRANSITION "transition"

inline void AppendAmpersandIfNonempty(nsACString& aString)
{
  if (! aString.IsEmpty())
    aString.Append('&');
}
inline void AppendInt16(nsACString& str, int16_t i)
{
  nsAutoCString tmp;
  tmp.AppendInt(i);
  str.Append(tmp);
}
inline void AppendInt32(nsACString& str, int32_t i)
{
  nsAutoCString tmp;
  tmp.AppendInt(i);
  str.Append(tmp);
}
inline void AppendInt64(nsACString& str, int64_t i)
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
  
  






  inline int64_t DecodeFolder(const nsCString &aName)
  {
    nsNavBookmarks *bs = nsNavBookmarks::GetBookmarksService();
    NS_ENSURE_TRUE(bs, false);
    int64_t folderID = -1;

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

  










  inline nsresult AppendFolder(nsCString &aQuery, int64_t aFolderID)
  {
    nsNavBookmarks *bs = nsNavBookmarks::GetBookmarksService();
    NS_ENSURE_STATE(bs);
    int64_t folderID;

    if (NS_SUCCEEDED(bs->GetPlacesRoot(&folderID)) &&
        aFolderID == folderID) {
      aQuery.AppendLiteral(PLACES_ROOT_FOLDER);
    }
    else if (NS_SUCCEEDED(bs->GetBookmarksMenuFolder(&folderID)) &&
             aFolderID == folderID) {
      aQuery.AppendLiteral(BOOKMARKS_MENU_FOLDER);
    }
    else if (NS_SUCCEEDED(bs->GetTagsFolder(&folderID)) &&
             aFolderID == folderID) {
      aQuery.AppendLiteral(TAGS_FOLDER);
    }
    else if (NS_SUCCEEDED(bs->GetUnfiledBookmarksFolder(&folderID)) &&
             aFolderID == folderID) {
      aQuery.AppendLiteral(UNFILED_BOOKMARKS_FOLDER);
    }
    else if (NS_SUCCEEDED(bs->GetToolbarFolder(&folderID)) &&
             aFolderID == folderID) {
      aQuery.AppendLiteral(TOOLBAR_FOLDER);
    }
    else {
      
      aQuery.AppendInt(aFolderID);
    }

    return NS_OK;
  }
}






NS_IMETHODIMP
nsNavHistory::QueryStringToQueries(const nsACString& aQueryString,
                                   nsINavHistoryQuery*** aQueries,
                                   uint32_t* aResultCount,
                                   nsINavHistoryQueryOptions** aOptions)
{
  NS_ENSURE_ARG_POINTER(aQueries);
  NS_ENSURE_ARG_POINTER(aResultCount);
  NS_ENSURE_ARG_POINTER(aOptions);

  *aQueries = nullptr;
  *aResultCount = 0;
  nsCOMPtr<nsNavHistoryQueryOptions> options;
  nsCOMArray<nsNavHistoryQuery> queries;
  nsresult rv = QueryStringToQueryArray(aQueryString, &queries,
                                        getter_AddRefs(options));
  NS_ENSURE_SUCCESS(rv, rv);

  *aResultCount = queries.Count();
  if (queries.Count() > 0) {
    
    *aQueries = static_cast<nsINavHistoryQuery**>
                           (moz_xmalloc(sizeof(nsINavHistoryQuery*) * queries.Count()));
    NS_ENSURE_TRUE(*aQueries, NS_ERROR_OUT_OF_MEMORY);
    for (int32_t i = 0; i < queries.Count(); i ++) {
      (*aQueries)[i] = queries[i];
      NS_ADDREF((*aQueries)[i]);
    }
  }
  options.forget(aOptions);
  return NS_OK;
}







nsresult
nsNavHistory::QueryStringToQueryArray(const nsACString& aQueryString,
                                      nsCOMArray<nsNavHistoryQuery>* aQueries,
                                      nsNavHistoryQueryOptions** aOptions)
{
  nsresult rv;
  aQueries->Clear();
  *aOptions = nullptr;

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

  options.forget(aOptions);
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistory::QueriesToQueryString(nsINavHistoryQuery **aQueries,
                                   uint32_t aQueryCount,
                                   nsINavHistoryQueryOptions* aOptions,
                                   nsACString& aQueryString)
{
  NS_ENSURE_ARG(aQueries);
  NS_ENSURE_ARG(aOptions);

  nsCOMPtr<nsNavHistoryQueryOptions> options = do_QueryInterface(aOptions);
  NS_ENSURE_TRUE(options, NS_ERROR_INVALID_ARG);

  nsAutoCString queryString;
  for (uint32_t queryIndex = 0; queryIndex < aQueryCount;  queryIndex ++) {
    nsCOMPtr<nsNavHistoryQuery> query = do_QueryInterface(aQueries[queryIndex]);
    if (queryIndex > 0) {
      AppendAmpersandIfNonempty(queryString);
      queryString += NS_LITERAL_CSTRING(QUERYKEY_SEPARATOR);
    }

    bool hasIt;

    
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

    
    int32_t minVisits;
    if (NS_SUCCEEDED(query->GetMinVisits(&minVisits)) && minVisits >= 0) {
      AppendAmpersandIfNonempty(queryString);
      queryString.AppendLiteral(QUERYKEY_MIN_VISITS "=");
      AppendInt32(queryString, minVisits);
    }

    int32_t maxVisits;
    if (NS_SUCCEEDED(query->GetMaxVisits(&maxVisits)) && maxVisits >= 0) {
      AppendAmpersandIfNonempty(queryString);
      queryString.AppendLiteral(QUERYKEY_MAX_VISITS "=");
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
      nsAutoCString domain;
      nsresult rv = query->GetDomain(domain);
      NS_ENSURE_SUCCESS(rv, rv);
      nsCString escapedDomain;
      bool success = NS_Escape(domain, escapedDomain, url_XAlphas);
      NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

      AppendAmpersandIfNonempty(queryString);
      queryString.AppendLiteral(QUERYKEY_DOMAIN "=");
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
      nsAutoCString uriSpec;
      nsresult rv = uri->GetSpec(uriSpec);
      NS_ENSURE_SUCCESS(rv, rv);
      nsAutoCString escaped;
      bool success = NS_Escape(uriSpec, escaped, url_XAlphas);
      NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

      AppendAmpersandIfNonempty(queryString);
      queryString.AppendLiteral(QUERYKEY_URI "=");
      queryString.Append(escaped);
    }

    
    query->GetHasAnnotation(&hasIt);
    if (hasIt) {
      AppendAmpersandIfNonempty(queryString);
      bool annotationIsNot;
      query->GetAnnotationIsNot(&annotationIsNot);
      if (annotationIsNot)
        queryString.AppendLiteral(QUERYKEY_NOTANNOTATION "=");
      else
        queryString.AppendLiteral(QUERYKEY_ANNOTATION "=");
      nsAutoCString annot;
      query->GetAnnotation(annot);
      nsAutoCString escaped;
      bool success = NS_Escape(annot, escaped, url_XAlphas);
      NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
      queryString.Append(escaped);
    }

    
    int64_t *folders = nullptr;
    uint32_t folderCount = 0;
    query->GetFolders(&folderCount, &folders);
    for (uint32_t i = 0; i < folderCount; ++i) {
      AppendAmpersandIfNonempty(queryString);
      queryString += NS_LITERAL_CSTRING(QUERYKEY_FOLDER "=");
      nsresult rv = PlacesFolderConversion::AppendFolder(queryString, folders[i]);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    free(folders);

    
    const nsTArray<nsString> &tags = query->Tags();
    for (uint32_t i = 0; i < tags.Length(); ++i) {
      nsAutoCString escapedTag;
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
 
    
    const nsTArray<uint32_t>& transitions = query->Transitions();
    for (uint32_t i = 0; i < transitions.Length(); ++i) {
      AppendAmpersandIfNonempty(queryString);
      queryString += NS_LITERAL_CSTRING(QUERYKEY_TRANSITION "=");
      AppendInt64(queryString, transitions[i]);
    }
  }

  
  if (options->SortingMode() != nsINavHistoryQueryOptions::SORT_BY_NONE) {
    AppendAmpersandIfNonempty(queryString);
    queryString += NS_LITERAL_CSTRING(QUERYKEY_SORT "=");
    AppendInt16(queryString, options->SortingMode());
    if (options->SortingMode() == nsINavHistoryQueryOptions::SORT_BY_ANNOTATION_DESCENDING ||
        options->SortingMode() == nsINavHistoryQueryOptions::SORT_BY_ANNOTATION_ASCENDING) {
      
      nsAutoCString sortingAnnotation;
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

  
  if (!options->ExpandQueries()) {
    AppendAmpersandIfNonempty(queryString);
    queryString += NS_LITERAL_CSTRING(QUERYKEY_EXPAND_QUERIES "=0");
  }

  
  if (options->IncludeHidden()) {
    AppendAmpersandIfNonempty(queryString);
    queryString += NS_LITERAL_CSTRING(QUERYKEY_INCLUDE_HIDDEN "=1");
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

  
  if (options->AsyncEnabled()) {
    AppendAmpersandIfNonempty(queryString);
    queryString += NS_LITERAL_CSTRING(QUERYKEY_ASYNC_ENABLED "=1");
  }

  aQueryString.AssignLiteral("place:");
  aQueryString.Append(queryString);
  return NS_OK;
}




nsresult
TokenizeQueryString(const nsACString& aQuery,
                    nsTArray<QueryKeyValuePair>* aTokens)
{
  
  const uint32_t prefixlen = 6; 
  nsCString query;
  if (aQuery.Length() >= prefixlen &&
      Substring(aQuery, 0, prefixlen).EqualsLiteral("place:"))
    query = Substring(aQuery, prefixlen);
  else
    query = aQuery;

  int32_t keyFirstIndex = 0;
  int32_t equalsIndex = 0;
  for (uint32_t i = 0; i < query.Length(); i ++) {
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

  nsTArray<int64_t> folders;
  nsTArray<nsString> tags;
  nsTArray<uint32_t> transitions;
  for (uint32_t i = 0; i < aTokens.Length(); i ++) {
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
      int32_t visits = kvp.value.ToInteger(&rv);
      if (NS_SUCCEEDED(rv))
        query->SetMinVisits(visits);
      else
        NS_WARNING("Bad number for minVisits in query");

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_MAX_VISITS)) {
      int32_t visits = kvp.value.ToInteger(&rv);
      if (NS_SUCCEEDED(rv))
        query->SetMaxVisits(visits);
      else
        NS_WARNING("Bad number for maxVisits in query");

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_ONLY_BOOKMARKED)) {
      SetQueryKeyBool(kvp.value, query, &nsINavHistoryQuery::SetOnlyBookmarked);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_DOMAIN_IS_HOST)) {
      SetQueryKeyBool(kvp.value, query, &nsINavHistoryQuery::SetDomainIsHost);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_DOMAIN)) {
      nsAutoCString unescapedDomain(kvp.value);
      NS_UnescapeURL(unescapedDomain); 
      rv = query->SetDomain(unescapedDomain);
      NS_ENSURE_SUCCESS(rv, rv);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_FOLDER)) {
      int64_t folder;
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
      nsAutoCString unescapedUri(kvp.value);
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
      nsAutoCString unescaped(kvp.value);
      NS_UnescapeURL(unescaped); 
      query->SetAnnotationIsNot(true);
      query->SetAnnotation(unescaped);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_ANNOTATION)) {
      nsAutoCString unescaped(kvp.value);
      NS_UnescapeURL(unescaped); 
      query->SetAnnotationIsNot(false);
      query->SetAnnotation(unescaped);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_TAG)) {
      nsAutoCString unescaped(kvp.value);
      NS_UnescapeURL(unescaped); 
      NS_ConvertUTF8toUTF16 tag(unescaped);
      if (!tags.Contains(tag)) {
        NS_ENSURE_TRUE(tags.AppendElement(tag), NS_ERROR_OUT_OF_MEMORY);
      }

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_NOTTAGS)) {
      SetQueryKeyBool(kvp.value, query, &nsINavHistoryQuery::SetTagsAreNot);

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_TRANSITION)) {
      uint32_t transition = kvp.value.ToInteger(&rv);
      if (NS_SUCCEEDED(rv)) {
        if (!transitions.Contains(transition))
          NS_ENSURE_TRUE(transitions.AppendElement(transition),
                         NS_ERROR_OUT_OF_MEMORY);
      }
      else {
        NS_WARNING("Invalid Int32 transition value.");
      }

    
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

      if (transitions.Length() > 0) {
        rv = query->SetTransitions(transitions);
        NS_ENSURE_SUCCESS(rv, rv);
        transitions.Clear();
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

    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_EXPAND_QUERIES)) {
      SetOptionsKeyBool(kvp.value, aOptions,
                        &nsINavHistoryQueryOptions::SetExpandQueries);
    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_INCLUDE_HIDDEN)) {
      SetOptionsKeyBool(kvp.value, aOptions,
                        &nsINavHistoryQueryOptions::SetIncludeHidden);
    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_MAX_RESULTS)) {
      SetOptionsKeyUint32(kvp.value, aOptions,
                          &nsINavHistoryQueryOptions::SetMaxResults);
    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_QUERY_TYPE)) {
      SetOptionsKeyUint16(kvp.value, aOptions,
                          &nsINavHistoryQueryOptions::SetQueryType);
    
    } else if (kvp.key.EqualsLiteral(QUERYKEY_ASYNC_ENABLED)) {
      SetOptionsKeyBool(kvp.value, aOptions,
                        &nsINavHistoryQueryOptions::SetAsyncEnabled);
    
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

  if (transitions.Length() > 0) {
    rv = query->SetTransitions(transitions);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}






nsresult
ParseQueryBooleanString(const nsCString& aString, bool* aValue)
{
  if (aString.EqualsLiteral("1") || aString.EqualsLiteral("true")) {
    *aValue = true;
    return NS_OK;
  } else if (aString.EqualsLiteral("0") || aString.EqualsLiteral("false")) {
    *aValue = false;
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}




NS_IMPL_ISUPPORTS(nsNavHistoryQuery, nsNavHistoryQuery, nsINavHistoryQuery)







nsNavHistoryQuery::nsNavHistoryQuery()
  : mMinVisits(-1), mMaxVisits(-1), mBeginTime(0),
    mBeginTimeReference(TIME_RELATIVE_EPOCH),
    mEndTime(0), mEndTimeReference(TIME_RELATIVE_EPOCH),
    mOnlyBookmarked(false),
    mDomainIsHost(false), mUriIsPrefix(false),
    mAnnotationIsNot(false),
    mTagsAreNot(false)
{
  
  mDomain.SetIsVoid(true);
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


NS_IMETHODIMP nsNavHistoryQuery::GetBeginTimeReference(uint32_t* _retval)
{
  *_retval = mBeginTimeReference;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetBeginTimeReference(uint32_t aReference)
{
  if (aReference > TIME_RELATIVE_NOW)
    return NS_ERROR_INVALID_ARG;
  mBeginTimeReference = aReference;
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetHasBeginTime(bool* _retval)
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


NS_IMETHODIMP nsNavHistoryQuery::GetEndTimeReference(uint32_t* _retval)
{
  *_retval = mEndTimeReference;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetEndTimeReference(uint32_t aReference)
{
  if (aReference > TIME_RELATIVE_NOW)
    return NS_ERROR_INVALID_ARG;
  mEndTimeReference = aReference;
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetHasEndTime(bool* _retval)
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
NS_IMETHODIMP nsNavHistoryQuery::GetHasSearchTerms(bool* _retval)
{
  *_retval = (! mSearchTerms.IsEmpty());
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetMinVisits(int32_t* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = mMinVisits;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetMinVisits(int32_t aVisits)
{
  mMinVisits = aVisits;
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetMaxVisits(int32_t* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = mMaxVisits;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetMaxVisits(int32_t aVisits)
{
  mMaxVisits = aVisits;
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetOnlyBookmarked(bool *aOnlyBookmarked)
{
  *aOnlyBookmarked = mOnlyBookmarked;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetOnlyBookmarked(bool aOnlyBookmarked)
{
  mOnlyBookmarked = aOnlyBookmarked;
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetDomainIsHost(bool *aDomainIsHost)
{
  *aDomainIsHost = mDomainIsHost;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetDomainIsHost(bool aDomainIsHost)
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
NS_IMETHODIMP nsNavHistoryQuery::GetHasDomain(bool* _retval)
{
  
  *_retval = (! mDomain.IsVoid());
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetUriIsPrefix(bool* aIsPrefix)
{
  *aIsPrefix = mUriIsPrefix;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetUriIsPrefix(bool aIsPrefix)
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
NS_IMETHODIMP nsNavHistoryQuery::GetHasUri(bool* aHasUri)
{
  *aHasUri = (mUri != nullptr);
  return NS_OK;
}


NS_IMETHODIMP nsNavHistoryQuery::GetAnnotationIsNot(bool* aIsNot)
{
  *aIsNot = mAnnotationIsNot;
  return NS_OK;
}
NS_IMETHODIMP nsNavHistoryQuery::SetAnnotationIsNot(bool aIsNot)
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
NS_IMETHODIMP nsNavHistoryQuery::GetHasAnnotation(bool* aHasIt)
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

  uint32_t arrayLen = mTags.Length();

  if (arrayLen == 0)
    rv = out->SetAsEmptyArray();
  else {
    
    const char16_t **array = reinterpret_cast<const char16_t **>
                              (NS_Alloc(arrayLen * sizeof(char16_t *)));
    NS_ENSURE_TRUE(array, NS_ERROR_OUT_OF_MEMORY);

    for (uint32_t i = 0; i < arrayLen; ++i) {
      array[i] = mTags[i].get();
    }

    rv = out->SetAsArray(nsIDataType::VTYPE_WCHAR_STR,
                         nullptr,
                         arrayLen,
                         reinterpret_cast<void *>(array));
    NS_Free(array);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  out.forget(aTags);
  return NS_OK;
}

NS_IMETHODIMP nsNavHistoryQuery::SetTags(nsIVariant *aTags)
{
  NS_ENSURE_ARG(aTags);

  uint16_t dataType;
  aTags->GetDataType(&dataType);

  
  if (dataType == nsIDataType::VTYPE_EMPTY_ARRAY) {
    mTags.Clear();
    return NS_OK;
  }

  
  NS_ENSURE_TRUE(dataType == nsIDataType::VTYPE_ARRAY, NS_ERROR_ILLEGAL_VALUE);

  uint16_t eltType;
  nsIID eltIID;
  uint32_t arrayLen;
  void *array;

  
  
  nsresult rv = aTags->GetAsArray(&eltType, &eltIID, &arrayLen, &array);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (eltType != nsIDataType::VTYPE_WCHAR_STR) {
    switch (eltType) {
    case nsIDataType::VTYPE_ID:
    case nsIDataType::VTYPE_CHAR_STR:
      {
        char **charArray = reinterpret_cast<char **>(array);
        for (uint32_t i = 0; i < arrayLen; ++i) {
          if (charArray[i])
            NS_Free(charArray[i]);
        }
      }
      break;
    case nsIDataType::VTYPE_INTERFACE:
    case nsIDataType::VTYPE_INTERFACE_IS:
      {
        nsISupports **supportsArray = reinterpret_cast<nsISupports **>(array);
        for (uint32_t i = 0; i < arrayLen; ++i) {
          NS_IF_RELEASE(supportsArray[i]);
        }
      }
      break;
    
    }
    NS_Free(array);
    return NS_ERROR_ILLEGAL_VALUE;
  }

  char16_t **tags = reinterpret_cast<char16_t **>(array);
  mTags.Clear();

  
  for (uint32_t i = 0; i < arrayLen; ++i) {

    
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


NS_IMETHODIMP nsNavHistoryQuery::GetTagsAreNot(bool *aTagsAreNot)
{
  NS_ENSURE_ARG_POINTER(aTagsAreNot);
  *aTagsAreNot = mTagsAreNot;
  return NS_OK;
}

NS_IMETHODIMP nsNavHistoryQuery::SetTagsAreNot(bool aTagsAreNot)
{
  mTagsAreNot = aTagsAreNot;
  return NS_OK;
}

NS_IMETHODIMP nsNavHistoryQuery::GetFolders(uint32_t *aCount,
                                            int64_t **aFolders)
{
  uint32_t count = mFolders.Length();
  int64_t *folders = nullptr;
  if (count > 0) {
    folders = static_cast<int64_t*>
                         (moz_xmalloc(count * sizeof(int64_t)));
    NS_ENSURE_TRUE(folders, NS_ERROR_OUT_OF_MEMORY);

    for (uint32_t i = 0; i < count; ++i) {
      folders[i] = mFolders[i];
    }
  }
  *aCount = count;
  *aFolders = folders;
  return NS_OK;
}

NS_IMETHODIMP nsNavHistoryQuery::GetFolderCount(uint32_t *aCount)
{
  *aCount = mFolders.Length();
  return NS_OK;
}

NS_IMETHODIMP nsNavHistoryQuery::SetFolders(const int64_t *aFolders,
                                            uint32_t aFolderCount)
{
  if (!mFolders.ReplaceElementsAt(0, mFolders.Length(),
                                  aFolders, aFolderCount)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

NS_IMETHODIMP nsNavHistoryQuery::GetTransitions(uint32_t* aCount,
                                                uint32_t** aTransitions)
{
  uint32_t count = mTransitions.Length();
  uint32_t* transitions = nullptr;
  if (count > 0) {
    transitions = reinterpret_cast<uint32_t*>
                  (NS_Alloc(count * sizeof(uint32_t)));
    NS_ENSURE_TRUE(transitions, NS_ERROR_OUT_OF_MEMORY);
    for (uint32_t i = 0; i < count; ++i) {
      transitions[i] = mTransitions[i];
    }
  }
  *aCount = count;
  *aTransitions = transitions;
  return NS_OK;
}

NS_IMETHODIMP nsNavHistoryQuery::GetTransitionCount(uint32_t* aCount)
{
  *aCount = mTransitions.Length();
  return NS_OK;
}

NS_IMETHODIMP nsNavHistoryQuery::SetTransitions(const uint32_t* aTransitions,
                                                uint32_t aCount)
{
  if (!mTransitions.ReplaceElementsAt(0, mTransitions.Length(), aTransitions,
                                      aCount))
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

NS_IMETHODIMP nsNavHistoryQuery::Clone(nsINavHistoryQuery** _retval)
{
  *_retval = nullptr;

  nsNavHistoryQuery *clone = new nsNavHistoryQuery(*this);
  NS_ENSURE_TRUE(clone, NS_ERROR_OUT_OF_MEMORY);

  clone->mRefCnt = 0; 
  NS_ADDREF(*_retval = clone);
  return NS_OK;
}



NS_IMPL_ISUPPORTS(nsNavHistoryQueryOptions, nsNavHistoryQueryOptions, nsINavHistoryQueryOptions)


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetSortingMode(uint16_t* aMode)
{
  *aMode = mSort;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryOptions::SetSortingMode(uint16_t aMode)
{
  if (aMode > SORT_BY_FRECENCY_DESCENDING)
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
nsNavHistoryQueryOptions::GetResultType(uint16_t* aType)
{
  *aType = mResultType;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryOptions::SetResultType(uint16_t aType)
{
  if (aType > RESULTS_AS_TAG_CONTENTS)
    return NS_ERROR_INVALID_ARG;
  
  
  if (aType == RESULTS_AS_TAG_QUERY || aType == RESULTS_AS_TAG_CONTENTS)
    mQueryType = QUERY_TYPE_BOOKMARKS;
  mResultType = aType;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetExcludeItems(bool* aExclude)
{
  *aExclude = mExcludeItems;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryOptions::SetExcludeItems(bool aExclude)
{
  mExcludeItems = aExclude;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetExcludeQueries(bool* aExclude)
{
  *aExclude = mExcludeQueries;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryOptions::SetExcludeQueries(bool aExclude)
{
  mExcludeQueries = aExclude;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetExcludeReadOnlyFolders(bool* aExclude)
{
  *aExclude = mExcludeReadOnlyFolders;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryOptions::SetExcludeReadOnlyFolders(bool aExclude)
{
  mExcludeReadOnlyFolders = aExclude;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetExpandQueries(bool* aExpand)
{
  *aExpand = mExpandQueries;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryOptions::SetExpandQueries(bool aExpand)
{
  mExpandQueries = aExpand;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetIncludeHidden(bool* aIncludeHidden)
{
  *aIncludeHidden = mIncludeHidden;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryOptions::SetIncludeHidden(bool aIncludeHidden)
{
  mIncludeHidden = aIncludeHidden;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetMaxResults(uint32_t* aMaxResults)
{
  *aMaxResults = mMaxResults;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryOptions::SetMaxResults(uint32_t aMaxResults)
{
  mMaxResults = aMaxResults;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetQueryType(uint16_t* _retval)
{
  *_retval = mQueryType;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryOptions::SetQueryType(uint16_t aQueryType)
{
  
  
  if (mResultType == RESULTS_AS_TAG_CONTENTS ||
      mResultType == RESULTS_AS_TAG_QUERY)
   return NS_OK;
  mQueryType = aQueryType;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryOptions::GetAsyncEnabled(bool* _asyncEnabled)
{
  *_asyncEnabled = mAsyncEnabled;
  return NS_OK;
}
NS_IMETHODIMP
nsNavHistoryQueryOptions::SetAsyncEnabled(bool aAsyncEnabled)
{
  mAsyncEnabled = aAsyncEnabled;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistoryQueryOptions::Clone(nsINavHistoryQueryOptions** aResult)
{
  nsNavHistoryQueryOptions *clone = nullptr;
  nsresult rv = Clone(&clone);
  *aResult = clone;
  return rv;
}

nsresult
nsNavHistoryQueryOptions::Clone(nsNavHistoryQueryOptions **aResult)
{
  *aResult = nullptr;
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
  result->mAsyncEnabled = mAsyncEnabled;

  resultHolder.swap(*aResult);
  return NS_OK;
}




void 
AppendBoolKeyValueIfTrue(nsACString& aString, const nsCString& aName,
                         nsINavHistoryQuery* aQuery,
                         BoolQueryGetter getter)
{
  bool value;
  DebugOnly<nsresult> rv = (aQuery->*getter)(&value);
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
  uint32_t value;
  DebugOnly<nsresult> rv = (aQuery->*getter)(&value);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failure getting value");
  if (value) {
    AppendAmpersandIfNonempty(aString);
    aString += aName;

    
    nsAutoCString appendMe("=");
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
  PRTime value;
  DebugOnly<nsresult> rv = (aQuery->*getter)(&value);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failure getting value");
  if (value) {
    AppendAmpersandIfNonempty(aString);
    aString += aName;
    nsAutoCString appendMe("=");
    appendMe.AppendInt(static_cast<int64_t>(value));
    aString.Append(appendMe);
  }
}




void 
SetQueryKeyBool(const nsCString& aValue, nsINavHistoryQuery* aQuery,
                BoolQuerySetter setter)
{
  bool value;
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
  bool value;
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
  uint32_t value = aValue.ToInteger(&rv);
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
  uint32_t value = aValue.ToInteger(&rv);
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
  uint16_t value = static_cast<uint16_t>(aValue.ToInteger(&rv));
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
  int64_t value;
  if (PR_sscanf(aValue.get(), "%lld", &value) == 1) {
    rv = (aQuery->*setter)(value);
    if (NS_FAILED(rv)) {
      NS_WARNING("Error setting Int64 key value");
    }
  } else {
    NS_WARNING("Invalid Int64 value in query string.");
  }
}
