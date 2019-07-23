



#include "base/rand_util.h"

#include <fcntl.h>
#include <unistd.h>

#include "base/file_util.h"
#include "base/logging.h"

namespace base {

uint64 RandUint64() {
  uint64 number;

  int urandom_fd = open("/dev/urandom", O_RDONLY);
  CHECK(urandom_fd >= 0);
  bool success = file_util::ReadFromFD(urandom_fd,
                                       reinterpret_cast<char*>(&number),
                                       sizeof(number));
  CHECK(success);
  close(urandom_fd);

  return number;
}

}  
