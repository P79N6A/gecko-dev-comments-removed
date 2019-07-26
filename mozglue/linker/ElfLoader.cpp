



#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <dlfcn.h>
#include <unistd.h>
#include <algorithm>
#include <fcntl.h>
#include "ElfLoader.h"
#include "CustomElf.h"
#include "Mappable.h"
#include "Logging.h"

#if defined(ANDROID)
#include <sys/syscall.h>

#include <android/api-level.h>
#if __ANDROID_API__ < 8


extern "C" {

inline int sigaltstack(const stack_t *ss, stack_t *oss) {
  return syscall(__NR_sigaltstack, ss, oss);
}

} 
#endif 
#endif 

using namespace mozilla;

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#ifndef PAGE_MASK
#define PAGE_MASK (~ (PAGE_SIZE - 1))
#endif





void *
__wrap_dlopen(const char *path, int flags)
{
  RefPtr<LibHandle> handle = ElfLoader::Singleton.Load(path, flags);
  if (handle)
    handle->AddDirectRef();
  return handle;
}

const char *
__wrap_dlerror(void)
{
  const char *error = ElfLoader::Singleton.lastError;
  ElfLoader::Singleton.lastError = NULL;
  return error;
}

void *
__wrap_dlsym(void *handle, const char *symbol)
{
  if (!handle) {
    ElfLoader::Singleton.lastError = "dlsym(NULL, sym) unsupported";
    return NULL;
  }
  if (handle != RTLD_DEFAULT && handle != RTLD_NEXT) {
    LibHandle *h = reinterpret_cast<LibHandle *>(handle);
    return h->GetSymbolPtr(symbol);
  }
  return dlsym(handle, symbol);
}

int
__wrap_dlclose(void *handle)
{
  if (!handle) {
    ElfLoader::Singleton.lastError = "No handle given to dlclose()";
    return -1;
  }
  reinterpret_cast<LibHandle *>(handle)->ReleaseDirectRef();
  return 0;
}

int
__wrap_dladdr(void *addr, Dl_info *info)
{
  RefPtr<LibHandle> handle = ElfLoader::Singleton.GetHandleByPtr(addr);
  if (!handle)
    return 0;
  info->dli_fname = handle->GetPath();
  return 1;
}

int
__wrap_dl_iterate_phdr(dl_phdr_cb callback, void *data)
{
  if (!ElfLoader::Singleton.dbg)
    return -1;

  for (ElfLoader::DebuggerHelper::iterator it = ElfLoader::Singleton.dbg.begin();
       it < ElfLoader::Singleton.dbg.end(); ++it) {
    dl_phdr_info info;
    info.dlpi_addr = reinterpret_cast<Elf::Addr>(it->l_addr);
    info.dlpi_name = it->l_name;
    info.dlpi_phdr = NULL;
    info.dlpi_phnum = 0;

    
    
    uint8_t mapped;
    
    if (!mincore(const_cast<void*>(it->l_addr), PAGE_SIZE, &mapped)) {
      const Elf::Ehdr *ehdr = Elf::Ehdr::validate(it->l_addr);
      if (ehdr) {
        info.dlpi_phdr = reinterpret_cast<const Elf::Phdr *>(
                         reinterpret_cast<const char *>(ehdr) + ehdr->e_phoff);
        info.dlpi_phnum = ehdr->e_phnum;
      }
    }

    int ret = callback(&info, sizeof(dl_phdr_info), data);
    if (ret)
      return ret;
  }
  return 0;
}





MFBT_API size_t
__dl_get_mappable_length(void *handle) {
  if (!handle)
    return 0;
  return reinterpret_cast<LibHandle *>(handle)->GetMappableLength();
}

MFBT_API void *
__dl_mmap(void *handle, void *addr, size_t length, off_t offset)
{
  if (!handle)
    return NULL;
  return reinterpret_cast<LibHandle *>(handle)->MappableMMap(addr, length,
                                                             offset);
}

MFBT_API void
__dl_munmap(void *handle, void *addr, size_t length)
{
  if (!handle)
    return;
  return reinterpret_cast<LibHandle *>(handle)->MappableMUnmap(addr, length);
}

