



#ifndef BaseElf_h
#define BaseElf_h

#include "ElfLoader.h"
#include "Elfxx.h"






class BaseElf: public LibHandle
{
public:
  


  static unsigned long Hash(const char *symbol);

  



  void *GetSymbolPtr(const char *symbol, unsigned long hash) const;

  



  const Elf::Sym *GetSymbol(const char *symbol, unsigned long hash) const;

  BaseElf(const char *path, Mappable *mappable = nullptr)
  : LibHandle(path)
  , mappable(mappable)
  {
  }

protected:
   



   virtual void *GetSymbolPtr(const char *symbol) const;
   virtual bool Contains(void *addr) const;
   virtual void *GetBase() const { return GetPtr(0); }

#ifdef __ARM_EABI__
  virtual const void *FindExidx(int *pcount) const;
#endif

  virtual Mappable *GetMappable() const { return NULL; };

public:

  



  void *GetPtr(const Elf::Addr offset) const
  {
    if (reinterpret_cast<void *>(offset) > base)
      return reinterpret_cast<void *>(offset);
    return base + offset;
  }

  


  template <typename T>
  const T *GetPtr(const Elf::Addr offset) const
  {
    if (reinterpret_cast<void *>(offset) > base)
      return reinterpret_cast<const T *>(offset);
    return reinterpret_cast<const T *>(base + offset);
  }

  
  

  mozilla::RefPtr<Mappable> mappable;

  
  MappedPtr base;

  
  Array<Elf::Word> buckets;
  UnsizedArray<Elf::Word> chains;


  
  Elf::Strtab strtab;

  
  UnsizedArray<Elf::Sym> symtab;

#ifdef __ARM_EABI__
  
  Array<uint32_t[2]> arm_exidx;
#endif
};





class LoadedElf: public BaseElf
{
public:
  



  static already_AddRefed<LibHandle> Create(const char *path,
                                                 void *base_addr);

private:
  LoadedElf(const char *path)
  : BaseElf(path) { }

  ~LoadedElf()
  {
    

    base.release();
    ElfLoader::Singleton.Forget(this);
  }

  




  bool InitDyn(const Elf::Phdr *pt_dyn);
};


#endif 
