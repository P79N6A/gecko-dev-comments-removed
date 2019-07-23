



#include "base/test_file_util.h"

#include <sys/mman.h>
#include <errno.h>
#include "base/logging.h"
#include "base/file_util.h"

namespace file_util {

bool EvictFileFromSystemCache(const FilePath& file) {
  
  
  
  

  file_util::MemoryMappedFile mapped_file;
  if (!mapped_file.Initialize(file)) {
    DLOG(WARNING) << "failed to memory map " << file.value();
    return false;
  }
  
  if (msync(const_cast<uint8*>(mapped_file.data()), mapped_file.length(),
            MS_INVALIDATE) != 0) {
    DLOG(WARNING) << "failed to invalidate memory map of " << file.value() 
        << ", errno: " << errno;
    return false;
  }
  
  return true;
}

}  