namespace {




const char *
LeafName(const char *path)
{
  const char *lastSlash = strrchr(path, '/');
  if (lastSlash)
    return lastSlash + 1;
  return path;
}

} 




LibHandle::~LibHandle()
{
  free(path);
  if (mappable && mappable->GetKind() != Mappable::MAPPABLE_EXTRACT_FILE)
    delete mappable;
}

const char *
LibHandle::GetName() const
{
  return path ? LeafName(path) : NULL;
}

size_t
LibHandle::GetMappableLength() const
{
  MOZ_ASSERT(mappable != NULL, "GetMappableLength needs to be called first,"
                               " and only once");
  mappable = GetMappable();
  if (!mappable)
    return 0;
  return mappable->GetLength();
}

void *
LibHandle::MappableMMap(void *addr, size_t length, off_t offset) const
{
  MOZ_ASSERT(mappable == NULL, "MappableMMap must be called after"
                               " GetMappableLength");
  void* mapped = mappable->mmap(addr, length, PROT_READ, MAP_PRIVATE, offset);
  if (mapped != MAP_FAILED) {
    
    for (size_t off = 0; off < length; off += PAGE_SIZE) {
      mappable->ensure(reinterpret_cast<char *>(mapped) + off);
    }
  }
  return mapped;
}

void
LibHandle::MappableMUnmap(void *addr, size_t length) const
{
  MOZ_ASSERT(mappable == NULL, "MappableMUnmap must be called after"
                               " MappableMMap and GetMappableLength");
  mappable->munmap(addr, length);
}




TemporaryRef<LibHandle>
SystemElf::Load(const char *path, int flags)
{
  

  if (path && path[0] == '/' && (access(path, F_OK) == -1)){
    debug("dlopen(\"%s\", 0x%x) = %p", path, flags, (void *)NULL);
    return NULL;
  }

  void *handle = dlopen(path, flags);
  debug("dlopen(\"%s\", 0x%x) = %p", path, flags, handle);
  ElfLoader::Singleton.lastError = dlerror();
  if (handle) {
    SystemElf *elf = new SystemElf(path, handle);
    ElfLoader::Singleton.Register(elf);
    return elf;
  }
  return NULL;
}

SystemElf::~SystemElf()
{
  if (!dlhandle)
    return;
  debug("dlclose(%p [\"%s\"])", dlhandle, GetPath());
  dlclose(dlhandle);
  ElfLoader::Singleton.lastError = dlerror();
  ElfLoader::Singleton.Forget(this);
}

void *
SystemElf::GetSymbolPtr(const char *symbol) const
{
  void *sym = dlsym(dlhandle, symbol);
  debug("dlsym(%p [\"%s\"], \"%s\") = %p", dlhandle, GetPath(), symbol, sym);
  ElfLoader::Singleton.lastError = dlerror();
  return sym;
}

Mappable *
SystemElf::GetMappable() const
{
  const char *path = GetPath();
  if (!path)
    return NULL;
#ifdef ANDROID
  
  const char *name = LeafName(path);
  std::string systemPath;
  if (name == path) {
    systemPath = "/system/lib/";
    systemPath += path;
    path = systemPath.c_str();
  }
#endif

  return MappableFile::Create(path);
}






ElfLoader ElfLoader::Singleton;

