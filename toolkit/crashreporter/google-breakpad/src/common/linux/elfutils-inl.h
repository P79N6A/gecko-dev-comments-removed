




























#ifndef COMMON_LINUX_ELFUTILS_INL_H__
#define COMMON_LINUX_ELFUTILS_INL_H__

#include "common/linux/linux_libc_support.h"
#include "elfutils.h"

namespace google_breakpad {

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
  assert(name != NULL);
  assert(sections != NULL);
  assert(nsection > 0);

  int name_len = my_strlen(name);
  if (name_len == 0)
    return NULL;

  for (int i = 0; i < nsection; ++i) {
    const char* section_name = section_names + sections[i].sh_name;
    if (sections[i].sh_type == section_type &&
        names_end - section_name >= name_len + 1 &&
        my_strcmp(name, section_name) == 0) {
      return sections + i;
    }
  }
  return NULL;
}

}  

#endif  
