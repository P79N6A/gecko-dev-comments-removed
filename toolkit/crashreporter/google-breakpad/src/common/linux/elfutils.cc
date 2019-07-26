




























#include "common/linux/elfutils.h"

#include <assert.h>
#include <string.h>

#include "common/linux/linux_libc_support.h"
#include "common/linux/elfutils-inl.h"

namespace google_breakpad {

namespace {

template<typename ElfClass>
void FindElfClassSection(const char *elf_base,
                         const char *section_name,
                         typename ElfClass::Word section_type,
                         const void **section_start,
                         int *section_size) {
  typedef typename ElfClass::Ehdr Ehdr;
  typedef typename ElfClass::Shdr Shdr;

  assert(elf_base);
  assert(section_start);
  assert(section_size);

  assert(my_strncmp(elf_base, ELFMAG, SELFMAG) == 0);

  const Ehdr* elf_header = reinterpret_cast<const Ehdr*>(elf_base);
  assert(elf_header->e_ident[EI_CLASS] == ElfClass::kClass);

  const Shdr* sections =
    GetOffset<ElfClass,Shdr>(elf_header, elf_header->e_shoff);
  const Shdr* section_names = sections + elf_header->e_shstrndx;
  const char* names =
    GetOffset<ElfClass,char>(elf_header, section_names->sh_offset);
  const char *names_end = names + section_names->sh_size;

  const Shdr* section =
    FindElfSectionByName<ElfClass>(section_name, section_type,
                                   sections, names, names_end,
                                   elf_header->e_shnum);

  if (section != NULL && section->sh_size > 0) {
    *section_start = elf_base + section->sh_offset;
    *section_size = section->sh_size;
  }
}

}  

bool IsValidElf(const void* elf_base) {
  return my_strncmp(reinterpret_cast<const char*>(elf_base),
                    ELFMAG, SELFMAG) == 0;
}

int ElfClass(const void* elf_base) {
  const ElfW(Ehdr)* elf_header =
    reinterpret_cast<const ElfW(Ehdr)*>(elf_base);

  return elf_header->e_ident[EI_CLASS];
}

bool FindElfSection(const void *elf_mapped_base,
                    const char *section_name,
                    uint32_t section_type,
                    const void **section_start,
                    int *section_size,
                    int *elfclass) {
  assert(elf_mapped_base);
  assert(section_start);
  assert(section_size);

  *section_start = NULL;
  *section_size = 0;

  if (!IsValidElf(elf_mapped_base))
    return false;

  int cls = ElfClass(elf_mapped_base);
  if (elfclass) {
    *elfclass = cls;
  }

  const char* elf_base =
    static_cast<const char*>(elf_mapped_base);

  if (cls == ELFCLASS32) {
    FindElfClassSection<ElfClass32>(elf_base, section_name, section_type,
                                    section_start, section_size);
    return *section_start != NULL;
  } else if (cls == ELFCLASS64) {
    FindElfClassSection<ElfClass64>(elf_base, section_name, section_type,
                                    section_start, section_size);
    return *section_start != NULL;
  }

  return false;
}

}  
