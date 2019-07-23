





































#include "mozilla/storage.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsWhitespaceTokenizer.h"
#include "nsEscape.h"
#include "mozIPlacesAutoComplete.h"
#include "SQLFunctions.h"

using namespace mozilla::storage;

namespace mozilla {
namespace places {




  
  

  
  nsresult
  MatchAutoCompleteFunction::create(mozIStorageConnection *aDBConn)
  {
    nsRefPtr<MatchAutoCompleteFunction> function(new MatchAutoCompleteFunction);
    NS_ENSURE_TRUE(function, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aDBConn->CreateFunction(
      NS_LITERAL_CSTRING("autocomplete_match"), kArgIndexLength, function
    );
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  
  void
  MatchAutoCompleteFunction::fixupURISpec(const nsCString &aURISpec,
                                          nsString &_fixedSpec)
  {
    nsCString unescapedSpec;
    (void)NS_UnescapeURL(aURISpec, esc_SkipControl | esc_AlwaysCopy,
                         unescapedSpec);

    
    
    NS_ASSERTION(_fixedSpec.IsEmpty(),
                 "Passing a non-empty string as an out parameter!");
    if (IsUTF8(unescapedSpec))
      CopyUTF8toUTF16(unescapedSpec, _fixedSpec);
    else
      CopyUTF8toUTF16(aURISpec, _fixedSpec);

    if (StringBeginsWith(_fixedSpec, NS_LITERAL_STRING("http://")))
      _fixedSpec.Cut(0, 7);
    else if (StringBeginsWith(_fixedSpec, NS_LITERAL_STRING("https://")))
      _fixedSpec.Cut(0, 8);
    else if (StringBeginsWith(_fixedSpec, NS_LITERAL_STRING("ftp://")))
      _fixedSpec.Cut(0, 6);
  }

  
  bool
  MatchAutoCompleteFunction::findAnywhere(const nsDependentSubstring &aToken,
                                          const nsAString &aSourceString)
  {
    return !!CaseInsensitiveFindInReadable(aToken, aSourceString);
  }

  
  bool
  MatchAutoCompleteFunction::findBeginning(const nsDependentSubstring &aToken,
                                           const nsAString &aSourceString)
  {
    return !!StringBeginsWith(aSourceString, aToken,
                              nsCaseInsensitiveStringComparator());
  }

  
  bool
  MatchAutoCompleteFunction::findOnBoundary(const nsDependentSubstring &aToken,
                                            const nsAString &aSourceString)
  {
    
    if (aSourceString.IsEmpty())
      return false;

    
    const nsCaseInsensitiveStringComparator caseInsensitiveCompare;

    const_wchar_iterator tokenStart(aToken.BeginReading()),
                         tokenEnd(aToken.EndReading()),
                         sourceStart(aSourceString.BeginReading()),
                         sourceEnd(aSourceString.EndReading());

    
    do {
      
      const_wchar_iterator testTokenItr(tokenStart),
                           testSourceItr(sourceStart);

      
      while (!caseInsensitiveCompare(*testTokenItr, *testSourceItr)) {
        
        testTokenItr++;
        testSourceItr++;

        
        if (testTokenItr == tokenEnd)
          return true;

        
        
        if (testSourceItr == sourceEnd)
          return false;
      }

      
      
      if (!isWordBoundary(ToLowerCase(*sourceStart++)))
        sourceStart = nextWordBoundary(sourceStart, sourceEnd);
    } while (sourceStart != sourceEnd);

    return false;
  }

  
  MatchAutoCompleteFunction::const_wchar_iterator
  MatchAutoCompleteFunction::nextWordBoundary(const_wchar_iterator aStart,
                                              const_wchar_iterator aEnd)
  {
    while (aStart != aEnd && !isWordBoundary(*aStart))
      aStart++;
    return aStart;
  }

  
  bool
  MatchAutoCompleteFunction::isWordBoundary(const PRUnichar &aChar)
  {
    
    
    
    return !(PRUnichar('a') <= aChar && aChar <= PRUnichar('z'));
  }

  
  MatchAutoCompleteFunction::searchFunctionPtr
  MatchAutoCompleteFunction::getSearchFunction(PRInt32 aBehavior)
  {
    switch (aBehavior) {
      case mozIPlacesAutoComplete::MATCH_ANYWHERE:
        return findAnywhere;
      case mozIPlacesAutoComplete::MATCH_BEGINNING:
        return findBeginning;
      case mozIPlacesAutoComplete::MATCH_BOUNDARY:
      default:
        return findOnBoundary;
    };
  }

  NS_IMPL_THREADSAFE_ISUPPORTS1(
    MatchAutoCompleteFunction,
    mozIStorageFunction
  )

  
  

  NS_IMETHODIMP
  MatchAutoCompleteFunction::OnFunctionCall(mozIStorageValueArray *aArguments,
                                            nsIVariant **_result)
  {
    
    
    PRInt32 searchBehavior = aArguments->AsInt32(kArgIndexSearchBehavior);
    #define HAS_BEHAVIOR(aBitName) \
      (searchBehavior & mozIPlacesAutoComplete::BEHAVIOR_##aBitName)

    nsAutoString searchString;
    (void)aArguments->GetString(kArgSearchString, searchString);
    nsCString url;
    (void)aArguments->GetUTF8String(kArgIndexURL, url);

    
    
    if (!HAS_BEHAVIOR(JAVASCRIPT) &&
        !StringBeginsWith(searchString, NS_LITERAL_STRING("javascript:")) &&
        StringBeginsWith(url, NS_LITERAL_CSTRING("javascript:"))) {
      NS_IF_ADDREF(*_result = new IntegerVariant(0));
      NS_ENSURE_TRUE(*_result, NS_ERROR_OUT_OF_MEMORY);
      return NS_OK;
    }

    PRInt32 visitCount = aArguments->AsInt32(kArgIndexVisitCount);
    bool typed = aArguments->AsInt32(kArgIndexTyped) ? true : false;
    bool bookmark = aArguments->AsInt32(kArgIndexBookmark) ? true : false;
    nsAutoString tags;
    (void)aArguments->GetString(kArgIndexTags, tags);

    
    
    bool matches = !(
      (HAS_BEHAVIOR(HISTORY) && visitCount == 0) ||
      (HAS_BEHAVIOR(TYPED) && !typed) ||
      (HAS_BEHAVIOR(BOOKMARK) && !bookmark) ||
      (HAS_BEHAVIOR(TAG) && tags.IsVoid())
    );
    if (!matches) {
      NS_IF_ADDREF(*_result = new IntegerVariant(0));
      NS_ENSURE_TRUE(*_result, NS_ERROR_OUT_OF_MEMORY);
      return NS_OK;
    }

    
    nsString fixedURI;
    fixupURISpec(url, fixedURI);

    
    PRInt32 matchBehavior = aArguments->AsInt32(kArgIndexMatchBehavior);
    searchFunctionPtr searchFunction = getSearchFunction(matchBehavior);

    nsAutoString title;
    (void)aArguments->GetString(kArgIndexTitle, title);

    
    
    nsWhitespaceTokenizer tokenizer(searchString);
    while (matches && tokenizer.hasMoreTokens()) {
      const nsDependentSubstring &token = tokenizer.nextToken();

      bool matchTags = searchFunction(token, tags);
      bool matchTitle = searchFunction(token, title);

      
      matches = matchTags || matchTitle;
      if (HAS_BEHAVIOR(TITLE) && !matches)
        break;

      bool matchURL = searchFunction(token, fixedURI);
      
      
      if (HAS_BEHAVIOR(URL) && !matchURL)
        matches = false;
      else
        matches = matches || matchURL;
    }

    NS_IF_ADDREF(*_result = new IntegerVariant(matches ? 1 : 0));
    NS_ENSURE_TRUE(*_result, NS_ERROR_OUT_OF_MEMORY);
    return NS_OK;
    #undef HAS_BEHAVIOR
  }

} 
} 
