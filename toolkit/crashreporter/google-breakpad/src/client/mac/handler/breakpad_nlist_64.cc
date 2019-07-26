


































































#include "breakpad_nlist_64.h"

#include <CoreFoundation/CoreFoundation.h>
#include <fcntl.h>
#include <mach-o/nlist.h>
#include <mach-o/loader.h>
#include <mach-o/fat.h>
#include <mach/mach.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <TargetConditionals.h>
#include <unistd.h>





struct exec {
  unsigned short  a_machtype;     
  unsigned short  a_magic;        
  unsigned long a_text;         
  unsigned long a_data;         
  unsigned long a_bss;          
  unsigned long a_syms;         
  unsigned long a_entry;        
  unsigned long a_trsize;       
  unsigned long a_drsize;       
};

#define OMAGIC  0407            /* old impure format */
#define NMAGIC  0410            /* read-only text */
#define ZMAGIC  0413            /* demand load format */

#define N_BADMAG(x)                                                     \
  (((x).a_magic)!=OMAGIC && ((x).a_magic)!=NMAGIC && ((x).a_magic)!=ZMAGIC)
#define N_TXTOFF(x)                                     \
  ((x).a_magic==ZMAGIC ? 0 : sizeof (struct exec))
#define N_SYMOFF(x)                                                     \
  (N_TXTOFF(x) + (x).a_text+(x).a_data + (x).a_trsize+(x).a_drsize)



template<typename T>
struct MachBits {};

typedef struct nlist nlist32;
typedef struct nlist_64 nlist64;

template<>
struct MachBits<nlist32> {
  typedef mach_header mach_header_type;
  typedef uint32_t word_type;
  static const uint32_t magic = MH_MAGIC;
};

template<>
struct MachBits<nlist64> {
  typedef mach_header_64 mach_header_type;
  typedef uint64_t word_type;
  static const uint32_t magic = MH_MAGIC_64;
};

template<typename nlist_type>
int
__breakpad_fdnlist(int fd, nlist_type *list, const char **symbolNames,
                   cpu_type_t cpu_type);





template <typename nlist_type>
int breakpad_nlist_common(const char *name,
                          nlist_type *list,
                          const char **symbolNames,
                          cpu_type_t cpu_type) {
  int fd = open(name, O_RDONLY, 0);
  if (fd < 0)
    return -1;
  int n = __breakpad_fdnlist(fd, list, symbolNames, cpu_type);
  close(fd);
  return n;
}

int breakpad_nlist(const char *name,
                   struct nlist *list,
                   const char **symbolNames,
                   cpu_type_t cpu_type) {
  return breakpad_nlist_common(name, list, symbolNames, cpu_type);
}

int breakpad_nlist(const char *name,
                   struct nlist_64 *list,
                   const char **symbolNames,
                   cpu_type_t cpu_type) {
  return breakpad_nlist_common(name, list, symbolNames, cpu_type);
}