TemporaryRef<LibHandle>
ElfLoader::Load(const char *path, int flags, LibHandle *parent)
{
  RefPtr<LibHandle> handle;

  
  if (!path) {
    handle = SystemElf::Load(NULL, flags);
    return handle;
  }

  
  const char *name = LeafName(path);

  

  if (name == path) {
    for (LibHandleList::iterator it = handles.begin(); it < handles.end(); ++it)
      if ((*it)->GetName() && (strcmp((*it)->GetName(), name) == 0))
        return *it;
  } else {
    for (LibHandleList::iterator it = handles.begin(); it < handles.end(); ++it)
      if ((*it)->GetPath() && (strcmp((*it)->GetPath(), path) == 0))
        return *it;
  }

  char *abs_path = NULL;
#ifdef MOZ_DEBUG_LINKER
  const char *requested_path = path;
#endif

  


  if ((name == path) && parent) {
    const char *parentPath = parent->GetPath();
    abs_path = new char[strlen(parentPath) + strlen(path)];
    strcpy(abs_path, parentPath);
    char *slash = strrchr(abs_path, '/');
    strcpy(slash + 1, path);
    path = abs_path;
  }

  Mappable *mappable = GetMappableFromPath(path);

  
  if (mappable)
    handle = CustomElf::Load(mappable, path, flags);

  
  if (!handle)
    handle = SystemElf::Load(path, flags);

  

  if (!handle && abs_path)
    handle = SystemElf::Load(name, flags);

  delete [] abs_path;
  debug("ElfLoader::Load(\"%s\", 0x%x, %p [\"%s\"]) = %p", requested_path, flags,
        reinterpret_cast<void *>(parent), parent ? parent->GetPath() : "",
        static_cast<void *>(handle));

  return handle;
}

mozilla::TemporaryRef<LibHandle>
ElfLoader::GetHandleByPtr(void *addr)
{
  
  for (LibHandleList::iterator it = handles.begin(); it < handles.end(); ++it) {
    if ((*it)->Contains(addr))
      return *it;
  }
  return NULL;
}

Mappable *
ElfLoader::GetMappableFromPath(const char *path)
{
  const char *name = LeafName(path);
  Mappable *mappable = NULL;
  RefPtr<Zip> zip;
  const char *subpath;
  if ((subpath = strchr(path, '!'))) {
    char *zip_path = strndup(path, subpath - path);
    while (*(++subpath) == '/') { }
    zip = ZipCollection::GetZip(zip_path);
    Zip::Stream s;
    if (zip && zip->GetStream(subpath, &s)) {
      



      const char *extract = getenv("MOZ_LINKER_EXTRACT");
      if (extract && !strncmp(extract, "1", 2 ))
        mappable = MappableExtractFile::Create(name, zip, &s);
      if (!mappable) {
        if (s.GetType() == Zip::Stream::DEFLATE) {
          mappable = MappableDeflate::Create(name, zip, &s);
        } else if (s.GetType() == Zip::Stream::STORE) {
          mappable = MappableSeekableZStream::Create(name, zip, &s);
        }
      }
    }
  }
  
  if (!mappable && !zip)
    mappable = MappableFile::Create(path);

  return mappable;
}

void
ElfLoader::Register(LibHandle *handle)
{
  handles.push_back(handle);
  if (dbg && !handle->IsSystemElf())
    dbg.Add(static_cast<CustomElf *>(handle));
}

void
ElfLoader::Forget(LibHandle *handle)
{
  LibHandleList::iterator it = std::find(handles.begin(), handles.end(), handle);
  if (it != handles.end()) {
    debug("ElfLoader::Forget(%p [\"%s\"])", reinterpret_cast<void *>(handle),
                                            handle->GetPath());
    if (dbg && !handle->IsSystemElf())
      dbg.Remove(static_cast<CustomElf *>(handle));
    handles.erase(it);
  } else {
    debug("ElfLoader::Forget(%p [\"%s\"]): Handle not found",
          reinterpret_cast<void *>(handle), handle->GetPath());
  }
}

ElfLoader::~ElfLoader()
{
  LibHandleList list;
  




  for (LibHandleList::reverse_iterator it = handles.rbegin();
       it < handles.rend(); ++it) {
    if ((*it)->DirectRefCount()) {
      if ((*it)->IsSystemElf()) {
        static_cast<SystemElf *>(*it)->Forget();
      } else {
        list.push_back(*it);
      }
    }
  }
  
  for (LibHandleList::iterator it = list.begin(); it < list.end(); ++it) {
    while ((*it)->ReleaseDirectRef()) { }
  }
  
  if (handles.size()) {
    list = handles;
    for (LibHandleList::reverse_iterator it = list.rbegin();
         it < list.rend(); ++it) {
      if ((*it)->IsSystemElf()) {
        debug("ElfLoader::~ElfLoader(): Remaining handle for \"%s\" "
              "[%d direct refs, %d refs total]", (*it)->GetPath(),
              (*it)->DirectRefCount(), (*it)->refCount());
      } else {
        debug("ElfLoader::~ElfLoader(): Unexpected remaining handle for \"%s\" "
              "[%d direct refs, %d refs total]", (*it)->GetPath(),
              (*it)->DirectRefCount(), (*it)->refCount());
        


      }
    }
  }
}

