




























#ifndef CLIENT_LINUX_MINIDUMP_WRITER_MINIDUMP_WRITER_H_
#define CLIENT_LINUX_MINIDUMP_WRITER_MINIDUMP_WRITER_H_

#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include <list>
#include <utility>

#include "client/linux/minidump_writer/linux_dumper.h"
#include "google_breakpad/common/minidump_format.h"

namespace google_breakpad {

class ExceptionHandler;

struct MappingEntry {
  MappingInfo first;
  uint8_t second[sizeof(MDGUID)];
};


typedef std::list<MappingEntry> MappingList;



struct AppMemory {
  void* ptr;
  size_t length;

  bool operator==(const struct AppMemory& other) const {
    return ptr == other.ptr;
  }

  bool operator==(const void* other) const {
    return ptr == other;
  }
};
typedef std::list<AppMemory> AppMemoryList;











bool WriteMinidump(const char* minidump_path, pid_t crashing_process,
                   const void* blob, size_t blob_size);

bool WriteMinidump(int minidump_fd, pid_t crashing_process,
                   const void* blob, size_t blob_size);






bool WriteMinidump(const char* minidump_path, pid_t process,
                   pid_t process_blamed_thread);



bool WriteMinidump(const char* minidump_path, pid_t crashing_process,
                   const void* blob, size_t blob_size,
                   const MappingList& mappings,
                   const AppMemoryList& appdata);
bool WriteMinidump(int minidump_fd, pid_t crashing_process,
                   const void* blob, size_t blob_size,
                   const MappingList& mappings,
                   const AppMemoryList& appdata);


bool WriteMinidump(const char* minidump_path, off_t minidump_size_limit,
                   pid_t crashing_process,
                   const void* blob, size_t blob_size,
                   const MappingList& mappings,
                   const AppMemoryList& appdata);
bool WriteMinidump(int minidump_fd, off_t minidump_size_limit,
                   pid_t crashing_process,
                   const void* blob, size_t blob_size,
                   const MappingList& mappings,
                   const AppMemoryList& appdata);

bool WriteMinidump(const char* filename,
                   const MappingList& mappings,
                   const AppMemoryList& appdata,
                   LinuxDumper* dumper);

}  

#endif  
