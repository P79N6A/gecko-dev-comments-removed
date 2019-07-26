




























#ifndef GOOGLE_BREAKPAD_ANDROID_INCLUDE_LINK_H
#define GOOGLE_BREAKPAD_ANDROID_INCLUDE_LINK_H


#include <elf.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ElfW(type)      _ElfW (Elf, ELFSIZE, type)
#define _ElfW(e,w,t)    _ElfW_1 (e, w, _##t)
#define _ElfW_1(e,w,t)  e##w##t

struct r_debug {
  int              r_version;
  struct link_map* r_map;
  ElfW(Addr)       r_brk;
  enum {
    RT_CONSISTENT,
    RT_ADD,
    RT_DELETE }    r_state;
  ElfW(Addr)       r_ldbase;
};

struct link_map {
  ElfW(Addr)       l_addr;
  char*            l_name;
  ElfW(Dyn)*       l_ld;
  struct link_map* l_next;
  struct link_map* l_prev;
};

#ifdef __cplusplus
}  
#endif  

#endif
