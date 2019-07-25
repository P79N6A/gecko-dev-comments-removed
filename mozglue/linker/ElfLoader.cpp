



#include <cstring>
#include <cstdlib>
#include <dlfcn.h>
#include <unistd.h>
#include <algorithm>
#include <fcntl.h>
#include "ElfLoader.h"
#include "CustomElf.h"
#include "Mappable.h"
#include "Logging.h"

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
  return path ? LeafName(path) : NULL;
}




TemporaryRef<LibHandle>
SystemElf::Load(const char *path, int flags)
{
  

  if (path && path[0] == '/' && (access(path, F_OK) == -1)){
    debug("dlopen(\"%s\", %x) = %p", path, flags, (void *)NULL);
    return NULL;
  }

  void *handle = dlopen(path, flags);
  debug("dlopen(\"%s\", %x) = %p", path, flags, handle);
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
  const char *requested_path = path;

  


  if ((name == path) && parent) {
    const char *parentPath = parent->GetPath();
    abs_path = new char[strlen(parentPath) + strlen(path)];
    strcpy(abs_path, parentPath);
    char *slash = strrchr(abs_path, '/');
    strcpy(slash + 1, path);
    path = abs_path;
  }

  



  Mappable *mappable = NULL;
  RefPtr<Zip> zip;
  const char *subpath;
  if ((subpath = strchr(path, '!'))) {
    char *zip_path = strndup(path, subpath - path);
    while (*(++subpath) == '/') { }
    zip = zips.GetZip(zip_path);
    Zip::Stream s;
    if (zip && zip->GetStream(subpath, &s)) {
      if (s.GetType() == Zip::Stream::DEFLATE) {
        



        const char *extract = getenv("MOZ_LINKER_EXTRACT");
        if (extract && !strncmp(extract, "1", 2 ))
          mappable = MappableExtractFile::Create(name, &s);
        
        if (!mappable)
          mappable = MappableDeflate::Create(name, zip, &s);
      }
    }
  }
  
  if (!mappable && !zip)
    mappable = MappableFile::Create(path);

  
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

void
ElfLoader::Register(LibHandle *handle)
{
  handles.push_back(handle);
  if (dbg && !handle->IsSystemElf())
    dbg->Add(static_cast<CustomElf *>(handle));
}

void
ElfLoader::Forget(LibHandle *handle)
{
  LibHandleList::iterator it = std::find(handles.begin(), handles.end(), handle);
  if (it != handles.end()) {
    debug("ElfLoader::Forget(%p [\"%s\"])", reinterpret_cast<void *>(handle),
                                            handle->GetPath());
    if (dbg && !handle->IsSystemElf())
      dbg->Remove(static_cast<CustomElf *>(handle));
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
        delete (*it);
      } else {
        debug("ElfLoader::~ElfLoader(): Unexpected remaining handle for \"%s\" "
              "[%d direct refs, %d refs total]", (*it)->GetPath(),
              (*it)->DirectRefCount(), (*it)->refCount());
        


      }
    }
  }
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

void
ElfLoader::InitDebugger()
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
  debug("DT_DEBUG points at %p", dbg);
}

















void
ElfLoader::r_debug::Add(ElfLoader::link_map *map)
{
  if (!r_brk)
    return;
  r_state = RT_ADD;
  r_brk();
  map->l_prev = NULL;
  map->l_next = r_map;
  r_map->l_prev = map;
  r_map = map;
  r_state = RT_CONSISTENT;
  r_brk();
}

void
ElfLoader::r_debug::Remove(ElfLoader::link_map *map)
{
  if (!r_brk)
    return;
  r_state = RT_DELETE;
  r_brk();
  if (r_map == map)
    r_map = map->l_next;
  else
    map->l_prev->l_next = map->l_next;
  map->l_next->l_prev = map->l_prev;
  r_state = RT_CONSISTENT;
  r_brk();
}
