





#ifndef mozilla_storage_FileSystemModule_h
#define mozilla_storage_FileSystemModule_h

#include "nscore.h"

struct sqlite3;

namespace mozilla {
namespace storage {

int RegisterFileSystemModule(sqlite3* aDB, const char* aName);

} 
} 

#endif 
