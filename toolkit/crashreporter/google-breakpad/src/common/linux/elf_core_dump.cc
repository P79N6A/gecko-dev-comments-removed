































#include "common/linux/elf_core_dump.h"

#include <stddef.h>
#include <string.h>

namespace google_breakpad {



ElfCoreDump::Note::Note() {}

ElfCoreDump::Note::Note(const MemoryRange& content) : content_(content) {}

bool ElfCoreDump::Note::IsValid() const {
  return GetHeader() != NULL;
}

const ElfCoreDump::Nhdr* ElfCoreDump::Note::GetHeader() const {
  return content_.GetData<Nhdr>(0);
}

ElfCoreDump::Word ElfCoreDump::Note::GetType() const {
  const Nhdr* header = GetHeader();
  
  return header ? header->n_type : 0;
}

MemoryRange ElfCoreDump::Note::GetName() const {
  const Nhdr* header = GetHeader();
  if (header) {
      return content_.Subrange(sizeof(Nhdr), header->n_namesz);
  }
  return MemoryRange();
}

MemoryRange ElfCoreDump::Note::GetDescription() const {
  const Nhdr* header = GetHeader();
  if (header) {
    return content_.Subrange(AlignedSize(sizeof(Nhdr) + header->n_namesz),
                             header->n_descsz);
  }
  return MemoryRange();
}

ElfCoreDump::Note ElfCoreDump::Note::GetNextNote() const {
  MemoryRange next_content;
  const Nhdr* header = GetHeader();
  if (header) {
    size_t next_offset = AlignedSize(sizeof(Nhdr) + header->n_namesz);
    next_offset = AlignedSize(next_offset + header->n_descsz);
    next_content =
        content_.Subrange(next_offset, content_.length() - next_offset);
  }
  return Note(next_content);
}


size_t ElfCoreDump::Note::AlignedSize(size_t size) {
  size_t mask = sizeof(Word) - 1;
  return (size + mask) & ~mask;
}




ElfCoreDump::ElfCoreDump() {}

ElfCoreDump::ElfCoreDump(const MemoryRange& content)
    : content_(content) {
}

void ElfCoreDump::SetContent(const MemoryRange& content) {
  content_ = content;
}

bool ElfCoreDump::IsValid() const {
  const Ehdr* header = GetHeader();
  return (header &&
          header->e_ident[0] == ELFMAG0 &&
          header->e_ident[1] == ELFMAG1 &&
          header->e_ident[2] == ELFMAG2 &&
          header->e_ident[3] == ELFMAG3 &&
          header->e_ident[4] == kClass &&
          header->e_version == EV_CURRENT &&
          header->e_type == ET_CORE);
}

const ElfCoreDump::Ehdr* ElfCoreDump::GetHeader() const {
  return content_.GetData<Ehdr>(0);
}

const ElfCoreDump::Phdr* ElfCoreDump::GetProgramHeader(unsigned index) const {
  const Ehdr* header = GetHeader();
  if (header) {
    return reinterpret_cast<const Phdr*>(content_.GetArrayElement(
        header->e_phoff, header->e_phentsize, index));
  }
  return NULL;
}

const ElfCoreDump::Phdr* ElfCoreDump::GetFirstProgramHeaderOfType(
    Word type) const {
  for (unsigned i = 0, n = GetProgramHeaderCount(); i < n; ++i) {
    const Phdr* program = GetProgramHeader(i);
    if (program->p_type == type) {
      return program;
    }
  }
  return NULL;
}

unsigned ElfCoreDump::GetProgramHeaderCount() const {
  const Ehdr* header = GetHeader();
  return header ? header->e_phnum : 0;
}

bool ElfCoreDump::CopyData(void* buffer, Addr virtual_address, size_t length) {
  for (unsigned i = 0, n = GetProgramHeaderCount(); i < n; ++i) {
    const Phdr* program = GetProgramHeader(i);
    if (program->p_type != PT_LOAD)
      continue;

    size_t offset_in_segment = virtual_address - program->p_vaddr;
    if (virtual_address >= program->p_vaddr &&
        offset_in_segment < program->p_filesz) {
      const void* data =
          content_.GetData(program->p_offset + offset_in_segment, length);
      if (data) {
        memcpy(buffer, data, length);
        return true;
      }
    }
  }
  return false;
}

ElfCoreDump::Note ElfCoreDump::GetFirstNote() const {
  MemoryRange note_content;
  const Phdr* program_header = GetFirstProgramHeaderOfType(PT_NOTE);
  if (program_header) {
    note_content = content_.Subrange(program_header->p_offset,
                                     program_header->p_filesz);
  }
  return Note(note_content);
}

}  
