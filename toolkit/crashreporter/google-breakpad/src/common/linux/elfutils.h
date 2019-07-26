































#ifndef COMMON_LINUX_ELFUTILS_H__
#define COMMON_LINUX_ELFUTILS_H__

#include <elf.h>
#include <link.h>
#include <stdint.h>

namespace google_breakpad {



struct ElfClass32 {
  typedef Elf32_Addr Addr;
  typedef Elf32_Ehdr Ehdr;
  typedef Elf32_Nhdr Nhdr;
  typedef Elf32_Phdr Phdr;
  typedef Elf32_Shdr Shdr;
  typedef Elf32_Half Half;
  typedef Elf32_Off Off;
  typedef Elf32_Word Word;
  static const int kClass = ELFCLASS32;
  static const size_t kAddrSize = sizeof(Elf32_Addr);
};

struct ElfClass64 {
  typedef Elf64_Addr Addr;
  typedef Elf64_Ehdr Ehdr;
  typedef Elf64_Nhdr Nhdr;
  typedef Elf64_Phdr Phdr;
  typedef Elf64_Shdr Shdr;
  typedef Elf64_Half Half;
  typedef Elf64_Off Off;
  typedef Elf64_Word Word;
  static const int kClass = ELFCLASS64;
  static const size_t kAddrSize = sizeof(Elf64_Addr);
};

bool IsValidElf(const void* elf_header);
int ElfClass(const void* elf_base);






bool FindElfSection(const void *elf_mapped_base,
                    const char *section_name,
                    uint32_t section_type,
                    const void **section_start,
                    int *section_size,
                    int *elfclass);



template<typename ElfClass>
const typename ElfClass::Shdr*
FindElfSectionByName(const char* name,
                     typename ElfClass::Word section_type,
                     const typename ElfClass::Shdr* sections,
                     const char* section_names,
                     const char* names_end,
                     int nsection);






bool FindElfSegment(const void *elf_mapped_base,
                    uint32_t segment_type,
                    const void **segment_start,
                    int *segment_size,
                    int *elfclass);





template<typename ElfClass, typename T>
const T*
GetOffset(const typename ElfClass::Ehdr* elf_header,
          typename ElfClass::Off offset);

}  

#endif  
