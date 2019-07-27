




#ifndef mozilla_ICUUtils_h__
#define mozilla_ICUUtils_h__


#ifdef ENABLE_INTL_API



#ifdef MOZILLA_INTERNAL_API

#include "mozilla/Scoped.h"
#include "nsStringGlue.h"
#include "unicode/unum.h" 

class nsIContent;

namespace {
  struct ScopedUNumberFormatTraits {
    typedef UNumberFormat* type;
    static type empty() { return nullptr; }
    static void release(type handle) { if (handle) unum_close(handle); }
  };
};
typedef mozilla::Scoped<ScopedUNumberFormatTraits> AutoCloseUNumberFormat;

class ICUUtils
{
public:

  



  class LanguageTagIterForContent {
  public:
    explicit LanguageTagIterForContent(nsIContent* aContent)
      : mContent(aContent)
      , mCurrentFallbackIndex(-1)
    {}

    













    void GetNext(nsACString& aBCP47LangTag);

    bool IsAtStart() const {
      return mCurrentFallbackIndex < 0;
    }

  private:
    nsIContent* mContent;
    int8_t mCurrentFallbackIndex;
  };

  




  static bool LocalizeNumber(double aValue,
                             LanguageTagIterForContent& aLangTags,
                             nsAString& aLocalizedValue);

  



  static double ParseNumber(nsAString& aValue,
                            LanguageTagIterForContent& aLangTags);

  static void AssignUCharArrayToString(UChar* aICUString,
                                       int32_t aLength,
                                       nsAString& aMozString);

#if 0
  
  

  


  static Locale BCP47CodeToLocale(const nsAString& aBCP47Code);

  static void ToMozString(UnicodeString& aICUString, nsAString& aMozString);
  static void ToICUString(nsAString& aMozString, UnicodeString& aICUString);
#endif
};

#endif 
#endif 

#endif 

