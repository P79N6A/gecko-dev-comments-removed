




#ifndef nsAtomTable_h__
#define nsAtomTable_h__

#include "mozilla/MemoryReporting.h"
#include <stddef.h>

void NS_PurgeAtomTable();

size_t NS_SizeOfAtomTablesIncludingThis(mozilla::MallocSizeOf aMallocSizeOf);

#endif 
