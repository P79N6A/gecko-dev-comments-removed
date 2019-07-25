



#ifndef CustomElf_h
#define CustomElf_h





#ifdef ANDROID
#include <linux/elf.h>
#else
#include <elf.h>
#endif
#include <endian.h>
#include "ElfLoader.h"
#include "Logging.h"




#ifdef HAVE_64BIT_OS
#define Elf_(type) Elf64_ ## type
#define ELFCLASS ELFCLASS64
#define ELF_R_TYPE ELF64_R_TYPE
#define ELF_R_SYM ELF64_R_SYM
#define PRIxAddr "lx"
#else
#define Elf_(type) Elf32_ ## type
#define ELFCLASS ELFCLASS32
#define ELF_R_TYPE ELF32_R_TYPE
#define ELF_R_SYM ELF32_R_SYM
#define PRIxAddr "x"
#endif

#ifndef __BYTE_ORDER
#error Cannot find endianness
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ELFDATA ELFDATA2LSB
#elif __BYTE_ORDER == __BIG_ENDIAN
#define ELFDATA ELFDATA2MSB
#endif

#ifdef __linux__
#define ELFOSABI ELFOSABI_LINUX
#ifdef EI_ABIVERSION
#define ELFABIVERSION 0
#endif
#else
#error Unknown ELF OSABI
#endif

#if defined(__i386__)
#define ELFMACHINE EM_386


#define R_ABS R_386_32
#define R_GLOB_DAT R_386_GLOB_DAT
#define R_JMP_SLOT R_386_JMP_SLOT
#define R_RELATIVE R_386_RELATIVE
#define RELOC(n) DT_REL ## n
#define UNSUPPORTED_RELOC(n) DT_RELA ## n
#define STR_RELOC(n) "DT_REL" # n
#define Reloc Rel

#elif defined(__x86_64__)
#define ELFMACHINE EM_X86_64

#define R_ABS R_X86_64_64
#define R_GLOB_DAT R_X86_64_GLOB_DAT
#define R_JMP_SLOT R_X86_64_JUMP_SLOT
#define R_RELATIVE R_X86_64_RELATIVE
#define RELOC(n) DT_RELA ## n
#define UNSUPPORTED_RELOC(n) DT_REL ## n
#define STR_RELOC(n) "DT_RELA" # n
#define Reloc Rela

#elif defined(__arm__)
#define ELFMACHINE EM_ARM

#ifndef R_ARM_ABS32
#define R_ARM_ABS32 2
#endif
#ifndef R_ARM_GLOB_DAT
#define R_ARM_GLOB_DAT 21
#endif
#ifndef R_ARM_JUMP_SLOT
#define R_ARM_JUMP_SLOT 22
#endif
#ifndef R_ARM_RELATIVE
#define R_ARM_RELATIVE 23
#endif

#define R_ABS R_ARM_ABS32
#define R_GLOB_DAT R_ARM_GLOB_DAT
#define R_JMP_SLOT R_ARM_JUMP_SLOT
#define R_RELATIVE R_ARM_RELATIVE
#define RELOC(n) DT_REL ## n
#define UNSUPPORTED_RELOC(n) DT_RELA ## n
#define STR_RELOC(n) "DT_REL" # n
#define Reloc Rel

#else
#error Unknown ELF machine type
#endif




#ifndef STN_UNDEF
#define STN_UNDEF 0
#endif

#ifndef DT_INIT_ARRAY
#define DT_INIT_ARRAY 25
#endif

#ifndef DT_FINI_ARRAY
#define DT_FINI_ARRAY 26
#endif

#ifndef DT_INIT_ARRAYSZ
#define DT_INIT_ARRAYSZ 27
#endif

#ifndef DT_FINI_ARRAYSZ
#define DT_FINI_ARRAYSZ 28
#endif

namespace Elf {




typedef Elf_(Phdr) Phdr;
typedef Elf_(Dyn) Dyn;
typedef Elf_(Sym) Sym;
typedef Elf_(Addr) Addr;
typedef Elf_(Word) Word;




struct Ehdr: public Elf_(Ehdr)
{
  




  static const Ehdr *validate(const void *buf);
};




class Strtab: public UnsizedArray<const char>
{
public:
  


  const char *GetStringAt(off_t index) const
  {
    return &UnsizedArray<const char>::operator[](index);
  }
};




struct Rel: public Elf_(Rel)
{
  



  Addr GetAddend(void *base) const
  {
    return *(reinterpret_cast<const Addr *>(
                   reinterpret_cast<const char *>(base) + r_offset));
  }
};




struct Rela: public Elf_(Rela)
{
  


  Addr GetAddend(void *base) const
  {
    return r_addend;
  }
};

} 





class CustomElf: public LibHandle
{
public:
  








  static mozilla::TemporaryRef<LibHandle> Load(int fd,
                                               const char *path, int flags);

  


  virtual ~CustomElf();
  virtual void *GetSymbolPtr(const char *symbol) const;
  virtual bool Contains(void *addr) const;

private:
  



  const Elf::Sym *GetSymbol(const char *symbol, unsigned long hash) const;

  



  void *GetSymbolPtr(const char *symbol, unsigned long hash) const;

  




  void *GetSymbolPtrInDeps(const char *symbol) const;

  


  CustomElf(int fd, const char *path)
  : LibHandle(path), fd(fd), init(0), fini(0), initialized(false)
  { }

  



  void *GetPtr(const Elf::Addr offset) const
  {
    return base + offset;
  }

  


  template <typename T>
  const T *GetPtr(const Elf::Addr offset) const
  {
    return reinterpret_cast<const T *>(base + offset);
  }

  



  bool LoadSegment(const Elf::Phdr *pt_load) const;

  




  bool InitDyn(const Elf::Phdr *pt_dyn);

  



  bool Relocate();

  



  bool RelocateJumps();

  



  bool CallInit();

  



  void CallFini();

  


  void CallFunction(void *ptr) const
  {
    

    union {
      void *ptr;
      void (*func)(void);
    } f;
    f.ptr = ptr;
    f.func();
  }

  


  void CallFunction(Elf::Addr addr) const
  {
    return CallFunction(GetPtr(addr));
  }

  
  AutoCloseFD fd;

  
  MappedPtr base;

  
  Elf::Strtab strtab;

  
  UnsizedArray<Elf::Sym> symtab;

  
  Array<Elf::Word> buckets;
  UnsizedArray<Elf::Word> chains;

  
  std::vector<mozilla::RefPtr<LibHandle> > dependencies;

  
  Array<Elf::Reloc> relocations;

  
  Array<Elf::Reloc> jumprels;

  

  Elf::Addr init, fini;

  

  Array<void *> init_array, fini_array;

  bool initialized;
};

#endif 