template<typename nlist_type>
int __breakpad_fdnlist(int fd, nlist_type *list, const char **symbolNames,
                       cpu_type_t cpu_type) {
  typedef typename MachBits<nlist_type>::mach_header_type mach_header_type;
  typedef typename MachBits<nlist_type>::word_type word_type;

  const uint32_t magic = MachBits<nlist_type>::magic;

  int maxlen = 500;
  int nreq = 0;
  for (nlist_type* q = list;
       symbolNames[q-list] && symbolNames[q-list][0];
       q++, nreq++) {

    q->n_type = 0;
    q->n_value = 0;
    q->n_desc = 0;
    q->n_sect = 0;
    q->n_un.n_strx = 0;
  }

  struct exec buf;
  if (read(fd, (char *)&buf, sizeof(buf)) != sizeof(buf) ||
      (N_BADMAG(buf) && *((uint32_t *)&buf) != magic &&
        CFSwapInt32BigToHost(*((uint32_t *)&buf)) != FAT_MAGIC &&
       
       (*((uint32_t*)&buf)) != FAT_MAGIC)) {
    return -1;
  }

  
  unsigned arch_offset = 0;
  if (CFSwapInt32BigToHost(*((uint32_t *)&buf)) == FAT_MAGIC ||
      
      *((unsigned int *)&buf) == FAT_MAGIC) {
    
    host_t host = mach_host_self();
    unsigned hic = HOST_BASIC_INFO_COUNT;
    struct host_basic_info hbi;
    kern_return_t kr;
    if ((kr = host_info(host, HOST_BASIC_INFO,
                        (host_info_t)(&hbi), &hic)) != KERN_SUCCESS) {
      return -1;
    }
    mach_port_deallocate(mach_task_self(), host);

    
    struct fat_header fh;
    if (lseek(fd, 0, SEEK_SET) == -1) {
      return -1;
    }
    if (read(fd, (char *)&fh, sizeof(fh)) != sizeof(fh)) {
      return -1;
    }

    
    fh.nfat_arch = CFSwapInt32BigToHost(fh.nfat_arch);

    
    struct fat_arch *fat_archs =
        (struct fat_arch *)malloc(fh.nfat_arch * sizeof(struct fat_arch));
    if (fat_archs == NULL) {
      return -1;
    }
    if (read(fd, (char *)fat_archs,
             sizeof(struct fat_arch) * fh.nfat_arch) !=
        (ssize_t)(sizeof(struct fat_arch) * fh.nfat_arch)) {
      free(fat_archs);
      return -1;
    }

    



    for (unsigned i = 0; i < fh.nfat_arch; i++) {
      fat_archs[i].cputype =
        CFSwapInt32BigToHost(fat_archs[i].cputype);
      fat_archs[i].cpusubtype =
        CFSwapInt32BigToHost(fat_archs[i].cpusubtype);
      fat_archs[i].offset =
        CFSwapInt32BigToHost(fat_archs[i].offset);
      fat_archs[i].size =
        CFSwapInt32BigToHost(fat_archs[i].size);
      fat_archs[i].align =
        CFSwapInt32BigToHost(fat_archs[i].align);
    }

    struct fat_arch *fap = NULL;
    for (unsigned i = 0; i < fh.nfat_arch; i++) {
      if (fat_archs[i].cputype == cpu_type) {
        fap = &fat_archs[i];
        break;
      }
    }

    if (!fap) {
      free(fat_archs);
      return -1;
    }
    arch_offset = fap->offset;
    free(fat_archs);

    
    if (lseek(fd, arch_offset, SEEK_SET) == -1) {
      return -1;
    }
    if (read(fd, (char *)&buf, sizeof(buf)) != sizeof(buf)) {
      return -1;
    }
  }

  off_t sa;  
  off_t ss;  
  register_t n;
  if (*((unsigned int *)&buf) == magic) {
    if (lseek(fd, arch_offset, SEEK_SET) == -1) {
      return -1;
    }
    mach_header_type mh;
    if (read(fd, (char *)&mh, sizeof(mh)) != sizeof(mh)) {
      return -1;
    }

    struct load_command *load_commands =
        (struct load_command *)malloc(mh.sizeofcmds);
    if (load_commands == NULL) {
      return -1;
    }
    if (read(fd, (char *)load_commands, mh.sizeofcmds) !=
        (ssize_t)mh.sizeofcmds) {
      free(load_commands);
      return -1;
    }
    struct symtab_command *stp = NULL;
    struct load_command *lcp = load_commands;
    
    
    for (uint32_t i = 0; i < mh.ncmds; i++) {
      if (lcp->cmdsize % sizeof(word_type) != 0 ||
          lcp->cmdsize <= 0 ||
          (char *)lcp + lcp->cmdsize >
          (char *)load_commands + mh.sizeofcmds) {
        free(load_commands);
        return -1;
      }
      if (lcp->cmd == LC_SYMTAB) {
        if (lcp->cmdsize !=
            sizeof(struct symtab_command)) {
          free(load_commands);
          return -1;
        }
        stp = (struct symtab_command *)lcp;
        break;
      }
      lcp = (struct load_command *)
        ((char *)lcp + lcp->cmdsize);
    }
    if (stp == NULL) {
      free(load_commands);
      return -1;
    }
    
    sa = stp->symoff + arch_offset;
    
    ss = stp->stroff + arch_offset;
    
    
    n = stp->nsyms * sizeof(nlist_type);
    free(load_commands);
  } else {
    sa = N_SYMOFF(buf) + arch_offset;
    ss = sa + buf.a_syms + arch_offset;
    n = buf.a_syms;
  }

  if (lseek(fd, sa, SEEK_SET) == -1) {
    return -1;
  }

  
  
  
  
  
  while (n) {
    nlist_type space[BUFSIZ/sizeof (nlist_type)];
    register_t m = sizeof (space);

    if (n < m)
      m = n;
    if (read(fd, (char *)space, m) != m)
      break;
    n -= m;
    off_t savpos = lseek(fd, 0, SEEK_CUR);
    if (savpos == -1) {
      return -1;
    }
    for (nlist_type* q = space; (m -= sizeof(nlist_type)) >= 0; q++) {
      char nambuf[BUFSIZ];

      if (q->n_un.n_strx == 0 || q->n_type & N_STAB)
        continue;

      
      
      if (lseek(fd, ss+q->n_un.n_strx, SEEK_SET) == -1) {
        return -1;
      }
      if (read(fd, nambuf, maxlen+1) == -1) {
        return -1;
      }
      const char *s2 = nambuf;
      for (nlist_type *p = list; 
           symbolNames[p-list] && symbolNames[p-list][0];
           p++) {
        
        
        const char *s1 = symbolNames[p - list];
        while (*s1) {
          if (*s1++ != *s2++)
            goto cont;
        }
        if (*s2)
          goto cont;

        p->n_value = q->n_value;
        p->n_type = q->n_type;
        p->n_desc = q->n_desc;
        p->n_sect = q->n_sect;
        p->n_un.n_strx = q->n_un.n_strx;
        if (--nreq == 0)
          return nreq;

        break;
      cont:           ;
      }
    }
    if (lseek(fd, savpos, SEEK_SET) == -1) {
      return -1;
    }
  }
  return nreq;
}
