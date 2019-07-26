



#include <cstring>
#include <sys/mman.h>
#include <vector>
#include <dlfcn.h>
#include "CustomElf.h"
#include "Mappable.h"
#include "Logging.h"

using namespace Elf;
using namespace mozilla;





#ifdef ANDROID
extern "C" {
  void report_mapping(char *name, void *base, uint32_t len, uint32_t offset);
}
#else
#define report_mapping(...)
#endif

const Ehdr *Ehdr::validate(const void *buf)
{
  if (!buf || buf == MAP_FAILED)
    return nullptr;

  const Ehdr *ehdr = reinterpret_cast<const Ehdr *>(buf);

  
  if (memcmp(ELFMAG, &ehdr->e_ident, SELFMAG) ||
      ehdr->e_ident[EI_CLASS] != ELFCLASS ||
      ehdr->e_ident[EI_DATA] != ELFDATA ||
      ehdr->e_ident[EI_VERSION] != 1 ||
      (ehdr->e_ident[EI_OSABI] != ELFOSABI && ehdr->e_ident[EI_OSABI] != ELFOSABI_NONE) ||
#ifdef EI_ABIVERSION
      ehdr->e_ident[EI_ABIVERSION] != ELFABIVERSION ||
#endif
      (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) ||
      ehdr->e_machine != ELFMACHINE ||
      ehdr->e_version != 1 ||
      ehdr->e_phentsize != sizeof(Phdr))
    return nullptr;

  return ehdr;
}

namespace {

void debug_phdr(const char *type, const Phdr *phdr)
{
  DEBUG_LOG("%s @0x%08" PRIxAddr " ("
            "filesz: 0x%08" PRIxAddr ", "
            "memsz: 0x%08" PRIxAddr ", "
            "offset: 0x%08" PRIxAddr ", "
            "flags: %c%c%c)",
            type, phdr->p_vaddr, phdr->p_filesz, phdr->p_memsz,
            phdr->p_offset, phdr->p_flags & PF_R ? 'r' : '-',
            phdr->p_flags & PF_W ? 'w' : '-', phdr->p_flags & PF_X ? 'x' : '-');
}

void
__void_stub(void)
{
}

} 





class Mappable1stPagePtr: public GenericMappedPtr<Mappable1stPagePtr> {
public:
  Mappable1stPagePtr(Mappable *mappable)
  : GenericMappedPtr<Mappable1stPagePtr>(
      mappable->mmap(nullptr, PageSize(), PROT_READ, MAP_PRIVATE, 0))
  , mappable(mappable)
  {
    
    mappable->ensure(*this);
  }

private:
  friend class GenericMappedPtr<Mappable1stPagePtr>;
  void munmap(void *buf, size_t length) {
    mappable->munmap(buf, length);
  }

  mozilla::RefPtr<Mappable> mappable;
};


