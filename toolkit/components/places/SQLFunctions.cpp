




#include "mozilla/storage.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsWhitespaceTokenizer.h"
#include "nsEscape.h"
#include "mozIPlacesAutoComplete.h"
#include "SQLFunctions.h"
#include "nsMathUtils.h"
#include "nsUTF8Utils.h"
#include "nsINavHistoryService.h"
#include "nsPrintfCString.h"
#include "nsNavHistory.h"
#include "mozilla/Likely.h"

using namespace mozilla::storage;






namespace {

  typedef nsACString::const_char_iterator const_char_iterator;

  













  static
  MOZ_ALWAYS_INLINE const_char_iterator
  nextWordBoundary(const_char_iterator const aStart,
                   const_char_iterator const aNext,
                   const_char_iterator const aEnd) {

    const_char_iterator cur = aStart;
    if (('a' <= *cur && *cur <= 'z') ||
        ('A' <= *cur && *cur <= 'Z')) {

      
      
      
      do {
        cur++;
      } while (cur < aEnd && 'a' <= *cur && *cur <= 'z');
    }
    else {
      cur = aNext;
    }

    return cur;
  }

  enum FindInStringBehavior {
    eFindOnBoundary,
    eFindAnywhere
  };

  

















  static
  MOZ_ALWAYS_INLINE bool
  findInString(const nsDependentCSubstring &aToken,
               const nsACString &aSourceString,
               FindInStringBehavior aBehavior)
  {
    
    
    NS_PRECONDITION(!aToken.IsEmpty(), "Don't search for an empty token!");

    
    if (aSourceString.IsEmpty()) {
      return false;
    }

    const_char_iterator tokenStart(aToken.BeginReading()),
                        tokenEnd(aToken.EndReading()),
                        sourceStart(aSourceString.BeginReading()),
                        sourceEnd(aSourceString.EndReading());

    do {
      
      

      
      
      
      const_char_iterator sourceNext, tokenCur;
      bool error;
      if (CaseInsensitiveUTF8CharsEqual(sourceStart, tokenStart,
                                        sourceEnd, tokenEnd,
                                        &sourceNext, &tokenCur, &error)) {

        
        
        

        const_char_iterator sourceCur = sourceNext;
        while (true) {
          if (tokenCur >= tokenEnd) {
            
            return true;
          }

          if (sourceCur >= sourceEnd) {
            
            
            return false;
          }

          if (!CaseInsensitiveUTF8CharsEqual(sourceCur, tokenCur,
                                             sourceEnd, tokenEnd,
                                             &sourceCur, &tokenCur, &error)) {
            
            
            break;
          }
        }
      }

      
      if (MOZ_UNLIKELY(error)) {
        return false;
      }

      
      
      

      if (aBehavior == eFindOnBoundary) {
        sourceStart = nextWordBoundary(sourceStart, sourceNext, sourceEnd);
      }
      else {
        sourceStart = sourceNext;
      }

    } while (sourceStart < sourceEnd);

    return false;
  }

} 

namespace mozilla {
namespace places {




  
  

