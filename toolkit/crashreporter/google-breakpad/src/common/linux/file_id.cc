

































#include "common/linux/file_id.h"

#include <arpa/inet.h>
#include <assert.h>
#include <string.h>

#include <algorithm>

#include "common/linux/elfutils.h"
#include "common/linux/linux_libc_support.h"
#include "common/linux/memory_mapped_file.h"
#include "third_party/lss/linux_syscall_support.h"

namespace google_breakpad {

#ifndef NT_GNU_BUILD_ID
#define NT_GNU_BUILD_ID 3
#endif

FileID::FileID(const char* path) {
  strncpy(path_, path, sizeof(path_));
}


#define NOTE_PADDING(a) ((a + 3) & ~3)




template<typename ElfClass>
static bool ElfClassBuildIDNoteIdentifier(const void *section, int length,
                                          uint8_t identifier[kMDGUIDSize]) {
  typedef typename ElfClass::Nhdr Nhdr;

  const void* section_end = reinterpret_cast<const char*>(section) + length;
  const Nhdr* note_header = reinterpret_cast<const Nhdr*>(section);
  while (reinterpret_cast<const void *>(note_header) < section_end) {
    if (note_header->n_type == NT_GNU_BUILD_ID)
      break;
    note_header = reinterpret_cast<const Nhdr*>(
                  reinterpret_cast<const char*>(note_header) + sizeof(Nhdr) +
                  NOTE_PADDING(note_header->n_namesz) +
                  NOTE_PADDING(note_header->n_descsz));
  }
  if (reinterpret_cast<const void *>(note_header) >= section_end ||
      note_header->n_descsz == 0) {
    return false;
  }

  const char* build_id = reinterpret_cast<const char*>(note_header) +
    sizeof(Nhdr) + NOTE_PADDING(note_header->n_namesz);
  
  
  my_memset(identifier, 0, kMDGUIDSize);
  memcpy(identifier, build_id,
         std::min(kMDGUIDSize, (size_t)note_header->n_descsz));

  return true;
}



static bool FindElfBuildIDNote(const void *elf_mapped_base,
                               uint8_t identifier[kMDGUIDSize]) {
  void* note_section;
  int note_size, elfclass;
  if ((!FindElfSegment(elf_mapped_base, PT_NOTE,
                       (const void**)&note_section, &note_size, &elfclass) ||
      note_size == 0)  &&
      (!FindElfSection(elf_mapped_base, ".note.gnu.build-id", SHT_NOTE,
                       (const void**)&note_section, &note_size, &elfclass) ||
      note_size == 0)) {
    return false;
  }

  if (elfclass == ELFCLASS32) {
    return ElfClassBuildIDNoteIdentifier<ElfClass32>(note_section, note_size,
                                                     identifier);
  } else if (elfclass == ELFCLASS64) {
    return ElfClassBuildIDNoteIdentifier<ElfClass64>(note_section, note_size,
                                                     identifier);
  }

  return false;
}



static bool HashElfTextSection(const void *elf_mapped_base,
                               uint8_t identifier[kMDGUIDSize]) {
  void* text_section;
  int text_size;
  if (!FindElfSection(elf_mapped_base, ".text", SHT_PROGBITS,
                      (const void**)&text_section, &text_size, NULL) ||
      text_size == 0) {
    return false;
  }

  my_memset(identifier, 0, kMDGUIDSize);
  const uint8_t* ptr = reinterpret_cast<const uint8_t*>(text_section);
  const uint8_t* ptr_end = ptr + std::min(text_size, 4096);
  while (ptr < ptr_end) {
    for (unsigned i = 0; i < kMDGUIDSize; i++)
      identifier[i] ^= ptr[i];
    ptr += kMDGUIDSize;
  }
  return true;
}


bool FileID::ElfFileIdentifierFromMappedFile(const void* base,
                                             uint8_t identifier[kMDGUIDSize]) {
  
  if (FindElfBuildIDNote(base, identifier))
    return true;

  
  return HashElfTextSection(base, identifier);
}

bool FileID::ElfFileIdentifier(uint8_t identifier[kMDGUIDSize]) {
  MemoryMappedFile mapped_file(path_);
  if (!mapped_file.data())  
    return false;

  return ElfFileIdentifierFromMappedFile(mapped_file.data(), identifier);
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