TemporaryRef<LibHandle>
CustomElf::Load(Mappable *mappable, const char *path, int flags)
{
  DEBUG_LOG("CustomElf::Load(\"%s\", 0x%x) = ...", path, flags);
  if (!mappable)
    return nullptr;
  

  RefPtr<CustomElf> elf = new CustomElf(mappable, path);
  
  Mappable1stPagePtr ehdr_raw(mappable);
  if (ehdr_raw == MAP_FAILED)
    return nullptr;

  const Ehdr *ehdr = Ehdr::validate(ehdr_raw);
  if (!ehdr)
    return nullptr;

  
  std::vector<const Phdr *> pt_loads;
  Addr min_vaddr = (Addr) -1; 
  Addr max_vaddr = 0;         
  const Phdr *dyn = nullptr;

  const Phdr *first_phdr = reinterpret_cast<const Phdr *>(
                           reinterpret_cast<const char *>(ehdr) + ehdr->e_phoff);
  const Phdr *end_phdr = &first_phdr[ehdr->e_phnum];
#ifdef __ARM_EABI__
  const Phdr *arm_exidx_phdr = nullptr;
#endif

  for (const Phdr *phdr = first_phdr; phdr < end_phdr; phdr++) {
    switch (phdr->p_type) {
      case PT_LOAD:
        debug_phdr("PT_LOAD", phdr);
        pt_loads.push_back(phdr);
        if (phdr->p_vaddr < min_vaddr)
          min_vaddr = phdr->p_vaddr;
        if (max_vaddr < phdr->p_vaddr + phdr->p_memsz)
          max_vaddr = phdr->p_vaddr + phdr->p_memsz;
        break;
      case PT_DYNAMIC:
        debug_phdr("PT_DYNAMIC", phdr);
        if (!dyn) {
          dyn = phdr;
        } else {
          LOG("%s: Multiple PT_DYNAMIC segments detected", elf->GetPath());
          return nullptr;
        }
        break;
      case PT_TLS:
        debug_phdr("PT_TLS", phdr);
        if (phdr->p_memsz) {
          LOG("%s: TLS is not supported", elf->GetPath());
          return nullptr;
        }
        break;
      case PT_GNU_STACK:
        debug_phdr("PT_GNU_STACK", phdr);

#ifndef ANDROID
        if (phdr->p_flags & PF_X) {
          LOG("%s: Executable stack is not supported", elf->GetPath());
          return nullptr;
        }
#endif
        break;
#ifdef __ARM_EABI__
      case PT_ARM_EXIDX:
        

        arm_exidx_phdr = phdr;
        break;
#endif
      default:
        DEBUG_LOG("%s: Warning: program header type #%d not handled",
                  elf->GetPath(), phdr->p_type);
    }
  }

  if (min_vaddr != 0) {
    LOG("%s: Unsupported minimal virtual address: 0x%08" PRIxAddr,
        elf->GetPath(), min_vaddr);
    return nullptr;
  }
  if (!dyn) {
    LOG("%s: No PT_DYNAMIC segment found", elf->GetPath());
    return nullptr;
  }

  









  elf->base.Assign(MemoryRange::mmap(nullptr, max_vaddr, PROT_NONE,
                                     MAP_SHARED | MAP_ANONYMOUS, -1, 0));
  if ((elf->base == MAP_FAILED) ||
      (mmap(elf->base, max_vaddr, PROT_NONE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) != elf->base)) {
    LOG("%s: Failed to mmap", elf->GetPath());
    return nullptr;
  }

  
  for (std::vector<const Phdr *>::iterator it = pt_loads.begin();
       it < pt_loads.end(); ++it)
    if (!elf->LoadSegment(*it))
      return nullptr;

  
  mappable->finalize();

  report_mapping(const_cast<char *>(elf->GetName()), elf->base,
                 (max_vaddr + PAGE_SIZE - 1) & PAGE_MASK, 0);

  elf->l_addr = elf->base;
  elf->l_name = elf->GetPath();
  elf->l_ld = elf->GetPtr<Dyn>(dyn->p_vaddr);
  ElfLoader::Singleton.Register(elf);

  if (!elf->InitDyn(dyn))
    return nullptr;

#ifdef __ARM_EABI__
  if (arm_exidx_phdr)
    elf->arm_exidx.InitSize(elf->GetPtr(arm_exidx_phdr->p_vaddr),
                            arm_exidx_phdr->p_memsz);
#endif

  elf->stats("oneLibLoaded");
  DEBUG_LOG("CustomElf::Load(\"%s\", 0x%x) = %p", path, flags,
            static_cast<void *>(elf));
  return elf;
}

CustomElf::~CustomElf()
{
  DEBUG_LOG("CustomElf::~CustomElf(%p [\"%s\"])",
            reinterpret_cast<void *>(this), GetPath());
  CallFini();
  


  ElfLoader::__wrap_cxa_finalize(this);
  ElfLoader::Singleton.Forget(this);
}

namespace {




unsigned long
ElfHash(const char *symbol)
{
  const unsigned char *sym = reinterpret_cast<const unsigned char *>(symbol);
  unsigned long h = 0, g;
  while (*sym) {
    h = (h << 4) + *sym++;
    if ((g = h & 0xf0000000))
      h ^= g >> 24;
    h &= ~g;
  }
  return h;
}

} 

void *
CustomElf::GetSymbolPtr(const char *symbol) const
{
  return GetSymbolPtr(symbol, ElfHash(symbol));
}

void *
CustomElf::GetSymbolPtr(const char *symbol, unsigned long hash) const
{
  const Sym *sym = GetSymbol(symbol, hash);
  void *ptr = nullptr;
  if (sym && sym->st_shndx != SHN_UNDEF)
    ptr = GetPtr(sym->st_value);
  DEBUG_LOG("CustomElf::GetSymbolPtr(%p [\"%s\"], \"%s\") = %p",
            reinterpret_cast<const void *>(this), GetPath(), symbol, ptr);
  return ptr;
}

