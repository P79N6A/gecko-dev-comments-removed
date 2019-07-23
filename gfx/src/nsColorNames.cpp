




































#include "nsColorNames.h"
#include "nsString.h"
#include "nsStaticNameTable.h"
#include "nsReadableUtils.h"


#define GFX_COLOR(_name, _value) #_name,
const char* const kColorNames[] = {
#include "nsColorNameList.h"
};
#undef GFX_COLOR


#define GFX_COLOR(_name, _value) _value,
const nscolor nsColorNames::kColors[] = {
#include "nsColorNameList.h"
};
#undef GFX_COLOR

static PRInt32 gTableRefCount;
static nsStaticCaseInsensitiveNameTable* gColorTable;

void
nsColorNames::AddRefTable(void) 
{
  if (0 == gTableRefCount++) {
    NS_ASSERTION(!gColorTable, "pre existing array!");
    gColorTable = new nsStaticCaseInsensitiveNameTable();
    if (gColorTable) {
#ifdef DEBUG
    {
      
      for (PRInt32 index = 0; index < eColorName_COUNT; ++index) {
        nsCAutoString temp1(kColorNames[index]);
        nsCAutoString temp2(kColorNames[index]);
        ToLowerCase(temp1);
        NS_ASSERTION(temp1.Equals(temp2), "upper case char in table");
      }
    }
#endif      
      gColorTable->Init(kColorNames, eColorName_COUNT); 
    }
  }
}

void
nsColorNames::ReleaseTable(void) 
{
  if (0 == --gTableRefCount) {
    if (gColorTable) {
      delete gColorTable;
      gColorTable = nsnull;
    }
  }
}

nsColorName 
nsColorNames::LookupName(const nsACString& aColor)
{
  NS_ASSERTION(gColorTable, "no lookup table, needs addref");
  if (gColorTable) {
    return nsColorName(gColorTable->Lookup(aColor));
  }  
  return eColorName_UNKNOWN;
}

nsColorName 
nsColorNames::LookupName(const nsAString& aColor)
{
  NS_ASSERTION(gColorTable, "no lookup table, needs addref");
  if (gColorTable) {
    return nsColorName(gColorTable->Lookup(aColor));
  }  
  return eColorName_UNKNOWN;
}

const nsAFlatCString& 
nsColorNames::GetStringValue(nsColorName aColor)
{
  NS_ASSERTION(gColorTable, "no lookup table, needs addref");
  if (gColorTable) {
    return gColorTable->GetStringValue(PRInt32(aColor));
  } else {
    static nsDependentCString kNullStr("");
    return kNullStr;
  }
}

