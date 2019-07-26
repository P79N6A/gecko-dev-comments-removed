







#ifndef nsStaticNameTable_h___
#define nsStaticNameTable_h___

#include "pldhash.h"
















class nsStaticCaseInsensitiveNameTable
{
public:
  enum { NOT_FOUND = -1 };

  bool             Init(const char* const aNames[], int32_t Count);
  int32_t          Lookup(const nsACString& aName);
  int32_t          Lookup(const nsAString& aName);
  const nsAFlatCString& GetStringValue(int32_t index);

  nsStaticCaseInsensitiveNameTable();
  ~nsStaticCaseInsensitiveNameTable();

private:
  nsDependentCString*   mNameArray;
  PLDHashTable mNameTable;
  nsDependentCString    mNullStr;
};

#endif