void
ElfLoader::stats(const char *when)
{
  for (LibHandleList::iterator it = Singleton.handles.begin();
       it < Singleton.handles.end(); ++it)
    if (!(*it)->IsSystemElf())
      static_cast<CustomElf *>(*it)->stats(when);
}

#ifdef __ARM_EABI__
int
ElfLoader::__wrap_aeabi_atexit(void *that, ElfLoader::Destructor destructor,
                               void *dso_handle)
{
  Singleton.destructors.push_back(
    DestructorCaller(destructor, that, dso_handle));
  return 0;
}
#else
int
ElfLoader::__wrap_cxa_atexit(ElfLoader::Destructor destructor, void *that,
                             void *dso_handle)
{
  Singleton.destructors.push_back(
    DestructorCaller(destructor, that, dso_handle));
  return 0;
}
#endif

void
ElfLoader::__wrap_cxa_finalize(void *dso_handle)
{
  

  std::vector<DestructorCaller>::reverse_iterator it;
  for (it = Singleton.destructors.rbegin();
       it < Singleton.destructors.rend(); ++it) {
    if (it->IsForHandle(dso_handle)) {
      it->Call();
    }
  }
}

void
ElfLoader::DestructorCaller::Call()
{
  if (destructor) {
    debug("ElfLoader::DestructorCaller::Call(%p, %p, %p)",
          FunctionPtr(destructor), object, dso_handle);
    destructor(object);
    destructor = NULL;
  }
}

ElfLoader::DebuggerHelper::DebuggerHelper(): dbg(NULL)
{
  


























  struct AuxVector {
    Elf::Addr type;
    Elf::Addr value;
  };

  
  extern char **environ;

  






  char **env;
  for (env = environ; *env; env++)
    if (*env + strlen(*env) + 1 == env[1])
      break;
  if (!*env)
    return;

  



  char **scan = reinterpret_cast<char **>(
                reinterpret_cast<uintptr_t>(*env) & ~(sizeof(void *) - 1));
  while (*env != *scan)
    scan--;

  

  while (*scan++);
  AuxVector *auxv = reinterpret_cast<AuxVector *>(scan);

  


  Array<Elf::Phdr> phdrs;
  char *base = NULL;
  while (auxv->type) {
    if (auxv->type == AT_PHDR) {
      phdrs.Init(reinterpret_cast<Elf::Phdr*>(auxv->value));
      
      base = reinterpret_cast<char *>(auxv->value & PAGE_MASK);
    }
    if (auxv->type == AT_PHNUM)
      phdrs.Init(auxv->value);
    auxv++;
  }

  if (!phdrs) {
    debug("Couldn't find program headers");
    return;
  }

  




  MappedPtr mem(mmap(base, PAGE_SIZE, PROT_NONE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0), PAGE_SIZE);
  if (mem == base) {
    
    int fd = open("/proc/self/exe", O_RDONLY);
    if (fd == -1) {
      debug("Failed to open /proc/self/exe");
      return;
    }
    mem.Assign(mmap(base, PAGE_SIZE, PROT_READ, MAP_PRIVATE, fd, 0), PAGE_SIZE);
    
    if (mem != base) {
      debug("Couldn't read program headers");
      return;
    }
  }
  

  if (!Elf::Ehdr::validate(base)) {
     debug("Couldn't find program base");
     return;
  }

  
  Array<Elf::Dyn> dyns;
  for (Array<Elf::Phdr>::iterator phdr = phdrs.begin(); phdr < phdrs.end();
       ++phdr) {
    




    if (phdr->p_type == PT_LOAD && phdr->p_offset == 0)
      base -= phdr->p_vaddr;
    if (phdr->p_type == PT_DYNAMIC)
      dyns.Init(base + phdr->p_vaddr, phdr->p_filesz);
  }
  if (!dyns) {
    debug("Failed to find PT_DYNAMIC section in program");
    return;
  }

  
  for (Array<Elf::Dyn>::iterator dyn = dyns.begin(); dyn < dyns.end(); ++dyn) {
    if (dyn->d_tag == DT_DEBUG) {
      dbg = reinterpret_cast<r_debug *>(dyn->d_un.d_ptr);
      break;
    }
  }
  debug("DT_DEBUG points at %p", static_cast<void *>(dbg));
}