  MatchAutoCompleteFunction::~MatchAutoCompleteFunction()
  {
  }

  
  nsresult
  MatchAutoCompleteFunction::create(mozIStorageConnection *aDBConn)
  {
    nsRefPtr<MatchAutoCompleteFunction> function =
      new MatchAutoCompleteFunction();

    nsresult rv = aDBConn->CreateFunction(
      NS_LITERAL_CSTRING("autocomplete_match"), kArgIndexLength, function
    );
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  
  void
  MatchAutoCompleteFunction::fixupURISpec(const nsCString &aURISpec,
                                          int32_t aMatchBehavior,
                                          nsCString &_fixedSpec)
  {
    nsCString unescapedSpec;
    (void)NS_UnescapeURL(aURISpec, esc_SkipControl | esc_AlwaysCopy,
                         unescapedSpec);

    
    
    NS_ASSERTION(_fixedSpec.IsEmpty(),
                 "Passing a non-empty string as an out parameter!");
    if (IsUTF8(unescapedSpec))
      _fixedSpec.Assign(unescapedSpec);
    else
      _fixedSpec.Assign(aURISpec);

    if (aMatchBehavior == mozIPlacesAutoComplete::MATCH_ANYWHERE_UNMODIFIED)
      return;

    if (StringBeginsWith(_fixedSpec, NS_LITERAL_CSTRING("http://")))
      _fixedSpec.Cut(0, 7);
    else if (StringBeginsWith(_fixedSpec, NS_LITERAL_CSTRING("https://")))
      _fixedSpec.Cut(0, 8);
    else if (StringBeginsWith(_fixedSpec, NS_LITERAL_CSTRING("ftp://")))
      _fixedSpec.Cut(0, 6);

    if (StringBeginsWith(_fixedSpec, NS_LITERAL_CSTRING("www.")))
      _fixedSpec.Cut(0, 4);
  }

  
  bool
  MatchAutoCompleteFunction::findAnywhere(const nsDependentCSubstring &aToken,
                                          const nsACString &aSourceString)
  {
    

    return findInString(aToken, aSourceString, eFindAnywhere);
  }

  
  bool
  MatchAutoCompleteFunction::findOnBoundary(const nsDependentCSubstring &aToken,
                                            const nsACString &aSourceString)
  {
    return findInString(aToken, aSourceString, eFindOnBoundary);
  }

  
  bool
  MatchAutoCompleteFunction::findBeginning(const nsDependentCSubstring &aToken,
                                           const nsACString &aSourceString)
  {
    NS_PRECONDITION(!aToken.IsEmpty(), "Don't search for an empty token!");

    
    
    
    
    
    

    const_char_iterator tokenStart(aToken.BeginReading()),
                        tokenEnd(aToken.EndReading()),
                        sourceStart(aSourceString.BeginReading()),
                        sourceEnd(aSourceString.EndReading());

    bool dummy;
    while (sourceStart < sourceEnd &&
           CaseInsensitiveUTF8CharsEqual(sourceStart, tokenStart,
                                         sourceEnd, tokenEnd,
                                         &sourceStart, &tokenStart, &dummy)) {

      
      if (tokenStart >= tokenEnd) {
        return true;
      }
    }

    
    
    

    return false;
  }

  
  bool
  MatchAutoCompleteFunction::findBeginningCaseSensitive(
    const nsDependentCSubstring &aToken,
    const nsACString &aSourceString)
  {
    NS_PRECONDITION(!aToken.IsEmpty(), "Don't search for an empty token!");

    return StringBeginsWith(aSourceString, aToken);
  }

  
  MatchAutoCompleteFunction::searchFunctionPtr
  MatchAutoCompleteFunction::getSearchFunction(int32_t aBehavior)
  {
    switch (aBehavior) {
      case mozIPlacesAutoComplete::MATCH_ANYWHERE:
      case mozIPlacesAutoComplete::MATCH_ANYWHERE_UNMODIFIED:
        return findAnywhere;
      case mozIPlacesAutoComplete::MATCH_BEGINNING:
        return findBeginning;
      case mozIPlacesAutoComplete::MATCH_BEGINNING_CASE_SENSITIVE:
        return findBeginningCaseSensitive;
      case mozIPlacesAutoComplete::MATCH_BOUNDARY:
      default:
        return findOnBoundary;
    };
  }

  NS_IMPL_ISUPPORTS(
    MatchAutoCompleteFunction,
    mozIStorageFunction
  )

  
  

  NS_IMETHODIMP
  MatchAutoCompleteFunction::OnFunctionCall(mozIStorageValueArray *aArguments,
                                            nsIVariant **_result)
  {
    
    
    int32_t searchBehavior = aArguments->AsInt32(kArgIndexSearchBehavior);
    #define HAS_BEHAVIOR(aBitName) \
      (searchBehavior & mozIPlacesAutoComplete::BEHAVIOR_##aBitName)

    nsAutoCString searchString;
    (void)aArguments->GetUTF8String(kArgSearchString, searchString);
    nsCString url;
    (void)aArguments->GetUTF8String(kArgIndexURL, url);

    int32_t matchBehavior = aArguments->AsInt32(kArgIndexMatchBehavior);

    
    
    if (matchBehavior != mozIPlacesAutoComplete::MATCH_ANYWHERE_UNMODIFIED &&
        !HAS_BEHAVIOR(JAVASCRIPT) &&
        !StringBeginsWith(searchString, NS_LITERAL_CSTRING("javascript:")) &&
        StringBeginsWith(url, NS_LITERAL_CSTRING("javascript:"))) {
      NS_ADDREF(*_result = new IntegerVariant(0));
      return NS_OK;
    }

    int32_t visitCount = aArguments->AsInt32(kArgIndexVisitCount);
    bool typed = aArguments->AsInt32(kArgIndexTyped) ? true : false;
    bool bookmark = aArguments->AsInt32(kArgIndexBookmark) ? true : false;
    nsAutoCString tags;
    (void)aArguments->GetUTF8String(kArgIndexTags, tags);
    int32_t openPageCount = aArguments->AsInt32(kArgIndexOpenPageCount);
    bool matches = false;
    if (HAS_BEHAVIOR(RESTRICT)) {
      
      
      matches = (!HAS_BEHAVIOR(HISTORY) || visitCount > 0) &&
                (!HAS_BEHAVIOR(TYPED) || typed) &&
                (!HAS_BEHAVIOR(BOOKMARK) || bookmark) &&
                (!HAS_BEHAVIOR(TAG) || !tags.IsVoid()) &&
                (!HAS_BEHAVIOR(OPENPAGE) || openPageCount > 0);
    } else {
      
      
      matches = (HAS_BEHAVIOR(HISTORY) && visitCount > 0) ||
                (HAS_BEHAVIOR(TYPED) && typed) ||
                (HAS_BEHAVIOR(BOOKMARK) && bookmark) ||
                (HAS_BEHAVIOR(TAG) && !tags.IsVoid()) ||
                (HAS_BEHAVIOR(OPENPAGE) && openPageCount > 0);
    }

    if (!matches) {
      NS_ADDREF(*_result = new IntegerVariant(0));
      return NS_OK;
    }

    
    searchFunctionPtr searchFunction = getSearchFunction(matchBehavior);

    
    nsCString fixedURI;
    fixupURISpec(url, matchBehavior, fixedURI);

    nsAutoCString title;
    (void)aArguments->GetUTF8String(kArgIndexTitle, title);

    
    
    nsCWhitespaceTokenizer tokenizer(searchString);
    while (matches && tokenizer.hasMoreTokens()) {
      const nsDependentCSubstring &token = tokenizer.nextToken();

      if (HAS_BEHAVIOR(TITLE) && HAS_BEHAVIOR(URL)) {
        matches = (searchFunction(token, title) || searchFunction(token, tags)) &&
                  searchFunction(token, fixedURI);
      }
      else if (HAS_BEHAVIOR(TITLE)) {
        matches = searchFunction(token, title) || searchFunction(token, tags);
      }
      else if (HAS_BEHAVIOR(URL)) {
        matches = searchFunction(token, fixedURI);
      }
      else {
        matches = searchFunction(token, title) ||
                  searchFunction(token, tags) ||
                  searchFunction(token, fixedURI);
      }
    }

    NS_ADDREF(*_result = new IntegerVariant(matches ? 1 : 0));
    return NS_OK;
    #undef HAS_BEHAVIOR
  }





  
  

  CalculateFrecencyFunction::~CalculateFrecencyFunction()
  {
  }

  
  nsresult
  CalculateFrecencyFunction::create(mozIStorageConnection *aDBConn)
  {
    nsRefPtr<CalculateFrecencyFunction> function =
      new CalculateFrecencyFunction();

    nsresult rv = aDBConn->CreateFunction(
      NS_LITERAL_CSTRING("calculate_frecency"), 1, function
    );
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  NS_IMPL_ISUPPORTS(
    CalculateFrecencyFunction,
    mozIStorageFunction
  )

  
  

  NS_IMETHODIMP
  CalculateFrecencyFunction::OnFunctionCall(mozIStorageValueArray *aArguments,
                                            nsIVariant **_result)
  {
    
    uint32_t numEntries;
    nsresult rv = aArguments->GetNumEntries(&numEntries);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ASSERTION(numEntries > 0, "unexpected number of arguments");

    int64_t pageId = aArguments->AsInt64(0);
    int32_t typed = numEntries > 1 ? aArguments->AsInt32(1) : 0;
    int32_t fullVisitCount = numEntries > 2 ? aArguments->AsInt32(2) : 0;
    int64_t bookmarkId = numEntries > 3 ? aArguments->AsInt64(3) : 0;
    int32_t visitCount = 0;
    int32_t hidden = 0;
    int32_t isQuery = 0;
    float pointsForSampledVisits = 0.0;

    
    const nsNavHistory* history = nsNavHistory::GetConstHistoryService();
    NS_ENSURE_STATE(history);
    nsRefPtr<Database> DB = Database::GetDatabase();
    NS_ENSURE_STATE(DB);

    if (pageId > 0) {
      
      
      nsRefPtr<mozIStorageStatement> getPageInfo = DB->GetStatement(
        "SELECT typed, hidden, visit_count, "
          "(SELECT count(*) FROM moz_historyvisits WHERE place_id = :page_id), "
          "EXISTS (SELECT 1 FROM moz_bookmarks WHERE fk = :page_id), "
          "(url > 'place:' AND url < 'place;') "
        "FROM moz_places "
        "WHERE id = :page_id "
      );
      NS_ENSURE_STATE(getPageInfo);
      mozStorageStatementScoper infoScoper(getPageInfo);

      rv = getPageInfo->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), pageId);
      NS_ENSURE_SUCCESS(rv, rv);

      bool hasResult;
      rv = getPageInfo->ExecuteStep(&hasResult);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_TRUE(hasResult, NS_ERROR_UNEXPECTED);
      rv = getPageInfo->GetInt32(0, &typed);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = getPageInfo->GetInt32(1, &hidden);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = getPageInfo->GetInt32(2, &visitCount);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = getPageInfo->GetInt32(3, &fullVisitCount);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = getPageInfo->GetInt64(4, &bookmarkId);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = getPageInfo->GetInt32(5, &isQuery);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      
      
      
      
      nsCOMPtr<mozIStorageStatement> getVisits = DB->GetStatement(
        NS_LITERAL_CSTRING(
          "/* do not warn (bug 659740 - SQLite may ignore index if few visits exist) */"
          "SELECT "
            "ROUND((strftime('%s','now','localtime','utc') - v.visit_date/1000000)/86400), "
            "IFNULL(r.visit_type, v.visit_type), "
            "v.visit_date "
          "FROM moz_historyvisits v "
          "LEFT JOIN moz_historyvisits r ON r.id = v.from_visit AND v.visit_type BETWEEN "
        ) + nsPrintfCString("%d AND %d ", nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT,
                                          nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY) +
        NS_LITERAL_CSTRING(
          "WHERE v.place_id = :page_id "
          "ORDER BY v.visit_date DESC "
        )
      );
      NS_ENSURE_STATE(getVisits);
      mozStorageStatementScoper visitsScoper(getVisits);

      rv = getVisits->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), pageId);
      NS_ENSURE_SUCCESS(rv, rv);

      
      int32_t numSampledVisits = 0;
      for (int32_t maxVisits = history->GetNumVisitsForFrecency();
           numSampledVisits < maxVisits &&
           NS_SUCCEEDED(getVisits->ExecuteStep(&hasResult)) && hasResult;
           numSampledVisits++) {
        int32_t visitType;
        rv = getVisits->GetInt32(1, &visitType);
        NS_ENSURE_SUCCESS(rv, rv);
        int32_t bonus = history->GetFrecencyTransitionBonus(visitType, true);

        
        if (bookmarkId) {
          bonus += history->GetFrecencyTransitionBonus(nsINavHistoryService::TRANSITION_BOOKMARK, true);
        }

        
        if (bonus) {
          int32_t ageInDays = getVisits->AsInt32(0);
          int32_t weight = history->GetFrecencyAgedWeight(ageInDays);
          pointsForSampledVisits += (float)(weight * (bonus / 100.0));
        }
      }

      
      if (numSampledVisits) {
        
        if (!pointsForSampledVisits) {
          
          
          
          NS_ADDREF(*_result = new IntegerVariant(-visitCount));
        }
        else {
          
          
          
          NS_ADDREF(*_result = new IntegerVariant((int32_t) ceilf(fullVisitCount * ceilf(pointsForSampledVisits) / numSampledVisits)));
        }

        return NS_OK;
      }
    }

    
    

    
    
    
    
    
    int32_t bonus = 0;

    
    
