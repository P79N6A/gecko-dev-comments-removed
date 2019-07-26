




















































#ifndef BASE_SYMBOLIZE_H_
#define BASE_SYMBOLIZE_H_

#include "utilities.h"
#include "config.h"
#include "glog/logging.h"

#ifdef HAVE_SYMBOLIZE

#if defined(__ELF__)  
#include <elf.h>
#include <link.h>  


#ifndef ElfW
# if SIZEOF_VOID_P == 4
#  define ElfW(type) Elf32_##type
# elif SIZEOF_VOID_P == 8
#  define ElfW(type) Elf64_##type
# else
#  error "Unknown sizeof(void *)"
# endif
#endif

_START_GOOGLE_NAMESPACE_



bool GetSectionHeaderByName(int fd, const char *name, size_t name_len,
                            ElfW(Shdr) *out);

_END_GOOGLE_NAMESPACE_

#endif  

_START_GOOGLE_NAMESPACE_








typedef int (*SymbolizeCallback)(int fd, void *pc, char *out, size_t out_size,
                                 uint64 relocation);
void InstallSymbolizeCallback(SymbolizeCallback callback);

_END_GOOGLE_NAMESPACE_

#endif

_START_GOOGLE_NAMESPACE_





bool Symbolize(void *pc, char *out, int out_size);

_END_GOOGLE_NAMESPACE_

#endif  