class EnsureWritable
{
public:
  template <typename T>
  EnsureWritable(T *ptr, size_t length_ = sizeof(T))
  {
    MOZ_ASSERT(length_ < PAGE_SIZE);
    prot = -1;
    page = MAP_FAILED;

    uintptr_t firstPage = reinterpret_cast<uintptr_t>(ptr) & PAGE_MASK;
    length = (reinterpret_cast<uintptr_t>(ptr) + length_ - firstPage
              + PAGE_SIZE - 1) & PAGE_MASK;

    uintptr_t end;

    prot = getProt(firstPage, &end);
    if (prot == -1 || (firstPage + length) > end)
      MOZ_CRASH();

    if (prot & PROT_WRITE)
      return;

    page = reinterpret_cast<void *>(firstPage);
    mprotect(page, length, prot | PROT_WRITE);
  }

  ~EnsureWritable()
  {
    if (page != MAP_FAILED) {
      mprotect(page, length, prot);
}
  }

private:
  int getProt(uintptr_t addr, uintptr_t *end)
  {
    

    int result = 0;
    AutoCloseFILE f(fopen("/proc/self/maps", "r"));
    while (f) {
      unsigned long long startAddr, endAddr;
      char perms[5];
      if (fscanf(f, "%llx-%llx %4s %*1024[^\n] ", &startAddr, &endAddr, perms) != 3)
        return -1;
      if (addr < startAddr || addr >= endAddr)
        continue;
      if (perms[0] == 'r')
        result |= PROT_READ;
      else if (perms[0] != '-')
        return -1;
      if (perms[1] == 'w')
        result |= PROT_WRITE;
      else if (perms[1] != '-')
        return -1;
      if (perms[2] == 'x')
        result |= PROT_EXEC;
      else if (perms[2] != '-')
        return -1;
      *end = endAddr;
      return result;
    }
    return -1;
  }

  int prot;
  void *page;
  size_t length;
};

















void
ElfLoader::DebuggerHelper::Add(ElfLoader::link_map *map)
{
  if (!dbg->r_brk)
    return;
  dbg->r_state = r_debug::RT_ADD;
  dbg->r_brk();
  map->l_prev = NULL;
  map->l_next = dbg->r_map;
  if (!firstAdded) {
    firstAdded = map;
    

    EnsureWritable w(&dbg->r_map->l_prev);
    dbg->r_map->l_prev = map;
  } else
    dbg->r_map->l_prev = map;
  dbg->r_map = map;
  dbg->r_state = r_debug::RT_CONSISTENT;
  dbg->r_brk();
}

void
ElfLoader::DebuggerHelper::Remove(ElfLoader::link_map *map)
{
  if (!dbg->r_brk)
    return;
  dbg->r_state = r_debug::RT_DELETE;
  dbg->r_brk();
  if (dbg->r_map == map)
    dbg->r_map = map->l_next;
  else
    map->l_prev->l_next = map->l_next;
  if (map == firstAdded) {
    firstAdded = map->l_prev;
    

    EnsureWritable w(&map->l_next->l_prev);
    map->l_next->l_prev = map->l_prev;
  } else
    map->l_next->l_prev = map->l_prev;
  dbg->r_state = r_debug::RT_CONSISTENT;
  dbg->r_brk();
}

#if defined(ANDROID)















extern "C" int
sigaction(int signum, const struct sigaction *act,
          struct sigaction *oldact);




int
sys_sigaction(int signum, const struct sigaction *act,
              struct sigaction *oldact)
{
  return syscall(__NR_sigaction, signum, act, oldact);
}



