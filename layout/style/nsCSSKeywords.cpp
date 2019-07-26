






#include "nsCSSKeywords.h"
#include "nsString.h"
#include "nsStaticNameTable.h"
#include "nsReadableUtils.h"


extern const char* const kCSSRawKeywords[];


#define CSS_KEY(_name,_id) #_name,
const char* const kCSSRawKeywords[] = {
#include "nsCSSKeywordList.h"
};
#undef CSS_KEY

static int32_t gTableRefCount;
static nsStaticCaseInsensitiveNameTable* gKeywordTable;

void
nsCSSKeywords::AddRefTable(void) 
{
  if (0 == gTableRefCount++) {
    NS_ASSERTION(!gKeywordTable, "pre existing array!");
    gKeywordTable = new nsStaticCaseInsensitiveNameTable();
    if (gKeywordTable) {
#ifdef DEBUG
    {
      
      int32_t index = 0;
      for (; index < eCSSKeyword_COUNT && kCSSRawKeywords[index]; ++index) {
        nsAutoCString temp1(kCSSRawKeywords[index]);
        nsAutoCString temp2(kCSSRawKeywords[index]);
        ToLowerCase(temp1);
        NS_ASSERTION(temp1.Equals(temp2), "upper case char in table");
        NS_ASSERTION(-1 == temp1.FindChar('_'), "underscore char in table");
      }
      NS_ASSERTION(index == eCSSKeyword_COUNT, "kCSSRawKeywords and eCSSKeyword_COUNT are out of sync");
    }
#endif      
      gKeywordTable->Init(kCSSRawKeywords, eCSSKeyword_COUNT); 
    }
  }
}

void
nsCSSKeywords::ReleaseTable(void) 
{
  if (0 == --gTableRefCount) {
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