    if (bookmarkId && !isQuery) {
      bonus += history->GetFrecencyTransitionBonus(nsINavHistoryService::TRANSITION_BOOKMARK, false);;
      
      
      fullVisitCount = 1;
    }

    if (typed) {
      bonus += history->GetFrecencyTransitionBonus(nsINavHistoryService::TRANSITION_TYPED, false);
    }

    
    pointsForSampledVisits = history->GetFrecencyBucketWeight(1) * (bonus / (float)100.0); 

    
    
    NS_ADDREF(*_result = new IntegerVariant((int32_t) ceilf(fullVisitCount * ceilf(pointsForSampledVisits))));

    return NS_OK;
  }




  GenerateGUIDFunction::~GenerateGUIDFunction()
  {
  }

  
  

  
  nsresult
  GenerateGUIDFunction::create(mozIStorageConnection *aDBConn)
  {
    nsRefPtr<GenerateGUIDFunction> function = new GenerateGUIDFunction();
    nsresult rv = aDBConn->CreateFunction(
      NS_LITERAL_CSTRING("generate_guid"), 0, function
    );
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  NS_IMPL_ISUPPORTS(
    GenerateGUIDFunction,
    mozIStorageFunction
  )

  
  

  NS_IMETHODIMP
  GenerateGUIDFunction::OnFunctionCall(mozIStorageValueArray *aArguments,
                                       nsIVariant **_result)
  {
    nsAutoCString guid;
    nsresult rv = GenerateGUID(guid);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ADDREF(*_result = new UTF8TextVariant(guid));
    return NS_OK;
  }




  GetUnreversedHostFunction::~GetUnreversedHostFunction()
  {
  }

  
  

  
  nsresult
  GetUnreversedHostFunction::create(mozIStorageConnection *aDBConn)
  {
    nsRefPtr<GetUnreversedHostFunction> function = new GetUnreversedHostFunction();
    nsresult rv = aDBConn->CreateFunction(
      NS_LITERAL_CSTRING("get_unreversed_host"), 1, function
    );
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  NS_IMPL_ISUPPORTS(
    GetUnreversedHostFunction,
    mozIStorageFunction
  )

  
  

  NS_IMETHODIMP
  GetUnreversedHostFunction::OnFunctionCall(mozIStorageValueArray *aArguments,
                                            nsIVariant **_result)
  {
    
    MOZ_ASSERT(aArguments);

    nsAutoString src;
    aArguments->GetString(0, src);

    nsCOMPtr<nsIWritableVariant> result =
      do_CreateInstance("@mozilla.org/variant;1");
    NS_ENSURE_STATE(result);

    if (src.Length()>1) {
      src.Truncate(src.Length() - 1);
      nsAutoString dest;
      ReverseString(src, dest);
      result->SetAsAString(dest);
    }
    else {
      result->SetAsAString(EmptyString());
    }
    result.forget(_result);
    return NS_OK;
  }




  FixupURLFunction::~FixupURLFunction()
  {
  }

  
  

  
  nsresult
  FixupURLFunction::create(mozIStorageConnection *aDBConn)
  {
    nsRefPtr<FixupURLFunction> function = new FixupURLFunction();
    nsresult rv = aDBConn->CreateFunction(
      NS_LITERAL_CSTRING("fixup_url"), 1, function
    );
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  NS_IMPL_ISUPPORTS(
    FixupURLFunction,
    mozIStorageFunction
  )

  
  

  NS_IMETHODIMP
  FixupURLFunction::OnFunctionCall(mozIStorageValueArray *aArguments,
                                   nsIVariant **_result)
  {
    
    MOZ_ASSERT(aArguments);

    nsAutoString src;
    aArguments->GetString(0, src);

    nsCOMPtr<nsIWritableVariant> result =
      do_CreateInstance("@mozilla.org/variant;1");
    NS_ENSURE_STATE(result);

    
    if (StringBeginsWith(src, NS_LITERAL_STRING("www."))) {
      src.Cut(0, 4);
    }

    result->SetAsAString(src);
    result.forget(_result);
    return NS_OK;
  }




  FrecencyNotificationFunction::~FrecencyNotificationFunction()
  {
  }

  
  nsresult
  FrecencyNotificationFunction::create(mozIStorageConnection *aDBConn)
  {
    nsRefPtr<FrecencyNotificationFunction> function =
      new FrecencyNotificationFunction();
    nsresult rv = aDBConn->CreateFunction(
      NS_LITERAL_CSTRING("notify_frecency"), 5, function
    );
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  NS_IMPL_ISUPPORTS(
    FrecencyNotificationFunction,
    mozIStorageFunction
  )

  NS_IMETHODIMP
  FrecencyNotificationFunction::OnFunctionCall(mozIStorageValueArray *aArgs,
                                               nsIVariant **_result)
  {
    uint32_t numArgs;
    nsresult rv = aArgs->GetNumEntries(&numArgs);
    NS_ENSURE_SUCCESS(rv, rv);
    MOZ_ASSERT(numArgs == 5);

    int32_t newFrecency = aArgs->AsInt32(0);

    nsAutoCString spec;
    rv = aArgs->GetUTF8String(1, spec);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoCString guid;
    rv = aArgs->GetUTF8String(2, guid);
    NS_ENSURE_SUCCESS(rv, rv);

    bool hidden = static_cast<bool>(aArgs->AsInt32(3));
    PRTime lastVisitDate = static_cast<PRTime>(aArgs->AsInt64(4));

    const nsNavHistory* navHistory = nsNavHistory::GetConstHistoryService();
    NS_ENSURE_STATE(navHistory);
    navHistory->DispatchFrecencyChangedNotification(spec, newFrecency, guid,
                                                    hidden, lastVisitDate);

    nsCOMPtr<nsIWritableVariant> result =
      do_CreateInstance("@mozilla.org/variant;1");
    NS_ENSURE_STATE(result);
    rv = result->SetAsInt32(newFrecency);
    NS_ENSURE_SUCCESS(rv, rv);
    result.forget(_result);
    return NS_OK;
  }

} 
} 
