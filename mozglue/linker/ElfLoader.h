



#ifndef ElfLoader_h
#define ElfLoader_h

#include <vector>
#include <dlfcn.h>
#include "mozilla/RefPtr.h"
#include "Zip.h"




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
  virtual bool IsSystemElf() const { return false; }

private:
  int directRefCnt;
  char *path;
};




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




class ElfLoader
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
  ElfLoader() { InitDebugger(); }
  ~ElfLoader();

  
  typedef std::vector<LibHandle *> LibHandleList;
  LibHandleList handles;

protected:
  friend class CustomElf;
  
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

  
  ZipCollection zips;

public:
  
  struct link_map {
    
    const void *l_addr;
    
    const char *l_name;
    
    const void *l_ld;
    
    link_map *l_next, *l_prev;
  };

private:
  


  class r_debug {
  public:
    
    void Add(link_map *map);

    
    void Remove(link_map *map);

  private:
    
    int r_version;

    
    struct link_map *r_map;

    


    void (*r_brk)(void);

    

    enum {
      RT_CONSISTENT, 
      RT_ADD,        
      RT_DELETE      
    } r_state;
  };
  r_debug *dbg;

  


  void InitDebugger();
};

#endif 
