






























#include <dirent.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/frame.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <functional>

#include "client/solaris/handler/solaris_lwp.h"
#include "common/solaris/message_output.h"

using namespace google_breakpad;


namespace {

uintptr_t stack_base_address = 0;
static const int HEADER_MAX = 2000;
static const int MAP_MAX = 1000;



struct AddressValidatingContext {
  uintptr_t address;
  bool is_mapped;

  AddressValidatingContext() : address(0UL), is_mapped(false) {
  }
};


static bool LocalAtoi(char *s, int *r) {
  assert(s != NULL);
  assert(r != NULL);
  char *endptr = NULL;
  int ret = strtol(s, &endptr, 10);
  if (endptr == s)
    return false;
  *r = ret;
  return true;
}



static bool AddressNotInModuleCallback(const ModuleInfo &module_info,
                                       void *context) {
  AddressValidatingContext *addr =
    reinterpret_cast<AddressValidatingContext *>(context);
  if (addr->is_mapped = ((module_info.start_addr > 0) &&
                         (addr->address >= module_info.start_addr) &&
                         (addr->address <= module_info.start_addr +
                          module_info.size))) {
    stack_base_address = module_info.start_addr + module_info.size;
  }

  return !addr->is_mapped;
}

static int IterateLwpAll(int pid,
                         CallbackParam<LwpidCallback> *callback_param) {
  char lwp_path[40];
  DIR *dir;
  int count = 0;

  snprintf(lwp_path, sizeof (lwp_path), "/proc/%d/lwp", (int)pid);
  if ((dir = opendir(lwp_path)) == NULL)
    return -1;

  struct dirent *entry = NULL;
  while ((entry = readdir(dir)) != NULL) {
    if ((strcmp(entry->d_name, ".") != 0) &&
        (strcmp(entry->d_name, "..") != 0)) {
      int lwpid = 0;
      int last_pid = 0;
      if (LocalAtoi(entry->d_name, &lwpid) && last_pid != lwpid) {
        last_pid = lwpid;
        ++count;
        if (callback_param &&
            !(callback_param->call_back)(lwpid, callback_param->context)) {
          break;
        }
      }
    }
  }

  closedir(dir);
  return count;
}

#if defined(__i386) && !defined(NO_FRAME_POINTER)
void *GetNextFrame(void **last_ebp) {
  void *sp = *last_ebp;
  if ((unsigned long)sp == (unsigned long)last_ebp)
    return NULL;
  if ((unsigned long)sp & (sizeof(void *) - 1))
    return NULL;
  if ((unsigned long)sp - (unsigned long)last_ebp > 100000)
    return NULL;
  return sp;
}
#elif defined(__sparc)
void *GetNextFrame(void *last_ebp) {
  return reinterpret_cast<struct frame *>(last_ebp)->fr_savfp;
}
#else
void *GetNextFrame(void **last_ebp) {
  return reinterpret_cast<void*>(last_ebp);
}
#endif


class AutoCloser {
 public:
  AutoCloser(int fd) : fd_(fd) {}
  ~AutoCloser() { if (fd_) close(fd_); }
 private:
  int fd_;
};



static bool ControlLwp(int lwpid, void *context) {
  
  if (lwpid != pthread_self()) {
    int ctlfd;
    char procname[PATH_MAX];
    bool suspend = *(bool *)context;

    
    snprintf(procname, sizeof (procname), "/proc/self/lwp/%d/lwpctl", lwpid);

    if ((ctlfd = open(procname, O_WRONLY|O_EXCL)) < 0) {
      print_message2(2, "failed to open %s in ControlLwp\n", procname);
      return false;
    }

    AutoCloser autocloser(ctlfd);

    long ctl[2];
    ctl[0] = suspend ? PCSTOP : PCRUN;
    ctl[1] = 0;
    if (write(ctlfd, ctl, sizeof (ctl)) != sizeof (ctl)) {
      print_message2(2, "failed in lwp %d\n", lwpid);
      return false;
    }
  }

  return true;
}






static bool read_lfile(int pid, const char *lname, prheader_t *lhp) {
  char lpath[PATH_MAX];
  struct stat statb;
  int fd;
  size_t size;

  snprintf(lpath, sizeof (lpath), "/proc/%d/%s", pid, lname);
  if ((fd = open(lpath, O_RDONLY)) < 0) {
    print_message2(2, "failed to open %s in read_lfile\n", lpath);
    return false;
  }

  AutoCloser autocloser(fd);

  if (fstat(fd, &statb) != 0)
    return false;

  size = statb.st_size;
  if ((size / sizeof (prheader_t)) + 32 > HEADER_MAX) {
    print_message1(2, "map size overflow\n");
    return false;
  }

  if (pread(fd, lhp, size, 0) <= sizeof (prheader_t))
    return false;

  return true;
}

}  

