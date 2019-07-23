








































#ifndef nsStaticNameTable_h___
#define nsStaticNameTable_h___

#include "pldhash.h"
















class NS_COM nsStaticCaseInsensitiveNameTable
{
public:
  enum { NOT_FOUND = -1 };

  PRBool           Init(const char* const aNames[], PRInt32 Count);
  PRInt32          Lookup(const nsACString& aName);
  PRInt32          Lookup(const nsAString& aName);
  const nsAFlatCString& GetStringValue(PRInt32 index);

  nsStaticCaseInsensitiveNameTable();
  ~nsStaticCaseInsensitiveNameTable();

private:
  nsDependentCString*   mNameArray;
  PLDHashTable mNameTable;
  nsDependentCString    mNullStr;
};

#endif
