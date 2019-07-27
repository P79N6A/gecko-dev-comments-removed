






































#ifndef LulElfInt_h
#define LulElfInt_h




#include <elf.h>
#include <stdlib.h>

#include "mozilla/Assertions.h"

#include "LulPlatformMacros.h"






#if defined(LUL_OS_android)



#ifndef EM_X86_64
#define EM_X86_64  62
#endif

#ifndef EM_PPC64
#define EM_PPC64   21
#endif

#ifndef EM_S390
#define EM_S390    22
#endif

#ifndef NT_GNU_BUILD_ID
#define NT_GNU_BUILD_ID 3
#endif

#define ElfW(type)      _ElfW (Elf, ELFSIZE, type)
#define _ElfW(e,w,t)    _ElfW_1 (e, w, _##t)
#define _ElfW_1(e,w,t)  e##w##t


extern "C" {
  extern char*  basename(const char*  path);
};
#else

# include <link.h>
#endif


namespace lul {



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






static const size_t kMDGUIDSize = sizeof(MDGUID);

class FileID {
 public:

  
  
  
  static bool ElfFileIdentifierFromMappedFile(const void* base,
                                              uint8_t identifier[kMDGUIDSize]);

  
  
  
  
  static void ConvertIdentifierToString(const uint8_t identifier[kMDGUIDSize],
                                        char* buffer, int buffer_length);
};



template<typename ElfClass, typename T>
const T* GetOffset(const typename ElfClass::Ehdr* elf_header,
                   typename ElfClass::Off offset) {
  return reinterpret_cast<const T*>(reinterpret_cast<uintptr_t>(elf_header) +
                                    offset);
}

template<typename ElfClass>
const typename ElfClass::Shdr* FindElfSectionByName(
    const char* name,
    typename ElfClass::Word section_type,
    const typename ElfClass::Shdr* sections,
    const char* section_names,
    const char* names_end,
    int nsection) {
  MOZ_ASSERT(name != NULL);
  MOZ_ASSERT(sections != NULL);
  MOZ_ASSERT(nsection > 0);

  int name_len = strlen(name);
  if (name_len == 0)
    return NULL;

  for (int i = 0; i < nsection; ++i) {
    const char* section_name = section_names + sections[i].sh_name;
    if (sections[i].sh_type == section_type &&
        names_end - section_name >= name_len + 1 &&
        strcmp(name, section_name) == 0) {
      return sections + i;
    }
  }
  return NULL;
}

} 



#include "LulElfExt.h"

#endif 
