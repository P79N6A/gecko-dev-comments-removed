



#ifndef CustomElf_h
#define CustomElf_h

#include "ElfLoader.h"
#include "BaseElf.h"
#include "Logging.h"
#include "Elfxx.h"





class CustomElf: public BaseElf, private ElfLoader::link_map
{
  friend class ElfLoader;
  friend class SEGVHandler;
public:
  








  static already_AddRefed<LibHandle> Load(Mappable *mappable,
                                               const char *path, int flags);

  


  virtual ~CustomElf();

protected:
  virtual Mappable *GetMappable() const;

public:
  




  virtual void stats(const char *when) const;

  



  virtual BaseElf *AsBaseElf() { return this; }

private:
  




  void *GetSymbolPtrInDeps(const char *symbol) const;

  


  CustomElf(Mappable *mappable, const char *path)
  : BaseElf(path, mappable)
  , link_map()
  , init(0)
  , fini(0)
  , initialized(false)
  , has_text_relocs(false)
  { }

  



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

  
  std::vector<mozilla::RefPtr<LibHandle> > dependencies;

  
  Array<Elf::Reloc> relocations;

  
  Array<Elf::Reloc> jumprels;

  

  Elf::Addr init, fini;

  

  Array<void *> init_array, fini_array;

  bool initialized;

  bool has_text_relocs;
};

#endif 
