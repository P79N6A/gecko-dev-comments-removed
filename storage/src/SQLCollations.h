






































#ifndef _mozilla_storage_SQLCollations_h_
#define _mozilla_storage_SQLCollations_h_

#include "mozStorageService.h"
#include "nscore.h"
#include "nsString.h"

#include "sqlite3.h"

namespace mozilla {
namespace storage {











NS_HIDDEN_(int) registerCollations(sqlite3 *aDB, Service *aService);
























NS_HIDDEN_(int) localeCollation8(void *aService,
                                 int aLen1,
                                 const void *aStr1,
                                 int aLen2,
                                 const void *aStr2);






















NS_HIDDEN_(int) localeCollationCaseSensitive8(void *aService,
                                              int aLen1,
                                              const void *aStr1,
                                              int aLen2,
                                              const void *aStr2);






















NS_HIDDEN_(int) localeCollationAccentSensitive8(void *aService,
                                                int aLen1,
                                                const void *aStr1,
                                                int aLen2,
                                                const void *aStr2);





















NS_HIDDEN_(int) localeCollationCaseAccentSensitive8(void *aService,
                                                    int aLen1,
                                                    const void *aStr1,
                                                    int aLen2,
                                                    const void *aStr2);





















NS_HIDDEN_(int) localeCollation16(void *aService,
                                  int aLen1,
                                  const void *aStr1,
                                  int aLen2,
                                  const void *aStr2);






















NS_HIDDEN_(int) localeCollationCaseSensitive16(void *aService,
                                               int aLen1,
                                               const void *aStr1,
                                               int aLen2,
                                               const void *aStr2);






















NS_HIDDEN_(int) localeCollationAccentSensitive16(void *aService,
                                                 int aLen1,
                                                 const void *aStr1,
                                                 int aLen2,
                                                 const void *aStr2);





















NS_HIDDEN_(int) localeCollationCaseAccentSensitive16(void *aService,
                                                     int aLen1,
                                                     const void *aStr1,
                                                     int aLen2,
                                                     const void *aStr2);

} 
} 

#endif 