void *
CustomElf::GetSymbolPtrInDeps(const char *symbol) const
{
  
  if (symbol[0] == 'd' && symbol[1] == 'l') {
    if (strcmp(symbol + 2, "open") == 0)
      return FunctionPtr(__wrap_dlopen);
    if (strcmp(symbol + 2, "error") == 0)
      return FunctionPtr(__wrap_dlerror);
    if (strcmp(symbol + 2, "close") == 0)
      return FunctionPtr(__wrap_dlclose);
    if (strcmp(symbol + 2, "sym") == 0)
      return FunctionPtr(__wrap_dlsym);
    if (strcmp(symbol + 2, "addr") == 0)
      return FunctionPtr(__wrap_dladdr);
    if (strcmp(symbol + 2, "_iterate_phdr") == 0)
      return FunctionPtr(__wrap_dl_iterate_phdr);
  } else if (symbol[0] == '_' && symbol[1] == '_') {
  
#ifdef __ARM_EABI__
    if (strcmp(symbol + 2, "aeabi_atexit") == 0)
      return FunctionPtr(&ElfLoader::__wrap_aeabi_atexit);
#else
    if (strcmp(symbol + 2, "cxa_atexit") == 0)
      return FunctionPtr(&ElfLoader::__wrap_cxa_atexit);
#endif
    if (strcmp(symbol + 2, "cxa_finalize") == 0)
      return FunctionPtr(&ElfLoader::__wrap_cxa_finalize);
    if (strcmp(symbol + 2, "dso_handle") == 0)
      return const_cast<CustomElf *>(this);
    if (strcmp(symbol + 2, "moz_linker_stats") == 0)
      return FunctionPtr(&ElfLoader::stats);
#ifdef __ARM_EABI__
    if (strcmp(symbol + 2, "gnu_Unwind_Find_exidx") == 0)
      return FunctionPtr(__wrap___gnu_Unwind_Find_exidx);
#endif
  }

#define MISSING_FLASH_SYMNAME_START "_ZN7android10VectorImpl19reservedVectorImpl"

  
  
  if (strncmp(symbol,
              MISSING_FLASH_SYMNAME_START,
              sizeof(MISSING_FLASH_SYMNAME_START) - 1) == 0) {
    return FunctionPtr(__void_stub);
  }

  void *sym;
  



#ifdef __GLIBC__
  sym = dlsym(RTLD_DEFAULT, symbol);
  DEBUG_LOG("dlsym(RTLD_DEFAULT, \"%s\") = %p", symbol, sym);
  if (sym)
    return sym;
#endif

  





  unsigned long hash = ElfHash(symbol);
  for (std::vector<RefPtr<LibHandle> >::const_iterator it = dependencies.begin();
       it < dependencies.end(); ++it) {
    if (!(*it)->IsSystemElf()) {
      sym = reinterpret_cast<CustomElf *>((*it).get())->GetSymbolPtr(symbol, hash);
#ifndef __GLIBC__
    } else {
      sym = (*it)->GetSymbolPtr(symbol);
#endif
    }
    if (sym)
      return sym;
  }
  return nullptr;
}

const Sym *
CustomElf::GetSymbol(const char *symbol, unsigned long hash) const
{
  







  size_t bucket = hash % buckets.numElements();
  for (size_t y = buckets[bucket]; y != STN_UNDEF; y = chains[y]) {
    if (strcmp(symbol, strtab.GetStringAt(symtab[y].st_name)))
      continue;
    return &symtab[y];
  }
  return nullptr;
}

bool
CustomElf::Contains(void *addr) const
{
  return base.Contains(addr);
}

#ifdef __ARM_EABI__
const void *
CustomElf::FindExidx(int *pcount) const
{
  if (arm_exidx) {
    *pcount = arm_exidx.numElements();
    return arm_exidx;
  }
  *pcount = 0;
  return nullptr;
}
#endif

void
CustomElf::stats(const char *when) const
{
  mappable->stats(when, GetPath());
}

