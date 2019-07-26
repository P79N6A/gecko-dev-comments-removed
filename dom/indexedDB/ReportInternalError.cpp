





#include "ReportInternalError.h"

#include "mozilla/IntegerPrintfMacros.h"

#include "nsContentUtils.h"
#include "nsPrintfCString.h"

BEGIN_INDEXEDDB_NAMESPACE

void
ReportInternalError(const char* aFile, uint32_t aLine, const char* aStr)
{
  
  for (const char* p = aFile; *p; ++p) {
    if (*p == '/' && *(p + 1)) {
      aFile = p + 1;
    }
  }

  nsContentUtils::LogSimpleConsoleError(
    NS_ConvertUTF8toUTF16(nsPrintfCString(
                          "IndexedDB %s: %s:%lu", aStr, aFile, aLine)),
    "indexedDB");
}

END_INDEXEDDB_NAMESPACE
