




#include "mozilla/Attributes.h"
#include "mozilla/Types.h"

#define EXPORT_CDECL(type)   MOZ_EXPORT type

MOZ_BEGIN_EXTERN_C

  EXPORT_CDECL(void) set_errno(int status);
  EXPORT_CDECL(int) get_errno();

#if defined(XP_WIN)
  EXPORT_CDECL(void) set_last_error(int status);
  EXPORT_CDECL(int) get_last_error();
#endif 

MOZ_END_EXTERN_C