template <typename T>
static bool
Divert(T func, T new_func)
{
  void *ptr = FunctionPtr(func);
  uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

#if defined(__i386__)
  
  EnsureWritable w(ptr, 5);
  *reinterpret_cast<unsigned char *>(addr) = 0xe9; 
  *reinterpret_cast<intptr_t *>(addr + 1) =
    reinterpret_cast<uintptr_t>(new_func) - addr - 5; 
  return true;
#elif defined(__arm__)
  const unsigned char trampoline[] = {
                            
    0x46, 0x04,             
    0x78, 0x47,             
    0x46, 0x04,             
                            
    0x04, 0xf0, 0x1f, 0xe5, 
                            
  };
  const unsigned char *start;
  if (addr & 0x01) {
    

    addr--;
    
    if (addr & 0x02)
      start = trampoline;
    else
      start = trampoline + 2;
  } else {
    
    start = trampoline + 6;
  }

  size_t len = sizeof(trampoline) - (start - trampoline);
  EnsureWritable w(reinterpret_cast<void *>(addr), len + sizeof(void *));
  memcpy(reinterpret_cast<void *>(addr), start, len);
  *reinterpret_cast<void **>(addr + len) = FunctionPtr(new_func);
  cacheflush(addr, addr + len + sizeof(void *), 0);
  return true;
#else
  return false;
#endif
}
#else
#define sys_sigaction sigaction
template <typename T>
static bool
Divert(T func, T new_func)
{
  return false;
}
#endif

SEGVHandler::SEGVHandler()
: registeredHandler(false)
{
  if (!Divert(sigaction, __wrap_sigaction))
    return;
  

  if (sigaltstack(NULL, &oldStack) == -1 || !oldStack.ss_sp ||
      oldStack.ss_size < stackSize) {
    stackPtr.Assign(mmap(NULL, stackSize, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0), stackSize);
    stack_t stack;
    stack.ss_sp = stackPtr;
    stack.ss_size = stackSize;
    stack.ss_flags = 0;
    sigaltstack(&stack, NULL);
  }
  

  struct sigaction action;
  action.sa_sigaction = &SEGVHandler::handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = SA_SIGINFO | SA_NODEFER | SA_ONSTACK;
  action.sa_restorer = NULL;
  registeredHandler = !sys_sigaction(SIGSEGV, &action, &this->action);
}

SEGVHandler::~SEGVHandler()
{
  
  sigaltstack(&oldStack, NULL);
  
  sys_sigaction(SIGSEGV, &this->action, NULL);
}


void SEGVHandler::handler(int signum, siginfo_t *info, void *context)
{
  
  debug("Caught segmentation fault @%p", info->si_addr);

  

  if (info->si_code == SEGV_ACCERR) {
    mozilla::RefPtr<LibHandle> handle =
      ElfLoader::Singleton.GetHandleByPtr(info->si_addr);
    if (handle && !handle->IsSystemElf()) {
      debug("Within the address space of a CustomElf");
      CustomElf *elf = static_cast<CustomElf *>(static_cast<LibHandle *>(handle));
      if (elf->mappable->ensure(info->si_addr))
        return;
    }
  }

  
  SEGVHandler &that = ElfLoader::Singleton;
  if (that.action.sa_flags & SA_SIGINFO) {
    debug("Redispatching to registered handler @%p",
          FunctionPtr(that.action.sa_sigaction));
    that.action.sa_sigaction(signum, info, context);
  } else if (that.action.sa_handler == SIG_DFL) {
    debug("Redispatching to default handler");
    
    sys_sigaction(signum, &that.action, NULL);
    raise(signum);
  } else if (that.action.sa_handler != SIG_IGN) {
    debug("Redispatching to registered handler @%p",
          FunctionPtr(that.action.sa_handler));
    that.action.sa_handler(signum);
  } else {
    debug("Ignoring");
  }
}

int
SEGVHandler::__wrap_sigaction(int signum, const struct sigaction *act,
                              struct sigaction *oldact)
{
  
  if (signum != SIGSEGV)
    return sys_sigaction(signum, act, oldact);

  SEGVHandler &that = ElfLoader::Singleton;
  if (oldact)
    *oldact = that.action;
  if (act)
    that.action = *act;
  return 0;
}