namespace google_breakpad {

SolarisLwp::SolarisLwp(int pid) : pid_(pid) {
}

SolarisLwp::~SolarisLwp() {
}

int SolarisLwp::ControlAllLwps(bool suspend) {
  CallbackParam<LwpidCallback> callback_param(ControlLwp, &suspend);
  return IterateLwpAll(pid_, &callback_param);
}

int SolarisLwp::GetLwpCount() const {
  return IterateLwpAll(pid_, NULL);
}

int SolarisLwp::Lwp_iter_all(int pid,
                             CallbackParam<LwpCallback> *callback_param) const {
  lwpstatus_t *Lsp;
  lwpstatus_t *sp;
  prheader_t lphp[HEADER_MAX];
  prheader_t lhp[HEADER_MAX];
  prheader_t *Lphp = lphp;
  prheader_t *Lhp = lhp;
  lwpsinfo_t *Lpsp;
  long nstat;
  long ninfo;
  int rv = 0;

  



  if (read_lfile(pid, "lstatus", Lhp) == NULL)
    return -1;
  if (read_lfile(pid, "lpsinfo", Lphp) == NULL) {
    return -1;
  }

  Lsp = (lwpstatus_t *)(uintptr_t)(Lhp + 1);
  Lpsp = (lwpsinfo_t *)(uintptr_t)(Lphp + 1);

  for (ninfo = Lphp->pr_nent; ninfo != 0; --ninfo) {
    if (Lpsp->pr_sname != 'Z') {
      sp = Lsp;
      Lsp = (lwpstatus_t *)((uintptr_t)Lsp + Lhp->pr_entsize);
    } else {
      sp = NULL;
    }
    if (callback_param &&
        !(callback_param->call_back)(sp, callback_param->context))
      break;
    ++rv;
    Lpsp = (lwpsinfo_t *)((uintptr_t)Lpsp + Lphp->pr_entsize);
  }

  return rv;
}

uintptr_t SolarisLwp::GetLwpStackBottom(uintptr_t current_esp) const {
  AddressValidatingContext addr;
  addr.address = current_esp;
  CallbackParam<ModuleCallback> callback_param(AddressNotInModuleCallback,
                                               &addr);
  ListModules(&callback_param);
  return stack_base_address;
}

int SolarisLwp::GetModuleCount() const {
  return ListModules(NULL);
}

int SolarisLwp::ListModules(
    CallbackParam<ModuleCallback> *callback_param) const {
  const char *maps_path = "/proc/self/map";
  struct stat status;
  int fd = 0, num;
  prmap_t map_array[MAP_MAX];
  prmap_t *maps = map_array;
  size_t size;

  if ((fd = open(maps_path, O_RDONLY)) == -1) {
    print_message2(2, "failed to open %s in ListModules\n", maps_path);
    return -1;
  }

  AutoCloser autocloser(fd);

  if (fstat(fd, &status))
    return -1;

  



  size = status.st_size;
  if ((num = (int)(size / sizeof (prmap_t))) > MAP_MAX) {
    print_message1(2, "map size overflow\n");
    return -1;
  }

  if (read(fd, (void *)maps, size) < 0) {
    print_message2(2, "failed to read %d\n", fd);
    return -1;
  }

  prmap_t *_maps;
  int _num;
  int module_count = 0;
  
  







  for (_num = 0, _maps = maps; _num < num; ++_num, ++_maps) {
    ModuleInfo module;
    char *name = _maps->pr_mapname;

    memset(&module, 0, sizeof (module));
    module.start_addr = _maps->pr_vaddr;
    module.size = _maps->pr_size;
    if (strlen(name) > 0) {
      int objectfd = 0;
      char path[PATH_MAX];
      char buf[SELFMAG];

      snprintf(path, sizeof (path), "/proc/self/object/%s", name);
      if ((objectfd = open(path, O_RDONLY)) < 0) {
        print_message1(2, "can't open module file\n");
        continue;
      }

      AutoCloser autocloser(objectfd);

      if (read(objectfd, buf, SELFMAG) != SELFMAG) {
        print_message1(2, "can't read module file\n");
        continue;
      }
      if (buf[0] != ELFMAG0 || buf[1] != ELFMAG1 ||
          buf[2] != ELFMAG2 || buf[3] != ELFMAG3) {
        continue;
      }

      strncpy(module.name, name, sizeof (module.name) - 1);
      ++module_count;
    }
    if (callback_param &&
        (!callback_param->call_back(module, callback_param->context))) {
      break;
    }
  }

  return module_count;
}




bool SolarisLwp::IsAddressMapped(uintptr_t address) const {
  AddressValidatingContext addr;
  addr.address = address;
  CallbackParam<ModuleCallback> callback_param(AddressNotInModuleCallback,
                                               &addr);
  ListModules(&callback_param);
  return addr.is_mapped;
}







bool SolarisLwp::FindSigContext(uintptr_t sighandler_ebp,
                                ucontext_t **sig_ctx) {
  uintptr_t previous_ebp;
  uintptr_t sig_ebp;
  const int MAX_STACK_DEPTH = 50;
  int depth_counter = 0;

  do {
#if TARGET_CPU_SPARC
    previous_ebp = reinterpret_cast<uintptr_t>(GetNextFrame(
                                  reinterpret_cast<void*>(sighandler_ebp)));
    *sig_ctx = reinterpret_cast<ucontext_t*>(sighandler_ebp + sizeof (struct frame));
    uintptr_t sig_esp = (*sig_ctx)->uc_mcontext.gregs[REG_O6];
    if (sig_esp < previous_ebp && sig_esp > sighandler_ebp)
      sig_ebp = (uintptr_t)(((struct frame *)sig_esp)->fr_savfp);

#elif TARGET_CPU_X86
    previous_ebp = reinterpret_cast<uintptr_t>(GetNextFrame(
                                  reinterpret_cast<void**>(sighandler_ebp)));
    *sig_ctx = reinterpret_cast<ucontext_t*>(sighandler_ebp + sizeof (struct frame) +
                                             3 * sizeof(uintptr_t));
    sig_ebp = (*sig_ctx)->uc_mcontext.gregs[EBP];
#endif
    sighandler_ebp = previous_ebp;
    depth_counter++;
  } while(previous_ebp != sig_ebp && sighandler_ebp != 0 &&
          IsAddressMapped(sighandler_ebp) && depth_counter < MAX_STACK_DEPTH);

  return previous_ebp == sig_ebp && previous_ebp != 0;
}

}  
