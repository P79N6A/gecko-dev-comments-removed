







#ifndef nsStaticNameTable_h___
#define nsStaticNameTable_h___

#include "pldhash.h"
#include "nsString.h"

















class nsStaticCaseInsensitiveNameTable
{
public:
  enum { NOT_FOUND = -1 };

  int32_t          Lookup(const nsACString& aName);
  int32_t          Lookup(const nsAString& aName);
  const nsAFlatCString& GetStringValue(int32_t aIndex);

  nsStaticCaseInsensitiveNameTable(const char* const aNames[], int32_t aLength);
  ~nsStaticCaseInsensitiveNameTable();

private:
  nsDependentCString*   mNameArray;
  PLDHashTable2         mNameTable;
  nsDependentCString    mNullStr;
};

#endif