bool
CustomElf::LoadSegment(const Phdr *pt_load) const
{
  if (pt_load->p_type != PT_LOAD) {
    DEBUG_LOG("%s: Elf::LoadSegment only takes PT_LOAD program headers", GetPath());
    return false;;
  }

  int prot = ((pt_load->p_flags & PF_X) ? PROT_EXEC : 0) |
             ((pt_load->p_flags & PF_W) ? PROT_WRITE : 0) |
             ((pt_load->p_flags & PF_R) ? PROT_READ : 0);

  
  Addr align = PageSize();
  Addr align_offset;
  void *mapped, *where;
  do {
    align_offset = pt_load->p_vaddr - AlignedPtr(pt_load->p_vaddr, align);
    where = GetPtr(pt_load->p_vaddr - align_offset);
    DEBUG_LOG("%s: Loading segment @%p %c%c%c", GetPath(), where,
                                                prot & PROT_READ ? 'r' : '-',
                                                prot & PROT_WRITE ? 'w' : '-',
                                                prot & PROT_EXEC ? 'x' : '-');
    mapped = mappable->mmap(where, pt_load->p_filesz + align_offset,
                            prot, MAP_PRIVATE | MAP_FIXED,
                            pt_load->p_offset - align_offset);
    if ((mapped != MAP_FAILED) || (pt_load->p_vaddr == 0) ||
        (pt_load->p_align == align))
      break;
    




    DEBUG_LOG("%s: Failed to mmap, retrying", GetPath());
    align = pt_load->p_align;
  } while (1);

  if (mapped != where) {
    if (mapped == MAP_FAILED) {
      LOG("%s: Failed to mmap", GetPath());
    } else {
      LOG("%s: Didn't map at the expected location (wanted: %p, got: %p)",
          GetPath(), where, mapped);
    }
    return false;
  }

  


  const char *ondemand = getenv("MOZ_LINKER_ONDEMAND");
  if (!ElfLoader::Singleton.hasRegisteredHandler() ||
      (ondemand && !strncmp(ondemand, "0", 2 ))) {
    for (Addr off = 0; off < pt_load->p_filesz + align_offset;
         off += PageSize()) {
      mappable->ensure(reinterpret_cast<char *>(mapped) + off);
    }
  }
  





  if (pt_load->p_memsz > pt_load->p_filesz) {
    Addr file_end = pt_load->p_vaddr + pt_load->p_filesz;
    Addr mem_end = pt_load->p_vaddr + pt_load->p_memsz;
    Addr next_page = PageAlignedEndPtr(file_end);
    if (next_page > file_end) {
      

      void *ptr = GetPtr(file_end);
      mappable->ensure(ptr);
      memset(ptr, 0, next_page - file_end);
    }
    if (mem_end > next_page) {
      if (mprotect(GetPtr(next_page), mem_end - next_page, prot) < 0) {
        LOG("%s: Failed to mprotect", GetPath());
        return false;
      }
    }
  }
  return true;
}

namespace {

void debug_dyn(const char *type, const Dyn *dyn)
{
  DEBUG_LOG("%s 0x%08" PRIxAddr, type, dyn->d_un.d_val);
}

} 

