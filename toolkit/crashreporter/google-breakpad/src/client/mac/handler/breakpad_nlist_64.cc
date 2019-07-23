



































































#ifdef __LP64__

#include <mach-o/nlist.h>
#include <mach-o/loader.h>
#include <mach-o/fat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include "breakpad_nlist_64.h"
#include <TargetConditionals.h>
#include <stdio.h>
#include <mach/mach.h>





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

int
__breakpad_fdnlist_64(int fd, breakpad_nlist *list, const char **symbolNames);





int
breakpad_nlist_64(const char *name,
                  breakpad_nlist *list,
                  const char **symbolNames) {
  int fd, n;

  fd = open(name, O_RDONLY, 0);
  if (fd < 0)
    return (-1);
  n = __breakpad_fdnlist_64(fd, list, symbolNames);
  (void)close(fd);
  return (n);
}



int
__breakpad_fdnlist_64(int fd, breakpad_nlist *list, const char **symbolNames) {
  register breakpad_nlist *p, *q;
  breakpad_nlist space[BUFSIZ/sizeof (breakpad_nlist)];

  const register char *s1, *s2;
  register int n, m;
  int maxlen, nreq;
  off_t sa;             
  off_t ss;             
  struct exec buf;
  unsigned  arch_offset = 0;

  maxlen = 500;
  for (q = list, nreq = 0;
       symbolNames[q-list] && symbolNames[q-list][0];
       q++, nreq++) {

    q->n_type = 0;
    q->n_value = 0;
    q->n_desc = 0;
    q->n_sect = 0;
    q->n_un.n_strx = 0;
  }

  if (read(fd, (char *)&buf, sizeof(buf)) != sizeof(buf) ||
      (N_BADMAG(buf) && *((long *)&buf) != MH_MAGIC &&
       NXSwapBigLongToHost(*((long *)&buf)) != FAT_MAGIC) &&
      
      (*((uint32_t*)&buf)) != FAT_MAGIC) {
    return (-1);
  }

  
  if (NXSwapBigLongToHost(*((long *)&buf)) == FAT_MAGIC ||
      
      *((int*)&buf) == FAT_MAGIC) {
    struct host_basic_info hbi;
    struct fat_header fh;
    struct fat_arch *fat_archs, *fap;
    unsigned i;
    host_t host;

    
    host = mach_host_self();
    i = HOST_BASIC_INFO_COUNT;
    kern_return_t kr;
    if ((kr=host_info(host, HOST_BASIC_INFO,
                      (host_info_t)(&hbi), &i)) != KERN_SUCCESS) {
      return (-1);
    }
    mach_port_deallocate(mach_task_self(), host);

    
    lseek(fd, 0, SEEK_SET);
    if (read(fd, (char *)&fh, sizeof(fh)) != sizeof(fh)) {
      return (-1);
    }

    
    fh.nfat_arch = NXSwapBigLongToHost(fh.nfat_arch);

    
    fat_archs = (struct fat_arch *)malloc(fh.nfat_arch *
                                          sizeof(struct fat_arch));
    if (fat_archs == NULL) {
      return (-1);
    }
    if (read(fd, (char *)fat_archs,
             sizeof(struct fat_arch) * fh.nfat_arch) !=
        sizeof(struct fat_arch) * fh.nfat_arch) {
      free(fat_archs);
      return (-1);
    }

    



    for (i = 0; i < fh.nfat_arch; i++) {
      fat_archs[i].cputype =
        NXSwapBigLongToHost(fat_archs[i].cputype);
      fat_archs[i].cpusubtype =
        NXSwapBigLongToHost(fat_archs[i].cpusubtype);
      fat_archs[i].offset =
        NXSwapBigLongToHost(fat_archs[i].offset);
      fat_archs[i].size =
        NXSwapBigLongToHost(fat_archs[i].size);
      fat_archs[i].align =
        NXSwapBigLongToHost(fat_archs[i].align);
    }

    fap = NULL;
    for (i = 0; i < fh.nfat_arch; i++) {
      
      
      
      
      
#if TARGET_CPU_X86_64
      if (fat_archs[i].cputype == CPU_TYPE_X86_64) {
#elif TARGET_CPU_PPC64
      if (fat_archs[i].cputype == CPU_TYPE_POWERPC64) {
#else
#error undefined cpu!
        {
#endif
          fap = &fat_archs[i];
          break;
        }
      }

      if (!fap) {
        free(fat_archs);
        return (-1);
      }
      arch_offset = fap->offset;
      free(fat_archs);

      
      lseek(fd, arch_offset, SEEK_SET);
      if (read(fd, (char *)&buf, sizeof(buf)) != sizeof(buf)) {
        return (-1);
      }
    }

    if (*((int *)&buf) == MH_MAGIC_64) {
      struct mach_header_64 mh;
      struct load_command *load_commands, *lcp;
      struct symtab_command *stp;
      long i;

      lseek(fd, arch_offset, SEEK_SET);
      if (read(fd, (char *)&mh, sizeof(mh)) != sizeof(mh)) {
        return (-1);
      }
      load_commands = (struct load_command *)malloc(mh.sizeofcmds);
      if (load_commands == NULL) {
        return (-1);
      }
      if (read(fd, (char *)load_commands, mh.sizeofcmds) !=
          mh.sizeofcmds) {
        free(load_commands);
        return (-1);
      }
      stp = NULL;
      lcp = load_commands;
      
      
      for (i = 0; i < mh.ncmds; i++) {
        if (lcp->cmdsize % sizeof(long) != 0 ||
            lcp->cmdsize <= 0 ||
            (char *)lcp + lcp->cmdsize >
            (char *)load_commands + mh.sizeofcmds) {
          free(load_commands);
          return (-1);
        }
        if (lcp->cmd == LC_SYMTAB) {
          if (lcp->cmdsize !=
              sizeof(struct symtab_command)) {
            free(load_commands);
            return (-1);
          }
          stp = (struct symtab_command *)lcp;
          break;
        }
        lcp = (struct load_command *)
          ((char *)lcp + lcp->cmdsize);
      }
      if (stp == NULL) {
        free(load_commands);
        return (-1);
      }
      
      sa = stp->symoff + arch_offset;
      
      ss = stp->stroff + arch_offset;
      
      
      n = stp->nsyms * sizeof(breakpad_nlist);
      free(load_commands);
    }
    else {
      sa = N_SYMOFF(buf) + arch_offset;
      ss = sa + buf.a_syms + arch_offset;
      n = buf.a_syms;
    }

    lseek(fd, sa, SEEK_SET);

    
    
    
    
    
    while (n) {
      long savpos;

      m = sizeof (space);
      if (n < m)
        m = n;
      if (read(fd, (char *)space, m) != m)
        break;
      n -= m;
      savpos = lseek(fd, 0, SEEK_CUR);
      for (q = space; (m -= sizeof(breakpad_nlist)) >= 0; q++) {
        char nambuf[BUFSIZ];

        if (q->n_un.n_strx == 0 || q->n_type & N_STAB)
          continue;

        
        
        lseek(fd, ss+q->n_un.n_strx, SEEK_SET);
        read(fd, nambuf, maxlen+1);
        s2 = nambuf;
        for (p = list; 
             symbolNames[p-list] && 
               symbolNames[p-list][0]; 
             p++) {
          
          
          s1 = symbolNames[p - list];
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
            return (nreq);

          break;
        cont:           ;
        }
      }
      lseek(fd, savpos, SEEK_SET);
    }
    return (nreq);
  }

#endif
