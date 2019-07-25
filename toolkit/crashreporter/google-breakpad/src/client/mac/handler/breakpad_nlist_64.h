
































#ifndef CLIENT_MAC_HANDLER_BREAKPAD_NLIST_H__

#include <mach/machine.h>

int breakpad_nlist(const char *name,
                   struct nlist *list,
                   const char **symbolNames,
                   cpu_type_t cpu_type);
int breakpad_nlist(const char *name,
                   struct nlist_64 *list,
                   const char **symbolNames,
                   cpu_type_t cpu_type);

#endif  
