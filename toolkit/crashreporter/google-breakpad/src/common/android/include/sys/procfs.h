




























#ifndef GOOGLE_BREAKPAD_COMMON_ANDROID_SYS_PROCFS_H
#define GOOGLE_BREAKPAD_COMMON_ANDROID_SYS_PROCFS_H

#ifdef __BIONIC_HAVE_SYS_PROCFS_H

#include_next <sys/procfs.h>

#else

#include <sys/cdefs.h>
#include <sys/user.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __x86_64__
typedef unsigned long long elf_greg_t;
#else
typedef unsigned long  elf_greg_t;
#endif

#ifdef __arm__
#define ELF_NGREG (sizeof(struct user_regs) / sizeof(elf_greg_t))
#else
#define ELF_NGREG (sizeof(struct user_regs_struct) / sizeof(elf_greg_t))
#endif

typedef elf_greg_t elf_gregset_t[ELF_NGREG];

struct elf_siginfo {
  int si_signo;
  int si_code;
  int si_errno;
};

struct elf_prstatus {
  struct elf_siginfo pr_info;
  short              pr_cursig;
  unsigned long      pr_sigpend;
  unsigned long      pr_sighold;
  pid_t              pr_pid;
  pid_t              pr_ppid;
  pid_t              pr_pgrp;
  pid_t              pd_sid;
  struct timeval     pr_utime;
  struct timeval     pr_stime;
  struct timeval     pr_cutime;
  struct timeval     pr_cstime;
  elf_gregset_t      pr_reg;
  int                pr_fpvalid;
};

#define ELF_PRARGSZ 80

struct elf_prpsinfo {
  char           pr_state;
  char           pr_sname;
  char           pr_zomb;
  char           pr_nice;
  unsigned long  pr_flags;
#ifdef __x86_64__
  unsigned int   pr_uid;
  unsigned int   pr_gid;
#else
  unsigned short pr_uid;
  unsigned short pr_gid;
#endif
  int pr_pid;
  int pr_ppid;
  int pr_pgrp;
  int pr_sid;
  char pr_fname[16];
  char pr_psargs[ELF_PRARGSZ];
};

#ifdef __cplusplus
}  
#endif  

#endif

#endif
