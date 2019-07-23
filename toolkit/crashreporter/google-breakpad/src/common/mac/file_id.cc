


































#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "common/mac/file_id.h"
#include "common/mac/macho_id.h"

using MacFileUtilities::MachoID;

namespace google_breakpad {

FileID::FileID(const char *path) {
  strlcpy(path_, path, sizeof(path_));
}

bool FileID::FileIdentifier(unsigned char identifier[16]) {
  int fd = open(path_, O_RDONLY);
  if (fd == -1)
    return false;

  MD5_CTX md5;
  MD5_Init(&md5);

  
  
  unsigned char buffer[4096 * 2];
  size_t buffer_size = sizeof(buffer);
  while ((buffer_size = read(fd, buffer, buffer_size) > 0)) {
    MD5_Update(&md5, buffer, buffer_size);
  }

  close(fd);
  MD5_Final(identifier, &md5);

  return true;
}

bool FileID::MachoIdentifier(int cpu_type, unsigned char identifier[16]) {
  MachoID macho(path_);

  if (macho.UUIDCommand(cpu_type, identifier))
    return true;

  if (macho.IDCommand(cpu_type, identifier))
    return true;

  return macho.MD5(cpu_type, identifier);
}


void FileID::ConvertIdentifierToString(const unsigned char identifier[16],
                                       char *buffer, int buffer_length) {
  int buffer_idx = 0;
  for (int idx = 0; (buffer_idx < buffer_length) && (idx < 16); ++idx) {
    int hi = (identifier[idx] >> 4) & 0x0F;
    int lo = (identifier[idx]) & 0x0F;

    if (idx == 4 || idx == 6 || idx == 8 || idx == 10)
      buffer[buffer_idx++] = '-';

    buffer[buffer_idx++] = (hi >= 10) ? 'A' + hi - 10 : '0' + hi;
    buffer[buffer_idx++] = (lo >= 10) ? 'A' + lo - 10 : '0' + lo;
  }

  
  buffer[(buffer_idx < buffer_length) ? buffer_idx : buffer_idx - 1] = 0;
}

}  
