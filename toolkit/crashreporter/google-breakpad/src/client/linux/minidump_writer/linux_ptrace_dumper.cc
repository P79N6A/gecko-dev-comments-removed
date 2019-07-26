





































#include "client/linux/minidump_writer/linux_ptrace_dumper.h"

#include <asm/ptrace.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "client/linux/minidump_writer/directory_reader.h"
#include "client/linux/minidump_writer/line_reader.h"
#include "common/linux/linux_libc_support.h"
#include "third_party/lss/linux_syscall_support.h"


static bool SuspendThread(pid_t pid) {
  
  errno = 0;
  if (sys_ptrace(PTRACE_ATTACH, pid, NULL, NULL) != 0 &&
      errno != 0) {
    return false;
  }
  while (sys_waitpid(pid, NULL, __WALL) < 0) {
    if (errno != EINTR) {
      sys_ptrace(PTRACE_DETACH, pid, NULL, NULL);
      return false;
    }
  }
#if defined(__i386) || defined(__x86_64)
  
  
  
  
  
  
  
  user_regs_struct regs;
  if (sys_ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1 ||
#if defined(__i386)
      !regs.esp
#elif defined(__x86_64)
      !regs.rsp
#endif
      ) {
    sys_ptrace(PTRACE_DETACH, pid, NULL, NULL);
    return false;
  }
#endif
  return true;
}


static bool ResumeThread(pid_t pid) {
  return sys_ptrace(PTRACE_DETACH, pid, NULL, NULL) >= 0;
}

namespace google_breakpad {

LinuxPtraceDumper::LinuxPtraceDumper(pid_t pid)
    : LinuxDumper(pid),
      threads_suspended_(false) {
}

bool LinuxPtraceDumper::BuildProcPath(char* path, pid_t pid,
                                      const char* node) const {
  if (!path || !node || pid <= 0)
    return false;

  size_t node_len = my_strlen(node);
  if (node_len == 0)
    return false;

  const unsigned pid_len = my_uint_len(pid);
  const size_t total_length = 6 + pid_len + 1 + node_len;
  if (total_length >= NAME_MAX)
    return false;

  my_memcpy(path, "/proc/", 6);
  my_uitos(path + 6, pid, pid_len);
  path[6 + pid_len] = '/';
  my_memcpy(path + 6 + pid_len + 1, node, node_len);
  path[total_length] = '\0';
  return true;
}

void LinuxPtraceDumper::CopyFromProcess(void* dest, pid_t child,
                                        const void* src, size_t length) {
  unsigned long tmp = 55;
  size_t done = 0;
  static const size_t word_size = sizeof(tmp);
  uint8_t* const local = (uint8_t*) dest;
  uint8_t* const remote = (uint8_t*) src;

  while (done < length) {
    const size_t l = (length - done > word_size) ? word_size : (length - done);
    if (sys_ptrace(PTRACE_PEEKDATA, child, remote + done, &tmp) == -1) {
      tmp = 0;
    }
    my_memcpy(local + done, &tmp, l);
    done += l;
  }
}





bool LinuxPtraceDumper::GetThreadInfoByIndex(size_t index, ThreadInfo* info) {
  if (index >= threads_.size())
    return false;

  pid_t tid = threads_[index];

  assert(info != NULL);
  char status_path[NAME_MAX];
  if (!BuildProcPath(status_path, tid, "status"))
    return false;

  const int fd = sys_open(status_path, O_RDONLY, 0);
  if (fd < 0)
    return false;

  LineReader* const line_reader = new(allocator_) LineReader(fd);
  const char* line;
  unsigned line_len;

  info->ppid = info->tgid = -1;

  while (line_reader->GetNextLine(&line, &line_len)) {
    if (my_strncmp("Tgid:\t", line, 6) == 0) {
      my_strtoui(&info->tgid, line + 6);
    } else if (my_strncmp("PPid:\t", line, 6) == 0) {
      my_strtoui(&info->ppid, line + 6);
    }

    line_reader->PopLine(line_len);
  }
  sys_close(fd);

  if (info->ppid == -1 || info->tgid == -1)
    return false;

  if (sys_ptrace(PTRACE_GETREGS, tid, NULL, &info->regs) == -1) {
    return false;
  }

  if (sys_ptrace(PTRACE_GETFPREGS, tid, NULL, &info->fpregs) == -1) {
    return false;
  }

#if defined(__i386)
  if (sys_ptrace(PTRACE_GETFPXREGS, tid, NULL, &info->fpxregs) == -1)
    return false;
#endif

#if defined(__i386) || defined(__x86_64)
  for (unsigned i = 0; i < ThreadInfo::kNumDebugRegisters; ++i) {
    if (sys_ptrace(
        PTRACE_PEEKUSER, tid,
        reinterpret_cast<void*> (offsetof(struct user,
                                          u_debugreg[0]) + i *
                                 sizeof(debugreg_t)),
        &info->dregs[i]) == -1) {
      return false;
    }
  }
#endif

  const uint8_t* stack_pointer;
#if defined(__i386)
  my_memcpy(&stack_pointer, &info->regs.esp, sizeof(info->regs.esp));
#elif defined(__x86_64)
  my_memcpy(&stack_pointer, &info->regs.rsp, sizeof(info->regs.rsp));
#elif defined(__ARM_EABI__)
  my_memcpy(&stack_pointer, &info->regs.ARM_sp, sizeof(info->regs.ARM_sp));
#else
#error "This code hasn't been ported to your platform yet."
#endif
  info->stack_pointer = reinterpret_cast<uintptr_t>(stack_pointer);

  return true;
}

bool LinuxPtraceDumper::IsPostMortem() const {
  return false;
}

bool LinuxPtraceDumper::ThreadsSuspend() {
  if (threads_suspended_)
    return true;
  for (size_t i = 0; i < threads_.size(); ++i) {
    if (!SuspendThread(threads_[i])) {
      
      
      
      my_memmove(&threads_[i], &threads_[i+1],
                 (threads_.size() - i - 1) * sizeof(threads_[i]));
      threads_.resize(threads_.size() - 1);
      --i;
    }
  }
  threads_suspended_ = true;
  return threads_.size() > 0;
}

bool LinuxPtraceDumper::ThreadsResume() {
  if (!threads_suspended_)
    return false;
  bool good = true;
  for (size_t i = 0; i < threads_.size(); ++i)
    good &= ResumeThread(threads_[i]);
  threads_suspended_ = false;
  return good;
}



bool LinuxPtraceDumper::EnumerateThreads() {
  char task_path[NAME_MAX];
  if (!BuildProcPath(task_path, pid_, "task"))
    return false;

  const int fd = sys_open(task_path, O_RDONLY | O_DIRECTORY, 0);
  if (fd < 0)
    return false;
  DirectoryReader* dir_reader = new(allocator_) DirectoryReader(fd);

  
  
  int last_tid = -1;
  const char* dent_name;
  while (dir_reader->GetNextEntry(&dent_name)) {
    if (my_strcmp(dent_name, ".") &&
        my_strcmp(dent_name, "..")) {
      int tid = 0;
      if (my_strtoui(&tid, dent_name) &&
          last_tid != tid) {
        last_tid = tid;
        threads_.push_back(tid);
      }
    }
    dir_reader->PopEntry();
  }

  sys_close(fd);
  return true;
}

}  
