































#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/linux/eintr_wrapper.h"
#include "common/tests/file_utils.h"

namespace google_breakpad {

bool CopyFile(const char* from_path, const char* to_path) {
  int infile = HANDLE_EINTR(open(from_path, O_RDONLY));
  if (infile < 0) {
    perror("open");
    return false;
  }

  int outfile = HANDLE_EINTR(creat(to_path, 0666));
  if (outfile < 0) {
    perror("creat");
    if (HANDLE_EINTR(close(infile)) < 0) {
      perror("close");
    }
    return false;
  }

  char buffer[1024];
  bool result = true;

  while (result) {
    ssize_t bytes_read = HANDLE_EINTR(read(infile, buffer, sizeof(buffer)));
    if (bytes_read < 0) {
      perror("read");
      result = false;
      break;
    }
    if (bytes_read == 0)
      break;
    ssize_t bytes_written_per_read = 0;
    do {
      ssize_t bytes_written_partial = HANDLE_EINTR(write(
          outfile,
          &buffer[bytes_written_per_read],
          bytes_read - bytes_written_per_read));
      if (bytes_written_partial < 0) {
        perror("write");
        result = false;
        break;
      }
      bytes_written_per_read += bytes_written_partial;
    } while (bytes_written_per_read < bytes_read);
  }

  if (HANDLE_EINTR(close(infile)) == -1) {
    perror("close");
    result = false;
  }
  if (HANDLE_EINTR(close(outfile)) == -1) {
    perror("close");
    result = false;
  }

  return result;
}

bool ReadFile(const char* path, void* buffer, ssize_t* buffer_size) {
  int fd = HANDLE_EINTR(open(path, O_RDONLY));
  if (fd == -1) {
    perror("open");
    return false;
  }

  bool ok = true;
  if (buffer && buffer_size && *buffer_size > 0) {
    memset(buffer, 0, sizeof(*buffer_size));
    *buffer_size = HANDLE_EINTR(read(fd, buffer, *buffer_size));
    if (*buffer_size == -1) {
      perror("read");
      ok = false;
    }
  }
  if (HANDLE_EINTR(close(fd)) == -1) {
    perror("close");
    ok = false;
  }
  return ok;
}

bool WriteFile(const char* path, const void* buffer, size_t buffer_size) {
  int fd = HANDLE_EINTR(open(path, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU));
  if (fd == -1) {
    perror("open");
    return false;
  }

  bool ok = true;
  if (buffer) {
    size_t bytes_written_total = 0;
    ssize_t bytes_written_partial = 0;
    const char* data = reinterpret_cast<const char*>(buffer);
    while (bytes_written_total < buffer_size) {
      bytes_written_partial =
          HANDLE_EINTR(write(fd, data + bytes_written_total,
                             buffer_size - bytes_written_total));
      if (bytes_written_partial < 0) {
        perror("write");
        ok = false;
        break;
      }
      bytes_written_total += bytes_written_partial;
    }
  }
  if (HANDLE_EINTR(close(fd)) == -1) {
    perror("close");
    ok = false;
  }
  return ok;
}

}  
