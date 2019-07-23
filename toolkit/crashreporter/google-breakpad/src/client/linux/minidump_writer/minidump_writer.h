




























#ifndef CLIENT_LINUX_MINIDUMP_WRITER_MINIDUMP_WRITER_H_
#define CLIENT_LINUX_MINIDUMP_WRITER_MINIDUMP_WRITER_H_

#include <stdint.h>
#include <unistd.h>

namespace google_breakpad {











bool WriteMinidump(const char* filename, pid_t crashing_process,
                   const void* blob, size_t blob_size);

}  

#endif  
