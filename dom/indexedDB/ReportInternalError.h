





#ifndef mozilla_dom_indexeddb_reportinternalerror_h__
#define mozilla_dom_indexeddb_reportinternalerror_h__

#include "nsDebug.h"

#include "IndexedDatabase.h"

#define IDB_WARNING(x)                                                         \
  mozilla::dom::indexedDB::ReportInternalError(__FILE__, __LINE__, x);         \
  NS_WARNING(x)

#define IDB_REPORT_INTERNAL_ERR()                                              \
  mozilla::dom::indexedDB::ReportInternalError(__FILE__, __LINE__,             \
                                               "UnknownErr")


#define IDB_ENSURE_TRUE(x, ret)                                                \
  do {                                                                         \
    if (MOZ_UNLIKELY(!(x))) {                                                  \
       IDB_REPORT_INTERNAL_ERR();                                              \
       NS_WARNING("IDB_ENSURE_TRUE(" #x ") failed");                           \
       return ret;                                                             \
    }                                                                          \
  } while(0)


#define IDB_ENSURE_SUCCESS(res, ret)                                           \
  do {                                                                         \
    nsresult __rv = res; /* Don't evaluate |res| more than once */             \
    if (NS_FAILED(__rv)) {                                                     \
      IDB_REPORT_INTERNAL_ERR();                                               \
      NS_ENSURE_SUCCESS_BODY(res, ret)                                         \
      return ret;                                                              \
    }                                                                          \
  } while(0)


namespace mozilla {
namespace dom {
namespace indexedDB {

void
ReportInternalError(const char* aFile, uint32_t aLine, const char* aStr);

} 
} 
} 

#endif  
