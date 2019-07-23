

































#include <cassert>
#include <cstdio>
#include <elf.h>
#include <fcntl.h>
#include <link.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

#include "common/linux/file_id.h"
#include "common/md5.h"

namespace google_breakpad {

static bool FindElfTextSection(const void *elf_mapped_base,
                               const void **text_start,
                               int *text_size) {
  assert(elf_mapped_base);
  assert(text_start);
  assert(text_size);

  const unsigned char *elf_base =
    static_cast<const unsigned char *>(elf_mapped_base);
  const ElfW(Ehdr) *elf_header =
    reinterpret_cast<const ElfW(Ehdr) *>(elf_base);
  if (memcmp(elf_header, ELFMAG, SELFMAG) != 0)
    return false;
  *text_start = NULL;
  *text_size = 0;
  const ElfW(Shdr) *sections =
    reinterpret_cast<const ElfW(Shdr) *>(elf_base + elf_header->e_shoff);
  const char *text_section_name = ".text";
  int name_len = strlen(text_section_name);
  const ElfW(Shdr) *string_section = sections + elf_header->e_shstrndx;
  const ElfW(Shdr) *text_section = NULL;
  for (int i = 0; i < elf_header->e_shnum; ++i) {
    if (sections[i].sh_type == SHT_PROGBITS) {
      const char *section_name = (char*)(elf_base +
                                         string_section->sh_offset +
                                         sections[i].sh_name);
      if (!strncmp(section_name, text_section_name, name_len)) {
        text_section = &sections[i];
        break;
      }
    }
  }
  if (text_section != NULL && text_section->sh_size > 0) {
    int text_section_size = text_section->sh_size;
    *text_start = elf_base + text_section->sh_offset;
    *text_size = text_section_size;
  }
  return true;
}

FileID::FileID(const char *path) {
  strncpy(path_, path, sizeof(path_));
}

bool FileID::ElfFileIdentifier(unsigned char identifier[16]) {
  int fd = open(path_, O_RDONLY);
  if (fd < 0)
    return false;
  struct stat st;
  if (fstat(fd, &st) != 0 && st.st_size <= 0) {
    close(fd);
    return false;
  }
  void *base = mmap(NULL, st.st_size,
                    PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (!base) {
    close(fd);
    return false;
  }
  bool success = false;
  const void *text_section = NULL;
  int text_size = 0;
  if (FindElfTextSection(base, &text_section, &text_size) && (text_size > 0)) {
    struct MD5Context md5;
    MD5Init(&md5);
    MD5Update(&md5,
              static_cast<const unsigned char*>(text_section),
              text_size);
    MD5Final(identifier, &md5);
    success = true;
  }

  close(fd);
  munmap(base, st.st_size);
  return success;
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
