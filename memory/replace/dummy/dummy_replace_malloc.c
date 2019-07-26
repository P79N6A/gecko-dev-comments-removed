



#include "mozilla/Types.h"





#define MALLOC_FUNCS MALLOC_FUNCS_ALL
#define MALLOC_DECL(name, ...) \
  MOZ_EXPORT void replace_ ## name() { }

#include "malloc_decls.h"
