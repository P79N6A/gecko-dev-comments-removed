



#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <dlfcn.h>
#include <unistd.h>
#include <algorithm>
#include <fcntl.h>
#include "ElfLoader.h"
#include "BaseElf.h"
#include "CustomElf.h"
#include "Mappable.h"
#include "Logging.h"
#include <inttypes.h>

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

#ifdef __ARM_EABI__
extern "C" MOZ_EXPORT const void *
__gnu_Unwind_Find_exidx(void *pc, int *pcount) __attribute__((weak));
#endif



extern "C" Elf::Dyn _DYNAMIC[];

using namespace mozilla;





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
  ElfLoader::Singleton.lastError = nullptr;
  return error;
}

void *
__wrap_dlsym(void *handle, const char *symbol)
{
  if (!handle) {
    ElfLoader::Singleton.lastError = "dlsym(NULL, sym) unsupported";
    return nullptr;
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
  if (!handle) {
    return dladdr(addr, info);
  }
  info->dli_fname = handle->GetPath();
  info->dli_fbase = handle->GetBase();
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
    info.dlpi_phdr = nullptr;
    info.dlpi_phnum = 0;

    
    
    uint8_t mapped;
    
    if (!mincore(const_cast<void*>(it->l_addr), PageSize(), &mapped)) {
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

#ifdef __ARM_EABI__
const void *
__wrap___gnu_Unwind_Find_exidx(void *pc, int *pcount)
{
  RefPtr<LibHandle> handle = ElfLoader::Singleton.GetHandleByPtr(pc);
  if (handle)
    return handle->FindExidx(pcount);
  if (__gnu_Unwind_Find_exidx)
    return __gnu_Unwind_Find_exidx(pc, pcount);
  *pcount = 0;
  return nullptr;
}
#endif





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
    return nullptr;
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

MFBT_API bool
IsSignalHandlingBroken()
{
  return ElfLoader::Singleton.isSignalHandlingBroken();
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
}

const char *
LibHandle::GetName() const
{
  return path ? LeafName(path) : nullptr;
}

size_t
LibHandle::GetMappableLength() const
{
  if (!mappable)
    mappable = GetMappable();
  if (!mappable)
    return 0;
  return mappable->GetLength();
}

void *
LibHandle::MappableMMap(void *addr, size_t length, off_t offset) const
{
  if (!mappable)
    mappable = GetMappable();
  if (!mappable)
    return MAP_FAILED;
  void* mapped = mappable->mmap(addr, length, PROT_READ, MAP_PRIVATE, offset);
  if (mapped != MAP_FAILED) {
    
    for (size_t off = 0; off < length; off += PageSize()) {
      mappable->ensure(reinterpret_cast<char *>(mapped) + off);
    }
  }
  return mapped;
}

void
LibHandle::MappableMUnmap(void *addr, size_t length) const
{
  if (mappable)
    mappable->munmap(addr, length);
}




TemporaryRef<LibHandle>
SystemElf::Load(const char *path, int flags)
{
  

  if (path && path[0] == '/' && (access(path, F_OK) == -1)){
    DEBUG_LOG("dlopen(\"%s\", 0x%x) = %p", path, flags, (void *)nullptr);
    return nullptr;
  }

  void *handle = dlopen(path, flags);
  DEBUG_LOG("dlopen(\"%s\", 0x%x) = %p", path, flags, handle);
  ElfLoader::Singleton.lastError = dlerror();
  if (handle) {
    SystemElf *elf = new SystemElf(path, handle);
    ElfLoader::Singleton.Register(elf);
    return elf;
  }
  return nullptr;
}

SystemElf::~SystemElf()
{
  if (!dlhandle)
    return;
  DEBUG_LOG("dlclose(%p [\"%s\"])", dlhandle, GetPath());
  dlclose(dlhandle);
  ElfLoader::Singleton.lastError = dlerror();
  ElfLoader::Singleton.Forget(this);
}

void *
SystemElf::GetSymbolPtr(const char *symbol) const
{
  void *sym = dlsym(dlhandle, symbol);
  DEBUG_LOG("dlsym(%p [\"%s\"], \"%s\") = %p", dlhandle, GetPath(), symbol, sym);
  ElfLoader::Singleton.lastError = dlerror();
  return sym;
}

Mappable *
SystemElf::GetMappable() const
{
  const char *path = GetPath();
  if (!path)
    return nullptr;
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

#ifdef __ARM_EABI__
const void *
SystemElf::FindExidx(int *pcount) const
{
  

  *pcount = 0;
  return nullptr;
}
#endif






ElfLoader ElfLoader::Singleton;

TemporaryRef<LibHandle>
ElfLoader::Load(const char *path, int flags, LibHandle *parent)
{
  
  Logging::Init();

  
  if (!self_elf)
    Init();

  RefPtr<LibHandle> handle;

  
  if (!path) {
    handle = SystemElf::Load(nullptr, flags);
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

  char *abs_path = nullptr;
  const char *requested_path = path;

  


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
  DEBUG_LOG("ElfLoader::Load(\"%s\", 0x%x, %p [\"%s\"]) = %p", requested_path, flags,
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
  return nullptr;
}

Mappable *
ElfLoader::GetMappableFromPath(const char *path)
{
  const char *name = LeafName(path);
  Mappable *mappable = nullptr;
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
}

void
ElfLoader::Register(CustomElf *handle)
{
  Register(static_cast<LibHandle *>(handle));
  if (dbg) {
    dbg.Add(handle);
  }
}

void
ElfLoader::Forget(LibHandle *handle)
{
  
  Logging::Init();

  LibHandleList::iterator it = std::find(handles.begin(), handles.end(), handle);
  if (it != handles.end()) {
    DEBUG_LOG("ElfLoader::Forget(%p [\"%s\"])", reinterpret_cast<void *>(handle),
                                                handle->GetPath());
    handles.erase(it);
  } else {
    DEBUG_LOG("ElfLoader::Forget(%p [\"%s\"]): Handle not found",
              reinterpret_cast<void *>(handle), handle->GetPath());
  }
}

void
ElfLoader::Forget(CustomElf *handle)
{
  Forget(static_cast<LibHandle *>(handle));
  if (dbg) {
    dbg.Remove(handle);
  }
}

void
ElfLoader::Init()
{
  Dl_info info;
  


  if (dladdr(_DYNAMIC, &info) != 0) {
    self_elf = LoadedElf::Create(info.dli_fname, info.dli_fbase);
  }
#if defined(ANDROID)
  if (dladdr(FunctionPtr(syscall), &info) != 0) {
    libc = LoadedElf::Create(info.dli_fname, info.dli_fbase);
  }
#endif
}

ElfLoader::~ElfLoader()
{
  LibHandleList list;

  if (!Singleton.IsShutdownExpected()) {
    MOZ_CRASH("Unexpected shutdown");
  }

  
  self_elf = nullptr;
#if defined(ANDROID)
  libc = nullptr;
#endif

  




  for (LibHandleList::reverse_iterator it = handles.rbegin();
       it < handles.rend(); ++it) {
    if ((*it)->DirectRefCount()) {
      if (SystemElf *se = (*it)->AsSystemElf()) {
        se->Forget();
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
      if ((*it)->AsSystemElf()) {
        DEBUG_LOG("ElfLoader::~ElfLoader(): Remaining handle for \"%s\" "
                  "[%d direct refs, %d refs total]", (*it)->GetPath(),
                  (*it)->DirectRefCount(), (*it)->refCount());
      } else {
        DEBUG_LOG("ElfLoader::~ElfLoader(): Unexpected remaining handle for \"%s\" "
                  "[%d direct refs, %d refs total]", (*it)->GetPath(),
                  (*it)->DirectRefCount(), (*it)->refCount());
        


      }
    }
  }
}

void
ElfLoader::stats(const char *when)
{
  if (MOZ_LIKELY(!Logging::isVerbose()))
    return;

  for (LibHandleList::iterator it = Singleton.handles.begin();
       it < Singleton.handles.end(); ++it)
    (*it)->stats(when);
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
    DEBUG_LOG("ElfLoader::DestructorCaller::Call(%p, %p, %p)",
              FunctionPtr(destructor), object, dso_handle);
    destructor(object);
    destructor = nullptr;
  }
}

ElfLoader::DebuggerHelper::DebuggerHelper(): dbg(nullptr), firstAdded(nullptr)
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

  
  while (!*scan)
    scan++;

  AuxVector *auxv = reinterpret_cast<AuxVector *>(scan);

  


  Array<Elf::Phdr> phdrs;
  char *base = nullptr;
  while (auxv->type) {
    if (auxv->type == AT_PHDR) {
      phdrs.Init(reinterpret_cast<Elf::Phdr*>(auxv->value));
      
      base = reinterpret_cast<char *>(PageAlignedPtr(auxv->value));
    }
    if (auxv->type == AT_PHNUM)
      phdrs.Init(auxv->value);
    auxv++;
  }

  if (!phdrs) {
    DEBUG_LOG("Couldn't find program headers");
    return;
  }

  




  MappedPtr mem(MemoryRange::mmap(base, PageSize(), PROT_NONE,
                                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  if (mem == base) {
    
    int fd = open("/proc/self/exe", O_RDONLY);
    if (fd == -1) {
      DEBUG_LOG("Failed to open /proc/self/exe");
      return;
    }
    mem.Assign(MemoryRange::mmap(base, PageSize(), PROT_READ, MAP_PRIVATE,
                                 fd, 0));
    
    if (mem != base) {
      DEBUG_LOG("Couldn't read program headers");
      return;
    }
  }
  

  if (!Elf::Ehdr::validate(base)) {
     DEBUG_LOG("Couldn't find program base");
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
    DEBUG_LOG("Failed to find PT_DYNAMIC section in program");
    return;
  }

  
  for (Array<Elf::Dyn>::iterator dyn = dyns.begin(); dyn < dyns.end(); ++dyn) {
    if (dyn->d_tag == DT_DEBUG) {
      dbg = reinterpret_cast<r_debug *>(dyn->d_un.d_ptr);
      break;
    }
  }
  DEBUG_LOG("DT_DEBUG points at %p", static_cast<void *>(dbg));
}






class EnsureWritable
{
public:
  template <typename T>
  EnsureWritable(T *ptr, size_t length_ = sizeof(T))
  {
    MOZ_ASSERT(length_ < PageSize());
    prot = -1;
    page = MAP_FAILED;

    char *firstPage = PageAlignedPtr(reinterpret_cast<char *>(ptr));
    char *lastPageEnd = PageAlignedEndPtr(reinterpret_cast<char *>(ptr) + length_);
    length = lastPageEnd - firstPage;
    uintptr_t start = reinterpret_cast<uintptr_t>(firstPage);
    uintptr_t end;

    prot = getProt(start, &end);
    if (prot == -1 || (start + length) > end)
      MOZ_CRASH();

    if (prot & PROT_WRITE)
      return;

    page = firstPage;
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
  map->l_prev = nullptr;
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
  else if (map->l_prev) {
    map->l_prev->l_next = map->l_next;
  }
  if (map == firstAdded) {
    firstAdded = map->l_prev;
    

    EnsureWritable w(&map->l_next->l_prev);
    map->l_next->l_prev = map->l_prev;
  } else if (map->l_next) {
    map->l_next->l_prev = map->l_prev;
  }
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

namespace {


static uint64_t ProcessTimeStamp_Now()
{
  struct timespec ts;
  int rv = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);

  if (rv != 0) {
    return 0;
  }

  uint64_t baseNs = (uint64_t)ts.tv_sec * 1000000000;
  return baseNs + (uint64_t)ts.tv_nsec;
}

}



struct TmpData {
  volatile int crash_int;
  volatile uint64_t crash_timestamp;
};

SEGVHandler::SEGVHandler()
: initialized(false), registeredHandler(false), signalHandlingBroken(true)
, signalHandlingSlow(true)
{
  



  Logging::Init();

  



  oldStack.ss_flags = SS_ONSTACK;

  
  struct sigaction old_action;
  sys_sigaction(SIGSEGV, nullptr, &old_action);

  








  struct sigaction action;
  action.sa_sigaction = &SEGVHandler::test_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = SA_SIGINFO | SA_NODEFER;
  action.sa_restorer = nullptr;
  stackPtr.Assign(MemoryRange::mmap(nullptr, PageSize(),
                                    PROT_READ | PROT_WRITE,
                                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  if (stackPtr.get() == MAP_FAILED)
    return;
  if (sys_sigaction(SIGSEGV, &action, nullptr))
    return;

  TmpData *data = reinterpret_cast<TmpData*>(stackPtr.get());
  data->crash_timestamp = ProcessTimeStamp_Now();
  mprotect(stackPtr, stackPtr.GetLength(), PROT_NONE);
  data->crash_int = 123;
  
  sys_sigaction(SIGSEGV, &old_action, nullptr);
  stackPtr.Assign(MAP_FAILED, 0);
}

void
SEGVHandler::FinishInitialization()
{
  

  initialized = true;

  if (signalHandlingBroken || signalHandlingSlow)
    return;

  typedef int (*sigaction_func)(int, const struct sigaction *,
                                struct sigaction *);

  sigaction_func libc_sigaction;

#if defined(ANDROID)
  

























  void *libc = dlopen("libc.so", RTLD_GLOBAL | RTLD_LAZY);
  if (libc) {
    




    libc_sigaction = reinterpret_cast<sigaction_func>(dlsym(libc, "__sigaction"));

    if (!libc_sigaction) {
      libc_sigaction =
        reinterpret_cast<sigaction_func>(dlsym(libc, "sigaction"));
    }
  } else
#endif
  {
    libc_sigaction = sigaction;
  }

  if (!Divert(libc_sigaction, __wrap_sigaction))
    return;

  

  if (sigaltstack(nullptr, &oldStack) == 0) {
    if (oldStack.ss_flags == SS_ONSTACK)
      oldStack.ss_flags = 0;
    if (!oldStack.ss_sp || oldStack.ss_size < stackSize) {
      stackPtr.Assign(MemoryRange::mmap(nullptr, stackSize,
                                        PROT_READ | PROT_WRITE,
                                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
      if (stackPtr.get() == MAP_FAILED)
        return;
      stack_t stack;
      stack.ss_sp = stackPtr;
      stack.ss_size = stackSize;
      stack.ss_flags = 0;
      if (sigaltstack(&stack, nullptr) != 0)
        return;
    }
  }
  

  action.sa_sigaction = &SEGVHandler::handler;
  action.sa_flags = SA_SIGINFO | SA_NODEFER | SA_ONSTACK;
  registeredHandler = !sys_sigaction(SIGSEGV, &action, &this->action);
}

SEGVHandler::~SEGVHandler()
{
  
  if (oldStack.ss_flags != SS_ONSTACK)
    sigaltstack(&oldStack, nullptr);
  
  if (registeredHandler)
    sys_sigaction(SIGSEGV, &this->action, nullptr);
}





void SEGVHandler::test_handler(int signum, siginfo_t *info, void *context)
{
  SEGVHandler &that = ElfLoader::Singleton;
  if (signum == SIGSEGV && info &&
      info->si_addr == that.stackPtr.get())
    that.signalHandlingBroken = false;
  mprotect(that.stackPtr, that.stackPtr.GetLength(), PROT_READ | PROT_WRITE);
  TmpData *data = reinterpret_cast<TmpData*>(that.stackPtr.get());
  uint64_t latency = ProcessTimeStamp_Now() - data->crash_timestamp;
  DEBUG_LOG("SEGVHandler latency: %" PRIu64, latency);
  


  if (latency <= 150000)
    that.signalHandlingSlow = false;
}


void SEGVHandler::handler(int signum, siginfo_t *info, void *context)
{
  
  DEBUG_LOG("Caught segmentation fault @%p", info->si_addr);

  

  if (info->si_code == SEGV_ACCERR) {
    mozilla::RefPtr<LibHandle> handle =
      ElfLoader::Singleton.GetHandleByPtr(info->si_addr);
    BaseElf *elf;
    if (handle && (elf = handle->AsBaseElf())) {
      DEBUG_LOG("Within the address space of %s", handle->GetPath());
      if (elf->mappable && elf->mappable->ensure(info->si_addr)) {
        return;
      }
    }
  }

  
  SEGVHandler &that = ElfLoader::Singleton;
  if (that.action.sa_flags & SA_SIGINFO) {
    DEBUG_LOG("Redispatching to registered handler @%p",
              FunctionPtr(that.action.sa_sigaction));
    that.action.sa_sigaction(signum, info, context);
  } else if (that.action.sa_handler == SIG_DFL) {
    DEBUG_LOG("Redispatching to default handler");
    
    sys_sigaction(signum, &that.action, nullptr);
    raise(signum);
  } else if (that.action.sa_handler != SIG_IGN) {
    DEBUG_LOG("Redispatching to registered handler @%p",
              FunctionPtr(that.action.sa_handler));
    that.action.sa_handler(signum);
  } else {
    DEBUG_LOG("Ignoring");
  }
}

int
SEGVHandler::__wrap_sigaction(int signum, const struct sigaction *act,
                              struct sigaction *oldact)
{
  SEGVHandler &that = ElfLoader::Singleton;

  
  if (!that.registeredHandler || (signum != SIGSEGV))
    return sys_sigaction(signum, act, oldact);

  if (oldact)
    *oldact = that.action;
  if (act)
    that.action = *act;
  return 0;
}

Logging Logging::Singleton;