bool
CustomElf::InitDyn(const Phdr *pt_dyn)
{
  
  const Dyn *first_dyn = GetPtr<Dyn>(pt_dyn->p_vaddr);
  const Dyn *end_dyn = GetPtr<Dyn>(pt_dyn->p_vaddr + pt_dyn->p_filesz);
  std::vector<Word> dt_needed;
  size_t symnum = 0;
  for (const Dyn *dyn = first_dyn; dyn < end_dyn && dyn->d_tag; dyn++) {
    switch (dyn->d_tag) {
      case DT_NEEDED:
        debug_dyn("DT_NEEDED", dyn);
        dt_needed.push_back(dyn->d_un.d_val);
        break;
      case DT_HASH:
        {
          debug_dyn("DT_HASH", dyn);
          const Word *hash_table_header = GetPtr<Word>(dyn->d_un.d_ptr);
          symnum = hash_table_header[1];
          buckets.Init(&hash_table_header[2], hash_table_header[0]);
          chains.Init(&*buckets.end());
        }
        break;
      case DT_STRTAB:
        debug_dyn("DT_STRTAB", dyn);
        strtab.Init(GetPtr(dyn->d_un.d_ptr));
        break;
      case DT_SYMTAB:
        debug_dyn("DT_SYMTAB", dyn);
        symtab.Init(GetPtr(dyn->d_un.d_ptr));
        break;
      case DT_SYMENT:
        debug_dyn("DT_SYMENT", dyn);
        if (dyn->d_un.d_val != sizeof(Sym)) {
          LOG("%s: Unsupported DT_SYMENT", GetPath());
          return false;
        }
        break;
      case DT_TEXTREL:
        LOG("%s: Text relocations are not supported", GetPath());
        return false;
      case DT_STRSZ: 
        debug_dyn("DT_STRSZ", dyn);
        break;
      case UNSUPPORTED_RELOC():
      case UNSUPPORTED_RELOC(SZ):
      case UNSUPPORTED_RELOC(ENT):
        LOG("%s: Unsupported relocations", GetPath());
        return false;
      case RELOC():
        debug_dyn(STR_RELOC(), dyn);
        relocations.Init(GetPtr(dyn->d_un.d_ptr));
        break;
      case RELOC(SZ):
        debug_dyn(STR_RELOC(SZ), dyn);
        relocations.InitSize(dyn->d_un.d_val);
        break;
      case RELOC(ENT):
        debug_dyn(STR_RELOC(ENT), dyn);
        if (dyn->d_un.d_val != sizeof(Reloc)) {
          LOG("%s: Unsupported DT_RELENT", GetPath());
          return false;
        }
        break;
      case DT_JMPREL:
        debug_dyn("DT_JMPREL", dyn);
        jumprels.Init(GetPtr(dyn->d_un.d_ptr));
        break;
      case DT_PLTRELSZ:
        debug_dyn("DT_PLTRELSZ", dyn);
        jumprels.InitSize(dyn->d_un.d_val);
        break;
      case DT_PLTGOT:
        debug_dyn("DT_PLTGOT", dyn);
        break;
      case DT_INIT:
        debug_dyn("DT_INIT", dyn);
        init = dyn->d_un.d_ptr;
        break;
      case DT_INIT_ARRAY:
        debug_dyn("DT_INIT_ARRAY", dyn);
        init_array.Init(GetPtr(dyn->d_un.d_ptr));
        break;
      case DT_INIT_ARRAYSZ:
        debug_dyn("DT_INIT_ARRAYSZ", dyn);
        init_array.InitSize(dyn->d_un.d_val);
        break;
      case DT_FINI:
        debug_dyn("DT_FINI", dyn);
        fini = dyn->d_un.d_ptr;
        break;
      case DT_FINI_ARRAY:
        debug_dyn("DT_FINI_ARRAY", dyn);
        fini_array.Init(GetPtr(dyn->d_un.d_ptr));
        break;
      case DT_FINI_ARRAYSZ:
        debug_dyn("DT_FINI_ARRAYSZ", dyn);
        fini_array.InitSize(dyn->d_un.d_val);
        break;
      case DT_PLTREL:
        if (dyn->d_un.d_val != RELOC()) {
          LOG("%s: Error: DT_PLTREL is not " STR_RELOC(), GetPath());
          return false;
        }
        break;
      case DT_FLAGS:
        {
           Addr flags = dyn->d_un.d_val;
           
           if (flags & DF_TEXTREL) {
             LOG("%s: Text relocations are not supported", GetPath());
             return false;
           }
           
           flags &= ~DF_SYMBOLIC;
           if (flags)
             LOG("%s: Warning: unhandled flags #%" PRIxAddr" not handled",
                 GetPath(), flags);
        }
        break;
      case DT_SONAME: 
      case DT_SYMBOLIC: 


      case RELOC(COUNT): 


      case UNSUPPORTED_RELOC(COUNT): 

      case DT_FLAGS_1: 

      case DT_VERSYM: 
      case DT_VERDEF: 
      case DT_VERDEFNUM:
      case DT_VERNEED:
      case DT_VERNEEDNUM:
        
        break;
      default:
        LOG("%s: Warning: dynamic header type #%" PRIxAddr" not handled",
            GetPath(), dyn->d_tag);
    }
  }

  if (!buckets || !symnum) {
    LOG("%s: Missing or broken DT_HASH", GetPath());
    return false;
  }
  if (!strtab) {
    LOG("%s: Missing DT_STRTAB", GetPath());
    return false;
  }
  if (!symtab) {
    LOG("%s: Missing DT_SYMTAB", GetPath());
    return false;
  }

  
  for (size_t i = 0; i < dt_needed.size(); i++) {
    const char *name = strtab.GetStringAt(dt_needed[i]);
    RefPtr<LibHandle> handle =
      ElfLoader::Singleton.Load(name, RTLD_GLOBAL | RTLD_LAZY, this);
    if (!handle)
      return false;
    dependencies.push_back(handle);
  }

  
  return Relocate() && RelocateJumps() && CallInit();
}

