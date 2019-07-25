

































#include "common/linux/file_id.h"

#include <arpa/inet.h>
#include <assert.h>
#include <elf.h>
#include <fcntl.h>
#if defined(__ANDROID__)
#include "client/linux/android_link.h"
#else
#include <link.h>
#endif
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>

#include "common/linux/linux_libc_support.h"
#include "common/linux/linux_syscall_support.h"

namespace google_breakpad {

FileID::FileID(const char* path) {
  strncpy(path_, path, sizeof(path_));
}



  static bool FindElfTextSection(const void *elf_mapped_base,
                                 const void **text_start,
                                 int *text_size) {
  assert(elf_mapped_base);
  assert(text_start);
  assert(text_size);

  const char* elf_base =
    static_cast<const char*>(elf_mapped_base);
  const ElfW(Ehdr)* elf_header =
    reinterpret_cast<const ElfW(Ehdr)*>(elf_base);
  if (my_strncmp(elf_base, ELFMAG, SELFMAG) != 0)
    return false;
#if __ELF_NATIVE_CLASS == 32 || ELFSIZE == 32
#define ELFCLASS ELFCLASS32
#else
#define ELFCLASS ELFCLASS64
#endif
  
  if (elf_header->e_ident[EI_CLASS] != ELFCLASS)
    return false;
  *text_start = NULL;
  *text_size = 0;
  const ElfW(Shdr)* sections =
    reinterpret_cast<const ElfW(Shdr)*>(elf_base + elf_header->e_shoff);
  const char* text_section_name = ".text";
  int name_len = my_strlen(text_section_name);
  const ElfW(Shdr)* string_section = sections + elf_header->e_shstrndx;
  const ElfW(Shdr)* text_section = NULL;
  for (int i = 0; i < elf_header->e_shnum; ++i) {
    if (sections[i].sh_type == SHT_PROGBITS) {
      const char* section_name = (char*)(elf_base +
                                         string_section->sh_offset +
                                         sections[i].sh_name);
      if (!my_strncmp(section_name, text_section_name, name_len)) {
        text_section = &sections[i];
        break;
      }
    }
  }
  if (text_section != NULL && text_section->sh_size > 0) {
    *text_start = elf_base + text_section->sh_offset;
    *text_size = text_section->sh_size;
  }
  return true;
}


bool FileID::ElfFileIdentifierFromMappedFile(void* base,
                                             uint8_t identifier[kMDGUIDSize])
{
  const void* text_section = NULL;
  int text_size = 0;
  bool success = false;
  if (FindElfTextSection(base, &text_section, &text_size) && (text_size > 0)) {
    my_memset(identifier, 0, kMDGUIDSize);
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(text_section);
    const uint8_t* ptr_end = ptr + std::min(text_size, 4096);
    while (ptr < ptr_end) {
      for (unsigned i = 0; i < kMDGUIDSize; i++)
        identifier[i] ^= ptr[i];
      ptr += kMDGUIDSize;
    }
    success = true;
  }
  return success;
}

bool FileID::ElfFileIdentifier(uint8_t identifier[kMDGUIDSize]) {
  int fd = open(path_, O_RDONLY);
  if (fd < 0)
    return false;
  struct stat st;
  if (fstat(fd, &st) != 0) {
    close(fd);
    return false;
  }
  void* base = mmap(NULL, st.st_size,
                    PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  close(fd);
  if (base == MAP_FAILED)
    return false;

  bool success = ElfFileIdentifierFromMappedFile(base, identifier);
  munmap(base, st.st_size);
  return success;
}


void FileID::ConvertIdentifierToString(const uint8_t identifier[kMDGUIDSize],
                                       char* buffer, int buffer_length) {
  uint8_t identifier_swapped[kMDGUIDSize];

  
  memcpy(identifier_swapped, identifier, kMDGUIDSize);
  uint32_t* data1 = reinterpret_cast<uint32_t*>(identifier_swapped);
  *data1 = htonl(*data1);
  uint16_t* data2 = reinterpret_cast<uint16_t*>(identifier_swapped + 4);
  *data2 = htons(*data2);
  uint16_t* data3 = reinterpret_cast<uint16_t*>(identifier_swapped + 6);
  *data3 = htons(*data3);

  int buffer_idx = 0;
  for (unsigned int idx = 0;
       (buffer_idx < buffer_length) && (idx < kMDGUIDSize);
       ++idx) {
    int hi = (identifier_swapped[idx] >> 4) & 0x0F;
    int lo = (identifier_swapped[idx]) & 0x0F;

    if (idx == 4 || idx == 6 || idx == 8 || idx == 10)
      buffer[buffer_idx++] = '-';

    buffer[buffer_idx++] = (hi >= 10) ? 'A' + hi - 10 : '0' + hi;
    buffer[buffer_idx++] = (lo >= 10) ? 'A' + lo - 10 : '0' + lo;
  }

  
  buffer[(buffer_idx < buffer_length) ? buffer_idx : buffer_idx - 1] = 0;
}

}  
