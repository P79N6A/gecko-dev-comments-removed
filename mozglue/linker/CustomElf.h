



#ifndef CustomElf_h
#define CustomElf_h

#include "ElfLoader.h"
#include "Logging.h"
#include "Elfxx.h"





class CustomElf: public LibHandle, private ElfLoader::link_map
{
  friend class ElfLoader;
  friend class SEGVHandler;
public:
  








  static mozilla::TemporaryRef<LibHandle> Load(Mappable *mappable,
                                               const char *path, int flags);

  


  virtual ~CustomElf();
  virtual void *GetSymbolPtr(const char *symbol) const;
  virtual bool Contains(void *addr) const;
  virtual void *GetBase() const { return GetPtr(0); }

#ifdef __ARM_EABI__
  virtual const void *FindExidx(int *pcount) const;
#endif

protected:
  virtual Mappable *GetMappable() const;

public:
  




  void stats(const char *when) const;

private:
  



  const Elf::Sym *GetSymbol(const char *symbol, unsigned long hash) const;

  



  void *GetSymbolPtr(const char *symbol, unsigned long hash) const;

  




  void *GetSymbolPtrInDeps(const char *symbol) const;

  


  CustomElf(Mappable *mappable, const char *path)
  : LibHandle(path)
  , mappable(mappable)
  , init(0)
  , fini(0)
  , initialized(false)
  , has_text_relocs(false)
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
    DEBUG_LOG("%s: Calling function @%p", GetPath(), ptr);
    f.func();
  }

  


  void CallFunction(Elf::Addr addr) const
  {
    return CallFunction(GetPtr(addr));
  }

  
  mozilla::RefPtr<Mappable> mappable;

  
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

  bool has_text_relocs;

#ifdef __ARM_EABI__
  
  Array<uint32_t[2]> arm_exidx;
#endif
};

#endif 
