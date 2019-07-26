




#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <limits>
#include <set>

#include "base/basictypes.h"
#include "base/eintr_wrapper.h"
#include "base/logging.h"
#include "base/platform_thread.h"
#include "base/process_util.h"
#include "base/scoped_ptr.h"
#include "base/sys_info.h"
#include "base/time.h"
#include "base/waitable_event.h"
#include "base/dir_reader_posix.h"

const int kMicrosecondsPerSecond = 1000000;

namespace base {

ProcessId GetCurrentProcId() {
  return getpid();
}

ProcessHandle GetCurrentProcessHandle() {
  return GetCurrentProcId();
}

bool OpenProcessHandle(ProcessId pid, ProcessHandle* handle) {
  
  
  *handle = pid;
  return true;
}

bool OpenPrivilegedProcessHandle(ProcessId pid, ProcessHandle* handle) {
  
  
  return OpenProcessHandle(pid, handle);
}

void CloseProcessHandle(ProcessHandle process) {
  
  return;
}

ProcessId GetProcId(ProcessHandle process) {
  return process;
}




bool KillProcess(ProcessHandle process_id, int exit_code, bool wait) {
  bool result = kill(process_id, SIGTERM) == 0;

  if (result && wait) {
    int tries = 60;
    
    while (tries-- > 0) {
      int pid = HANDLE_EINTR(waitpid(process_id, NULL, WNOHANG));
      if (pid == process_id)
        break;

      sleep(1);
    }

    result = kill(process_id, SIGKILL) == 0;
  }

  if (!result)
    DLOG(ERROR) << "Unable to terminate process.";

  return result;
}

#ifdef ANDROID
typedef unsigned long int rlim_t;
#endif


class ScopedDIRClose {
 public:
  inline void operator()(DIR* x) const {
    if (x) {
      closedir(x);
    }
  }
};
typedef scoped_ptr_malloc<DIR, ScopedDIRClose> ScopedDIR;


void CloseSuperfluousFds(const base::InjectiveMultimap& saved_mapping) {
  
  
#if defined(ANDROID)
  static const rlim_t kSystemDefaultMaxFds = 1024;
  static const char kFDDir[] = "/proc/self/fd";
#elif defined(OS_LINUX)
  static const rlim_t kSystemDefaultMaxFds = 8192;
  static const char kFDDir[] = "/proc/self/fd";
#elif defined(OS_MACOSX)
  static const rlim_t kSystemDefaultMaxFds = 256;
  static const char kFDDir[] = "/dev/fd";
#elif defined(OS_BSD)
  
  static const rlim_t kSystemDefaultMaxFds = 1024;
  
  static const char kFDDir[] = "/dev/fd";
#endif

  
  struct rlimit nofile;
  rlim_t max_fds;
  if (getrlimit(RLIMIT_NOFILE, &nofile)) {
    
    max_fds = kSystemDefaultMaxFds;
    DLOG(ERROR) << "getrlimit(RLIMIT_NOFILE) failed: " << errno;
  } else {
    max_fds = nofile.rlim_cur;
  }

  if (max_fds > INT_MAX)
    max_fds = INT_MAX;

  DirReaderPosix fd_dir(kFDDir);

  if (!fd_dir.IsValid()) {
    
    for (rlim_t i = 0; i < max_fds; ++i) {
      const int fd = static_cast<int>(i);
      if (fd == STDIN_FILENO || fd == STDOUT_FILENO || fd == STDERR_FILENO)
        continue;
      InjectiveMultimap::const_iterator j;
      for (j = saved_mapping.begin(); j != saved_mapping.end(); j++) {
        if (fd == j->dest)
          break;
      }
      if (j != saved_mapping.end())
        continue;

      
      
      HANDLE_EINTR(close(fd));
    }
    return;
  }

  const int dir_fd = fd_dir.fd();

  for ( ; fd_dir.Next(); ) {
    
    if (fd_dir.name()[0] == '.')
      continue;

    char *endptr;
    errno = 0;
    const long int fd = strtol(fd_dir.name(), &endptr, 10);
    if (fd_dir.name()[0] == 0 || *endptr || fd < 0 || errno)
      continue;
    if (fd == STDIN_FILENO || fd == STDOUT_FILENO || fd == STDERR_FILENO)
      continue;
    InjectiveMultimap::const_iterator i;
    for (i = saved_mapping.begin(); i != saved_mapping.end(); i++) {
      if (fd == i->dest)
        break;
    }
    if (i != saved_mapping.end())
      continue;
    if (fd == dir_fd)
      continue;

    
    
    
    
    if (fd < static_cast<int>(max_fds)) {
      int ret = HANDLE_EINTR(close(fd));
      if (ret != 0) {
        DLOG(ERROR) << "Problem closing fd";
      }
    }
  }
}





void SetAllFDsToCloseOnExec() {
#if defined(OS_LINUX)
  const char fd_dir[] = "/proc/self/fd";
#elif defined(OS_MACOSX) || defined(OS_BSD)
  const char fd_dir[] = "/dev/fd";
#endif
  ScopedDIR dir_closer(opendir(fd_dir));
  DIR *dir = dir_closer.get();
  if (NULL == dir) {
    DLOG(ERROR) << "Unable to open " << fd_dir;
    return;
  }

  struct dirent *ent;
  while ((ent = readdir(dir))) {
    
    if (ent->d_name[0] == '.')
      continue;
    int i = atoi(ent->d_name);
    
    if (i <= STDERR_FILENO)
      continue;

    int flags = fcntl(i, F_GETFD);
    if ((flags == -1) || (fcntl(i, F_SETFD, flags | FD_CLOEXEC) == -1)) {
      DLOG(ERROR) << "fcntl failure.";
    }
  }
}

ProcessMetrics::ProcessMetrics(ProcessHandle process) : process_(process),
                                                        last_time_(0),
                                                        last_system_time_(0) {
  processor_count_ = base::SysInfo::NumberOfProcessors();
}


ProcessMetrics* ProcessMetrics::CreateProcessMetrics(ProcessHandle process) {
  return new ProcessMetrics(process);
}

ProcessMetrics::~ProcessMetrics() { }

bool DidProcessCrash(bool* child_exited, ProcessHandle handle) {
  int status;
  const int result = HANDLE_EINTR(waitpid(handle, &status, WNOHANG));
  if (result == -1) {
    LOG(ERROR) << "waitpid failed pid:" << handle << " errno:" << errno;
    if (child_exited)
      *child_exited = false;
    return false;
  } else if (result == 0) {
    
    if (child_exited)
      *child_exited = false;
    return false;
  }

  if (child_exited)
    *child_exited = true;

  if (WIFSIGNALED(status)) {
    switch(WTERMSIG(status)) {
      case SIGSEGV:
      case SIGILL:
      case SIGABRT:
      case SIGFPE:
        return true;
      default:
        return false;
    }
  }

  if (WIFEXITED(status))
    return WEXITSTATUS(status) != 0;

  return false;
}

bool WaitForExitCode(ProcessHandle handle, int* exit_code) {
  int status;
  if (HANDLE_EINTR(waitpid(handle, &status, 0)) == -1) {
    NOTREACHED();
    return false;
  }

  if (WIFEXITED(status)) {
    *exit_code = WEXITSTATUS(status);
    return true;
  }

  
  DCHECK(WIFSIGNALED(status));
  return false;
}

namespace {

int64_t TimeValToMicroseconds(const struct timeval& tv) {
  return tv.tv_sec * kMicrosecondsPerSecond + tv.tv_usec;
}

}

int ProcessMetrics::GetCPUUsage() {
  struct timeval now;
  struct rusage usage;

  int retval = gettimeofday(&now, NULL);
  if (retval)
    return 0;
  retval = getrusage(RUSAGE_SELF, &usage);
  if (retval)
    return 0;

  int64_t system_time = (TimeValToMicroseconds(usage.ru_stime) +
                       TimeValToMicroseconds(usage.ru_utime)) /
                        processor_count_;
  int64_t time = TimeValToMicroseconds(now);

  if ((last_system_time_ == 0) || (last_time_ == 0)) {
    
    last_system_time_ = system_time;
    last_time_ = time;
    return 0;
  }

  int64_t system_time_delta = system_time - last_system_time_;
  int64_t time_delta = time - last_time_;
  DCHECK(time_delta != 0);
  if (time_delta == 0)
    return 0;

  
  int cpu = static_cast<int>((system_time_delta * 100 + time_delta / 2) /
                             time_delta);

  last_system_time_ = system_time;
  last_time_ = time;

  return cpu;
}

bool GetAppOutput(const CommandLine& cl, std::string* output) {
  int pipe_fd[2];
  pid_t pid;

  
  InjectiveMultimap fd_shuffle1, fd_shuffle2;
  fd_shuffle1.reserve(3);
  fd_shuffle2.reserve(3);
  const std::vector<std::string>& argv = cl.argv();
  scoped_array<char*> argv_cstr(new char*[argv.size() + 1]);

  if (pipe(pipe_fd) < 0)
    return false;

  switch (pid = fork()) {
    case -1:  
      close(pipe_fd[0]);
      close(pipe_fd[1]);
      return false;
    case 0:  
      {
        
        
        
        
        
        int dev_null = open("/dev/null", O_WRONLY);
        if (dev_null < 0)
          _exit(127);

        fd_shuffle1.push_back(InjectionArc(pipe_fd[1], STDOUT_FILENO, true));
        fd_shuffle1.push_back(InjectionArc(dev_null, STDERR_FILENO, true));
        fd_shuffle1.push_back(InjectionArc(dev_null, STDIN_FILENO, true));
        
        

        std::copy(fd_shuffle1.begin(), fd_shuffle1.end(),
                  std::back_inserter(fd_shuffle2));

        
        if (!ShuffleFileDescriptors(&fd_shuffle1))
          _exit(127);

        CloseSuperfluousFds(fd_shuffle2);

        for (size_t i = 0; i < argv.size(); i++)
          argv_cstr[i] = const_cast<char*>(argv[i].c_str());
        argv_cstr[argv.size()] = NULL;
        execvp(argv_cstr[0], argv_cstr.get());
        _exit(127);
      }
    default:  
      {
        
        
        
        close(pipe_fd[1]);

        int exit_code = EXIT_FAILURE;
        bool success = WaitForExitCode(pid, &exit_code);
        if (!success || exit_code != EXIT_SUCCESS) {
          close(pipe_fd[0]);
          return false;
        }

        char buffer[256];
        std::string buf_output;

        while (true) {
          ssize_t bytes_read =
              HANDLE_EINTR(read(pipe_fd[0], buffer, sizeof(buffer)));
          if (bytes_read <= 0)
            break;
          buf_output.append(buffer, bytes_read);
        }
        output->swap(buf_output);
        close(pipe_fd[0]);
        return true;
      }
  }
}

}  
