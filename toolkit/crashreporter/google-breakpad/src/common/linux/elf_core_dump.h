































#ifndef COMMON_LINUX_ELF_CORE_DUMP_H_
#define COMMON_LINUX_ELF_CORE_DUMP_H_

#include <elf.h>
#include <link.h>
#include <stddef.h>

#include "common/memory_range.h"

namespace google_breakpad {



class ElfCoreDump {
 public:
  
  typedef ElfW(Ehdr) Ehdr;
  typedef ElfW(Nhdr) Nhdr;
  typedef ElfW(Phdr) Phdr;
  typedef ElfW(Word) Word;
  typedef ElfW(Addr) Addr;
#if __WORDSIZE == 32
  static const int kClass = ELFCLASS32;
#elif __WORDSIZE == 64
  static const int kClass = ELFCLASS64;
#else
#error "Unsupported __WORDSIZE for ElfCoreDump."
#endif

  
  
  class Note {
   public:
    Note();

    
    explicit Note(const MemoryRange& content);

    
    
    bool IsValid() const;

    
    
    const Nhdr* GetHeader() const;

    
    Word GetType() const;

    
    
    MemoryRange GetName() const;

    
    
    MemoryRange GetDescription() const;

    
    
    Note GetNextNote() const;

   private:
    
    
    static size_t AlignedSize(size_t size);

    
    MemoryRange content_;
  };

  ElfCoreDump();

  
  explicit ElfCoreDump(const MemoryRange& content);

  
  void SetContent(const MemoryRange& content);

  
  bool IsValid() const;

  
  
  const Ehdr* GetHeader() const;

  
  
  const Phdr* GetProgramHeader(unsigned index) const;

  
  
  
  const Phdr* GetFirstProgramHeaderOfType(Word type) const;

  
  
  unsigned GetProgramHeaderCount() const;

  
  
  
  
  bool CopyData(void* buffer, Addr virtual_address, size_t length);

  
  
  Note GetFirstNote() const;

 private:
  
  MemoryRange content_;
};

}  

#endif  
