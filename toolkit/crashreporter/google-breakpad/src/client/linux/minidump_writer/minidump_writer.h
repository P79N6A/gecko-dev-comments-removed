




























#ifndef CLIENT_LINUX_MINIDUMP_WRITER_MINIDUMP_WRITER_H_
#define CLIENT_LINUX_MINIDUMP_WRITER_MINIDUMP_WRITER_H_

#include <list>
#include <utility>

#include <stdint.h>
#include <unistd.h>

#include "google_breakpad/common/minidump_format.h"

namespace google_breakpad {


typedef std::pair<struct MappingInfo, u_int8_t[sizeof(MDGUID)]> MappingEntry;
typedef std::list<MappingEntry> MappingList;



struct AppMemory {
  AppMemory(void *ptr, size_t length) : ptr(ptr), length(length) {}

  void *ptr;
  size_t length;
};
typedef std::list<AppMemory> AppMemoryList;











bool WriteMinidump(const char* filename, pid_t crashing_process,
                   const void* blob, size_t blob_size);






bool WriteMinidump(const char* filename, pid_t process,
                   pid_t process_blamed_thread);



bool WriteMinidump(const char* filename, pid_t crashing_process,
                   const void* blob, size_t blob_size,
                   const MappingList& mappings,
                   const AppMemoryList& appdata);

}  

#endif  
