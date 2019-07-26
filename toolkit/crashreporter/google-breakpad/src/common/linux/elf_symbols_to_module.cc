































#include "common/linux/elf_symbols_to_module.h"

#include <elf.h>
#include <string.h>

#include "common/byte_cursor.h"
#include "common/module.h"

namespace google_breakpad {

class ELFSymbolIterator {
public:
  
  
  struct Symbol {
    
    
    bool at_end;

    
    size_t index;

    
    
    size_t name_offset;

    
    uint64_t value;
    uint64_t size;
    unsigned char info;
    uint16_t shndx;
  };

  
  
  
  
  
  ELFSymbolIterator(const ByteBuffer *buffer, bool big_endian,
                    size_t value_size)
    : value_size_(value_size), cursor_(buffer, big_endian) {
    
    
    assert(value_size == 4 || value_size == 8);
    symbol_.index = 0;
    Fetch();
  }

  
  
  ELFSymbolIterator &operator++() { Fetch(); symbol_.index++; return *this; }

  
  
  
  const Symbol &operator*() const { return symbol_; }
  const Symbol *operator->() const { return &symbol_; }

private:
  
  void Fetch() {
    
    unsigned char other;
    if (value_size_ == 4) {
      
      cursor_
        .Read(4, false, &symbol_.name_offset)
        .Read(4, false, &symbol_.value)
        .Read(4, false, &symbol_.size)
        .Read(1, false, &symbol_.info)
        .Read(1, false, &other)
        .Read(2, false, &symbol_.shndx);
    } else {
      
      cursor_
        .Read(4, false, &symbol_.name_offset)
        .Read(1, false, &symbol_.info)
        .Read(1, false, &other)
        .Read(2, false, &symbol_.shndx)
        .Read(8, false, &symbol_.value)
        .Read(8, false, &symbol_.size);
    }
    symbol_.at_end = !cursor_;
  }

  
  size_t value_size_;

  
  ByteCursor cursor_;

  
  Symbol symbol_;
};

const char *SymbolString(ptrdiff_t offset, ByteBuffer& strings) {
  if (offset < 0 || (size_t) offset >= strings.Size()) {
    
    offset = 0;
  }
  return reinterpret_cast<const char *>(strings.start + offset);
}

bool ELFSymbolsToModule(const uint8_t *symtab_section,
                        size_t symtab_size,
                        const uint8_t *string_section,
                        size_t string_size,
                        const bool big_endian,
                        size_t value_size,
                        Module *module) {
  ByteBuffer symbols(symtab_section, symtab_size);
  
  if (string_section[string_size - 1] != '\0') {
    const void* null_terminator = memrchr(string_section, '\0', string_size);
    string_size = reinterpret_cast<const uint8_t*>(null_terminator)
      - string_section;
  }
  ByteBuffer strings(string_section, string_size);

  
  ELFSymbolIterator iterator(&symbols, big_endian, value_size);

  while(!iterator->at_end) {
    if (ELF32_ST_TYPE(iterator->info) == STT_FUNC &&
        iterator->shndx != SHN_UNDEF) {
      Module::Extern *ext = new Module::Extern;
      ext->name = SymbolString(iterator->name_offset, strings);
      ext->address = iterator->value;
      module->AddExtern(ext);
    }
    ++iterator;
  }
  return true;
}

}  
