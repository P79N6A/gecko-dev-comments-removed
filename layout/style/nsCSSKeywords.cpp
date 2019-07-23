






































#include "nsCSSKeywords.h"
#include "nsString.h"
#include "nsStaticNameTable.h"
#include "nsReadableUtils.h"
#include "nsStyleConsts.h"


extern const char* const kCSSRawKeywords[];


#define CSS_KEY(_name,_id) #_name,
const char* const kCSSRawKeywords[] = {
#include "nsCSSKeywordList.h"
};
#undef CSS_KEY

static PRInt32 gTableRefCount;
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
      
      PRInt32 index = 0;
      for (; index < eCSSKeyword_COUNT && kCSSRawKeywords[index]; ++index) {
        nsCAutoString temp1(kCSSRawKeywords[index]);
        nsCAutoString temp2(kCSSRawKeywords[index]);
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
      gKeywordTable = nsnull;
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
    return gKeywordTable->GetStringValue(PRInt32(aKeyword));
  } else {
    static nsDependentCString kNullStr("");
    return kNullStr;
  }
}

