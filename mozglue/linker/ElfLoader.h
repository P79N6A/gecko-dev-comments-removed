



#ifndef ElfLoader_h
#define ElfLoader_h

#include <vector>
#include <dlfcn.h>
#include <signal.h>
#include "mozilla/RefPtr.h"
#include "Zip.h"
#include "Elfxx.h"
#include "Mappable.h"




extern "C" {
  void *__wrap_dlopen(const char *path, int flags);
  const char *__wrap_dlerror(void);
  void *__wrap_dlsym(void *handle, const char *symbol);
  int __wrap_dlclose(void *handle);

#ifndef HAVE_DLADDR
  typedef struct {
    const char *dli_fname;
    void *dli_fbase;
    const char *dli_sname;
    void *dli_saddr;
  } Dl_info;
#endif
  int __wrap_dladdr(void *addr, Dl_info *info);

  struct dl_phdr_info {
    Elf::Addr dlpi_addr;
    const char *dlpi_name;
    const Elf::Phdr *dlpi_phdr;
    Elf::Half dlpi_phnum;
  };

  typedef int (*dl_phdr_cb)(struct dl_phdr_info *, size_t, void *);
  int __wrap_dl_iterate_phdr(dl_phdr_cb callback, void *data);

#ifdef __ARM_EABI__
  const void *__wrap___gnu_Unwind_Find_exidx(void *pc, int *pcount);
#endif




MFBT_API size_t
__dl_get_mappable_length(void *handle);

MFBT_API void *
__dl_mmap(void *handle, void *addr, size_t length, off_t offset);

MFBT_API void
__dl_munmap(void *handle, void *addr, size_t length);

MFBT_API bool
IsSignalHandlingBroken();

}







class LibHandle;

namespace mozilla {
namespace detail {

template <> inline void RefCounted<LibHandle, AtomicRefCount>::Release() const;

template <> inline RefCounted<LibHandle, AtomicRefCount>::~RefCounted()
{
  MOZ_ASSERT(mRefCnt == 0x7fffdead);
}

} 
} 





class LibHandle: public mozilla::external::AtomicRefCounted<LibHandle>
{
public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(LibHandle)
  



  LibHandle(const char *path)
  : directRefCnt(0), path(path ? strdup(path) : nullptr), mappable(nullptr) { }

  


  virtual ~LibHandle();

  




  virtual void *GetSymbolPtr(const char *symbol) const = 0;

  



  virtual bool Contains(void *addr) const = 0;

  


  virtual void *GetBase() const = 0;

  


  const char *GetName() const;

  



  const char *GetPath() const
  {
    return path;
  }

  





  void AddDirectRef()
  {
    ++directRefCnt;
    mozilla::external::AtomicRefCounted<LibHandle>::AddRef();
  }

  



  bool ReleaseDirectRef()
  {
    bool ret = false;
    if (directRefCnt) {
      MOZ_ASSERT(directRefCnt <=
                 mozilla::external::AtomicRefCounted<LibHandle>::refCount());
      if (--directRefCnt)
        ret = true;
      mozilla::external::AtomicRefCounted<LibHandle>::Release();
    }
    return ret;
  }

  


  MozRefCountType DirectRefCount()
  {
    return directRefCnt;
  }

  



  size_t GetMappableLength() const;

  



  void *MappableMMap(void *addr, size_t length, off_t offset) const;

  



  void MappableMUnmap(void *addr, size_t length) const;

#ifdef __ARM_EABI__
  



  virtual const void *FindExidx(int *pcount) const = 0;
#endif

protected:
  


  virtual Mappable *GetMappable() const = 0;

  



  friend class ElfLoader;
  friend class CustomElf;
  friend class SEGVHandler;
  virtual bool IsSystemElf() const { return false; }

private:
  MozRefCountType directRefCnt;
  char *path;

  
  mutable mozilla::RefPtr<Mappable> mappable;
};









namespace mozilla {
namespace detail {

template <> inline void RefCounted<LibHandle, AtomicRefCount>::Release() const {
#ifdef DEBUG
  if (mRefCnt > 0x7fff0000)
    MOZ_ASSERT(mRefCnt > 0x7fffdead);
#endif
  MOZ_ASSERT(mRefCnt > 0);
  if (mRefCnt > 0) {
    if (0 == --mRefCnt) {
#ifdef DEBUG
      mRefCnt = 0x7fffdead;
#else
      mRefCnt = 1;
#endif
      delete static_cast<const LibHandle*>(this);
    }
  }
}

} 
} 




class SystemElf: public LibHandle
{
public:
  



  static mozilla::TemporaryRef<LibHandle> Load(const char *path, int flags);

  


