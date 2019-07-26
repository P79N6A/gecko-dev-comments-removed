





#ifndef mozilla_storage_SQLCollations_h
#define mozilla_storage_SQLCollations_h

#include "mozStorageService.h"
#include "nscore.h"
#include "nsString.h"

#include "sqlite3.h"

namespace mozilla {
namespace storage {











int registerCollations(sqlite3 *aDB, Service *aService);
























int localeCollation8(void *aService,
                                 int aLen1,
                                 const void *aStr1,
                                 int aLen2,
                                 const void *aStr2);






















int localeCollationCaseSensitive8(void *aService,
                                              int aLen1,
                                              const void *aStr1,
                                              int aLen2,
                                              const void *aStr2);






















int localeCollationAccentSensitive8(void *aService,
                                                int aLen1,
                                                const void *aStr1,
                                                int aLen2,
                                                const void *aStr2);





















int localeCollationCaseAccentSensitive8(void *aService,
                                                    int aLen1,
                                                    const void *aStr1,
                                                    int aLen2,
                                                    const void *aStr2);





















int localeCollation16(void *aService,
                                  int aLen1,
                                  const void *aStr1,
                                  int aLen2,
                                  const void *aStr2);






















int localeCollationCaseSensitive16(void *aService,
                                               int aLen1,
                                               const void *aStr1,
                                               int aLen2,
                                               const void *aStr2);






















int localeCollationAccentSensitive16(void *aService,
                                                 int aLen1,
                                                 const void *aStr1,
                                                 int aLen2,
                                                 const void *aStr2);





















int localeCollationCaseAccentSensitive16(void *aService,
                                                     int aLen1,
                                                     const void *aStr1,
                                                     int aLen2,
                                                     const void *aStr2);

} 
} 

#endif 
