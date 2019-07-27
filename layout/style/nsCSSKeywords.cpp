






#include "nsCSSKeywords.h"
#include "nsString.h"
#include "nsStaticNameTable.h"


extern const char* const kCSSRawKeywords[];


#define CSS_KEY(_name,_id) #_name,
const char* const kCSSRawKeywords[] = {
#include "nsCSSKeywordList.h"
};
#undef CSS_KEY

static int32_t gKeywordTableRefCount;
static nsStaticCaseInsensitiveNameTable* gKeywordTable;

void
nsCSSKeywords::AddRefTable(void) 
{
  if (0 == gKeywordTableRefCount++) {
    NS_ASSERTION(!gKeywordTable, "pre existing array!");
    gKeywordTable =
      new nsStaticCaseInsensitiveNameTable(kCSSRawKeywords, eCSSKeyword_COUNT);
#ifdef DEBUG
    
    int32_t index = 0;
    for (; index < eCSSKeyword_COUNT && kCSSRawKeywords[index]; ++index) {
      nsAutoCString temp(kCSSRawKeywords[index]);
      NS_ASSERTION(-1 == temp.FindChar('_'), "underscore char in table");
    }
    NS_ASSERTION(index == eCSSKeyword_COUNT, "kCSSRawKeywords and eCSSKeyword_COUNT are out of sync");
#endif
  }
}

void
nsCSSKeywords::ReleaseTable(void) 
{
  if (0 == --gKeywordTableRefCount) {
    if (gKeywordTable) {
      delete gKeywordTable;
      gKeywordTable = nullptr;
    }
  }
}

nsCSSKeyword 
nsCSSKeywords::LookupKeyword(const nsACString& aKeyword)
{
  NS_ASSERTION(gKeywordTable, "no lookup table, needs addref");
  if (gKeywordTable) {
    return nsCSSKeyword(gKeywordTable->Lookup(aKeyword));
  }  
  return eCSSKeyword_UNKNOWN;
}

nsCSSKeyword 
nsCSSKeywords::LookupKeyword(const nsAString& aKeyword)
{
  NS_ASSERTION(gKeywordTable, "no lookup table, needs addref");
  if (gKeywordTable) {
    return nsCSSKeyword(gKeywordTable->Lookup(aKeyword));
  }  
  return eCSSKeyword_UNKNOWN;
}

const nsAFlatCString& 
nsCSSKeywords::GetStringValue(nsCSSKeyword aKeyword)
{
  NS_ASSERTION(gKeywordTable, "no lookup table, needs addref");
  NS_ASSERTION(0 <= aKeyword && aKeyword < eCSSKeyword_COUNT, "out of range");
  if (gKeywordTable) {
    return gKeywordTable->GetStringValue(int32_t(aKeyword));
  } else {
    static nsDependentCString kNullStr("");
    return kNullStr;
  }
}