  virtual ~SystemElf();
  virtual void *GetSymbolPtr(const char *symbol) const;
  virtual bool Contains(void *addr) const { return false;  }
  virtual void *GetBase() const { return nullptr;  }

#ifdef __ARM_EABI__
  virtual const void *FindExidx(int *pcount) const;
#endif

protected:
  virtual Mappable *GetMappable() const;

  



  friend class ElfLoader;
  virtual bool IsSystemElf() const { return true; }

  



  void Forget()
  {
    dlhandle = nullptr;
  }

private:
  


  SystemElf(const char *path, void *handle)
  : LibHandle(path), dlhandle(handle) { }

  
  void *dlhandle;
};







class SEGVHandler
{
public:
  bool hasRegisteredHandler() {
    if (! initialized)
      FinishInitialization();
    return registeredHandler;
  }

  bool isSignalHandlingBroken() {
    return signalHandlingBroken;
  }

protected:
  SEGVHandler();
  ~SEGVHandler();

private:
  static int __wrap_sigaction(int signum, const struct sigaction *act,
                              struct sigaction *oldact);

  



  void FinishInitialization();

  


  struct sigaction action;
  
  


  static void handler(int signum, siginfo_t *info, void *context);

  


  static void test_handler(int signum, siginfo_t *info, void *context);

  



  static const size_t stackSize = 12 * 1024;

  


  stack_t oldStack;

  



  MappedPtr stackPtr;

  bool initialized;
  bool registeredHandler;
  bool signalHandlingBroken;
  bool signalHandlingSlow;
};




class ElfLoader: public SEGVHandler
{
public:
  


  static ElfLoader Singleton;

  





  mozilla::TemporaryRef<LibHandle> Load(const char *path, int flags,
                                        LibHandle *parent = nullptr);

  





  mozilla::TemporaryRef<LibHandle> GetHandleByPtr(void *addr);

  





  static Mappable *GetMappableFromPath(const char *path);

protected:
  



  void Register(LibHandle *handle);

  



  void Forget(LibHandle *handle);

  
  friend class SystemElf;
  friend const char *__wrap_dlerror(void);
  friend void *__wrap_dlsym(void *handle, const char *symbol);
  friend int __wrap_dlclose(void *handle);
  const char *lastError;

private:
  ~ElfLoader();

  
  typedef std::vector<LibHandle *> LibHandleList;
  LibHandleList handles;

protected:
  friend class CustomElf;
  




  static void stats(const char *when);

  
  typedef void (*Destructor)(void *object);

  















#ifdef __ARM_EABI__
  static int __wrap_aeabi_atexit(void *that, Destructor destructor,
                                 void *dso_handle);
#else
  static int __wrap_cxa_atexit(Destructor destructor, void *that,
                               void *dso_handle);
#endif

  static void __wrap_cxa_finalize(void *dso_handle);

  



  class DestructorCaller {
  public:
    DestructorCaller(Destructor destructor, void *object, void *dso_handle)
    : destructor(destructor), object(object), dso_handle(dso_handle) { }

    



    void Call();

    


    bool IsForHandle(void *handle) const
    {
      return handle == dso_handle;
    }

  private:
    Destructor destructor;
    void *object;
    void *dso_handle;
  };

private:
  
  std::vector<DestructorCaller> destructors;

  
  class DebuggerHelper;
public:
  
  struct link_map {
    
    const void *l_addr;
    
    const char *l_name;
    
    const void *l_ld;

  private:
    friend class ElfLoader::DebuggerHelper;
    
    link_map *l_next, *l_prev;
  };

private:
  


  struct r_debug {
    
    int r_version;

    
    link_map *r_map;

    


    void (*r_brk)(void);

    

    enum {
      RT_CONSISTENT, 
      RT_ADD,        
      RT_DELETE      
    } r_state;
  };

  

  class DebuggerHelper
  {
  public:
    DebuggerHelper();

    operator bool()
    {
      return dbg;
    }

    
    void Add(link_map *map);

    
    void Remove(link_map *map);

    
    class iterator
    {
    public:
      const link_map *operator ->() const
      {
        return item;
      }

      const link_map &operator ++()
      {
        item = item->l_next;
        return *item;
      }

      bool operator<(const iterator &other) const
      {
        if (other.item == nullptr)
          return item ? true : false;
        MOZ_CRASH("DebuggerHelper::iterator::operator< called with something else than DebuggerHelper::end()");
      }
    protected:
      friend class DebuggerHelper;
      iterator(const link_map *item): item(item) { }

    private:
      const link_map *item;
    };

    iterator begin() const
    {
      return iterator(dbg ? dbg->r_map : nullptr);
    }

    iterator end() const
    {
      return iterator(nullptr);
    }

  private:
    r_debug *dbg;
    link_map *firstAdded;
  };
  friend int __wrap_dl_iterate_phdr(dl_phdr_cb callback, void *data);
  DebuggerHelper dbg;
};

#endif 
