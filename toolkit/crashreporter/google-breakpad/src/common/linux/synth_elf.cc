#include "common/linux/synth_elf.h"

#include <assert.h>
#include <elf.h>
#include <stdio.h>
#include <string.h>

#include "common/using_std_string.h"

namespace google_breakpad {
namespace synth_elf {

#ifndef NT_GNU_BUILD_ID
#define NT_GNU_BUILD_ID 3
#endif

ELF::ELF(uint16_t machine,
         uint8_t file_class,
         Endianness endianness)
  : Section(endianness),
    addr_size_(file_class == ELFCLASS64 ? 8 : 4),
    program_count_(0),
    section_count_(0),
    section_header_table_(endianness),
    section_header_strings_(endianness) {
  
  assert(machine == EM_386 ||
         machine == EM_X86_64 ||
         machine == EM_ARM);
  assert(file_class == ELFCLASS32 || file_class == ELFCLASS64);

  start() = 0;
  
  
  
  D8(ELFMAG0);
  D8(ELFMAG1);
  D8(ELFMAG2);
  D8(ELFMAG3);
  
  D8(file_class);
  
  D8(endianness == kLittleEndian ? ELFDATA2LSB : ELFDATA2MSB);
  
  D8(EV_CURRENT);
  
  D8(ELFOSABI_SYSV);
  
  D8(0);
  
  Append(7, 0);
  assert(Size() == EI_NIDENT);

  
  D16(ET_EXEC);  
  
  D16(machine);
  
  D32(EV_CURRENT);
  
  Append(endianness, addr_size_, 0);
  
  Append(endianness, addr_size_, program_header_label_);
  
  Append(endianness, addr_size_, section_header_label_);
  
  D32(0);
  
  D16(addr_size_ == 8 ? sizeof(Elf64_Ehdr) : sizeof(Elf32_Ehdr));
  
  D16(addr_size_ == 8 ? sizeof(Elf64_Phdr) : sizeof(Elf32_Phdr));
  
  D16(program_count_label_);
  
  D16(addr_size_ == 8 ? sizeof(Elf64_Shdr) : sizeof(Elf32_Shdr));
  
  D16(section_count_label_);
  
  D16(section_header_string_index_);

  
  Section shn_undef;
  AddSection("", shn_undef, SHT_NULL);
}

int ELF::AddSection(const string& name, const Section& section,
                    uint32_t type, uint32_t flags, uint64_t addr,
                    uint32_t link, uint64_t entsize, uint64_t offset) {
  Label offset_label;
  Label string_label(section_header_strings_.Add(name));
  size_t size = section.Size();

  int index = section_count_;
  ++section_count_;

  section_header_table_
    
    .D32(string_label)
    
    .D32(type)
    
    .Append(endianness(), addr_size_, flags)
    
    .Append(endianness(), addr_size_, addr)
    
    .Append(endianness(), addr_size_, offset_label)
    
    .Append(endianness(), addr_size_, size)
    
    .D32(link)
    
    .D32(0)
    
    .Append(endianness(), addr_size_, 0)
    
    .Append(endianness(), addr_size_, entsize);

  
  
  if (type == SHT_NULL) {
    offset_label = 0;
  } else if (type == SHT_NOBITS) {
    offset_label = offset;
  } else {
    Mark(&offset_label);
    Append(section);
    Align(4);
  }
  return index;
}

void ELF::Finish() {
  
  section_header_string_index_ = section_count_;
  
  AddSection(".shstrtab", section_header_strings_, SHT_STRTAB);
  
  
  section_count_label_ = section_count_;
  program_count_label_ = program_count_;
  
  program_header_label_ = 0;

  
  Mark(&section_header_label_);
  Append(section_header_table_);
}

SymbolTable::SymbolTable(Endianness endianness,
                         size_t addr_size,
                         StringTable& table) : Section(endianness),
                                               addr_size_(addr_size),
                                               table_(table) {
  assert(addr_size_ == 4 || addr_size_ == 8);
}

void SymbolTable::AddSymbol(const string& name, uint32_t value,
                            uint32_t size, unsigned info, uint16_t shndx) {
  assert(addr_size_ == 4);
  D32(table_.Add(name));
  D32(value);
  D32(size);
  D8(info);
  D8(0); 
  D16(shndx);
}

void SymbolTable::AddSymbol(const string& name, uint64_t value,
                            uint64_t size, unsigned info, uint16_t shndx) {
  assert(addr_size_ == 8);
  D32(table_.Add(name));
  D8(info);
  D8(0); 
  D16(shndx);
  D64(value);
  D64(size);
}

BuildIDNote::BuildIDNote(const uint8_t* id_bytes,
                         size_t id_size,
                         Endianness endianness) : Section(endianness) {
  const char kNoteName[] = "GNU";
  
  Elf32_Nhdr note_header;
  memset(&note_header, 0, sizeof(note_header));
  note_header.n_namesz = sizeof(kNoteName);
  note_header.n_descsz = id_size;
  note_header.n_type = NT_GNU_BUILD_ID;

  Append(reinterpret_cast<const uint8_t*>(&note_header),
         sizeof(note_header));
  AppendCString(kNoteName);
  Append(id_bytes, id_size);
}


void BuildIDNote::AppendSection(ELF& elf,
                                const uint8_t* id_bytes,
                                size_t id_size) {
  const char kBuildIDSectionName[] = ".note.gnu.build-id";
  BuildIDNote note(id_bytes, id_size, elf.endianness());
  elf.AddSection(kBuildIDSectionName, note, SHT_NOTE);
}

}  
}  
