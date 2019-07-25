






































#include "mozilla/Util.h"

#include "SQLCollations.h"

namespace mozilla {
namespace storage {




namespace {






















int
localeCollationHelper8(void *aService,
                       int aLen1,
                       const void *aStr1,
                       int aLen2,
                       const void *aStr2,
                       PRInt32 aComparisonStrength)
{
  NS_ConvertUTF8toUTF16 str1(static_cast<const char *>(aStr1), aLen1);
  NS_ConvertUTF8toUTF16 str2(static_cast<const char *>(aStr2), aLen2);
  Service *serv = static_cast<Service *>(aService);
  return serv->localeCompareStrings(str1, str2, aComparisonStrength);
}






















int
localeCollationHelper16(void *aService,
                        int aLen1,
                        const void *aStr1,
                        int aLen2,
                        const void *aStr2,
                        PRInt32 aComparisonStrength)
{
  const PRUnichar *buf1 = static_cast<const PRUnichar *>(aStr1);
  const PRUnichar *buf2 = static_cast<const PRUnichar *>(aStr2);

  
  
  
  
  nsDependentSubstring str1(buf1, buf1 + (aLen1 / sizeof(PRUnichar)));
  nsDependentSubstring str2(buf2, buf2 + (aLen2 / sizeof(PRUnichar)));
  Service *serv = static_cast<Service *>(aService);
  return serv->localeCompareStrings(str1, str2, aComparisonStrength);
}



struct Collations {
  const char *zName;
  int enc;
  int(*xCompare)(void*, int, const void*, int, const void*);
};

} 




int
registerCollations(sqlite3 *aDB,
                   Service *aService)
{
  Collations collations[] = {
    {"locale",
     SQLITE_UTF8,
     localeCollation8},
    {"locale_case_sensitive",
     SQLITE_UTF8,
     localeCollationCaseSensitive8},
    {"locale_accent_sensitive",
     SQLITE_UTF8,
     localeCollationAccentSensitive8},
    {"locale_case_accent_sensitive",
     SQLITE_UTF8,
     localeCollationCaseAccentSensitive8},
    {"locale",
     SQLITE_UTF16,
     localeCollation16},
    {"locale_case_sensitive",
     SQLITE_UTF16,
     localeCollationCaseSensitive16},
    {"locale_accent_sensitive",
     SQLITE_UTF16,
     localeCollationAccentSensitive16},
    {"locale_case_accent_sensitive",
     SQLITE_UTF16,
     localeCollationCaseAccentSensitive16},
  };

  int rv = SQLITE_OK;
  for (size_t i = 0; SQLITE_OK == rv && i < ArrayLength(collations); ++i) {
    struct Collations *p = &collations[i];
    rv = ::sqlite3_create_collation(aDB, p->zName, p->enc, aService,
                                    p->xCompare);
  }

  return rv;
}




int
localeCollation8(void *aService,
                 int aLen1,
                 const void *aStr1,
                 int aLen2,
                 const void *aStr2)
{
  return localeCollationHelper8(aService, aLen1, aStr1, aLen2, aStr2,
                                nsICollation::kCollationCaseInSensitive);
}

int
localeCollationCaseSensitive8(void *aService,
                              int aLen1,
                              const void *aStr1,
                              int aLen2,
                              const void *aStr2)
{
  return localeCollationHelper8(aService, aLen1, aStr1, aLen2, aStr2,
                                nsICollation::kCollationAccentInsenstive);
}

int
localeCollationAccentSensitive8(void *aService,
                                int aLen1,
                                const void *aStr1,
                                int aLen2,
                                const void *aStr2)
{
  return localeCollationHelper8(aService, aLen1, aStr1, aLen2, aStr2,
                                nsICollation::kCollationCaseInsensitiveAscii);
}

int
localeCollationCaseAccentSensitive8(void *aService,
                                    int aLen1,
                                    const void *aStr1,
                                    int aLen2,
                                    const void *aStr2)
{
  return localeCollationHelper8(aService, aLen1, aStr1, aLen2, aStr2,
                                nsICollation::kCollationCaseSensitive);
}

int
localeCollation16(void *aService,
                  int aLen1,
                  const void *aStr1,
                  int aLen2,
                  const void *aStr2)
{
  return localeCollationHelper16(aService, aLen1, aStr1, aLen2, aStr2,
                                 nsICollation::kCollationCaseInSensitive);
}

int
localeCollationCaseSensitive16(void *aService,
                               int aLen1,
                               const void *aStr1,
                               int aLen2,
                               const void *aStr2)
{
  return localeCollationHelper16(aService, aLen1, aStr1, aLen2, aStr2,
                                 nsICollation::kCollationAccentInsenstive);
}

int
localeCollationAccentSensitive16(void *aService,
                                 int aLen1,
                                 const void *aStr1,
                                 int aLen2,
                                 const void *aStr2)
{
  return localeCollationHelper16(aService, aLen1, aStr1, aLen2, aStr2,
                                 nsICollation::kCollationCaseInsensitiveAscii);
}

int
localeCollationCaseAccentSensitive16(void *aService,
                                     int aLen1,
                                     const void *aStr1,
                                     int aLen2,
                                     const void *aStr2)
{
  return localeCollationHelper16(aService, aLen1, aStr1, aLen2, aStr2,
                                 nsICollation::kCollationCaseSensitive);
}

} 
} 
