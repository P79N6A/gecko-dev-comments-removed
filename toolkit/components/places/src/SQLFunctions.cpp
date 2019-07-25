





































#include "mozilla/storage.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsWhitespaceTokenizer.h"
#include "nsEscape.h"
#include "mozIPlacesAutoComplete.h"
#include "SQLFunctions.h"
#include "nsUTF8Utils.h"

using namespace mozilla::storage;




namespace {

  typedef nsACString::const_char_iterator const_char_iterator;

  













  static
  NS_ALWAYS_INLINE const_char_iterator
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
  NS_ALWAYS_INLINE bool
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
      PRBool error;
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

      
      if (NS_UNLIKELY(error)) {
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

    PRBool dummy;
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

    nsCAutoString searchString;
    (void)aArguments->GetUTF8String(kArgSearchString, searchString);
    nsCString url;
    (void)aArguments->GetUTF8String(kArgIndexURL, url);

    
    
    if (!HAS_BEHAVIOR(JAVASCRIPT) &&
        !StringBeginsWith(searchString, NS_LITERAL_CSTRING("javascript:")) &&
        StringBeginsWith(url, NS_LITERAL_CSTRING("javascript:"))) {
      NS_IF_ADDREF(*_result = new IntegerVariant(0));
      NS_ENSURE_TRUE(*_result, NS_ERROR_OUT_OF_MEMORY);
      return NS_OK;
    }

    PRInt32 visitCount = aArguments->AsInt32(kArgIndexVisitCount);
    bool typed = aArguments->AsInt32(kArgIndexTyped) ? true : false;
    bool bookmark = aArguments->AsInt32(kArgIndexBookmark) ? true : false;
    nsCAutoString tags;
    (void)aArguments->GetUTF8String(kArgIndexTags, tags);
    PRInt32 openPageCount = aArguments->AsInt32(kArgIndexOpenPageCount);

    
    
    bool matches = !(
      (HAS_BEHAVIOR(HISTORY) && visitCount == 0) ||
      (HAS_BEHAVIOR(TYPED) && !typed) ||
      (HAS_BEHAVIOR(BOOKMARK) && !bookmark) ||
      (HAS_BEHAVIOR(TAG) && tags.IsVoid()) ||
      (HAS_BEHAVIOR(OPENPAGE) && openPageCount == 0)
    );
    if (!matches) {
      NS_IF_ADDREF(*_result = new IntegerVariant(0));
      NS_ENSURE_TRUE(*_result, NS_ERROR_OUT_OF_MEMORY);
      return NS_OK;
    }

    
    nsCString fixedURI;
    fixupURISpec(url, fixedURI);

    
    PRInt32 matchBehavior = aArguments->AsInt32(kArgIndexMatchBehavior);
    searchFunctionPtr searchFunction = getSearchFunction(matchBehavior);

    nsCAutoString title;
    (void)aArguments->GetUTF8String(kArgIndexTitle, title);

    
    
    nsCWhitespaceTokenizer tokenizer(searchString);
    while (matches && tokenizer.hasMoreTokens()) {
      const nsDependentCSubstring &token = tokenizer.nextToken();

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