bool
CustomElf::Relocate()
{
  DEBUG_LOG("Relocate %s @%p", GetPath(), static_cast<void *>(base));
  uint32_t symtab_index = (uint32_t) -1;
  void *symptr = nullptr;
  for (Array<Reloc>::iterator rel = relocations.begin();
       rel < relocations.end(); ++rel) {
    
    void *ptr = GetPtr(rel->r_offset);

    
    if (ELF_R_TYPE(rel->r_info) == R_RELATIVE) {
      *(void **) ptr = GetPtr(rel->GetAddend(base));
      continue;
    }
    
    
    if (symtab_index != ELF_R_SYM(rel->r_info)) {
      symtab_index = ELF_R_SYM(rel->r_info);
      const Sym sym = symtab[symtab_index];
      if (sym.st_shndx != SHN_UNDEF) {
        symptr = GetPtr(sym.st_value);
      } else {
        
        symptr = GetSymbolPtrInDeps(strtab.GetStringAt(sym.st_name));
      }
    }

    if (symptr == nullptr)
      LOG("%s: Warning: relocation to NULL @0x%08" PRIxAddr,
          GetPath(), rel->r_offset);

    
    switch (ELF_R_TYPE(rel->r_info)) {
    case R_GLOB_DAT:
      
      *(void **) ptr = symptr;
      break;
    case R_ABS:
      
      *(const char **) ptr = (const char *)symptr + rel->GetAddend(base);
      break;
    default:
      LOG("%s: Unsupported relocation type: 0x%" PRIxAddr,
          GetPath(), ELF_R_TYPE(rel->r_info));
      return false;
    }
  }
  return true;
}

bool
CustomElf::RelocateJumps()
{
  
  for (Array<Reloc>::iterator rel = jumprels.begin();
       rel < jumprels.end(); ++rel) {
    
    void *ptr = GetPtr(rel->r_offset);

    
    if (ELF_R_TYPE(rel->r_info) != R_JMP_SLOT) {
      LOG("%s: Jump relocation type mismatch", GetPath());
      return false;
    }

    
    const Sym sym = symtab[ELF_R_SYM(rel->r_info)];
    void *symptr;
    if (sym.st_shndx != SHN_UNDEF)
      symptr = GetPtr(sym.st_value);
    else
      symptr = GetSymbolPtrInDeps(strtab.GetStringAt(sym.st_name));

    if (symptr == nullptr) {
      LOG("%s: %s: relocation to NULL @0x%08" PRIxAddr " for symbol \"%s\"",
          GetPath(),
          (ELF_ST_BIND(sym.st_info) == STB_WEAK) ? "Warning" : "Error",
          rel->r_offset, strtab.GetStringAt(sym.st_name));
      if (ELF_ST_BIND(sym.st_info) != STB_WEAK)
        return false;
    }
    
    *(void **) ptr = symptr;
  }
  return true;
}

bool
CustomElf::CallInit()
{
  if (init)
    CallFunction(init);

  for (Array<void *>::iterator it = init_array.begin();
       it < init_array.end(); ++it) {
    
    if (*it && *it != reinterpret_cast<void *>(-1))
      CallFunction(*it);
  }
  initialized = true;
  return true;
}

void
CustomElf::CallFini()
{
  if (!initialized)
    return;
  for (Array<void *>::reverse_iterator it = fini_array.rbegin();
       it < fini_array.rend(); ++it) {
    
    if (*it && *it != reinterpret_cast<void *>(-1))
      CallFunction(*it);
  }
  if (fini)
    CallFunction(fini);
}

Mappable *
CustomElf::GetMappable() const
{
  if (!mappable)
    return nullptr;
  if (mappable->GetKind() == Mappable::MAPPABLE_EXTRACT_FILE)
    return mappable;
  return ElfLoader::GetMappableFromPath(GetPath());
}
