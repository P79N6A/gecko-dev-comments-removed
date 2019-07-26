



#ifndef ElfLoader_h
#define ElfLoader_h

#include <vector>
#include <dlfcn.h>
#include <signal.h>
#include "mozilla/RefPtr.h"
#include "Zip.h"
#include "Elfxx.h"




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

  sighandler_t __wrap_signal(int signum, sighandler_t handler);
  int __wrap_sigaction(int signum, const struct sigaction *act,
                       struct sigaction *oldact);

  struct dl_phdr_info {
    Elf::Addr dlpi_addr;
    const char *dlpi_name;
    const Elf::Phdr *dlpi_phdr;
    Elf::Half dlpi_phnum;
  };

  typedef int (*dl_phdr_cb)(struct dl_phdr_info *, size_t, void *);
  int __wrap_dl_iterate_phdr(dl_phdr_cb callback, void *data);
}







class LibHandle;

namespace mozilla {

template <> inline void RefCounted<LibHandle>::Release();

template <> inline RefCounted<LibHandle>::~RefCounted()
{
  MOZ_ASSERT(refCnt == 0x7fffdead);
}

} 





class LibHandle: public mozilla::RefCounted<LibHandle>
{
public:
  



  LibHandle(const char *path)
  : directRefCnt(0), path(path ? strdup(path) : NULL) { }

  


  virtual ~LibHandle();

  




  virtual void *GetSymbolPtr(const char *symbol) const = 0;

  



  virtual bool Contains(void *addr) const = 0;

  


  const char *GetName() const;

  



  const char *GetPath() const
  {
    return path;
  }

  





  void AddDirectRef()
  {
    ++directRefCnt;
    mozilla::RefCounted<LibHandle>::AddRef();
  }

  



  bool ReleaseDirectRef()
  {
    bool ret = false;
    if (directRefCnt) {
      MOZ_ASSERT(directRefCnt <= mozilla::RefCounted<LibHandle>::refCount());
      if (--directRefCnt)
        ret = true;
      mozilla::RefCounted<LibHandle>::Release();
    }
    return ret;
  }

  


  int DirectRefCount()
  {
    return directRefCnt;
  }

protected:
  



  friend class ElfLoader;
  friend class CustomElf;
  friend class SEGVHandler;
  virtual bool IsSystemElf() const { return false; }

private:
  int directRefCnt;
  char *path;
};









namespace mozilla {

template <> inline void RefCounted<LibHandle>::Release() {
#ifdef DEBUG
  if (refCnt > 0x7fff0000)
    MOZ_ASSERT(refCnt > 0x7fffdead);
#endif
  MOZ_ASSERT(refCnt > 0);
  if (refCnt > 0) {
    if (0 == --refCnt) {
#ifdef DEBUG
      refCnt = 0x7fffdead;
#else
      refCnt = 1;
#endif
      delete static_cast<LibHandle*>(this);
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

protected:
  



  friend class ElfLoader;
  virtual bool IsSystemElf() const { return true; }

  



  void Forget()
  {
    dlhandle = NULL;
  }

private:
  


  SystemElf(const char *path, void *handle)
  : LibHandle(path), dlhandle(handle) { }

  
  void *dlhandle;
};









class SEGVHandler
{
protected:
  SEGVHandler();
  ~SEGVHandler();

private:
  friend sighandler_t __wrap_signal(int signum, sighandler_t handler);
  friend int __wrap_sigaction(int signum, const struct sigaction *act,
                              struct sigaction *oldact);

  


  struct sigaction action;
  
  


  static void handler(int signum, siginfo_t *info, void *context);

  



  static const size_t stackSize = 12 * 1024;

  


  stack_t oldStack;

  



  MappedPtr stackPtr;
};




class ElfLoader: public SEGVHandler
{
public:
  


  static ElfLoader Singleton;

  





  mozilla::TemporaryRef<LibHandle> Load(const char *path, int flags,
                                        LibHandle *parent = NULL);

  





  mozilla::TemporaryRef<LibHandle> GetHandleByPtr(void *addr);

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
        if (other.item == NULL)
          return item ? true : false;
        MOZ_NOT_REACHED("DebuggerHelper::iterator::operator< called with something else than DebuggerHelper::end()");
      }
    protected:
      friend class DebuggerHelper;
      iterator(const link_map *item): item(item) { }

    private:
      const link_map *item;
    };

    iterator begin() const
    {
      return iterator(dbg ? dbg->r_map : NULL);
    }

    iterator end() const
    {
      return iterator(NULL);
    }

  private:
    r_debug *dbg;
    link_map *firstAdded;
  };
  friend int __wrap_dl_iterate_phdr(dl_phdr_cb callback, void *data);
  DebuggerHelper dbg;
};

#endif 
