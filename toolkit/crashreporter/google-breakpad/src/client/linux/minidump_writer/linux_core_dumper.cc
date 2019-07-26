































#include "client/linux/minidump_writer/linux_core_dumper.h"

#include <asm/ptrace.h>
#include <assert.h>
#include <elf.h>
#include <stdio.h>
#include <string.h>
#include <sys/procfs.h>

#include "common/linux/linux_libc_support.h"

namespace google_breakpad {

LinuxCoreDumper::LinuxCoreDumper(pid_t pid,
                                 const char* core_path,
                                 const char* procfs_path)
    : LinuxDumper(pid),
      core_path_(core_path),
      procfs_path_(procfs_path),
      thread_infos_(&allocator_, 8) {
  assert(core_path_);
}

bool LinuxCoreDumper::BuildProcPath(char* path, pid_t pid,
                                    const char* node) const {
  if (!path || !node)
    return false;

  size_t node_len = my_strlen(node);
  if (node_len == 0)
    return false;

  size_t procfs_path_len = my_strlen(procfs_path_);
  size_t total_length = procfs_path_len + 1 + node_len;
  if (total_length >= NAME_MAX)
    return false;

  memcpy(path, procfs_path_, procfs_path_len);
  path[procfs_path_len] = '/';
  memcpy(path + procfs_path_len + 1, node, node_len);
  path[total_length] = '\0';
  return true;
}

void LinuxCoreDumper::CopyFromProcess(void* dest, pid_t child,
                                      const void* src, size_t length) {
  ElfCoreDump::Addr virtual_address = reinterpret_cast<ElfCoreDump::Addr>(src);
  
  
  
  if (!core_.CopyData(dest, virtual_address, length)) {
    
    
    memset(dest, 0xab, length);
  }
}

bool LinuxCoreDumper::GetThreadInfoByIndex(size_t index, ThreadInfo* info) {
  if (index >= thread_infos_.size())
    return false;

  *info = thread_infos_[index];
  const uint8_t* stack_pointer;
#if defined(__i386)
  memcpy(&stack_pointer, &info->regs.esp, sizeof(info->regs.esp));
#elif defined(__x86_64)
  memcpy(&stack_pointer, &info->regs.rsp, sizeof(info->regs.rsp));
#elif defined(__ARM_EABI__)
  memcpy(&stack_pointer, &info->regs.ARM_sp, sizeof(info->regs.ARM_sp));
#else
#error "This code hasn't been ported to your platform yet."
#endif
  info->stack_pointer = reinterpret_cast<uintptr_t>(stack_pointer);
  return true;
}

bool LinuxCoreDumper::IsPostMortem() const {
  return true;
}

bool LinuxCoreDumper::ThreadsSuspend() {
  return true;
}

bool LinuxCoreDumper::ThreadsResume() {
  return true;
}

bool LinuxCoreDumper::EnumerateThreads() {
  if (!mapped_core_file_.Map(core_path_)) {
    fprintf(stderr, "Could not map core dump file into memory\n");
    return false;
  }

  core_.SetContent(mapped_core_file_.content());
  if (!core_.IsValid()) {
    fprintf(stderr, "Invalid core dump file\n");
    return false;
  }

  ElfCoreDump::Note note = core_.GetFirstNote();
  if (!note.IsValid()) {
    fprintf(stderr, "PT_NOTE section not found\n");
    return false;
  }

  bool first_thread = true;
  do {
    ElfCoreDump::Word type = note.GetType();
    MemoryRange name = note.GetName();
    MemoryRange description = note.GetDescription();

    if (type == 0 || name.IsEmpty() || description.IsEmpty()) {
      fprintf(stderr, "Could not found a valid PT_NOTE.\n");
      return false;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    switch (type) {
      case NT_PRSTATUS: {
        if (description.length() != sizeof(elf_prstatus)) {
          fprintf(stderr, "Found NT_PRSTATUS descriptor of unexpected size\n");
          return false;
        }

        const elf_prstatus* status =
            reinterpret_cast<const elf_prstatus*>(description.data());
        pid_t pid = status->pr_pid;
        ThreadInfo info;
        memset(&info, 0, sizeof(ThreadInfo));
        info.tgid = status->pr_pgrp;
        info.ppid = status->pr_ppid;
        memcpy(&info.regs, status->pr_reg, sizeof(info.regs));
        if (first_thread) {
          crash_thread_ = pid;
          crash_signal_ = status->pr_info.si_signo;
        }
        first_thread = false;
        threads_.push_back(pid);
        thread_infos_.push_back(info);
        break;
      }
#if defined(__i386) || defined(__x86_64)
      case NT_FPREGSET: {
        if (thread_infos_.empty())
          return false;

        ThreadInfo* info = &thread_infos_.back();
        if (description.length() != sizeof(info->fpregs)) {
          fprintf(stderr, "Found NT_FPREGSET descriptor of unexpected size\n");
          return false;
        }

        memcpy(&info->fpregs, description.data(), sizeof(info->fpregs));
        break;
      }
#endif
#if defined(__i386)
      case NT_PRXFPREG: {
        if (thread_infos_.empty())
          return false;

        ThreadInfo* info = &thread_infos_.back();
        if (description.length() != sizeof(info->fpxregs)) {
          fprintf(stderr, "Found NT_PRXFPREG descriptor of unexpected size\n");
          return false;
        }

        memcpy(&info->fpxregs, description.data(), sizeof(info->fpxregs));
        break;
      }
#endif
    }
    note = note.GetNextNote();
  } while (note.IsValid());

  return true;
}

}  
