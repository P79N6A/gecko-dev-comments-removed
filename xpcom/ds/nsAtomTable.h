





#ifndef nsAtomTable_h__
#define nsAtomTable_h__

#include "mozilla/MemoryReporting.h"
#include <stddef.h>

void NS_PurgeAtomTable();

void NS_SizeOfAtomTablesIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                      size_t* aMain, size_t* aStatic);

#endif 
